#include "graphics.h"


// depending on the colorspace which is used by the image, the colors get converted to fit to a 8x8x8 RGB Display
struct rgb_color convert_colorspace(uint32_t c, uint8_t colorspace){
	static struct rgb_color colors;
	
	// dirty, no
	if(colorspace == RGB565){
		colors.r=((c )>>11)<<3;
		colors.g=((c & 0x7E0)>>5)<<2;
		colors.b=((c &0x1F))<<3;
	}
	else if (colorspace ==RGB332){
		colors.r=((c )>>5)<<5;
		colors.g=((c & 0x1C)>>2)<<5;
		colors.b=((c &0x03))<<6;
	}
	else if(colorspace==RGB888){

	}

	return colors;

}


void CopyImg_RGB565(GFX_CTX *g,picture *img, uint16_t x, uint16_t y, uint8_t rotate,uint8_t invert_color){


  	uint32_t pixel_color;
  	uint8_t red, green, blue;
	// get 16 Bit color values
	const uint16_t *value;
  	value=&img->table[0];
  	static struct rgb_color conv_colors;
  

  	uint32_t xn, yn;

	if(rotate==NO_ROTATE){
		// for each row
		for(yn=0; yn <= img->height;yn++){
			// increment through x values
			for(xn=0; xn <= img->width;xn++){
				// get the color information from the location needed
				pixel_color=value[yn*img->width+xn];
				// convert the colors ti RGB888
				conv_colors=convert_colorspace(pixel_color,RGB565);
				// if the color isnt pure white (0x00), print the pixel. When its pure white, pixel is skipped
				if(pixel_color!=0x00 )gfx_draw_point_at(g, x+xn, y+yn, (GFX_COLOR){.c = {conv_colors.b,conv_colors.g,conv_colors.r,0xFF}});
			
				// probably here will come som kind of invert (pressed button)
				if(pixel_color==0x00 && invert_color==1)gfx_draw_point_at(g, x+xn, y+yn, (GFX_COLOR){.c = {0,0,0xFF,0xFF}});
				

			}
		}
	}

// similar, flipped x/y
if(rotate==ROTATE_CCW){
  for(xn=0; xn <= img->height;xn++){
	for(yn=0; yn <= img->width;yn++){
		pixel_color=value[xn*img->width+yn];
		conv_colors=convert_colorspace(pixel_color,RGB565);
		gfx_draw_point_at(g, x+xn, y+yn, COLOR(conv_colors.r,conv_colors.g,conv_colors.b));
	}
  }
}
// also similar, rotate CW
if(rotate==ROTATE_CW){
  for(xn=0; xn <= img->height;xn++){
	for(yn=0; yn <= img->width;yn++){
		pixel_color=value[xn*img->width+yn];
		conv_colors=convert_colorspace(pixel_color,RGB565);
		gfx_draw_point_at(g, x+xn, y-yn, COLOR(conv_colors.r,conv_colors.g,conv_colors.b));

	}
  }
}



}

void CopyImg_RGB332(GFX_CTX *g,picture *img, uint16_t x, uint16_t y){


  uint32_t pixel_color;
  uint8_t red, green, blue;
  // get 8 Bit color values
  const uint8_t *value;
  value=&img->table[0];
  uint32_t xn, yn;
  static struct rgb_color conv_colors;


    for(yn=0; yn <= img->height;yn++){
	for(xn=0; xn <= img->width;xn++){
		pixel_color=value[yn*img->width+xn];
		conv_colors=convert_colorspace(pixel_color,RGB332);
		gfx_draw_point_at(g, x+xn, y+yn, COLOR(conv_colors.r,conv_colors.g,conv_colors.b));

	}
  }


}