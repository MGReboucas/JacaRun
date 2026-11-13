#pragma once
#include "axmol.h"
#include "CharacterAnimation.h"
#include "CharacterAtlas.h"
#include <initializer_list>
#include <cmath>
#include "Profile.h"

// Attachment points measured on the illustrated atlases (top of hat, neck).
// Cosmetics inherit the sprite transform, including jumping, sliding and biting.
inline void DrawJacaAccessory(ax::DrawNode* d, int id, JacaAtlas atlas, int frame, const AtlasCrop& crop) {
    using namespace ax;
    auto accessory=static_cast<Sprite*>(d->getChildByTag(200));
    if(accessory) accessory->setVisible(false);
    if(id<=0 || id>=CosmeticCount) return;
    struct Anchors { float x,y,nx,ny; };
    static constexpr Anchors idle[]={{184,104,213,270},{566,101,596,265},{951,100,980,269},{1338,102,1360,269},
        {167,610,209,785},{552,611,583,780},{965,711,1016,885},{1345,578,1384,735}};
    static constexpr Anchors run[]={{213,102,247,265},{581,117,611,289},{971,99,1005,260},{1357,93,1401,262},
        {196,578,234,765},{590,645,631,806},{936,618,990,787},{1336,600,1386,766}};
    static constexpr Anchors action[]={{203,60,248,221},{561,89,601,236},{962,94,999,257},{1383,211,1410,361},
        {195,592,235,769},{569,601,603,759},{938,600,996,777},{1354,744,1395,886}};
    const auto a=(atlas==JacaAtlas::Idle?idle:atlas==JacaAtlas::Run?run:action)[frame];
    const Vec2 h(a.x-crop.x,crop.height-(a.y-crop.y));
    const Vec2 neck(a.nx-crop.x,crop.height-(a.ny-crop.y));
    // Original illustrated assets. Attachment offsets are in atlas pixels,
    // so previews and animated poses share the exact same fitting.
    static const Rect originalTiles[]={{32,82,448,312},{545,172,444,156},{1048,82,458,313},
        {33,646,426,292},{571,576,403,334},{1067,562,412,402}};
    static const Size originalSizes[]={{185,130},{110,40},{205,140},{145,99},{155,128},{285,278}};
    static const Vec2 originalOffsets[]={{-118,-98},{-12,-88},{-112,-75},{-84,-58},{-65,-60},{-103,-210}};
    if(!accessory) {
        accessory=Sprite::create("art/jaca-cosmetics-v2.png");
        d->addChild(accessory,1,200);
    }
    Rect tile;
    Size size;
    Vec2 offset;
    bool atNeck=id==4 || id==7;
    if(id<=6) {
        accessory->setTexture("art/jaca-cosmetics-v2.png");
        tile=originalTiles[id-1]; size=originalSizes[id-1]; offset=originalOffsets[id-1];
    } else {
        accessory->setTexture("art/jaca-cosmetics-expansion-v1.png");
        static const Rect tiles[]={{17,91,383,276},{437,134,372,147},{847,71,391,274},
            {28,465,356,292},{437,456,371,322},{841,504,396,237},
            {16,934,388,243},{447,878,352,297},{844,861,386,318}};
        static const Size sizes[]={{145,104},{110,43},{205,144},{160,131},{175,152},
            {205,123},{160,100},{170,143},{180,148}};
        static const Vec2 offsets[]={{-84,-58},{-12,-88},{-112,-75},{-68,-60},{-85,-65},
            {-100,-72},{-90,-85},{-75,-58},{-78,-60}};
        const int index=id-7;
        tile=tiles[index]; size=sizes[index]; offset=offsets[index];
    }
    accessory->setVisible(true);
    accessory->setTextureRect(tile);
    accessory->setAnchorPoint({0,0});
    accessory->setPosition((atNeck?neck:h)+offset);
    accessory->setScaleX(size.width/tile.size.width);
    accessory->setScaleY(size.height/tile.size.height);
    accessory->setRotation(0);
    accessory->setColor(Color3B::WHITE);
}
