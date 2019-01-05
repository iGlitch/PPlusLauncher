#ifndef _DISC_H_
#define _DISC_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <ogcsys.h>

	enum discTypes
	{
		IS_NGC_DISC = 0,
		IS_WII_DISC,
		IS_UNK_DISC
	};
#define WII_MAGIC 0x5D1C9EA3
#define NGC_MAGIC 0xC2339F3D


	s32 Disc_Open();
	s32 Disc_FindPartition(u32 *outbuf);
	void Disc_SetLowMemPre();
	void Disc_SetLowMem(u32 IOS);
	void Disc_SetTime();
	GXRModeObj *Disc_SelectVMode(u8 videoselected, u32 *rmode_reg);
	void Disc_SetVMode(GXRModeObj *rmode, u32 rmode_reg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
