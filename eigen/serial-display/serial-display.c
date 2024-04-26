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
extern char _ebss, _stack;


uint32_t usart_table[6] = {
	USART1,
	USART2,
	USART3,
	UART4,
	UART5,
	USART6,
};


#include "../../demos/util/util.h"
#include "../../demos/util/helpers.h"

void draw_digit(GFX_CTX *g, int x, int y, int d, GFX_COLOR color, GFX_COLOR outline);
void draw_colon(GFX_CTX *g, int x, int y, GFX_COLOR color, GFX_COLOR outline);
void draw_dp(GFX_CTX *g, int x, int y, GFX_COLOR color, GFX_COLOR outline);
void dma2d_digit(int x, int y, int d, uint32_t color, uint32_t outline);
void display_clock(GFX_CTX *g, int x, int y, uint32_t tm);
void dma2d_clock(int x, int y, uint32_t tm, int ds);
void generate_background(void);
void generate_digits(void);

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

/*
 *   This defines a parameterized 7 segment display graphic
 *   overall graphic is DISP_WIDTH pixels wide by DISP_HEIGHT
 *   pixels tall.
 *
 *   Segments are SEG_THICK pixels "thick".
 *
 *   Gaps between segments are SEG_GAP pixel(s)
 */

#define DISP_HEIGHT 100
#define DISP_WIDTH (DISP_HEIGHT / 2)

void _close_r(void);





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

/*
 * This generates a "pleasant" background which looks a bit
 * like graph paper.
 */
