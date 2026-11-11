/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 Copyright (c) 2019-present Axmol Engine contributors (see AUTHORS.md).

 https://axmol.dev/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "MainScene.h"
#include "FishMotion.h"
#include "EnvironmentCycle.h"
#include "CharacterAnimation.h"
#include "CharacterAtlas.h"
#include "StoryAtlas.h"
#include "StoryContactAtlas.h"
#include "audio/AudioEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>

using namespace ax;
namespace {
constexpr float Contact = 168;
// More preview distance at top speed while keeping the character prominent.
constexpr float PixelsPerMeter = 9;
constexpr float HeightScale = 76;
constexpr float CrocScale = 1.28f;
const Color4F Ink(0.035f, 0.17f, 0.18f, 1);
const Color4F Lime(0.78f, 0.94f, 0.39f, 1);
const Color4B Cream(247, 243, 219, 255);
const Color4B Muted(168, 205, 185, 255);
const Color4B Dark(12, 49, 49, 255);
const char* EnvironmentFiles[] = {"art/mangue.png", "art/vila.png", "art/floresta.png"};
void ellipse(DrawNode* d, Vec2 p, float rx, float ry, Color4F color) {
    d->drawSolidCircle(p, 1, 0, 32, rx, ry, color);
}
void rect(DrawNode* d, Rect r, Color4F color) {
    d->drawSolidRect(r.origin, r.origin + Vec2(r.size.width, r.size.height), color);
}
void pill(DrawNode* d, Rect r, Color4F color) {
    const float radius = std::min({r.size.height / 2, r.size.width / 2, 18.0f});
    std::vector<Vec2> outline;
    const Vec2 centers[] = {{r.getMaxX()-radius,r.getMaxY()-radius},
                            {r.getMinX()+radius,r.getMaxY()-radius},
                            {r.getMinX()+radius,r.getMinY()+radius},
                            {r.getMaxX()-radius,r.getMinY()+radius}};
    for (int corner=0; corner<4; ++corner) {
        for (int step=0; step<=8; ++step) {
            const float angle=(corner*90.0f+step*90.0f/8)*3.14159265f/180;
            outline.push_back(centers[corner]+Vec2(std::cos(angle),std::sin(angle))*radius);
        }
    }
    // One convex surface: translucent corners never overlap the center.
    d->drawSolidPoly(outline.data(), static_cast<int>(outline.size()), color);
}
// Keep glyphs inside their allocated content box, including long balances/translations.
Label* fitLabel(Label* label,float width,float height) {
    const auto size=label->getContentSize();
    label->setScale(std::min({1.0f,width/std::max(1.0f,size.width),height/std::max(1.0f,size.height)}));
    return label;
}
std::string whole(double value) { return std::to_string(static_cast<long long>(value)); }
}

