#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>
#include <cpu_speed.h>

#include <graphics.h>
#include <sprite.h>

#define W 14
#define H 6

uint8_t bitmap[] = {
	0b00000011, 0b00000000,
	0b01000111, 0b10001000,
	0b10001100, 0b11000100,
	0b11011011, 0b01101100,
	0b01110111, 0b10111000,
	0b00111111, 0b11100000,
};

Sprite alien;

void setup_alien(void) {
	int x = rand() % (LCD_X - W);
	int y = rand() % (LCD_Y - H);
	sprite_init(&alien, x, y, W, H, bitmap);
}

void update_alien() {
	double dx = ((double) rand()) / RAND_MAX - 0.5;
	double dy = ((double) rand()) / RAND_MAX - 0.5;
	double v = sqrt(dx*dx + dy*dy + 1e-10);

	dx /= v;
	dy /= v;

	alien.x += dx;
	alien.y += dy;

	int x = round(alien.x);
	int y = round(alien.y);

	if ( x < 0 || x + W >= LCD_X ) alien.x -= dx;
	if ( y < 0 || y + H >= LCD_Y ) alien.y -= dy;
}

void setup(void) {
	set_clock_speed(CPU_8MHz);
	lcd_init(LCD_DEFAULT_CONTRAST);
	clear_screen();
	setup_alien();
	sprite_draw(&alien);
	show_screen();
}

void process(void) {
	clear_screen();
	update_alien();
	sprite_draw(&alien);
	show_screen();
}

int main(void) {
	setup();

	for ( ;; ) {
		process();
		_delay_ms(10);
	}
}
