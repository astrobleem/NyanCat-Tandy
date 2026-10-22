/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : march & july 2012                                                  *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#ifndef SCREEN_H
#define SCREEN_H

#define ENDL 0xEE

typedef enum
{
    /* order does matter, the first device must be compatible with the other ones */
    vVGAH,
    vVGA,
    vEGAH,
    vEGA,
    vPlantronicsColorPlus,
    vTandy,
    vNULL
} VIDEODEVICE;

typedef struct
{
    unsigned char *SpriteAdr;
    unsigned short MemAdr;
    /* Used only by the Tandy renderer: 1 keeps source size, 3 enlarges it. */
    unsigned char TandyScale;
    /* Tandy-only: keep star/erase writes out of the foreground's union bounds. */
    unsigned char TandyClip;
} SPRITEPOS;

typedef struct SCREEN_
{
    VIDEODEVICE videoDevice;
    unsigned char cpage; /* current page */
    unsigned short vpage; /* byte offset to current screen (0 or pagesize) */
    unsigned short pagesize; /* size of a screen in byte */
    volatile unsigned short needRefresh; /* shared with the IRQ0 handler */
    void (*SetPalette)(const unsigned char *palette, const int nbc);
    void (*SetVideoMode)(const int mode);
    void (*ChangeVideoPage)(struct SCREEN_ *scr);
    void (*EraseScreen)(struct SCREEN_ *scr, unsigned char color);
    void (*PrintSprite)(const struct SCREEN_ *scr, const SPRITEPOS *sprite);
    void (*PrintSpriteTr)(const struct SCREEN_ *scr, const SPRITEPOS *sprite, unsigned char alpha);
} SCREEN;

void Screen_SetPalette16c(const unsigned char *palette, const int nbc);
void Screen_SetPalette256c(const unsigned char *palette, const int nbc);
void Screen_SetVideoMode(const int mode);
void Screen_SetGraphicMode(SCREEN *scr);
void Screen_ChangeVideoPage(SCREEN *scr);
void Screen_TandyReleaseCache(void);
void Screen_TandySetStarClip(unsigned short left, unsigned short top,
                             unsigned short right, unsigned short bottom);
#ifdef TANDY_CACHE_TEST
int Screen_TandyCacheTestSynthetic(void);
int Screen_TandyCacheTestAssetColors(void);
int Screen_TandyCacheValidateAsset(const unsigned char *source);
int Screen_TandyCacheTestScale3(const unsigned char *source);
int Screen_TandyCacheTestFallback(void);
int Screen_TandyCacheTestClipPair(void);
int Screen_TandyCacheTestStarOverlap(const SPRITEPOS *frames,
                                     int frameCount, int frameStride);
void Screen_TandyCacheTestProgress(int phase, int frame);
#endif

SCREEN *CreateScreen(VIDEODEVICE device);
SCREEN *DeleteScreen(SCREEN *scr);

#endif
