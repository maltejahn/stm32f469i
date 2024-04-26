/*
 * DMA2D Demo
 *
 * Copyright (C) 2016 Chuck McManis, All rights reserved.
 *
 * This demo shows how the Bit Block Transfer (BitBlT)
 * peripheral known as DMA2D can be used to speed up
 * graphical sorts of things.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma2d.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <gfx.h>
#include <malloc.h>

#include "main.h"
#include "include/graphics.h"

#include "../../demos/util/util.h"
#include "../../demos/util/helpers.h"

// include picture files
#include "Emo2_Image.h"
#include "Emo1_VGA_Image.h"
#include "bild.h"
#include "sky.h"
extern char _ebss, _stack;

#define TOUCH_PRESSED 1
#define TOUCH_NOT_PRESSED 0


uint8_t invert;

uint32_t usart_table[6] = {
	USART1,
	USART2,
	USART3,
	UART4,
	UART5,
	USART6,
};


struct touch{
	uint16_t	x;
    uint16_t	y;


	} ;

	struct button button1;
	struct touch mytouch;

	
	struct button button1 = {0,400,360,480,&Emo2_Image,0};
	


void dma2d_digit(int x, int y, int d, uint32_t color, uint32_t outline);

void generate_background(void);
void init_ui(GFX_CTX *g);

void init_ui(GFX_CTX *g){
	
	//just flip the image to see if buttons are working - later i will be replaced by a icon where inverting colors indicate a status
	uint8_t rot=NO_ROTATE;
	if(button1.touch_status==0x01){
		rot=ROTATE_CCW;
	}
	CopyImg_RGB565(g,&Emo2_Image,button1.start_x,button1.start_y,rot,invert);
	//CopyImg_RGB565(g,&sky_Image,50,50,NO_ROTATE, 0);

}
uint8_t check_button(struct button thisbutton);
uint8_t check_button(struct button thisbutton){
	if(mytouch.x>= thisbutton.start_x && mytouch.x<= thisbutton.end_x && mytouch.y>= thisbutton.start_y && mytouch.y<= thisbutton.end_x )
	{
		return TOUCH_PRESSED;
	}
	else{
		return TOUCH_NOT_PRESSED;
	}

}








/*
 * relocate the heap to the DRAM, 10MB at 0xC0000000
 */
void
local_heap_setup(uint8_t **start, uint8_t **end)
{
	console_puts("Local heap setup\n");
	*start = (uint8_t *)(0xc0000000);
	*end = (uint8_t *)(0xc0000000 + (10 * 1024 * 1024));
}









/* another frame buffer 2MB "before" the standard one */
#define BACKGROUND_FB (FRAMEBUFFER_ADDRESS - 0x200000U)
#define MAX_OPTS	6

void dma2d_bgfill(void);
void dma2d_fill(uint32_t color);

/*
 * The DMA2D device can copy memory to memory, so in this case
 * we have another frame buffer with just the background in it.
 * And that is copied from there to the main display. It happens
 * faster than the tight loop fill but slightly slower than the
 * register to memory fill.
 */
void
dma2d_bgfill(void)
{
#ifdef MEMORY_BENCHMARK
	uint32_t t1, t0;
#endif

	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		DMA2D_IFCR |= 0x3F;
		if (DMA2D_ISR & DMA2D_ISR_CEIF) {
			printf("Failed to clear configuration error\n");
			while (1);
		}
	}

#ifdef MEMORY_BENCHMARK
	t0 = mtime();
#endif
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_M2M);
	/* no change in alpha, same color mode, no CLUT */
	DMA2D_FGPFCCR = 0x0;
	DMA2D_FGMAR = (uint32_t) BACKGROUND_FB;
	DMA2D_FGOR = 0; /* full screen */
	DMA2D_OOR =	0;
	DMA2D_NLR = DMA2D_SET(NLR, PL, 800) | 480; /* 480 lines */
	DMA2D_OMAR = (uint32_t) FRAMEBUFFER_ADDRESS;

	/* kick it off */
	DMA2D_CR |= DMA2D_CR_START;
	while ((DMA2D_CR & DMA2D_CR_START));
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("Configuration error!\n");
		while (1);
	}
#ifdef MEMORY_BENCHMARK
	t1 = mtime();
	printf("Transfer rate (M2M) %6.2f MB/sec\n", 1464.84 / (float) (t1 - t0));
#endif
}

/*
 * Use the DMA2D peripheral in register to memory
 * mode to clear the frame buffer to a single
 * color.
 */
void
dma2d_fill(uint32_t color)
{
#ifdef MEMORY_BENCHMARK
	uint32_t t1, t0;

	t0 = mtime();
#endif
	DMA2D_IFCR |= 0x3F;
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_R2M);
	DMA2D_OPFCCR = 0x0; /* ARGB8888 pixels */
	/* force it to have full alpha */
	DMA2D_OCOLR = 0xff000000 | color;
	DMA2D_OOR =	0;
	DMA2D_NLR = DMA2D_SET(NLR, PL, 800) | 480; /* 480 lines */
	DMA2D_OMAR = (uint32_t) FRAMEBUFFER_ADDRESS;

	/* kick it off */
	DMA2D_CR |= DMA2D_CR_START;
	while (DMA2D_CR & DMA2D_CR_START);
