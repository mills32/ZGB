#include "Sound.h"

#include <stdarg.h>
#include "BankManager.h"

#include "carillon_player.h"
void CP_UpdateMusic(); 
void CP_Mute_Chan(UINT8 chan);
void CP_Reset_Chan(UINT8 chan); 
extern UINT8 CP_Muted_Chan;

const UINT8 FX_REG_SIZES[] = {5, 4, 5, 4, 3};
const UINT16 FX_ADDRESS[] = {0xFF10, 0xFF16, 0xFF1A, 0xFF20, 0xFF24};

extern UINT8 music_mute_frames;

void PlayFx(SOUND_CHANNEL channel, UINT8 mute_frames, ...) {
	UINT8 i;
	UINT8* reg = (UINT8*)FX_ADDRESS[channel];
	va_list list;

	CP_Muted_Chan = channel;
	PUSH_BANK(1);
	CP_Mute_Chan(CP_Muted_Chan);
	CP_Reset_Chan(CP_Muted_Chan);
	POP_BANK;	
	
	va_start(list, mute_frames);
	for(i = 0; i < FX_REG_SIZES[channel]; ++i, ++reg) {
		*reg = va_arg(list, INT16);
	}
	va_end(list);
	

	music_mute_frames = mute_frames;
}