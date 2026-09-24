#pragma once
#include "GameManager.h"
#include <algorithm>
#include <vector>

// Collision events are consumed once by the core. Their pictures have a separate lifetime.
struct VisualEntity {
    Entity entity;
    EntityOutcome outcome = EntityOutcome::Pending;
    float effectTime = 0;
};

class RunPresentation {
public:
    static constexpr float CollectionSeconds = 0.36f;
    void Reset() { objects.clear(); }
    void Sync(const std::vector<Entity>& upcoming) {
        for (const auto& entity : upcoming) {
            if (Find(entity.id) == objects.end()) objects.push_back({entity});
        }
    }
    void Resolve(const std::vector<EntityResolution>& results) {
        for (const auto& result : results) {
            auto found = Find(result.entity.id);
            if (found == objects.end()) {
                objects.push_back({result.entity, result.outcome, 0});
            } else if (found->outcome == EntityOutcome::Pending) {
                found->outcome = result.outcome;
                found->effectTime = 0;
            }
        }
    }
    void Update(float dt, double distance, float contact, float pixelsPerMeter) {
        for (auto& object : objects) {
            if (object.outcome == EntityOutcome::Collected) object.effectTime += dt;
        }
        objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const VisualEntity& object) {
            const double x = contact + (object.entity.distance - distance) * pixelsPerMeter;
            return x < -120 || (object.outcome == EntityOutcome::Collected && object.effectTime >= CollectionSeconds);
        }), objects.end());
    }
    const std::vector<VisualEntity>& GetObjects() const { return objects; }
private:
    std::vector<VisualEntity> objects;
    std::vector<VisualEntity>::iterator Find(std::uint64_t id) {
        return std::find_if(objects.begin(), objects.end(), [id](const VisualEntity& o) { return o.entity.id == id; });
    }
};
