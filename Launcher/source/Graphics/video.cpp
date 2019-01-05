/****************************************************************************
* libwiigui Template
* Tantric 2009
*
* video.cpp
* Video routines
***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include "../IOSLoader/sys.h"

#define DEFAULT_FIFO_SIZE 256 * 1024
static unsigned int *xfb[2];
static int whichfb = 0; // Switch
static GXRModeObj *vmode; // Menu video mode
static void *gp_fifo;
static Mtx GXmodelView2D;
int screenheight;
int screenwidth;
//u32 FrameTimer = 0;

int getScreenHeight()
{
	return screenheight;
}

int getScreenWidth()
{
	return screenwidth;
}

/****************************************************************************
* ResetVideo_Menu
*
* Reset the video/rendering mode for the menu
****************************************************************************/
void
ResetVideo_Menu()
{
	Mtx44 p;
	f32 yscale;
	u32 xfbHeight;

	VIDEO_Configure(vmode);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	else
	while (VIDEO_GetNextField())
		VIDEO_WaitVSync();

	// clears the bg to color and clears the z buffer
	GXColor background = { 0, 0, 0, 255 };
	GX_SetCopyClear(background, 0x00ffffff);

	yscale = GX_GetYScaleFactor(vmode->efbHeight, vmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0, 0, vmode->fbWidth, vmode->efbHeight);
	GX_SetDispCopySrc(0, 0, vmode->fbWidth, vmode->efbHeight);
	GX_SetDispCopyDst(vmode->fbWidth, xfbHeight);
	GX_SetCopyFilter(vmode->aa, vmode->sample_pattern, GX_TRUE, vmode->vfilter);
	GX_SetFieldMode(vmode->field_rendering, ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

	if (vmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_ClearVtxDesc();
	GX_InvVtxCache();
	GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply(GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	guOrtho(p, 0, 479, 0, 639, 0, 300);
	GX_LoadProjectionMtx(p, GX_ORTHOGRAPHIC);

	GX_SetViewport(0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
}

void Menu_Render()
{
	GX_DrawDone();
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_CopyDisp(MEM_K1_TO_K0(xfb[whichfb]), GX_TRUE);
	DCFlushRange(xfb[whichfb], 2 * vmode->fbWidth * vmode->xfbHeight);

	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	whichfb ^= 1; // flip framebuffer
	GX_InvalidateTexAll();

	//FrameTimer++;
}

void
InitVideo()
{
	///VIDEO_Init();

	bool m_wide = CONF_GetAspectRatio() == CONF_ASPECT_16_9;
	vmode = VIDEO_GetPreferredMode(NULL);

	
	u32 type = CONF_GetVideo();
	bool m_50hz = false;
	if (vmode == &TVPal528IntDf)
	{
		vmode = &TVPal576IntDfScale;
		m_50hz = true;
	}
	vmode->viWidth = 640; //m_wide ? 678 : 672;
	
	//CONF_VIDEO_NTSC and CONF_VIDEO_MPAL and m_rmode TVEurgb60Hz480IntDf are the same max height and width.
	/*
	if (type == CONF_VIDEO_PAL && vmode != &TVEurgb60Hz480IntDf)
	{
		vmode->viHeight = VI_MAX_HEIGHT_PAL;
		vmode->viXOrigin = (VI_MAX_WIDTH_PAL - vmode->viWidth) / 2;
		vmode->viYOrigin = (VI_MAX_HEIGHT_PAL - vmode->viHeight) / 2;
	}
	else
	{
		vmode->viHeight = VI_MAX_HEIGHT_NTSC;
		vmode->viXOrigin = (VI_MAX_WIDTH_NTSC - vmode->viWidth) / 2;
		vmode->viYOrigin = (VI_MAX_HEIGHT_NTSC - vmode->viHeight) / 2;
	}

	s8 hoffset = 0;  //Use horizontal offset set in wii menu.
	if (CONF_GetDisplayOffsetH(&hoffset) == 0)
		vmode->viXOrigin += hoffset;*/


	screenheight = vmode->viHeight;
	screenwidth = vmode->viWidth;

	// Allocate the video buffers
	xfb[0] = (u32 *)SYS_AllocateFramebuffer(vmode);
	xfb[1] = (u32 *)SYS_AllocateFramebuffer(vmode);
	gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
	GXColor background = { 0, 0, 0, 0xff };
	GX_SetCopyClear(background, 0x00ffffff);
	GX_SetDispCopyGamma(GX_GM_1_0);
	GX_SetCullMode(GX_CULL_NONE);


	// A console is always useful while debugging
	//console_init(xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2);

	// Clear framebuffers etc.
	//VIDEO_SetNextFramebuffer(xfb[0]);


	// Initialize GX

	ResetVideo_Menu();
	VIDEO_Configure(vmode);
	VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);
	Menu_Render();
	Menu_Render();
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	// Finally, the video is up and ready for use :)
}

/****************************************************************************
* StopGX
*
* Stops GX (when exiting)
***************************************************************************/
void StopGX()
{
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();

	GX_DrawDone();
	GX_Flush();
	GX_AbortFrame();
	/*free(MEM_K1_TO_K0(xfb[0]));
	xfb[0] = NULL;
	free(MEM_K1_TO_K0(xfb[1]));
	xfb[1] = NULL;*/
	free(gp_fifo);
	gp_fifo = NULL;


}



/****************************************************************************
* Menu_DrawImg
*
* Draws the specified image on screen using GX
***************************************************************************/

void drawTexturedBox(GXTexObj *texObj, f32 xpos, f32 ypos, u16 width, u16 height, u8 red, u8 green, u8 blue, u8 alpha)
{
	f32 scaleX = 1.0f;
	f32 scaleY = 1.0f;
	f32 degrees = 0.0f;
	GX_LoadTexObj(texObj, GX_TEXMAP0);
	GX_InvalidateTexAll();

	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	Mtx m, m1, m2, mv;
	width >>= 1;
	height >>= 1;

	guMtxIdentity(m1);
	guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0);
	guVector axis = (guVector) { 0, 0, 1 };
	guMtxRotAxisDeg(m2, &axis, degrees);
	guMtxConcat(m2, m1, m);

	guMtxTransApply(m, m, xpos + width, ypos + height, 0);
	guMtxConcat(GXmodelView2D, m, mv);
	GX_LoadPosMtxImm(mv, GX_PNMTX0);

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position3f32(-width, -height, 0);
	GX_Color4u8(red, green, blue, alpha);
	GX_TexCoord2f32(0, 0);

	GX_Position3f32(width, -height, 0);
	GX_Color4u8(red, green, blue, alpha);
	GX_TexCoord2f32(1, 0);

	GX_Position3f32(width, height, 0);
	GX_Color4u8(red, green, blue, alpha);
	GX_TexCoord2f32(1, 1);

	GX_Position3f32(-width, height, 0);
	GX_Color4u8(red, green, blue, alpha);
	GX_TexCoord2f32(0, 1);
	GX_End();
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
}

void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[],
	f32 degrees, f32 scaleX, f32 scaleY, u8 alpha)
{
	if (data == NULL)
		return;

	GXTexObj texObj;

	GX_InitTexObj(&texObj, data, width, height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);
	GX_InvalidateTexAll();

	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	Mtx m, m1, m2, mv;
	width >>= 1;
	height >>= 1;

	guMtxIdentity(m1);
	guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0);
	guVector axis = (guVector) { 0, 0, 1 };
	guMtxRotAxisDeg(m2, &axis, degrees);
	guMtxConcat(m2, m1, m);

	guMtxTransApply(m, m, xpos + width, ypos + height, 0);
	guMtxConcat(GXmodelView2D, m, mv);
	GX_LoadPosMtxImm(mv, GX_PNMTX0);

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position3f32(-width, -height, 0);
	GX_Color4u8(0xFF, 0xFF, 0xFF, alpha);
	GX_TexCoord2f32(0, 0);

	GX_Position3f32(width, -height, 0);
	GX_Color4u8(0xFF, 0xFF, 0xFF, alpha);
	GX_TexCoord2f32(1, 0);

	GX_Position3f32(width, height, 0);
	GX_Color4u8(0xFF, 0xFF, 0xFF, alpha);
	GX_TexCoord2f32(1, 1);

	GX_Position3f32(-width, height, 0);
	GX_Color4u8(0xFF, 0xFF, 0xFF, alpha);
	GX_TexCoord2f32(0, 1);
	GX_End();
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
}

