#include <asndlib.h>
#include <ogc/audio.h>
#include "sfx.h"
const u32	sfx_select_pcm_size; 
const u32	sfx_confirm_pcm_size; 
const u32	sfx_back_pcm_size;
bool sfxEnabled = false;
void SFX_Init()
{
	sfxEnabled = true;
}

void playSFX(ESFXType sfx)
{
	if (!sfxEnabled)
		return;
	switch (sfx)
	{
	case SFX_SELECT:
		ASND_SetVoice(1, VOICE_STEREO_16BIT_LE, 48000, 0, sfx_select_pcm, sfx_select_pcm_size, 255, 255, NULL);
		break;
	case SFX_BACK:
		ASND_SetVoice(1, VOICE_STEREO_16BIT_LE, 48000, 0, sfx_back_pcm, sfx_back_pcm_size, 255, 255, NULL);
		break;
	case SFX_CONFIRM:
		ASND_SetVoice(1, VOICE_STEREO_16BIT_LE, 48000, 0, sfx_confirm_pcm, sfx_confirm_pcm_size, 255, 255, NULL);
		break;
	}
}