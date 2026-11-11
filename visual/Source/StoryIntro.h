#pragma once
#include <algorithm>
struct StoryIntro {
    static constexpr int BeatCount=6;
    inline static constexpr float Beats[BeatCount]={2.4f,2.6f,1.9f,2.6f,2.6f,3.1f};
    static constexpr float Duration=15.2f;
    static int BiteFrame(float phase) { return phase<.08f?0:phase<.32f?1:phase<.72f?2:3; }
    float time=0;
    bool active=true;
    static float Start(int beat) {
        float result=0; for(int i=0;i<std::clamp(beat,0,BeatCount);++i) result+=Beats[i]; return result;
    }
    int Beat() const {
        for(int i=1;i<BeatCount;++i) if(time<Start(i)) return i-1;
        return BeatCount-1;
    }
    float Phase() const {
        if(time>=Duration) return 1;
        return std::clamp((time-Start(Beat()))/Beats[Beat()],0.0f,1.0f);
    }
    void Update(float dt) { if(active) { time=std::min(Duration,time+std::clamp(dt,0.0f,.1f)); if(time>=Duration) active=false; } }
    void Skip() { time=Duration; active=false; }
};
