#include "myutils.h"
#include "string.h"
#include <citro3d.h>

#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>


// -------------------
// -- GENERAL UTILS --
// -------------------




// -----------------
// -- COLOR UTILS --
// -----------------

//Take an rgb WITHOUT alpha and convert to 3ds full color (with alpha)
u32 rgb24_to_rgba32c(u32 rgb)
{
   return 0xFF000000 | ((rgb >> 16) & 0x0000FF) | (rgb & 0x00FF00) | ((rgb << 16) & 0xFF0000);
}

//Convert a citro2D rgba to a weird shifted 16bit thing that Citro2D needed for proper 
//clearing. I assume it's because of byte ordering in the 3DS and Citro not
//properly converting the color for that one instance. TODO: ask about that bug
//if anyone ever gets back to you on that forum.
u32 rgba32c_to_rgba16c_32(u32 val)
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
u16 rgba32c_to_rgba16c(u32 val)
{
   return rgba32c_to_rgba16c_32(val) >> 16;
}

//Convert a proper 16 bit citro2d color (with the opposite byte ordering
//preserved) to a citro2D rgba
u32 rgba16c_to_rgba32c(u16 val)
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

u16 rgba32c_to_rgba16(u32 val)
{
   //16 : 0b                   ARRRRRGG GGGBBBBB
   //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return 
      ((val & 0x80000000) >> 16) |
      ((val & 0x00F80000) >> 19) | //Blue?
      ((val & 0x0000F800) >> 6) | //green?
      ((val & 0x000000F8) << 7)  //red?
      ;
}

u32 rgba16_to_rgba32c(u16 val)
{
   //16 : 0b                   ARRRRRGG GGGBBBBB
   //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return 
      ((val & 0x8000) ? 0xFF000000 : 0) |
      ((val & 0x001F) << 19) | //Blue?
      ((val & 0x03E0) << 6) | //green?
      ((val & 0x7C00) >> 7)  //red?
      ;
}

//Convert a whole palette from regular RGB (no alpha) to true 16 bit values
//used for in-memory palettes (and written drawing data)
void convert_palette(u32 * original, u16 * destination, u16 size)
{
   for(int i = 0; i < size; i++)
      destination[i] = rgba32c_to_rgba16(rgb24_to_rgba32c(original[i]));
}



// ----------------
// -- MENU UTILS --
// ----------------

//Menu items must be packed together, separated by \0. Last item needs two \0
//after. CONTROL WILL BE GIVEN FULLY TO THIS MENU UNTIL IT FINISHES!
s32 easy_menu(const char * title, const char * menu_items, u8 top, u8 display, u32 exit_buttons)
{
   s32 menu_on = 0;
   u32 menu_num = 0;
   u32 menu_ofs = 0;

   const char ** menu_str = malloc(MAX_MENU_ITEMS * sizeof(char *)); //[MAX_MENU_ITEMS];
   menu_str[0] = menu_items;
   bool has_title = (title != NULL && strlen(title));

   while(menu_num < MAX_MENU_ITEMS && *menu_str[menu_num] != 0)
   {
      menu_str[menu_num + 1] = menu_str[menu_num] + strlen(menu_str[menu_num]) + 1;
      menu_num++;
   }

   if(!display || display > menu_num) display = menu_num;

   //Print title, 1 over
   if(has_title)
   {
      printf("\x1b[%d;1H\x1b[0m %-49s", top, title);
      printf("%-50s","");
   }

   u8 menudispofs = (has_title ? 2 : 1);

   //I want to see how inefficient printf is, so I'm doing this awful on purpose 
   while(aptMainLoop())
   {
      hidScanInput();
      u32 kDown = hidKeysDown();
      u32 kRepeat = hidKeysDownRepeat();
      if(kDown & KEY_A) break;
      if(kDown & exit_buttons) { menu_on = -1; break; }
      if(kRepeat & KEY_UP) menu_on = (menu_on - 1 + menu_num) % menu_num;
      if(kRepeat & KEY_DOWN) menu_on = (menu_on + 1) % menu_num;

      if(menu_on < menu_ofs)
         menu_ofs = menu_on;
      if(menu_on >= menu_ofs + display)
         menu_ofs = menu_on - display + 1;

      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      //Print menu. When you get to the selected item, do a different bg
      printf("\x1b[0m\x1b[%d;3H%s", top + menudispofs - 1, menu_ofs > 0 ? "..." : "   ");
      printf("\x1b[%d;3H%s", top + menudispofs + display, (menu_num > menu_ofs + display) ? "..." : "   ");
      for(u8 i = 0; i < display; i++)
      {
         u8 menutop = top + menudispofs + i;
         u32 im = i + menu_ofs;
         if(menu_on == im)
            printf("\x1b[%d;1H\x1b[47;30m  %-48s", menutop, menu_str[im]);
         else
            printf("\x1b[%d;1H\x1b[0m  %-48s", menutop, menu_str[im]);
      }
      C3D_FrameEnd(0);
   }

   //Clear the menu area
   for(u8 i = 0; i < menudispofs + display + 1; i++)
      printf("\x1b[%d;1H\x1b[0m%-50s", top + i, "");

   return menu_on;
}

