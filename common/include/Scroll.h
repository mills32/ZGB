#ifndef SCROLL_H
#define SCROLL_H

#include <gb/gb.h> 

extern unsigned char* scroll_map;
extern INT16 scroll_x;
extern INT16 scroll_y;
extern UINT16 scroll_w;
extern UINT16 scroll_h;
extern UINT16 scroll_tiles_w;
extern UINT16 scroll_tiles_h;
extern UINT8  scroll_collisions[128];
extern UINT8  scroll_collisions_down[128];
extern UINT8 scroll_tile_info[256];
extern UINT8  scroll_bank;
extern UINT8 scroll_offset_x;
extern UINT8 scroll_offset_y;

extern struct Sprite* scroll_target;
extern UINT8 clamp_enabled;


#define InitScrollTiles(first_tile, n_tiles, tile_data, tile_bank) ZInitScrollTilesColor(first_tile, n_tiles, tile_data, tile_bank, 0)
#define InitScrollTilesColor(first_tile, n_tiles, tile_data, tile_bank) ZInitScrollTilesColor(first_tile, n_tiles, tile_data, tile_bank, tile_data##CGB)

void ZInitScrollTilesColor(UINT8 first_tile, UINT8 n_tiles, UINT8* tile_data, UINT8 tile_bank, UINT8* palette_entries);
void InitScroll(UINT16 map_w, UINT16 map_h, unsigned char* map, const UINT8* coll_list, const UINT8* coll_list_down, UINT8 bank, unsigned char* color_map);
void InitWindow(UINT8 x, UINT8 y, UINT8 w, UINT8 h, UINT8* map, UINT8 bank, unsigned char* color_map);
void ScrollSetMap(UINT16 map_w, UINT16 map_h, unsigned char* map, UINT8 bank, unsigned char* color_map);
void ScrollUpdateRow(INT16 x, INT16 y);
void ScrollUpdateColumn(INT16 x, INT16 y);
void MoveScroll(INT16 x, INT16 y);
void RefreshScroll();
void FinishPendingScrollUpdates();

UINT8* GetScrollTilePtr(UINT16 x, UINT16 y);
UINT8 GetScrollTile(UINT16 x, UINT16 y);
UINT8 ScrollFindTile(UINT16 map_w, unsigned char* map, UINT8 bank, UINT8 tile,
	UINT8 start_x, UINT8 start_y, UINT8 w, UINT8 h,
	UINT16* x, UINT16* y);
	
void EditCollision(UINT8 tile, UINT8 col);
void EditCollisionDown(UINT8 tile, UINT8 col);

extern UINT8 ZGB_Parallax;
void Set_Parallax(unsigned char *tiles, UINT8 bank);
void Move_Parallax();

#endif