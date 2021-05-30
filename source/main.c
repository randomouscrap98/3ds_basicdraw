#include <3ds.h>
#include <citro3d.h>
#include "../include/citro2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "constants.h"

//#define DEBUG_COORD
#define DEBUG_DATAPRINT
#define DEBUG_PRINT
#define DEBUG_RUNTESTS

#ifdef DEBUG_PRINT
#define LOGDBG(f_, ...) printf((f_), ## __VA_ARGS__)
#else
#define LOGDBG(f_, ...)
#endif

// TODO: Figure out these weirdness things:
// - Can't draw on the first 8 pixels along the edge of a target, system crashes
// - Fill color works perfectly fine using line/rect calls, but ClearTarget
//   MUST have the proper 16 bit format....
// - ClearTarget with a transparent color seems to make the color stick using
//   DrawLine unless a DrawRect (or perhaps other) call is performed.
// - Can't fill with transparency to clear.... 


// -- SIMPLE UTILS --

//Take an rgb WITHOUT alpha and convert to 3ds full color (with alpha)
u32 rgb_to_full(u32 rgb)
{
   return 0xFF000000 | ((rgb >> 16) & 0x0000FF) | (rgb & 0x00FF00) | ((rgb << 16) & 0xFF0000);
}

//Convert a citro2D rgba to a weird shifted 16bit thing that Citro2D needed for proper 
//clearing. I assume it's because of byte ordering in the 3DS and Citro not
//properly converting the color for that one instance. TODO: ask about that bug
//if anyone ever gets back to you on that forum.
u32 full_to_half(u32 val)
{
   //Format: 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   //Half  : 0b GGBBBBBA RRRRRGGG 00000000 00000000
   u8 green = (val & 0x0000FF00) >> 11; //crush last three bits

   return 
      (
         (val & 0xFF000000 ? 0x0100 : 0) |   //Alpha is lowest bit on upper byte
         (val & 0x000000F8) |                //Red is slightly nice because it's already in the right position
         ((green & 0x1c) >> 2) | ((green & 0x03) << 14) | //Green is split between bytes
         ((val & 0x00F80000) >> 10)          //Blue is just shifted and crushed
      ) << 16; //first 2 bytes are trash
}

//Convert a citro2D rgba to a proper 16 bit color (but with the opposite byte
//ordering preserved)
u16 full_to_truehalf(u32 val)
{
   return full_to_half(val) >> 16;
}

//Convert a proper 16 bit citro2d color (with the opposite byte ordering
//preserved) to a citro2D rgba
u32 half_to_full(u16 val)
{
   //Half : 0b                   GGBBBBBA RRRRRGGG
   //Full : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return
      (
         (val & 0x0100 ? 0xFF000000 : 0)  |  //Alpha always 255 or 0
         ((val & 0x3E00) << 10) |            //Blue just shift a lot
         ((val & 0x0007) << 13) | ((val & 0xc000) >> 3) |  //Green is split
         (val & 0x00f8)                      //Red is easy, already in the right place
      );
}

//Convert a whole palette from regular RGB (no alpha) to true 16 bit values
//used for in-memory palettes (and written drawing data)
void convert_palette(u32 * original, u16 * destination, u16 size)
{
   for(int i = 0; i < size; i++)
      destination[i] = full_to_truehalf(rgb_to_full(original[i]));
}



// -- DATA CONVERT UTILS --
#define DCV_START 48
#define DCV_BITSPER 6
#define DCV_VARIBITSPER 5
#define DCV_MAXVAL(x) ((1 << (x * DCV_BITSPER)) - 1)
#define DCV_VARIMAXVAL(x) ((1 << (x * DCV_VARIBITSPER)) - 1)
#define DCV_VARISTEP (1 << DCV_VARIBITSPER) 
#define DCV_VARIMAXSCAN 7

//Container needs at least 'chars' amount of space to store this int
char * int_to_chars(u32 num, const u8 chars, char * container)
{
   //WARN: clamping rather than ignoring! Hope this is ok
	num = C2D_Clamp(num, 0, DCV_MAXVAL(chars)); 

   //Place each converted character, Little Endian
	for(int i = 0; i < chars; i++)
      container[i] = DCV_START + ((num >> (i * DCV_BITSPER)) & DCV_MAXVAL(1));

   //Return the next place you can start placing characters (so you can
   //continue reusing this function)
	return container + chars;
}