bool easy_confirm(const char * title, u8 top)
{
   return 1 == easy_menu(title, "No\0Yes\0", top, 0, KEY_B);
}

bool easy_warn(const char * warn, const char * title, u8 top)
{
   printf("\x1b[%d;1H\x1b[31;40m %-49s", top, warn);
   bool value = easy_confirm(title, top + 1);
   printf("\x1b[%d;1H%-50s",top, "");
   return value;
}

bool enter_text_fixed(const char * title, u8 top, char * container, u8 fixed_length,
      bool clear, u32 exit_buttons)
{
   bool confirmed = false;
   bool has_title = (title != NULL && strlen(title));
   char chars[ENTERTEXT_CHARARRSIZE];
   strcpy(chars, ENTERTEXT_CHAR);
   u8 av_ch = strlen(chars);
   if(clear) memset(container, chars[0], fixed_length);
   container[fixed_length] = '\0';

   u8 ud = top + (has_title ? 2 : 1);
   u8 pos = 0;

   //Print title, 1 over
   if(has_title)
      printf("\x1b[%d;1H\x1b[0m %-49s", top, title);

   printf("%-150s","");

   //I want to see how inefficient printf is, so I'm doing this awful on purpose 
   while(aptMainLoop())
   {
      hidScanInput();
      u32 kDown = hidKeysDown();
      u32 kRepeat = hidKeysDownRepeat();
      u8 current_index = (strchr(chars, container[pos]) - chars);
      if(kDown & KEY_A) { confirmed = true; break; }
      if(kDown & exit_buttons) break;
      if(kRepeat & KEY_RIGHT && pos < fixed_length - 1) pos++;
      if(kRepeat & KEY_LEFT && pos > 0) pos--;
      if(kRepeat & KEY_UP) container[pos] = chars[(current_index + 1) % av_ch];
      if(kRepeat & KEY_DOWN) container[pos] = chars[(current_index - 1 + av_ch) % av_ch];

      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      //Print menu. When you get to the selected item, do a different bg
      for(u8 i = 0; i < fixed_length; i++)
      {
         u8 lr = 3 + i;
         printf("\x1b[%d;%dH%c\x1b[%d;%dH%c\x1b[%d;%dH%c", 
               ud, lr, container[i], //The char data
               ud - 1, lr, i==pos ? '-' : ' ',  //The up/down
               ud + 1, lr, i==pos ? '-' : ' ');
      }
      C3D_FrameEnd(0);
   }

   //Clear the text area
   for(u8 i = 0; i < (has_title ? 2 : 1) + 2; i++)
      printf("\x1b[%d;1H\x1b[0m%-50s", top + i, "");

   return confirmed;
}

const char * get_menu_item(const char * menu_items, u32 length, u32 item)
{
   u32 strings = 0;

   for(u32 i = 0; i < length; i++)
   {
      //This will always be true on the NEXT character, which is after the \0
      //and is good, keep it first.
      if(strings == item)
         return menu_items + i;

      if(!menu_items[i])
         strings++;
   }

   return NULL;
}


// -- PRINT STUFF --
void printf_flush(const char * format, ...)
{
   C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
   C3D_FrameEnd(0);
}

//As expected, this causes a stack exception
//int mkdir_p(char* file_path, mode_t mode) {
//    if(!(file_path && *file_path)) return -2; //my special return, whatever
//    for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
//        *p = '\0';
//        if (mkdir(file_path, mode) == -1) {
//            if (errno != EEXIST) {
//                *p = '/';
//                return -1;
//            }
//        }
//        *p = '/';
//    }
//    return 0;
//}

//Taken verbatim from https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > PATH_MAX-1) {
        errno = ENAMETOOLONG;
        //LOGDBG("MKDIR_P: DIRECTORY TOO LONG: %ld, %s\n", PATH_MAX, path);
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                {
                    //LOGDBG("MKDIR_P: Couldn't make inner path: %s\n", _path);
                    return -1; 
                }
            }

            *p = '/';
        }
    }   

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
        {
           //LOGDBG("MKDIR_P: Couldn't make final path: %s\n", _path);
            return -1; 
        }
    }   

    //LOGDBG("Created directory: %s\n", path);

    return 0;
}

//Taken from https://stackoverflow.com/a/230070/1066474
bool file_exists (char * filename)
{
   struct stat buffer;
   return (stat (filename, &buffer) == 0);
}

//Get directories in menu format (separated by 0, last item has 2 0)
s32 get_directories(char * directory, char * container, u32 c_size)
{
   s32 count = 0;
   char * current_file = container;
   DIR * dir = opendir(directory);

   if(!dir) 
      return -1;

   struct dirent * entry = readdir(dir);

   while(entry != NULL)
   {
      if(entry->d_type == DT_DIR)
      {
         u32 len = strlen(entry->d_name);

         //No more files will fit
         if(current_file - container + len + 2 > c_size)
            break;

         //Copy entry into just past the last slot (where the 0 is)
         memcpy(current_file, entry->d_name, len);
         current_file += len;
         *current_file = 0;
         current_file++;

         count++;
      }

      entry = readdir(dir);
   }

   *current_file = 0;

   return count;
}
