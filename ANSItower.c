#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <cpu_speed.h>
#include <macros.h>
#include <sprite.h>
#include <graphics.h>
#include <lcd.h>
#include <stdbool.h>
#include <math.h>
#include "lcd_model.h"

//Global variables
int x;
int y;
//Random Values
uint8_t randx;
uint8_t randy;
//Bools
bool start = true;
bool pick_up = false;
bool shieldPickUp = false;
bool game_over = false;
bool load = false;
bool nextFloor = false;
bool visible = true;
//Movement
double dx;
double dy;
double monMoveX;
double monMoveY;
double monSpeed = 0.5;
//Pause
int Score = 0;
int Lives = 3;
int Floor = 0;
//Timer
volatile uint16_t overflow_count = 0;

//Timer Overflow
ISR(TIMER3_OVF_vect){
	overflow_count++;
}

int elapsed_time( void ){
	int time = (overflow_count * 65536.0 + TCNT3) * 8.0 / 8000000.0;
	return time;
}

//DEFINING FUNCTIONS
void draw_int(uint8_t x, uint8_t y, int value, colour_t colour);
char intbuffer[10];
void draw_int(uint8_t x, uint8_t y, int value, colour_t colour) {
	snprintf(intbuffer, sizeof(intbuffer), "%d", value);
	draw_string(x, y, intbuffer, colour);
}
#define sprite_step(sprite)(sprite.x += sprite.dx, sprite.y += sprite.dy)
#define sprite_turn_to(sprite, _dx, _dy)(sprite.dx = _dx, sprite.dy = _dy)
#define sprite_visible(sprite) (sprite.is_visible)