#ifdef MEMORY_BENCHMARK
	t1 = mtime();
	printf("Transfer rate (R2M) %6.2f MB/sec\n", 1464.84 / (float) (t1 - t0));
#endif
}

/*
 * This set of utility functions are used once to
 * render our "background" into memory. Later we
 * will use the dma2d_bgfill to copy it into the
 * main display to "reset" the display to its non-drawn on
 * state.
 *
 * The background is a set up to look like quadrille
 * graph paper with darker lines every 5 lines.
 */
#define GRID_BG		COLOR(0xff, 0xff, 0xff)	/* white */
#define LIGHT_GRID	COLOR(0xc0, 0xc0, 0xff)	/* light blue */
#define DARK_GRID	COLOR(0xc0, 0xc0, 0xe0)	/* dark blue */

void bg_draw_pixel(void *, int, int, GFX_COLOR);

/* add pointer to frame buffer, and change to GFX_COLOR with
 * .raw being the holder for the color.
 */
/* Our own pixel writer for the background stuff */
void
bg_draw_pixel(void *fb, int x, int y, GFX_COLOR color)
{
	*((uint32_t *) fb + (y * 800) + x) = color.raw;
}

// static background image - frames for button, frames for info buttons
void
generate_background(void)
{
	int i, x, y;
	uint32_t *t;
	GFX_CTX local_ctx;
	GFX_CTX	*g;

	g = gfx_init(&local_ctx, bg_draw_pixel, 800, 480, GFX_FONT_LARGE, (void *) BACKGROUND_FB);
	for (i = 0, t = (uint32_t *)(BACKGROUND_FB); i < 800 * 480; i++, t++) {
		*t = 0xffffffff; // clear to white 
	}
	// draw a grid 
/*	t = (uint32_t *) BACKGROUND_FB;
	for (y = 1; y < 480; y++) {
		for (x = 1; x < 800; x++) {
			// major grid line 
			if ((x % 50) == 0) {
				gfx_draw_point_at(g, x-1, y, DARK_GRID);
				gfx_draw_point_at(g, x, y, DARK_GRID);
				gfx_draw_point_at(g, x+1, y, DARK_GRID);
			} else if ((y % 50) == 0) {
				gfx_draw_point_at(g, x, y-1, DARK_GRID);
				gfx_draw_point_at(g, x, y, DARK_GRID);
				gfx_draw_point_at(g, x, y+1, DARK_GRID);
			} else if (((x % 25) == 0) || ((y % 25) == 0)) {
				gfx_draw_point_at(g, x, y, LIGHT_GRID);
			}
		}
	}
	gfx_draw_rounded_rectangle_at(g, 0, 0, 800, 480, 15, DARK_GRID);
	gfx_draw_rounded_rectangle_at(g, 1, 1, 798, 478, 15, DARK_GRID);
	gfx_draw_rounded_rectangle_at(g, 2, 2, 796, 476, 15, DARK_GRID);
	*/

	gfx_draw_rounded_rectangle_at(g, 0, 350, 800, 130, 15, GFX_COLOR_RED);
	gfx_draw_rounded_rectangle_at(g, 1, 351, 798, 128, 15, GFX_COLOR_RED);
	gfx_draw_rounded_rectangle_at(g, 2, 352, 796, 126, 15, GFX_COLOR_RED);
	gfx_draw_rounded_rectangle_at(g, 3, 353, 794, 124, 15, GFX_COLOR_RED);
	gfx_draw_rounded_rectangle_at(g, 4, 354, 792, 122, 15, GFX_COLOR_RED);
}









#define N_FRAMES	10
uint32_t frame_times[N_FRAMES] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int te_lock = 1;

static void
draw_pixel(void *buf, int x, int y, GFX_COLOR c)
{
	lcd_draw_pixel(buf, x, y, c.raw);
}

static void usart_config(uint32_t usart)
{
	/* Setup USART parameters. */
	usart_set_baudrate(usart, 38400);
	usart_set_databits(usart, 8);
	usart_set_stopbits(usart, USART_STOPBITS_1);
	usart_set_mode(usart, USART_MODE_TX_RX);
	usart_set_parity(usart, USART_PARITY_NONE);
	usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);
}


// USART6_TX -> PC6, Alternate Function 8
// USART6_RX -> PC7, Alternate Function 8
static void usart_setup(void)
	{

	//nvic_enable_irq(NVIC_USART6_IRQ);


	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6); /* USART6 */

	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7); /* USART6 */


	gpio_set_af(GPIOC, GPIO_AF8, GPIO6 | GPIO7); /* USART6 */


	usart_config(USART6);


	//usart_enable_rx_interrupt(USART6);


	usart_enable(USART6);
	
	}


