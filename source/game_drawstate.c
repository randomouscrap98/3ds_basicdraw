
#include <citro2d.h>
#include <stdlib.h>

#include "lib/myutils.h"
#include "game_drawstate.h"


struct ToolData default_tooldata[] = {
   //Pencil
   { 2, LINESTYLE_STROKE, false },
   //Eraser
   { 4, LINESTYLE_STROKE, true, 0 }
};

u32 default_palette[] = { 
   //Endesga 64
   0xff0040, 0x131313, 0x1b1b1b, 0x272727, 0x3d3d3d, 0x5d5d5d, 0x858585, 0xb4b4b4,
   0xffffff, 0xc7cfdd, 0x92a1b9, 0x657392, 0x424c6e, 0x2a2f4e, 0x1a1932, 0x0e071b,
   0x1c121c, 0x391f21, 0x5d2c28, 0x8a4836, 0xbf6f4a, 0xe69c69, 0xf6ca9f, 0xf9e6cf,
   0xedab50, 0xe07438, 0xc64524, 0x8e251d, 0xff5000, 0xed7614, 0xffa214, 0xffc825,
   0xffeb57, 0xd3fc7e, 0x99e65f, 0x5ac54f, 0x33984b, 0x1e6f50, 0x134c4c, 0x0c2e44,
   0x00396d, 0x0069aa, 0x0098dc, 0x00cdf9, 0x0cf1ff, 0x94fdff, 0xfdd2ed, 0xf389f5,
   0xdb3ffd, 0x7a09fa, 0x3003d9, 0x0c0293, 0x03193f, 0x3b1443, 0x622461, 0x93388f,
   0xca52c9, 0xc85086, 0xf68187, 0xf5555d, 0xea323c, 0xc42430, 0x891e2b, 0x571c27,

   //Resurrect
   0x2e222f, 0x3e3546, 0x625565, 0x966c6c, 0xab947a, 0x694f62, 0x7f708a, 0x9babb2,
   0xc7dcd0, 0xffffff, 0x6e2727, 0xb33831, 0xea4f36, 0xf57d4a, 0xae2334, 0xe83b3b,
   0xfb6b1d, 0xf79617, 0xf9c22b, 0x7a3045, 0x9e4539, 0xcd683d, 0xe6904e, 0xfbb954,
   0x4c3e24, 0x676633, 0xa2a947, 0xd5e04b, 0xfbff86, 0x165a4c, 0x239063, 0x1ebc73,
   0x91db69, 0xcddf6c, 0x313638, 0x374e4a, 0x547e64, 0x92a984, 0xb2ba90, 0x0b5e65,
   0x0b8a8f, 0x0eaf9b, 0x30e1b9, 0x8ff8e2, 0x323353, 0x484a77, 0x4d65b4, 0x4d9be6,
   0x8fd3ff, 0x45293f, 0x6b3e75, 0x905ea9, 0xa884f3, 0xeaaded, 0x753c54, 0xa24b6f,
   0xcf657f, 0xed8099, 0x831c5d, 0xc32454, 0xf04f78, 0xf68181, 0xfca790, 0xfdcbb0,

   //AAP-64
   0x060608, 0x141013, 0x3b1725, 0x73172d, 0xb4202a, 0xdf3e23, 0xfa6a0a, 0xf9a31b,
   0xffd541, 0xfffc40, 0xd6f264, 0x9cdb43, 0x59c135, 0x14a02e, 0x1a7a3e, 0x24523b,
   0x122020, 0x143464, 0x285cc4, 0x249fde, 0x20d6c7, 0xa6fcdb, 0xffffff, 0xfef3c0,
   0xfad6b8, 0xf5a097, 0xe86a73, 0xbc4a9b, 0x793a80, 0x403353, 0x242234, 0x221c1a,
   0x322b28, 0x71413b, 0xbb7547, 0xdba463, 0xf4d29c, 0xdae0ea, 0xb3b9d1, 0x8b93af,
   0x6d758d, 0x4a5462, 0x333941, 0x422433, 0x5b3138, 0x8e5252, 0xba756a, 0xe9b5a3,
   0xe3e6ff, 0xb9bffb, 0x849be4, 0x588dbe, 0x477d85, 0x23674e, 0x328464, 0x5daf8d,
   0x92dcba, 0xcdf7e2, 0xe4d2aa, 0xc7b08b, 0xa08662, 0x796755, 0x5a4e44, 0x423934,

   //Famicube
   0x000000, 0xe03c28, 0xffffff, 0xd7d7d7, 0xa8a8a8, 0x7b7b7b, 0x343434, 0x151515,
   0x0d2030, 0x415d66, 0x71a6a1, 0xbdffca, 0x25e2cd, 0x0a98ac, 0x005280, 0x00604b,
   0x20b562, 0x58d332, 0x139d08, 0x004e00, 0x172808, 0x376d03, 0x6ab417, 0x8cd612,
   0xbeeb71, 0xeeffa9, 0xb6c121, 0x939717, 0xcc8f15, 0xffbb31, 0xffe737, 0xf68f37,
   0xad4e1a, 0x231712, 0x5c3c0d, 0xae6c37, 0xc59782, 0xe2d7b5, 0x4f1507, 0x823c3d,
   0xda655e, 0xe18289, 0xf5b784, 0xffe9c5, 0xff82ce, 0xcf3c71, 0x871646, 0xa328b3,
   0xcc69e4, 0xd59cfc, 0xfec9ed, 0xe2c9ff, 0xa675fe, 0x6a31ca, 0x5a1991, 0x211640,
   0x3d34a5, 0x6264dc, 0x9ba0ef, 0x98dcff, 0x5ba8ff, 0x0a89ff, 0x024aca, 0x00177d,

   //Sweet Canyon 64
   0x0f0e11, 0x2d2c33, 0x40404a, 0x51545c, 0x6b7179, 0x7c8389, 0xa8b2b6, 0xd5d5d5,
   0xeeebe0, 0xf1dbb1, 0xeec99f, 0xe1a17e, 0xcc9562, 0xab7b49, 0x9a643a, 0x86482f,
   0x783a29, 0x6a3328, 0x541d29, 0x42192c, 0x512240, 0x782349, 0x8b2e5d, 0xa93e89,
   0xd062c8, 0xec94ea, 0xf2bdfc, 0xeaebff, 0xa2fafa, 0x64e7e7, 0x54cfd8, 0x2fb6c3,
   0x2c89af, 0x25739d, 0x2a5684, 0x214574, 0x1f2966, 0x101445, 0x3c0d3b, 0x66164c,
   0x901f3d, 0xbb3030, 0xdc473c, 0xec6a45, 0xfb9b41, 0xf0c04c, 0xf4d66e, 0xfffb76,
   0xccf17a, 0x97d948, 0x6fba3b, 0x229443, 0x1d7e45, 0x116548, 0x0c4f3f, 0x0a3639,
   0x251746, 0x48246d, 0x69189c, 0x9f20c0, 0xe527d2, 0xff51cf, 0xff7ada, 0xff9edb,

   //Rewild 64
   0x172038, 0x253a5e, 0x3c5e8b, 0x4f8fba, 0x73bed3, 0xa4dddb, 0x193024, 0x245938,
   0x2b8435, 0x62ac4c, 0xa2dc6e, 0xc5e49b, 0x19332d, 0x25562e, 0x468232, 0x75a743,
   0xa8ca58, 0xd0da91, 0x5f6d43, 0x97933a, 0xa9b74c, 0xcfd467, 0xd5dc97, 0xd6dea6,
   0x382a28, 0x43322f, 0x564238, 0x715a42, 0x867150, 0xb1a282, 0x4d2b32, 0x7a4841,
   0xad7757, 0xc09473, 0xd7b594, 0xe7d5b3, 0x341c27, 0x602c2c, 0x884b2b, 0xbe772b,
   0xde9e41, 0xe8c170, 0x241527, 0x411d31, 0x752438, 0xa53030, 0xcf573c, 0xda863e,
   0x1e1d39, 0x402751, 0x7a367b, 0xa23e8c, 0xc65197, 0xdf84a5, 0x090a14, 0x10141f,
   0x151d28, 0x202e37, 0x394a50, 0x577277, 0x819796, 0xa8b5b2, 0xc7cfcc, 0xebede9

};


