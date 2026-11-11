#pragma once
#include <algorithm>
struct StoryIntro {
    static constexpr int BeatCount=5;
    static constexpr float BeatSeconds=2.5f;
    static constexpr float Duration=BeatSeconds*BeatCount;
    float time=0;
    bool active=true;
    int Beat() const { return std::min(BeatCount-1,static_cast<int>(time/BeatSeconds)); }
    float Phase() const { return std::clamp((time-Beat()*BeatSeconds)/BeatSeconds,0.0f,1.0f); }
    void Update(float dt) { if(active) { time=std::min(Duration,time+std::clamp(dt,0.0f,.1f)); if(time>=Duration) active=false; } }
    void Skip() { time=Duration; active=false; }
};
