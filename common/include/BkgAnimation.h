#ifndef BKGANIM_H
#define BKGANIM_H

#include <gb/gb.h> 

typedef struct {
	UINT8 x;
	UINT8 y;
	UINT8 sx;
	UINT8 sy;
	UINT8 frame;
	UINT8 frames;
	UINT16 timer;
	UINT8 speed;
	unsigned char *map;
	UINT8 pos;
	UINT8 bank;
} MAP_ANIMATION;

typedef struct {
	UINT16 frame;
	UINT8 frames;
	UINT8 tiles;
	UINT16 timer;
	UINT8 speed;
	unsigned char *map;
	UINT8 bank;
}TILE_ANIMATION;

//MAP Animation: Animates MAP tiles using the ones loaded with InitScrollTiles
//It should be used to make Big BKG animations
void LOAD_MAP_ANIM(MAP_ANIMATION*, UINT8 sx, UINT8 sy, UINT8 frames, unsigned char *map, UINT8 bank);
void MAP_ANIMATE(MAP_ANIMATION* anim, UINT8 x, UINT8 y, UINT8 speed); 

//MAP TILE Animation: Animates a specific tile number using custom data
void LOAD_TILE_ANIM(TILE_ANIMATION* anim, UINT8 tiles, UINT8 frames, unsigned char *map, UINT8 bank);
void TILE_ANIMATE(TILE_ANIMATION* anim, UINT8 tile, UINT8 speed);

//MAP TILE Animation: replaces specific tiles by null tiles (to collect things from BKG)
void BKG_COLLECT_ITEM(UINT16 first_tile, UINT8 tiles, UINT8 color);

//REPLACE TILE: 
void BKG_EDIT_TILE(UINT8 x, UINT8 y, UINT8 number);
void WIN_EDIT_TILE(UINT8 x, UINT8 y, UINT8 number);

//SET TILE DATA: 
//void ZGB_SET_DATA(UINT8 tile, UINT8 tiles, UINT16 *source);
#endif