void set_screenstate_defaults(struct ScreenState * state)
{
   state->offset_x = 0;
   state->offset_y = 0;
   state->zoom = 1;
   state->layer_width = DEFAULT_LAYER_WIDTH;
   state->layer_height = DEFAULT_LAYER_HEIGHT;
   state->screen_width = 320;  //These two should literally never change 
   state->screen_height = 240; //GSP_SCREEN_WIDTH; //GSP_SCREEN_HEIGHT_BOTTOM;
   state->screen_color = DEFAULT_SCREEN_COLOR;
   state->bg_color = DEFAULT_BG_COLOR;
}

void set_screenstate_offset(struct ScreenState * state, u16 offset_x, u16 offset_y)
{
   float maxofsx = state->layer_width * state->zoom - state->screen_width;
   float maxofsy = state->layer_height * state->zoom - state->screen_height;
   state->offset_x = C2D_Clamp(offset_x, 0, maxofsx < 0 ? 0 : maxofsx);
   state->offset_y = C2D_Clamp(offset_y, 0, maxofsy < 0 ? 0 : maxofsy);
}

void set_screenstate_zoom(struct ScreenState * state, float zoom)
{
   float zoom_ratio = zoom / state->zoom;
   u16 center_x = state->screen_width >> 1;
   u16 center_y = state->screen_height >> 1;
   u16 new_ofsx = zoom_ratio * (state->offset_x + center_x) - center_x;
   u16 new_ofsy = zoom_ratio * (state->offset_y + center_y) - center_y;
   state->zoom = zoom;
   set_screenstate_offset(state, new_ofsx, new_ofsy);
}