//Container should always be the start of where you want to read the int.
//The container should have at least count available. You can increment
//container by count afterwards (always)
u32 chars_to_int(const char * container, const u8 count)
{
   u32 result = 0;
   for(int i = 0; i < count; i++)
      result += ((container[i] - DCV_START) << (i * DCV_BITSPER));
   return result;
}

//A dumb form of 2's compliment that doesn't carry the leading 1's
s32 special_to_signed(u32 special)
{
   if(special & 1)
      return ((special >> 1) * -1) -1;
   else
      return special >> 1;
}

u32 signed_to_special(s32 value)
{
   if(value >= 0)
      return value << 1;
   else
      return ((value << 1) * -1) - 1;
}

//Container needs at LEAST 8 bytes free to store a variable width int
char * int_to_varwidth(u32 value, char * container)
{
   u8 c = 0;
   u8 i = 0;

   do 
   {
      c = value & DCV_VARIMAXVAL(1);
      value = value >> DCV_VARIBITSPER;
      if(value) c |= DCV_VARISTEP; //Continue on, set the uppermost bit
      container[i++] = DCV_START + c;
   } 
   while(value);

   if(i >= DCV_VARIMAXSCAN)
      LOGDBG("WARN: variable width create too long: %d\n",i);

   //Return the NEXT place you can place values (just like the other func)
   return container + i;
}

//Read a variable width value from the given container. Stops if it goes too
//far though, which may give bad values
u32 varwidth_to_int(char * container, u8 * read_count)
{
   u8 c = 0;
   u8 i = 0;
   u32 result = 0;

   do 
   {
      c = container[i] - DCV_START;
      result += (c & DCV_VARIMAXVAL(1)) << (DCV_VARIBITSPER * i);
      i++;
   } 
   while(c & DCV_VARISTEP && (i < DCV_VARIMAXSCAN)); //Keep going while the high bit is set

   if(i >= DCV_VARIMAXSCAN)
      LOGDBG("WARN: variable width read too long: %d\n",i);

   *read_count = i;

   return result;
}



// -- SCREEN UTILS? --

//Represents a transformation of the screen
struct ScreenModifier
{
   float ofs_x;
   float ofs_y;
   float zoom;
};

//Generic page difference using cpad values, return the new page position given 
//the existing position and the circlepad input
float calc_pagepos(s16 d, float existing_pos)
{
   u16 cpadmag = abs(d);

   if(cpadmag > CPAD_DEADZONE)
   {
      return existing_pos + (d < 0 ? -1 : 1) * 
            (CPAD_PAGECONST + pow(cpadmag * CPAD_PAGEMULT, CPAD_PAGECURVE));
   }
   else
   {
      return existing_pos;
   }
}

//Easy way to set the screen offset (translation) safely for the given screen
//modifier. Clamps the values appropriately
void set_screenmodifier_ofs(struct ScreenModifier * mod, u16 ofs_x, u16 ofs_y)
{
   mod->ofs_x = C2D_Clamp(ofs_x, 0, PAGEWIDTH * mod->zoom - SCREENWIDTH);
   mod->ofs_y = C2D_Clamp(ofs_y, 0, PAGEHEIGHT * mod->zoom - SCREENHEIGHT);
}

//Easy way to set the screen zoom while preserving the center of the screen.
//So, the image should not appear to shift too much while zooming. The offsets
//WILL be modified after this function is completed!
void set_screenmodifier_zoom(struct ScreenModifier * mod, float zoom)
{
   float zoom_ratio = zoom / mod->zoom;
   u16 center_x = SCREENWIDTH >> 1;
   u16 center_y = SCREENHEIGHT >> 1;
   u16 new_ofsx = zoom_ratio * (mod->ofs_x + center_x) - center_x;
   u16 new_ofsy = zoom_ratio * (mod->ofs_y + center_y) - center_y;
   mod->zoom = zoom;
   set_screenmodifier_ofs(mod, new_ofsx, new_ofsy);
}

