#include "graphics.h"


const uint8_t b3to8lookup[8] = { 0, 0x3e, 0x5d, 0x7c, 0x9b, 0xba, 0xd9, 0xFF };
const uint8_t b2to8lookup[4] = { 0, 0x7e, 0xBD, 0xFF };

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



		colors.r=(c & 0xe0)>>5;
		colors.r=(uint8_t)b3to8lookup[(uint8_t)colors.r];
		colors.g=(((c & 0x1C)>>2));
		colors.g=(uint8_t)b3to8lookup[(uint8_t)colors.g];
		colors.b=(((c &0x03)));
		colors.b=(uint8_t)b2to8lookup[(uint8_t)colors.b];



	}
	else if(colorspace==RGB888){
		colors.r=((c >> 16)&0xff);
		colors.g=((c >>8 )&0xff);
		colors.b=((c))&0xff;
	}

	return colors;

}
void draw_image_pixel(GFX_CTX *g, uint16_t x, uint16_t y,struct  rgb_color c){
	gfx_draw_point_at(g, x, y, (GFX_COLOR){.c = {c.b,c.g,c.r,0xFF}});
}


void CopyImg_RGB565(GFX_CTX *g,picture *img, uint16_t x, uint16_t y, uint8_t rotate,uint32_t bg_color,uint8_t transparency){


  	uint32_t pixel_color;
  	uint8_t red, green, blue;
	// get 16 Bit color values
	const uint16_t *value;
  	value=&img->table[0];
  	static struct rgb_color conv_colors,bg_c;
  

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
				bg_c=convert_colorspace(bg_color,RGB888);

				

				if(pixel_color!=0xffff ) draw_image_pixel(g, x+xn, y+yn,conv_colors);
				//if(pixel_color!=0xffff )gfx_draw_point_at(g, x+xn, y+yn, (GFX_COLOR){.c = {conv_colors.b,conv_colors.g,conv_colors.r,0xFF}});
				//if(pixel_color!=0xFFFF )gfx_draw_point_at(g, x+xn, y+yn, COLOR(conv_colors.r,conv_colors.g,conv_colors.b));

				// probably here will come som kind of invert (pressed button)
				
				
				if(pixel_color==0xffff && bg_color!=0 && transparency == NOT_TRANSPARENT) draw_image_pixel(g, x+xn, y+yn,bg_c);
				//if(pixel_color==0xffff && bg_color!=0)gfx_draw_point_at(g, x+xn, y+yn, (GFX_COLOR){.c = {bg_c.b,bg_c.g,bg_c.r,0xFF}});
				

			}
		}
	}

// similar, flipped x/y
if(rotate==ROTATE_CCW){
  for(xn=0; xn <= img->height;xn++){
	for(yn=0; yn <= img->width;yn++){
		pixel_color=value[xn*img->width+yn];
		conv_colors=convert_colorspace(pixel_color,RGB565);
		bg_c=convert_colorspace(bg_color,RGB888);
		//gfx_draw_point_at(g, x+xn, y+yn, COLOR(conv_colors.r,conv_colors.g,conv_colors.b));
		if(pixel_color!=0xffff ) draw_image_pixel(g, x+xn, y+yn,conv_colors);
		if(pixel_color==0xffff && bg_color!=0 && transparency == NOT_TRANSPARENT) draw_image_pixel(g, x+xn, y+yn,bg_c);

	}
  }
}
// also similar, rotate CW
if(rotate==ROTATE_CW){
  for(xn=0; xn <= img->height;xn++){
	for(yn=0; yn <= img->width;yn++){
		pixel_color=value[xn*img->width+yn];
		conv_colors=convert_colorspace(pixel_color,RGB565);
		bg_c=convert_colorspace(bg_color,RGB888);
		//gfx_draw_point_at(g, x+xn, y-yn, COLOR(conv_colors.r,conv_colors.g,conv_colors.b));
		if(pixel_color!=0xffff ) draw_image_pixel(g, x+xn, y-yn,conv_colors);
		if(pixel_color==0xffff && bg_color!=0 && transparency == NOT_TRANSPARENT) draw_image_pixel(g, x+xn, y-yn,bg_c);

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