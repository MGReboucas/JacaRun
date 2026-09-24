#include "../visual/Source/RunPresentation.h"
#include "../visual/Source/GestureInput.h"
#include <iostream>
#include <stdexcept>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while(false)

void PersistenceAndCollection() {
    GameManager game;
    RunPresentation view;
    game.StartGame(1, false);
    game.Spawn(EntityType::Ground, 3);
    game.Spawn(EntityType::Coin, 3, 0.5); // Missed while airborne; same coordinate, distinct ID.
    game.Spawn(EntityType::Fish, 3, 2);
    view.Sync(game.GetLevel().GetEntities());
    CHECK(view.GetObjects().size() == 3);
    CHECK(game.Jump());
    game.Update(0.4);
    CHECK(game.IsPlaying());
    CHECK(game.GetFrameResolutions().size() == 3);
    CHECK(game.GetFrameResolutions()[0].outcome == EntityOutcome::Passed);
    CHECK(game.GetFrameResolutions()[1].outcome == EntityOutcome::Missed);
    CHECK(game.GetFrameResolutions()[2].outcome == EntityOutcome::Collected);
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
int main() {
    try {
        PersistenceAndCollection();
        HitIsVisibleAndEventsBounded();
        SwipesAndTwoFingerPause();
        std::cout << "3/3 presentation and gesture groups passed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
