#include "Scroll.h"
#include "Sprite.h"
#include "SpriteManager.h"
#include "BankManager.h"
#include "Math.h"


#define SCREEN_TILES_W       20 // 160 >> 3 = 20
#define SCREEN_TILES_H       18 // 144 >> 3 = 18
#define SCREEN_PAD_LEFT   1
#define SCREEN_PAD_RIGHT  2
#define SCREEN_PAD_TOP    1
#define SCREEN_PAD_BOTTOM 2

#define SCREEN_TILE_REFRES_W (SCREEN_TILES_W + SCREEN_PAD_LEFT + SCREEN_PAD_RIGHT)
#define SCREEN_TILE_REFRES_H (SCREEN_TILES_H + SCREEN_PAD_TOP  + SCREEN_PAD_BOTTOM)

#define TOP_MOVEMENT_LIMIT 40u
#define BOTTOM_MOVEMENT_LIMIT 90u

//To be defined on the main app
UINT8 GetTileReplacement(UINT8* tile_ptr, UINT8* tile);

unsigned char* scroll_map = 0;
unsigned char* scroll_cmap = 0;
INT16 scroll_x;
INT16 scroll_y;
UINT16 scroll_w;
UINT16 scroll_h;
UINT16 scroll_tiles_w;
UINT16 scroll_tiles_h;
struct Sprite* scroll_target = 0;
INT16 scroll_target_offset_x = 0;
INT16 scroll_target_offset_y = 0;
UINT8 scroll_collisions[128];
UINT8 scroll_collisions_down[128];
UINT8 scroll_tile_info[256];
UINT8 scroll_bank;
UINT8 scroll_offset_x = 0;
UINT8 scroll_offset_y = 0;

INT16 pending_h_x, pending_h_y;
UINT8 pending_h_i;
unsigned char* pending_h_map = 0;
unsigned char* pending_w_map = 0;
#ifdef CGB
unsigned char* pending_h_cmap = 0;
unsigned char* pending_w_cmap = 0;
#endif
INT16 pending_w_x, pending_w_y;
UINT8 pending_w_i;

extern UINT8 vbl_count;
UINT8 current_vbl_count;


void ZGB_SET_TILE(UINT8 x, UINT8 y, UINT8 t){
	x; y; t;
__asm	
	LDA		HL,3(SP)	// Skip return address
	
	DEC 	HL
	LD		D,#0
	LD		B,#0
	LD		E,(HL)		// E = First	argument(X)
	INC		HL
	LD		A,(HL+)		// A = Second argument (Y)
	; Multiply Y*32
	RLA					// Y*2 
	RLA					// Y*4
	RLA					// Y*8
	LD		C,A			// BC = Y
	LD		A,(HL)		// A = Third argument (TILE)
	LD  	HL,#0x9800	// MAP address to HL
	ADD		HL,BC		// Add BC to 9800
	ADD		HL,BC		// Add BC 
	ADD		HL,BC		// Add BC 
	ADD		HL,BC		// Add BC = + 32*Y
	ADD		HL,DE		// Add DE to 9800 = + 32*Y+X
	
	LD		B,A		
//while 0xff41 & 02 != 0 (cannot write)
	ld	de,#0xff41
1$:
	ld	a,(de)
	and	a, #0x02
	jr	NZ,1$
	
	LD  	(HL),B		// Tile Number to map address
//Check again stat is 0 or 1
	ld	a,(de)
	and	a, #0x02
	jr	NZ,1$
	ret
	
__endasm;
}