void
generate_background(void)
{
	int i, x, y;
	uint32_t *t;
	GFX_CTX local_ctx;
	GFX_CTX	*g;

	g = gfx_init(&local_ctx, bg_draw_pixel, 800, 480, GFX_FONT_LARGE, (void *) BACKGROUND_FB);
	for (i = 0, t = (uint32_t *)(BACKGROUND_FB); i < 800 * 480; i++, t++) {
		*t = 0xffffffff; /* clear to white */
	}
	/* draw a grid */
	t = (uint32_t *) BACKGROUND_FB;
	for (y = 1; y < 480; y++) {
		for (x = 1; x < 800; x++) {
			/* major grid line */
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
}

/* Digits zero through 9, : and . */
struct digit_fb {
	int	w, h;	/* width and height */
	uint8_t *data;
} digits[12];

/*
 * This should be a data buffer which can hold 10 digits,
 * a colon, and a decimal point, note that pixels in this
 * buffer are one byte each.
 */
void digit_draw_pixel(void *, int x, int y, GFX_COLOR color);
void print_digit(int n);


/* This is the simple graphics helper function to draw into it */
void
digit_draw_pixel(void *fb, int x, int y, GFX_COLOR color)
{
	struct digit_fb *digit = (struct digit_fb *)(fb);
	uint8_t	c = (uint8_t) color.raw;
	*(digit->data + (y * digit->w) + x) = c;
}





/*
 * This then uses the DMA2D peripheral to copy a digit from the
 * pre-rendered digit buffer, and render it into the main display
 * area. It does what many people call a "cookie cutter" blit, which
 * is that the pixels that have color (are non-zero) are rendered
 * but when the digit buffer has a value of '0' the background is
 * rendered.
 *
 * The digit colors are stored in the Foreground color lookup
 * table, they are as passed in, except color 0 (background)
 * is always 0.
 *
 * DMA2D is set up to read the original image (which has alpha of
 * 0xff (opaque) and then it multiples that alpha and the color
 * alpha, and 0xFF renders the digit color opaquely, 0x00 renders
 * the existing color. When drawing drop shadows we use an alpha
 * of 0x80 which makes the drop shadows 50% transparent.
 */
#ifndef DMA2D_FG_CLUT
#define DMA2D_FG_CLUT	(uint32_t *)(DMA2D_BASE + 0x0400UL)
#endif
#ifndef DMA2D_BG_CLUT
#define DMA2D_BG_CLUT	(uint32_t *)(DMA2D_BASE + 0x0800UL)
#endif

void
dma2d_digit(int x, int y, int d, uint32_t color, uint32_t outline)
{
	uint32_t t;
	struct digit_fb *digit;

	while (DMA2D_CR & DMA2D_CR_START);
	/* This is going to be a memory to memory with PFC transfer */
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_M2MWB);

	digit = &digits[d];

	*(DMA2D_FG_CLUT) = 0x0; /* transparent black */
	*(DMA2D_FG_CLUT+1) = color; /* foreground */
	*(DMA2D_FG_CLUT+2) = outline; /* outline color */

	/* compute target address */
	t = (uint32_t) FRAMEBUFFER_ADDRESS + (800 * 4 * y) + x * 4;
	/* Output goes to the main frame buffer */
	DMA2D_OMAR = t;
	/* Its also the pixels we want to read incase the digit is
	 * transparent at that point
	 */
	DMA2D_BGMAR = t;
	DMA2D_BGPFCCR = DMA2D_SET(xPFCCR, CM, DMA2D_xPFCCR_CM_ARGB8888) |
					DMA2D_SET(xPFCCR, AM, 0);

	/* output frame buffer is ARGB8888 */
	DMA2D_OPFCCR = 0; /* DMA2D_OPFCCR_CM_ARGB8888; */

	/*
	 * This sets the size of the "box" we're going to copy. For the
	 * digits it is DISP_WIDTH + SKEW_MAX pixels wide for the ':' and
	 * '.' character is will be SEG_THICK + SKEW_MAX wide, it is always
	 * DISP_HEIGHT tall.
	 */

	/* So this then describes the box size */
	DMA2D_NLR = DMA2D_SET(NLR, PL, digit->w) | DISP_HEIGHT;
	/*
	 * This is how many additional pixels we need to move to get to
	 * the next line of output.
	 */
	DMA2D_OOR = 800 - digit->w;
	/*
	 * This is how many additional pixels we need to move to get to
	 * the next line of background (which happens to be the output
	 * so it is the same).
	 */
	DMA2D_BGOR = 800 - digit->w;
	/*
	 * And finally this is the additional pixels we need to move
	 * to get to the next line of the pre-rendered digit buffer.
	 */
	DMA2D_FGOR = 0;

	/*
	 * And this points to the top left corner of the prerendered
	 * digit buffer, where the digit (or character) top left
	 * corner is.
	 */
	DMA2D_FGMAR = (uint32_t) (digit->data);

	/* Set up the foreground data descriptor
	 *    - We are only using 3 of the colors in the lookup table (CLUT)
	 *	  - We don't set alpha since it is in the lookup table
	 *	  - Alpha mode is 'don't modify' (00)
	 *	  - CLUT color mode is ARGB8888 (0)
	 *	  - Color Mode is L8 (0101) or one 8 byte per pixel
	 *
	 */
	DMA2D_FGPFCCR = DMA2D_SET(xPFCCR, CS, 255) |
					DMA2D_SET(xPFCCR, ALPHA, 255) |
					DMA2D_SET(xPFCCR, AM, 0) |
					DMA2D_SET(xPFCCR, CM, DMA2D_xPFCCR_CM_L8);
	/*
	 * Transfer it!
	 */
	DMA2D_CR |= DMA2D_CR_START;
	if (DMA2D_ISR & DMA2D_ISR_CEIF) {
		printf("Configuration error\n");
		while (1);
	}
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
	int	can_switch;
	int	opt, ds;
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
	opt = 4; /* screen clearing mode */
	can_switch = -1; /* auto switching every 10 seconds */
	t0 = mtime();
	ds = 0;
	

	
	while (1) {
		//usart_send(USART6, 0x33);
		//gpio_toggle(GPIOG, GPIO6);
			
		//usart_send_blocking(USART6, 0x32);

		//usart_send(5,0x34);
		//console_puts("bla");


dma2d_bgfill();




		/* In both cases we write the notes using the graphics library */
		gfx_set_text_color(g, GFX_COLOR_BLACK, GFX_COLOR_BLACK);
		gfx_set_text_size(g, 3);
		gfx_set_text_cursor(g, 25, 55 + DISP_HEIGHT + ((gfx_get_text_height(g) * 3) + 2));
		gfx_puts(g, (char *)"Hello world Nils Supa Display DMA2D!");
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
