#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

enum class Gesture { None, Up, Down, Pause };
struct GesturePoint { float x; float y; };

// Coordinates are design points, independent of pixel density. A swipe fires on MOVE.
class GestureInput {
public:
    bool Begin(int id, GesturePoint point, float time) {
        if (fingers.empty()) { paired = false; eligible = true; beganAt = time; used = false; }
        if (fingers.size() >= 2) { eligible = false; return false; }
        fingers.push_back({id, point, point});
        if (fingers.size() == 2) { paired = true; eligible = eligible && !used; }
        return true;
    }
    Gesture Move(int id, GesturePoint point) {
        auto finger = Find(id);
        if (finger == fingers.end()) return Gesture::None;
        finger->last = point;
        const float dx = point.x - finger->start.x, dy = point.y - finger->start.y;
        if (std::hypot(dx, dy) > 22) eligible = false;
        if (paired || used) return Gesture::None;
        if (std::abs(dy) >= 30 && std::abs(dy) > std::abs(dx) * 1.15f) {
            used = true;
            return dy > 0 ? Gesture::Up : Gesture::Down;
        }
        return Gesture::None;
    }
    Gesture End(int id, GesturePoint point, float time) {
        auto result = Move(id, point);
        auto finger = Find(id);
        if (finger == fingers.end()) return Gesture::None;
        fingers.erase(finger);
        if (fingers.empty() && paired && eligible && time - beganAt <= 0.35f) return Gesture::Pause;
        return result;
    }
    void Cancel() { fingers.clear(); paired = false; eligible = false; used = false; }
private:
    struct Finger { int id; GesturePoint start; GesturePoint last; };
    std::vector<Finger> fingers;
    bool paired = false, eligible = false, used = false;
    float beganAt = 0;
    std::vector<Finger>::iterator Find(int id) {
        return std::find_if(fingers.begin(), fingers.end(), [id](const Finger& f) { return f.id == id; });
    }
};
