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

#include "AppDelegate.h"
#include "MainScene.h"
using namespace ax;
AppDelegate::AppDelegate() = default;
AppDelegate::~AppDelegate() = default;
void AppDelegate::initGfxContextAttrs() {
    GfxContextAttrs attrs = {8, 8, 8, 8, 24, 8, 0};
    RenderView::setGfxContextAttrs(attrs);
}
bool AppDelegate::applicationDidFinishLaunching() {
    auto director = Director::getInstance();
    auto view = director->getRenderView();
    if (!view) {
#if (AX_TARGET_PLATFORM != AX_PLATFORM_ANDROID) && (AX_TARGET_PLATFORM != AX_PLATFORM_IOS)
        view = RenderViewImpl::createWithRect("JacaRun | Mangue em movimento", Rect(0, 0, 432, 810));
#else
        view = RenderViewImpl::create("JacaRun");
#endif
        director->setRenderView(view);
    }
    director->setStatsDisplay(false);
    director->setAnimationInterval(1.0 / 60.0);
    director->setClearColor(Color4F(0.04f, 0.14f, 0.15f, 1));
    view->setDesignResolutionSize(480, 900, ResolutionPolicy::SHOW_ALL);
    director->runWithScene(utils::createInstance<MainScene>());
    return true;
}
void AppDelegate::applicationDidEnterBackground() {
    if (auto scene = dynamic_cast<MainScene*>(Director::getInstance()->getRunningScene()))
        scene->suspend();
    Director::getInstance()->stopAnimation();
}
void AppDelegate::applicationWillEnterForeground() {
    // Resume animation, but leave the run paused until the player chooses to continue.
    Director::getInstance()->startAnimation();
}
void AppDelegate::applicationWillQuit() {
    if (auto scene = dynamic_cast<MainScene*>(Director::getInstance()->getRunningScene()))
        scene->save();
}
