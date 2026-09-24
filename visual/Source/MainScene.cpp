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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>

using namespace ax;
namespace {
constexpr float Floor = 294;
constexpr float Contact = 151;
constexpr float PixelsPerMeter = 11;
constexpr float HeightScale = 54;
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
    float radius = std::min(r.size.height / 2, 18.0f);
    rect(d, Rect(r.origin.x + radius, r.origin.y, r.size.width - 2 * radius, r.size.height), color);
    ellipse(d, {r.origin.x + radius, r.getMidY()}, radius, r.size.height / 2, color);
    ellipse(d, {r.getMaxX() - radius, r.getMidY()}, radius, r.size.height / 2, color);
}
std::string whole(double value) { return std::to_string(static_cast<long long>(value)); }
}

bool MainScene::init() {
    if (!Scene::init()) return false;
    world = DrawNode::create(); addChild(world);
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
    if (smokeStep < 0 && !game.GetProfile().Load(savePath, error)) {
        saveAllowed = false;
        saveWarning = "Save indisponivel. Original preservado.";
    }

    auto touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        if (activeTouch != -1) return false;
        activeTouch = t->getID();
        touchStart = t->getLocation();
        touchAction = hit(touchStart);
        // Movement buttons respond on press; menu buttons on release.
        if (touchAction == Action::Jump || touchAction == Action::Slide) act(touchAction);
        return true;
    };
    touch->onTouchEnded = [this](Touch* t, Event*) {
        if (t->getID() != activeTouch) return;
        activeTouch = -1;
        const auto end = t->getLocation();
        if (touchAction == Action::Jump || touchAction == Action::Slide) return;
        if (touchAction != Action::None) {
            if (hit(end) == touchAction) act(touchAction);
        } else if (game.IsPlaying()) {
            const auto delta = end - touchStart;
            if (delta.y < -35) act(Action::Slide);
            else if (delta.y > 35 || delta.length() < 18) act(Action::Jump);
        }
    };
    touch->onTouchCancelled = [this](Touch*, Event*) { activeTouch = -1; touchAction = Action::None; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
    auto keyboard = EventListenerKeyboard::create();
    keyboard->onKeyPressed = [this](EventKeyboard::KeyCode key, Event*) {
        using Key = EventKeyboard::KeyCode;
        if (key == Key::KEY_SPACE || key == Key::KEY_UP_ARROW || key == Key::KEY_W) act(Action::Jump);
        if (key == Key::KEY_DOWN_ARROW || key == Key::KEY_S) act(Action::Slide);
        if (key == Key::KEY_P || key == Key::KEY_ESCAPE) {
            if (shop) act(Action::Back); else act(Action::Pause);
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
void MainScene::start(bool procedural) {
    shop = false;
    auto seed = static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    game.StartGame(seed, procedural);
    game.TakeMessages();
    feedback = "Pule as raizes. Deslize sob os galhos.";
    feedbackTime = 4;
    rebuildUI();
}
void MainScene::act(Action action) {
    switch (action) {
    case Action::Play: if (!game.IsPlaying() && game.GetState() != GameState::Paused) start(); break;
    case Action::Jump: game.Jump(); break;
    case Action::Slide: game.Slide(); break;
    case Action::Pause:
        if (game.IsPlaying() || game.GetState() == GameState::Paused) {
            game.TogglePause(); rebuildUI();
        }
        break;
    case Action::Menu: game.ReturnToMenu(); shop = false; save(); rebuildUI(); break;
    case Action::Shop:
        if (!game.IsPlaying() && game.GetState() != GameState::Paused) { shop = true; rebuildUI(); }
        break;
    case Action::Back: shop = false; rebuildUI(); break;
    case Action::Item0: case Action::Item1: case Action::Item2: case Action::Item3: {
        if (!shop) break;
        const int id = static_cast<int>(action) - static_cast<int>(Action::Item0);
        if (game.GetProfile().owned[id] || game.BuyAccessory(id)) game.EquipAccessory(id);
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
    if (game.IsPlaying()) { game.TogglePause(); rebuildUI(); }
    activeTouch = -1;
    save();
}

void MainScene::rebuildUI() {
    ui->removeAllChildren(); panels->clear(); buttons.clear();
    score = distance = coins = status = nullptr;
    const auto state = game.GetState();
    // SHOW_ALL keeps the layout inside the device viewport. Inset further for cutouts.
    const auto safe = Director::getInstance()->getSafeAreaRect();
    const float top = std::min(866.0f, safe.getMaxY() - 22.0f);
    const float bottom = std::max(28.0f, safe.getMinY() + 18.0f);
    if (shop) {
        rect(panels, {20, 130, 440, 626}, Ink);
        text("SEU ESTILO NO MANGUE", 16, {240, 715}, Muted);
        text("Escolha seu Jaca", 32, {240, 668}, Cream, true);
        text(whole(game.GetProfile().coins) + " moedas disponiveis", 20, {240, 624}, Muted);
        const char* titles[] = {"Original", "Bone do mangue", "Oculos tropicais", "Chapeu de pescador"};
        for (int i = 0; i < 4; ++i) {
            std::string caption = titles[i];
            if (game.GetProfile().equipped == i) caption += "  /  equipado";
            else if (game.GetProfile().owned[i]) caption += "  /  usar";
            else caption += "  /  " + whole(AccessoryCatalog()[i].price) + " moedas";
            button(caption, {44, 517.0f - i * 77, 392, 58},
                   static_cast<Action>(static_cast<int>(Action::Item0) + i), game.GetProfile().equipped == i);
        }
        button("VOLTAR", {110, 165, 260, 55}, Action::Back);
        status = text("Cosmeticos comprados com moedas do jogo.", 15, {240, 111}, Muted);
        return;
    }
    if (state == GameState::Menu) {
        text("MANGUE EM MOVIMENTO", 17, {240, top - 25}, Muted);
        text("JACARUN", 74, {240, top - 92}, Cream, true);
        text("Uma corrida. Mil historias no mangue.", 20, {240, top - 154}, Muted);
        pill(panels, {91, 550, 298, 44}, Color4F(0.08f, 0.26f, 0.25f, 1));
        text("RECORDE  " + whole(game.GetProfile().bestScore) + "  /  NIVEL " +
             whole(game.GetProfile().GetLevel()), 16, {240, 573}, Cream);
        button("VAMOS CORRER", {55, 170, 370, 68}, Action::Play, true);
        button("SEU JACA  /  " + whole(game.GetProfile().coins) + " moedas",
               {55, 96, 370, 55}, Action::Shop);
        text("PROTOTIPO 0.2  /  ARTE PROVISORIA", 13, {240, bottom}, Muted);
        if (!saveWarning.empty()) text(saveWarning, 14, {240, 65}, Color4B(255, 194, 116, 255));
        return;
    }
    pill(panels, {20, top - 103, 337, 106}, Color4F(0.035f, 0.17f, 0.18f, 0.94f));
    text("PONTOS", 12, {75, top - 15}, Muted);
    score = text(whole(game.GetScore()), 34, {77, top - 48}, Cream, true);
    text("DISTANCIA", 12, {191, top - 15}, Muted);
    distance = text(whole(game.GetDistance()) + " m", 24, {193, top - 51}, Cream, true);
    text("MOEDAS", 12, {302, top - 15}, Muted);
    coins = text(whole(game.GetRunCoins()), 26, {302, top - 51}, Color4B(245, 207, 93, 255), true);
    if (state == GameState::Playing) {
        button("II", {376, top - 66, 78, 65}, Action::Pause);
        status = text("", 18, {240, 686}, Cream);
        button("DESLIZAR", {24, bottom + 39, 208, 76}, Action::Slide);
        button("PULAR", {248, bottom + 39, 208, 76}, Action::Jump, true);
        text("Toque para pular  /  arraste para baixo para deslizar", 15, {240, bottom + 15}, Muted);
        text("TECLADO  /  ESPACO + SETA BAIXO  /  P PAUSA", 12, {240, bottom - 8}, Muted);
        text("MANGUE 01", 14, {77, 225}, Muted);
        return;
    }
    rect(panels, {0, 0, 480, 750}, Color4F(0.02f, 0.11f, 0.12f, 0.72f));
    pill(panels, {28, 166, 424, 510}, Ink);
    if (state == GameState::Paused) {
        text("RESPIRE UM POUCO", 16, {240, 620}, Muted);
        text("Corrida pausada", 36, {240, 567}, Cream, true);
        text("Seu Jaca espera por voce.", 21, {240, 512}, Muted);
        button("CONTINUAR", {59, 351, 362, 66}, Action::Pause, true);
        button("ENCERRAR E VOLTAR", {59, 266, 362, 58}, Action::Menu);
        text("As moedas coletadas serao guardadas.", 16, {240, 216}, Muted);
    } else {
        text("ATE A PROXIMA CORRIDA", 16, {240, 623}, Muted);
        text("Boa, Jaca!", 45, {240, 567}, Cream, true);
        text(whole(game.GetScore()) + " pontos", 37, {240, 504}, Cream, true);
        text(whole(game.GetDistance()) + " m  /  +" + whole(game.GetRunCoins()) +
             " moedas  /  " + whole(game.GetFoods()) + " alimentos", 19, {240, 455}, Muted);
        text("RECORDE  " + whole(game.GetProfile().bestScore), 16, {240, 414}, Muted);
        button("CORRER DE NOVO", {59, 313, 362, 66}, Action::Play, true);
        button("VOLTAR AO INICIO", {59, 235, 362, 56}, Action::Menu);
        if (!saveWarning.empty()) text(saveWarning, 14, {240, 196}, Color4B(255, 194, 116, 255));
    }
}

void MainScene::drawCrocodile(Vec2 feet, float scale) {
    const bool sliding = game.GetPlayer().IsSliding() && game.IsPlaying();
    auto point = [&](float x, float y) { return feet + Vec2(x * scale, y * scale * (sliding ? 0.46f : 1.0f)); };
    auto oval = [&](float x, float y, float rx, float ry, Color4F c) {
        ellipse(world, point(x,y), rx*scale, ry*scale*(sliding ? 0.46f : 1.0f), c);
    };
    const Color4F green(0.49f, 0.75f, 0.32f, 1), bright(0.72f, 0.87f, 0.39f, 1);
    Vec2 tail[] = {point(-23, 15), point(-74, 30), point(-45, 2), point(-7, 7)};
    world->drawSolidPoly(tail, 4, green);
    oval(-9, 24, 36, 20, green);
    oval(24, 28, 33, 16, bright);
    oval(29, 17, 27, 8, Color4F(0.86f, 0.92f, 0.56f, 1));
    for (int i = 0; i < 5; ++i) {
        float x = -34.0f + i * 12;
        Vec2 spike[] = {point(x,37), point(x+4,47), point(x+10,38)};
        world->drawSolidPoly(spike, 3, Color4F(0.29f, 0.54f, 0.26f, 1));
    }
    oval(12, 45, 10, 11, bright);
    oval(14, 47, 6, 7, Color4F(0.98f,0.98f,0.83f,1));
    oval(17, 47, 2.6f, 4.5f, Ink);
    oval(49, 32, 2, 2, Ink);
    world->drawSegment(point(23,22), point(55,23), scale, Ink);
    for (int i=0; i<3; ++i) {
        const float x = 31.0f + i*8;
        Vec2 tooth[] = {point(x,22),point(x+5,22),point(x+2,17)};
        world->drawSolidPoly(tooth,3,Color4F(1,0.98f,0.83f,1));
    }
    const float step = game.IsPlaying() && game.GetPlayer().IsGrounded() && !sliding ?
        std::sin(clock * 20) * 5 : 0;
    oval(-25 + step, 4, 13, 5, green);
    oval(13 - step, 4, 13, 5, green);
    const int accessory = game.GetProfile().equipped;
    if (accessory == 1) {
        world->drawSegment(point(3,57), point(29,57), 4*scale, Color4F(1,0.55f,0.29f,1));
        oval(9,60,12,7,Color4F(1,0.55f,0.29f,1));
    } else if (accessory == 2) {
        oval(15,47,9,6,Ink);
        world->drawSegment(point(6,48),point(-6,49),2*scale,Ink);
    } else if (accessory == 3) {
        world->drawSegment(point(-2,57),point(29,57),4*scale,Color4F(0.85f,0.71f,0.40f,1));
        world->drawSegment(point(8,62),point(18,62),7*scale,Color4F(0.94f,0.83f,0.54f,1));
    }
    if (game.GetShieldSeconds() > 0)
        world->drawCircle(point(-3,24), 67*scale, 0, 48, false, Color4F(0.56f,0.92f,0.96f,0.8f));
}

void MainScene::drawWorld() {
    world->clear();
    rect(world, {0,0,480,900}, Color4F(0.08f,0.25f,0.25f,1));
    rect(world, {0,380,480,340}, Color4F(0.22f,0.43f,0.38f,1));
    ellipse(world, {353,650}, 49,49,Color4F(0.91f,0.80f,0.48f,1));
    const float travel = static_cast<float>(game.GetDistance());
    // Three layers move at different speeds; the actual course uses core distance.
    for(int layer=0; layer<3; ++layer) {
        const float spacing=143.0f+layer*40;
        const float shift=std::fmod(travel*(1.0f+layer*1.1f),spacing);
        const Color4F shade = layer==0 ? Color4F(0.16f,0.36f,0.32f,1) :
            layer==1 ? Color4F(0.10f,0.29f,0.27f,1) : Color4F(0.06f,0.22f,0.22f,1);
        for(int i=-1;i<5;++i) {
            float x=i*spacing-shift+35*layer, y=396.0f-layer*23;
            world->drawSegment({x,y},{x+20,633.0f-layer*24},7+layer*3,shade);
            world->drawSegment({x+10,530},{x-35,585},5,shade);
            world->drawSegment({x+15,y+37},{x-35,y-10},5,shade);
            world->drawSegment({x+15,y+37},{x+55,y-10},5,shade);
            ellipse(world,{x+12,637.0f-layer*26},91,43,shade);
        }
    }
    rect(world,{0,Floor,480,73},Color4F(0.16f,0.39f,0.34f,1));
    for(int i=0;i<10;++i) {
        float x=std::fmod(i*69.0f-travel*3+10000,560)-40;
        world->drawSegment({x,318.0f+(i%3)*17},{x+30,318.0f+(i%3)*17},1,
                           Color4F(0.33f,0.56f,0.44f,0.5f));
    }
    rect(world,{0,245,480,49},Color4F(0.36f,0.39f,0.24f,1));
    rect(world,{0,Floor-5,480,5},Color4F(0.65f,0.68f,0.37f,1));
    rect(world,{0,0,480,245},Ink);
    const float groundShift = std::fmod(travel*PixelsPerMeter,90.0f);
    for(int i=0;i<7;++i) {
        float x=i*90.0f-groundShift;
        world->drawSegment({x,263},{x+25,270},2,Color4F(0.23f,0.29f,0.20f,1));
        world->drawSegment({x+44,283},{x+51,289},2,Color4F(0.74f,0.72f,0.42f,1));
    }
    if (game.GetState() != GameState::Menu) {
        for(const auto& e:game.GetLevel().GetEntities()) {
            const float x=Contact+static_cast<float>(e.distance-game.GetDistance())*PixelsPerMeter;
            if(x < -70 || x > 550) continue;
            const float y=Floor+static_cast<float>(e.height)*HeightScale;
            switch(e.type) {
            case EntityType::Ground: {
                Vec2 root[]={{x,Floor},{x+7,Floor+38},{x+17,Floor+44},{x+25,Floor+16},{x+47,Floor}};
                world->drawSolidPoly(root,5,Color4F(0.56f,0.34f,0.21f,1));
                world->drawSegment({x+12,Floor+5},{x+13,Floor+31},2,Color4F(0.81f,0.56f,0.31f,1));
                break;
            }
            case EntityType::High:
                world->drawSegment({x+19,Floor+194},{x+10,Floor+50},11,Color4F(0.43f,0.29f,0.20f,1));
                world->drawSegment({x+5,Floor+47},{x+55,Floor+47},12,Color4F(0.58f,0.39f,0.24f,1));
                ellipse(world,{x+29,Floor+184},48,22,Color4F(0.24f,0.48f,0.28f,1));
                break;
            case EntityType::Coin:
                ellipse(world,{x,y},10,13,Color4F(0.96f,0.70f,0.23f,1));
                ellipse(world,{x,y},6,9,Color4F(1,0.88f,0.43f,1));
                world->drawSegment({x,y-4},{x,y+4},1,Color4F(0.69f,0.43f,0.12f,1));
                break;
            case EntityType::Crab:
                ellipse(world,{x,y},12,8,Color4F(1,0.48f,0.33f,1));
                for(int side:{-1,1}) {
                    world->drawSegment({x+side*7.0f,y},{x+side*17.0f,y-7},2,Color4F(1,0.48f,0.33f,1));
                    ellipse(world,{x+side*16.0f,y+9},5,6,Color4F(1,0.48f,0.33f,1));
                    ellipse(world,{x+side*4.0f,y+5},2,2,Ink);
                } break;
            case EntityType::Fish: case EntityType::RareFish: {
                Color4F c=e.type==EntityType::RareFish?Color4F(1,0.73f,0.31f,1):Color4F(0.51f,0.84f,0.84f,1);
                ellipse(world,{x,y},15,9,c);
                Vec2 tail[]={{x+10,y},{x+24,y+9},{x+24,y-9}};
                world->drawSolidPoly(tail,3,c);
                ellipse(world,{x-8,y+2},2,2,Ink); break;
            }
            case EntityType::Shield: {
                Vec2 shield[]={{x-15,y+16},{x+15,y+16},{x+12,y-6},{x,y-20},{x-12,y-6}};
                world->drawSolidPoly(shield,5,Color4F(0.46f,0.83f,0.92f,1));
                world->drawSegment({x,y-8},{x,y+8},2,Ink); break;
            }
            case EntityType::Magnet:
                world->drawSegment({x-9,y+11},{x-9,y-7},4,Color4F(0.98f,0.50f,0.42f,1));
                world->drawSegment({x+9,y+11},{x+9,y-7},4,Color4F(0.98f,0.50f,0.42f,1));
                world->drawSegment({x-9,y-7},{x+9,y-7},4,Color4F(0.98f,0.50f,0.42f,1));
                break;
            }
        }
    }
    const bool menu = game.GetState()==GameState::Menu;
    const float playerX = menu ? 248 : Contact-55;
    const float playerY = Floor + (menu ? 0 : static_cast<float>(game.GetPlayer().GetPositionY())*HeightScale);
    ellipse(world,{playerX-4,Floor+1},menu?91:51,6,Color4F(0.03f,0.13f,0.12f,0.45f));
    drawCrocodile({playerX,playerY},menu?1.65f:1);
}

void MainScene::update(float dt) {
    // Clamp a slow frame instead of teleporting through obstacles after a stall.
    dt=std::clamp(dt,0.0f,0.1f);
    clock += dt;
    const auto before=game.GetState();
    game.Update(dt);
    if (before!=game.GetState()) { save(); rebuildUI(); }
    for(auto& message:game.TakeMessages()) {
        feedback=std::move(message); feedbackTime=2.5f;
    }
    feedbackTime=std::max(0.0f,feedbackTime-dt);
    if (score) score->setString(whole(game.GetScore()));
    if (distance) distance->setString(whole(game.GetDistance())+" m");
    if (coins) coins->setString(whole(game.GetRunCoins()));
    if (status) {
        std::string info;
        if (!shop && game.GetShieldSeconds()>0) info+="ESCUDO "+whole(std::ceil(game.GetShieldSeconds()))+"s  ";
        if (!shop && game.GetMagnetSeconds()>0) info+="IMA "+whole(std::ceil(game.GetMagnetSeconds()))+"s  ";
        if (!shop && game.GetCombo()>0) info+="COMBO "+whole(game.GetCombo())+" / x"+whole(game.GetMultiplier());
        if(info.empty() && feedbackTime>0) info=feedback;
        if(info.empty() && shop) info="Cosmeticos comprados com moedas do jogo.";
        status->setString(info);
        if(status->getContentSize().width>440) status->setScale(440/status->getContentSize().width);
        else status->setScale(1);
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
    if(smokeTime>22) { finishSmoke(false,"Timed out"); return; }
    const double d=game.GetDistance();
    if(smokeStep==0 && smokeTime>0.4f) { capture("01-menu.png"); smokeStep=1; }
    else if(smokeStep==1 && smokeTime>0.9f) {
        start(false);
        game.Spawn(EntityType::Coin,2,0.5);
        game.Spawn(EntityType::Crab,4,0.5);
        game.Spawn(EntityType::Ground,12);
        game.Spawn(EntityType::High,26);
        game.Spawn(EntityType::Ground,50);
        smokeStep=2;
    } else if(smokeStep==2) {
        if(!smokeJump && d>9.2) { smokeTouch({350,105},{350,105}); smokeJump=true; }
        if(smokeJump && game.GetPlayer().GetPositionY()>1.5 && smokeStep==2) {
            capture("02-jump.png"); smokeStep=3;
        }
    } else if(smokeStep==3) {
        if(!smokeSlide && d>23.2) { smokeTouch({350,500},{350,430}); smokeSlide=true; }
        if(d>26.5) {
            if(!game.IsPlaying() || game.GetObstaclesPassed()!=2 || game.GetRunCoins()!=1 || game.GetFoods()!=1) {
                finishSmoke(false,"Obstacle or collection integration failed"); return;
            }
            suspend(); pausedDistance=game.GetDistance(); smokeStep=4; capture("03-pause.png");
        }
    } else if(smokeStep==4 && smokeTime>6) {
        if(game.GetState()!=GameState::Paused || game.GetDistance()!=pausedDistance) {
            finishSmoke(false,"Background pause moved the simulation"); return;
        }
        smokeTouch({240,384},{240,384}); smokeStep=5;
    } else if(smokeStep==5 && game.GetState()==GameState::GameOver) {
        capture("04-game-over.png");
        const auto bank=game.GetProfile().coins;
        save();
        Profile restored; std::string error;
        if(bank!=1 || !restored.Load(savePath,error) || restored.coins!=bank) {
            finishSmoke(false,"Profile persistence failed"); return;
        }
        smokeStep=6;
    } else if(smokeStep==6) {
        smokeTouch({240,345},{240,345});
        if(!game.IsPlaying() || game.GetDistance()!=0 || game.GetProfile().coins!=1) {
            finishSmoke(false,"Restart failed"); return;
        }
        finishSmoke(true,"Menu; touch jump/root; swipe slide/branch; coin/food; background pause; touch resume; collision; save/reload; touch restart.");
    }
    if(smokeStep>=2 && smokeStep<=3 && game.GetState()==GameState::GameOver)
        finishSmoke(false,"Unexpected early collision");
#endif
}
