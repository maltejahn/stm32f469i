/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2013-2015 Piotr Esden-Tempski <piotr@esden.net>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include <string.h>

void console_puts(char *s);
void console_putc(char c);
void interpret_command(void);
char uart_rx_buffer[128];
uint8_t rx_position=0;

//#define UART_PORTG
#define UART_PORTC

static void clock_setup(void)
{
	/* Enable GPIOG clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Enable clocks for USART6. */
	rcc_periph_clock_enable(RCC_USART6);
}

static void usart_setup(void)
{
	/* Enable the USART6 interrupt. */
	nvic_enable_irq(NVIC_USART6_IRQ);

// Settings for PC6+7
#ifdef UART_PORTC
	// Setup GPIO pins for USART6 transmit. 
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);

	// Setup GPIO pins for USART6 receive. 
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);
	gpio_set_output_options(GPIOC, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO7);

	// Setup USART3 TX and RX pin as alternate function. 
	gpio_set_af(GPIOC, GPIO_AF8, GPIO6);
	gpio_set_af(GPIOC, GPIO_AF8, GPIO7);
#endif

#ifdef UART_PORTG
// Settings for PG14(TX, AF8,D1) and PG9(RX, AF8, D0)
	// Setup GPIO pins for USART6 transmit. 
	gpio_mode_setup(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO14);

	// Setup GPIO pins for USART6 receive. 
	gpio_mode_setup(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_output_options(GPIOG, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO9);

	// Setup USART3 TX and RX pin as alternate function. 
	gpio_set_af(GPIOG, GPIO_AF8, GPIO14);
	gpio_set_af(GPIOG, GPIO_AF8, GPIO9);
#endif


	/* Setup USART6 parameters. */
	usart_set_baudrate(USART6, 57600);
	usart_set_databits(USART6, 8);
	usart_set_stopbits(USART6, USART_STOPBITS_1);
	usart_set_mode(USART6, USART_MODE_TX_RX);
	usart_set_parity(USART6, USART_PARITY_NONE);
	usart_set_flow_control(USART6, USART_FLOWCONTROL_NONE);

	/* Enable USART6 Receive interrupt. */
	usart_enable_rx_interrupt(USART6);

	/* Finally enable the USART. */
	usart_enable(USART6);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO13 on GPIO port G for LED. */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
}

int main(void)
{
	clock_setup();
	gpio_setup();
	usart_setup();
	console_puts("Hi there");

	while (1) {
		__asm__("NOP");
	}

	return 0;
}

// just compare what was received after a 
void interpret_command(void){

if(strcmp(uart_rx_buffer,"test")==0)
{
	console_puts("test ok");
}
else{
	console_puts("unknown command");
}
	

/*
for(uint8_t i=0; i<=rx_position; i++){
		uint32_t reg;
		do{
			reg=USART_SR(USART6);
		}while((reg & USART_SR_TXE)==0);
		USART_DR(USART6) = (uint16_t) uart_rx_buffer[i] & 0xff;
}
*/

rx_position=0;


}
// just put a char to the console
void console_putc(char c)
{
	uint32_t	reg;
	do {
		reg = USART_SR(USART6);
	} while ((reg & USART_SR_TXE) == 0);
	USART_DR(USART6) = (uint16_t) c & 0xff;
}

// put a string to the consoele
void console_puts(char *s)
{
	while (*s != '\000') {
		console_putc(*s);
		/* Add in a carraige return, after sending line feed */
		if (*s == '\n') {
			console_putc('\r');
		}
		s++;
	}
}

// ISR: Only RX ISR is used
void usart6_isr(void)
{
	static uint8_t data = 'A';

	/* Check if we were called because of RXNE. */
	if (((USART_CR1(USART6) & USART_CR1_RXNEIE) != 0) &&
	    ((USART_SR(USART6) & USART_SR_RXNE) != 0)) {

		/* Indicate that we got data. */
		gpio_toggle(GPIOG, GPIO6);

		/* Retrieve the data from the peripheral. */
		data = usart_recv(USART6);
		// if CR received discard this, otherwise add to input buffer
		if(data!=0x0D)uart_rx_buffer[rx_position++]=data;
		// If CR received, do something with the rx buffer
		if(data ==0x0D)
		{
			interpret_command();
			
		}


	}

}