void UPDATE_TILE(INT16 x, INT16 y, UINT8* t, UINT8* c) {
	UINT8 replacement = *t;
	UINT8 i;
	struct Sprite* s = 0;
	UINT8 type = 255u;
	UINT8 id = 0u;
	UINT16 tmp_y;
	
	#ifdef CGB
	UINT8 color_map = *c;
	#else
	c;
	#endif

	if(x < 0 || y < 0 || U_LESS_THAN(scroll_tiles_w - 1, x) || U_LESS_THAN(scroll_tiles_h - 1, y)) {
		replacement = 0;
	} else {
		type = GetTileReplacement(t, &replacement);
		if(type != 255u) {
			tmp_y = y << 8;
			id = (0x00FF & x) | ((0xFF00 & tmp_y)); // (y >> 3) << 8 == y << 5
			for(i = 0u; i != sprite_manager_updatables[0]; ++i) {
				s = sprite_manager_sprites[sprite_manager_updatables[i + 1]];
				if((s->unique_id == id) && ((UINT16)s->type == (UINT16)type)) {
					break;
				}
			}

			if(i == sprite_manager_updatables[0]) {
				s = SpriteManagerAdd(type, x << 3, (y - 1) << 3);
				if(s) {
					s->unique_id = id;
				}
			}
		}
	}
		
	x = 0x1F & (x + scroll_offset_x);
	y = 0x1F & (y + scroll_offset_y);
	ZGB_SET_TILE( x, y, replacement);
	#ifdef CGB
	VBK_REG = 1;
		ZGB_SET_TILE( x, y, color_map);
	VBK_REG = 0;
	#endif
}

void ZInitScrollTilesColor(UINT8 first_tile, UINT8 n_tiles, UINT8* tile_data, UINT8 tile_bank, UINT8* palette_entries) {
	UINT8 i;

	PUSH_BANK(tile_bank);
	set_bkg_data(first_tile, n_tiles, tile_data);
	for(i = first_tile; i < first_tile + n_tiles; ++i) {
		scroll_tile_info[i] = palette_entries ? palette_entries[i] : 0;
	}
	POP_BANK;
}

void InitWindow(UINT8 x, UINT8 y, UINT8 w, UINT8 h, UINT8* map, UINT8 bank, UINT8* cmap) {
	cmap;
	PUSH_BANK(bank);
	set_win_tiles(x, y, w, h, map);
	
	#ifdef CGB
	if(cmap) {
		VBK_REG = 1;
			set_win_tiles(x, y, w, h, cmap);
		VBK_REG = 0;
	}
	#endif

	POP_BANK;
}

INT8 scroll_h_border = 0;
UINT8 clamp_enabled = 1;
void ClampScrollLimits(UINT16* x, UINT16* y) {
	if(clamp_enabled) {
		if(U_LESS_THAN(*x, 0u)) {
			*x = 0u;		
		}
		if(*x > (scroll_w - SCREENWIDTH)) {
			*x = (scroll_w - SCREENWIDTH);
		}
		if(U_LESS_THAN(*y, 0u)) {
			*y = 0u;		
		}
		if(*y > (scroll_h - SCREENHEIGHT + scroll_h_border)) {
			*y = (scroll_h - SCREENHEIGHT + scroll_h_border);
		}
	}
}

void ScrollSetMap(UINT16 map_w, UINT16 map_h, unsigned char* map, UINT8 bank, unsigned char* cmap) {
	scroll_tiles_w = map_w;
	scroll_tiles_h = map_h;
	scroll_map = map;
	#ifdef CGB
	scroll_cmap = cmap;
	#endif
	scroll_x = 0;
	scroll_y = 0;
	scroll_w = map_w << 3;
	scroll_h = map_h << 3;
	scroll_bank = bank;
	if(scroll_target) {
		scroll_x = scroll_target->x - (SCREENWIDTH >> 1);
		scroll_y = scroll_target->y - BOTTOM_MOVEMENT_LIMIT; //Move the camera to its bottom limit
		ClampScrollLimits(&scroll_x, &scroll_y);
	}
	pending_h_i = 0;
	pending_w_i = 0;
}

void InitScroll(UINT16 map_w, UINT16 map_h, unsigned char* map, const UINT8* coll_list, const UINT8* coll_list_down, UINT8 bank, unsigned char* color_map) {
	UINT8 i;
	INT16 y;
	
	ScrollSetMap(map_w, map_h, map, bank, color_map);

	for(i = 0u; i != 128; ++i) {
		scroll_collisions[i] = 0u;
		scroll_collisions_down[i] = 0u;
	}
	if(coll_list) {
		for(i = 0u; coll_list[i] != 0u; ++i) {
			scroll_collisions[coll_list[i]] = 1u;
		}
	}
	if(coll_list_down) {
		for(i = 0u; coll_list_down[i] != 0u; ++i) {
			scroll_collisions_down[coll_list_down[i]] = 1u;
		}
	}

	//Change bank now, after copying the collision array (it can be in a different bank)
	PUSH_BANK(bank);
	y = scroll_y >> 3;
	for(i = 0u; i != (SCREEN_TILE_REFRES_H) && y != scroll_h; ++i, y ++) {
		ScrollUpdateRow((scroll_x >> 3) - SCREEN_PAD_LEFT,  y - SCREEN_PAD_TOP);
	}
	POP_BANK;
}

