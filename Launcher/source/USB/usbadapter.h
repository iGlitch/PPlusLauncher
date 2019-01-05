#ifndef _USBADAPTER_H_
#define _USBADAPTER_H_

s32 USBAdapter_Init();
s32 USBAdapter_Open();
s32 USBAdapter_Read();
void USBAdapter_SetPollInBackground(s32 value);
void USBAdapter_Start();
void USBAdapter_ReadBackground();
void * usbAdapterThreadFunction();

void USBAdapter_Stop();

u16 UPAD_ButtonsUp(int pad);
u16 UPAD_ButtonsDown(int pad);
u16 UPAD_ButtonsHeld(int pad);

s8 UPAD_SubStickX(int pad);
s8 UPAD_SubStickY(int pad);

s8 UPAD_StickX(int pad);
s8 UPAD_StickY(int pad);

u8 UPAD_TriggerL(int pad);
u8 UPAD_TriggerR(int pad);

s32 UPAD_ScanPads();
#endif