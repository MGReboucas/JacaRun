#pragma once
#include "CharacterAnimation.h"
#include "DressedJacaAtlas.h"
#include "Profile.h"

struct JacaArtwork {
    const char* file;
    AtlasCrop crop;
    float density;
};

static_assert(sizeof(DressedJacaAtlases)/sizeof(DressedJacaAtlases[0]) == CosmeticCount-1,
              "Every shop visual needs a complete 24-pose character atlas");

inline JacaArtwork ResolveJacaArtwork(int skin, JacaAtlas atlas, int frame) {
    frame=std::clamp(frame,0,7);
    if(skin==5 && atlas==JacaAtlas::Run)
        return {CrownRunAtlas.file,CrownRunAtlas.frames[frame],CrownRunAtlas.density};
    if(skin==9 && atlas!=JacaAtlas::Action) {
        const auto& explorer=atlas==JacaAtlas::Idle?ExplorerIdleAtlas:ExplorerRunAtlas;
        return {explorer.file,explorer.frames[frame],explorer.density};
    }
    if(skin>0 && skin<CosmeticCount) {
        const auto& dressed=DressedJacaAtlases[skin-1];
        const int offset=atlas==JacaAtlas::Idle?0:atlas==JacaAtlas::Run?8:16;
        return {dressed.file,dressed.frames[offset+frame],dressed.density};
    }
    if(atlas==JacaAtlas::Idle) return {"art/jaca-idle-v2.png",IdleCrops[frame],1};
    if(atlas==JacaAtlas::Run) return {"art/jaca-run-v2.png",RunCrops[frame],1};
    return {"art/jaca-actions-v2.png",ActionCrops[frame],1};
}
