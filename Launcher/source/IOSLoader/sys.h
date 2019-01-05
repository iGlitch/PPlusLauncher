#ifndef _SYSCHECKS_H_
#define _SYSCHECKS_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HW_AHBPROT              ((vu32*)0xCD800064)
static s32 iosVersion;

void systest();

bool IsDolphin();
bool AHBRPOT_Patched();
bool IsFromHBC();
bool _isNetworkAvailable();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif