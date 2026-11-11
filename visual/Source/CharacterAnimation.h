#pragma once
#include "CharacterMotion.h"

enum class JacaAtlas { Run, Idle, Action };
struct JacaPose {
    JacaAtlas atlas = JacaAtlas::Run;
    int frame = 0;
    float width = 100, height = 100;
    float scaleX = 1, scaleY = 1, angle = 0, bob = 0;
};

inline JacaPose AnimateJaca(const CharacterMotion& motion, bool idle, bool grounded,
                            bool sliding, bool diving, bool defeated, double velocityY,
                            float entranceTime = -1) {
    JacaPose p;
    p.frame=std::min(7,static_cast<int>(motion.phase*8/6.2831853f));
    p.scaleX=1+.12f*motion.landing-.05f*motion.lift;
    p.scaleY=1-.17f*motion.landing+.09f*motion.lift;
    if(idle) {
        p.atlas=JacaAtlas::Idle; p.width=84; p.height=103;
        const float t=std::fmod(motion.time,9.0f);
        p.frame=t<1.2f?0:t<2.3f?1:t<2.45f?2:t<3.9f?3:t<4.8f?4:t<5.5f?0:t<6.25f?5:t<7.6f?3:t<7.75f?2:0;
        p.scaleX=1-std::sin(motion.time*2.2f)*.012f;
        p.scaleY=1+std::sin(motion.time*2.2f)*.018f;
        p.angle=std::sin(motion.time*1.4f)*.7f;
        if(p.frame==4) p.angle+=std::sin(motion.time*12)*2.0f;
        if(entranceTime>=0 && entranceTime<3.6f) {
            if(entranceTime<.85f) { p.frame=6; p.width=106; p.height=72; }
            else if(entranceTime<1.65f) { p.frame=7; p.width=92; p.height=98; p.angle=-5; }
            else if(entranceTime<2.1f) {
                p.frame=6; p.width=106; p.height=72;
                const float t=(entranceTime-1.65f)/.45f;
                p.width=106-22*t; p.height=72+31*t;
                p.scaleY=.82f+.18f*t;
            } else if(entranceTime<2.55f) { p.frame=5; p.angle=std::sin(entranceTime*28)*2; }
            else { p.frame=4; p.angle=std::sin(entranceTime*12)*2; }
        }
        return p;
    }
    if(defeated) {
        p.atlas=JacaAtlas::Action;
        p.frame=motion.defeat<.28f?6:7;
        p.width=p.frame==6?103:124; p.height=p.frame==6?100:51;
        p.bob=motion.defeat<.28f?std::sin(motion.defeat/.28f*3.14159265f)*13:0;
        p.angle=p.frame==6?-12*std::sin(motion.defeat/.28f*3.14159265f):0;
        const float settle=std::clamp((motion.defeat-.28f)/.3f,0.0f,1.0f);
        p.scaleY=p.frame==7?1-.16f*std::sin(settle*3.14159265f):1;
        p.scaleX=1;
        return p;
    }
    // Physical posture wins over food reactions: eating never stands up under a branch.
    if(sliding || diving || motion.slide>.12f) {
        p.atlas=JacaAtlas::Action; p.frame=3; p.width=115; p.height=43;
        p.scaleX=1+.025f*std::sin(motion.time*18);
        p.scaleY=1+.09f*(1-motion.slide);
        p.angle=diving?std::clamp(motion.lean,0.0f,10.0f):0;
        return p;
    }
    if(!grounded) {
        p.atlas=JacaAtlas::Action; p.frame=velocityY>1.5?0:velocityY<-1.5?2:1;
        p.width=p.frame==1?89:94; p.height=p.frame==1?94:104;
        p.angle=motion.lean;
        if(motion.bite>0) { p.frame=motion.bite>.19f?4:5; p.scaleX+=.04f*motion.Mouth(); }
        return p;
    }
    if(motion.hurt>0) {
        p.atlas=JacaAtlas::Action; p.frame=6; p.angle=-6*motion.hurt/.36f;
    } else if(motion.bite>0) {
        p.atlas=JacaAtlas::Action; p.frame=motion.bite>.19f?4:5;
        p.scaleX+=.05f*motion.Mouth(); p.angle=-2*motion.Mouth();
    } else {
        const float step=std::sin(motion.phase*2);
        p.bob=std::max(0.0f,step)*2.6f;
        p.scaleX+=step*.018f; p.scaleY-=step*.022f;
        p.angle=std::sin(motion.phase)*1.3f;
    }
    return p;
}
