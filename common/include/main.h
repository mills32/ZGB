#ifndef MAIN_H
#define MAIN_H

#include <gb/gb.h>

#include "Sprite.h"

typedef void (*Void_Func_Void)();
typedef void (*Void_Func_SpritePtr)(struct Sprite*);

#define DECLARE_STATE(STATE_IDX)   extern UINT8 bank_##STATE_IDX;  void Start_##STATE_IDX(); void Update_##STATE_IDX()
extern UINT8 stateBanks[];
extern Void_Func_Void startFuncs[];
extern Void_Func_Void updateFuncs[];

#define DECLARE_SPRITE(SPRITE_IDX) extern UINT8 bank_##SPRITE_IDX; void Start_##SPRITE_IDX(); void Update_##SPRITE_IDX(); void Destroy_##SPRITE_IDX()
extern UINT8 spriteBanks[];
extern Void_Func_Void spriteStartFuncs[];
extern Void_Func_Void spriteUpdateFuncs[];
extern Void_Func_Void spriteDestroyFuncs[];

extern UINT8* spriteDatas[];
extern UINT8 spriteDataBanks[];
extern FrameSize spriteFrameSizes[];
extern UINT8 spriteNumFrames[];
extern UINT8 spriteIdxs[];
extern UINT8* spritePalDatas[];


extern UINT8 current_state;
void SetState(UINT8 state);
extern UINT8 delta_time;

#ifdef CGB
extern UINT8 pbank; //Parallax bank
typedef enum {
	BG_PALETTE,
	SPRITES_PALETTE
} PALETTE_TYPE;
#endif
void SetPalette(UWORD *bpal, UWORD *spal, UINT8 bbank);

#endif