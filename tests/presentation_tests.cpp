#include "../visual/Source/StoryIntro.h"
#include "../visual/Source/RunPresentation.h"
#include "../visual/Source/CharacterMotion.h"
#include "../visual/Source/GestureInput.h"
#include <iostream>
#include "../visual/Source/MenuEntrance.h"
#include "../visual/Source/FishMotion.h"
#include "../visual/Source/EnvironmentCycle.h"
#include "../visual/Source/CharacterAnimation.h"
#include <stdexcept>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while(false)

void PersistenceAndCollection() {
    GameManager game;
    RunPresentation view;
    game.StartGame(1, false);
    game.Spawn(EntityType::Ground, 3);
    game.Spawn(EntityType::Coin, 3, 0.5); // Missed while airborne; same coordinate, distinct ID.
    game.Spawn(EntityType::Fish, 3, 1.5);
    view.Sync(game.GetLevel().GetEntities());
    CHECK(view.GetObjects().size() == 3);
    CHECK(game.Jump());
    game.Update(0.4);
    CHECK(game.IsPlaying());
    CHECK(game.GetFrameResolutions().size() == 3);
    CHECK(game.GetFrameResolutions()[0].outcome == EntityOutcome::Collected);
    CHECK(game.GetFrameResolutions()[1].outcome == EntityOutcome::Passed);
    CHECK(game.GetFrameResolutions()[2].outcome == EntityOutcome::Missed);
    view.Resolve(game.GetFrameResolutions());
    view.Sync(game.GetLevel().GetEntities());
    view.Update(0.01f, game.GetDistance(), 166, 12);
    CHECK(view.GetObjects().size() == 3); // Nothing pops away on contact.
    view.Update(0.4f, game.GetDistance(), 166, 12);
    CHECK(view.GetObjects().size() == 2); // Only the collected fish finishes its animation.
    game.Update(0.1);
    CHECK(game.GetFrameResolutions().empty());
    CHECK(game.GetObstaclesPassed() == 1 && game.GetFoods() == 1 && game.GetRunCoins() == 0);
    view.Update(0, 12, 166, 12);
    CHECK(view.GetObjects().size() == 2); // Both still on screen behind the crocodile.
    view.Update(0, 30, 166, 12);
    CHECK(view.GetObjects().empty()); // Removed only beyond the left edge.
    view.Reset();
}
void HitIsVisibleAndEventsBounded() {
    GameManager game;
    RunPresentation view;
    game.StartGame(1, false);
    game.Spawn(EntityType::High, 1);
    game.Spawn(EntityType::Crab, 1, .5);
    view.Sync(game.GetLevel().GetEntities());
    game.Update(.2);
    CHECK(game.GetState() == GameState::GameOver);
    view.Resolve(game.GetFrameResolutions());
    view.Update(0, game.GetDistance(), 166, 12);
    CHECK(view.GetObjects().size() == 2); // Preserve even objects after the lethal event.
    CHECK(view.GetObjects()[0].outcome == EntityOutcome::Hit);
    game.Update(.2);
    CHECK(game.GetFrameResolutions().empty());
    game.StartGame(3, false);
    view.Reset();
    CHECK(view.GetObjects().empty());
}
void SwipesAndTwoFingerPause() {
    GestureInput gestures;
    CHECK(gestures.Begin(1, {50, 50}, 0));
    CHECK(gestures.Move(1, {53, 84}) == Gesture::Up); // Fires before finger release.
    CHECK(gestures.Move(1, {53, 125}) == Gesture::None);
    CHECK(gestures.End(1, {53, 125}, .2f) == Gesture::None);
    gestures.Begin(1, {50, 50}, 1);
    CHECK(gestures.End(1, {50, 10}, 1.2f) == Gesture::Down);
    gestures.Begin(1, {50, 50}, 2);
    CHECK(gestures.End(1, {120, 51}, 2.1f) == Gesture::None); // Horizontal is not jump.
    gestures.Begin(1, {50, 50}, 3);
    CHECK(gestures.End(1, {51, 51}, 3.1f) == Gesture::None); // Tap is not jump.
    gestures.Begin(1, {50, 50}, 4);
    gestures.Begin(2, {80, 50}, 4.05f);
    CHECK(gestures.End(1, {50, 50}, 4.12f) == Gesture::None);
    CHECK(gestures.End(2, {80, 50}, 4.15f) == Gesture::Pause);
    gestures.Begin(1, {50, 50}, 5);
    gestures.Begin(2, {80, 50}, 5.05f);
    CHECK(gestures.Move(1, {50, 90}) == Gesture::None);
    gestures.End(1, {50, 90}, 5.12f);
    CHECK(gestures.End(2, {80, 50}, 5.15f) == Gesture::None);
    gestures.Begin(1, {50, 50}, 6);
    gestures.Cancel();
    CHECK(gestures.End(1, {50, 90}, 6.2f) == Gesture::None);
}
void AnimationPausesAndRecovers() {
    CharacterMotion motion;
    motion.Update(.1f,true,false,false,24);
    CHECK(motion.lift>0 && motion.Height()>1);
    const auto frozen=motion;
    motion.Update(10,false,true,true,60);
    CHECK(motion.time==frozen.time && motion.phase==frozen.phase && motion.Height()==frozen.Height());
    motion.Update(.05f,true,true,false,24);
    CHECK(motion.landing>0 && motion.Width()>1);
    for(int i=0;i<30;++i) motion.Update(.05f,true,true,true,24);
    CHECK(motion.slide>.99f && motion.Height()<.47f);
    for(int i=0;i<30;++i) motion.Update(.05f,true,true,false,24);
    CHECK(motion.slide<.001f && std::abs(motion.Height()-1)<.001f);
    motion.Eat(); motion.Update(.1f,true,true,false,24);
    CHECK(motion.Mouth()>.5f);
    const float mouth=motion.Mouth();
    motion.Update(.1f,false,true,false,24); CHECK(motion.Mouth()==mouth);
    for(int i=0;i<5;++i) motion.Update(.1f,true,true,false,24);
    CHECK(motion.Mouth()==0);
    motion.Eat(); motion.Reset(); CHECK(motion.Mouth()==0);
    motion.Reset(); CHECK(motion.time==0 && motion.phase==0 && motion.Height()==1);
}