//Player collisions
bool sprite_collisions (Sprite s1, int width_1, int height_1, Sprite s2, int width_2, int height_2){
	bool collided = true;
	//sprite: protag
	int px = s1.x;
	int py = s1.y;
	int pr = px + width_1;
	int pb = py + height_1;
	//sprite object
	int sx = s2.x;
	int sy = s2.y;
	int sr = sx + width_2;
	int sb = sy + height_2;
	//collisions
	if (pr < sx) { collided = false; }
	if (pb < sy) { collided = false; }
	if (sr < px) { collided = false; }
	if (sb < py) { collided = false; }
	return collided;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////SPRITES
//MISC
#define misc_w 72
#define misc_h 8
uint8_t miscImage[72];
Sprite misc;
void setup_misc ( void ){
	for (int i = 0; i < 72; i++){
		miscImage[i] = 0b00000000;
	}
	sprite_init(&misc, (LCD_X / 2) - 44, -50, misc_w, misc_h, miscImage);
}
//Tower walls for collision detection 
#define towerWall_w 8
#define towerWall_h 88
#define towerFront_w 37
#define towerFront_h 1
uint8_t towerLeftImage[88];
uint8_t towerRightImage[88];
uint8_t towerFrontImage[37];

Sprite towerLeft;
Sprite towerRight;
Sprite towerFrontLeft;
Sprite towerFrontRight;

void setup_towerWall( void ){
	for (int i = 0; i < 88; i++){
		towerLeftImage[i] = 0b10000000;
	}
	for (int j = 0; j < 88; j++){
		towerRightImage[j] = 0b00000001;
	}
	sprite_init(&towerLeft, (LCD_X / 2) - 44, -72, towerWall_w, towerWall_h, towerLeftImage);
	sprite_init(&towerRight, (LCD_X / 2) + 36, -72, towerWall_w, towerWall_h, towerRightImage);
}
void setup_towerFrontWall( void ){
	for (int a = 0; a < 37; a++){
		towerFrontImage[a] = 0b11111111;
	}
	sprite_init(&towerFrontLeft, (LCD_X / 2) - 44, -72 + 88, towerFront_w, towerFront_h, towerFrontImage);
	sprite_init(&towerFrontRight, (LCD_X / 2) + 7, -72 + 88, towerFront_w, towerFront_h, towerFrontImage);
}

//Door
#define door_w 16
#define door_h 15
uint8_t doorImage[] = {
	0b00000111,0b11100000,
	0b00011100,0b00111000,
	0b00110000,0b00001100,
	0b01100000,0b00000110,
	0b11000000,0b00000011,
	0b10000000,0b00000001,
	0b10000000,0b00011001,
	0b10000000,0b00011001,
	0b10000000,0b00000001,
	0b10000000,0b00000001,
	0b10000000,0b00000001,
	0b10000000,0b00000001,
	0b10000000,0b00000001,
	0b10000000,0b00000001,
	0b10000000,0b00000001,
};
Sprite door;
void setup_door( void ){
	x = (LCD_X / 2) - 8;
	y = 1;
	randx = 1+ rand() % (154 - 16);
	randy = 1+ rand() % (100 - 15);
	if (Floor == 0){
		sprite_init(&door, x, y, door_w, door_h, doorImage);
	}
	else
	{
		sprite_init(&door, randx, randy, door_w, door_h, doorImage);
	}
}

//Border
#define wall_w 8
#define wall_h 100
#define front_wall_w 154
#define front_wall_h 8
uint8_t wallImage[100];
uint8_t frontWallImage[154];
Sprite wall_right;
Sprite wall_left;
Sprite front_wall_top;
Sprite front_wall_bottom;

void setup_wall( void ){
	//126 - 72
	for (int i = 0; i < 100; i++)
	{
		wallImage[i] = 0b11111111; 
	}	
	sprite_init(&wall_right, -35, -50, wall_w, wall_h, wallImage);
	sprite_init(&wall_left, LCD_X + 27, -50, wall_w, wall_h, wallImage);	
}

void setup_front_wall( void ){
	for (int i = 0; i < 154; i++)
	{
		frontWallImage[i] = 0b11111111;
	}
	sprite_init(&front_wall_top, -35, -50 , front_wall_w, front_wall_h, frontWallImage); 
	sprite_init(&front_wall_bottom, -35, 50, front_wall_w, front_wall_h, frontWallImage); 
}

//Protagonist 
#define protag_w 7
#define protag_h 11
uint8_t protagImage[] = {
	0b00101000,
	0b00111000,
	0b00101000,
	0b00111000,
	0b00000000,
	0b11111110,
	0b10101010,
	0b10111010,
	0b00101000,
	0b00101000,
	0b00101000,
};
Sprite protag;
void setup_protag( void ){
	x = (LCD_X / 2) - (protag_w / 2);
	y = 30;	
	sprite_init(&protag, x, y, protag_w, protag_h, protagImage);	
}

//Monster
#define monster_w 5
#define monster_h 7
uint8_t monsterImage[] = {
	0b01110000,
	0b11111000,
	0b10101000,
	0b11111000,
	0b11111000,
	0b10101000,
	0b10101000,
};
Sprite monster_1;
Sprite monster_2;
Sprite monster_3;
Sprite monster_4;
Sprite monster_5;
void setup_monster_1( void ){
	x = LCD_X + 15;
	y = -20;
	randx = 1+ rand() % (154 - 5);
	randy = 1+ rand() % (100 - 7);
	if (Floor == 0){
		sprite_init(&monster_1, x, y, monster_w, monster_h, monsterImage);
	}
	else 
	{
		sprite_init(&monster_1, randx, randy, monster_w, monster_h, monsterImage);
	}
}
void setup_monster_2( void ){
	randx = 1+ rand() % (154 - 5);
	randy = 1+ rand() % (100 - 7);
	if (Floor > 0) 
	{
		sprite_init(&monster_2, randx, randy, monster_w, monster_h, monsterImage);
	}
}
void setup_monster_3( void ){
	randx = 1+ rand() % (154 - 5);
	randy = 1+ rand() % (100 - 7);
	if (Floor > 0) 
	{
		sprite_init(&monster_3, randx, randy, monster_w, monster_h, monsterImage);
	}
}
void setup_monster_4( void ){
	randx = 1+ rand() % (154 - 5);
	randy = 1+ rand() % (100 - 7);
	if (Floor > 0) 
	{
		sprite_init(&monster_4, randx, randy, monster_w, monster_h, monsterImage);
	}
}
void setup_monster_5( void ){
	randx = 1+ rand() % (154 - 5);
	randy = 1+ rand() % (100 - 7);
	if (Floor > 0) 
	{
		sprite_init(&monster_5, randx, randy, monster_w, monster_h, monsterImage);
	}
}

//Key
#define key_w 7
#define key_h 3
uint8_t keyImage[] = {
	0b11100000,
	0b10111110,
	0b11101010,
};
Sprite key;
void setup_key( void ){
	x = -15;
	y = -20;
	randx = 1+ rand() % (154 - 7);
	randy = 1+ rand() % (100 - 3);
	if (Floor == 0){
		sprite_init(&key, x, y, key_w, key_h, keyImage);
	}
	else 
	{
		sprite_init(&key, randx, randy, key_w, key_h, keyImage);
	}
}

//Treasure
#define treasure_w 6
#define treasure_h 5
uint8_t treasureImage[] = {
	0b01111000,
	0b10001100,
	0b10110100,
	0b11000100,
	0b01111000,
};
Sprite treasure_1;
Sprite treasure_2;
Sprite treasure_3;
Sprite treasure_4;
Sprite treasure_5;
void setup_treasure_1( void ){
	randx = 1+ rand() % (154 - 6);
	randy = 1+ rand() % (100 - 5);
	if (Floor > 0){
		sprite_init(&treasure_1, randx, randy, treasure_w, treasure_h, treasureImage);
	}
}
void setup_treasure_2( void ){
	randx = 1+ rand() % (154 - 6);
	randy = 1+ rand() % (100 - 5);
	if (Floor > 0){
		sprite_init(&treasure_2, randx, randy, treasure_w, treasure_h, treasureImage);
	}
}
void setup_treasure_3( void ){
	randx = 1+ rand() % (154 - 6);
	randy = 1+ rand() % (100 - 5);
	if (Floor > 0){
		sprite_init(&treasure_3, randx, randy, treasure_w, treasure_h, treasureImage);
	}
}
void setup_treasure_4( void ){
	randx = 1+ rand() % (154 - 6);
	randy = 1+ rand() % (100 - 5);
	if (Floor > 0){
		sprite_init(&treasure_4, randx, randy, treasure_w, treasure_h, treasureImage);
	}
}
void setup_treasure_5( void ){
	randx = 1+ rand() % (154 - 6);
	randy = 1+ rand() % (100 - 5);
	if (Floor > 0){
		sprite_init(&treasure_5, randx, randy, treasure_w, treasure_h, treasureImage);
	}
}
//Shield
#define shield_w 6
#define shield_h 5
uint8_t shieldImage[] = {
	0b10110100,
	0b11111100,
	0b11111100,
	0b01111000,
	0b00110000,
};
Sprite shield;
void setup_shield( void ){
	randx = 1+ rand() % (154 - 6);
	randy = 1+ rand() % (100 - 5);
	if (Floor > 0){
		sprite_init(&shield, randx, randy, shield_w, shield_h, shieldImage);
	}
}

#define vertWall_w 3
#define vertWall_h 31
uint8_t vertWallImage[31];
Sprite vertWall_1;
Sprite vertWall_2;
Sprite vertWall_3;

void setup_vertWall_1( void ){
	for (int i = 0; i < 31; i++){
		vertWallImage[i] = 0b11100000;
	}
	randx = 1+ rand() % (154 - 3);
	randy = 1+ rand() % (100 - 31);
	if (Floor > 0){
		sprite_init(&vertWall_1, randx, randy, vertWall_w, vertWall_h, vertWallImage);
	}
}

void setup_vertWall_2( void ){
	for (int i = 0; i < 31; i++){
		vertWallImage[i] = 0b11100000;
	}
	randx = 1+ rand() % (154 - 3);
	randy = 1+ rand() % (100 - 31);
	if (Floor > 0){
		sprite_init(&vertWall_2, randx, randy, vertWall_w, vertWall_h, vertWallImage);
	}
}

void setup_vertWall_3( void ){
	for (int i = 0; i < 31; i++){
		vertWallImage[i] = 0b11100000;
	}
	randx = 1+ rand() % (154 - 3);
	randy = 1+ rand() % (100 - 31);
	if (Floor > 0){
		sprite_init(&vertWall_3, randx, randy, vertWall_w, vertWall_h, vertWallImage);
	}
}

#define horiWall_w 20
#define horiWall_h 3
uint8_t horiWallImage[20];
Sprite horiWall_1;
Sprite horiWall_2;
Sprite horiWall_3;

void setup_horiWall_1( void ){
	for (int i = 0; i < 20; i++){
		horiWallImage[i] = 0b11111111;
	}
	randx = 1+ rand() % (154 - 20);
	randy = 1+ rand() % (100 - 3);
	if (Floor > 0){
		sprite_init(&horiWall_1, randx, randy, horiWall_w, horiWall_h, horiWallImage);
	}
}

void setup_horiWall_2( void ){
	for (int i = 0; i < 20; i++){
		horiWallImage[i] = 0b11111111;
	}
	randx = 1+ rand() % (154 - 20);
	randy = 1+ rand() % (100 - 3);
	if (Floor > 0){
		sprite_init(&horiWall_2, randx, randy, horiWall_w, horiWall_h, horiWallImage);
	}
}

void setup_horiWall_3( void ){
	for (int i = 0; i < 20; i++){
		horiWallImage[i] = 0b11111111;
	}
	randx = 1+ rand() % (154 - 20);
	randy = 1+ rand() % (100 - 3);
	if (Floor > 0){
		sprite_init(&horiWall_3, randx, randy, horiWall_w, horiWall_h, horiWallImage);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
//Start menu 
void start_menu( void ){
	draw_string(20, 0, "ANSI Tower", FG_COLOUR);
	draw_string(25, 8, "Jai Hunt", FG_COLOUR);
	draw_string(25, 16, "10006869", FG_COLOUR);
	draw_string(0, 40, "Press SW2", FG_COLOUR);
	start = true;
	show_screen();
}	

//Countdown initation
void countdown ( void ){
	clear_screen();
	draw_string(0, 40, "...3", FG_COLOUR);
	show_screen();
	_delay_ms(300);
	clear_screen();
	draw_string(0, 40, "...2", FG_COLOUR);
	show_screen();
	_delay_ms(300);
	clear_screen();
	draw_string(0, 40, "...1", FG_COLOUR);
	show_screen();
	_delay_ms(300);
	clear_screen(); 
}

//Pause screen
void pause ( void ){
	//Timer setup
	clear_screen();
	int count = elapsed_time();
	int minutes = count/60;
	int seconds = count%60;
	//Display
	draw_string(1, 0, "Score:", FG_COLOUR);
	draw_int(35, 0, Score, FG_COLOUR);
	draw_string(1, 8, "Lives:", FG_COLOUR);
	draw_int(35, 8, Lives, FG_COLOUR);
	draw_string(1, 16, "Floor:", FG_COLOUR);
	draw_int(35, 16, Floor, FG_COLOUR);
	//DRAW TIMER 
	char TIMER_STRING[60];
	snprintf(TIMER_STRING, sizeof TIMER_STRING, "Time: %02d:%02d", minutes, seconds);	
	draw_string(1, 24, TIMER_STRING, FG_COLOUR);
	show_screen();
}

//Loading Screen
void loading ( void ){
	clear_screen();
	pick_up = false;
	draw_string(1, 0, "Score:", FG_COLOUR);
	draw_int(35, 0, Score, FG_COLOUR);
	draw_string(1, 16, "Floor:", FG_COLOUR);
	draw_int(35, 16, Floor, FG_COLOUR);
	show_screen();
	_delay_ms(2000);
	load = false;
}

//Main Game
void setup_sprites ( void ){
	setup_protag();
	setup_monster_1();
	setup_key();
	setup_misc();
	setup_towerWall();
	setup_towerFrontWall();
	setup_door();
	setup_wall();
	setup_front_wall();
}
void main_game ( void ){
	sprite_draw(&protag);
	sprite_draw(&monster_1);
	sprite_draw(&key);
	sprite_draw(&wall_left);
	sprite_draw(&wall_right);
	sprite_draw(&front_wall_top);
	sprite_draw(&front_wall_bottom);
	sprite_draw(&misc);
	sprite_draw(&towerLeft);
	sprite_draw(&towerRight);
	sprite_draw(&towerFrontLeft);
	sprite_draw(&towerFrontRight);
	sprite_draw(&door);
	show_screen();
}

//Gameover screen
void gameover( void ){
	clear_screen();
	draw_string(20, 0, "GAME OVER", FG_COLOUR);
	draw_string(1, 8, "Score:", FG_COLOUR);
	draw_int(35, 8, Score, FG_COLOUR);
	draw_string(1, 16, "Floor:", FG_COLOUR);
	draw_int(35, 16, Floor, FG_COLOUR);
	LCD_CMD(lcd_set_display_mode, lcd_display_inverse);
	show_screen();
	if (BIT_IS_SET(PINF, 5)|| BIT_IS_SET(PINF, 6)){
		clear_screen();
		_delay_ms(100);
		pick_up = false;
		start = false;
		start_menu();
		Lives = 3;
		Score = 0;
		Floor = 0;
		setup_sprites();
		show_screen();
	}
}

//Monster AI
void monsterAI_1 ( void ){
	monMoveX = protag.x - monster_1.x;
	monMoveY = protag.y - monster_1.y;
	double dist = sqrt(monMoveX * monMoveX + monMoveY * monMoveY);
	monMoveX = (monMoveX * monSpeed) / dist;
	monMoveY = (monMoveY * monSpeed) / dist;
	sprite_turn_to(monster_1, monMoveX, monMoveY);
	sprite_step(monster_1);
}	

void monsterAI_2 ( void ){
	monMoveX = protag.x - monster_2.x;
	monMoveY = protag.y - monster_2.y;
	double dist = sqrt(monMoveX * monMoveX + monMoveY * monMoveY);
	monMoveX = (monMoveX * monSpeed) / dist;
	monMoveY = (monMoveY * monSpeed) / dist;
	sprite_turn_to(monster_2, monMoveX, monMoveY);
	sprite_step(monster_2);
}	

void monsterAI_3 ( void ){
	monMoveX = protag.x - monster_3.x;
	monMoveY = protag.y - monster_3.y;
	double dist = sqrt(monMoveX * monMoveX + monMoveY * monMoveY);
	monMoveX = (monMoveX * monSpeed) / dist;
	monMoveY = (monMoveY * monSpeed) / dist;
	sprite_turn_to(monster_3, monMoveX, monMoveY);
	sprite_step(monster_3);
}	

void monsterAI_4 ( void ){
	monMoveX = protag.x - monster_4.x;
	monMoveY = protag.y - monster_4.y;
	double dist = sqrt(monMoveX * monMoveX + monMoveY * monMoveY);
	monMoveX = (monMoveX * monSpeed) / dist;
	monMoveY = (monMoveY * monSpeed) / dist;
	sprite_turn_to(monster_4, monMoveX, monMoveY);
	sprite_step(monster_4);
}

void monsterAI_5 ( void ){
	monMoveX = protag.x - monster_5.x;
	monMoveY = protag.y - monster_5.y;
	double dist = sqrt(monMoveX * monMoveX + monMoveY * monMoveY);
	monMoveX = (monMoveX * monSpeed) / dist;
	monMoveY = (monMoveY * monSpeed) / dist;
	sprite_turn_to(monster_5, monMoveX, monMoveY);
	sprite_step(monster_5);
}		
//////////////////////////RANDOM
//Random floor setup
void randFloor_setup( void ){
	setup_towerWall();
	setup_towerFrontWall();
	setup_protag();
	setup_monster_1();
	setup_monster_2();
	setup_monster_3();
	setup_monster_4();
	setup_monster_5();
	setup_key();
	setup_shield();
	setup_treasure_1();
	setup_treasure_2();
	setup_treasure_3();
	setup_treasure_4();
	setup_treasure_5();
	setup_door();
	setup_vertWall_1();
	setup_vertWall_2();
	setup_vertWall_3();
	setup_horiWall_1();
	setup_horiWall_2();
	setup_horiWall_3();
}

//Drawing random floor
void randFloor_main( void ){
	sprite_draw(&wall_left);
	sprite_draw(&wall_right);
	sprite_draw(&front_wall_top);
	sprite_draw(&front_wall_bottom);
	sprite_draw(&protag);
	sprite_draw(&monster_1);
	sprite_draw(&monster_2);
	sprite_draw(&monster_3);
	sprite_draw(&monster_4);
	sprite_draw(&monster_5);
	sprite_draw(&key);
	sprite_draw(&treasure_1);
	sprite_draw(&treasure_2);
	sprite_draw(&treasure_3);
	sprite_draw(&treasure_4);
	sprite_draw(&treasure_5);
	sprite_draw(&shield);
	sprite_draw(&door);
	sprite_draw(&vertWall_1);
	sprite_draw(&vertWall_2);
	sprite_draw(&vertWall_3);
	sprite_draw(&horiWall_1);
	sprite_draw(&horiWall_2);
	sprite_draw(&horiWall_3);
}

void collision ( void ){
	//wall collisions
	if (sprite_collisions(protag, protag_w, protag_h, wall_right, wall_w, wall_h)){
		dx = -2;
	}
	if (sprite_collisions(protag, protag_w, protag_h, wall_left, wall_w, wall_h)){
		dx = 2;
	}
	if (sprite_collisions(protag, protag_w, protag_h, front_wall_top, front_wall_w, front_wall_h)){
		dy = -2;
	}
	if (sprite_collisions(protag, protag_w, protag_h, front_wall_bottom, front_wall_w, front_wall_h)){
		dy = 2;
	}
	//tower collision
	if (sprite_collisions(protag, protag_w, protag_h, towerLeft, towerWall_w, towerWall_h)){
		dx = 2;
	}	
	if (sprite_collisions(protag, protag_w, protag_h, towerRight, towerWall_w, towerWall_h)){
		dx = -2;
	}
	if (sprite_collisions(protag, protag_w, protag_h, towerFrontLeft, towerFront_w, towerFront_h)){
		dy = -2;
	}	
	if (sprite_collisions(protag, protag_w, protag_h, towerFrontRight, towerFront_w, towerFront_h)){
		dy = -2;
	}
	//monster - tower collision
	if (sprite_collisions(monster_1, monster_w, monster_h, towerLeft, towerWall_w, towerWall_h)){
		monster_1.x = monster_1.x - 2;
	}	
	if (sprite_collisions(monster_1, monster_w, monster_h, towerRight, towerWall_w, towerWall_h)){
		monster_1.x = monster_1.x + 2;
	}
	if (sprite_collisions(monster_1, monster_w, monster_h, towerFrontLeft, towerFront_w, towerFront_h)){
		monster_1.y = monster_1.y + 2;
	}	
	if (sprite_collisions(monster_1, monster_w, monster_h, towerFrontRight, towerFront_w, towerFront_h)){
		monster_1.y = monster_1.y + 2;
	}
	//treasure collision
	if (sprite_collisions(protag, protag_w, protag_h, treasure_1, treasure_w, treasure_h)){
		Score = Score + 10;
		treasure_1.is_visible = false;
	}
	if (sprite_collisions(protag, protag_w, protag_h, treasure_2, treasure_w, treasure_h)){
		Score = Score + 10;
		treasure_2.is_visible = false;
	}
		if (sprite_collisions(protag, protag_w, protag_h, treasure_3, treasure_w, treasure_h)){
		Score = Score + 10;
		treasure_3.is_visible = false;
	}
		if (sprite_collisions(protag, protag_w, protag_h, treasure_4, treasure_w, treasure_h)){
		Score = Score + 10;
		treasure_4.is_visible = false;
	}
		if (sprite_collisions(protag, protag_w, protag_h, treasure_5, treasure_w, treasure_h)){
		Score = Score + 10;
		treasure_5.is_visible = false;
	}
	//shield collision
	if (sprite_collisions(protag, protag_w, protag_h, shield, shield_w, shield_h)){
		shieldPickUp = true;
	}
	//random wall collision
	if (sprite_collisions(protag, protag_w, protag_h, vertWall_1, vertWall_w, vertWall_h)){
		dx = -2;
	}	
	if (sprite_collisions(protag, protag_w, protag_h, vertWall_2, vertWall_w, vertWall_h)){
		dx = 2;
	}	
	if (sprite_collisions(protag, protag_w, protag_h, vertWall_3, vertWall_w, vertWall_h)){
		dx = 2;
	}	
	if (sprite_collisions(monster_1, monster_w, monster_h, vertWall_1, vertWall_w, vertWall_h)){
		monster_1.x = monster_1.x + 2;
	}	
	if (sprite_collisions(monster_1, monster_w, monster_h, vertWall_2, vertWall_w, vertWall_h)){
		monster_1.x = monster_1.x + 2;
	}	
	if (sprite_collisions(monster_1, monster_w, monster_h, vertWall_3, vertWall_w, vertWall_h)){
				monster_1.x = monster_1.x + 2;
	}	
	if (sprite_collisions(monster_2, monster_w, monster_h, vertWall_1, vertWall_w, vertWall_h)){
		monster_2.x = monster_2.x + 2;
	}	
	if (sprite_collisions(monster_2, monster_w, monster_h, vertWall_2, vertWall_w, vertWall_h)){
		monster_2.x = monster_2.x + 2;
	}	
	if (sprite_collisions(monster_3, monster_w, monster_h, vertWall_3, vertWall_w, vertWall_h)){
				monster_3.x = monster_3.x + 2;
	}	
		if (sprite_collisions(monster_3, monster_w, monster_h, vertWall_1, vertWall_w, vertWall_h)){
		monster_3.x = monster_3.x + 2;
	}	
	if (sprite_collisions(monster_4, monster_w, monster_h, vertWall_2, vertWall_w, vertWall_h)){
		monster_4.x = monster_4.x + 2;
	}	
	if (sprite_collisions(monster_5, monster_w, monster_h, vertWall_3, vertWall_w, vertWall_h)){
			monster_5.x = monster_5.x + 2;
	}	
		if (sprite_collisions(monster_4, monster_w, monster_h, vertWall_1, vertWall_w, vertWall_h)){
		monster_4.x = monster_4.x + 2;
	}	
	if (sprite_collisions(monster_4, monster_w, monster_h, vertWall_2, vertWall_w, vertWall_h)){
		monster_4.x = monster_4.x + 2;
	}	
	if (sprite_collisions(monster_5, monster_w, monster_h, vertWall_3, vertWall_w, vertWall_h)){
				monster_5.x = monster_5.x + 2;
	}	
	if (sprite_collisions(protag, protag_w, protag_h, horiWall_1, horiWall_w, horiWall_h)){
		dy = 2;
	}	
	if (sprite_collisions(protag, protag_w, protag_h, horiWall_2, horiWall_w, horiWall_h)){
		dy = -2;
	}		
	if (sprite_collisions(protag, protag_w, protag_h, horiWall_3, horiWall_w, horiWall_h)){
		dy = -2;
	}		
	if (sprite_collisions(monster_1, monster_w, monster_h, horiWall_1, horiWall_w, horiWall_h)){
	monster_1.x = monster_1.x + 2;		
	}	
	if (sprite_collisions(monster_1, monster_w, monster_h, horiWall_2, horiWall_w, horiWall_h)){
	monster_1.x = monster_1.x + 2;		
	}	
	if (sprite_collisions(monster_1, monster_w, monster_h, horiWall_3, horiWall_w, horiWall_h)){
	monster_1.x = monster_1.x + 2;		
	}	
	//key collision
	if (sprite_collisions(protag, protag_w, protag_h, key, key_w, key_h)){
		pick_up = true;
	}
	//monster collision
	if (sprite_collisions(protag, protag_w, protag_h, monster_1, key_w, key_h) || sprite_collisions(protag, protag_w, protag_h, monster_2, key_w, key_h) || sprite_collisions(protag, protag_w, protag_h, monster_3, key_w, key_h) || sprite_collisions(protag, protag_w, protag_h, monster_4, key_w, key_h) || sprite_collisions(protag, protag_w, protag_h, monster_5, key_w, key_h)){
		Lives = Lives - 1;
		pick_up = false;
		//respawn
		clear_screen();	
		setup_sprites();
		main_game();
		show_screen();
		if (shieldPickUp == true){
			shieldPickUp = false;
			shield.is_visible = false;
		}
	}
	//door collision
	if (sprite_collisions(protag, protag_w, protag_h, door, door_w, door_h)){ 
		dy = -2;
	}
	if (pick_up == true && nextFloor == false){
		if (sprite_collisions(protag, protag_w, protag_h, door, door_w, door_h)){ 
			load = true;
			Floor = Floor + 1;
			Score = Score + 100;
			pick_up = false;
			while (load == true){
				loading();
			}
			randFloor_setup();
		}
	}
}


//Player Movement
void movement( void ){
	dx = 0;
	dy = 0;
	if (BIT_IS_SET(PIND, 1)) dy = 2; //Up Joystick
	if (BIT_IS_SET(PINB, 7)) dy = -2; //Down Joystick
	if (BIT_IS_SET(PINB, 1)) dx = 2; //Left joystick
	if (BIT_IS_SET(PIND, 0)) dx = -2; //Right joystick
	//Collision detection
	collision();
	//scrolling map movement
	if (Floor == 0){
		monster_1.x += dx;
		monster_1.y += dy;
		clear_screen();
		sprite_draw(&monster_1);
		wall_right.x += dx;
		wall_right.y += dy;
		sprite_draw(&wall_right);
		wall_left.x += dx;
		wall_left.y += dy;
		sprite_draw(&wall_left);
		front_wall_top.x += dx;
		front_wall_top.y += dy;
		sprite_draw(&front_wall_top);
		front_wall_bottom.x += dx;
		front_wall_bottom.y += dy;
		sprite_draw(&front_wall_bottom);
		key.x += dx;
		key.y += dy;
		sprite_draw(&key);
		misc.x += dx;
		misc.y += dy;
		sprite_draw(&misc);
		towerLeft.x += dx;
		towerLeft.y += dy;
		sprite_draw(&towerLeft);
		towerRight.x += dx;
		towerRight.y += dy;
		sprite_draw(&towerRight);
		towerFrontLeft.x += dx;
		towerFrontLeft.y += dy;
		sprite_draw(&towerFrontLeft);
		towerFrontRight.x += dx;
		towerFrontRight.y += dy;
		sprite_draw(&towerFrontRight);
		door.x += dx;
		door.y += dy;
		sprite_draw(&door);
		sprite_draw(&protag);
		show_screen();
	}
	else if (Floor >= 1){
		wall_right.x += dx;
		wall_right.y += dy;
		clear_screen();
		sprite_draw(&wall_right);
		wall_left.x += dx;
		wall_left.y += dy;
		sprite_draw(&wall_left);	
		front_wall_top.x += dx;
		front_wall_top.y += dy;
		sprite_draw(&front_wall_top);
		front_wall_bottom.x += dx;
		front_wall_bottom.y += dy;
		sprite_draw(&front_wall_bottom);		
		monster_1.x += dx;
		monster_1.y += dy;
		monster_2.x += dx;
		monster_2.y += dy;
		monster_3.x += dx;
		monster_3.y += dy;
		monster_4.x += dx;
		monster_4.y += dy;
		monster_5.x += dx;
		monster_5.y += dy;
		sprite_draw(&monster_1);
		sprite_draw(&monster_2);
		sprite_draw(&monster_3);
		sprite_draw(&monster_4);
		sprite_draw(&monster_5);
		key.x += dx;
		key.y += dy;
		sprite_draw(&key);
		shield.x += dx;
		shield.y += dy;
		sprite_draw(&shield);
		door.x += dx;
		door.y += dy;
		sprite_draw(&door);
		treasure_1.x += dx;
		treasure_1.y += dy;
		treasure_2.x += dx;
		treasure_2.y += dy;
		treasure_3.x += dx;
		treasure_3.y += dy;
		treasure_4.x += dx;
		treasure_4.y += dy;
		treasure_5.x += dx;
		treasure_5.y += dy;
		sprite_draw(&treasure_1);
		sprite_draw(&treasure_2);
		sprite_draw(&treasure_3);
		sprite_draw(&treasure_4);
		sprite_draw(&treasure_5);
		vertWall_1.x += dx;
		vertWall_2.x += dx;
		vertWall_3.x += dx;
		vertWall_1.y += dy;
		vertWall_2.y += dy;
		vertWall_3.y += dy;
		sprite_draw(&vertWall_1);
		sprite_draw(&vertWall_2);
		sprite_draw(&vertWall_3);
		horiWall_1.x += dx;
		horiWall_2.x += dx;
		horiWall_3.x += dx;
		horiWall_1.y += dy;
		horiWall_2.y += dy;
		horiWall_3.y += dy;
		sprite_draw(&horiWall_1);
		sprite_draw(&horiWall_2);
		sprite_draw(&horiWall_3);
		sprite_draw(&protag);
		show_screen();
	}
	
	//key movements
	if (pick_up == true){
		key.x = protag.x - 9;
		key.y = protag.y + 6;
	}
	//shield movements
	if (shieldPickUp == true){
		shield.x = protag.x - 9;
		shield.y = protag.y + 6;
	}	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Main game setup and process
void setup( void ) {
	set_clock_speed(CPU_8MHz);
	lcd_init(LCD_DEFAULT_CONTRAST);
	SET_OUTPUT(DDRC, 7); //lcd
	SET_BIT(PORTC, 7); //backlight
	TCCR3A = 0; //timer
	TCCR3B = 2; 
	TIMSK3 = 1; //overflow
	sei(); //interrupt
	CLEAR_BIT(DDRF, 6); //left button
	CLEAR_BIT(DDRF, 5); //right button
	CLEAR_BIT(DDRB, 1); //left joystick
	CLEAR_BIT(DDRD, 0); //right joystick
	CLEAR_BIT(DDRD, 1); //up joystick
	CLEAR_BIT(DDRB, 7); //down joystic
	CLEAR_BIT(DDRB, 0); //middle joystick
	//Sprite setup
	start_menu();
	setup_sprites();
}

void process ( void ){
	if (Lives == 0){
		game_over = true;
	}
	if (game_over == true){
		gameover();
	}
	while (start == true){
		LCD_CMD(lcd_set_display_mode, lcd_display_normal);
		if (BIT_IS_SET(PINF, 5)|| BIT_IS_SET(PINF, 6)){
			clear_screen();
			_delay_ms(100);
			game_over = false;			
			countdown();
			start = false;
		}
	}
	if (game_over == false){
		while (BIT_IS_SET(PINB, 0)){
			pause();
			clear_screen();		
		} 
		clear_screen();
		//monster only moves when on screen
		if (monster_1.x > -monster_w && monster_1.y > -monster_h && monster_1.x < front_wall_w && monster_1.y < wall_h){
			monsterAI_1();
			monsterAI_2();
			monsterAI_3();
			monsterAI_4();
			monsterAI_5();
		}
		//level generation
		if (Floor == 0){
			main_game();
		}
		else
		{
			randFloor_main();
		}
		movement();
		collision();
		show_screen();
	}
}

int main( void ) {
		setup();

	for ( ;; ) {
		process();
		_delay_ms(10);
	}

	return 0;
}