void ScrollUpdateRowR() {
	UINT8 i = 0u;
	
	for(i = 0u; i != 5 && pending_w_i != 0; ++i, -- pending_w_i)  {
		#ifdef CGB
		UPDATE_TILE(pending_w_x ++, pending_w_y, pending_w_map ++, pending_w_cmap++);
		#else
		UPDATE_TILE(pending_w_x ++, pending_w_y, pending_w_map ++,0);
		#endif
	}
}

void ScrollUpdateRowWithDelay(INT16 x, INT16 y) {
	while(pending_w_i) {
		ScrollUpdateRowR();
	}

	pending_w_x = x;
	pending_w_y = y;
	pending_w_i = SCREEN_TILE_REFRES_W;
	pending_w_map = scroll_map + scroll_tiles_w * y + x;

	#ifdef CGB
	pending_w_cmap = scroll_cmap + scroll_tiles_w * y + x;
	#endif
}

void ScrollUpdateRow(INT16 x, INT16 y) {
	UINT8 i = 0u;
	unsigned char* map = scroll_map + scroll_tiles_w * y + x;

	#ifdef CGB
	unsigned char* cmap = scroll_cmap + scroll_tiles_w * y + x;
	#endif

	PUSH_BANK(scroll_bank);
	for(i = 0u; i != SCREEN_TILE_REFRES_W; ++i) {
		#ifdef CGB
		UPDATE_TILE(x + i, y, map ++, cmap ++);
		#else
		UPDATE_TILE(x + i, y, map ++, 0);
		#endif
	}
	POP_BANK;
}

void ScrollUpdateColumnR() {
	UINT8 i = 0u;

	for(i = 0u; i != 5 && pending_h_i != 0; ++i, pending_h_i --) {
		#ifdef CGB
		UPDATE_TILE(pending_h_x, pending_h_y ++, pending_h_map, pending_h_cmap);
		pending_h_map += scroll_tiles_w;
		pending_h_cmap += scroll_tiles_w;
		#else
		UPDATE_TILE(pending_h_x, pending_h_y ++, pending_h_map, 0);
		pending_h_map += scroll_tiles_w;
		#endif
	}
}

void ScrollUpdateColumnWithDelay(INT16 x, INT16 y) {
	while(pending_h_i) {
		ScrollUpdateColumnR();
	}

	pending_h_x = x;
	pending_h_y = y;
	pending_h_i = SCREEN_TILE_REFRES_H;
	pending_h_map = scroll_map + scroll_tiles_w * y + x;

	#ifdef CGB
	pending_h_cmap = scroll_cmap + scroll_tiles_w * y + x;
	#endif
}

void ScrollUpdateColumn(INT16 x, INT16 y) {
	UINT8 i = 0u;
	unsigned char* map = &scroll_map[scroll_tiles_w * y + x];
	#ifdef CGB
	unsigned char* cmap = &scroll_cmap[scroll_tiles_w * y + x];
	#endif
	
	PUSH_BANK(scroll_bank);
	for(i = 0u; i != SCREEN_TILE_REFRES_H; ++i) {
		#ifdef CGB
		UPDATE_TILE(x, y + i, map, cmap);
		map += scroll_tiles_w;
		cmap += scroll_tiles_w;
		#else
		UPDATE_TILE(x, y + i, map, 0);
		map += scroll_tiles_w;
		#endif
	}
	POP_BANK;
}