void init_default_drawstate(struct DrawState * state)
{
   state->zoom_power = 0;
   state->page = 0;
   state->layer = DEFAULT_START_LAYER; 

   state->palette_count = sizeof(default_palette) / sizeof(u32);
   state->palette = malloc(state->palette_count * sizeof(u16));
   convert_palette(default_palette, state->palette, state->palette_count);
   state->current_color = state->palette + DEFAULT_PALETTE_STARTINDEX;

   state->tools = malloc(sizeof(default_tooldata));
   memcpy(state->tools, default_tooldata, sizeof(default_tooldata));
   state->current_tool = state->tools + TOOL_PENCIL;

   state->min_width = DEFAULT_MIN_WIDTH;
   state->max_width = DEFAULT_MAX_WIDTH;
}

//Only really applies to default initialized states
void free_drawstate(struct DrawState * state)
{
   free(state->tools);
   free(state->palette);
}

void shift_drawstate_color(struct DrawState * state, s16 ofs)
{
   u16 i = state->current_color - state->palette;
   state->current_color = state->palette +
      ((i + ofs + state->palette_count) % (state->palette_count));
}

void shift_drawstate_width(struct DrawState * state, s16 ofs)
{
   //instead of rolling, this one clamps
   state->current_tool->width =
      C2D_Clamp(state->current_tool->width + ofs, state->min_width, state->max_width);
}

u16 get_drawstate_color(struct DrawState * state)
{
   return state->current_tool->has_static_color ? 
      state->current_tool->static_color : *state->current_color;
}

void set_drawstate_tool(struct DrawState * state, u8 tool)
{
   state->current_tool = state->tools + tool;
}