void FishLeavesAndReturnsToWater() {
    for(float catchHeight : {130.0f,180.0f}) {
        CHECK(!FishLeap(30,22,catchHeight).airborne);
        CHECK(FishLeap(24,22,catchHeight).height==22);
        CHECK(FishLeap(0,22,catchHeight).height==catchHeight);
        CHECK(FishLeap(12,22,catchHeight).angle<0);
        CHECK(FishLeap(-12,22,catchHeight).angle>0);
        CHECK(FishLeap(-24,22,catchHeight).height==22);
        CHECK(!FishLeap(-30,22,catchHeight).airborne);
        CHECK(FishLeap(12,22,catchHeight).height==FishLeap(-12,22,catchHeight).height);
    }
}
void MenuEntranceCompletesAndSkips() {
    MenuEntrance intro;
    CHECK(intro.Progress()==0); CHECK(intro.Zoom()==3);
    float previousZoom=intro.Zoom();
    float previous=0;
    for(int i=0;i<40;++i) {
        intro.Update(.1f); CHECK(intro.Progress()>=previous);
        previous=intro.Progress();
        CHECK(intro.Zoom()<=previousZoom); previousZoom=intro.Zoom();
    }
    CHECK(intro.Progress()==1); CHECK(intro.Zoom()==1);
    intro.Update(.1f); CHECK(intro.Progress()==1);
    MenuEntrance skipped; skipped.Finish(); CHECK(skipped.Progress()==1); CHECK(skipped.Zoom()==1);
}
void EnvironmentsFollowDistance() {
    for(bool foreground : {false,true}) {
        CHECK(EnvironmentCoverage(0,480,foreground)==0);
        CHECK(EnvironmentCoverage(1,0,foreground)==1);
        CHECK(EnvironmentCoverage(.5f,400,foreground)>EnvironmentCoverage(.5f,80,foreground));
        for(int step=1;step<=100;++step)
            CHECK(EnvironmentCoverage(step/100.0f,240,foreground)>=EnvironmentCoverage((step-1)/100.0f,240,foreground));
    }
    CHECK(EnvironmentAtDistance(0).current==0);
    CHECK(EnvironmentAtDistance(EnvironmentSegmentMeters-.01).current==0);
    for(int segment=1;segment<=12;++segment) {
        const double boundary=segment*EnvironmentSegmentMeters;
        const auto begin=EnvironmentAtDistance(boundary);
        CHECK(begin.current==segment%3 && begin.previous==(segment+2)%3 && begin.progress==0);
        CHECK(EnvironmentAtDistance(boundary+EnvironmentTransitionMeters*.5).progress==.5f);
        CHECK(EnvironmentAtDistance(boundary+EnvironmentTransitionMeters).progress==1);
        CHECK(EnvironmentAtDistance(boundary+EnvironmentSegmentMeters-1).current==segment%3);
    }
    GameManager game;
    game.StartGame(1,false);
    while(game.GetDistance()<EnvironmentSegmentMeters+EnvironmentTransitionMeters*.5) game.Update(.1);
    const auto before=EnvironmentAtDistance(game.GetDistance());
    game.TogglePause(); game.Update(10);
    const auto paused=EnvironmentAtDistance(game.GetDistance());
    CHECK(paused.current==before.current && paused.progress==before.progress);
    game.TogglePause(); game.Update(.1);
    CHECK(EnvironmentAtDistance(game.GetDistance()).progress>before.progress);
    game.ReturnToMenu();
    game.StartGame(1,false);
    CHECK(game.GetDistance()==0);
    CHECK(EnvironmentAtDistance(game.GetDistance()).current==0);
}
void ExpressiveCharacterStates() {
    CharacterMotion motion;
    unsigned frames=0;
    for(int i=0;i<100;++i) {
        motion.Update(.01f,true,true,false,24);
        const auto p=AnimateJaca(motion,false,true,false,false,false,0);
        CHECK(p.atlas==JacaAtlas::Run && p.frame>=0 && p.frame<8);
        CHECK(p.scaleX>.8f && p.scaleY>.8f);
        frames|=1u<<p.frame;
    }
    CHECK(frames==255);
    const auto frozen=motion;
    motion.Update(5,false,false,true,60,-20,true);
    CHECK(motion.phase==frozen.phase && motion.time==frozen.time && motion.lean==frozen.lean && motion.defeat==frozen.defeat);
    motion.Update(.1f,true,false,false,24,12);
    CHECK(motion.phase==frozen.phase);
    CHECK(AnimateJaca(motion,false,false,false,false,false,12).frame==0);
    CHECK(AnimateJaca(motion,false,false,false,false,false,0).frame==1);
    CHECK(AnimateJaca(motion,false,false,false,false,false,-12).frame==2);
    motion.Eat();
    CHECK(AnimateJaca(motion,false,false,false,false,false,0).frame==4);
    auto slide=AnimateJaca(motion,false,true,true,false,false,0);
    CHECK(slide.frame==3 && slide.height==43);
    CHECK(AnimateJaca(motion,false,false,true,true,false,-12).frame==3);
    motion.bite=.1f;
    CHECK(AnimateJaca(motion,false,true,false,false,false,0).frame==5);
    motion.Reset();
    motion.time=2.35f; CHECK(AnimateJaca(motion,true,true,false,false,false,0).frame==2);
    motion.time=4.3f; CHECK(AnimateJaca(motion,true,true,false,false,false,0).frame==4);
    motion.time=5.8f; CHECK(AnimateJaca(motion,true,true,false,false,false,0).frame==5);
    CHECK(AnimateJaca(motion,true,true,false,false,false,0,.2f).frame==6);
    CHECK(AnimateJaca(motion,true,true,false,false,false,0,1.2f).frame==7);
    CHECK(AnimateJaca(motion,true,true,false,false,false,0,3.0f).frame==4);
    motion.Reset(); motion.Hit(); motion.Update(.1f,true,true,false,24,0,true);
    CHECK(AnimateJaca(motion,false,true,false,false,true,0).frame==6);
    for(int i=0;i<4;++i) motion.Update(.1f,true,true,false,24,0,true);
    CHECK(AnimateJaca(motion,false,true,false,false,true,0).frame==7);
    const float defeat=motion.defeat;
    motion.Update(1,false,true,false,24,0,true); CHECK(motion.defeat==defeat);
    motion.Reset(); CHECK(motion.defeat==0 && motion.hurt==0 && motion.lean==0);
    MenuEntrance intro; intro.time=1.2f; CHECK(intro.Hop()>30);
    intro.Finish(); CHECK(intro.Hop()==0);
}
int main() {
    try {
        PersistenceAndCollection();
        HitIsVisibleAndEventsBounded();
        SwipesAndTwoFingerPause();
        AnimationPausesAndRecovers();
        FishLeavesAndReturnsToWater();
        MenuEntranceCompletesAndSkips();
        EnvironmentsFollowDistance();
        ExpressiveCharacterStates();
        StoryIntro story;
        CHECK(story.active && story.Beat()==0);
        story.Update(-1); CHECK(story.time==0);
        story.time=StoryIntro::Start(1); CHECK(story.Beat()==1);
        story.time=StoryIntro::Start(2); CHECK(story.Beat()==2);
        for(int i=0;i<200;++i) story.Update(.1f);
        CHECK(!story.active && story.time==StoryIntro::Duration);
        for(int beat=0;beat<StoryIntro::BeatCount;++beat) {
            StoryIntro shot; shot.time=StoryIntro::Start(beat);
            CHECK(shot.Beat()==beat && shot.Phase()==0);
            shot.Update(.1f); CHECK(shot.Phase()>0 && shot.Beat()==beat);
            shot.Skip(); CHECK(!shot.active && shot.Beat()==StoryIntro::BeatCount-1 && shot.Phase()==1);
        }
        StoryIntro skipped; skipped.Skip(); skipped.Update(.1f); CHECK(!skipped.active);

        int previous=-1; bool seen[4]={};
        for(int tick=0;tick<=190;++tick) {
            const int frame=StoryIntro::BiteFrame(tick/190.0f);
            CHECK(frame>=previous && frame>=0 && frame<4);
            seen[frame]=true; previous=frame;
        }
        for(bool pose:seen) CHECK(pose);
        CHECK(StoryIntro::BiteFrame(1)==3);
        std::cout << "10/10 presentation, gesture, animation, environment and story groups passed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