static void clock_setup_rcc(void)
{
	rcc_periph_clock_enable(RCC_DMA2D);
	rcc_periph_clock_enable(RCC_USART6);
	rcc_periph_clock_enable(RCC_GPIOG);
}

static void gpio_led_setup(void)
{
	gpio_mode_setup(GPIOG,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,GPIO6);
}





/*
 * This is the code for the simple DMA2D Demo
 *
 * Basically it lets you put display digits on the
 * screen manually or with the DMA2D peripheral. It
 * has various 'bling' levels, and it tracks performance
 * by measuring how long it takes to go from one frame
 * to the next.
 */
int
main(void) {
	int	i;
	uint32_t t1, t0;
	char buf[35];
	char mybuf[35];
	char *scr_opt;
	int f_ndx = 0;
	float avg_frame;
	//int	can_switch;
	int	opt, ds;
	uint16_t touch_x, touch_y;
	touch_event *te;
	

	GFX_CTX local_context;
	GFX_CTX *g;
	//clock_setup(168000000,0);
	clock_setup_rcc();
	console_setup(57600);
	usart_setup();
	gpio_led_setup();
	
	/* Enable the clock to the DMA2D device */


	
	fprintf(stderr, "DMA2D Demo program : Digits Gone Wild\n");

	printf("Generate background\n");
	generate_background();
	printf("Generate digits\n");
	
	g = gfx_init(&local_context, draw_pixel, 800, 480, GFX_FONT_LARGE, 
						(void *)FRAMEBUFFER_ADDRESS);

	
	//opt = 4; /* screen clearing mode */
	//can_switch = -1; /* auto switching every 10 seconds */
	t0 = mtime();
	ds = 0;
	

	
	
	while (1) {
		//usart_send(USART6, 0x33);
		//gpio_toggle(GPIOG, GPIO6);
			
		//usart_send_blocking(USART6, 0x32);

		//usart_send(5,0x34);
		//console_puts("bla");


dma2d_bgfill();
init_ui(g);

		te=get_touch(0);
		if(te != NULL){
			mytouch.x=te->tp[0].x;
			mytouch.y=te->tp[0].y;
			/*
			if(mytouch.x>50 && mytouch.x<200 && mytouch.y >400 && mytouch.y<480) {
				invert=1;

			}
			else{
				invert=0;
			}*/

			if(check_button(button1)== TOUCH_PRESSED){
				invert=1;
				button1.touch_status=TOUCH_PRESSED;
				
			}
			else{
				invert=0;
				button1.touch_status=TOUCH_NOT_PRESSED;
				
			}
		}



		/* In both cases we write the notes using the graphics library */
		gfx_set_text_color(g, GFX_COLOR_BLACK, GFX_COLOR_BLACK);
		gfx_set_text_size(g, 3);
		gfx_set_text_cursor(g, 25, 55 + DISP_HEIGHT + ((gfx_get_text_height(g) * 3) + 2));
		gfx_puts(g, (char *)"Hello world Supa Display DMA2D!");


		//CopyImg_RGB565(g,&Emo2_Image,300,250,ROTATE_CCW);
		//CopyImg_RGB565(g,&Emo2_Image,200,250,ROTATE_CW);
		//CopyImg_RGB565(g,&Emo2_Image,50,250,NO_ROTATE);
		//CopyImg_RGB565(g,&bild_Image,350,10,ROTATE_CW);
		//CopyImg_RGB332(g,&Emo1_VGA_Image,300,50);
		//CopyImg_RGB565(g,&sky_Image,50,50,NO_ROTATE, invert);
		t1 = mtime();

		/* this computes a running average of the last 10 frames */
		/* XXX cleanup text BUG: Text height doesn't reflect magnify */
		frame_times[f_ndx] = t1 - t0;
		f_ndx = (f_ndx + 1) % N_FRAMES;
		for (i = 0, avg_frame = 0; i < N_FRAMES; i++) {
			avg_frame += frame_times[i];
		}
		avg_frame = avg_frame / (float) N_FRAMES;
		snprintf(buf, 35, "FPS: %6.2f", 1000.0 / avg_frame);
				gfx_set_text_cursor(g, 25, 55 + DISP_HEIGHT + 2 * ((gfx_get_text_height(g) * 3) + 2));
		gfx_puts(g, (char *)buf);

		gfx_set_text_cursor(g, 25, 55 + DISP_HEIGHT + 3 * ((gfx_get_text_height(g) * 3) + 2));
		gfx_puts(g, "TEST: ");
		snprintf(buf, 35, "x %3d", mytouch.x);
		gfx_puts(g, (char *)buf);
		snprintf(buf, 35, "y %3d", mytouch.y);
		gfx_puts(g, (char *)buf);


		lcd_flip(te_lock);
		if (opt == 2) {
			/* XXX doesn't display clock data if we don't pause here */
			msleep(100);
		}
		/*
		 * The demo runs continuously but it watches for characters
		 * typed at the console. There are a few options you can select.
		 */
		

		t0 = t1;
	}
}