bool MainScene::init() {
    if (!Scene::init()) return false;
    viewHeight = Director::getInstance()->getVisibleSize().height;
    floorY = viewHeight * 0.35f;
    scenery=Node::create(); addChild(scenery);
    world = DrawNode::create(); scenery->addChild(world);
    menuMask=DrawNode::create();
    auto menuClip=ClippingNode::create(menuMask);
    menuClip->setAlphaThreshold(.5f);
    menuActor=DrawNode::create(); menuClip->addChild(menuActor); scenery->addChild(menuClip);
    panels = DrawNode::create(); addChild(panels, 1);
    ui = Node::create(); addChild(ui, 2);
    savePath = std::filesystem::u8path(FileUtils::getInstance()->getWritablePath()) / "jacarun.save";
#ifndef NDEBUG
    if (const char* directory = std::getenv("JACARUN_VISUAL_SMOKE"); directory && *directory) {
        smokeDirectory = std::filesystem::u8path(directory);
        std::filesystem::create_directories(smokeDirectory);
        savePath = smokeDirectory / "smoke-profile.save";
        smokeStep = 0; story.Skip();
    }
#endif
    std::string error;
    soundEnabled=UserDefault::getInstance()->getBoolForKey("sound-enabled",true);
    // Decode all backgrounds before the run, avoiding loading stalls at biome boundaries.
    for(const char* file:EnvironmentFiles) Director::getInstance()->getTextureCache()->addImage(file);
    for(const char* file:{"art/jaca-run-v2.png","art/jaca-idle-v2.png","art/jaca-actions-v2.png"})
        Director::getInstance()->getTextureCache()->addImage(file);
    AudioEngine::setMaxAudioInstance(8);
    if(soundEnabled && smokeStep<0) AudioEngine::preload("audio/story-score.wav");
    if (smokeStep < 0 && !game.GetProfile().Load(savePath, error)) {
        saveAllowed = false;
        saveWarning = "Save indisponivel. Original preservado.";
    }

    auto touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        auto p = t->getLocation();
        if (game.IsPlaying() && hit(p)==Action::Pause) {
            act(Action::Pause); return true;
        }
        if (game.IsPlaying()) return gestures.Begin(t->getID(), {p.x, p.y}, clock);
        if (activeTouch != -1) return false;
        activeTouch = t->getID();
        touchStart = p;
        touchAction = hit(p);
        return true;
    };
    touch->onTouchMoved = [this](Touch* t, Event*) {
        if (!game.IsPlaying()) return;
        auto p = t->getLocation();
        handleGesture(gestures.Move(t->getID(), {p.x, p.y}));
    };
    touch->onTouchEnded = [this](Touch* t, Event*) {
        auto p = t->getLocation();
        if (game.IsPlaying()) {
            handleGesture(gestures.End(t->getID(), {p.x, p.y}, clock));
            return;
        }
        if (t->getID() != activeTouch) return;
        activeTouch = -1;
        if (game.GetState()!=GameState::Menu && !shop && !goals && !help && !settings && p.y - touchStart.y > 35 && p.y - touchStart.y > std::abs(p.x-touchStart.x)*1.15f) {
            if (game.GetState() == GameState::Paused) act(Action::Pause);
            else act(Action::Play);
        } else if (touchAction != Action::None) {
            if (hit(p) == touchAction && p.distance(touchStart)<18) act(touchAction);
        } else if(game.GetState()==GameState::Menu && !story.active && !shop && !goals && !help && !settings && p.distance(touchStart)<18) {
            act(Action::Play);
        }
    };
    touch->onTouchCancelled = [this](Touch*, Event*) {
        activeTouch = -1; touchAction = Action::None; gestures.Cancel();
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
    auto keyboard = EventListenerKeyboard::create();
    keyboard->onKeyPressed = [this](EventKeyboard::KeyCode key, Event*) {
        using Key = EventKeyboard::KeyCode;
        if (key == Key::KEY_SPACE || key == Key::KEY_UP_ARROW || key == Key::KEY_W) act(Action::Jump);
        if (key == Key::KEY_DOWN_ARROW || key == Key::KEY_S) act(Action::Slide);
        if (key == Key::KEY_P || key == Key::KEY_ESCAPE || key == Key::KEY_BACK) {
            if (shop || goals || help || settings) act(Action::Back); else act(Action::Pause);
        }
        if (key == Key::KEY_ENTER) {
            if (game.GetState() == GameState::Paused) act(Action::Pause);
            else if (!game.IsPlaying()) act(Action::Play);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboard, this);
    rebuildUI();
    scheduleUpdate();
    return true;
}

Label* MainScene::text(const std::string& value, float size, Vec2 point, Color4B color, bool bold) {
    auto label = Label::createWithTTF(value, bold ? "fonts/Kanit-SemiBold.ttf" : "fonts/Kanit-Regular.ttf", size);
    label->setPosition(point);
    label->setTextColor(color);
    ui->addChild(label);
    return label;
}
void MainScene::button(const std::string& title, Rect bounds, Action action, bool primary, float fontSize) {
    pill(panels, {bounds.origin.x-2,bounds.origin.y-5,bounds.size.width+4,bounds.size.height+7}, Color4F(.16f,.075f,.025f,1));
    pill(panels, bounds, primary ? Color4F(1,.68f,.12f,1) : Color4F(.48f,.23f,.075f,1));
    panels->drawSegment({bounds.getMinX()+18,bounds.getMaxY()-4},{bounds.getMaxX()-18,bounds.getMaxY()-4},1.2f,Color4F(1,.83f,.36f,.65f));
    auto caption=text(title,fontSize,{bounds.getMidX(),bounds.getMidY()},primary?Dark:Cream,true);
    caption->setAlignment(TextHAlignment::CENTER);
    fitLabel(caption,bounds.size.width-32,bounds.size.height-20);
    buttons.push_back({bounds, action});
}
MainScene::Action MainScene::hit(Vec2 point) const {
    for (const auto& b : buttons) if (b.bounds.containsPoint(point)) return b.action;
    return Action::None;
}
void MainScene::handleGesture(Gesture gesture) {
    if (gesture == Gesture::Up) act(Action::Jump);
    else if (gesture == Gesture::Down) act(Action::Slide);
    else if (gesture == Gesture::Pause) act(Action::Pause);
}
void MainScene::start(bool procedural) {
    entrance.Finish();
    shop = goals = settings = false;
    motion.Reset(); particles.clear();
    dustTime = pickupPulse = impactTime = resultTime = toastTime = 0;
    shieldImpact = false;
    notifiedMissions = 0; resultPage = 0;
    coinNoticeTime = paceNoticeTime = 0; coinNoticeCount = 0; lastPace = -1;
    auto seed = static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    game.StartGame(seed, procedural);
    presentation.Reset();
    presentation.Sync(game.GetLevel().GetEntities());
    gestures.Cancel();
    runTime = 0;
    game.TakeMessages();
    feedback.clear();
    feedbackTime = 0;
    rebuildUI();
}
void MainScene::act(Action action) {
    if(story.active) {
        if(action==Action::SkipStory || action==Action::Play || action==Action::Pause || action==Action::Back) {
            story.Skip(); if(storyMusicId>=0) AudioEngine::stop(storyMusicId); storyMusicId=-1; chaseEntry=true; entrance.time=0; rebuildUI();
        }
        return;
    }
    switch (action) {
    case Action::Play:
        if(settings) break;
        if (!game.IsPlaying() && game.GetState() != GameState::Paused) {
            if(!help && smokeStep<0 && !UserDefault::getInstance()->getBoolForKey("controls-seen",false)) {
                help=true; rebuildUI();
            } else {
                if(help && smokeStep<0) {
                    UserDefault::getInstance()->setBoolForKey("controls-seen",true);
                    UserDefault::getInstance()->flush();
                }
                help=false; start();
            }
        }
        break;
    case Action::Settings: settings=true; shop=goals=help=false; rebuildUI(); break;
    case Action::Help: settings=false; help=true; rebuildUI(); break;
    case Action::Jump:
        if (game.Jump()) { burst({Contact-60, floorY+4}, Color4F(.80f,.78f,.49f,.8f), 7, .65f); sound("jump"); }
        break;
    case Action::Slide:
        if (game.Slide()) { burst({Contact-80, floorY+5}, Color4F(.80f,.78f,.49f,.8f), 9, .7f); sound("slide"); }
        break;
    case Action::Pause:
        if (game.IsPlaying() || game.GetState() == GameState::Paused) {
            game.TogglePause(); AudioEngine::stopAll(); musicId=-1; rebuildUI();
        }
        break;
    case Action::ReplayStory:
        if(game.GetState()==GameState::Menu) { shop=goals=help=settings=false; story=StoryIntro{}; rebuildUI(); }
        break;
    case Action::Menu: game.ReturnToMenu(); shop = goals = help = settings = false; save(); rebuildUI(); break;
    case Action::Shop:
        if (!game.IsPlaying() && game.GetState() != GameState::Paused) { shop = true; shopNotice="Escolha seu visual. Deixe sua marca!"; rebuildUI(); }
        break;
    case Action::Back: shop = goals = help = settings = false; rebuildUI(); break;
    case Action::Goals:
        if (!game.IsPlaying() && game.GetState() != GameState::Paused) {
            shop = false; goals = true; goalPage = 0; rebuildUI();
        }
        break;
    case Action::Missions: achievementTab=false; goalPage=0; rebuildUI(); break;
    case Action::Achievements: achievementTab=true; goalPage=0; rebuildUI(); break;
    case Action::Sound:
        soundEnabled=!soundEnabled;
        if(smokeStep<0) {
            UserDefault::getInstance()->setBoolForKey("sound-enabled",soundEnabled);
            UserDefault::getInstance()->flush();
        }
        if(!soundEnabled) { AudioEngine::stopAll(); musicId=-1; } else sound("coin");
        rebuildUI(); break;
    case Action::Looks: upgradeShop=false; shopPage=0; shopNotice="Visuais mudam o estilo do seu Jaca."; rebuildUI(); break;
    case Action::Upgrades: upgradeShop=true; shopPage=0; shopNotice="Melhorias permanentes para ir mais longe."; rebuildUI(); break;
    case Action::Previous:
        if(goals) goalPage=std::max(0,goalPage-1); else if(shop) shopPage=std::max(0,shopPage-1); else if(resultPage>0) --resultPage;
        rebuildUI(); break;
    case Action::Next:
        if(goals) goalPage=std::min(1,goalPage+1); else if(shop) shopPage=std::min((CosmeticCount-1)/3,shopPage+1); else if((resultPage+1)*2<game.GetCompletedGoals().size()) ++resultPage;
        rebuildUI(); break;
    case Action::Item0: case Action::Item1: case Action::Item2: {
        if (!shop) break;
        const int id = (upgradeShop ? CosmeticCount : shopPage*3) +
                       static_cast<int>(action) - static_cast<int>(Action::Item0);
        if(id>=ShopItemCount || (!upgradeShop && id>=CosmeticCount)) break;
        if (game.GetProfile().owned[id] || game.BuyAccessory(id)) {
            if(id<CosmeticCount) { game.EquipAccessory(id); shopNotice="Visual equipado!"; }
            else shopNotice="Melhoria permanente ativa nas suas corridas.";
        } else shopNotice="Faltam "+whole(AccessoryCatalog()[id].price-game.GetProfile().coins)+" moedas. Continue correndo!";
        save(); rebuildUI(); break;
    }
    default: break;
    }
}
void MainScene::save() {
    if (!saveAllowed) return;
    std::string error;
    if (!game.GetProfile().Save(savePath, error)) saveWarning = "Nao foi possivel salvar o progresso.";
    else saveWarning.clear();
}
void MainScene::suspend() {
    AudioEngine::stopAll(); musicId=-1;
    if (game.IsPlaying()) { game.TogglePause(); rebuildUI(); }
    activeTouch = -1;
    gestures.Cancel();
    save();
}

void MainScene::drawStory(float) {
    const int beat=story.Beat();
    const float h=viewHeight;
    const auto safe=Director::getInstance()->getSafeAreaRect();
    const float top=std::min(h-20.0f,safe.getMaxY()-16);
    const float bottom=std::max(24.0f,safe.getMinY()+18);
    // One edge-to-edge illustrated stage. UI stays outside the camera transform.
    storyStage=Node::create(); ui->addChild(storyStage);
    auto background=Sprite::create("art/story-creek-v1.png");
    storyBackground=background;
    background->setPosition({240,h*.5f});
    background->setScale(std::max(532.0f/1024,(h+70)/1536));
    storyStage->addChild(background,-10);
    auto actor=[&](const char* file,const AtlasCrop& c,float height,Vec2 base,bool flip,bool submerged) {
        Node* parent=storyStage;
        if(submerged) {
            auto mask=DrawNode::create();
            rect(mask,{-600,h*.42f,1680,h+500},Color4F(1,1,1,1));
            auto clip=ClippingNode::create(mask); clip->setAlphaThreshold(.5f);
            storyStage->addChild(clip,2); parent=clip;
        }
        auto root=Node::create(); root->setPosition(base); parent->addChild(root,2);
        auto sprite=Sprite::create(file,Rect(c.x,c.y,c.width,c.height));
        sprite->setAnchorPoint({.5f,0}); sprite->setScale(height/c.height);
        sprite->setFlippedX(flip); root->addChild(sprite); return root;
    };
    const float size=std::min(1.12f,h/900);
    const int hippoPose[]={0,1,1,2,1,3};
    const float hippoHeight[]={350,300,270,410,290,300};
    const float hippoX[]={222,47,-110,240,-90,75};
    storyHippoBase={hippoX[beat],h*(beat==5?.34f:.39f)};
    storyHippo=actor("art/bento-story-v1.png",BentoStoryCrops[hippoPose[beat]],hippoHeight[beat]*size,storyHippoBase,false,true);
    storyHippo->setVisible(beat==5);
    storyHippoSprite=static_cast<Sprite*>(storyHippo->getChildren().at(0));
    const int jacaPose[]={0,0,2,1,1,3};
    const float jacaHeight[]={250,265,340,200,330,245};
    const float jacaX[]={520,335,255,510,250,285};
    storyJacaBase={jacaX[beat],h*(beat==5?.47f:.405f)};
    storyJaca=actor("art/jaca-story-v1.png",JacaStoryCrops[jacaPose[beat]],jacaHeight[beat]*size,storyJacaBase,beat==2,beat!=5);
    storyJaca->setVisible(beat>=4);
    storyJacaSprite=static_cast<Sprite*>(storyJaca->getChildren().at(0));
    // Each contact is illustrated together: hands, mouth and fish cannot drift apart.
    storyContactBase={240,h*.405f};
    const int interaction=beat==0?0:beat==1?1:3;
    const auto& contactCrop=beat==2?JacaBiteCrops[0]:StoryInteractionCrops[interaction];
    const float contactHeight=beat==1?std::min(310*size,470.0f*contactCrop.height/contactCrop.width):std::min(350*size,420.0f*contactCrop.height/contactCrop.width);
    storyContact=actor(beat==2?"art/jaca-bite-sequence.png":"art/story-interactions-v2.png",contactCrop,contactHeight,storyContactBase,false,beat!=2);
    storyContactSprite=static_cast<Sprite*>(storyContact->getChildren().at(0));
    storyContact->setVisible(beat>0 && beat<4);
    if(beat==0) {
        storyOpeningFish=actor("art/story-arrival-v1.png",StoryArrivalCrops[0],72*size,{310,h*.42f-19*size},false,true);
        storyHippoBase={-200,h*.39f};
        storyHippoSprite->setTexture("art/story-arrival-v1.png");
    }
    storyEffects=DrawNode::create(); storyStage->addChild(storyEffects,5);
    // Soft top/bottom scrims preserve the full-screen painting and readable dialogue.
    auto scrim=DrawNode::create(); ui->addChild(scrim,20);
    auto lower=LayerGradient::create(Color4B(4,17,19,245),Color4B(4,17,19,0),{0,1});
    lower->setContentSize({480,bottom+230}); ui->addChild(lower,19);
    auto upper=LayerGradient::create(Color4B(4,17,19,190),Color4B(4,17,19,0),{0,-1});
    upper->setContentSize({480,155}); upper->setPosition({0,h-155}); ui->addChild(upper,19);
    auto label=[&](const std::string& value,float font,Vec2 pos,Color4B color,bool bold) {
        auto result=text(value,font,pos,color,bold); result->setLocalZOrder(21);
        result->setWidth(426); result->setAlignment(TextHAlignment::CENTER);
        result->enableOutline(Color4B(5,30,28,160),1); return result;
    };
    auto chapter=label("JACARUN  /  O PRIMEIRO ENCONTRO",13,{24,top-20},Cream,true);
    chapter->setWidth(295); chapter->setAnchorPoint({0,.5f}); chapter->setAlignment(TextHAlignment::LEFT);
    const Rect skip(354,top-40,106,40);
    pill(scrim,skip,Color4F(.035f,.15f,.15f,.80f));
    auto skipLabel=label("PULAR  >",16,{407,top-19},Cream,true); skipLabel->setWidth(100);
    buttons.push_back({skip,Action::SkipStory});
    const char* speakers[]={"ENQUANTO ISSO, NO BREJO...","JACA","JACA","BENTO","JACA","QUANDO A CONVERSA N\u00c3O RESOLVE..."};
    const char* lines[]={"Dourado estava sozinho.\nMas n\u00e3o por muito tempo.","S\u00f3 um lanchinho.\nNingu\u00e9m vai perceber.","AI! O LANCHE\nME MORDEU!","\u00c9 MEU AMIGO,\nSEU FOMINHA!","Eu tava s\u00f3...\ndando um beijinho!","CORRE, JACA!"};
    const char* notes[]={"Bento vinha buscar seu pequeno amigo.","Jaca escolheu o peixe errado.","Dourado sabia se defender.","Dourado fica protegido nos bra\u00e7os do Bento.","A pausa mais longa da vida do Jaca.","\u201cVOLTA AQUI, BON\u00c9 VERMELHO!\u201d"};
    label(speakers[beat],15,{240,bottom+169},Color4B(255,218,126,255),true);
    storyLine=label(lines[beat],beat==3?32:27,{240,bottom+115},Cream,true);
    storyNote=label(notes[beat],16,{240,bottom+54},Color4B(205,232,215,255),false);
    const char* exclamations[]={"","","NHAC!","\u00d4\u00d4\u00d4!","...","FUI!"};
    storyExclamation=Label::createWithTTF(exclamations[beat],"fonts/Kanit-SemiBold.ttf",beat==3?43:34);
    storyExclamation->setTextColor(Color4B(255,227,139,255));
    storyExclamation->enableOutline(Color4B(33,51,24,255),3);
    storyStage->addChild(storyExclamation,6);
    storyProgress=DrawNode::create(); ui->addChild(storyProgress,20);
    storyFade=DrawNode::create(); ui->addChild(storyFade,19);
    animateStory();
}

void MainScene::animateStory() {
    if(!storyStage || !storyEffects) return;
    const int beat=story.Beat(); const float t=story.Phase();
    const float h=viewHeight,size=std::min(1.12f,h/900);
    auto smooth=[](float x) { x=std::clamp(x,0.0f,1.0f); return x*x*(3-2*x); };
    const float ease=smooth(t), water=h*.42f;
    float push=1+.04f*ease,shake=0;
    if(beat==2) { push=1.02f+.065f*smooth((t-.08f)/.18f); shake=std::sin(t*95)*4*std::max(0.0f,1-std::abs(t-.12f)*6); }
    if(beat==3) { push=1.07f-.025f*ease; shake=std::sin(t*80)*6*std::max(0.0f,1-t*3); }
    if(beat==5) push=1.035f-.035f*ease;
    storyStage->setScale(push); storyStage->setPosition({240*(1-push)+shake,h*.52f*(1-push)});
    storyBackground->setPosition({240-(beat==5?18*ease:0),h*.5f});
    Vec2 jacaOffset,hippoOffset;
    float jump=0;
    if(beat==0) {
        const float arrival=smooth((t-.38f)/.58f);
        storyHippo->setVisible(t>.38f);
        hippoOffset={355*arrival,arrival>0 && arrival<1?3*std::abs(std::sin(t*25)):0};
        const auto& hippoCrop=StoryArrivalCrops[1+(static_cast<int>(story.time*5)%2)*2];
        storyHippoSprite->setTextureRect(Rect(hippoCrop.x,hippoCrop.y,hippoCrop.width,hippoCrop.height));
        storyHippoSprite->setScale(290*size/hippoCrop.height);
        auto fish=static_cast<Sprite*>(storyOpeningFish->getChildren().at(0));
        const auto& fishCrop=StoryArrivalCrops[(static_cast<int>(story.time*4)%2)*2];
        fish->setTextureRect(Rect(fishCrop.x,fishCrop.y,fishCrop.width,fishCrop.height));
        fish->setScale(72*size/fishCrop.height);
        storyOpeningFish->setPosition({310+3*std::sin(t*6.283185f),water-19*size+2*std::sin(t*12)});
    }
    if(beat==1) { jacaOffset={-45*smooth(t/.72f),std::sin(t*9)*3}; hippoOffset={8*ease,0}; storyJaca->setRotation(-3*std::sin(t*3.14159f)); }
    if(beat==2) {
        const float recoil=smooth((t-.28f)/.13f)*(1-.6f*smooth((t-.6f)/.4f));
        jacaOffset={45*recoil,22*recoil}; storyJaca->setRotation(12*recoil);
    }
    if(beat==3) hippoOffset={0,18*std::sin(std::min(1.0f,t*3)*3.14159f)};
    if(beat==4) { jacaOffset={-5*std::sin(t*5),std::sin(t*6.283185f)*3}; storyJaca->setRotation(t<.35f?0:std::sin((t-.35f)*9)*3); }
    if(beat==5) {
        jump=std::clamp((t-.12f)/.58f,0.0f,1.0f);
        const float run=smooth((t-.76f)/.24f);
        jacaOffset={55*jump+260*run,std::min(155.0f,h*.135f)*4*jump*(1-jump)+h*.085f*smooth(jump)};
        if(t<.12f) jacaOffset.y=-10*std::sin(t/.12f*3.14159f);
        hippoOffset={55*ease,10*std::abs(std::sin(t*22))};
        const bool running=t>=.70f;
        const int frame=running?static_cast<int>(story.time*12)%8:jump<.40f?0:jump<.60f?1:2;
        const auto& crop=running?RunCrops[frame]:ActionCrops[frame];
        storyJacaSprite->setTexture(running?"art/jaca-run-v2.png":"art/jaca-actions-v2.png");
        storyJacaSprite->setTextureRect(Rect(crop.x,crop.y,crop.width,crop.height));
        const float density=running?245*size/345:245*size/crop.height;
        const float settle=running?std::sin(std::clamp((t-.70f)/.08f,0.0f,1.0f)*3.14159f):0;
        storyJacaSprite->setScaleX(density*(1+.12f*settle));
        storyJacaSprite->setScaleY(density*(1-.18f*settle)*(t<.12f?.88f:1));
        storyJaca->setRotation(running?0:-9+18*jump);
        storyJacaSprite->setFlippedX(false);
    }
    storyJaca->setPosition(storyJacaBase+jacaOffset); storyHippo->setPosition(storyHippoBase+hippoOffset);
    Vec2 contactOffset(0,std::sin(t*6.283185f)*3);
    if(beat==2) {
        const auto& crop=JacaBiteCrops[StoryIntro::BiteFrame(t)];
        storyContactSprite->setTextureRect(Rect(crop.x,crop.y,crop.width,crop.height));
        storyContactSprite->setScale(280*size/476);
        const float recoil=smooth((t-.08f)/.16f)*(1-.65f*smooth((t-.55f)/.45f));
        contactOffset={18*recoil,12*recoil}; storyContact->setRotation(4*recoil);
    }
    if(beat==3) contactOffset=hippoOffset;
    storyContact->setPosition(storyContactBase+contactOffset);
    if(beat==5) {
        const auto& crop=BentoGuardianCrops[static_cast<int>(story.time*8)%4];
        storyHippoSprite->setTexture("art/bento-guardian-run.png");
        storyHippoSprite->setTextureRect(Rect(crop.x,crop.y,crop.width,crop.height));
        storyHippoSprite->setScale(300*size/489);
    }
    storyEffects->clear();
    for(int ring=0;ring<3;++ring) {
        const float cycle=std::fmod(t+ring*.32f,1.0f);
        const Vec2 origin(beat==0?310.0f:beat==3?240.0f:beat==2 || beat==4?250.0f:storyHippoBase.x+hippoOffset.x,water+3);
        storyEffects->drawCircle(origin,1,0,64,false,65+cycle*70,5+cycle*8,Color4F(.69f,.96f,.89f,(1-cycle)*.45f));
    }
    const float burst=beat==3?std::clamp(t*2.5f,0.0f,1.0f):beat==5?jump:beat==2?std::clamp((t-.3f)*2,0.0f,1.0f):0;
    if(burst>0 && burst<1) for(int i=0;i<20;++i) {
        const float side=i%2?-1.0f:1.0f,origin=beat==5?285.0f:240.0f;
        ellipse(storyEffects,{origin+side*(25+burst*(60+i*4)),water+std::sin(burst*3.14159f)*(40+i*4)},
                2.5f,4.5f,Color4F(.72f,.97f,.94f,(1-burst)*.85f));
    }
    if(beat==5 && t>.35f) for(int i=0;i<7;++i) {
        const float y=h*.5f+i*29;
        storyEffects->drawSegment({420-i*23.0f,y},{456-i*23.0f,y},1.4f,Color4F(.92f,1,.79f,.30f));
    }
    float pop=0;
    if(beat==2) pop=smooth((t-.08f)/.06f)*(1-smooth((t-.67f)/.16f));
    if(beat==3) pop=smooth(t/.08f)*(1-smooth((t-.4f)/.18f));
    if(beat==4) pop=1-smooth((t-.3f)/.2f);
    if(beat==5) pop=smooth((t-.16f)/.08f)*(1-smooth((t-.65f)/.2f));
    storyExclamation->setOpacity(static_cast<uint8_t>(255*pop));
    storyExclamation->setScale(.8f+.2f*pop);
    storyExclamation->setRotation(beat==3?-7:6);
    storyExclamation->setPosition(beat==3?Vec2(340,h*.76f):beat==5?Vec2(110,h*.73f):Vec2(155,h*.77f));
    // Let reactions land before explanatory text; the dialogue itself stays immediately readable.
    storyNote->setOpacity(static_cast<uint8_t>(255*smooth((t-.22f)/.18f)));
    const float bottom=std::max(24.0f,Director::getInstance()->getSafeAreaRect().getMinY()+18);
    storyProgress->clear();
    for(int i=0;i<StoryIntro::BeatCount;++i) {
        const float x=24+i*73;
        pill(storyProgress,{x,bottom,65,3},Color4F(.58f,.72f,.65f,.3f));
        const float fill=i<beat?1:i==beat?t:0;
        if(fill>0) pill(storyProgress,{x,bottom,65*fill,3},Color4F(.91f,.77f,.36f,1));
    }
    storyFade->clear();
    const float fade=.20f*(1-std::clamp(t/.07f,0.0f,1.0f));
    if(fade>0) rect(storyFade,{0,0,480,h},Color4F(.015f,.07f,.08f,fade));
}

void MainScene::rebuildUI() {
    storyJaca=storyHippo=storyStage=storyContact=storyOpeningFish=nullptr; storyContactSprite=storyHippoSprite=storyJacaSprite=storyBackground=nullptr; storyLine=storyNote=storyExclamation=nullptr; storyEffects=storyFade=storyProgress=nullptr;
    ui->removeAllChildren(); panels->clear(); buttons.clear(); menuHint=nullptr; menuBrand=nullptr;
    score = distance = coins = status = pace = gestureHint = coinNotice = nullptr;
    missionHud = missionToast = resultGoals = nullptr;
    gestures.Cancel(); activeTouch = -1; touchAction = Action::None;
    const auto state = game.GetState();
    const auto safe = Director::getInstance()->getSafeAreaRect();
    const float top = std::min(viewHeight - 28.0f, safe.getMaxY() - 24.0f);
    const float bottom = std::max(26.0f, safe.getMinY() + 18.0f);
    const float center = viewHeight * .52f;
    if(story.active) { drawStory(center); return; }
    if(settings) {
        pill(panels,{24,center-200,432,390},Ink);
        text("CONFIGURA\u00c7\u00d5ES",30,{240,center+135},Cream,true);
        text("Deixe o jogo do seu jeito",18,{240,center+93},Muted);
        text("\u00c1UDIO",16,{240,center+42},Color4B(255,218,126,255),true);
        button(soundEnabled?"SOM: LIGADO":"SOM: DESLIGADO",{55,center-33,370,56},Action::Sound,true);
        text("M\u00fasica e efeitos sonoros",17,{240,center-65},Muted);
        button("VOLTAR",{140,center-155,200,48},Action::Back);
        return;
    }
    if(help) {
        pill(panels,{20,center-354,440,684},Ink);
        text("COMO JOGAR",32,{240,center+270},Cream,true);
        text("Aprenda no seu ritmo",20,{240,center+227},Muted);
        const char* titles[]={"1. PULE","2. DESLIZE","3. PAUSE"};
        const char* details[]={"Para cima: pule os obst\u00e1culos.\nTamb\u00e9m funciona agachado.",
                              "Para baixo: deslize sob galhos.\nNo ar: des\u00e7a e deslize.",
                              "Toque em II no canto da tela.\nDois dedos tamb\u00e9m pausam."};
        for(int row=0;row<3;++row) {
            const float y=center+105-row*110.0f;
            pill(panels,{40,y-32,400,99},Color4F(.08f,.25f,.24f,1));
            fitLabel(text(titles[row],22,{240,y+41},Color4B(199,240,100,255),true),368,28);
            auto instructions=text(details[row],19,{240,y-1},Cream);
            instructions->setAlignment(TextHAlignment::CENTER); fitLabel(instructions,368,40);
        }
        text("Moedas compram visuais e melhorias.\nPegue os peixes que saltam da \u00e1gua.",17,{240,center-186},Muted)->setAlignment(TextHAlignment::CENTER);
        button("VAMOS CORRER",{55,center-273,370,56},Action::Play,true);
        button("VOLTAR",{150,center-324,180,44},Action::Back);
        return;
    }
    if (goals) {
        const auto& profile = game.GetProfile();
        pill(panels, {20, center-360, 440, 690}, Color4F(.035f,.17f,.18f,1));
        text("CADA CORRIDA CONTA", 16, {240,center+293}, Muted);
        text("Trilhas do mangue", 32, {240,center+254}, Cream, true);
        text(whole(profile.missionsCompleted)+" miss\u00f5es completas  /  "+whole(profile.coins)+" moedas",
             17, {240,center+214}, Color4B(255,219,122,255));
        button("MISS\u00d5ES", {40,center+145,192,44}, Action::Missions,!achievementTab);
        button("CONQUISTAS", {248,center+145,192,44}, Action::Achievements,achievementTab);
        for(int row=0; row<3; ++row) {
            const int id = achievementTab ? goalPage*3+row : profile.missionIds[row];
            const auto& goal = achievementTab ? AchievementCatalog()[id] : MissionCatalog()[id];
            const auto value = achievementTab ? profile.AchievementProgress(id) : profile.missionProgress[row];
            const bool done = achievementTab && (profile.achievements & (1u<<id));
            const float y = center+17-row*110.0f;
            pill(panels,{36,y,408,105},Color4F(.08f,.25f,.24f,1));
            fitLabel(text(goal.title,21,{52,y+77},done?Color4B(199,240,100,255):Cream,true),376,26)->setAnchorPoint({0,.5f});
            fitLabel(text(goal.description,15,{52,y+53},Muted),376,20)->setAnchorPoint({0,.5f});
            fitLabel(text(done?"CONQUISTADA":whole(value)+" / "+whole(goal.target),14,{52,y+32},Cream),180,18)->setAnchorPoint({0,.5f});
            fitLabel(text(done?"B\u00d4NUS RECEBIDO":"+"+whole(goal.reward)+" moedas",14,{428,y+32},Color4B(255,219,122,255)),180,18)->setAnchorPoint({1,.5f});
            pill(panels,{52,y+13,376,8},Color4F(.035f,.16f,.17f,1));
            const float fill=376*std::clamp(static_cast<float>(done?1.0:static_cast<double>(value)/goal.target),0.0f,1.0f);
            if(fill>.1f) pill(panels,{52,y+13,fill,8},Lime);
        }
        if(achievementTab) {
            button("<",{40,center-263,55,44},Action::Previous);
            text(whole(goalPage+1)+" / 2",15,{240,center-245},Muted);
            button(">",{385,center-263,55,44},Action::Next);
        } else text("Progresso acumulado. Novas miss\u00f5es ao concluir.",14,{240,center-242},Muted);
        button("VOLTAR",{150,center-301,180,44},Action::Back);
        fitLabel(text(saveWarning.empty()?"B\u00f4nus creditados ao encerrar a corrida.":saveWarning,13,{240,center-330},Muted),392,22);
        return;
    }
    if (shop) {
        const auto& profile=game.GetProfile();
        const int pages=(CosmeticCount+2)/3;
        const Color4B gold(255,219,122,255), mint(199,240,100,255);
        rect(panels,{0,0,480,viewHeight},Color4F(.015f,.07f,.08f,.48f));
        pill(panels,{18,center-367,444,703},Color4F(.25f,.39f,.24f,1));
        pill(panels,{21,center-364,438,697},Color4F(.035f,.14f,.15f,1));
        art(panels,"art/jaca-portrait.png",{0,0,512,512},{39,center+237},{72,72},1);
        text("LOJA DO MANGUE",14,{126,center+296},gold,true)->setAnchorPoint({0,.5f});
        fitLabel(text("Seu Jaca, seu estilo",28,{126,center+266},Cream,true),310,36)->setAnchorPoint({0,.5f});
        pill(panels,{40,center+198,400,34},Color4F(.09f,.26f,.23f,1));
        ellipse(panels,{62,center+215},8,10,Color4F(1,.77f,.22f,1));
        fitLabel(text(whole(profile.coins)+" MOEDAS",19,{80,center+215},gold,true),170,24)->setAnchorPoint({0,.5f});
        fitLabel(text("SUAS CONQUISTAS RENDEM",12,{424,center+215},Muted),164,20)->setAnchorPoint({1,.5f});
        button("VISUAIS",{40,center+145,192,44},Action::Looks,!upgradeShop);
        button("MELHORIAS",{248,center+145,192,44},Action::Upgrades,upgradeShop);
        const char* descriptions[]={"O parceiro de toda aventura.","Sol na cara? Vai de boné.",
            "Estilo de sangue frio.","Pronto para explorar o mangue.","Um toque de coragem.",
            "O mangue tem um novo rei.","Seu estilo foi longe demais!"};
        for(int row=0;row<3;++row) {
            const int id=(upgradeShop?CosmeticCount:shopPage*3)+row;
            if(id>=ShopItemCount || (!upgradeShop && id>=CosmeticCount)) break;
            const auto& item=AccessoryCatalog()[id];
            const bool owned=profile.owned[id], equipped=id<CosmeticCount && profile.equipped==id;
            const bool active=owned && (upgradeShop || equipped);
            const bool affordable=profile.coins>=item.price;
            const float y=center+22-row*110.0f;
            pill(panels,{35,y-1,410,103},active?Color4F(.48f,.67f,.27f,1):Color4F(.16f,.34f,.29f,1));
            pill(panels,{37,y+1,406,99},active?Color4F(.10f,.29f,.23f,1):Color4F(.065f,.22f,.21f,1));
            pill(panels,{44,y+8,101,83},Color4F(.035f,.15f,.16f,1));
            if(upgradeShop) {
                ellipse(panels,{94,y+53},30,30,Color4F(.13f,.37f,.30f,1));
                const char* symbols[]={"+2s","+4s","+3s"};
                const char* effects[]={"COMBO","IMA","FRENESI"};
                text(symbols[row],25,{94,y+56},gold,true);
                text(effects[row],12,{94,y+23},mint,true);
            } else drawCrocodile({91,y+14},.68f,panels,id);
            auto name=text(item.name,20,{155,y+80},Cream,true);
            fitLabel(name,272,26)->setAnchorPoint({0,.5f});
            auto detail=text(upgradeShop?item.description:descriptions[id],13,{155,y+57},Muted);
            fitLabel(detail,272,21)->setAnchorPoint({0,.5f});
            const std::string price=owned?(equipped?"SEU FAVORITO":upgradeShop?"PERMANENTE":"NA COLECAO"):whole(item.price)+" moedas";
            fitLabel(text(price,14,{155,y+24},owned?mint:gold,true),133,22)->setAnchorPoint({0,.5f});
            const std::string caption=active?(upgradeShop?"ATIVA":"EM USO"):owned?"EQUIPAR":"COMPRAR";
            if(active) {
                pill(panels,{300,y+5,130,40},Color4F(.17f,.36f,.25f,1));
                text(caption,18,{365,y+25},mint,true);
            } else button(caption,{300,y+5,130,40},static_cast<Action>(static_cast<int>(Action::Item0)+row),owned || affordable);
        }
        if(!upgradeShop) {
            if(shopPage>0) button("<",{40,center-253,55,44},Action::Previous);
            text("COLECAO  "+whole(shopPage+1)+" / "+whole(pages),14,{240,center-232},Muted);
            if(shopPage+1<pages) button(">",{385,center-253,55,44},Action::Next);
            if(shopPage==pages-1) {
                text("Cada corrida, um novo estilo.",22,{240,center-40},Cream,true);
                text("Junte moedas e complete sua colecao!",16,{240,center-73},Muted);
            }
        } else text("Compre uma vez. Aproveite em toda corrida.",14,{240,center-232},Muted);
        button("VOLTAR",{150,center-293,180,40},Action::Back);
        status=text(shopNotice,13,{240,center-328},Muted);
        status->setWidth(390); status->setAlignment(TextHAlignment::CENTER); fitLabel(status,390,36);
        return;
    }
    if (state == GameState::Menu) {
        menuBrand=Node::create(); menuBrand->setCascadeOpacityEnabled(true); ui->addChild(menuBrand);
        auto brandChild=[&](Node* child) {
            child->retain(); child->removeFromParent(); menuBrand->addChild(child); child->release();
        };
        brandChild(art(panels,"art/logo.png",{15,140,1510,780},{80,top-165},{320,165},1));
        pill(panels,{24,top-255,208,68},Color4F(.04f,.22f,.22f,.92f));
        text("RECORDE",13,{128,top-207},Muted,true);
        fitLabel(text(whole(game.GetProfile().bestScore),24,{128,top-233},Cream,true),176,28);
        button("SEU JACA\n"+whole(game.GetProfile().coins)+" MOEDAS",{248,top-255,208,68},Action::Shop,false,19);
        button("MISS\u00d5ES E CONQUISTAS",{24,top-314,432,44},Action::Goals,false,18);
        menuHint=text("TOQUE NO JACA OU NO BOT\u00c3O",14,{240,viewHeight*.53f},Cream);
        menuHint->enableOutline(Color4B(28,55,22,255),2);
        button("TOQUE PARA COME\u00c7AR",{44,bottom+105,392,64},Action::Play,true,22);
        button("COMO JOGAR",{24,bottom+14,144,54},Action::Help,false,16);
        button("INTRO",{176,bottom+14,84,54},Action::ReplayStory,false,16);
        button("CONFIGURA\u00c7\u00d5ES",{268,bottom+14,188,54},Action::Settings,false,16);
        if (!saveWarning.empty()) text(saveWarning, 13, {240, bottom-7}, Color4B(255,194,116,255));
        return;
    }
    // A compact HUD floats over the forest; there are no gameplay buttons or reserved panels.
    pill(panels, {22, top - 62, 436, 65}, Color4F(.035f,.16f,.17f,1));
    text("PONTOS", 16, {72, top - 10}, Muted);
    score = text(whole(game.GetScore()), 28, {73, top - 37}, Cream, true);
    distance = text(whole(game.GetDistance()) + " m", 24, {243, top - 30}, Cream, true);
    ellipse(panels, {376, top - 29}, 9, 11, Color4F(1,.79f,.31f,1));
    coins = text(whole(game.GetRunCoins()), 24, {413, top - 29}, Color4B(255,219,122,255), true);
    if (state == GameState::Playing) {
        button("II",{410,top-119,48,48},Action::Pause);
        status = text("", 18, {211, top - 101}, Cream, true);
        status->enableOutline(Color4B(8,32,23,255),2);
        status->setWidth(354); status->setAlignment(TextHAlignment::CENTER);
        pace = text("", 18, {211, top - 137}, Color4B(255,219,122,255), true);
        pace->setWidth(354); pace->setAlignment(TextHAlignment::CENTER);
        pace->enableOutline(Color4B(13,47,42,210), 1);
        pill(panels, {22,bottom+105,436,48}, Color4F(.025f,.12f,.14f,.76f));
        missionHud = text("",17,{240,bottom+130},Cream);
        missionHud->setWidth(404); missionHud->setAlignment(TextHAlignment::CENTER);
        missionHud->enableOutline(Color4B(13,47,42,230),2);
        missionToast = text("",28,{240,top-225},Color4B(220,255,145,255),true);
        missionToast->setWidth(430); missionToast->setAlignment(TextHAlignment::CENTER);
        coinNotice = text("",30,{240,top-285},Color4B(255,224,128,255),true);
        coinNotice->enableOutline(Color4B(8,27,25,255),3);
        missionToast->enableOutline(Color4B(13,47,42,230),2);
        gestureHint = text("ARRASTE PARA CIMA: PULO\nPARA BAIXO: DESLIZE  /  2 DEDOS: PAUSA\nPEGUE OS PEIXES QUANDO SALTAREM",
                           17, {240, bottom + 49}, Cream);
        gestureHint->setAlignment(TextHAlignment::CENTER);
        gestureHint->enableOutline(Color4B(13,47,42,210), 1);
        return;
    }
    // Let the impact, recoil and collapse read before covering the lane with results.
    if(state==GameState::GameOver && motion.defeat<.72f) return;
    rect(panels, {0,0,480,viewHeight}, Color4F(.015f,.09f,.11f,.49f));
    pill(panels, {28, center - 300, 424, state==GameState::Paused?560.0f:626.0f}, Color4F(.035f,.17f,.18f,.96f));
    if (state == GameState::Paused) {
        text("RESPIRE UM POUCO", 16, {240, center + 187}, Muted);
        text("Corrida pausada", 36, {240, center + 135}, Cream, true);
        text("Arraste para cima para continuar.", 20, {240, center + 82}, Muted);
        button("CONTINUAR", {59, center - 66, 362, 66}, Action::Pause, true);
        button("ENCERRAR E VOLTAR", {59, center - 149, 362, 58}, Action::Menu);
        text("As moedas coletadas ser\u00e3o guardadas.", 16, {240, center - 198}, Muted);
        button(soundEnabled?"SOM: SIM":"SOM: N\u00c3O",{160,center-262,160,38},Action::Sound);
    } else {
        const auto& rewards=game.GetProgressRewards();
        const auto& dizzy=ActionCrops[7];
        art(panels,"art/jaca-actions-v2.png",Rect(dizzy.x,dizzy.y,dizzy.width,dizzy.height),
            {130,center+202},{220,220.0f*dizzy.height/dizzy.width},1);
        text(rewards.coins>0?"OBJETIVOS CONCLU\u00cdDOS!":"MAIS UMA HISTORIA NO MANGUE",14,{240,center+184},Muted);
        text("Boa, Jaca!",40,{240,center+149},Cream,true);
        text(whole(game.GetScore())+" pontos",32,{240,center+108},Cream,true);
        fitLabel(text(whole(game.GetDistance())+" m  /  Recorde: "+whole(game.GetProfile().bestScore)+" pontos",16,{240,center+77},Muted),376,22);
        pill(panels,{49,center+8,382,58},Color4F(.10f,.29f,.25f,1));
        text("+"+whole(game.GetRunCoins()+rewards.coins)+" MOEDAS",25,{240,center+44},Color4B(255,219,122,255),true);
        text("Coleta: "+whole(game.GetRunCoins())+"  /  Objetivos: "+whole(rewards.coins),15,{240,center+21},Muted);
        resultGoals=text("",16,{240,center-37},Cream);
        resultGoals->setAlignment(TextHAlignment::CENTER);
        resultGoals->setWidth(game.GetCompletedGoals().size()>2?240:376);
        if(game.GetCompletedGoals().size()>2) {
            button("<",{59,center-66,48,44},Action::Previous);
            button(">",{373,center-66,48,44},Action::Next);
        }
        button("CORRER DE NOVO",{59,center-145,362,60},Action::Play,true);
        button("VER OBJETIVOS",{59,center-205,362,46},Action::Goals);
        button("VOLTAR AO IN\u00cdCIO",{59,center-259,362,42},Action::Menu);
        if(!saveWarning.empty()) fitLabel(text(saveWarning,13,{240,center-278},Color4B(255,194,116,255)),376,20);
    }
}

Sprite* MainScene::art(Node* parent, const char* file, Rect crop, Vec2 position, Size size, int order) {
    Sprite* sprite=nullptr;
    if(parent==panels) {
        sprite=Sprite::create(); ui->addChild(sprite,order);
    } else {
        if(artUsed==artPool.size()) {
            sprite=Sprite::create(); scenery->addChild(sprite); artPool.push_back(sprite);
        } else sprite=artPool[artUsed];
        ++artUsed;
        if(sprite->getParent()!=parent) {
            sprite->retain(); sprite->removeFromParent(); parent->addChild(sprite); sprite->release();
        }
    }
    sprite->setTexture(file);
    sprite->setTextureRect(crop);
    sprite->setAnchorPoint({0,0}); sprite->setPosition(position);
    sprite->setScaleX(size.width/crop.size.width); sprite->setScaleY(size.height/crop.size.height);
    sprite->setRotation(0); sprite->setFlippedX(false); sprite->setOpacity(255);
    sprite->setColor(Color3B::WHITE); sprite->setVisible(true); sprite->setLocalZOrder(order);
    for(auto child:sprite->getChildren()) child->setVisible(false);
    return sprite;
}

void MainScene::drawBackdrop(float ground) {
    const float travel=static_cast<float>(game.GetDistance());
    const float width=viewHeight*1.5f;
    const float offset=travel*1.2f+width*.32f;
    const int tile=static_cast<int>(offset/width);
    const auto blend=EnvironmentAtDistance(game.GetState()==GameState::Menu?0:game.GetDistance());
    const int environment=blend.progress<1?blend.previous:blend.current;
    // Keep old artwork opaque; reveal the incoming biome in spatial strips.
    auto layer = [&](const char* file, Rect crop, Vec2 position, Size size,
                     int order, bool flipped, bool foreground, bool incoming) {
        if(!incoming) {
            auto sprite=art(scenery,file,crop,position,size,order);
            sprite->setFlippedX(flipped);
            return;
        }
        constexpr float stripWidth=8;
        for(float left=std::max(0.0f,position.x);left<std::min(480.0f,position.x+size.width);left+=stripWidth) {
            const float right=std::min({left+stripWidth,480.0f,position.x+size.width});
            const float opacity=EnvironmentCoverage(blend.progress,(left+right)*.5f,foreground);
            if(opacity<=0) continue;
            const float u=(left-position.x)/size.width;
            const float span=(right-left)/size.width;
            Rect slice(crop.origin.x+(flipped?1-u-span:u)*crop.size.width,
                       crop.origin.y,span*crop.size.width,crop.size.height);
            auto sprite=art(scenery,file,slice,{left,position.y},{right-left,size.height},order);
            sprite->setFlippedX(flipped);
            sprite->setOpacity(static_cast<uint8_t>(255*opacity));
        }
    };
    const bool transitioning=blend.progress>0 && blend.progress<1;
    for(int i=0;i<2;++i) {
        const Vec2 position(i*width-std::fmod(offset,width),0);
        layer(EnvironmentFiles[environment],{0,0,1536,1024},position,
              {width+1,viewHeight},-10,(tile+i)%2!=0,false,false);
        if(transitioning)
            layer(EnvironmentFiles[blend.current],{0,0,1536,1024},position,
                  {width+1,viewHeight},-9,(tile+i)%2!=0,false,true);
    }
    const Rect strips[]={{0,100,1536,220},{0,425,1536,240},{0,735,1536,260}};
    const float surface[]={70,65,110};
    for(int i=0;i<2;++i) {
        const float x=i*768.0f-std::fmod(travel*PixelsPerMeter,768.0f);
        layer("art/ground-atlas.png",strips[environment],
              {x,ground-(strips[environment].size.height-surface[environment])*.5f},
              {769,strips[environment].size.height*.5f},-5,false,true,false);
        if(transitioning) {
            const int next=blend.current;
            layer("art/ground-atlas.png",strips[next],
                  {x,ground-(strips[next].size.height-surface[next])*.5f},
                  {769,strips[next].size.height*.5f},-4,false,true,true);
        }
    }
    // A subtle lower vignette keeps mission text readable while leaving the illustration visible.
    for(int i=0;i<12;++i)
        rect(world,{0,i*ground/12,480,ground/12+1},Color4F(.015f,.09f,.065f,.48f*(1-i/12.0f)));
}

void MainScene::drawCrocodile(Vec2 feet, float scale, DrawNode* target, int previewAccessory) {
    auto* drawing = target ? target : world;
    const bool sliding = !target && (game.GetPlayer().IsSliding() || game.GetPlayer().IsDiving()) && game.GetState() != GameState::Menu;
    const bool airborne = !target && !game.GetPlayer().IsGrounded() && game.GetState()!=GameState::Menu;
    const bool chaseRunner=target==menuActor && chaseEntry && entrance.time<1.1f && !shop && !goals && !help && !settings;
    const bool idle = !chaseRunner && (target || game.GetState()==GameState::Menu);
    const float bob = idle ? 0 : airborne || sliding ? 0 : std::max(0.0f,std::sin(motion.phase*2))*2.6f;
    const float lean = airborne ? std::clamp(static_cast<float>(game.GetPlayer().GetVelocityY())*.012f,-.12f,.12f) : -.035f*motion.slide;
    const float sx=motion.Width(), sy=motion.Height();
    auto point = [&](float x, float y) {
        return feet + Vec2((x*sx-y*lean)*scale,(y*sy+x*lean+bob)*scale);
    };
    const bool shield = !target && game.GetState()!=GameState::Menu && game.GetShieldSeconds()>0;
    const float shieldPulse=.5f+.5f*std::sin(motion.time*3.8f);
    const float shieldAlpha=game.GetShieldSeconds()<3 ? .62f+.38f*(.5f+.5f*std::sin(motion.time*9)) : 1;
    const Vec2 shieldCenter=point(5,sliding?25:43);
    const float shieldX=(76+shieldPulse*2)*scale;
    const float shieldY=(73-28*motion.slide+shieldPulse*2)*scale;
    auto shieldPoint=[&](float angle,float radius=1.0f) {
        return shieldCenter+Vec2(std::cos(angle)*shieldX,std::sin(angle)*shieldY)*radius;
    };
    if(shield) {
        // Broad halo behind the actor gives the energy shell depth without tinting his face.
        for(int i=4;i>=0;--i)
            ellipse(drawing,shieldCenter,shieldX+i*3,shieldY+i*3,
                    Color4F(.18f,.80f,1,(.025f+shieldPulse*.006f)*shieldAlpha));
        ellipse(drawing,shieldCenter,shieldX*.94f,shieldY*.94f,Color4F(.16f,.64f,.88f,.13f*shieldAlpha));
    }
    const bool defeated=!target && game.GetState()==GameState::GameOver;
    auto pose=AnimateJaca(motion,idle,!airborne,sliding,game.GetPlayer().IsDiving(),defeated,
        game.GetPlayer().GetVelocityY(),target==menuActor && !chaseEntry?entrance.time:-1);
    const auto& bounds=pose.atlas==JacaAtlas::Run?RunCrops[pose.frame]:
                       pose.atlas==JacaAtlas::Idle?IdleCrops[pose.frame]:ActionCrops[pose.frame];
    const char* file=pose.atlas==JacaAtlas::Run?"art/jaca-run-v2.png":
                     pose.atlas==JacaAtlas::Idle?"art/jaca-idle-v2.png":"art/jaca-actions-v2.png";
    const Rect crop(static_cast<float>(bounds.x),static_cast<float>(bounds.y),
                    static_cast<float>(bounds.width),static_cast<float>(bounds.height));
    // Keep pixel density stable across stride frames instead of stretching every pose to a box.
    if(pose.atlas==JacaAtlas::Run) { pose.width=bounds.width*.30f; pose.height=bounds.height*.30f; }
    const float width=pose.width*scale*pose.scaleX, height=pose.height*scale*pose.scaleY;
    auto actor=art(drawing,file,crop,{0,0},{width,height},2);
    // Transform around the planted feet; the contact coordinate remains independent of the sprite.
    actor->setAnchorPoint({.65f,0});
    actor->setPosition({feet.x+20*scale,feet.y+pose.bob*scale});
    actor->setRotation(pose.angle);
    auto decor=static_cast<DrawNode*>(actor->getChildByTag(100));
    if(!decor) { decor=DrawNode::create(); actor->addChild(decor,1,100); }
    decor->clear(); decor->setVisible(true);
    if(defeated && motion.defeat>.28f) {
        for(int i=0;i<3;++i) {
            const float a=motion.time*3+i*2.0944f;
            const Vec2 p(crop.size.width*.72f+std::cos(a)*54,crop.size.height+23+std::sin(a)*12);
            Vec2 star[8];
            for(int j=0;j<8;++j) {
                const float angle=j*.785398f;
                star[j]=p+Vec2(std::cos(angle),std::sin(angle))*(j%2?4.0f:12.0f);
            }
            decor->drawSolidPoly(star,8,Color4F(1,.82f,.13f,1));
        }
    }
    const int accessory=previewAccessory>=0?previewAccessory:game.GetProfile().equipped;
    // Cosmetic overlays use atlas-local coordinates, so previews and gameplay share the same look.
    const Vec2 eye(crop.size.width*.73f,crop.size.height*.67f);
    const Vec2 cap(crop.size.width*.57f,crop.size.height*.87f);
    if(accessory==1) {
        ellipse(decor,cap,43,18,Color4F(1,.58f,.08f,.95f));
    } else if(accessory==2) {
        ellipse(decor,eye,26,19,Ink);
        decor->drawSegment(eye-Vec2(35,-7),eye+Vec2(25,7),5,Ink);
        decor->drawSegment(eye+Vec2(3,9),eye+Vec2(15,15),3,Color4F(.5f,.9f,1,1));
    } else if(accessory==3) {
        ellipse(decor,cap,65,12,Color4F(.85f,.62f,.24f,1));
        rect(decor,{cap.x-33,cap.y,66,32},Color4F(.97f,.78f,.36f,1));
    } else if(accessory==4) {
        decor->drawSegment(cap-Vec2(36,8),cap+Vec2(38,-8),8,Color4F(.9f,.06f,.12f,1));
        decor->drawSegment(cap-Vec2(30,8),cap-Vec2(62,32),8,Color4F(.9f,.06f,.12f,1));
    } else if(accessory==5) {
        Vec2 crown[]={cap+Vec2(-38,0),cap+Vec2(-45,45),cap+Vec2(-17,27),cap+Vec2(0,58),cap+Vec2(18,27),cap+Vec2(44,45),cap+Vec2(37,0)};
        decor->drawSolidPoly(crown,7,Color4F(1,.8f,.14f,1));
        ellipse(decor,cap+Vec2(0,12),7,9,Color4F(.95f,.1f,.12f,1));
    } else if(accessory==6) {
        decor->drawCircle(eye,82,0,48,false,Color4F(.65f,.92f,1,.9f));
        ellipse(decor,eye+Vec2(45,45),9,21,Color4F(.8f,.98f,1,.65f));
    }
    if(shield) {
        // Transparent front surface, luminous rim and counter-rotating energy trails.
        ellipse(drawing,shieldCenter,shieldX*.97f,shieldY*.97f,Color4F(.36f,.88f,1,.055f*shieldAlpha));
        for(int i=0;i<64;++i) {
            const float a=i*6.2831853f/64, b=(i+1)*6.2831853f/64;
            drawing->drawSegment(shieldPoint(a),shieldPoint(b),2.4f,Color4F(.27f,.85f,1,.48f*shieldAlpha));
        }
        for(int band=0;band<3;++band) {
            const float head=motion.time*(band==1?-1.5f:1.1f)+band*2.0944f;
            for(int i=0;i<15;++i) {
                const float a=head-i*.045f;
                drawing->drawSegment(shieldPoint(a),shieldPoint(a-.045f),2.0f,
                    Color4F(.72f,.98f,1,(1-i/15.0f)*.9f*shieldAlpha));
            }
            const Vec2 p=shieldPoint(head);
            ellipse(drawing,p,7,7,Color4F(.32f,.85f,1,.16f*shieldAlpha));
            ellipse(drawing,p,2.8f,2.8f,Color4F(.88f,1,1,shieldAlpha));
        }
        // Curved reflection on the upper shell reads as a bubble, not a flat outline.
        for(int i=0;i<14;++i) {
            const float a=1.15f+i*.055f;
            drawing->drawSegment(shieldPoint(a,.87f),shieldPoint(a+.055f,.87f),2.8f,
                Color4F(.85f,1,1,.48f*shieldAlpha));
        }
        for(int i=0;i<8;++i) {
            const float a=motion.time*.55f+i*.785398f;
            const Vec2 p=shieldPoint(a,1.08f+.025f*std::sin(motion.time*2+i));
            const float sparkle=(.5f+.5f*std::sin(motion.time*4+i))*shieldAlpha;
            drawing->drawSegment(p-Vec2(3,0),p+Vec2(3,0),.8f,Color4F(.7f,1,1,sparkle));
            drawing->drawSegment(p-Vec2(0,3),p+Vec2(0,3),.8f,Color4F(.7f,1,1,sparkle));
        }
    }
}

void MainScene::burst(Vec2 point, Color4F color, int count, float strength) {
    for(int i=0;i<count && particles.size()<160;++i) {
        const float angle=(i+.5f)*2.39996f;
        const float speed=(40+(i%4)*23.0f)*strength;
        const float life=.32f+(i%4)*.07f;
        particles.push_back({point,{std::cos(angle)*speed,std::sin(angle)*speed+28},life,life,2.5f+(i%3),color});
    }
}

void MainScene::sound(const char* name) {
    if(!soundEnabled || smokeStep>=0) return;
    AudioEngine::play2d(std::string("audio/")+name+".wav",false,std::string(name)=="coin" ? .70f : std::string(name)=="pace" ? .75f : .48f);
}

void MainScene::drawEntity(const VisualEntity& object) {
    const auto& e = object.entity;
    float x = Contact + static_cast<float>(e.distance - game.GetDistance()) * PixelsPerMeter;
    // Place the dangerous part around the contact coordinate, not a full sprite ahead.
    if (IsObstacle(e.type)) x -= 40;
    if (x < -120 || x > 570) return;
    float y = floorY + static_cast<float>(e.height) * HeightScale;
    float fishAngle = 0;
    const bool isFish = e.type==EntityType::Fish || e.type==EntityType::RareFish;
    if(isFish && object.outcome!=EntityOutcome::Collected) {
        const auto pose=FishLeap(e.distance-game.GetDistance(),floorY+22,y);
        const Color4F water(.62f,.86f,.83f,.65f);
        // Rings and droplets locate the leap in the tidal channel.
        world->drawCircle({x,floorY+22},1,0,32,false,29,5,water);
        world->drawCircle({x,floorY+22},1,0,32,false,19,3,Color4F(.72f,.92f,.88f,.45f));
        if(pose.splash>0) for(int i=0;i<5;++i) {
            const float side=static_cast<float>(i-2);
            ellipse(world,{x+side*(7+pose.splash*5),floorY+24+pose.splash*(22-std::abs(side)*6)},
                    2,3+pose.splash*2,water);
        }
        if(!pose.airborne) return;
        y=pose.height; fishAngle=pose.angle;
        for(int i=1;i<=3;++i) {
            const float dropY=y-i*12.0f;
            if(dropY>floorY+25) ellipse(world,{x+i*4.0f,dropY},1.6f,3,Color4F(.65f,.9f,.91f,.65f-i*.12f));
        }
    }
    // Crabs walk on the bank rather than hovering at the food collision height.
    if(e.type==EntityType::Crab) y=floorY+13;
    float scale = 1, opacity = 1;
    if (object.outcome == EntityOutcome::Collected) {
        const float t = std::clamp(object.effectTime / RunPresentation::CollectionSeconds, 0.0f, 1.0f);
        x += (Contact - 16 - x) * t;
        y += 35 * std::sin(t * 3.14159f) + 19 * t;
        scale = 1 - .72f*t;
        opacity = 1 - t*t;
    }
    int tile=-1;
    Size dimensions(54,48);
    Vec2 origin(x-27,y-24);
    switch(e.type) {
    case EntityType::Ground: tile=0; dimensions={62,57}; origin={x-5,floorY}; break;
    case EntityType::Log: tile=1; dimensions={68,40}; origin={x-5,floorY}; break;
    case EntityType::Rock: tile=2; dimensions={64,48}; origin={x-5,floorY}; break;
    case EntityType::High: case EntityType::Vine:
        tile=3; dimensions={82,80}; origin={x-10,floorY+57}; break;
    case EntityType::Crab:
        tile=4; dimensions={42,32}; origin={x-21,floorY+std::abs(std::sin(motion.time*18))*3}; break;
    case EntityType::Fish: tile=5; break;
    case EntityType::RareFish: tile=6; break;
    case EntityType::Coin: tile=7; dimensions={25,28}; origin={x-12.5f,y-14}; break;
    default: break;
    }
    if(tile>=0) {
        const Rect crops[]={{8,110,365,305},{395,205,365,180},{780,165,360,235},{1155,125,375,295},
                            {15,650,360,280},{405,635,350,280},{785,625,355,280},{1200,625,295,305}};
        auto sprite=art(world,"art/objects-atlas.png",crops[tile],origin,{dimensions.width*scale,dimensions.height*scale},1);
        sprite->setOpacity(static_cast<uint8_t>(255*opacity));
        if(isFish) sprite->setRotation(-fishAngle*180/3.14159265f);
        if(e.type==EntityType::Coin) sprite->setScaleX(sprite->getScaleX()*(.35f+.65f*std::abs(std::cos(motion.time*3+static_cast<float>(e.id)))));
        return;
    }
    auto tint = [opacity](Color4F c) { c.a *= opacity; return c; };
    auto pos = [&](float dx, float dy) { return Vec2(x+(dx*std::cos(fishAngle)-dy*std::sin(fishAngle))*scale, y+(dx*std::sin(fishAngle)+dy*std::cos(fishAngle))*scale); };
    auto oval = [&](float dx,float dy,float rx,float ry,Color4F c) {
        if(isFish) {
            Vec2 outline[24];
            for(int i=0;i<24;++i) {
                const float angle=i*6.2831853f/24;
                outline[i]=pos(dx+std::cos(angle)*rx,dy+std::sin(angle)*ry);
            }
            world->drawSolidPoly(outline,24,tint(c));
        } else ellipse(world,pos(dx,dy),rx*scale,ry*scale,tint(c));
    };
    auto line = [&](float x1,float y1,float x2,float y2,float radius,Color4F c) {
        world->drawSegment(pos(x1,y1),pos(x2,y2),radius*scale,tint(c));
    };
    const Color4F wood(.47f,.27f,.15f,1), bark(.74f,.48f,.25f,1);
    switch(e.type) {
    case EntityType::Log:
        ellipse(world,{x+23,floorY+18},31,18,wood);
        world->drawSegment({x+2,floorY+32},{x+43,floorY+32},3,bark);
        ellipse(world,{x+48,floorY+18},12,18,Color4F(.85f,.66f,.36f,1));
        world->drawCircle({x+48,floorY+18},8,0,20,false,wood);
        break;
    case EntityType::Rock: {
        Vec2 rock[]={{x-5,floorY},{x+2,floorY+29},{x+19,floorY+46},
                     {x+40,floorY+36},{x+57,floorY+4}};
        world->drawSolidPoly(rock,5,Color4F(.43f,.51f,.52f,1));
        Vec2 face[]={{x+2,floorY+29},{x+19,floorY+46},{x+29,floorY+17}};
        world->drawSolidPoly(face,3,Color4F(.71f,.76f,.68f,1));
        world->drawSegment({x+30,floorY+34},{x+44,floorY+10},2,Color4F(.24f,.34f,.35f,1));
        break;
    }
    case EntityType::Vine:
        for(int strand=0;strand<3;++strand) {
            const float vx=x+strand*19;
            world->drawSegment({vx+12,floorY+218},{vx,floorY+65},4,Color4F(.43f,.60f,.23f,1));
            ellipse(world,{vx+8+std::sin(motion.time*2+strand)*4,floorY+130+strand*12.0f},10,17,Color4F(.64f,.75f,.28f,1));
        }
        world->drawSegment({x-5,floorY+62},{x+56,floorY+62},10,Color4F(.38f,.49f,.18f,1));
        world->drawSegment({x-3,floorY+67},{x+53,floorY+67},2,Color4F(.82f,.83f,.38f,1));
        break;
    case EntityType::Ground: {
        ellipse(world,{x+26,floorY+1},38,5,Color4F(.05f,.16f,.12f,.4f));
        Vec2 root[]={{x-3,floorY},{x+3,floorY+15},{x+9,floorY+45},{x+19,floorY+51},
                     {x+29,floorY+19},{x+43,floorY+9},{x+66,floorY}};
        world->drawSolidPoly(root,7,wood);
        world->drawSegment({x+14,floorY+8},{x+16,floorY+39},3,bark);
        world->drawSegment({x+22,floorY+13},{x+44,floorY+5},2,bark);
        ellipse(world,{x+37,floorY+9},8,4,Color4F(.51f,.63f,.28f,1));
        break;
    }
    case EntityType::High:
        world->drawSegment({x+40,floorY+211},{x+23,floorY+139},12,Color4F(.25f,.24f,.15f,1));
        world->drawSegment({x+23,floorY+139},{x+13,floorY+64},12,wood);
        world->drawSegment({x+3,floorY+60},{x+63,floorY+60},17,wood);
        world->drawSegment({x+6,floorY+65},{x+59,floorY+65},4,bark);
        world->drawSegment({x+20,floorY+92},{x+42,floorY+111},5,wood);
        ellipse(world,{x+32+std::sin(motion.time*1.8f)*3,floorY+202},49,28,Color4F(.22f,.43f,.24f,1));
        ellipse(world,{x+14+std::sin(motion.time*1.8f+.7f)*4,floorY+221},34,19,Color4F(.31f,.53f,.27f,1));
        ellipse(world,{x+52,floorY+211},29,18,Color4F(.38f,.57f,.29f,1));
        break;
    case EntityType::Coin: {
        const float face=.30f+.70f*std::abs(std::cos(motion.time*4+static_cast<float>(e.distance)));
        oval(0,0,12*face,15,Color4F(.69f,.40f,.12f,1));
        oval(0,2,10*face,13,Color4F(1,.78f,.26f,1));
        oval(-2,3,6*face,9,Color4F(1,.90f,.50f,1));
        line(0,-3,0,7,1.2f,Color4F(.75f,.49f,.15f,1));
        break;
    }
    case EntityType::Crab: {
        const float remaining=static_cast<float>(e.distance-game.GetDistance());
        const float approach=18-std::clamp(remaining,0.0f,18.0f);
        const float fleeing=remaining>0?approach/18:0;
        // Run forward along the bank, reaching the original collection point exactly.
        if(object.outcome==EntityOutcome::Pending) x-=std::clamp(remaining,0.0f,18.0f)*1.6f;
        const float phase=motion.time*11+approach*1.4f+static_cast<float>(e.id%31)*.73f;
        if(fleeing>0 && object.outcome==EntityOutcome::Pending) {
            for(int i=0;i<3;++i) {
                const float t=std::fmod(motion.time*3+i/3.0f,1.0f);
                ellipse(world,{x-21-t*18,floorY+3+t*7},1+t*2,1+t,Color4F(.80f,.76f,.52f,(1-t)*fleeing*.5f));
            }
        }
        const float bob=1.4f*std::sin(phase*2);
        x+=std::sin(phase*.5f)*3;
        y+=bob;
        if(object.outcome!=EntityOutcome::Collected)
            ellipse(world,{x,floorY+2},22,3,Color4F(.04f,.13f,.10f,.25f));
        for(int side:{-1,1}) {
            for(int leg=0;leg<3;++leg) {
                const float step=std::sin(phase+leg*2.094f+(side<0?3.14159f:0));
                const float knee=side*(17.0f+leg*2);
                const float foot=side*(21.0f+leg*3)+step*3;
                const float footY=-11-bob+std::max(0.0f,step)*4;
                line(side*8.0f,leg*2.0f-3,knee,-4+leg*2,1.8f,Color4F(.94f,.43f,.25f,1));
                line(knee,-4+leg*2,foot,footY,1.8f,Color4F(.77f,.31f,.19f,1));
            }
            const float claw=std::sin(phase*.45f+side)*3;
            const float opening=2+2*(1+std::sin(phase*.65f+side));
            const float cx=side*21.0f, cy=12+claw;
            line(side*10.0f,3,side*16.0f,8+claw,2.3f,Color4F(1,.58f,.31f,1));
            line(side*16.0f,8+claw,cx,cy,2.3f,Color4F(1,.58f,.31f,1));
            oval(cx,cy,5,5,Color4F(1,.57f,.32f,1));
            line(cx-3,cy+2,cx-opening,cy+8,2,Color4F(1,.66f,.40f,1));
            line(cx+3,cy+2,cx+opening,cy+8,2,Color4F(1,.66f,.40f,1));
        }
        oval(0,0,15,10,Color4F(.96f,.40f,.25f,1));
        oval(-2,3,10,5,Color4F(1,.61f,.36f,1));
        const float blink=std::sin(motion.time*1.9f+static_cast<float>(e.id))>.985f?.5f:3.5f;
        for(int side:{-1,1}) {
            line(side*5.0f,6,side*6.0f,12,1.5f,Color4F(.94f,.43f,.25f,1));
            oval(side*6.0f,12,3,blink,Color4F(1,.96f,.77f,1));
            oval(side*6.0f+std::sin(phase*.25f)+fleeing*1.2f,12,1.3f,std::min(1.8f,blink),Ink);
        }
        break;
    }
    case EntityType::Fish: case EntityType::RareFish: {
        const Color4F fish = e.type == EntityType::RareFish ? Color4F(1,.74f,.30f,1) : Color4F(.42f,.81f,.83f,1);
        const float fin=std::sin(motion.time*9+static_cast<float>(e.distance))*4;
        Vec2 tail[]={pos(11,0),pos(26,11+fin),pos(23,fin),pos(26,-11+fin)};
        world->drawSolidPoly(tail,4,tint(fish));
        oval(0,0,18,11,fish);
        oval(-2,-4,13,5,Color4F(.88f,.93f,.71f,1));
        oval(-9,3,4,4,Color4F(1,1,.9f,1));
        oval(-10,3,2,2,Ink);
        line(4,-4,6,4,1.4f,Color4F(.20f,.51f,.51f,1));
        break;
    }
    case EntityType::Shield: {
        const float pulse=.5f+.5f*std::sin(motion.time*4+static_cast<float>(e.id));
        oval(0,0,25+pulse*5,29+pulse*5,Color4F(.41f,.79f,.90f,.10f+pulse*.12f));
        Vec2 shape[]={pos(-18,19),pos(18,19),pos(14,-6),pos(0,-23),pos(-14,-6)};
        world->drawSolidPoly(shape,5,tint(Color4F(.41f,.79f,.90f,1)));
        line(0,-11,0,10,2,Color4F(.89f,1,1,1));
        line(-8,1,8,1,2,Color4F(.89f,1,1,1));
        break;
    }
    case EntityType::Magnet:
        for(int i=0;i<3;++i) {
            const float t=std::fmod(motion.time*1.2f+i/3.0f,1.0f);
            for(int side:{-1,1}) {
                const float dx=side*(12+23*(1-t));
                line(dx,5+t*6,dx+side*4,7+t*6,1.3f,Color4F(1,.74f,.36f,t*.7f));
            }
        }
        line(-10,14,-10,-7,5,Color4F(1,.47f,.37f,1));
        line(10,14,10,-7,5,Color4F(1,.47f,.37f,1));
        line(-10,-7,10,-7,5,Color4F(1,.47f,.37f,1));
        line(-10,12,-10,17,5,Color4F(.9f,.96f,.85f,1));
        line(10,12,10,17,5,Color4F(.9f,.96f,.85f,1));
        break;
    }
    if(object.outcome==EntityOutcome::Collected) {
        const float t=object.effectTime / RunPresentation::CollectionSeconds;
        for(int i=0;i<5;++i) {
            const float angle=i*1.2566f;
            ellipse(world,{x+std::cos(angle)*(8+24*t),y+std::sin(angle)*(8+24*t)},
                    2.5f*(1-t),2.5f*(1-t),Color4F(1,.9f,.48f,1-t));
        }
    }
}

void MainScene::drawWorld() {
    world->clear(); menuActor->clear(); menuMask->clear();
    for(auto sprite:artPool) sprite->setVisible(false);
    artUsed=0;
    const float h=viewHeight;
    const bool opening=game.GetState()==GameState::Menu && !shop && !goals && !help && !settings;
    scenery->setVisible(!story.active);
    const float zoom=opening && !chaseEntry?entrance.Zoom():1;
    const float camera=opening && !chaseEntry?entrance.CameraProgress():1;
    scenery->setScale(zoom);
    const Vec2 focus(245+75*entrance.Progress(),h*.29f+36+64*entrance.Progress());
    const Vec2 desired=(Vec2(240,h*.48f)-focus*zoom)*(1-camera);
    scenery->setPosition({std::clamp(desired.x,480*(1-zoom),0.0f),std::clamp(desired.y,h*(1-zoom),0.0f)});
    if(menuHint) menuHint->setOpacity(static_cast<uint8_t>(255*std::clamp((entrance.time-2.4f)/.5f,0.0f,1.0f)));
    if(menuBrand) menuBrand->setOpacity(static_cast<uint8_t>(255*std::clamp((camera-.65f)/.35f,0.0f,1.0f)));
    const float ground=game.GetState()==GameState::Menu ? h*.29f : floorY;
    const float travel=static_cast<float>(game.GetDistance());
    drawBackdrop(ground);
    // Quiet fireflies and drifting reflections give the mangrove a little life.
    for(int i=0;i<9;++i) {
        const float x=35+i*57.0f+std::sin(motion.time*.6f+i*2)*14;
        const float y=ground+95+(i%4)*72.0f+std::sin(motion.time+i)*9;
        const float glow=.10f+.16f*(1+std::sin(motion.time*1.8f+i))*.5f;
        ellipse(world,{x,y},5,5,Color4F(.94f,.93f,.56f,glow*.3f));
        ellipse(world,{x,y},1.5f,1.5f,Color4F(.94f,.93f,.56f,glow));
    }
    const bool menu=game.GetState()==GameState::Menu;
    if(!menu) for(const auto& entity:presentation.GetObjects()) drawEntity(entity);
    const float scale=menu?1.80f:CrocScale;
    const float arrival=entrance.Progress();
    const float idleCycle=std::fmod(motion.time,7.0f);
    const float shake=menu && arrival>=1 && idleCycle>6.4f?std::sin((idleCycle-6.4f)*38):0;
    const float entry=std::clamp(entrance.time/1.1f,0.0f,1.0f);
    const float playerX=menu?(chaseEntry?-100+350*(1-std::pow(1-entry,3)):215+35*arrival+shake*2):Contact-55*scale;
    const float fall=game.GetState()==GameState::GameOver?1-std::clamp(motion.defeat/.28f,0.0f,1.0f):1;
    const float altitude=menu?0:static_cast<float>(game.GetPlayer().GetPositionY())*fall;
    const float playerY=ground+(menu?(chaseEntry?0:-90*(1-arrival)+entrance.Hop()):altitude*HeightScale);
    ellipse(world,{playerX-4,ground+1},menu?100:59-altitude*8,6-altitude,Color4F(.04f,.13f,.10f,menu?.36f*arrival:.36f-altitude*.07f));
    if(!menu && game.GetFrenzySeconds()>0) {
        for(int i=0;i<3;++i) {
            const float x=playerX-60-i*19.0f;
            world->drawSegment({x,playerY+17+i*13.0f},{x-18,playerY+17+i*13.0f},2,Color4F(1,.83f,.37f,.35f));
        }
    }
    if(menu) {
        // Reveal the swimmer above the waterline, then lower the mask as he steps ashore.
        const float waterline=chaseEntry?ground-20:ground+21-40*arrival;
        rect(menuMask,{0,waterline,480,h-waterline},Color4F(1,1,1,1));
        drawCrocodile({playerX,playerY},scale,menuActor);
        if(!chaseEntry && shake!=0) {
            const float t=(idleCycle-6.4f)/.6f;
            for(int i=0;i<4;++i) ellipse(menuActor,{playerX-35+i*25.0f+(i-2)*t*12,ground+55+std::sin(t*3.14159f)*23},
                2,3,Color4F(.70f,.92f,.89f,(1-t)*.7f));
        }
        const float splash=std::sin(arrival*3.14159265f);
        if(!chaseEntry && arrival<1) {
            for(int i=0;i<3;++i) {
                world->drawCircle({playerX-15,ground+22},1,0,40,false,45+i*16+arrival*25,4+i*2,
                    Color4F(.70f,.91f,.85f,(1-arrival)*.55f));
            }
            for(int i=0;i<8;++i) {
                const float side=i<4?-1.0f:1.0f;
                ellipse(menuActor,{playerX+side*(35+(i%4)*17+arrival*20),ground+24+splash*(15+(i%4)*12)},
                        2.5f,4,Color4F(.70f,.92f,.89f,splash*.85f));
            }
            for(int i=0;i<4;++i) {
                const float t=std::fmod(motion.time*1.5f+i*.23f,1.0f);
                world->drawCircle({playerX-45+i*26.0f,ground+22+t*14},2+t*3,0,16,false,
                    Color4F(.70f,.92f,.89f,(1-t)*(1-arrival)*.65f));
            }
        }
    } else drawCrocodile({playerX,playerY},scale);
    for(const auto& p:particles) {
        auto color=p.color; color.a*=p.life/p.duration;
        ellipse(world,p.position,p.radius*p.life/p.duration,p.radius*p.life/p.duration,color);
    }
    if(impactTime>0) {
        const float t=1-impactTime/.35f;
        if(shieldImpact) {
            const Vec2 center(playerX-7*CrocScale,playerY+25*CrocScale);
            for(int i=0;i<12;++i) {
                const float a=i*6.2831853f/12;
                const Vec2 p=center+Vec2(std::cos(a)*98,std::sin(a)*74)*(1+t*.65f);
                const Vec2 tangent(-std::sin(a),std::cos(a));
                world->drawSegment(p-tangent*(7-4*t),p+tangent*(7-4*t),2.5f,
                                   Color4F(.65f,.96f,1,(1-t)*.9f));
            }
        } else world->drawCircle({playerX+20,playerY+25},30+t*75,0,40,false,Color4F(1,.77f,.46f,(1-t)*.8f));
    }
    // Foreground vegetation frames the lane without hiding obstacles.
    for(int i=0;i<7;++i) {
        const float x=i*113.0f-std::fmod(travel*8,113.0f)-30;
        const float y=18+(i%2)*15.0f;
        const float sway=std::sin(motion.time*.8f+i)*3;
        for(int leaf=-2;leaf<=2;++leaf) {
            const Vec2 start(x,y), tip(x+leaf*11+sway,y+37-std::abs(leaf)*7);
            world->drawSegment(start,tip,3.2f,Color4F(.11f,.30f,.22f,1));
            ellipse(world,tip,5,11,Color4F(.19f,.38f,.25f,1));
        }
    }
    if(game.IsPlaying()) {
        const auto safe=Director::getInstance()->getSafeAreaRect();
        const float bottom=std::max(26.0f,safe.getMinY()+18.0f);
        const int slot=static_cast<int>(runTime/5)%MissionSlots;
        const auto& goal=MissionCatalog()[game.GetProfile().missionIds[slot]];
        const float progress=static_cast<float>(game.GetMissionProgress(slot))/goal.target;
        pill(world,{44,bottom+111,392,5},Color4F(.12f,.31f,.28f,1));
        if(progress>0) pill(world,{44,bottom+111,392*progress,5},Lime);
    }
}

void MainScene::update(float dt) {
    const auto size = Director::getInstance()->getVisibleSize();
    if (std::abs(size.height - viewHeight) > .5f) {
        viewHeight = size.height;
        floorY = viewHeight * .35f;
        rebuildUI();
    }
    // Clamp a slow frame instead of teleporting through obstacles after a stall.
    dt=std::clamp(dt,0.0f,0.1f);
    clock += dt;
    soundCooldown=std::max(0.0f,soundCooldown-dt);
    const auto before=game.GetState();
    if(story.active) {
        const int beat=story.Beat();
        story.Update(dt);
        if(!story.active) { chaseEntry=true; entrance.time=0; rebuildUI(); }
        else {
            if(beat!=story.Beat()) {
                rebuildUI();

            }
            animateStory();
        }
    }
    if(before==GameState::Menu && !story.active) {
        if(chaseEntry && entrance.time<1.1f) chaseTravel+=dt*9*std::pow(1-entrance.time/1.1f,2);
        const float previousIntro=entrance.time;
        entrance.Update(dt);
        if(!chaseEntry && previousIntro<.85f && entrance.time>=.85f)
            burst({235,viewHeight*.29f+20},Color4F(.60f,.93f,1,.8f),18,1.3f);
        if(!chaseEntry && previousIntro<1.65f && entrance.time>=1.65f)
            burst({250,viewHeight*.29f+4},Color4F(.88f,.80f,.53f,.65f),10,.8f);
    }
    if(story.active && soundEnabled && smokeStep<0) {
        if(storyMusicId<0) {
            storyMusicId=AudioEngine::play2d("audio/story-score.wav",false,.55f);
            if(storyMusicId>=0) AudioEngine::setCurrentTime(storyMusicId,story.time);
        } else {
            const float audioTime=AudioEngine::getCurrentTime(storyMusicId);
            if(audioTime>=0 && std::abs(audioTime-story.time)>.25f) AudioEngine::setCurrentTime(storyMusicId,story.time);
        }
    } else if(storyMusicId>=0) { AudioEngine::stop(storyMusicId); storyMusicId=-1; }
    if(soundEnabled && smokeStep<0 && before==GameState::Playing) {
        if(musicId<0) musicId=AudioEngine::play2d("audio/mangue-loop.wav",true,.16f);
    } else if(musicId>=0) {
        AudioEngine::stop(musicId); musicId=-1;
    }
    const bool wasGrounded=game.GetPlayer().IsGrounded();
    presentation.Sync(game.GetLevel().GetEntities());
    game.Update(dt);
    presentation.Resolve(game.GetFrameResolutions());
    presentation.Sync(game.GetLevel().GetEntities());
    const float activeDt = before == GameState::Playing ? dt : 0;
    const float previousPhase=motion.phase;
    motion.Update(dt,before==GameState::Playing || before==GameState::Menu || before==GameState::GameOver,
                  before==GameState::Menu || game.GetPlayer().IsGrounded(),
                  before!=GameState::Menu && (game.GetPlayer().IsSliding() || game.GetPlayer().IsDiving()),before==GameState::Menu?0:game.GetSpeed(),
                  game.GetPlayer().GetVelocityY(),game.GetState()==GameState::GameOver);
    const float effectDt=before==GameState::Paused?0:dt;
    pickupPulse=std::max(0.0f,pickupPulse-effectDt*4);
    impactTime=std::max(0.0f,impactTime-effectDt);
    toastTime=std::max(0.0f,toastTime-activeDt);
    coinNoticeTime=std::max(0.0f,coinNoticeTime-activeDt);
    paceNoticeTime=std::max(0.0f,paceNoticeTime-activeDt);
    for(auto& p:particles) {
        p.life-=effectDt; p.velocity.y-=110*effectDt; p.position+=p.velocity*effectDt;
    }
    particles.erase(std::remove_if(particles.begin(),particles.end(),[](const Particle& p){return p.life<=0;}),particles.end());
    if(before==GameState::Playing) {
        if(!wasGrounded && game.GetPlayer().IsGrounded())
            burst({Contact-60,floorY+4},Color4F(.83f,.80f,.53f,.75f),12,.85f);
        dustTime+=dt;
        const bool footstep=motion.phase<previousPhase || (previousPhase<3.14159265f && motion.phase>=3.14159265f);
        const bool slidingDust=game.GetPlayer().IsSliding() && dustTime>.08f;
        if(game.GetPlayer().IsGrounded() && (slidingDust || (!game.GetPlayer().IsSliding() && footstep))) {
            dustTime=0;
            burst({Contact-106,floorY+5},Color4F(.74f,.73f,.49f,.45f),2,.35f);
        }
    }
    for(const auto& resolution:game.GetFrameResolutions()) {
        const auto& e=resolution.entity;
        if(resolution.outcome==EntityOutcome::Collected) {
            const bool coin=e.type==EntityType::Coin;
            if(coin) {
                if(coinNoticeTime<=0) coinNoticeCount=0;
                ++coinNoticeCount; coinNoticeTime=1.6f;
            }
            burst({Contact,floorY+static_cast<float>(e.height)*HeightScale},
                  coin?Color4F(1,.84f,.34f,1):Color4F(.62f,.96f,.78f,1),coin?7:12);
            pickupPulse=1;
            const bool food=e.type==EntityType::Crab || e.type==EntityType::Fish || e.type==EntityType::RareFish;
            if(food) { motion.Eat(); sound("chomp"); }
            else if(soundCooldown<=0) { sound(coin?"coin":"food"); soundCooldown=.07f; }
        } else if(resolution.outcome==EntityOutcome::Shielded || resolution.outcome==EntityOutcome::Hit) {
            shieldImpact=resolution.outcome==EntityOutcome::Shielded;
            motion.Hit();
            burst({Contact-30,floorY+32},shieldImpact?Color4F(.45f,.92f,1,1):Color4F(1,.70f,.38f,1),20,1.3f);
            impactTime=.35f;
            sound("hit");
        }
    }
    presentation.Update(activeDt, game.GetDistance(), Contact, PixelsPerMeter);
    if (before == GameState::Playing) runTime += dt;
    if (before!=game.GetState()) {
        save(); rebuildUI();
        if(game.GetState()==GameState::GameOver && game.GetProgressRewards().coins>0) sound("reward");
    }
    if(game.GetState()==GameState::GameOver && !shop && !goals && !help && !settings && !resultGoals && motion.defeat>=.72f)
        rebuildUI();
    for(auto& message:game.TakeMessages()) {
        feedback=std::move(message); feedbackTime=2.5f;
    }
    feedbackTime=std::max(0.0f,feedbackTime-dt);
    if (score) score->setString(whole(game.GetScore()));
    if (distance) distance->setString(whole(game.GetDistance())+" m");
    if (coins) {
        coins->setString(whole(game.GetRunCoins()));
        coins->setScale(1+pickupPulse*.13f);
    }
    if(coinNotice) {
        coinNotice->setString("+"+whole(coinNoticeCount)+(coinNoticeCount==1?" MOEDA":" MOEDAS"));
        coinNotice->setOpacity(static_cast<uint8_t>(255*std::clamp(coinNoticeTime*2,0.0f,1.0f)));
    }
    if(missionHud) {
        const int slot=static_cast<int>(runTime/5)%MissionSlots;
        const auto& goal=MissionCatalog()[game.GetProfile().missionIds[slot]];
        missionHud->setString(goal.description+std::string("  ")+whole(game.GetMissionProgress(slot))+"/"+whole(goal.target)+"  +"+whole(goal.reward));
        int newlyCompleted=0, newBonus=0;
        for(int i=0;i<MissionSlots;++i) {
            const auto& card=MissionCatalog()[game.GetProfile().missionIds[i]];
            if(!(notifiedMissions&(1u<<i)) && game.GetMissionProgress(i)>=card.target) {
                notifiedMissions|=1u<<i;
                ++newlyCompleted; newBonus+=card.reward;
            }
        }
        if(newlyCompleted>0) {
            missionToast->setString((newlyCompleted==1?std::string("MISS\u00c3O FEITA!"):whole(newlyCompleted)+" MISS\u00d5ES FEITAS!")+" +"+whole(newBonus));
            toastTime=4;
            sound("reward");
        }
        missionToast->setOpacity(static_cast<uint8_t>(255*std::clamp(toastTime,0.0f,1.0f)));
    }
    if(resultGoals) {
        resultTime+=dt;
        const auto& completed=game.GetCompletedGoals();
        if(completed.empty()) resultGoals->setString("Seu progresso nas miss\u00f5es foi guardado.\nCada tentativa deixa voce mais perto!");
        else {
            const auto page=std::min(resultPage,(completed.size()-1)/2);
            std::string summary=completed[page*2];
            if(page*2+1<completed.size()) summary+="\n"+completed[page*2+1];
            if(completed.size()>2) summary+="\n"+whole(page+1)+" / "+whole((completed.size()+1)/2);
            resultGoals->setString(summary);
        }
        fitLabel(resultGoals,completed.size()>2?240:376,76);
    }
    if (pace) {
        const double meters = game.GetDistance();
        const auto points = game.GetScore();
        const int tier=points>=100000?9:points>=75000?8:points>=50000?7:points>=35000?6:points>=20000?5:meters>=1800?4:meters>=1100?3:meters>=550?2:meters>=180?1:0;
        if(tier!=lastPace) {
            if(lastPace>=0) { sound("pace"); paceNoticeTime=3; }
            lastPace=tier;
        }
        pace->setColor(paceNoticeTime>0?Color3B(220,255,145):Color3B(255,219,122));
        pace->setString(points >= 100000 ? "DOMINIO / 100.000+ PONTOS" :
                        points >= 75000 ? "ELITE / RUMO AOS 100 MIL" :
                        points >= 50000 ? "MUITO DIFICIL / 50.000+" :
                        points >= 35000 ? "DIFICIL / PRESSAO CRESCENTE" :
                        points >= 20000 ? "PRESSAO / 20.000+ PONTOS" :
                        meters < 180 ? "RITMO 1 / AQUECIMENTO" :
                        meters < 550 ? "RITMO 2 / SEQUENCIAS DUPLAS" :
                        meters < 1100 ? "RITMO 3 / DESAFIOS TRIPLOS" :
                        meters < 1800 ? "RITMO 4 / MANGUE ACELERADO" :
                                        "RITMO 5 / CORRIDA SELVAGEM");
    }
    if (status) {
        std::string info;
        if (!shop && game.GetFrenzySeconds()>0) info+="DIST x"+whole(game.GetFrenzyMultiplier())+" "+whole(std::ceil(game.GetFrenzySeconds()))+"s  ";
        if (!shop && game.GetShieldSeconds()>0) info+="ESCUDO "+whole(std::ceil(game.GetShieldSeconds()))+"s  ";
        if (!shop && game.GetMagnetSeconds()>0) info+="IMA "+whole(std::ceil(game.GetMagnetSeconds()))+"s  ";
        if (!shop && game.GetCombo()>0) info+="COMBO "+whole(game.GetCombo())+" / x"+whole(game.GetMultiplier());
        if(info.empty() && feedbackTime>0) info=feedback;
        if(shop) info=saveWarning.empty()?shopNotice:saveWarning;
        status->setString(info);
        if(status->getContentSize().width>440) status->setScale(440/status->getContentSize().width);
        else status->setScale(1);
    }
    if (gestureHint) {
        gestureHint->setOpacity(static_cast<uint8_t>(255 * std::clamp(7.0f-runTime, 0.0f, 1.0f)));
    }
    drawWorld();
    smokeTick(dt);
}

void MainScene::capture(const std::string& name) {
#ifndef NDEBUG
    const auto path=(smokeDirectory/name).generic_string();
    utils::captureScreen([this](bool ok,std::string_view) {
        if(!ok) finishSmoke(false,"Screenshot capture failed");
    },path);
#endif
}
void MainScene::smokeTouch(Vec2 from, Vec2 to) {
#ifndef NDEBUG
    Touch touch;
    auto begin = Director::getInstance()->convertToUI(from);
    touch.setTouchInfo(123, begin.x, begin.y);
    EventTouch event;
    event.setTouches({&touch});
    event.setEventCode(EventTouch::EventCode::BEGAN);
    _eventDispatcher->dispatchEvent(&event);
    auto end = Director::getInstance()->convertToUI(to);
    touch.setTouchInfo(123, end.x, end.y);
    event.setEventCode(EventTouch::EventCode::MOVED);
    _eventDispatcher->dispatchEvent(&event);
    event.setEventCode(EventTouch::EventCode::ENDED);
    _eventDispatcher->dispatchEvent(&event);
#endif
}
void MainScene::smokePauseGesture() {
#ifndef NDEBUG
    Touch first, second;
    const auto a=Director::getInstance()->convertToUI({210, floorY+130});
    const auto b=Director::getInstance()->convertToUI({285, floorY+130});
    first.setTouchInfo(124,a.x,a.y); second.setTouchInfo(125,b.x,b.y);
    EventTouch event;
    event.setTouches({&first,&second});
    event.setEventCode(EventTouch::EventCode::BEGAN);
    _eventDispatcher->dispatchEvent(&event);
    event.setEventCode(EventTouch::EventCode::ENDED);
    _eventDispatcher->dispatchEvent(&event);
#endif
}
void MainScene::finishSmoke(bool success, const std::string& reason) {
#ifndef NDEBUG
    if(smokeStep==99) return;
    std::ofstream report(smokeDirectory/"result.txt");
    report<<(success?"PASS":"FAIL")<<"\n"<<reason<<"\n";
    smokeStep=99; smokeTime=0;
#endif
}
void MainScene::smokeTick(float dt) {
#ifndef NDEBUG
    if(smokeStep<0) return;
    smokeTime+=dt;
    if(smokeStep==99) {
        if(smokeTime>1) Director::getInstance()->end();
        return;
    }
    if(smokeTime>57) { finishSmoke(false,"Timed out"); return; }
    const double d=game.GetDistance();
    if(smokeStep==0 && smokeTime>.25f) { capture("27-menu-water.png"); smokeStep=60; }
    else if(smokeStep==60 && smokeTime>1.45f) { capture("28-menu-emerge.png"); smokeStep=61; }
    else if(smokeStep==61 && smokeTime>3.1f) { capture("39-menu-wave.png"); smokeStep=62; }
    else if(smokeStep==62 && smokeTime>3.75f) { capture("01-menu.png"); smokeStep=40; }
    else if(smokeStep==40 && smokeTime>0.55f) {
        const float bottom=std::max(26.0f,Director::getInstance()->getSafeAreaRect().getMinY()+18.0f);
        smokeTouch({362,bottom+41},{362,bottom+41});
        if(!settings || game.IsPlaying()) { finishSmoke(false,"Settings footer failed"); return; }
        smokeStep=113;
    } else if(smokeStep==113) {
        capture("60-settings.png"); smokeStep=114;
    } else if(smokeStep==114) {
        const float bottom=std::max(26.0f,Director::getInstance()->getSafeAreaRect().getMinY()+18.0f);
        const bool previous=soundEnabled;
        smokeTouch({240,viewHeight*.52f-5},{240,viewHeight*.52f-5});
        if(soundEnabled==previous || !settings) { finishSmoke(false,"Settings audio toggle failed"); return; }
        act(Action::Sound); act(Action::Back);
        smokeTouch({240,viewHeight*.45f},{240,viewHeight*.45f+70});
        if(game.GetState()!=GameState::Menu) { finishSmoke(false,"Menu swipe started a run"); return; }
        smokeTouch({240,viewHeight*.45f},{240,viewHeight*.45f});
        if(!game.IsPlaying()) { finishSmoke(false,"Menu tap did not start"); return; }
        act(Action::Menu);
        smokeTouch({96,bottom+41},{96,bottom+41}); smokeStep=41;
    }
    else if(smokeStep==41 && smokeTime>0.7f) {
        if(!help) { finishSmoke(false,"Help did not open"); return; }
        capture("20-help.png"); smokeStep=42;
    }
    else if(smokeStep==42 && smokeTime>0.85f) { act(Action::Back); smokeStep=1; }
    else if(smokeStep==1 && smokeTime>0.9f) {
        start(false);
        const auto safe=Director::getInstance()->getSafeAreaRect();
        const float top=std::min(viewHeight-28.0f,safe.getMaxY()-24.0f);
        smokeTouch({434,top-95},{434,top-95});
        if(game.GetState()!=GameState::Paused) { finishSmoke(false,"Visible pause failed"); return; }
        act(Action::Pause);
        game.Spawn(EntityType::Coin,2,0.5);
        game.Spawn(EntityType::Crab,4,0.5);
        game.Spawn(EntityType::Ground,12);
        game.Spawn(EntityType::High,40);
        game.Spawn(EntityType::Ground,72);
        game.Spawn(EntityType::Coin,12,0.5); // Must remain visible when missed in a jump.
        smokeStep=2;
    } else if(smokeStep==2) {
        if(!smokeJump && d>9.2) { smokeTouch({350,floorY+140},{350,floorY+205}); smokeJump=true; }
        if(smokeJump && game.GetPlayer().GetPositionY()>1.5 && smokeStep==2) {
            capture("02-jump.png"); smokeStep=3;
        }
    } else if(smokeStep==3) {
        if(!smokePersistence && d>13) {
            bool rootVisible=false, missedCoinVisible=false;
            for(const auto& o:presentation.GetObjects()) {
                if(o.entity.distance==12 && o.entity.type==EntityType::Ground && o.outcome==EntityOutcome::Passed) rootVisible=true;
                if(o.entity.distance==12 && o.entity.type==EntityType::Coin && o.outcome==EntityOutcome::Missed) missedCoinVisible=true;
            }
            if(!rootVisible || !missedCoinVisible) { finishSmoke(false,"Objects disappeared at the player"); return; }
            smokePersistence=true; capture("05-objects-behind.png");
        }
        if(!smokeSlide && d>37.2) { smokeTouch({350,floorY+160},{350,floorY+95}); smokeSlide=true; }
        if(d>40.5) {
            if(!game.IsPlaying() || game.GetObstaclesPassed()!=2 || game.GetRunCoins()!=1 || game.GetFoods()!=1) {
                finishSmoke(false,"Obstacle or collection integration failed"); return;
            }
            smokePauseGesture();
            if(game.GetState()!=GameState::Paused) { finishSmoke(false,"Two-finger pause failed"); return; }
            pausedDistance=game.GetDistance(); smokeStep=4; capture("03-pause.png");
        }
    } else if(smokeStep==4 && smokeTime>6) {
        if(game.GetState()!=GameState::Paused || game.GetDistance()!=pausedDistance) {
            finishSmoke(false,"Background pause moved the simulation"); return;
        }
        suspend();
        smokeTouch({240,viewHeight*.52f+80},{240,viewHeight*.52f+150}); smokeStep=5;
    } else if(smokeStep==5 && d>56) {
        if(!game.IsPlaying() || !game.GetPlayer().IsSliding()) {
            finishSmoke(false,"Slow slide ended before visual clearance"); return;
        }
        capture("08-slide-clearance.png"); smokeStep=10;
    } else if(smokeStep==10 && game.GetState()==GameState::GameOver) {
        if(motion.defeat<.1f) return;
        capture("40-impact.png"); smokeStep=63;
    } else if(smokeStep==63 && motion.defeat>.42f) {
        capture("41-collapse.png"); smokeStep=64;
    } else if(smokeStep==64 && motion.defeat>.8f) {
        capture("04-game-over.png");
        const auto bank=game.GetProfile().coins;
        save();
        Profile restored; std::string error;
        if(bank!=1 || !restored.Load(savePath,error) || restored.coins!=bank) {
            finishSmoke(false,"Profile persistence failed"); return;
        }
        smokeStep=6;
    } else if(smokeStep==6) {
        smokeTouch({240,viewHeight*.52f+100},{240,viewHeight*.52f+170});
        if(!game.IsPlaying() || game.GetDistance()!=0 || game.GetProfile().coins!=1) {
            finishSmoke(false,"Restart failed"); return;
        }
        game.EndRun(); start(false);
        Level preview; preview.Reset(0);
        for(const auto& entity:preview.GetEntities())
            if(entity.distance<65) game.Spawn(entity.type,entity.distance,entity.height);
        smokeStep=7;
    } else if(smokeStep==7 && d>19) {
        capture("06-coin-arc.png"); smokeStep=8;
    } else if(smokeStep==8 && d>24) {
        smokeTouch({350,floorY+140},{350,floorY+205}); smokeStep=9;
    } else if(smokeStep==9 && d>43) {
        if(!game.IsPlaying() || game.GetObstaclesPassed()!=1 || game.GetRunCoins()!=3 || game.GetFoods()!=1) {
            finishSmoke(false,"Distance-based jump or coin arc failed"); return;
        }
        capture("07-clearance.png");
        smokeStep=11;
    } else if(smokeStep==11) {
        // Art inspection fixture only; not a generated playable sequence.
        game.EndRun(); start(false);
        game.Spawn(EntityType::Log,10);
        game.Spawn(EntityType::Rock,21);
        game.Spawn(EntityType::Vine,32);
        smokeStep=12;
    } else if(smokeStep==12) {
        capture("09-new-obstacles.png");
        smokeStep=13;
    } else if(smokeStep==13) {
        game.EndRun(); game.ReturnToMenu(); game.GetProfile().coins=45000;
        act(Action::Shop); smokeStep=14;
    } else if(smokeStep==14) {
        capture("10-shop-looks.png"); smokeStep=19;
    } else if(smokeStep==19) {
        act(Action::Next); smokeStep=15;
    } else if(smokeStep==15) {
        const float center=viewHeight*.52f;
        smokeTouch({360,center-63},{360,center-63}); // Buy/equip the bandana from page two.
        if(game.GetProfile().coins!=30000 || game.GetProfile().equipped!=4) {
            finishSmoke(false,"Shop cosmetic purchase failed"); return;
        }
        smokeStep=16;
    } else if(smokeStep==16) {
        capture("11-shop-cosmetics.png"); smokeStep=20;
    } else if(smokeStep==20) {
        const float center=viewHeight*.52f;
        smokeTouch({344,center+167},{344,center+167}); smokeStep=17;
    } else if(smokeStep==17) {
        const float center=viewHeight*.52f;
        smokeTouch({360,center+47},{360,center+47});
        if(!game.GetProfile().owned[ComboUpgrade] || game.GetProfile().coins!=18000) {
            finishSmoke(false,"Shop upgrade purchase failed"); return;
        }
        Profile restored; std::string error;
        if(!restored.Load(savePath,error) || !restored.owned[ComboUpgrade] || restored.equipped!=4) {
            finishSmoke(false,"Shop v2 save failed"); return;
        }
        smokeStep=18;
    } else if(smokeStep==18) {
        capture("12-shop-upgrades.png");
        smokeStep=21;
    } else if(smokeStep==21) {
        act(Action::Back);
        const float bottom=std::max(26.0f,Director::getInstance()->getSafeAreaRect().getMinY()+18.0f);
        const float menuTop=std::min(viewHeight-28.0f,Director::getInstance()->getSafeAreaRect().getMaxY()-24.0f);
        smokeTouch({240,menuTop-292},{240,menuTop-292});
        if(!goals || shop) { finishSmoke(false,"Mission entry button failed"); return; }
        smokeStep=22;
    } else if(smokeStep==22) {
        capture("13-missions.png"); smokeStep=23;
    } else if(smokeStep==23) {
        const float center=viewHeight*.52f;
        smokeTouch({344,center+167},{344,center+167});
        if(!achievementTab) { finishSmoke(false,"Achievement tab failed"); return; }
        smokeStep=24;
    } else if(smokeStep==24) {
        capture("14-achievements.png"); smokeStep=25;
    } else if(smokeStep==25) {
        const float center=viewHeight*.52f;
        smokeTouch({410,center-245},{410,center-245});
        if(goalPage!=1) { finishSmoke(false,"Achievement paging failed"); return; }
        smokeStep=26;
    } else if(smokeStep==26) {
        capture("15-achievements-page2.png"); smokeStep=27;
    } else if(smokeStep==27) {
        // Isolated fixture: one run completes all three cards and five achievements.
        auto& profile=game.GetProfile();
        profile.missionIds={{0,1,2}}; profile.missionProgress={{249,14,7}};
        profile.lifetime={{499,99,24,29,9}}; profile.achievements=0; profile.missionsCompleted=0;
        start(false);
        game.Spawn(EntityType::Coin,.1,.5); game.Spawn(EntityType::Crab,.2,.5);
        game.Spawn(EntityType::High,6);
        smokeTouch({350,floorY+160},{350,floorY+95});
        smokeStep=28;
    } else if(smokeStep==28 && d>7) {
        capture("16-mission-complete.png"); smokeStep=29;
    } else if(smokeStep==29 && d>51) {
        game.EndRun(); save(); rebuildUI();
        if(game.GetProgressRewards().coins!=1680 || game.GetProfile().coins!=19681 ||
           game.GetProfile().missionsCompleted!=3 || game.GetProfile().achievements!=31) {
            finishSmoke(false,"Mission/achievement settlement failed"); return;
        }
        Profile restored; std::string error;
        if(!restored.Load(savePath,error) || restored.missionsCompleted!=3 || restored.achievements!=31 || restored.coins!=19681) {
            finishSmoke(false,"Progress v3 reload failed"); return;
        }
        game.EndRun();
        if(game.GetProfile().coins!=19681) { finishSmoke(false,"Duplicate rewards"); return; }
        smokeStep=30;
    } else if(smokeStep==30 && motion.defeat>.8f) {
        capture("17-reward-result.png"); smokeStep=31;
    } else if(smokeStep==31) {
        const float center=viewHeight*.52f;
        smokeTouch({397,center-44},{397,center-44});
        if(resultPage!=1) { finishSmoke(false,"Result next page failed"); return; }
        smokeTouch({83,center-44},{83,center-44});
        if(resultPage!=0) { finishSmoke(false,"Result previous page failed"); return; }
        act(Action::Goals); act(Action::Missions); smokeStep=32;
    } else if(smokeStep==32) {
        capture("18-next-missions.png"); smokeStep=33;
    } else if(smokeStep==33) {
        act(Action::Achievements); smokeStep=34;
    } else if(smokeStep==34) {
        capture("19-achievements-earned.png"); smokeStep=49;
    } else if(smokeStep==49) {
        start(false);
        game.Spawn(EntityType::Fish,26,1.9);
        game.Spawn(EntityType::RareFish,34,2.0);
        smokeStep=50;
    } else if(smokeStep==50 && d>5) {
        capture("21-fish-leap.png"); smokeStep=51;
    } else if(smokeStep==51 && d>25) {
        capture("22-fish-apex.png"); smokeStep=52;
    } else if(smokeStep==52 && d>43) {
        capture("23-fish-return.png");
        smokeStep=53;
    } else if(smokeStep==53) {
        game.ReturnToMenu();
        start(false);
        game.Spawn(EntityType::Crab,20,.5);
        game.Spawn(EntityType::Crab,29,.5);
        game.Spawn(EntityType::Shield,36,.7);
        game.Spawn(EntityType::Magnet,44,.7);
        smokeStep=54;
    } else if(smokeStep==54 && d>8) {
        capture("24-living-items-a.png"); smokeStep=55;
    } else if(smokeStep==55 && d>11) {
        capture("25-living-items-b.png");
        smokeStep=56;
    } else if(smokeStep==56 && d>20.7) {
        if(motion.Mouth()<.1f) { finishSmoke(false,"Food did not trigger bite"); return; }
        capture("26-bite.png");
        smokeStep=70;
    } else if(smokeStep==70) {
        game.ReturnToMenu(); start(false);
        smokeTouch({350,floorY+130},{350,floorY+205}); smokeStep=71;
    } else if(smokeStep==71 && d>6) {
        smokeTouch({350,floorY+205},{350,floorY+130});
        if(!game.GetPlayer().IsDiving()) { finishSmoke(false,"Air swipe did not initiate dive"); return; }
        smokeStep=72;
    } else if(smokeStep==72) {
        capture("29-air-slide.png"); smokeStep=73;
    } else if(smokeStep==73 && game.GetPlayer().IsGrounded()) {
        if(!game.GetPlayer().IsSliding()) { finishSmoke(false,"Dive did not land sliding"); return; }
        capture("30-dive-landing.png");
        smokeStep=74;
    } else if(smokeStep==74) {
        smokeTouch({350,floorY+130},{350,floorY+205});
        if(game.GetPlayer().IsGrounded() || game.GetPlayer().IsSliding()) {
            finishSmoke(false,"Up swipe did not cancel slide"); return;
        }
        smokeStep=75;
    } else if(smokeStep==75 && game.GetPlayer().GetPositionY()>.8) {
        capture("31-slide-to-jump.png");
        smokeStep=76;
    } else if(smokeStep==76) {
        game.ReturnToMenu(); start(false);
        game.Spawn(EntityType::Shield,2,.7);
        game.Spawn(EntityType::Ground,16,0);
        smokeStep=77;
    } else if(smokeStep==77 && d>3) {
        if(game.GetShieldSeconds()<=0) { finishSmoke(false,"Shield pickup failed"); return; }
        capture("32-shield-shell-a.png"); smokeStep=78;
    } else if(smokeStep==78 && d>9) {
        capture("33-shield-shell-b.png"); smokeStep=79;
    } else if(smokeStep==79 && d>16.1) {
        if(!game.IsPlaying() || game.GetShieldSeconds()>0 || !shieldImpact) {
            finishSmoke(false,"Shield did not absorb obstacle"); return;
        }
        capture("34-shield-shatter.png");
        act(Action::Menu); smokeStep=80;
    } else if(smokeStep==80) {
        start(false);
        while(game.GetDistance()<EnvironmentSegmentMeters+EnvironmentTransitionMeters+10) game.Update(.1);
        smokeStep=81;
    } else if(smokeStep==81) {
        if(EnvironmentAtDistance(d).current!=1) { finishSmoke(false,"Village cycle failed"); return; }
        capture("35-village-run.png");
        smokeStep=85;
    } else if(smokeStep==85) {
        while(game.GetDistance()<2*EnvironmentSegmentMeters+EnvironmentTransitionMeters+10) game.Update(.1);
        smokeStep=82;
    } else if(smokeStep==82) {
        if(EnvironmentAtDistance(d).current!=2) { finishSmoke(false,"Forest cycle failed"); return; }
        capture("36-forest-run.png"); smokeStep=86;
    } else if(smokeStep==86) {
        while(game.GetDistance()<3*EnvironmentSegmentMeters+EnvironmentTransitionMeters*.5) game.Update(.1);
        smokeStep=83;
    } else if(smokeStep==83) {
        const auto blend=EnvironmentAtDistance(d);
        if(blend.previous!=2 || blend.current!=0 || blend.progress<=0 || blend.progress>=1) {
            finishSmoke(false,"Environment transition failed"); return;
        }
        capture("38-environment-transition.png"); smokeStep=87;
    } else if(smokeStep==87) {
        while(game.GetDistance()<3*EnvironmentSegmentMeters+EnvironmentTransitionMeters+10) game.Update(.1);
        smokeStep=84;
    } else if(smokeStep==84) {
        if(EnvironmentAtDistance(d).current!=0 || EnvironmentAtDistance(d).progress!=1) {
            finishSmoke(false,"Environment wrap failed"); return;
        }
        capture("37-mangrove-return.png"); smokeStep=88;
    } else if(smokeStep==88) {
        act(Action::Menu); start(false); smokeStep=89;
    } else if(smokeStep==89) {
        if(d>2 || EnvironmentAtDistance(d).current!=0) { finishSmoke(false,"Environment restart failed"); return; }
        for(const char* file:{"art/jaca-run-v2.png","art/jaca-idle-v2.png","art/jaca-actions-v2.png","art/ground-atlas.png","art/objects-atlas.png","art/mangue.png","art/vila.png","art/floresta.png"}) {
            if(!Director::getInstance()->getTextureCache()->getTextureForKey(file)) {
                finishSmoke(false,std::string("Art not loaded: ")+file); return;
            }
        }
        act(Action::Menu); smokeStep=90;
    } else if(smokeStep==90) {
        motion.time=2.35f; drawWorld(); capture("42-idle-blink.png"); smokeStep=91;
    } else if(smokeStep==91) {
        motion.time=5.8f; drawWorld(); capture("43-idle-cap.png"); smokeStep=92;
    } else if(smokeStep==92) {
        act(Action::Shop); act(Action::Looks); act(Action::Next); act(Action::Next);
        game.GetProfile().coins=0; rebuildUI(); smokeStep=93;
    } else if(smokeStep==93) {
        capture("44-shop-last-page.png"); smokeStep=96;
    } else if(smokeStep==96) {
        const float center=viewHeight*.52f;
        smokeTouch({360,center+47},{360,center+47});
        if(game.GetProfile().coins!=0 || game.GetProfile().owned[6] || shopNotice.find("Faltam")==std::string::npos) {
            finishSmoke(false,"Unaffordable shop item changed profile or lacked feedback"); return;
        }
        smokeStep=94;
    } else if(smokeStep==94) {
        capture("45-shop-insufficient.png"); smokeStep=97;
    } else if(smokeStep==97) {
        act(Action::Previous);
        const int equipped=game.GetProfile().equipped;
        act(Action::Item1);
        if(game.GetProfile().coins!=0 || game.GetProfile().equipped!=equipped) {
            finishSmoke(false,"Owned cosmetic charged twice"); return;
        }
        act(Action::Upgrades); smokeStep=95;
    } else if(smokeStep==95) {
        capture("46-shop-upgrades-no-coins.png"); smokeStep=98;
    } else if(smokeStep==98) {
        act(Action::Back); start(false); act(Action::Menu); story=StoryIntro{}; rebuildUI(); smokeStep=110;
    } else if(smokeStep==110 && story.time>.45f) {
        if(!storyOpeningFish || storyHippo->isVisible() || storyContact->isVisible()) { finishSmoke(false,"Opening must show Dourado alone"); return; }
        capture("56-story-dourado-alone.png"); smokeStep=100;
    } else if(smokeStep==100 && story.time>StoryIntro::Start(0)+2.15f) {
        if(!storyHippo->isVisible() || storyHippo->getPositionX()<130) { finishSmoke(false,"Bento did not arrive beside Dourado"); return; }
        capture("47-story-bento.png"); smokeStep=101;
    } else if(smokeStep==101 && story.time>StoryIntro::Start(1)+1.0f) {
        capture("48-story-snack.png"); smokeStep=108;
    } else if(smokeStep==108 && story.time>StoryIntro::Start(1)+2.0f) {
        capture("54-story-temptation.png"); smokeStep=102;
    } else if(smokeStep==102 && story.time>StoryIntro::Start(2)+.65f) {
        if(!storyContact->isVisible() || storyJaca->isVisible() ||
           storyContactSprite->getTexture()!=Director::getInstance()->getTextureCache()->getTextureForKey("art/jaca-bite-sequence.png")) {
            finishSmoke(false,"Integrated bite pose missing or duplicated"); return;
        }
        capture("52-story-fish-bite.png"); smokeStep=109;
    } else if(smokeStep==109 && story.time>StoryIntro::Start(2)+1.4f) {
        capture("55-story-release.png"); smokeStep=103;
    } else if(smokeStep==103 && story.time>StoryIntro::Start(3)+.7f) {
        capture("49-story-roar.png"); smokeStep=104;
    } else if(smokeStep==104 && story.time>StoryIntro::Start(4)+1.1f) {
        capture("50-story-excuse.png"); smokeStep=111;
    } else if(smokeStep==111 && story.time>StoryIntro::Start(5)+.72f) {
        capture("58-story-jump-rise.png"); smokeStep=105;
    } else if(smokeStep==105 && story.time>StoryIntro::Start(5)+1.3f) {
        if(storyContact->isVisible() || storyHippoSprite->getTexture()!=Director::getInstance()->getTextureCache()->getTextureForKey("art/bento-guardian-run.png")) { finishSmoke(false,"Bento did not protect Dourado during chase"); return; }
        capture("51-story-escape.png"); smokeStep=112;
    } else if(smokeStep==112 && story.time>StoryIntro::Start(5)+2.24f) {
        capture("59-story-jump-land.png"); smokeStep=106;
    } else if(smokeStep==106 && story.time>StoryIntro::Start(5)+2.6f) {
        if(!storyJacaSprite || storyJacaSprite->getTexture()!=Director::getInstance()->getTextureCache()->getTextureForKey("art/jaca-run-v2.png")) {
            finishSmoke(false,"Story leap did not land into run animation"); return;
        }
        capture("53-story-run.png"); smokeStep=107;
    } else if(smokeStep==107 && !story.active && entrance.time>2.3f) {
        if(!chaseEntry) { finishSmoke(false,"Story did not continue into running entrance"); return; }
        capture("57-arrived-idle-menu.png");
        if(game.GetState()!=GameState::Menu || game.GetDistance()!=0) { finishSmoke(false,"Story advanced gameplay"); return; }
        act(Action::ReplayStory);
        if(!story.active || story.time!=0) { finishSmoke(false,"Story replay failed"); return; }
        const float top=std::min(viewHeight-20.0f,Director::getInstance()->getSafeAreaRect().getMaxY()-16);
        smokeTouch({407,top-20},{407,top-20});
        if(story.active || storyMusicId>=0) { finishSmoke(false,"Story skip did not stop presentation"); return; }
        start(false); act(Action::Menu);
        if(story.active) { finishSmoke(false,"Story repeated after run"); return; }
        for(const char* file:{"art/bento-story-v1.png","art/jaca-story-v1.png","art/story-creek-v1.png","art/jaca-actions-v2.png"})
            if(!Director::getInstance()->getTextureCache()->getTextureForKey(file)) { finishSmoke(false,"Story or defeat art missing"); return; }
        finishSmoke(true,"Gameplay, defeat image, integrated Dourado poses, bite sequence, protective Bento, chase, replay and skip passed.");
    }
    if(smokeStep>=2 && smokeStep<=3 && game.GetState()==GameState::GameOver)
        finishSmoke(false,"Unexpected early collision");
#endif
}
