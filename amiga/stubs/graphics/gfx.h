#ifndef GRAPHICS_GFX_H
#define GRAPHICS_GFX_H

#include <stdint.h>
#include "exec/exec.h"

#define GRAPHICS_GFX_H

typedef struct BitMap {
    UWORD bm_Width;
    UWORD bm_Height;
    UWORD bm_Depth;
    APTR bm_Planes[8];
    UWORD bm_RowBytes;
    UBYTE bm_Flags;
    UBYTE bm_pad;
} BitMap;

typedef struct RastPort {
    struct Layer* rp_Layer;
    UBYTE rp_Flags;
    UBYTE rp_pad;
    UWORD rp_Font;
    UWORD rp_FontStyle;
    UWORD rp_FontHeight;
    UWORD rp_FontWidth;
    UWORD rp_FontBaseLine;
    UWORD rp_FontBold;
    UWORD rp_FontItalic;
    UWORD rp_FontUnderline;
    UWORD rp_Mask;
    UWORD rp_FgPen;
    UWORD rp_BgPen;
    UWORD rp_AOlPen;
    UWORD rp_DrawMode;
    UWORD rp_AreaPtrnSize;
    APTR rp_AreaPtrn;
    APTR rp_TmpRas;
    struct AreaInfo* rp_AreaInfo;
    struct TmpRas* rp_TmpRas;
    struct GelsInfo* rp_GelsInfo;
    struct BitMap* rp_BitMap;
} RastPort;

typedef struct Layer {
    struct Node la_Node;
    UBYTE la_Flags;
    UBYTE la_pad;
    UWORD la_Width;
    UWORD la_Height;
    UWORD la_Depth;
    struct RastPort* la_RPort;
    struct BitMap* la_BitMap;
} Layer;

typedef struct ViewPort {
    struct Node vp_Node;
    UWORD vp_Width;
    UWORD vp_Height;
    UWORD vp_Depth;
    UWORD vp_Mode;
    UWORD vp_RasInfo;
    UWORD vp_RasInfoOffset;
    struct RasInfo* vp_RasInfo;
    struct ViewPort* vp_Next;
    struct ColorMap* vp_ColorMap;
    struct UCopList* vp_UCopList;
    struct CopList* vp_CopList;
    struct CopIns* vp_CopIns;
    UWORD vp_Sprites;
    UWORD vp_ColorMapEntries;
    UWORD vp_Reserved;
    UWORD vp_Flags;
} ViewPort;

typedef struct RasInfo {
    struct RasInfo* ri_Next;
    struct BitMap* ri_BitMap;
    UWORD ri_RxOffset;
    UWORD ri_RyOffset;
    UWORD ri_Width;
    UWORD ri_Height;
} RasInfo;

typedef struct ColorMap {
    struct Node cm_Node;
    UWORD cm_Flags;
    UWORD cm_Entries;
    APTR cm_Colors;
} ColorMap;

typedef struct AreaInfo {
    UWORD ai_Width;
    UWORD ai_Height;
    APTR ai_Buffer;
    UWORD ai_BufferSize;
    UWORD ai_BufferMax;
    UWORD ai_Count;
} AreaInfo;

typedef struct TmpRas {
    APTR tr_Ras;
    UWORD tr_Width;
    UWORD tr_Height;
    UWORD tr_Size;
} TmpRas;

typedef struct GelsInfo {
    struct VSprite* gi_Sprite;
    struct VSprite* gi_SpriteTail;
    struct VSprite* gi_SpriteHead;
    struct VSprite* gi_SpriteCursor;
    struct VSprite* gi_SpriteMax;
    UWORD gi_SpriteCount;
    UWORD gi_SpriteMaxCount;
} GelsInfo;

typedef struct VSprite {
    struct Node vs_Node;
    UWORD vs_Width;
    UWORD vs_Height;
    UWORD vs_Depth;
    UWORD vs_Flags;
    APTR vs_ImageData;
    APTR vs_BorderLine;
    APTR vs_CollisionData;
    APTR vs_HitMask;
    APTR vs_MaskData;
    APTR vs_SpriteData;
    APTR vs_NextSprite;
    UWORD vs_X;
    UWORD vs_Y;
    UWORD vs_OldX;
    UWORD vs_OldY;
    UWORD vs_XVel;
    UWORD vs_YVel;
    UWORD vs_Accel;
    UWORD vs_Tick;
    UWORD vs_Timer;
    UWORD vs_Flags2;
    UWORD vs_UserData;
    APTR vs_UserStuff;
} VSprite;

struct BitMap* AllocBitMap(UWORD width, UWORD height, UWORD depth, UWORD flags, struct BitMap* friendBitMap);
void FreeBitMap(struct BitMap* bitMap);
void InitRastPort(struct RastPort* rastPort);
void InitArea(struct AreaInfo* areaInfo, APTR buffer, UWORD size);
void InitTmpRas(struct TmpRas* tmpRas, APTR buffer, UWORD size);
void InitGels(struct VSprite* sprites, UWORD numSprites, struct GelsInfo* gelsInfo);

void SetRast(struct RastPort* rp, UWORD pen);
void RectFill(struct RastPort* rp, UWORD x1, UWORD y1, UWORD x2, UWORD y2);
void Move(struct RastPort* rp, UWORD x, UWORD y);
void Draw(struct RastPort* rp, UWORD x, UWORD y);
void Text(struct RastPort* rp, const char* text, UWORD length);

struct ViewPort* CreateViewPort(void);
void DeleteViewPort(struct ViewPort* viewport);
struct ColorMap* GetColorMap(UWORD entries);
void FreeColorMap(struct ColorMap* colorMap);

#endif /* GRAPHICS_GFX_H */