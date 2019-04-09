
#include "BankManager.h"
//Carillon Player Vars
UINT8 ZGB_CPBank = 0; 
UINT8 ZGB_CPSong = 0;
UINT8 ZGB_CP_ON;


//Keep RAM c7c0 - c7ef for player variables (it works!)
UINT8 __at (0xc7c0) CP_VARS[32] = {
	1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,
	1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4
};

void CP_LoadMusic(UINT8 bank, UINT8 song){ 
	ZGB_CP_ON = 1;
	ZGB_CPBank = bank;
	ZGB_CPSong = song;
	PUSH_BANK(ZGB_CPBank);
	__asm
	push	BC
	call	0x4000				//Init player
	pop		BC
	push	BC
	call	0x4003				//Start music playing
	pop		BC
	LDA		HL,2(SP)			// Get second parameter (song)
	LD		A,(_ZGB_CPSong)		// Call SongSelect AFTER MusicStart!
	call	0x400c				// (Not needed if SongNumber = 0)
	__endasm;
	POP_BANK;
}
void CP_UpdateMusic(){
	PUSH_BANK(ZGB_CPBank);
	__asm
	call	0x4100
	__endasm;
	POP_BANK;
}
void CP_StopMusic(){
	if (ZGB_CP_ON == 1){
		PUSH_BANK(ZGB_CPBank);
		__asm
		call	0x4006
		__endasm;
		ZGB_CPBank = 0;
		ZGB_CP_ON = 0;
		POP_BANK;
	}
}

	