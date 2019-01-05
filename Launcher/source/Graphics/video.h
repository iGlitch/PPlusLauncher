/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * video.h
 * Video routines
 ***************************************************************************/

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <ogcsys.h>

void InitVideo ();
void StopGX();
void ResetVideo_Menu();
void Menu_Render();
void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], f32 degrees, f32 scaleX, f32 scaleY, u8 alphaF );
void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, GXColor color, u8 filled);
void drawTexturedBox(GXTexObj *texObj, f32 xpos, f32 ypos, u16 width, u16 height, u8 red, u8 green, u8 blue, u8 alpha);
void drawBox(f32 x, f32 y, f32 width, f32 height, u8 red, u8 green, u8 blue, u8 alpha);
void drawCircle(f32 x, f32 y, u32 radius, u8 red, u8 green, u8 blue, u8 alpha, u8 red2, u8 green2, u8 blue2, u8 alpha2);
extern int screenheight;
extern int screenwidth;
extern u32 FrameTimer;

int getScreenHeight();
int getScreenWidth();
#endif