//Update screen translation based on cpad input
void update_screenmodifier(struct ScreenModifier * mod, circlePosition pos)
{
   set_screenmodifier_ofs(mod, calc_pagepos(pos.dx, mod->ofs_x), calc_pagepos(-pos.dy, mod->ofs_y));
}



// -- Stroke/line utilities --

//A line doesn't contain all the data needed to draw itself. That would be a
//line package
struct SimpleLine {
   u16 x1, y1, x2, y2;
};

// A basic representation of a collection of lines. Doesn't understand "tools"
// or any of that, it JUST has the information required to draw the lines.
struct LinePackage {
   u8 style;
   u16 color;
   u8 layer;
   u8 width;
   u16 page;
   struct SimpleLine * lines;
   u16 line_count;
};

//Add another stroke to a line collection (that represents a stroke). Works for
//the first stroke too.
struct SimpleLine * add_stroke(struct LinePackage * pending, 
      const touchPosition * pos, const struct ScreenModifier * mod)
{
   //This is for a stroke, do different things if we have different tools!
   struct SimpleLine * line = pending->lines + pending->line_count;

   line->x2 = pos->px / mod->zoom + mod->ofs_x / mod->zoom;
   line->y2 = pos->py / mod->zoom + mod->ofs_y / mod->zoom;

   if(pending->line_count == 0)
   {
      line->x1 = line->x2;
      line->y1 = line->y2;
   }
   else
   {
      line->x1 = pending->lines[pending->line_count - 1].x2;
      line->y1 = pending->lines[pending->line_count - 1].y2;
   }

   return line;
}

//A true macro, as in just dump code into the function later. Used ONLY for 
//convert_lines, hence "CVL"
#define CVL_LINECHECK if(container_size - (ptr - container) < DCV_VARIMAXSCAN) { \
   LOGDBG("ERROR: ran out of space in line conversion: original size: %ld\n",container_size); \
   return NULL; }

//Convert lines into data, but if it doesn't fit, return a null pointer.
//Returned pointer points to end + 1 in container
char * convert_lines(struct LinePackage * lines, char * container, u32 container_size)
{
   char * ptr = container;

   if(lines->line_count < 1)
   {
      LOGDBG("WARN: NO LINES TO CONVERT!\n");
      return NULL;
   }

   //This is a check that will need to be performed a lot
   CVL_LINECHECK

   //NOTE: the data is JUST the info for the lines, it's not "wrapped" into a
   //package or anything. So for instance, the length of the data is not
   //included at the start, as it may be stored in the final product

   //Dump page
   ptr = int_to_varwidth(lines->page, ptr);
   CVL_LINECHECK

   //1 byte style/layer, 1 byte width, 3 bytes color
   //3 bits of line style, 1 bit (for now) of layers, 2 unused
   ptr = int_to_chars((lines->style & 0x7) | (lines->layer << 3),1,ptr); 
   //6 bits of line width (minus 1)
   ptr = int_to_chars(lines->width - 1,1,ptr); 
   //16 bits of color (2 unused)
   ptr = int_to_chars(lines->color,3,ptr); 

   CVL_LINECHECK

   //Now for strokes, we store the first point, then move along the rest of the
   //points doing an offset storage
   if(lines->style == LINESTYLE_STROKE)
   {
      //Dump first point, save point data for later
      u16 x = lines->lines[0].x1;
      u16 y = lines->lines[0].y1;
      ptr = int_to_chars(x, 2, ptr);
      ptr = int_to_chars(y, 2, ptr);

      //Now compute distances between this point and previous, store those as
      //variable width values. This can save a significant amount for most
      //types of drawing.
      for(u16 i = 0; i < lines->line_count; i++)
      {
         CVL_LINECHECK

         if(x == lines->lines[i].x2 && y == lines->lines[i].y2)
            continue; //Don't need to store stationary lines

         ptr = int_to_varwidth(signed_to_special(lines->lines[i].x2 - x), ptr);
         ptr = int_to_varwidth(signed_to_special(lines->lines[i].y2 - y), ptr);

         x = lines->lines[i].x2;
         y = lines->lines[i].y2;
      }
   }
   else
   {
      //We DON'T support this! 
      LOGDBG("ERR: UNSUPPORTED STROKE: %d\n", lines->style);
      return NULL;
   }

   return ptr;
}



