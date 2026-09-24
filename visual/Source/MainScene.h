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

#pragma once
#include "axmol.h"
#include "GameManager.h"
#include <filesystem>
#include <string>
#include <vector>

class MainScene : public ax::Scene {
public:
    bool init() override;
    void update(float dt) override;
    void suspend();
    void save();
private:
    enum class Action { None, Play, Pause, Jump, Slide, Menu, Shop, Back, Item0, Item1, Item2, Item3 };
    struct Button { ax::Rect bounds; Action action; };
    GameManager game;
    ax::DrawNode* world = nullptr;
    ax::DrawNode* panels = nullptr;
    ax::Node* ui = nullptr;
    ax::Label* score = nullptr;
    ax::Label* distance = nullptr;
    ax::Label* coins = nullptr;
    ax::Label* status = nullptr;
    std::vector<Button> buttons;
    std::filesystem::path savePath;
    bool saveAllowed = true;
    bool shop = false;
    std::string saveWarning;
    ax::Vec2 touchStart;
    Action touchAction = Action::None;
    int activeTouch = -1;
    float clock = 0;
    float feedbackTime = 0;
    std::string feedback;
    int smokeStep = -1;
    float smokeTime = 0;
    double pausedDistance = 0;
    std::filesystem::path smokeDirectory;
    bool smokeJump = false;
    bool smokeSlide = false;
    void start(bool procedural = true);
    void act(Action action);
    Action hit(ax::Vec2 point) const;
    void rebuildUI();
    void drawWorld();
    void drawCrocodile(ax::Vec2 feet, float scale = 1);
    ax::Label* text(const std::string& value, float size, ax::Vec2 point,
                    ax::Color4B color = ax::Color4B(247, 243, 219, 255), bool bold = false);
    void button(const std::string& title, ax::Rect bounds, Action action, bool primary = false);
    void smokeTick(float dt);
    void capture(const std::string& name);
    void smokeTouch(ax::Vec2 from, ax::Vec2 to);
    void finishSmoke(bool success, const std::string& reason);
};
