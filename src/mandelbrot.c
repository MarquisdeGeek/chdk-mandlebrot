#include "camera_info.h"
#include "stdlib.h"
#include "keyboard.h"
#include "lang.h"
#include "conf.h"
#include "gui.h"
#include "gui_draw.h"
#include "gui_lang.h"
#include "gui_batt.h"
#include "gui_mbox.h"
#include "modes.h"

#include "module_def.h"

/*
Copyright 2014 Steven Goodwin

Released under the GNU GPL, version 2

Version 1.1 - 30th September 2014
  + Switch to doubles throughout
  + Use better starting co-ords that give square pixels
  + Allow reset to work, if drawing is paused
  + Invert the cursor square, instead of drawing it black

Version 1.0 - 28th September 2014

*/
void gui_game_menu_kbd_process();
int  gui_mandelbrot_keyboard();
void gui_mandelbrot_draw();

gui_handler GUI_MODE_MANDLEBROT = 
    /*GUI_MODE_MANDLEBROT*/  { GUI_MODE_MODULE, gui_mandelbrot_draw, gui_mandelbrot_keyboard, gui_game_menu_kbd_process, GUI_MODE_FLAG_NODRAWRESTORE };


static int mandle_palette[7];

#define MODE_RENDER    0
#define MODE_SELECT    1

struct {
  int     mode;
  double  xstart, xend;       // Area to render, in imaginary plane
  double  ystart, yend;
  //
  double  xstep, ystep;       // Incrementa in imaginary plane
  //
  int     max_iterations;
  int     xscreen, yscreen;   // relative position of next block to render
  int     speed;                // How many blocks are calculated on each draw frame. Lower=more responsivefor pause button
  int     paused;
  //
  int     cursor_x, cursor_y;   // the position during select mode. in mandelbrot-related co-ords
  int     cursor_w, cursor_h;   // size of the cursor, in screen co-ords. should remain const
  int     render_x, render_y;   // top left of the area in which the image will be drawn
  int     render_w, render_h;
  // 
} gState;

static void resetInterface() {
  gState.render_x = gState.render_y = 0;
  gState.render_w = camera_screen.width;
  gState.render_h = camera_screen.height;

  gState.cursor_x = gState.cursor_y = 0;
  
  // Ensure the cursor is the same ratio as the screen. This maintains 1:1 pixels
  gState.cursor_h = 20;
  gState.cursor_w = (gState.cursor_h * gState.render_w) / gState.render_h;

  gState.paused = 0;
}

static void recompute() {
  gState.xscreen = gState.yscreen = 0;
  gState.xstep = (gState.xend-gState.xstart)/gState.render_w;
  gState.ystep = (gState.yend-gState.ystart)/gState.render_h;
}

static void resetParameters() {
  gState.mode = MODE_RENDER;
  // These co-ords (in imaginary plan) gives us an aspect ratio of 1:1
  gState.xstart = -2.3;
  gState.ystart = -1.8;
  gState.xend = 1.3;
  gState.yend = 1.8;

  // We then rescale the view to match the screen so it's also 1:1
  gState.ystart = (gState.ystart * gState.render_h / gState.render_w);
  gState.yend = -gState.ystart;

  gState.render_w = camera_screen.width;
  gState.render_h = camera_screen.height;
  //
  gState.max_iterations = 32;
  //
  gState.speed = 4;
  //
  recompute();
  //
  gState.mode = MODE_RENDER;
  gState.paused = 0;
}

static void zoomInHere() {
  double x = gState.xstart + gState.cursor_x * gState.xstep;
  double y = gState.ystart + gState.cursor_y * gState.ystep;
  double w = gState.cursor_w * gState.xstep;
  double h = gState.cursor_h * gState.ystep;

  gState.xstart = x;
  gState.ystart = y;

  gState.xend = x + w;
  gState.yend = y + h;

  // Increasing the max iterations maintains detail as we zoom, since we
  // generally zoom around the edges of the set, which have detail
  gState.max_iterations = (gState.max_iterations * 10) / 8;
  //
  recompute();
  //
  gState.mode = MODE_RENDER;
  gState.paused = 0;
}