void RefreshScroll() {
	UINT16 ny = scroll_y;

	if(scroll_target) {
		if(U_LESS_THAN(BOTTOM_MOVEMENT_LIMIT, scroll_target->y - scroll_y)) {
			ny = scroll_target->y - BOTTOM_MOVEMENT_LIMIT;
		} else if(U_LESS_THAN(scroll_target->y - scroll_y, TOP_MOVEMENT_LIMIT)) {
			ny = scroll_target->y - TOP_MOVEMENT_LIMIT;
		}

		MoveScroll(scroll_target->x - (SCREENWIDTH >> 1), ny);
	}
}

void MoveScroll(INT16 x, INT16 y) {
	INT16 current_column, new_column, current_row, new_row;
	
	PUSH_BANK(scroll_bank);
	ClampScrollLimits(&x, &y);

	current_column = scroll_x >> 3;
	new_column     = x >> 3;
	current_row    = scroll_y >> 3;
	new_row        = y >> 3;

	if(current_column != new_column) {
		if(new_column > current_column) {
			ScrollUpdateColumnWithDelay(new_column - SCREEN_PAD_LEFT + SCREEN_TILE_REFRES_W - 1, new_row - SCREEN_PAD_TOP);
		} else {
			ScrollUpdateColumnWithDelay(new_column - SCREEN_PAD_LEFT, new_row - SCREEN_PAD_TOP);
		}
	}
	
	if(current_row != new_row) {
		if(new_row > current_row) {
			ScrollUpdateRowWithDelay(new_column - SCREEN_PAD_LEFT, new_row - SCREEN_PAD_TOP + SCREEN_TILE_REFRES_H - 1);
		} else {
			ScrollUpdateRowWithDelay(new_column - SCREEN_PAD_LEFT, new_row - SCREEN_PAD_TOP);
		}
	}

	scroll_x = x;
	scroll_y = y;

	if(pending_w_i) {
		ScrollUpdateRowR();
	}
	if(pending_h_i) {
		ScrollUpdateColumnR();
	}
	POP_BANK;
}

UINT8* GetScrollTilePtr(UINT16 x, UINT16 y) {
	//Ensure you have selected scroll_bank before calling this function
	//And it is returning a pointer so don't swap banks after you get the value
	return scroll_map + (scroll_tiles_w * y + x); //TODO: fix this mult!!
}

UINT8 GetScrollTile(UINT16 x, UINT16 y) {
	UINT8 ret;
	x = x >>3;
	y = y >>3;
	PUSH_BANK(scroll_bank);
		ret = *GetScrollTilePtr(x,y);
	POP_BANK;
	return ret;
}

UINT8 ScrollFindTile(UINT16 map_w, unsigned char* map, UINT8 bank, UINT8 tile,
	UINT8 start_x, UINT8 start_y, UINT8 w, UINT8 h,
	UINT16* x, UINT16* y) {
	UINT16 xt = 0;
	UINT16 yt = 0;
	UINT8 found = 1;

	PUSH_BANK(bank);
	for(xt = start_x; xt != start_x + w; ++ xt) {
		for(yt = start_y; yt != start_y + h; ++ yt) {
			if(map[map_w * yt + xt] == (UINT16)tile) { //That cast over there is mandatory and gave me a lot of headaches
				goto done;
			}
		}
	}
	found = 0;

done:
	POP_BANK;
	*x = xt;
	*y = yt;

	return found;
}


void EditCollision(UINT8 tile, UINT8 col){
	scroll_collisions[tile] = col;
}
void EditCollisionDown(UINT8 tile, UINT8 col){
	scroll_collisions_down[tile] = col;
}

UINT8 ZGB_Parallax;
#ifdef CGB
UINT8 __at(0xD000) Rtiles [1024];

extern const UINT16 Parallax_TableX[];
extern const UINT16 Parallax_TableY[];

UINT16 ParaOffsetX;
UINT16 ParaOffsetY;
UINT16 ParaOffset = 0;
UINT8 pbank;
UINT8 Parallax;
UINT8 SX;
UINT8 SY;

UINT8 Parallax_Columns = 4;
//Reserve 1 Kb for the parallax tiles
UINT8 __at(0xD000) Rtiles[1024];
//Store owr tiles address
unsigned char *Para_Tiles = 0;
UINT8 STAT	= 0x41;

