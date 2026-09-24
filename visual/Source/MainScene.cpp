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
        auto p = t->getLocation();
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
        if (!shop && p.y - touchStart.y > 35 && p.y - touchStart.y > std::abs(p.x-touchStart.x)*1.15f) {
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
void MainScene::handleGesture(Gesture gesture) {
    if (gesture == Gesture::Up) act(Action::Jump);
    else if (gesture == Gesture::Down) act(Action::Slide);
    else if (gesture == Gesture::Pause) act(Action::Pause);
}
void MainScene::start(bool procedural) {
    shop = false;
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
    gestures.Cancel();
    save();
}

void MainScene::rebuildUI() {
    ui->removeAllChildren(); panels->clear(); buttons.clear();
    score = distance = coins = status = pace = gestureHint = nullptr;
    gestures.Cancel(); activeTouch = -1; touchAction = Action::None;
    const auto state = game.GetState();
    const auto safe = Director::getInstance()->getSafeAreaRect();
    const float top = std::min(viewHeight - 28.0f, safe.getMaxY() - 24.0f);
    const float bottom = std::max(26.0f, safe.getMinY() + 18.0f);
    const float center = viewHeight * .52f;
    if (shop) {
        pill(panels, {20, center - 290, 440, 606}, Color4F(.035f,.17f,.18f,.95f));
        text("SEU ESTILO NO MANGUE", 16, {240, center + 270}, Muted);
        text("Escolha seu Jaca", 32, {240, center + 224}, Cream, true);
        text(whole(game.GetProfile().coins) + " moedas disponiveis", 20, {240, center + 180}, Muted);
        const char* titles[] = {"Original", "Bone do mangue", "Oculos tropicais", "Chapeu de pescador"};
        for (int i = 0; i < 4; ++i) {
            std::string caption = titles[i];
            if (game.GetProfile().equipped == i) caption += "  /  equipado";
            else if (game.GetProfile().owned[i]) caption += "  /  usar";
            else caption += "  /  " + whole(AccessoryCatalog()[i].price) + " moedas";
            button(caption, {44, center + 77 - i * 77, 392, 58},
                   static_cast<Action>(static_cast<int>(Action::Item0) + i), game.GetProfile().equipped == i);
        }
        button("VOLTAR", {110, center - 250, 260, 55}, Action::Back);
        status = text("Cosmeticos comprados com moedas do jogo.", 15, {240, center - 320}, Cream);
        return;
    }
    if (state == GameState::Menu) {
        text("MANGUE EM MOVIMENTO", 17, {240, top - 40}, Muted);
        text("JACARUN", 78, {240, top - 108}, Cream, true);
        text("Seu caminho passa pelo mangue.", 21, {240, top - 173}, Cream);
        pill(panels, {85, viewHeight * .60f, 310, 43}, Color4F(.04f,.22f,.22f,.82f));
        text("RECORDE  " + whole(game.GetProfile().bestScore) + "  /  NIVEL " +
             whole(game.GetProfile().GetLevel()), 16, {240, viewHeight * .60f + 22}, Cream);
        button("VAMOS CORRER", {55, bottom +  90, 370, 65}, Action::Play, true);
        button("SEU JACA  /  " + whole(game.GetProfile().coins) + " moedas",
               {55, bottom + 22, 370, 52}, Action::Shop);
        text("DESLIZE PARA CIMA PARA COMECAR", 13, {240, bottom + 177}, Cream);
        if (!saveWarning.empty()) text(saveWarning, 14, {240, bottom + 2}, Color4B(255,194,116,255));
        return;
    }
    // A compact HUD floats over the forest; there are no gameplay buttons or reserved panels.
    pill(panels, {22, top - 62, 436, 65}, Color4F(.035f,.16f,.17f,1));
    text("PONTOS", 11, {72, top - 10}, Muted);
    score = text(whole(game.GetScore()), 28, {73, top - 37}, Cream, true);
    distance = text(whole(game.GetDistance()) + " m", 24, {243, top - 30}, Cream, true);
    ellipse(panels, {376, top - 29}, 9, 11, Color4F(1,.79f,.31f,1));
    coins = text(whole(game.GetRunCoins()), 24, {413, top - 29}, Color4B(255,219,122,255), true);
    if (state == GameState::Playing) {
        status = text("", 17, {240, top - 93}, Cream);
        pace = text("", 14, {240, top - 121}, Muted);
        pace->enableOutline(Color4B(13,47,42,210), 1);
        gestureHint = text("ARRASTE PARA CIMA: PULO\nPARA BAIXO: DESLIZE  /  2 DEDOS: PAUSA",
                           17, {240, bottom + 49}, Cream);
        gestureHint->setAlignment(TextHAlignment::CENTER);
        gestureHint->enableOutline(Color4B(13,47,42,210), 1);
        return;
    }
    rect(panels, {0,0,480,viewHeight}, Color4F(.015f,.09f,.11f,.49f));
    pill(panels, {28, center - 257, 424, 493}, Color4F(.035f,.17f,.18f,.96f));
    if (state == GameState::Paused) {
        text("RESPIRE UM POUCO", 16, {240, center + 187}, Muted);
        text("Corrida pausada", 36, {240, center + 135}, Cream, true);
        text("Arraste para cima para continuar.", 20, {240, center + 82}, Muted);
        button("CONTINUAR", {59, center - 66, 362, 66}, Action::Pause, true);
        button("ENCERRAR E VOLTAR", {59, center - 149, 362, 58}, Action::Menu);
        text("As moedas coletadas serao guardadas.", 16, {240, center - 198}, Muted);
    } else {
        text("ATE A PROXIMA CORRIDA", 16, {240, center + 188}, Muted);
        text("Boa, Jaca!", 45, {240, center + 133}, Cream, true);
        text(whole(game.GetScore()) + " pontos", 37, {240, center + 68}, Cream, true);
        text(whole(game.GetDistance()) + " m  /  +" + whole(game.GetRunCoins()) +
             " moedas  /  " + whole(game.GetFoods()) + " alimentos", 19, {240, center + 18}, Muted);
        text("RECORDE  " + whole(game.GetProfile().bestScore), 16, {240, center - 20}, Muted);
        button("CORRER DE NOVO", {59, center - 119, 362, 66}, Action::Play, true);
        button("VOLTAR AO INICIO", {59, center - 195, 362, 56}, Action::Menu);
        if (!saveWarning.empty()) text(saveWarning, 14, {240, center - 229}, Color4B(255,194,116,255));
    }
}

void MainScene::drawCrocodile(Vec2 feet, float scale) {
    const bool sliding = game.GetPlayer().IsSliding() && game.GetState() != GameState::Menu;
    auto point = [&](float x, float y) { return feet + Vec2(x * scale, y * scale * (sliding ? 0.46f : 1.0f)); };
    auto oval = [&](float x, float y, float rx, float ry, Color4F c) {
        ellipse(world, point(x,y), rx*scale, ry*scale*(sliding ? 0.46f : 1.0f), c);
    };
    const Color4F green(0.39f, 0.68f, 0.30f, 1), bright(0.72f, 0.88f, 0.39f, 1);
    Vec2 tail[] = {point(-23, 15), point(-74, 30), point(-45, 2), point(-7, 7)};
    world->drawSolidPoly(tail, 4, green);
    oval(-9, 22, 38, 21, Color4F(.19f,.40f,.23f,1));
    oval(-9, 24, 36, 20, green);
    oval(-6, 15, 28, 10, Color4F(.63f,.79f,.35f,1));
    oval(24, 28, 33, 16, bright);
    oval(29, 17, 27, 8, Color4F(0.86f, 0.92f, 0.56f, 1));
    for (int i = 0; i < 5; ++i) {
        float x = -34.0f + i * 12;
        Vec2 spike[] = {point(x,37), point(x+4,47), point(x+10,38)};
        world->drawSolidPoly(spike, 3, Color4F(0.29f, 0.54f, 0.26f, 1));
    }
    oval(12, 45, 10, 11, bright);
    oval(14, 47, 6, 7, Color4F(0.98f,0.98f,0.83f,1));
    for (int i=0; i<5; ++i) oval(-31+i*9, 26+(i%2)*6, 2, 2, Color4F(.28f,.54f,.26f,1));
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

void MainScene::drawEntity(const VisualEntity& object) {
    const auto& e = object.entity;
    float x = Contact + static_cast<float>(e.distance - game.GetDistance()) * PixelsPerMeter;
    // Place the dangerous part around the contact coordinate, not a full sprite ahead.
    if (IsObstacle(e.type)) x -= 40;
    if (x < -120 || x > 570) return;
    float y = floorY + static_cast<float>(e.height) * HeightScale;
    float scale = 1, opacity = 1;
    if (object.outcome == EntityOutcome::Collected) {
        const float t = std::clamp(object.effectTime / RunPresentation::CollectionSeconds, 0.0f, 1.0f);
        x += (Contact - 16 - x) * t;
        y += 35 * std::sin(t * 3.14159f) + 19 * t;
        scale = 1 - .72f*t;
        opacity = 1 - t*t;
    }
    auto tint = [opacity](Color4F c) { c.a *= opacity; return c; };
    auto pos = [&](float dx, float dy) { return Vec2(x+dx*scale, y+dy*scale); };
    auto oval = [&](float dx,float dy,float rx,float ry,Color4F c) {
        ellipse(world,pos(dx,dy),rx*scale,ry*scale,tint(c));
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
            ellipse(world,{vx+8,floorY+130+strand*12.0f},10,17,Color4F(.64f,.75f,.28f,1));
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
        ellipse(world,{x+32,floorY+202},49,28,Color4F(.22f,.43f,.24f,1));
        ellipse(world,{x+14,floorY+221},34,19,Color4F(.31f,.53f,.27f,1));
        ellipse(world,{x+52,floorY+211},29,18,Color4F(.38f,.57f,.29f,1));
        break;
    case EntityType::Coin: {
        const float face=.77f+.23f*std::cos(clock*4+static_cast<float>(e.distance));
        oval(0,0,12*face,15,Color4F(.69f,.40f,.12f,1));
        oval(0,2,10*face,13,Color4F(1,.78f,.26f,1));
        oval(-2,3,6*face,9,Color4F(1,.90f,.50f,1));
        line(0,-3,0,7,1.2f,Color4F(.75f,.49f,.15f,1));
        break;
    }
    case EntityType::Crab:
        for(int side:{-1,1}) {
            line(side*7.0f,-1,side*18.0f,-10,2.2f,Color4F(.77f,.31f,.19f,1));
            line(side*9.0f,1,side*20.0f,-3,2.2f,Color4F(.94f,.43f,.25f,1));
            line(side*9.0f,4,side*19.0f,12,2,Color4F(1,.58f,.31f,1));
            oval(side*19.0f,13,6,7,Color4F(1,.57f,.32f,1));
        }
        oval(0,0,15,10,Color4F(.96f,.40f,.25f,1));
        oval(-2,3,10,5,Color4F(1,.61f,.36f,1));
        for(int side:{-1,1}) {
            oval(side*5.0f,9,3,4,Color4F(1,.96f,.77f,1));
            oval(side*5.0f,10,1.5f,2,Ink);
        } break;
    case EntityType::Fish: case EntityType::RareFish: {
        const Color4F fish = e.type == EntityType::RareFish ? Color4F(1,.74f,.30f,1) : Color4F(.42f,.81f,.83f,1);
        Vec2 tail[]={pos(11,0),pos(26,11),pos(23,0),pos(26,-11)};
        world->drawSolidPoly(tail,4,tint(fish));
        oval(0,0,18,11,fish);
        oval(-2,-4,13,5,Color4F(.88f,.93f,.71f,1));
        oval(-9,3,4,4,Color4F(1,1,.9f,1));
        oval(-10,3,2,2,Ink);
        line(4,-4,6,4,1.4f,Color4F(.20f,.51f,.51f,1));
        break;
    }
    case EntityType::Shield: {
        Vec2 shape[]={pos(-18,19),pos(18,19),pos(14,-6),pos(0,-23),pos(-14,-6)};
        world->drawSolidPoly(shape,5,tint(Color4F(.41f,.79f,.90f,1)));
        line(0,-11,0,10,2,Color4F(.89f,1,1,1));
        line(-8,1,8,1,2,Color4F(.89f,1,1,1));
        break;
    }
    case EntityType::Magnet:
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
    world->clear();
    const float h=viewHeight;
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
    const bool menu=game.GetState()==GameState::Menu;
    if(!menu) for(const auto& entity:presentation.GetObjects()) drawEntity(entity);
    const float scale=menu?1.80f:CrocScale;
    const float playerX=menu?250:Contact-55*scale;
    const float playerY=ground+(menu?0:static_cast<float>(game.GetPlayer().GetPositionY())*HeightScale);
    ellipse(world,{playerX-4,ground+1},menu?100:59,6,Color4F(.04f,.13f,.10f,.36f));
    drawCrocodile({playerX,playerY},scale);
    // Foreground vegetation frames the lane without hiding obstacles.
    for(int i=0;i<7;++i) {
        const float x=i*113.0f-std::fmod(travel*8,113.0f)-30;
        const float y=18+(i%2)*15.0f;
        const float sway=std::sin(clock*.8f+i)*3;
        for(int leaf=-2;leaf<=2;++leaf) {
            const Vec2 start(x,y), tip(x+leaf*11+sway,y+37-std::abs(leaf)*7);
            world->drawSegment(start,tip,3.2f,Color4F(.11f,.30f,.22f,1));
            ellipse(world,tip,5,11,Color4F(.19f,.38f,.25f,1));
        }
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
    const auto before=game.GetState();
    presentation.Sync(game.GetLevel().GetEntities());
    game.Update(dt);
    presentation.Resolve(game.GetFrameResolutions());
    presentation.Sync(game.GetLevel().GetEntities());
    const float activeDt = before == GameState::Playing ? dt : 0;
    presentation.Update(activeDt, game.GetDistance(), Contact, PixelsPerMeter);
    if (before == GameState::Playing) runTime += dt;
    if (before!=game.GetState()) { save(); rebuildUI(); }
    for(auto& message:game.TakeMessages()) {
        feedback=std::move(message); feedbackTime=2.5f;
    }
    feedbackTime=std::max(0.0f,feedbackTime-dt);
    if (score) score->setString(whole(game.GetScore()));
    if (distance) distance->setString(whole(game.GetDistance())+" m");
    if (coins) coins->setString(whole(game.GetRunCoins()));
    if (pace) {
        const double meters = game.GetDistance();
        const auto points = game.GetScore();
        pace->setString(points >= 100000 ? "INSANO / 100.000+ PONTOS" :
                        points >= 80000 ? "LIMITE / SEQUENCIAS DE SETE" :
                        points >= 60000 ? "BRUTAL / SEQUENCIAS DE SEIS" :
                        points >= 40000 ? "EXTREMO / SEQUENCIAS DE CINCO" :
                        points >= 20000 ? "FEROZ / SEQUENCIAS DE QUATRO" :
                        points >= 5000 ? "PRESSAO / SEQUENCIAS LONGAS" :
                        meters < 180 ? "RITMO 1 / AQUECIMENTO" :
                        meters < 550 ? "RITMO 2 / SEQUENCIAS DUPLAS" :
                        meters < 1100 ? "RITMO 3 / DESAFIOS TRIPLOS" :
                        meters < 1800 ? "RITMO 4 / MANGUE ACELERADO" :
                                        "RITMO 5 / CORRIDA SELVAGEM");
    }
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
    if(smokeTime>22) { finishSmoke(false,"Timed out"); return; }
    const double d=game.GetDistance();
    if(smokeStep==0 && smokeTime>0.4f) { capture("01-menu.png"); smokeStep=1; }
    else if(smokeStep==1 && smokeTime>0.9f) {
        start(false);
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
        if(!game.IsPlaying() || game.GetObstaclesPassed()!=1 || game.GetRunCoins()!=4) {
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
        finishSmoke(true,"Gestures, persistence, pause/save/restart, coin arc, action clearance, HUD and three new obstacle drawings.");
    }
    if(smokeStep>=2 && smokeStep<=3 && game.GetState()==GameState::GameOver)
        finishSmoke(false,"Unexpected early collision");
#endif
}
