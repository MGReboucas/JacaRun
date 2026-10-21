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
        smokeStep = 0;
    }
#endif
    std::string error;
    soundEnabled=UserDefault::getInstance()->getBoolForKey("sound-enabled",true);
    AudioEngine::setMaxAudioInstance(8);
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
        if (!shop && !goals && !help && p.y - touchStart.y > 35 && p.y - touchStart.y > std::abs(p.x-touchStart.x)*1.15f) {
            if (game.GetState() == GameState::Paused) act(Action::Pause);
            else act(Action::Play);
        } else if (touchAction != Action::None) {
            if (hit(p) == touchAction) act(touchAction);
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
            if (shop || goals || help) act(Action::Back); else act(Action::Pause);
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
void MainScene::button(const std::string& title, Rect bounds, Action action, bool primary) {
    pill(panels, bounds, primary ? Lime : Color4F(0.13f, 0.31f, 0.29f, 1));
    text(title, 21, {bounds.getMidX(), bounds.getMidY() + 1}, primary ? Dark : Cream, true);
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
    shop = goals = false;
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
    switch (action) {
    case Action::Play:
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
    case Action::Help: help=true; rebuildUI(); break;
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
    case Action::Menu: game.ReturnToMenu(); shop = goals = false; save(); rebuildUI(); break;
    case Action::Shop:
        if (!game.IsPlaying() && game.GetState() != GameState::Paused) { shop = true; rebuildUI(); }
        break;
    case Action::Back: shop = goals = help = false; rebuildUI(); break;
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
    case Action::Looks: upgradeShop=false; shopPage=0; rebuildUI(); break;
    case Action::Upgrades: upgradeShop=true; shopPage=0; rebuildUI(); break;
    case Action::Previous:
        if(goals) goalPage=std::max(0,goalPage-1); else if(shop) shopPage=std::max(0,shopPage-1); else if(resultPage>0) --resultPage;
        rebuildUI(); break;
    case Action::Next:
        if(goals) goalPage=std::min(1,goalPage+1); else if(shop) shopPage=std::min(2,shopPage+1); else if((resultPage+1)*2<game.GetCompletedGoals().size()) ++resultPage;
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

void MainScene::rebuildUI() {
    ui->removeAllChildren(); panels->clear(); buttons.clear(); menuHint=nullptr;
    score = distance = coins = status = pace = gestureHint = coinNotice = nullptr;
    missionHud = missionToast = resultGoals = nullptr;
    gestures.Cancel(); activeTouch = -1; touchAction = Action::None;
    const auto state = game.GetState();
    const auto safe = Director::getInstance()->getSafeAreaRect();
    const float top = std::min(viewHeight - 28.0f, safe.getMaxY() - 24.0f);
    const float bottom = std::max(26.0f, safe.getMinY() + 18.0f);
    const float center = viewHeight * .52f;
    if(help) {
        pill(panels,{20,center-320,440,650},Ink);
        text("COMO JOGAR",32,{240,center+270},Cream,true);
        text("Aprenda no seu ritmo",20,{240,center+227},Muted);
        const char* titles[]={"1. PULE","2. DESLIZE","3. PAUSE"};
        const char* details[]={"Para cima: pule os obst\u00e1culos.\nTamb\u00e9m funciona agachado.",
                              "Para baixo: deslize sob galhos.\nNo ar: des\u00e7a e deslize.",
                              "Toque em II no canto da tela.\nDois dedos tamb\u00e9m pausam."};
        for(int row=0;row<3;++row) {
            const float y=center+105-row*110.0f;
            pill(panels,{40,y-32,400,99},Color4F(.08f,.25f,.24f,1));
            text(titles[row],22,{240,y+41},Color4B(199,240,100,255),true);
            text(details[row],19,{240,y-1},Cream)->setAlignment(TextHAlignment::CENTER);
        }
        text("Moedas compram visuais e melhorias.\nPegue os peixes que saltam da \u00e1gua.",17,{240,center-186},Muted)->setAlignment(TextHAlignment::CENTER);
        button("VAMOS CORRER",{55,center-273,370,56},Action::Play,true);
        button("VOLTAR",{150,center-324,180,44},Action::Back);
        return;
    }
    if (goals) {
        const auto& profile = game.GetProfile();
        pill(panels, {20, center-320, 440, 650}, Color4F(.035f,.17f,.18f,1));
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
            const float y = center+17-row*115.0f;
            pill(panels,{36,y,408,105},Color4F(.08f,.25f,.24f,1));
            text(goal.title,21,{52,y+83},done?Color4B(199,240,100,255):Cream,true)->setAnchorPoint({0,.5f});
            text(goal.description,15,{52,y+59},Muted)->setAnchorPoint({0,.5f});
            text(done?"CONQUISTADA":whole(value)+" / "+whole(goal.target),14,{52,y+36},Cream)->setAnchorPoint({0,.5f});
            text(done?"B\u00d4NUS RECEBIDO":"+"+whole(goal.reward)+" moedas",14,{427,y+36},Color4B(255,219,122,255))->setAnchorPoint({1,.5f});
            pill(panels,{52,y+13,376,8},Color4F(.035f,.16f,.17f,1));
            const float fill=376*static_cast<float>(done?1.0:static_cast<double>(value)/goal.target);
            if(fill>.1f) pill(panels,{52,y+13,fill,8},Lime);
        }
        if(achievementTab) {
            button("<",{40,center-263,55,44},Action::Previous);
            text(whole(goalPage+1)+" / 2",15,{240,center-245},Muted);
            button(">",{385,center-263,55,44},Action::Next);
        } else text("Progresso acumulado. Novas miss\u00f5es ao concluir.",14,{240,center-242},Muted);
        button("VOLTAR",{150,center-301,180,44},Action::Back);
        text(saveWarning.empty()?"B\u00f4nus creditados ao encerrar a corrida.":saveWarning,13,{240,center-315},Muted);
        return;
    }
    if (shop) {
        pill(panels, {20, center - 320, 440, 650}, Color4F(.035f,.17f,.18f,1));
        text("LOJA DO MANGUE", 16, {240, center + 290}, Muted);
        text("Sua corrida vale mais", 30, {240, center + 254}, Cream, true);
        text(whole(game.GetProfile().coins) + " MOEDAS", 22, {240, center + 215}, Color4B(255,219,122,255),true);
        button("VISUAIS", {40,center+145,192,44}, Action::Looks,!upgradeShop);
        button("MELHORIAS", {248,center+145,192,44}, Action::Upgrades,upgradeShop);
        for (int row=0;row<3;++row) {
            const int id=(upgradeShop?CosmeticCount:shopPage*3)+row;
            if(id>=ShopItemCount || (!upgradeShop && id>=CosmeticCount)) break;
            const auto& item=AccessoryCatalog()[id];
            const auto& profile=game.GetProfile();
            const float y=center+22-row*110.0f;
            pill(panels,{36,y,408,101},Color4F(.08f,.25f,.24f,1));
            text(item.name,21,{upgradeShop?51.0f:155.0f,y+77},Cream,true)->setAnchorPoint({0,.5f});
            if(upgradeShop) text(item.description,16,{51,y+52},Muted)->setAnchorPoint({0,.5f});
            else drawCrocodile({94,y+24},.70f,panels,id);
            const bool owned=profile.owned[id], equipped=id<CosmeticCount && profile.equipped==id;
            std::string caption=owned ? (upgradeShop?"ATIVA":equipped?"EM USO":"EQUIPAR") : "COMPRAR";
            text(owned?"ADQUIRIDO":whole(item.price)+" moedas",16,{upgradeShop?51.0f:155.0f,upgradeShop?y+25:y+49},Cream)->setAnchorPoint({0,.5f});
            button(caption,{290,y+5,140,44},static_cast<Action>(static_cast<int>(Action::Item0)+row),owned || profile.coins>=item.price);
        }
        if(!upgradeShop) {
            button("<",{40,center-253,55,44},Action::Previous);
            text(whole(shopPage+1)+" / 3",15,{240,center-235},Muted);
            button(">",{385,center-253,55,44},Action::Next);
        }
        button("VOLTAR", {150, center - 293, 180, 34}, Action::Back);
        status = text(shopNotice,13,{240,center-309},Muted);
        return;
    }
    if (state == GameState::Menu) {
        button(soundEnabled?"SOM: SIM":"SOM: N\u00c3O",{302,top-24,154,44},Action::Sound);
        button("COMO JOGAR",{24,top-24,184,44},Action::Help);
        text("MANGUE EM MOVIMENTO", 17, {240, top - 40}, Muted);
        text("JACARUN", 78, {240, top - 108}, Cream, true);
        text("Seu caminho passa pelo mangue.", 21, {240, top - 173}, Cream);
        pill(panels, {85, viewHeight * .68f, 310, 43}, Color4F(.04f,.22f,.22f,.82f));
        text("RECORDE  " + whole(game.GetProfile().bestScore) + "  /  N\u00cdVEL " +
             whole(game.GetProfile().GetLevel()), 16, {240, viewHeight * .68f + 22}, Cream);
        button("VAMOS CORRER", {55, bottom +  90, 370, 65}, Action::Play, true);
        button("SEU JACA  /  " + whole(game.GetProfile().coins) + " moedas",
               {55, bottom + 22, 370, 52}, Action::Shop);
        button("MISS\u00d5ES E CONQUISTAS", {55,bottom+173,370,48}, Action::Goals);
        menuHint=text("DESLIZE PARA CIMA PARA COME\u00c7AR", 13, {240, viewHeight*.29f+125}, Cream);
        if (!saveWarning.empty()) text(saveWarning, 14, {240, bottom + 2}, Color4B(255,194,116,255));
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
        pill(panels, {22,top-183,378,110}, Color4F(.025f,.12f,.14f,.94f));
        status = text("", 22, {211, top - 106}, Cream, true);
        status->setWidth(354); status->setAlignment(TextHAlignment::CENTER);
        pace = text("", 23, {211, top - 155}, Color4B(255,219,122,255), true);
        pace->setWidth(354); pace->setAlignment(TextHAlignment::CENTER);
        pace->enableOutline(Color4B(13,47,42,210), 1);
        pill(panels, {22,bottom+105,436,70}, Color4F(.025f,.12f,.14f,.94f));
        missionHud = text("",21,{240,bottom+140},Cream);
        missionHud->setWidth(412); missionHud->setAlignment(TextHAlignment::CENTER);
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
    rect(panels, {0,0,480,viewHeight}, Color4F(.015f,.09f,.11f,.49f));
    pill(panels, {28, center - 300, 424, 560}, Color4F(.035f,.17f,.18f,.96f));
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
        text(rewards.coins>0?"OBJETIVOS CONCLU\u00cdDOS!":"MAIS UMA HISTORIA NO MANGUE",16,{240,center+225},Muted);
        text("Boa, Jaca!",45,{240,center+180},Cream,true);
        text(whole(game.GetScore())+" pontos",35,{240,center+126},Cream,true);
        text(whole(game.GetDistance())+" m  /  Recorde: "+whole(game.GetProfile().bestScore)+" pontos",17,{240,center+86},Muted);
        pill(panels,{49,center+8,382,58},Color4F(.10f,.29f,.25f,1));
        text("+"+whole(game.GetRunCoins()+rewards.coins)+" MOEDAS",25,{240,center+44},Color4B(255,219,122,255),true);
        text("Coleta: "+whole(game.GetRunCoins())+"  /  Objetivos: "+whole(rewards.coins),15,{240,center+21},Muted);
        resultGoals=text("",16,{240,center-37},Cream);
        resultGoals->setAlignment(TextHAlignment::CENTER);
        if(game.GetCompletedGoals().size()>2) {
            button("<",{59,center-66,48,44},Action::Previous);
            button(">",{373,center-66,48,44},Action::Next);
        }
        button("CORRER DE NOVO",{59,center-145,362,60},Action::Play,true);
        button("VER OBJETIVOS",{59,center-205,362,46},Action::Goals);
        button("VOLTAR AO IN\u00cdCIO",{59,center-259,362,42},Action::Menu);
        if(!saveWarning.empty()) text(saveWarning,13,{240,center-281},Color4B(255,194,116,255));
    }
}

void MainScene::drawCrocodile(Vec2 feet, float scale, DrawNode* target, int previewAccessory) {
    auto* drawing = target ? target : world;
    const bool sliding = !target && (game.GetPlayer().IsSliding() || game.GetPlayer().IsDiving()) && game.GetState() != GameState::Menu;
    const bool airborne = !target && !game.GetPlayer().IsGrounded() && game.GetState()!=GameState::Menu;
    const bool idle = target || game.GetState()==GameState::Menu;
    const float cycle = motion.phase;
    const float bob = idle ? std::sin(motion.time*2.5f)*1.8f :
        airborne || sliding ? 0 : std::abs(std::sin(cycle))*2.8f;
    const float lean = airborne ? std::clamp(static_cast<float>(game.GetPlayer().GetVelocityY())*.012f,-.12f,.12f) : -.035f*motion.slide;
    const float sx=motion.Width(), sy=motion.Height();
    auto point = [&](float x, float y) {
        return feet + Vec2((x*sx-y*lean)*scale,(y*sy+x*lean+bob)*scale);
    };
    auto oval = [&](float x, float y, float rx, float ry, Color4F c) {
        ellipse(drawing, point(x,y), rx*scale*sx, ry*scale*sy, c);
    };
    const bool shield = !target && game.GetState()!=GameState::Menu && game.GetShieldSeconds()>0;
    const float shieldPulse=.5f+.5f*std::sin(motion.time*3.8f);
    const float shieldAlpha=game.GetShieldSeconds()<3 ? .62f+.38f*(.5f+.5f*std::sin(motion.time*9)) : 1;
    const Vec2 shieldCenter=point(-7,25);
    const float shieldX=(76+shieldPulse*2)*scale;
    const float shieldY=(57-12*motion.slide+shieldPulse*2)*scale;
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
    const Color4F green(0.39f, 0.68f, 0.30f, 1), bright(0.72f, 0.88f, 0.39f, 1);
    const float mouth=target?0:motion.Mouth();
    const float wag=std::sin(cycle*.65f)*5;
    Vec2 tail[] = {point(-23, 15), point(-74, 30+wag), point(-45, 2+wag*.4f), point(-7, 7)};
    drawing->drawSolidPoly(tail, 4, green);
    oval(-9, 22, 38, 21, Color4F(.19f,.40f,.23f,1));
    oval(-9, 24, 36, 20, green);
    oval(-6, 15, 28, 10, Color4F(.63f,.79f,.35f,1));
    oval(24, 28, 33, 16, bright);
    oval(29, 17-mouth*12, 27, 8, Color4F(0.86f, 0.92f, 0.56f, 1));
    for (int i = 0; i < 5; ++i) {
        float x = -34.0f + i * 12;
        Vec2 spike[] = {point(x,37), point(x+4,47), point(x+10,38)};
        drawing->drawSolidPoly(spike, 3, Color4F(0.29f, 0.54f, 0.26f, 1));
    }
    oval(12, 45, 10, 11, bright);
    const bool blink=std::fmod(motion.time,4.3f)>4.13f;
    oval(14, 47, 6, blink?1.5f:7.0f, Color4F(0.98f,0.98f,0.83f,1));
    for (int i=0; i<5; ++i) oval(-31+i*9, 26+(i%2)*6, 2, 2, Color4F(.28f,.54f,.26f,1));
    oval(17, 47-(idle && std::fmod(motion.time,6.0f)>4.5f?2.0f:0), 2.6f, blink?1.0f:4.5f, Ink);
    oval(49, 32, 2, 2, Ink);
    if(mouth>.01f) {
        Vec2 opening[]={point(14,22),point(56,24),point(53,20-mouth*14),point(21,18-mouth*10)};
        drawing->drawSolidPoly(opening,4,Color4F(.20f,.12f,.10f,1));
        oval(37,18-mouth*10,10,2,Color4F(.91f,.42f,.36f,1));
    }
    drawing->drawSegment(point(23,22), point(55,23), scale, Ink);
    for (int i=0; i<3; ++i) {
        const float x = 31.0f + i*8;
        Vec2 tooth[] = {point(x,22),point(x+5,22),point(x+2,17)};
        drawing->drawSolidPoly(tooth,3,Color4F(1,0.98f,0.83f,1));
    }
    const float step = !idle && !airborne && !sliding ? std::sin(cycle)*8 : 0;
    const float tuck=airborne?8.0f:0;
    oval(-25+step,4+tuck+std::max(0.0f,step*.5f),13,5,green);
    oval(13-step,4+tuck+std::max(0.0f,-step*.5f),13,5,green);
    const int accessory = previewAccessory >= 0 ? previewAccessory : game.GetProfile().equipped;
    if (accessory == 1) {
        drawing->drawSegment(point(3,57), point(29,57), 4*scale, Color4F(1,0.55f,0.29f,1));
        oval(9,60,12,7,Color4F(1,0.55f,0.29f,1));
    } else if (accessory == 2) {
        oval(15,47,9,6,Ink);
        drawing->drawSegment(point(6,48),point(-6,49),2*scale,Ink);
    } else if (accessory == 3) {
        drawing->drawSegment(point(-2,57),point(29,57),4*scale,Color4F(0.85f,0.71f,0.40f,1));
        drawing->drawSegment(point(8,62),point(18,62),7*scale,Color4F(0.94f,0.83f,0.54f,1));
    } else if (accessory == 4) {
        drawing->drawSegment(point(2,55),point(28,55),5*scale,Color4F(.94f,.25f,.22f,1));
        Vec2 knot[]={point(3,55),point(-20,65),point(-14,44)};
        drawing->drawSolidPoly(knot,3,Color4F(.94f,.25f,.22f,1));
    } else if (accessory == 5) {
        Vec2 crown[]={point(-1,56),point(-3,76),point(8,66),point(16,82),point(23,66),point(34,76),point(30,56)};
        drawing->drawSolidPoly(crown,7,Color4F(1,.79f,.24f,1));
        oval(15,61,3,4,Color4F(.97f,.3f,.22f,1));
    } else if (accessory == 6) {
        drawing->drawCircle(point(20,41),32*scale,0,40,false,Color4F(.72f,.92f,1,1));
        drawing->drawSegment(point(-8,14),point(40,14),4*scale,Color4F(.83f,.88f,.89f,1));
        oval(37,59,4,7,Color4F(.78f,.95f,1,.8f));
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
    const float h=viewHeight;
    const bool opening=game.GetState()==GameState::Menu && !shop && !goals && !help;
    const float zoom=opening?entrance.Zoom():1;
    const float camera=opening?entrance.CameraProgress():1;
    scenery->setScale(zoom);
    const Vec2 focus(245+75*entrance.Progress(),h*.29f+36);
    const Vec2 desired=(Vec2(240,h*.48f)-focus*zoom)*(1-camera);
    scenery->setPosition({std::clamp(desired.x,480*(1-zoom),0.0f),std::clamp(desired.y,h*(1-zoom),0.0f)});
    if(menuHint) menuHint->setOpacity(static_cast<uint8_t>(255*std::clamp((camera-.8f)/.2f,0.0f,1.0f)));
    const float ground=game.GetState()==GameState::Menu ? h*.29f : floorY;
    // Sky reaches the real edges of the device; no letterboxed bands or reserved control area.
    for(int i=0;i<36;++i) {
        const float t=i/35.0f;
        rect(world,{0,h*i/36,480,h/36+1},Color4F(.60f-.48f*t,.66f-.37f*t,.43f-.13f*t,1));
    }
    const Vec2 sun(351,h*.77f);
    ellipse(world,sun,64,64,Color4F(.94f,.81f,.43f,.10f));
    ellipse(world,sun,48,48,Color4F(.98f,.85f,.49f,1));
    ellipse(world,{sun.x-6,sun.y+3},40,39,Color4F(1,.9f,.61f,.45f));
    const float travel=static_cast<float>(game.GetDistance());
    for(int layer=0;layer<3;++layer) {
        const float spacing=164.0f+layer*53;
        const float shift=std::fmod(travel*(1.05f+layer*1.2f),spacing);
        const float base=ground+130-layer*40.0f;
        const Color4F trunk=layer==0 ? Color4F(.30f,.49f,.39f,1) :
            layer==1 ? Color4F(.17f,.36f,.29f,1) : Color4F(.10f,.27f,.23f,1);
        const Color4F leaves=layer==0 ? Color4F(.30f,.50f,.38f,1) :
            layer==1 ? Color4F(.16f,.38f,.28f,1) : Color4F(.09f,.28f,.22f,1);
        for(int i=-1;i<5;++i) {
            const float x=i*spacing-shift+37*layer;
            const float canopy=h*(.70f+layer*.10f)+(i%2)*32;
            world->drawSegment({x,base},{x+29,canopy},6+layer*3.0f,trunk);
            world->drawSegment({x+12,base+(canopy-base)*.57f},{x-32,canopy-36},4+layer*2.0f,trunk);
            world->drawSegment({x+19,canopy-110},{x+71,canopy-35},5+layer*2.0f,trunk);
            for(int r=-1;r<=1;++r) {
                world->drawSegment({x+5,base+48},{x+r*27.0f,base+10},3+layer*1.5f,trunk);
                world->drawSegment({x+r*27.0f,base+10},{x+r*42.0f,base-13},3+layer*1.5f,trunk);
            }
            ellipse(world,{x+19,canopy+17},82+layer*8.0f,40+layer*4.0f,leaves);
            ellipse(world,{x-44,canopy+34},59,34,leaves);
            ellipse(world,{x+75,canopy+36},64,35,leaves);
            ellipse(world,{x+14,canopy+52},64,32,leaves);
        }
        rect(world,{0,ground+36,480,70+layer*8.0f},Color4F(.43f,.59f,.43f,.06f));
    }
    // Tidal channel, reflections and muddy bank stay visible below the running lane.
    rect(world,{0,ground,480,61},Color4F(.24f,.46f,.38f,1));
    for(int i=0;i<11;++i) {
        const float x=std::fmod(i*67.0f-travel*3+100000,560)-40;
        const float y=ground+11+(i%4)*13.0f;
        world->drawSegment({x,y},{x+21+(i%3)*12.0f,y},1,Color4F(.67f,.73f,.47f,.36f));
    }
    rect(world,{0,0,480,ground},Color4F(.21f,.30f,.20f,1));
    rect(world,{0,ground-69,480,69},Color4F(.42f,.43f,.25f,1));
    rect(world,{0,ground-8,480,8},Color4F(.62f,.65f,.34f,1));
    rect(world,{0,ground-3,480,3},Color4F(.76f,.75f,.43f,1));
    for(int i=0;i<8;++i) {
        const float x=i*91.0f-std::fmod(travel*PixelsPerMeter,91.0f)-45;
        ellipse(world,{x,ground-32},21,3,Color4F(.29f,.35f,.21f,1));
        world->drawSegment({x+41,ground-21},{x+52,ground-18},1.8f,Color4F(.70f,.64f,.37f,1));
        world->drawSegment({x+17,ground-3},{x+12,ground+6},1,Color4F(.40f,.57f,.28f,1));
    }
    for(int i=0;i<5;++i) {
        const float x=i*147.0f-std::fmod(travel*6,147.0f)-45;
        const float y=ground*.43f+(i%2)*20;
        ellipse(world,{x,y},67,17,Color4F(.11f,.27f,.25f,1));
        ellipse(world,{x+4,y+4},48,8,Color4F(.20f,.39f,.31f,1));
        world->drawSegment({x-23,y+5},{x+13,y+5},1,Color4F(.48f,.58f,.38f,.7f));
        ellipse(world,{x+61,y+29},10,5,Color4F(.48f,.49f,.30f,1));
    }
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
    const float playerX=menu?215+35*arrival+shake*2:Contact-55*scale;
    const float playerY=ground+(menu?-65*(1-arrival):static_cast<float>(game.GetPlayer().GetPositionY())*HeightScale);
    const float altitude=menu?0:static_cast<float>(game.GetPlayer().GetPositionY());
    ellipse(world,{playerX-4,ground+1},menu?100:59-altitude*8,6-altitude,Color4F(.04f,.13f,.10f,menu?.36f*arrival:.36f-altitude*.07f));
    if(!menu && game.GetFrenzySeconds()>0) {
        for(int i=0;i<3;++i) {
            const float x=playerX-60-i*19.0f;
            world->drawSegment({x,playerY+17+i*13.0f},{x-18,playerY+17+i*13.0f},2,Color4F(1,.83f,.37f,.35f));
        }
    }
    if(menu) {
        // Reveal the swimmer above the waterline, then lower the mask as he steps ashore.
        const float waterline=ground+21-40*arrival;
        rect(menuMask,{0,waterline,480,h-waterline},Color4F(1,1,1,1));
        drawCrocodile({playerX,playerY},scale,menuActor);
        if(shake!=0) {
            const float t=(idleCycle-6.4f)/.6f;
            for(int i=0;i<4;++i) ellipse(menuActor,{playerX-35+i*25.0f+(i-2)*t*12,ground+55+std::sin(t*3.14159f)*23},
                2,3,Color4F(.70f,.92f,.89f,(1-t)*.7f));
        }
        const float splash=std::sin(arrival*3.14159265f);
        if(arrival<1) {
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
        pill(world,{28,bottom+102,424,54},Color4F(.035f,.17f,.18f,.94f));
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
    if(before==GameState::Menu) entrance.Update(dt);
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
    motion.Update(dt,before==GameState::Playing || before==GameState::Menu,
                  before==GameState::Menu || game.GetPlayer().IsGrounded(),
                  before!=GameState::Menu && (game.GetPlayer().IsSliding() || game.GetPlayer().IsDiving()),before==GameState::Menu?0:game.GetSpeed());
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
        if(game.GetPlayer().IsGrounded() && dustTime>.10f) {
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
    if(smokeTime>35) { finishSmoke(false,"Timed out"); return; }
    const double d=game.GetDistance();
    if(smokeStep==0 && smokeTime>.25f) { capture("27-menu-water.png"); smokeStep=60; }
    else if(smokeStep==60 && smokeTime>1.45f) { capture("28-menu-emerge.png"); smokeStep=61; }
    else if(smokeStep==61 && smokeTime>3.75f) { capture("01-menu.png"); smokeStep=40; }
    else if(smokeStep==40 && smokeTime>0.55f) { act(Action::Help); smokeStep=41; }
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
        smokeTouch({240,bottom+196},{240,bottom+196});
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
    } else if(smokeStep==30) {
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
        finishSmoke(true,"Gameplay, save, swipe chain and animated shield absorption passed.");
    }
    if(smokeStep>=2 && smokeStep<=3 && game.GetState()==GameState::GameOver)
        finishSmoke(false,"Unexpected early collision");
#endif
}