/****************************************************************************
* Menu_DrawRectangle
*
* Draws a rectangle at the specified coordinates using GX
***************************************************************************/

void drawBox(f32 x, f32 y, f32 width, f32 height, u8 red, u8 green, u8 blue, u8 alpha)
{
	long n = 4;
	f32 x2 = x + width;
	f32 y2 = y + height;
	guVector v[] = { { x, y, 0.0f }, { x2, y, 0.0f }, { x2, y2, 0.0f }, { x, y2, 0.0f }, { x, y, 0.0f } };
	u8 fmt = GX_TRIANGLEFAN;


	GX_Begin(fmt, GX_VTXFMT0, n);
	for (long i = 0; i < n; ++i)
	{
		GX_Position3f32(v[i].x, v[i].y, v[i].z);
		GX_Color4u8(red, green, blue, alpha);
	}
	GX_End();
}

void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, GXColor color, u8 filled)
{
	long n = 4;
	f32 x2 = x + width;
	f32 y2 = y + height;
	guVector v[] = { { x, y, 0.0f }, { x2, y, 0.0f }, { x2, y2, 0.0f }, { x, y2, 0.0f }, { x, y, 0.0f } };
	u8 fmt = GX_TRIANGLEFAN;

	if (!filled)
	{
		fmt = GX_LINESTRIP;
		n = 5;
	}

	GX_Begin(fmt, GX_VTXFMT0, n);
	for (long i = 0; i < n; ++i)
	{
		GX_Position3f32(v[i].x, v[i].y, v[i].z);
		GX_Color4u8(color.r, color.g, color.b, color.a);
	}
	GX_End();
}

void drawCircle(f32 x, f32 y, u32 radius, u8 red, u8 green, u8 blue, u8 alpha, u8 red2, u8 green2, u8 blue2, u8 alpha2)
{

	u32 sides = (u32)radius;
	f32 angle_part = 2.0 * M_PI / ((f32)sides);

	GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, sides + 2);

	GX_Position3f32(x, y, 0.0);
	GX_Color4u8(red, green, blue, alpha);

	for (u32 i = 0; i <= sides; i++) {
		f32 rcos = cos(angle_part * ((f32)i));
		f32 rsin = sin(angle_part * ((f32)i));

		GX_Position3f32(x + rcos * radius, y + rsin * radius, 0.0);
		GX_Color4u8(red2, green2, blue2, alpha2);
	}

	GX_End();
}