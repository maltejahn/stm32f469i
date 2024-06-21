#ifndef _GRAPHICS_H
#define _GRAPHICS_H


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <gfx.h>





//--------------------------------------------------------------
// Struktur von einem Image
//--------------------------------------------------------------
typedef struct picture_t
{
  const uint16_t *table; // Tabelle mit den Daten
  uint16_t width;        // Breite des Bildes (in Pixel)
  uint16_t height;       // Hoehe des Bildes  (in Pixel)
  uint8_t colorspace;
}picture;


#define NO_ROTATE		0
#define ROTATE_CW		1
#define ROTATE_CCW		2
#define FLIP_HORIZONTAL 3
#define FLIP_VERTICAL	4

#define RGB332 1
#define RGB565 2
#define RGB888 3

#define DISP_HEIGHT 100
#define DISP_WIDTH (DISP_HEIGHT / 2)


#define BG_COLOR_RED 0xFFF000
#define BG_COLOR_GREEN 0x00FF00
#define BG_COLOR_BLUE 0x0000FF

#define TRANSPARENT 1
#define NOT_TRANSPARENT 0

void CopyImg_RGB565(GFX_CTX *g,picture *img, uint16_t x, uint16_t y, uint8_t rotate,uint32_t bg_color,uint8_t transparency);
void CopyImg_RGB332(GFX_CTX *g,picture *img, uint16_t x, uint16_t y);
struct rgb_color convert_colorspace(uint32_t c, uint8_t colorspace);
void draw_image_pixel(GFX_CTX *g, uint16_t x, uint16_t y,struct  rgb_color c);

struct rgb_color{
		uint8_t	* r;
		uint8_t	* g;
		uint8_t	* b;

	} ;




#endif  /*GRAPHICS.H */