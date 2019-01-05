#ifndef SFX_H
#define SFX_H


enum ESFXType
{
	SFX_SELECT = -1,
	SFX_CONFIRM = 0,
	SFX_BACK = 1
};

extern const u8		sfx_select_pcm[];
extern const u32	sfx_select_pcm_size;

extern const u8		sfx_confirm_pcm[];
extern const u32	sfx_confirm_pcm_size;

extern const u8		sfx_back_pcm[];
extern const u32	sfx_back_pcm_size;

void SFX_Init();
void playSFX(ESFXType sfx);


#endif