// -- DRAWING UTILS --

//Draw a rectangle centered and pixel aligned around the given point.
void draw_centeredrect(float x, float y, u16 width, u32 color)
{
   float ofs = width / 2.0;
   x = round(x - ofs);
   y = round(y - ofs);
   if(x < PAGE_EDGEBUF || y < PAGE_EDGEBUF) return;
   C2D_DrawRectSolid(x, y, 0.5f, width, width, color);
}

//Draw a line using a custom line drawing system (required like this because of
//javascript's general inability to draw non anti-aliased lines, and I want the
//strokes saved by this program to be 100% accurately reproducible on javascript)
void custom_drawline(const struct SimpleLine * line, u16 width, u32 color)
{
   float xdiff = line->x2 - line->x1;
   float ydiff = line->y2 - line->y1;
   float dist = sqrt(xdiff * xdiff + ydiff * ydiff);
   float ang = atan(ydiff/(xdiff?xdiff:0.0001))+(xdiff<0?M_PI:0);
   float xang = cos(ang);
   float yang = sin(ang);

   for(float i = 0; i <= dist; i+=0.5)
      draw_centeredrect(line->x1+xang*i, line->y1+yang*i, width, color);
}

//Draw the collection of lines given. 
//Assumes you're already on the appropriate page you want and all that
void draw_lines(const struct LinePackage * linepack, const struct ScreenModifier * mod)
{
   u32 color = half_to_full(linepack->color);

   //This was me trying to figure out what's going on with transparency
   //if(!(color & 0xFF000000))
   //{
   //   color = color | 0xAA000000;
   //   printf(".");
   //}

   struct SimpleLine * lines = linepack->lines;

   for(int i = 0; i < linepack->line_count; i++)
      custom_drawline(&lines[i], linepack->width, color);
}

//Draw the scrollbars on the sides of the screen for the given screen
//modification (translation AND zoom affect the scrollbars)
void draw_scrollbars(const struct ScreenModifier * mod)
{
   //Need to draw an n-thickness scrollbar on the right and bottom. Assumes
   //standard page size for screen modifier.

   //Bottom and right scrollbar bg
   C2D_DrawRectSolid(0, SCREENHEIGHT - SCROLL_WIDTH, 0.5f, 
         SCREENWIDTH, SCROLL_WIDTH, SCROLL_BG);
   C2D_DrawRectSolid(SCREENWIDTH - SCROLL_WIDTH, 0, 0.5f, 
         SCROLL_WIDTH, SCREENHEIGHT, SCROLL_BG);

   u16 sofs_x = (float)mod->ofs_x / PAGEWIDTH / mod->zoom * SCREENWIDTH;
   u16 sofs_y = (float)mod->ofs_y / PAGEHEIGHT / mod->zoom * SCREENHEIGHT;

   //bottom and right scrollbar bar
   C2D_DrawRectSolid(sofs_x, SCREENHEIGHT - SCROLL_WIDTH, 0.5f, 
         SCREENWIDTH * SCREENWIDTH / (float)PAGEWIDTH / mod->zoom, SCROLL_WIDTH, SCROLL_BAR);
   C2D_DrawRectSolid(SCREENWIDTH - SCROLL_WIDTH, sofs_y, 0.5f, 
         SCROLL_WIDTH, SCREENHEIGHT * SCREENHEIGHT / (float)PAGEHEIGHT / mod->zoom, SCROLL_BAR);
}

//Draw (JUST draw) the entire color picker area, which may include other
//stateful controls
void draw_colorpicker(u16 * palette, u16 palette_size, u16 selected_index)
{
   C2D_DrawRectSolid(0, 0, 0.5f, SCREENWIDTH, SCREENHEIGHT, PALETTE_BG);

   u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
   for(u16 i = 0; i < palette_size; i++)
   {
      //TODO: an implicit 8 wide thing
      u16 x = i & 7;
      u16 y = i >> 3;

      if(i == selected_index)
      {
         C2D_DrawRectSolid(
            PALETTE_OFSX + x * shift, 
            PALETTE_OFSY + y * shift, 0.5f, 
            shift, shift, PALETTE_SELECTED_COLOR);
      }

      C2D_DrawRectSolid(
         PALETTE_OFSX + x * shift + PALETTE_SWATCHMARGIN, 
         PALETTE_OFSY + y * shift + PALETTE_SWATCHMARGIN, 0.5f, 
         PALETTE_SWATCHWIDTH, PALETTE_SWATCHWIDTH, half_to_full(palette[i]));
   }
}