static void draw_mandel_pixel(int sx, int sy, int invert) {
  double x,y;
  double z,zi,newz,newzi;
  int inset, iterations;
  int col;

  x = gState.xstart;
  y = gState.ystart;

  x += sx * gState.xstep;
  y += sy * gState.ystep;

  z = 0;
  zi = 0;
  inset = 1;
  for (iterations=0; iterations<gState.max_iterations; iterations++) {
    /* z^2 = (a+bi)(a+bi) = a^2 + 2abi - b^2 */
    newz = (z*z)-(zi*zi) + x;
    newzi = 2*z*zi + y;
    z = newz;
    zi = newzi;
    if(((z*z)+(zi*zi)) > 4) {
      inset = 0;
      break;
    }
  }
  //
  if (inset) {
    draw_pixel( gState.render_x+sx, gState.render_y+sy, invert?COLOR_WHITE:COLOR_BLACK );
  } else {
    col = mandle_palette[iterations % sizeof(mandle_palette)/sizeof(mandle_palette[0])];
    draw_pixel( gState.render_x+sx, gState.render_y+sy, invert?~col:col);
  }

}
 
static void draw_mandel_area(int x, int y, int invert) {
  int i, j;

  for(j=0;j<gState.cursor_h;++j) {
    for(i=0;i<gState.cursor_w;++i) {
      draw_mandel_pixel(x+i, y+j, invert);
    }
  }
}


static void moveCursor(int newX, int newY) {
  draw_mandel_area(gState.cursor_x, gState.cursor_y, 0);

  if (newX >= 0 && newX < gState.render_w) {
    gState.cursor_x = newX;
  }

  if (newY >= 0 && newY < gState.render_h) {
    gState.cursor_y = newY;
  }
  
  draw_mandel_area(gState.cursor_x, gState.cursor_y, 1);
}

int gui_mandelbrot_keyboard() {
   if (gState.mode == MODE_SELECT) {
     switch (kbd_get_autoclicked_key()) {
          case KEY_UP:
              moveCursor(gState.cursor_x, gState.cursor_y-gState.cursor_h);
              break;
          case KEY_DOWN:
              moveCursor(gState.cursor_x, gState.cursor_y+gState.cursor_h);
              break;
          case KEY_LEFT:
              moveCursor(gState.cursor_x-gState.cursor_w, gState.cursor_y);
              break;
          case KEY_RIGHT:
              moveCursor(gState.cursor_x+gState.cursor_w, gState.cursor_y);
              break;
          case KEY_SET:
              zoomInHere();
              break;
          case KEY_ERASE:
          case KEY_DISPLAY:
              resetParameters();
              break;
      }
   } else {  // RENDER
      switch (kbd_get_autoclicked_key()) {
	case	KEY_SET:      
	  gState.paused = !gState.paused;
	  break;
	case KEY_DISPLAY:
	  if (gState.paused) {
	    resetParameters();
	  }
	  break;
	}
    }

    return 0;
}

void gui_mandelbrot_draw() {
int i;


  if (gState.mode == MODE_RENDER && !gState.paused) {

    for(i=0;i<gState.speed;++i) {

      draw_mandel_area(gState.xscreen, gState.yscreen, 0);

      gState.xscreen += gState.cursor_w;
      if (gState.xscreen >= gState.render_w) {
        gState.xscreen = 0;
        gState.yscreen += gState.cursor_h;
        if (gState.yscreen >= gState.render_h) {
          gState.mode = MODE_SELECT;
          resetInterface();
          moveCursor(0,0);
          break;
        }
      }

    }

  }
}

int gui_mandelbrot_init() {

  mandle_palette[0] = COLOR_RED;
  mandle_palette[1] = COLOR_GREEN;
  mandle_palette[2] = COLOR_BLUE;
  mandle_palette[3] = COLOR_YELLOW;
  mandle_palette[4] = COLOR_WHITE;
  mandle_palette[5] = COLOR_GREY;
  mandle_palette[6] = COLOR_BLUE_LT; 

  resetInterface();
  resetParameters();

  draw_filled_rect(0,0,camera_screen.width,camera_screen.height, COLOR_YELLOW);

  gui_set_mode(&GUI_MODE_MANDLEBROT);
  
  return 1;
}

int basic_module_init() {
  return gui_mandelbrot_init();
}

#include "simple_game.c"

/******************** Module Information structure ******************/

struct ModuleInfo _module_info =
{
    MODULEINFO_V1_MAGICNUM,
    sizeof(struct ModuleInfo),
    SIMPLE_MODULE_VERSION,		// Module version

    ANY_CHDK_BRANCH, 0,			// Requirements of CHDK version
    ANY_PLATFORM_ALLOWED,		// Specify platform dependency

    (int32_t)"Mandelbrot",			// Module name
    (int32_t)"Game",

    &_librun.base,

    ANY_VERSION,                // CONF version
    CAM_SCREEN_VERSION,         // CAM SCREEN version
    ANY_VERSION,                // CAM SENSOR version
    ANY_VERSION,                // CAM INFO version
};

/*************** END OF AUXILARY PART *******************/
