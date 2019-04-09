#include "main.h"

#include <gb/gb.h> 
#include <string.h>

#include "OAMManager.h"
#include "Scroll.h"
#include "Keys.h"
#include "SpriteManager.h"
#include "BankManager.h"
#include "Fade.h"
#include "Palette.h"

#include "carillon_player.h"

#ifdef CGB
void DMA_TRANSFER(UINT16 *source,UINT16 *destination); 
#endif	

extern UINT8 ZGB_CP_ON;
void CP_UpdateMusic();
void CP_Mute_Chan(UINT8 chan);
void CP_Reset_Chan(UINT8 chan); 
UINT8 CP_Muted_Chan;

extern UINT8 next_state;

UINT8 delta_time;
UINT8 current_state;
UINT8 state_running = 0;

void SetState(UINT8 state) {
	state_running = 0;
	next_state = state;
}

void* last_music = 0;


UINT8 vbl_count;
INT16 old_scroll_x, old_scroll_y;
UINT8 music_mute_frames = 0;
void vbl_update() {
	vbl_count ++;
	#ifdef CGB
	if (ZGB_Parallax == 1){
		DMA_TRANSFER((UINT16*)0xD000,(UINT16*)0x8800);//tile 128
	}
	#endif
	//Instead of assigning scroll_y to SCX_REG I do a small interpolation that smooths the scroll transition giving the
	//Illusion of a better frame rate
	if(old_scroll_x < scroll_x)
		old_scroll_x += (scroll_x - old_scroll_x + 1) >> 1;
	else if(old_scroll_x > scroll_x)
		old_scroll_x -= (old_scroll_x - scroll_x + 1) >> 1;
	SCX_REG = old_scroll_x + (scroll_offset_x << 3);

	if(old_scroll_y < scroll_y)
		old_scroll_y += (scroll_y - old_scroll_y + 1) >> 1;
	else if(old_scroll_y > scroll_y)
		old_scroll_y -= (old_scroll_y - scroll_y + 1) >> 1;
	SCY_REG = old_scroll_y + (scroll_offset_y << 3);

}

void InitStates();
void InitSprites();

void MusicUpdate() {
	if (ZGB_CP_ON == 1) {
		CP_UpdateMusic();
	}
	if(music_mute_frames != 0){
		PUSH_BANK(1);
		CP_Mute_Chan(CP_Muted_Chan);
		if(music_mute_frames == 1){
			CP_Reset_Chan(CP_Muted_Chan);
		}
		POP_BANK;
		
		music_mute_frames --;
	}
	REFRESH_BANK;
}


extern UWORD ZGB_Fading_BPal[32];
extern UWORD ZGB_Fading_SPal[32];
void SetPalette(UWORD *bpal, UWORD *spal, UINT8 bbank){
	#ifdef CGB
	UINT8 i;
	
	PUSH_BANK(bbank);	
		for(i = 0; i != 32; ++i) ZGB_Fading_BPal[i] = bpal[i];
		for(i = 0; i != 32; ++i) ZGB_Fading_SPal[i] = spal[i];
	POP_BANK;
	
	set_bkg_palette(0, 8, bpal);
	set_sprite_palette(0, 8, spal);	
	#endif
}

UINT16 default_palette[] = {RGB(31, 31, 31), RGB(20, 20, 20), RGB(10, 10, 10), RGB(0, 0, 0)};
void main() {
#ifdef CGB
	cpu_fast();
#endif

	PUSH_BANK(1);
	InitStates();
	InitSprites();
	POP_BANK;

	disable_interrupts();
	add_VBL(vbl_update);
	add_TIM(MusicUpdate);
#ifdef CGB
	TMA_REG = _cpu == CGB_TYPE ? 120U : 0xBCU;
#else
	TMA_REG = 0xBCU;
#endif
	TAC_REG = 0x04U;

	set_interrupts(VBL_IFLAG | TIM_IFLAG);
	enable_interrupts();

	while(1) {
		while (state_running) {
			if(!vbl_count)
				wait_vbl_done();
			delta_time = vbl_count == 1u ? 0u : 1u;
			vbl_count = 0;
			UPDATE_KEYS();
			PUSH_BANK(stateBanks[current_state]);
				updateFuncs[current_state]();
			POP_BANK;
			SpriteManagerUpdate();
		}

		FadeIn();
		DISPLAY_OFF

		last_music = 0;

		last_sprite_loaded = 0;
		SpriteManagerReset();
		state_running = 1;
		current_state = next_state;
		scroll_target = 0;
		
#ifdef CGB
		/*if (_cpu == CGB_TYPE) {
			SetPalette(default_palette, default_palette, 1);
			SetPalette(default_palette, default_palette, 1);
		} else */
#endif
			BGP_REG = OBP0_REG = OBP1_REG = PAL_DEF(0, 1, 2, 3);

		PUSH_BANK(stateBanks[current_state]);
			(startFuncs[current_state])();
		POP_BANK;
		old_scroll_x = scroll_x;
		old_scroll_y = scroll_y;

		if(state_running) {
			DISPLAY_ON;
			FadeOut();
		}
	}
}

