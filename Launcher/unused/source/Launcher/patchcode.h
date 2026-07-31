
#ifndef __PATCHCODE_H__
#define __PATCHCODE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern u32 hooktype;
extern u8 configbytes[2];
extern u8 menuhook;
extern const u32 viwiihooks[4];
extern const u32 kpadhooks[4];
extern const u32 joypadhooks[4];
extern const u32 gxdrawhooks[4];
extern const u32 gxflushhooks[4];
extern const u32 ossleepthreadhooks[4];
extern const u32 axnextframehooks[4];
extern const u32 wpadbuttonsdownhooks[4];
extern const u32 wpadbuttonsdown2hooks[4];

bool dogamehooks(void *addr, u32 len, bool channel);
bool domenuhooks(void *addr, u32 len);
void patchmenu(void *addr, u32 len, int patchnum);
void langpatcher(void *addr, u32 len);
void vidolpatcher(void *addr, u32 len);
void PatchCountryStrings(void *Address, int Size);
void PatchAspectRatio(void *addr, u32 len, u8 aspect);

#ifdef __cplusplus
}
#endif

#endif // __PATCHCODE_H__
