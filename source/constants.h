
// -- Some SUPER IMPORTANT constants

#define VERSION "1.0.0"
#define SAVE_BASE "/3ds/junkdraw/saves/"

#define SCREENWIDTH 320
#define SCREENHEIGHT 240

#define LAYER_WIDTH 1024
#define LAYER_HEIGHT 1024
#define LAYER_FORMAT GPU_RGBA5551
#define LAYER_EDGEBUF 8
#define LAYER_COUNT 2

#define CPAD_DEADZONE 40
#define CPAD_THEORETICALMAX 160
#define CPAD_PAGECONST 1
#define CPAD_PAGEMULT 0.02f
#define CPAD_PAGECURVE 3.2f

#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7

#define MAX_DRAW_DATA 5000000
#define MAX_STROKE_LINES 5000
#define MAX_STROKE_DATA MAX_STROKE_LINES << 3
#define MIN_ZOOMPOWER -2
#define MAX_ZOOMPOWER 4
#define MIN_WIDTH 1
#define MAX_WIDTH 64
#define MAX_PAGE 0xFFFF
#define MAX_FRAMELINES 500
#define MAX_DRAWDATA_SCAN 100000
#define MAX_FILENAME 256
#define MAX_FILENAMESHOW 5
#define MAX_ALLFILENAMES 65535

#define DRAWDATA_ALIGNMENT '.'

#define TOOL_PENCIL 0
#define TOOL_ERASER 1
#define TOOL_COUNT 2
#define TOOL_CHARS "pe"

#define LINESTYLE_STROKE 0

#define SCREEN_COLOR C2D_Color32(90,90,90,255)
#define CANVAS_BG_COLOR C2D_Color32(255,255,255,255)
#define CANVAS_LAYER_COLOR C2D_Color32(0,0,0,0)

#define SCROLL_WIDTH 3
#define SCROLL_BG C2D_Color32f(0.8,0.8,0.8,1)
#define SCROLL_BAR C2D_Color32f(0.5,0.5,0.5,1)

#define PALETTE_COLORS 64
#define PALETTE_SWATCHWIDTH 18
#define PALETTE_SWATCHMARGIN 2
#define PALETTE_SELECTED_COLOR C2D_Color32f(1,0,0,1)
#define PALETTE_BG C2D_Color32f(0.3,0.3,0.3,1)
#define PALETTE_STARTINDEX 1
#define PALETTE_MINISIZE 10
#define PALETTE_MINIPADDING 1
#define PALETTE_OFSX 10
#define PALETTE_OFSY 10

#define MAINMENU_TOP 6
#define MAINMENU_TITLE "Main menu:"
#define MAINMENU_ITEMS "New\0Save\0Load\0Host Local\0Connect Local\0Exit App\0"
#define MAINMENU_NEW 0
#define MAINMENU_SAVE 1
#define MAINMENU_LOAD 2
#define MAINMENU_HOSTLOCAL 3
#define MAINMENU_CONNECTLOCAL 4
#define MAINMENU_EXIT 5

#define DEBUG_PRINT_MINROW 21
#define DEBUG_PRINT_ROWS 5
#define STATUS_MAINCOLOR 36
#define STATUS_ACTIVECOLOR 37

//Weird stuff that should maybe be refactored
#define MY_C2DOBJLIMIT 16384
#define MY_C2DOBJLIMITSAFETY MY_C2DOBJLIMIT - 100
#define PSX1BLEN 30

typedef u16 page_num;
typedef u8 layer_num;
//typedef u16 stroke_num;


// The base palette definition

u32 base_palette[] = { 
   0xff0040, 0x131313, 0x1b1b1b, 0x272727, 0x3d3d3d, 0x5d5d5d, 0x858585, 0xb4b4b4,
   0xffffff, 0xc7cfdd, 0x92a1b9, 0x657392, 0x424c6e, 0x2a2f4e, 0x1a1932, 0x0e071b,
   0x1c121c, 0x391f21, 0x5d2c28, 0x8a4836, 0xbf6f4a, 0xe69c69, 0xf6ca9f, 0xf9e6cf,
   0xedab50, 0xe07438, 0xc64524, 0x8e251d, 0xff5000, 0xed7614, 0xffa214, 0xffc825,
   0xffeb57, 0xd3fc7e, 0x99e65f, 0x5ac54f, 0x33984b, 0x1e6f50, 0x134c4c, 0x0c2e44,
   0x00396d, 0x0069aa, 0x0098dc, 0x00cdf9, 0x0cf1ff, 0x94fdff, 0xfdd2ed, 0xf389f5,
   0xdb3ffd, 0x7a09fa, 0x3003d9, 0x0c0293, 0x03193f, 0x3b1443, 0x622461, 0x93388f,
   0xca52c9, 0xc85086, 0xf68187, 0xf5555d, 0xea323c, 0xc42430, 0x891e2b, 0x571c27
};