// -- CONTROL UTILS --

//Saved data for a tool. Each tool may (if it so desires) have different
//colors, widths, etc.
struct ToolData {
   s8 width;
   u16 color;
   u8 style;
   bool color_settable;
   //u8 color_redirect;
};

//Fill tools with default values for the start of the program.
void fill_defaulttools(struct ToolData * tool_data, u16 default_color)
{
   tool_data[TOOL_PENCIL].width = 2;
   tool_data[TOOL_PENCIL].color = default_color;
   tool_data[TOOL_PENCIL].style = LINESTYLE_STROKE;
   tool_data[TOOL_PENCIL].color_settable = true;
   //tool_data[TOOL_PENCIL].color_redirect = TOOL_PENCIL;
   tool_data[TOOL_ERASER].width = 4;
   tool_data[TOOL_ERASER].color = 0;
   tool_data[TOOL_ERASER].style = LINESTYLE_STROKE;
   tool_data[TOOL_ERASER].color_settable = false;
}

//Given a touch position (presumably on the color palette), update the selected
//palette index. 
void update_paletteindex(const touchPosition * pos, u8 * index)
{
   u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
   u16 xind = (pos->px - PALETTE_OFSX) / shift;
   u16 yind = (pos->py - PALETTE_OFSY) / shift;
   u16 new_index = (yind << 3) + xind;
   if(new_index >= 0 && new_index < PALETTE_COLORS)
      *index = new_index;
}



// -- LAYER UTILS --

//All the data associated with a single layer. TODO: still called "page"
struct PageData {
   Tex3DS_SubTexture subtex;     //Simple structures
   C3D_Tex texture;
   C2D_Image image;
   C3D_RenderTarget * target;    //Actual data?
};

//Create a LAYER from an off-screen texture.
void create_page(struct PageData * result, Tex3DS_SubTexture subtex)
{
   result->subtex = subtex;
   C3D_TexInitVRAM(&(result->texture), subtex.width, subtex.height, PAGEFORMAT);
   result->target = C3D_RenderTargetCreateFromTex(&(result->texture), GPU_TEXFACE_2D, 0, -1);
   result->image.tex = &(result->texture);
   result->image.subtex = &(result->subtex);
}

//Clean up a layer created by create_page
void delete_page(struct PageData page)
{
   C3D_RenderTargetDelete(page.target);
   C3D_TexDelete(&page.texture);
}



// -- DATA TRANSFER UTILS --

char * write_to_mem(char * stroke_data, char * stroke_end, char * mem, char * mem_end)
{
   u32 real_size = (stroke_end - stroke_data);
   u32 test_size = DCV_VARIMAXSCAN + real_size;
   u32 mem_free = MAX_DRAW_DATA - (mem_end - mem);
   char * new_end = mem_end;

   //Oops, the size of the data is more than the leftover space in draw_data!
   if(test_size > mem_free)
   {
      LOGDBG("ERR: Couldn't store lines! Required: %ld, has: %ld\n",
            test_size, mem_free);
   }
   else
   {
      //First, write the ACTUAL size (not the safe size used in the
      //calculations), then just memcopy the cvl into the draw_data
      new_end = int_to_varwidth(real_size, new_end);

      //Next, just memcpy
      memcpy(new_end, stroke_data, sizeof(char) * real_size);
      new_end += real_size;

      //no need to free or anything, we're reusing the stroke buffer
#ifdef DEBUG_DATAPRINT
      *stroke_end = '\0';
      printf("S:%ld MF:%ld D:%s\n", real_size, mem_free, stroke_data);
#endif
   }

   return new_end;
}


// -- TESTS --