void DMA_TRANSFER(UINT16 *source,UINT16 *destination){
	__asm
	PUSH	BC
	
	LDA		HL,7(SP)		// parameters 
	
	LD		D,(HL)			// DE = data destination address
	DEC		HL
	LD		E,(HL)
	
	LD		A,D
	LDH		(0x53),A      	// Load upper byte into destination register
	LD		A,E
	LDH		(0x54),A      	// Load lower byte into destination register

	DEC		HL
	LD		B,(HL)			// BC = data source address
	DEC		HL
	LD		C,(HL)
	
	LD		A,B	
    LDH		(0x51),A		// Load upper byte into source register
	LD		A,C
	LDH		(0x52),A    	// Load lower byte into source register
    
	LD		A,#15			// Number of tiles -1 (4*4)
	LDH		(0x55),A      	// Start transfer
	
	POP		BC
	__endasm;
	source; destination;
}

void CPU_TRANSFER(UINT16 *source,UINT16 *destination){
	__asm
	PUSH	BC
	LDA		HL,7(SP)		
	LD		D,(HL)			// DE = data destination address
	DEC		HL
	LD		E,(HL)
	DEC		HL
	LD		B,(HL)			
	DEC		HL
	LD		C,(HL)
	LD		H,B				// HL = data source address
	LD		L,C	

	LD		A,#4
	LD		(#_Parallax_Columns),A
	
2$:	
	LD		C,#7
	
1$:
    LD		A,(HLI)     //  Load source value into A increment pointer
    LD		(DE),A      //  Store at destination
    INC		E          	//  DE=DE+1
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E          	
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E          
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E          
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E      
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E      
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E      
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E        	// 8	
    
	DEC		C			// 8*7 = 56
	JR		NZ,1$  
	
    LD		A,(HLI)     // do the last 8 to increment DE
    LD		(DE),A    
    INC		E          
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E          	
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E          
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E          
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E      
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E       
    LD		A,(HLI)    
    LD		(DE),A      
    INC		DE     		// increment DE
    LD		A,(HLI)    
    LD		(DE),A      
    INC		E     		
	
	LD		B,#0
	LD		C,#64
	ADD		HL,BC		// Jump to next column
	
	LD		A,(#_Parallax_Columns)	// 4 columns
	DEC		A
	LD		(#_Parallax_Columns),A
	JR		NZ,2$		// start next column
	
	POP		BC	
	__endasm;
	source; destination;
}

#endif

void Update_Parallax(){
	#ifdef CGB
	//So.. This ensures the data to be copied well, but it has 1 pixel delay.
	//It's better a delay, than garbage on the top of the screen I guess.
	if (Parallax == 2) {
		DMA_TRANSFER((UINT16*)0xD000,(UINT16*)0x8800); //Tile 129
	}
	#endif
}

void Set_Parallax(unsigned char *tiles, UINT8 bank){
	#ifdef CGB
	ParaOffsetX = 0;
	ParaOffsetY = 0;
	Parallax = 0;
	PUSH_BANK(bank);
	Para_Tiles = tiles;
	//Copy Tiles to Ram 
	CPU_TRANSFER(Para_Tiles+ParaOffsetX+ParaOffsetY,(UINT16*)0xD000); 
	pbank = bank;
	//Update VRAM
	//add_VBL(Update_Parallax);
	POP_BANK;
	#endif
}

void Move_Parallax(){
	#ifdef CGB
	if (Parallax == 2){
		PUSH_BANK(1);
		SX = SCX_REG;
		ParaOffsetX = Parallax_TableX[SX]; 
		SY = SCY_REG;
		ParaOffsetY = Parallax_TableY[SY];
		POP_BANK;
		PUSH_BANK(pbank);
		//Prepare data for the DMA (Next frame)
		//This can be done while drawing
		if (ParaOffset == 64) ParaOffset = 0;
		CPU_TRANSFER(Para_Tiles+ParaOffsetX+ParaOffsetY,(UINT16*)0xD000); //Copy Tiles to VRam 
		POP_BANK;
		Parallax = 0;
		ParaOffset+=2;
	}
	Parallax++;
	#endif
}
