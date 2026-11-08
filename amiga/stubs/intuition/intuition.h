#ifndef INTUITION_INTUITION_H
#define INTUITION_INTUITION_H

#include <stdint.h>
#include "exec/exec.h"

#define INTUITION_INTUITION_H

typedef struct Screen {
    struct Node sc_Node;
    UBYTE sc_Flags;
    UBYTE sc_pad;
    UWORD sc_Width;
    UWORD sc_Height;
    UWORD sc_Depth;
    UWORD sc_Mode;
    UWORD sc_Type;
    struct Window* sc_FirstWindow;
    struct Window* sc_ActiveWindow;
    struct ViewPort sc_ViewPort;
    struct RasInfo sc_RasInfo;
} Screen;

typedef struct Window {
    struct Node wi_Node;
    UBYTE wi_Flags;
    UBYTE wi_pad;
    UWORD wi_Width;
    UWORD wi_Height;
    UWORD wi_Depth;
    UWORD wi_MinWidth;
    UWORD wi_MinHeight;
    UWORD wi_MaxWidth;
    UWORD wi_MaxHeight;
    struct Screen* wi_Screen;
    struct RastPort* wi_RPort;
    struct Layer* wi_Layer;
} Window;

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
} RastPort;

typedef struct Layer {
    struct Node la_Node;
    UBYTE la_Flags;
    UBYTE la_pad;
    UWORD la_Width;
    UWORD la_Height;
    UWORD la_Depth;
    struct RastPort* la_RPort;
} Layer;

typedef struct IntuiMessage {
    struct Message im_Message;
    UWORD im_Class;
    UWORD im_Code;
    UWORD im_Qualifier;
    UWORD im_MouseX;
    UWORD im_MouseY;
    LONG im_Seconds;
    LONG im_Micros;
    struct Window* im_Window;
    APTR im_IAddress;
    struct InputEvent* im_InputEvent;
} IntuiMessage;

typedef struct InputEvent {
    struct Node ie_Node;
    UWORD ie_Class;
    UWORD ie_SubClass;
    UWORD ie_Code;
    UWORD ie_Qualifier;
    UWORD ie_X;
    UWORD ie_Y;
    LONG ie_TimeSeconds;
    LONG ie_TimeMicros;
    APTR ie_EventAddress;
    struct InputEvent* ie_NextEvent;
} InputEvent;

typedef struct NewScreen {
    UWORD ns_Width;
    UWORD ns_Height;
    UWORD ns_Depth;
    UWORD ns_Mode;
    UWORD ns_Type;
    char* ns_Font;
    char* ns_DefaultTitle;
    struct Gadget* ns_Gadgets;
    struct Image* ns_Image;
} NewScreen;

typedef struct NewWindow {
    UWORD nw_LeftEdge;
    UWORD nw_TopEdge;
    UWORD nw_Width;
    UWORD nw_Height;
    UWORD nw_Depth;
    UWORD nw_MinWidth;
    UWORD nw_MinHeight;
    UWORD nw_MaxWidth;
    UWORD nw_MaxHeight;
    UBYTE nw_DetailPen;
    UBYTE nw_BlockPen;
    UWORD nw_IDCMPFlags;
    UWORD nw_Flags;
    struct Gadget* nw_Gadgets;
    struct Image* nw_Image;
    char* nw_Title;
    struct Screen* nw_Screen;
    struct BitMap* nw_BitMap;
    UWORD nw_MinWidth2;
    UWORD nw_MinHeight2;
    UWORD nw_MaxWidth2;
    UWORD nw_MaxHeight2;
    UWORD nw_Type;
} NewWindow;

typedef struct Gadget {
    struct Node gg_Node;
    UWORD gg_Width;
    UWORD gg_Height;
    UWORD gg_LeftEdge;
    UWORD gg_TopEdge;
    UWORD gg_Flags;
    UWORD gg_Activation;
    UWORD gg_GadgetType;
    APTR gg_GadgetRender;
    APTR gg_SelectRender;
    APTR gg_GadgetText;
    APTR gg_SpecialInfo;
    UWORD gg_GadgetID;
    APTR gg_UserData;
    struct Gadget* gg_NextGadget;
} Gadget;

typedef struct Image {
    UWORD im_Width;
    UWORD im_Height;
    UWORD im_LeftEdge;
    UWORD im_TopEdge;
    UWORD im_Depth;
    APTR im_ImageData;
    UWORD im_PlanePick;
    UWORD im_PlaneOnOff;
    struct Image* im_NextImage;
} Image;

typedef struct BitMap {
    UWORD bm_Width;
    UWORD bm_Height;
    UWORD bm_Depth;
    APTR bm_Planes[8];
} BitMap;

struct Screen* OpenScreen(struct NewScreen* newScreen);
void CloseScreen(struct Screen* screen);
struct Window* OpenWindow(struct NewWindow* newWindow);
void CloseWindow(struct Window* window);
struct IntuiMessage* GetMsg(struct MsgPort* port);
void ReplyMsg(struct Message* message);

#endif /* INTUITION_INTUITION_H */