#ifdef DEBUG_RUNTESTS
#include "tests.h" //Yes this is awful, don't care
#endif



// -- MAIN, OFC --

int main(int argc, char** argv)
{
   gfxInitDefault();
   C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
   C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

	//C2Di_Context* ctx = C2Di_GetContext();
	//C3D_ProcTexCombiner(&ctx->ptBlend, true, GPU_PT_U, GPU_PT_U);
   C2D_Prepare();

   consoleInit(GFX_TOP, NULL);
   C3D_RenderTarget* screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

   //weird byte order? 16 bits of color are at top
   const u32 screen_color = SCREEN_COLOR;
   const u32 bg_color = CANVAS_BG_COLOR;
   const u32 layer_color = full_to_half(CANVAS_LAYER_COLOR);

   u16 palette [PALETTE_COLORS];
   convert_palette(base_palette, palette, PALETTE_COLORS);
   u8 palette_index = PALETTE_STARTINDEX;

   const Tex3DS_SubTexture subtex = {
      PAGEWIDTH, PAGEHEIGHT,
      0.0f, 1.0f, 1.0f, 0.0f
   };

   struct PageData pages[PAGECOUNT];

   for(int i = 0; i < PAGECOUNT; i++)
      create_page(pages + i, subtex);

   struct ScreenModifier screen_mod = {0,0,1}; 

   bool touching = false;
   bool page_initialized = false;
   bool palette_active = false;

   circlePosition pos;
   touchPosition current_touch;
   u32 current_frame = 0;
   //u32 start_frame = 0;
   u32 end_frame = 0;
   s8 zoom_power = 0;
   s8 last_zoom_power = 0;

   u8 current_tool = 0;
   struct ToolData tool_data[TOOL_COUNT];
   fill_defaulttools(tool_data, palette[palette_index]);

   struct LinePackage pending;
   struct SimpleLine * pending_lines = malloc(MAX_STROKE_LINES * sizeof(struct SimpleLine));
   pending.lines = pending_lines; //Use the stack for my pending stroke
   pending.line_count = 0;
   pending.layer = PAGECOUNT - 1; //Always start on the top page
   pending.page = 0;

   char * draw_data = malloc(MAX_DRAW_DATA * sizeof(char));
   char * stroke_data = malloc(MAX_STROKE_DATA * sizeof(char));
   char * draw_data_end = draw_data;
   char * draw_pointer = draw_data;

   printf("     L - change color\n");
   printf("SELECT - change layers\n");
   printf(" C-PAD - scroll canvas\n");
   printf(" START - quit.\n\n");

#ifdef DEBUG_RUNTESTS
   run_tests();
#endif

   while(aptMainLoop())
   {
      hidScanInput();
      u32 kDown = hidKeysDown();
      u32 kUp = hidKeysUp();
      u32 kHeld = hidKeysHeld();
      hidTouchRead(&current_touch);
		hidCircleRead(&pos);

      // Respond to user input
      if(kDown & KEY_START) break;
      if(kDown & KEY_L) palette_active = !palette_active;
      if(kDown & KEY_DUP && zoom_power < MAX_ZOOM) zoom_power++;
      if(kDown & KEY_DDOWN && zoom_power > MIN_ZOOM) zoom_power--;
      if(kDown & KEY_DRIGHT) tool_data[current_tool].width += (kHeld & KEY_R ? 5 : 1);
      if(kDown & KEY_DLEFT) tool_data[current_tool].width -= (kHeld & KEY_R ? 5 : 1);
      if(kDown & KEY_SELECT) pending.layer = (pending.layer + 1) % PAGECOUNT;
      if(kDown & KEY_A) current_tool = TOOL_PENCIL;
      if(kDown & KEY_B) current_tool = TOOL_ERASER;
      if(kDown & KEY_TOUCH)
      {
         //start_frame = current_frame;
         pending.color = tool_data[current_tool].color;
         pending.style = tool_data[current_tool].style;
         pending.width = tool_data[current_tool].width;
      }
      if(kUp & KEY_TOUCH) end_frame = current_frame;

      //Update zoom separately, since the update is always the same
      if(zoom_power != last_zoom_power) set_screenmodifier_zoom(&screen_mod, pow(2, zoom_power));
      tool_data[current_tool].width = C2D_Clamp(tool_data[current_tool].width, MIN_WIDTH, MAX_WIDTH);

      if(kDown & ~(KEY_TOUCH) || !current_frame)
      {
         //TODO: make this nicer looking
         printf("\x1b[30;1HW:%02d L:%d Z:%03.2f T:%d C:%#06x",
               tool_data[current_tool].width, 
               pending.layer,
               screen_mod.zoom,
               current_tool,
               tool_data[current_tool].color
         );
      }

      touching = (kHeld & KEY_TOUCH) > 0;

      update_screenmodifier(&screen_mod, pos);

      // Render the scene
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

      //Apparently (not sure), all clearing should be done within our main loop?
      if(!page_initialized)
      {
         for(int i = 0; i < PAGECOUNT; i++)
            C2D_TargetClear(pages[i].target, layer_color); 
         page_initialized = true;
      }

      //The touch screen is used for several things
      if(touching && page_initialized)
      {
         if(palette_active)
         {
            if(tool_data[current_tool].color_settable)
            {
               update_paletteindex(&current_touch, &palette_index);
               tool_data[current_tool].color = palette[palette_index];
            }
         }
         else
         {
            //Keep this outside the if statement below so it can be used for
            //background drawing too (draw commands from other people)
            C2D_SceneBegin(pages[pending.layer].target);

            if(pending.line_count < MAX_STROKE_LINES)
            {
               //This is for a stroke, do different things if we have different tools!
               struct SimpleLine * line = add_stroke(&pending, &current_touch, &screen_mod);

               pending.lines = line; //Force the pending line to only show the end
               u16 oldcount = pending.line_count;
               pending.line_count = 1;

               //Draw ONLY the current line
               draw_lines(&pending, &screen_mod);
               //Reset pending lines to proper thing
               pending.lines = pending_lines;
               pending.line_count = oldcount + 1;
            }
         }
      }

      //TODO: Eventually, change this to put the data in different places?
      if(end_frame == current_frame && pending.line_count > 0)
      {
         char * cvl_end = convert_lines(&pending, stroke_data, MAX_STROKE_DATA);

         if(cvl_end == NULL)
         {
            LOGDBG("ERR: Couldn't convert lines!\n");
         }
         else
         {
            draw_data_end = write_to_mem(stroke_data, cvl_end, draw_data, draw_data_end);
         }

         pending.line_count = 0;
      }

      C2D_TargetClear(screen, screen_color);
      C2D_SceneBegin(screen);

      //Draw different things based on what's active on the touchscreen.
      if(palette_active)
      {
         draw_colorpicker(palette, PALETTE_COLORS, palette_index);
      }
      else
      {
         C2D_DrawRectSolid(-screen_mod.ofs_x, -screen_mod.ofs_y, 0.5f,
               PAGEWIDTH * screen_mod.zoom, PAGEHEIGHT * screen_mod.zoom, bg_color); //The bg color
         for(int i = 0; i < PAGECOUNT; i++)
         {
            C2D_DrawImageAt(pages[i].image, -screen_mod.ofs_x, -screen_mod.ofs_y, 0.5f, 
                  NULL, screen_mod.zoom, screen_mod.zoom);
         }
         //The selected color thing
         C2D_DrawRectSolid(0, 0, 0.5f, PALETTE_MINISIZE, PALETTE_MINISIZE, PALETTE_BG);
         C2D_DrawRectSolid(PALETTE_MINIPADDING, PALETTE_MINIPADDING, 0.5f, 
               PALETTE_MINISIZE - PALETTE_MINIPADDING * 2, 
               PALETTE_MINISIZE - PALETTE_MINIPADDING * 2, 
               half_to_full(palette[palette_index])); 
         draw_scrollbars(&screen_mod);
      }

      C3D_FrameEnd(0);

      last_zoom_power = zoom_power;
      current_frame++;
   }

   free(pending_lines);
   free(draw_data);
   free(stroke_data);

   for(int i = 0; i < PAGECOUNT; i++)
      delete_page(pages[i]);

   C2D_Fini();
   C3D_Fini();
   gfxExit();
   return 0;
}
