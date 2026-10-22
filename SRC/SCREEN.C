/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : march & july 2012                                                  *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <dos.h> /* MK_FP */
#include <conio.h> /* outportb */
#include "screen.h"

/* Defined by NYANDATA.H through NYANANIM.H in the application and cache test. */
extern unsigned char ARC1_EGA[];
extern unsigned char CAT1_EGA[];

#ifdef __WATCOMC__
#include <memory.h>
#include <malloc.h>
#define outportb outp
#define inportb inp
#else
/* __TURBOC__ */
#include <mem.h>
#include <alloc.h>
#endif

#define lo(value) (unsigned char)( value & 0x00FF )
#define hi(value) (unsigned char)((value & 0xFF00) >> 8)

/*
   http://webpages.charter.net/danrollins/techhelp/0067.HTM
   http://www.faqs.org/faqs/msdos-programmer-faq/part4/section-5.html
*/
VIDEODEVICE Screen_DetectVideo()
{
    union REGS regs;
    VIDEODEVICE retval = vNULL;

    regs.x.ax = 0x1200;
    regs.x.bx = 0x0032;
    int86(0x10, &regs, &regs);
    if (regs.h.al == 0x12)
    {
        retval = vVGAH;
    }
    else
    {
        regs.x.ax = 0x1200;
        regs.x.bx = 0x0010;
        int86(0x10, &regs, &regs);
        if (regs.h.bl < 4)
        {
            if (regs.h.bl == 3)
                retval = vEGAH;
            else
                retval = vEGA;
        }
        else
        {
            retval = vPlantronicsColorPlus;
        }
    }

    return retval;
}

void Screen_SetVideoMode(const int mode)
{
    union REGS regs;

    regs.x.ax = mode;
    int86(0x10, &regs, &regs);
}

/*
   http://www.seasip.info/VintagePC/cga.html
   http://wiki.osdev.org/VGA_Hardware
*/
void Screen_SetGraphicMode(SCREEN *scr)
{
    switch (scr->videoDevice)
    {
    case vTandy:
        /* BIOS mode 9 selects Tandy 320x200, 16-color graphics. */
        Screen_SetVideoMode(0x0009);
        break;

    case vPlantronicsColorPlus:
    {
        Screen_SetVideoMode(0x0004);

        /* Bit 4: 320x200 16-colour mode */
        outportb(0x3dd, 0x10);
        break;
    }

    case vEGA:
        Screen_SetVideoMode(0x000d);
        break;

    case vEGAH:
        Screen_SetVideoMode(0x0010);
        break;

    case vVGAH:
    {
        unsigned char data;

        Screen_SetVideoMode(0x0010);

        /* http://www.phatcode.net/res/224/files/html/ch45/45-04.html */
        /* Modify the sync polarity bits (bits 7 & 6) of the
           Miscellaneous Output register (readable at 0x3CC, writable at
           0x3C2) to select the 400-scan-line vertical scanning rate */
        data = inportb(0x3cc);
        outportb(0x3c2, (data & 0x3f) | 0x40);

        /* Now, tweak the registers needed to convert the vertical
           timings from 350 to 400 scan lines */
        outportb(0x3d4, 0x10); /* Vertical Sync Start register */
        outportb(0x3d5, 0x9c);

        outportb(0x3d4, 0x11); /* Vertical Sync End register */
        outportb(0x3d5, 0x8e);

        outportb(0x3d4, 0x12); /* Vertical Display End register */
        outportb(0x3d5, 0x8f);

        outportb(0x3d4, 0x15); /* Vertical Blank Start register */
        outportb(0x3d5, 0x96);

        outportb(0x3d4, 0x16); /* Vertical Blank End register */
        outportb(0x3d5, 0xb9);
        break;
    }

    case vVGA:
    {
        unsigned char data;

        Screen_SetVideoMode(0x0013);

        /* turn off chain-4 mode */
        outportb(0x03c4, 0x04);
        data = inportb(0x03c5);
        outportb(0x03c5, (data & 0xf7) | 0x04);

        /* turn off doubleword clocking */
        outportb(0x03d4, 0x14);
        data = inportb(0x03d5);
        outportb(0x03d5, data & 0xbf);

        /* turn off word clocking */
        outportb(0x03d4, 0x17);
        data = inportb(0x03d5);
        outportb(0x03d5, data | 0x40);

        break;
    }
    }
}

/* http://en.wikipedia.org/wiki/Talk%3AMode_X */
void Screen_ChangeVideoPage(SCREEN *scr)
{
    /* attente du timer FPS */
    while (scr->needRefresh == 0)
    {
        ;
    }
    scr->needRefresh = 0;

    if (scr->videoDevice == vPlantronicsColorPlus)
        return;

    /* Tandy mode 9 has a single 32 KB display page. */
    if (scr->videoDevice == vTandy)
        return;

    /* wait for display disable */
    while (inportb(0x3da) & 0x01);

    /* switch page */
    outportb(0x3d4, 0x0c);
    outportb(0x3d5, hi(scr->vpage));
    outportb(0x3d4, 0x0d);
    outportb(0x3d5, lo(scr->vpage));

    if (scr->cpage == 0)
    {
        scr->cpage = 1;
        scr->vpage = scr->pagesize;
    }
    else
    {
        scr->cpage = 0;
        scr->vpage = 0;
    }

    /* wait until we are in vertical sync */
    while (!(inportb(0x3da) & 0x08));
}

/****************************************************************************/
/******************************** PALETTE ***********************************/
/****************************************************************************/
/*
   http://webpages.charter.net/danrollins/techhelp/0137.HTM
   http://webpages.charter.net/danrollins/techhelp/0139.HTM
*/
void Screen_SetPalette16c(const unsigned char *palette, const int nbc)
{
    union REGS regs;
    struct SREGS sregs;

    regs.x.ax = 0x1002;
    regs.x.dx = FP_OFF(palette);
    sregs.es = FP_SEG(palette);
    int86x(0x10, &regs, &regs, &sregs);
}

void Screen_SetPalette256c(const unsigned char *palette, const int nbc)
{
    int j;
    outportb(0x03c6, 0xff); /* mask all registers */
    outportb(0x03c8, 0);
    for (j=0; j<nbc*3; j++)
    {
        outportb(0x03c9, palette[j]>>2); /* 6 bit value */
    }
}

/****************************************************************************/
/******************************** EraseScreen *******************************/
/****************************************************************************/

void Screen_EraseScreenCGA(SCREEN *scr, unsigned char color)
{
    int i;
    unsigned char bRG, bBI, pRG=0, pBI=0;
    char far *screenEven  = (char far *)MK_FP(0xb800, 0);
    char far *screenOdd   = (char far *)MK_FP(0xba00, 0);
    char far *screenEven2 = (char far *)MK_FP(0xbc00, 0);
    char far *screenOdd2  = (char far *)MK_FP(0xbe00, 0);

    bRG = (color & 0x0C) >> 2;
    bBI =  color & 0x03;
    for (i=0; i<4; i++)
    {
        unsigned char n = 6-(2*i);
        pRG |= bRG << n;
        pBI |= bBI << n;
    }

    memset(screenEven, pRG, scr->pagesize);
    memset(screenEven2, pBI, scr->pagesize);
    memset(screenOdd, pRG, scr->pagesize);
    memset(screenOdd2, pBI, scr->pagesize);

    scr->needRefresh = 1;
}

/* http://webpages.charter.net/danrollins/techhelp/0089.HTM */
void Screen_EraseScreenEGA(SCREEN *scr, unsigned char color)
{
    int i;
    unsigned char b0, b1, b2, b3;
    unsigned char p0=0, p1=0, p2=0, p3=0;
    char far *screen = (char far *)MK_FP(0xa000, scr->vpage);

    b0 =  color & 0x01;
    b1 = (color & 0x02) >> 1;
    b2 = (color & 0x04) >> 2;
    b3 = (color & 0x08) >> 3;
    for (i=0; i<8; i++)
    {
        unsigned char n = 7-i;
        p3 |= b3 << n;
        p2 |= b2 << n;
        p1 |= b1 << n;
        p0 |= b0 << n;
    }

    outportb(0x03c4, 0x02);
    outportb(0x03c5, 0x01);
    memset(screen, p0, scr->pagesize);
    outportb(0x03c5, 0x02);
    memset(screen, p1, scr->pagesize);
    outportb(0x03c5, 0x04);
    memset(screen, p2, scr->pagesize);
    outportb(0x03c5, 0x08);
    memset(screen, p3, scr->pagesize);

    scr->needRefresh = 1;
}

void Screen_EraseScreenVGA(SCREEN *scr, unsigned char color)
{
    char far *screen = (char far *)MK_FP(0xa000, scr->vpage);

    /* erase 4 planes at once */
    outportb(0x03c4, 0x02);
    outportb(0x03c5, 0x0f);
    memset(screen, color, scr->pagesize);
    scr->needRefresh = 1;
}

/****************************************************************************/
/******************************** PrintSprite *******************************/
/****************************************************************************/

void Screen_PrintSpriteCGA(const SCREEN *scr, const SPRITEPOS *sprite)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0, even = 1;
        unsigned char p0, p1;
        unsigned char *data = sprite->SpriteAdr;
        char far *screenEven  = (char far *)MK_FP(0xb800, sprite->MemAdr);
        char far *screenOdd   = (char far *)MK_FP(0xba00, sprite->MemAdr);
        char far *screenEven2 = (char far *)MK_FP(0xbc00, sprite->MemAdr);
        char far *screenOdd2  = (char far *)MK_FP(0xbe00, sprite->MemAdr);

        while (stop == 0)
        {
            p0 = data[pos++];
            p1 = data[pos++];
            if (even)
            {
                *screenEven = p0;
                screenEven++;
                *screenEven2 = p1;
                screenEven2++;
            }
            else
            {
                *screenOdd = p0;
                screenOdd++;
                *screenOdd2 = p1;
                screenOdd2++;
            }

            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                if (even)
                {
                    even = 0;
                    screenEven += delta;
                    screenEven2 += delta;
                }
                else
                {
                    even = 1;
                    screenOdd += delta;
                    screenOdd2 += delta;
                }
            }
        }
    }
}

void Screen_PrintSpriteTrCGA(const SCREEN *scr, const SPRITEPOS *sprite, unsigned char alpha)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0, even = 1, i;
        unsigned char p0, p1, values[4];
        unsigned char *data = sprite->SpriteAdr;
        char far *screenEven  = (char far *)MK_FP(0xb800, sprite->MemAdr);
        char far *screenOdd   = (char far *)MK_FP(0xba00, sprite->MemAdr);
        char far *screenEven2 = (char far *)MK_FP(0xbc00, sprite->MemAdr);
        char far *screenOdd2  = (char far *)MK_FP(0xbe00, sprite->MemAdr);

        while (stop == 0)
        {
            p0 = data[pos++];
            p1 = data[pos++];

            values[0] = ((p0 & 0xc0) >> 4) | ((p1 & 0xc0) >> 6);
            values[1] = ((p0 & 0x30) >> 2) | ((p1 & 0x30) >> 4);
            values[2] =  (p0 & 0x0c)       | ((p1 & 0x0c) >> 2);
            values[3] = ((p0 & 0x03) << 2) |  (p1 & 0x03);

            /* reading pixels from video memory */
            if (even)
            {
                p0 = *screenEven;
                p1 = *screenEven2;
            }
            else
            {
                p0 = *screenOdd;
                p1 = *screenOdd2;
            }

            /* replace pixels with video memory if needed */
            if (values[0] == alpha)
                values[0] = ((p0 & 0xc0) >> 4) | ((p1 & 0xc0) >> 6);
            if (values[1] == alpha)
                values[1] = ((p0 & 0x30) >> 2) | ((p1 & 0x30) >> 4);
            if (values[2] == alpha)
                values[2] =  (p0 & 0x0c)       | ((p1 & 0x0c) >> 2);
            if (values[3] == alpha)
                values[3] = ((p0 & 0x03) << 2) |  (p1 & 0x03);

            p0 = p1 = 0;
            for (i=0; i<4; i++)
            {
                unsigned char n = 6-(2*i);
                p0 |= ((values[i] & 0x0c) >> 2) << n;
                p1 |=  (values[i] & 0x03)       << n;
            }

            if (even)
            {
                *screenEven = p0;
                screenEven++;
                *screenEven2 = p1;
                screenEven2++;
            }
            else
            {
                *screenOdd = p0;
                screenOdd++;
                *screenOdd2 = p1;
                screenOdd2++;
            }

            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                if (even)
                {
                    even = 0;
                    screenEven += delta;
                    screenEven2 += delta;
                }
                else
                {
                    even = 1;
                    screenOdd += delta;
                    screenOdd2 += delta;
                }
            }
        }
    }
}

void Screen_PrintSpritePlanar(const SCREEN *scr, const SPRITEPOS *sprite)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0;
        unsigned char *data = sprite->SpriteAdr;
        char far *screen = (char far *)MK_FP(0xa000, scr->vpage + sprite->MemAdr);

        outportb(0x03c4, 0x02);
        while (stop == 0)
        {
            outportb(0x03c5, 0x01);
            *screen = data[pos++];

            outportb(0x03c5, 0x02);
            *screen = data[pos++];

            outportb(0x03c5, 0x04);
            *screen = data[pos++];

            outportb(0x03c5, 0x08);
            *screen = data[pos++];

            screen++;
            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                screen += delta;
            }
        }
    }
}

void Screen_PrintSpriteTr256cPlanar(const SCREEN *scr, const SPRITEPOS *sprite, unsigned char alpha)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0;
        unsigned char pixel;
        unsigned char *data = sprite->SpriteAdr;
        char far *screen = (char far *)MK_FP(0xa000, scr->vpage + sprite->MemAdr);

        outportb(0x03c4, 0x02);
        while (stop == 0)
        {
            pixel = data[pos++];
            if (pixel != alpha)
            {
                outportb(0x03c5, 0x01);
                *screen = pixel;
            }

            pixel = data[pos++];
            if (pixel != alpha)
            {
                outportb(0x03c5, 0x02);
                *screen = pixel;
            }

            pixel = data[pos++];
            if (pixel != alpha)
            {
                outportb(0x03c5, 0x04);
                *screen = pixel;
            }

            pixel = data[pos++];
            if (pixel != alpha)
            {
                outportb(0x03c5, 0x08);
                *screen = pixel;
            }

            screen++;
            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                screen += delta;
            }
        }
    }
}

void Screen_PrintSpriteTr16cPlanar(const SCREEN *scr, const SPRITEPOS *sprite, unsigned char alpha)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0, i;
        unsigned char p0, p1, p2, p3, values[8];
        unsigned char *data = sprite->SpriteAdr;
        char far *screen = (char far *)MK_FP(0xa000, scr->vpage + sprite->MemAdr);

        while (stop == 0)
        {
            p0 = data[pos++];
            p1 = data[pos++];
            p2 = data[pos++];
            p3 = data[pos++];

            /* compute pixels */
            values[0] = (p3 & 0x80) >> 4 | (p2 & 0x80) >> 5 | (p1 & 0x80) >> 6 | (p0 & 0x80) >> 7;
            values[1] = (p3 & 0x40) >> 3 | (p2 & 0x40) >> 4 | (p1 & 0x40) >> 5 | (p0 & 0x40) >> 6;
            values[2] = (p3 & 0x20) >> 2 | (p2 & 0x20) >> 3 | (p1 & 0x20) >> 4 | (p0 & 0x20) >> 5;
            values[3] = (p3 & 0x10) >> 1 | (p2 & 0x10) >> 2 | (p1 & 0x10) >> 3 | (p0 & 0x10) >> 4;
            values[4] = (p3 & 0x08)      | (p2 & 0x08) >> 1 | (p1 & 0x08) >> 2 | (p0 & 0x08) >> 3;
            values[5] = (p3 & 0x04) << 1 | (p2 & 0x04)      | (p1 & 0x04) >> 1 | (p0 & 0x04) >> 2;
            values[6] = (p3 & 0x02) << 2 | (p2 & 0x02) << 1 | (p1 & 0x02)      | (p0 & 0x02) >> 1;
            values[7] = (p3 & 0x01) << 3 | (p2 & 0x01) << 2 | (p1 & 0x01) << 1 | (p0 & 0x01)     ;

            /* reading pixels from video memory */
            outportb(0x03ce, 0x04);
            outportb(0x03cf, 0x00);
            p0 = *screen;
            outportb(0x03cf, 0x01);
            p1 = *screen;
            outportb(0x03cf, 0x02);
            p2 = *screen;
            outportb(0x03cf, 0x03);
            p3 = *screen;

            /* replace pixels with video memory if needed */
            if (values[0] == alpha)
                values[0] = (p3 & 0x80) >> 4 | (p2 & 0x80) >> 5 | (p1 & 0x80) >> 6 | (p0 & 0x80) >> 7;
            if (values[1] == alpha)
                values[1] = (p3 & 0x40) >> 3 | (p2 & 0x40) >> 4 | (p1 & 0x40) >> 5 | (p0 & 0x40) >> 6;
            if (values[2] == alpha)
                values[2] = (p3 & 0x20) >> 2 | (p2 & 0x20) >> 3 | (p1 & 0x20) >> 4 | (p0 & 0x20) >> 5;
            if (values[3] == alpha)
                values[3] = (p3 & 0x10) >> 1 | (p2 & 0x10) >> 2 | (p1 & 0x10) >> 3 | (p0 & 0x10) >> 4;
            if (values[4] == alpha)
                values[4] = (p3 & 0x08)      | (p2 & 0x08) >> 1 | (p1 & 0x08) >> 2 | (p0 & 0x08) >> 3;
            if (values[5] == alpha)
                values[5] = (p3 & 0x04) << 1 | (p2 & 0x04)      | (p1 & 0x04) >> 1 | (p0 & 0x04) >> 2;
            if (values[6] == alpha)
                values[6] = (p3 & 0x02) << 2 | (p2 & 0x02) << 1 | (p1 & 0x02)      | (p0 & 0x02) >> 1;
            if (values[7] == alpha)
                values[7] = (p3 & 0x01) << 3 | (p2 & 0x01) << 2 | (p1 & 0x01) << 1 | (p0 & 0x01)     ;

            p0 = p1 = p2 = p3 = 0;
            for (i=0; i<8; i++)
            {
                unsigned char n = 7-i;
                p3 |= ((values[i] & 8) >> 3) << n;
                p2 |= ((values[i] & 4) >> 2) << n;
                p1 |= ((values[i] & 2) >> 1) << n;
                p0 |=  (values[i] & 1)       << n;
            }

            outportb(0x03c4, 0x02);
            outportb(0x03c5, 0x01);
            *screen = p0;
            outportb(0x03c5, 0x02);
            *screen = p1;
            outportb(0x03c5, 0x04);
            *screen = p2;
            outportb(0x03c5, 0x08);
            *screen = p3;

            screen++;
            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                screen += delta;
            }
        }
    }
}

/* Tandy 320x200x16 stores four 8 KB scanline banks, two pixels per byte. */
static unsigned short Screen_TandyBaseOffset(unsigned short egaAdr)
{
    unsigned short line = egaAdr / 40;

    return (unsigned short)(((line & 3) << 13) +
                            ((line >> 2) * 160) +
                            ((egaAdr % 40) * 4));
}

#define TANDY_SPRITE_CACHE_COUNT 32
typedef struct
{
    unsigned short offset;
    unsigned short length;
    unsigned short addressDelta;
} TANDY_SPRITE_ROW;

typedef struct
{
    const unsigned char *source;
    unsigned char *packed;
    TANDY_SPRITE_ROW *rows;
    unsigned short rowCount;
    unsigned char scale;
} TANDY_SPRITE_CACHE;

static TANDY_SPRITE_CACHE TandySpriteCache[TANDY_SPRITE_CACHE_COUNT];
static unsigned char TandySpriteCacheUsed = 0;
static unsigned short TandyStarClipLeft = 0;
static unsigned short TandyStarClipTop = 0;
static unsigned short TandyStarClipRight = 0;
static unsigned short TandyStarClipBottom = 0;
static int TandyStarClipEnabled = 0;

void Screen_TandySetStarClip(unsigned short left, unsigned short top,
                             unsigned short right, unsigned short bottom)
{
    if (left <= right && top <= bottom && right < 320 && bottom < 200)
    {
        TandyStarClipLeft = left;
        TandyStarClipTop = top;
        TandyStarClipRight = right;
        TandyStarClipBottom = bottom;
        TandyStarClipEnabled = 1;
    }
    else
        TandyStarClipEnabled = 0;
}

/* Convert each immutable EGA sprite once and retain its row geometry. The EGA
   stream stores planes low-bit first (p0 through p3). Scale 3 expands each
   source pixel by exact horizontal and vertical replication; packed bytes are
   arbitrary values and never carry ENDL markers. */
static TANDY_SPRITE_CACHE *Screen_TandyPackedSprite(const unsigned char *source,
                                                    unsigned char scale)
{
    int i, sourcePos, packedPos, rowIndex, sourceRowCount, rowLength, stop;
    int rowRepeat, outputRowIndex;
    unsigned long sourcePackedLength, packedLength, sourceAdr, rowStartAdr;
    unsigned long addressDelta;
    unsigned char delta, p0, p1, p2, p3, color0, color1, shift;
    unsigned char *packed;
    TANDY_SPRITE_ROW *rows;
    TANDY_SPRITE_CACHE *cache;

    if (scale != 3)
        scale = 1;

    for (i=0; i<TandySpriteCacheUsed; i++)
    {
        if (TandySpriteCache[i].source == source &&
            TandySpriteCache[i].scale == scale)
            return &TandySpriteCache[i];
    }

    if (TandySpriteCacheUsed >= TANDY_SPRITE_CACHE_COUNT)
        return NULL;

    sourcePos = 0;
    sourceRowCount = 0;
    sourcePackedLength = 0;
    stop = 0;
    while (!stop)
    {
        rowLength = 0;
        while (source[sourcePos] != ENDL)
        {
            sourcePos += 4;
            rowLength += 4;
        }
        sourcePackedLength += (unsigned long)rowLength;
        sourcePos++;
        delta = source[sourcePos++];
        if (sourcePackedLength > 65535L || sourceRowCount >= 1024)
            return NULL;
        sourceRowCount++;
        if (delta == 0)
            stop = 1;
    }
    packedLength = sourcePackedLength * (unsigned long)scale;
    if (sourceRowCount == 0 || packedLength == 0 || packedLength > 65535L ||
        (unsigned long)sourceRowCount * (unsigned long)scale > 65535L)
        return NULL;

    packed = malloc((unsigned int)packedLength);
    if (packed == NULL)
        return NULL;
    rows = malloc((unsigned int)(sourceRowCount * scale * sizeof(TANDY_SPRITE_ROW)));
    if (rows == NULL)
    {
        free(packed);
        return NULL;
    }

    sourcePos = 0;
    packedPos = 0;
    sourceAdr = 0;
    outputRowIndex = 0;
    for (rowIndex=0; rowIndex<sourceRowCount; rowIndex++)
    {
        int packedRowStart = packedPos;

        rowStartAdr = sourceAdr;
        rowLength = 0;
        while (source[sourcePos] != ENDL)
        {
            p0 = source[sourcePos++];
            p1 = source[sourcePos++];
            p2 = source[sourcePos++];
            p3 = source[sourcePos++];

            for (i=0; i<4; i++)
            {
                shift = (unsigned char)(7-(2*i));
                color0 = (unsigned char)(((p3 >> shift) & 1) << 3 |
                                         ((p2 >> shift) & 1) << 2 |
                                         ((p1 >> shift) & 1) << 1 |
                                          (p0 >> shift) & 1);
                color1 = (unsigned char)(((p3 >> (shift-1)) & 1) << 3 |
                                         ((p2 >> (shift-1)) & 1) << 2 |
                                         ((p1 >> (shift-1)) & 1) << 1 |
                                          (p0 >> (shift-1)) & 1);
                if (scale == 3)
                {
                    packed[packedPos++] = (unsigned char)((color0 << 4) | color0);
                    packed[packedPos++] = (unsigned char)((color0 << 4) | color1);
                    packed[packedPos++] = (unsigned char)((color1 << 4) | color1);
                }
                else
                    packed[packedPos++] = (unsigned char)((color0 << 4) | color1);
            }
            rowLength += 4;
        }

        sourcePos++;
        delta = source[sourcePos++];
        for (rowRepeat=0; rowRepeat<scale; rowRepeat++)
        {
            addressDelta = rowStartAdr * (unsigned long)scale +
                           (unsigned long)rowRepeat * 40L;
            if (addressDelta > 65535L)
            {
                free(packed);
                free(rows);
                return NULL;
            }
            rows[outputRowIndex].offset = (unsigned short)packedRowStart;
            rows[outputRowIndex].length =
                (unsigned short)(rowLength * scale);
            rows[outputRowIndex].addressDelta = (unsigned short)addressDelta;
            outputRowIndex++;
        }
        sourceAdr = rowStartAdr + (unsigned long)(rowLength / 4) + delta;
    }

    cache = &TandySpriteCache[TandySpriteCacheUsed];
    cache->source = source;
    cache->packed = packed;
    cache->rows = rows;
    cache->rowCount = (unsigned short)outputRowIndex;
    cache->scale = scale;
    TandySpriteCacheUsed++;
    return cache;
}

void Screen_TandyReleaseCache(void)
{
    int i;

    for (i=0; i<TandySpriteCacheUsed; i++)
    {
        free(TandySpriteCache[i].packed);
        free(TandySpriteCache[i].rows);
        TandySpriteCache[i].source = NULL;
        TandySpriteCache[i].packed = NULL;
        TandySpriteCache[i].rows = NULL;
        TandySpriteCache[i].rowCount = 0;
        TandySpriteCache[i].scale = 0;
    }
    TandySpriteCacheUsed = 0;
    TandyStarClipEnabled = 0;
}

#ifdef TANDY_CACHE_TEST
static void Screen_TandyTestEncodeGroup(unsigned char *dst,
                                        const unsigned char *packed)
{
    int pixel;
    unsigned char planes[4] = {0, 0, 0, 0};

    for (pixel=0; pixel<8; pixel++)
    {
        unsigned char value = (pixel & 1) ?
            (unsigned char)(packed[pixel/2] & 0x0f) :
            (unsigned char)(packed[pixel/2] >> 4);
        unsigned char mask = (unsigned char)(0x80 >> pixel);

        if (value & 8) planes[3] |= mask;
        if (value & 4) planes[2] |= mask;
        if (value & 2) planes[1] |= mask;
        if (value & 1) planes[0] |= mask;
    }
    dst[0] = planes[0];
    dst[1] = planes[1];
    dst[2] = planes[2];
    dst[3] = planes[3];
}

int Screen_TandyCacheTestSynthetic(void)
{
    unsigned char source[264];
    unsigned char expectedTail[4] = {0xee, 0x00, 0xff, 0x55};
    unsigned char group[4];
    int i, ok = 1;
    TANDY_SPRITE_CACHE *cache;

    for (i=0; i<64; i++)
    {
        group[0] = (unsigned char)(i*4);
        group[1] = (unsigned char)(i*4+1);
        group[2] = (unsigned char)(i*4+2);
        group[3] = (unsigned char)(i*4+3);
        Screen_TandyTestEncodeGroup(source+i*4, group);
    }
    source[256] = ENDL;
    source[257] = 5;
    Screen_TandyTestEncodeGroup(source+258, expectedTail);
    source[262] = ENDL;
    source[263] = 0;

    cache = Screen_TandyPackedSprite(source, 1);
    if (cache == NULL || cache->rowCount != 2 ||
        cache->rows[0].offset != 0 || cache->rows[0].length != 256 ||
        cache->rows[0].addressDelta != 0 || cache->rows[1].offset != 256 ||
        cache->rows[1].length != 4 || cache->rows[1].addressDelta != 69)
        ok = 0;
    if (ok)
    {
        for (i=0; i<256; i++)
        {
            if (cache->packed[i] != (unsigned char)i)
            {
                ok = 0;
                break;
            }
        }
        for (i=0; i<4 && ok; i++)
        {
            if (cache->packed[256+i] != expectedTail[i])
                ok = 0;
        }
    }
    Screen_TandyReleaseCache();
    return ok;
}

int Screen_TandyCacheValidateAsset(const unsigned char *source)
{
    unsigned int nextOffset = 0;
    unsigned long nextAddress = 0;
    int sourcePos = 0, rowIndex = 0, rowLength, stop = 0;
    unsigned char delta;
    TANDY_SPRITE_CACHE *cache = Screen_TandyPackedSprite(source, 1);

    if (cache == NULL || cache->rowCount == 0)
        return 0;
    while (!stop)
    {
        rowLength = 0;
        while (source[sourcePos] != ENDL)
        {
            sourcePos += 4;
            rowLength += 4;
        }
        if (rowIndex >= cache->rowCount ||
            cache->rows[rowIndex].offset != nextOffset ||
            cache->rows[rowIndex].length != rowLength ||
            cache->rows[rowIndex].addressDelta != nextAddress ||
            (cache->rows[rowIndex].length & 3) != 0)
            return 0;
        nextOffset += cache->rows[rowIndex].length;
        nextAddress += (unsigned long)(rowLength / 4);
        sourcePos++;
        delta = source[sourcePos++];
        nextAddress += delta;
        stop = (delta == 0);
        rowIndex++;
    }
    return rowIndex == cache->rowCount;
}

int Screen_TandyCacheTestAssetColors(void)
{
    static const unsigned char arcStripeColors[6] = {11, 6, 1, 3, 8, 13};
    static const unsigned int expectedCatCounts[16] =
        {2, 0, 26, 0, 79, 8, 0, 99, 0, 6, 189, 0, 95, 0, 0, 0};
    unsigned int catCounts[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int i;
    unsigned char color;
    TANDY_SPRITE_CACHE *arc = Screen_TandyPackedSprite(ARC1_EGA, 1);
    TANDY_SPRITE_CACHE *cat = Screen_TandyPackedSprite(CAT1_EGA, 1);

    if (arc == NULL || arc->rowCount != 21 || cat == NULL)
        return 0;

    /* The center pixel of these source rows is inside each rainbow stripe. */
    for (i=0; i<6; i++)
    {
        const TANDY_SPRITE_ROW *row = &arc->rows[2 + i*3];
        color = (unsigned char)(arc->packed[row->offset + 4] >> 4);
        if (color != arcStripeColors[i])
            return 0;
    }

    for (i=0; i<cat->rows[cat->rowCount-1].offset +
                      cat->rows[cat->rowCount-1].length; i++)
    {
        ++catCounts[cat->packed[i] >> 4];
        ++catCounts[cat->packed[i] & 0x0f];
    }

    /* The source cat has true black outlines and gray fur; no color 3. */
    for (i=0; i<16; i++)
    {
        if (catCounts[i] != expectedCatCounts[i])
            return 0;
    }
    return 1;
}

int Screen_TandyCacheTestScale3(const unsigned char *source)
{
    unsigned int rowIndex, repeat, byteIndex;
    unsigned char sourceByte, color0, color1;
    unsigned char expected[3];
    int ok = 1;
    TANDY_SPRITE_CACHE *native = Screen_TandyPackedSprite(source, 1);
    TANDY_SPRITE_CACHE *scaled = Screen_TandyPackedSprite(source, 3);

    if (native == NULL || scaled == NULL ||
        scaled->rowCount != native->rowCount * 3)
        ok = 0;

    for (rowIndex=0; ok && rowIndex<native->rowCount; rowIndex++)
    {
        const TANDY_SPRITE_ROW *sourceRow = &native->rows[rowIndex];
        for (repeat=0; repeat<3 && ok; repeat++)
        {
            const TANDY_SPRITE_ROW *scaledRow =
                &scaled->rows[rowIndex*3+repeat];
            if (scaledRow->offset != sourceRow->offset * 3 ||
                scaledRow->length != sourceRow->length * 3 ||
                scaledRow->addressDelta != sourceRow->addressDelta * 3 + repeat * 40)
            {
                ok = 0;
                break;
            }
            for (byteIndex=0; byteIndex<sourceRow->length; byteIndex++)
            {
                sourceByte = native->packed[sourceRow->offset+byteIndex];
                color0 = (unsigned char)(sourceByte >> 4);
                color1 = (unsigned char)(sourceByte & 0x0f);
                expected[0] = (unsigned char)((color0 << 4) | color0);
                expected[1] = (unsigned char)((color0 << 4) | color1);
                expected[2] = (unsigned char)((color1 << 4) | color1);
                if (scaled->packed[scaledRow->offset+byteIndex*3] != expected[0] ||
                    scaled->packed[scaledRow->offset+byteIndex*3+1] != expected[1] ||
                    scaled->packed[scaledRow->offset+byteIndex*3+2] != expected[2])
                {
                    ok = 0;
                    break;
                }
            }
        }
    }
    Screen_TandyReleaseCache();
    return ok;
}
#endif

/* Slow-path renderer used only if the 3x cache cannot be allocated. The
   source row remains streaming; only one 12-byte expanded group is on stack. */
static void Screen_TandyDrawScaledFallback(char far *screen,
                                           const SPRITEPOS *sprite,
                                           unsigned char alpha,
                                           int transparent)
{
    int pos = 0, stop = 0, i, repeat, byteIndex;
    unsigned long sourceAdr = 0, targetAdr;
    unsigned short offset;
    unsigned char p0, p1, p2, p3, color0, color1, shift;
    unsigned char output[12], old, packedPixel;
    const unsigned char *data = sprite->SpriteAdr;

    while (!stop)
    {
        p0 = data[pos++];
        p1 = data[pos++];
        p2 = data[pos++];
        p3 = data[pos++];
        for (i=0; i<4; i++)
        {
            shift = (unsigned char)(7-(2*i));
            color0 = (unsigned char)(((p3 >> shift) & 1) << 3 |
                                     ((p2 >> shift) & 1) << 2 |
                                     ((p1 >> shift) & 1) << 1 |
                                      (p0 >> shift) & 1);
            color1 = (unsigned char)(((p3 >> (shift-1)) & 1) << 3 |
                                     ((p2 >> (shift-1)) & 1) << 2 |
                                     ((p1 >> (shift-1)) & 1) << 1 |
                                      (p0 >> (shift-1)) & 1);
            output[i*3] = (unsigned char)((color0 << 4) | color0);
            output[i*3+1] = (unsigned char)((color0 << 4) | color1);
            output[i*3+2] = (unsigned char)((color1 << 4) | color1);
        }

        for (repeat=0; repeat<3; repeat++)
        {
            targetAdr = (unsigned long)sprite->MemAdr + sourceAdr * 3L +
                        (unsigned long)repeat * 40L;
            offset = Screen_TandyBaseOffset((unsigned short)targetAdr);
            for (byteIndex=0; byteIndex<12; byteIndex++)
            {
                packedPixel = output[byteIndex];
                if (transparent)
                {
                    old = (unsigned char)screen[offset+byteIndex];
                    color0 = (unsigned char)(packedPixel >> 4);
                    color1 = (unsigned char)(packedPixel & 0x0f);
                    if (color0 == alpha)
                        color0 = (unsigned char)((old >> 4) & 0x0f);
                    if (color1 == alpha)
                        color1 = (unsigned char)(old & 0x0f);
                    packedPixel = (unsigned char)((color0 << 4) | color1);
                }
                screen[offset+byteIndex] = packedPixel;
            }
        }
        sourceAdr++;

        if (data[pos] == ENDL)
        {
            unsigned char delta = data[pos+1];
            pos += 2;
            sourceAdr += delta;
            if (delta == 0)
                stop = 1;
        }
    }
}

/* Write one packed pair while keeping star/erase sprites out of the fixed
   foreground envelope. The bounds are inclusive pixel coordinates. */
static void Screen_TandyWritePair(char far *screen, unsigned short offset,
                                  unsigned short x, unsigned short y,
                                  unsigned char packedPixel,
                                  unsigned char alpha, int transparent,
                                  int clip)
{
    unsigned char old, color0, color1;
    int preserve0 = 0, preserve1 = 0;

    color0 = (unsigned char)(packedPixel >> 4);
    color1 = (unsigned char)(packedPixel & 0x0f);
    if (transparent && color0 == alpha && color1 == alpha)
        return;

    if (clip && TandyStarClipEnabled &&
        y >= TandyStarClipTop && y <= TandyStarClipBottom)
    {
        preserve0 = (x >= TandyStarClipLeft && x <= TandyStarClipRight);
        preserve1 = ((unsigned short)(x + 1) >= TandyStarClipLeft &&
                     (unsigned short)(x + 1) <= TandyStarClipRight);
    }
    if (transparent)
    {
        if (color0 == alpha)
            preserve0 = 1;
        if (color1 == alpha)
            preserve1 = 1;
    }

    if (preserve0 || preserve1)
    {
        old = (unsigned char)screen[offset];
        if (preserve0)
            color0 = (unsigned char)((old >> 4) & 0x0f);
        if (preserve1)
            color1 = (unsigned char)(old & 0x0f);
    }
    screen[offset] = (unsigned char)((color0 << 4) | color1);
}

static void Screen_TandyDrawCached(char far *screen,
                                   const SPRITEPOS *sprite,
                                   const TANDY_SPRITE_CACHE *cache,
                                   unsigned char alpha,
                                   int transparent,
                                   int clip)
{
    int rowIndex, i;
    unsigned short offset, rowX, rowY;
    unsigned long egaAdr;
    unsigned char packedPixel;
    unsigned char transparentByte = (unsigned char)((alpha << 4) | alpha);
    const TANDY_SPRITE_ROW *row;

    for (rowIndex=0; rowIndex<cache->rowCount; rowIndex++)
    {
        row = &cache->rows[rowIndex];
        egaAdr = (unsigned long)sprite->MemAdr + row->addressDelta;
        offset = Screen_TandyBaseOffset((unsigned short)egaAdr);
        if (!transparent && !clip)
        {
            if (row->length != 0)
                memcpy(screen+offset, cache->packed+row->offset, row->length);
            continue;
        }
        rowX = (unsigned short)((egaAdr % 40L) * 8L);
        rowY = (unsigned short)(egaAdr / 40L);
        for (i=0; i<row->length; i++)
        {
            packedPixel = cache->packed[row->offset+i];
            if (transparent && packedPixel == transparentByte)
                continue;
            Screen_TandyWritePair(screen, (unsigned short)(offset+i),
                                  (unsigned short)(rowX + i*2), rowY,
                                  packedPixel, alpha, transparent, clip);
        }
    }
}

#ifdef TANDY_CACHE_TEST
int Screen_TandyCacheTestClipPair(void)
{
    static unsigned char page[32768];
    unsigned short offset;
    int ok = 1;

    memset(page, 0x55, sizeof(page));
    Screen_TandySetStarClip(81, 68, 247, 130);

    /* The left edge cuts through pixels 80/81 in one packed byte. */
    offset = (unsigned short)(Screen_TandyBaseOffset((unsigned short)(68*40+10)));
    Screen_TandyWritePair((char far *)page, offset, 80, 68, 0x12, 0, 0, 1);
    if (page[offset] != 0x15)
        ok = 0;

    /* Pixels 246/247 are protected; 248/249 are outside the clip. */
    offset = (unsigned short)(Screen_TandyBaseOffset((unsigned short)(68*40+30)) + 3);
    Screen_TandyWritePair((char far *)page, offset, 246, 68, 0x34, 0, 0, 1);
    if (page[offset] != 0x55)
        ok = 0;
    offset = (unsigned short)(Screen_TandyBaseOffset((unsigned short)(68*40+31)));
    Screen_TandyWritePair((char far *)page, offset, 248, 68, 0x67, 0, 0, 1);
    if (page[offset] != 0x67)
        ok = 0;

    /* The same horizontal coordinates are writable just above/below it. */
    offset = (unsigned short)(Screen_TandyBaseOffset((unsigned short)(67*40+10)));
    Screen_TandyWritePair((char far *)page, offset, 80, 67, 0x89, 0, 0, 1);
    if (page[offset] != 0x89)
        ok = 0;
    offset = (unsigned short)(Screen_TandyBaseOffset((unsigned short)(131*40+10)));
    Screen_TandyWritePair((char far *)page, offset, 80, 131, 0xab, 0, 0, 1);
    if (page[offset] != 0xab)
        ok = 0;

    /* Transparent source pixels stay untouched even outside the clip. */
    offset = (unsigned short)(Screen_TandyBaseOffset((unsigned short)(67*40+10)));
    Screen_TandyWritePair((char far *)page, offset, 80, 67, 0xf2, 15, 1, 1);
    if (page[offset] != 0x82)
        ok = 0;

    Screen_TandySetStarClip(1, 1, 0, 0);
    return ok;
}

static int Screen_TandyTestFallbackOne(const unsigned char *source,
                                       unsigned short memAdr,
                                       unsigned char alpha,
                                       int transparent)
{
    static unsigned char cachedPage[32768];
    static unsigned char fallbackPage[32768];
    SPRITEPOS sprite;
    TANDY_SPRITE_CACHE *cache;
    int equal;

    memset(cachedPage, 0x11, sizeof(cachedPage));
    memset(fallbackPage, 0x11, sizeof(fallbackPage));
    sprite.SpriteAdr = (unsigned char *)source;
    sprite.MemAdr = memAdr;
    sprite.TandyScale = 3;
    sprite.TandyClip = 0;
    cache = Screen_TandyPackedSprite(source, 3);
    if (cache == NULL)
        return 0;

    Screen_TandyDrawCached((char far *)cachedPage, &sprite, cache,
                           alpha, transparent, 0);
    Screen_TandyDrawScaledFallback((char far *)fallbackPage, &sprite,
                                   alpha, transparent);
    equal = (memcmp(cachedPage, fallbackPage, sizeof(cachedPage)) == 0);
    Screen_TandyReleaseCache();
    return equal;
}

int Screen_TandyCacheTestFallback(void)
{
#ifdef TANDY_CACHE_TEST
    Screen_TandyCacheTestProgress(3, 1);
#endif
    if (!Screen_TandyTestFallbackOne(CAT1_EGA, 2742, 15, 1))
        return 0;
#ifdef TANDY_CACHE_TEST
    Screen_TandyCacheTestProgress(3, 2);
#endif
    if (!Screen_TandyTestFallbackOne(CAT1_EGA, 2742, 0, 0))
        return 0;
    return 1;
}

static unsigned char Screen_TandyTestReadPixel(const unsigned char far *page,
                                               unsigned short x,
                                               unsigned short y)
{
    unsigned short egaAdr = (unsigned short)(y*40 + x/8);
    unsigned short offset = Screen_TandyBaseOffset(egaAdr);
    unsigned char packed = page[offset + ((x & 7) >> 1)];

    return (x & 1) ? (unsigned char)(packed & 0x0f) :
                     (unsigned char)(packed >> 4);
}

static void Screen_TandyTestWritePixel(unsigned char far *page,
                                       unsigned short x,
                                       unsigned short y,
                                       unsigned char color)
{
    unsigned short egaAdr = (unsigned short)(y*40 + x/8);
    unsigned short offset = (unsigned short)(Screen_TandyBaseOffset(egaAdr) +
                                               ((x & 7) >> 1));
    unsigned char packed = page[offset];

    if (x & 1)
        page[offset] = (unsigned char)((packed & 0xf0) | (color & 0x0f));
    else
        page[offset] = (unsigned char)((packed & 0x0f) | ((color & 0x0f) << 4));
}

static void Screen_TandyTestFillClip(unsigned char far *page,
                                     unsigned char color)
{
    unsigned short x, y;

    for (y=68; y<=130; y++)
        for (x=80; x<=247; x++)
            Screen_TandyTestWritePixel(page, x, y, color);
}

static void Screen_TandyTestReadClip(const unsigned char far *page,
                                     unsigned char *pixels)
{
    unsigned short x, y;
    unsigned int pos = 0;

    for (y=68; y<=130; y++)
        for (x=80; x<=247; x++)
            pixels[pos++] = Screen_TandyTestReadPixel(page, x, y);
}

static int Screen_TandyTestClipMatches(const unsigned char far *page,
                                       const unsigned char *expected)
{
    unsigned short x, y;
    unsigned int pos = 0;

    for (y=68; y<=130; y++)
        for (x=80; x<=247; x++)
            if (Screen_TandyTestReadPixel(page, x, y) != expected[pos++])
                return 0;
    return 1;
}

static int Screen_TandyTestDrawFrameLayer(unsigned char far *page,
                                          const SPRITEPOS *frame,
                                          int first, int last,
                                          int transparent)
{
    int i;
    unsigned char scale;
    TANDY_SPRITE_CACHE *cache;

    for (i=first; i<=last; i++)
    {
        scale = (frame[i].TandyScale == 3) ? 3 : 1;
        cache = Screen_TandyPackedSprite(frame[i].SpriteAdr, scale);
        if (cache == NULL)
            return 0;
        Screen_TandyDrawCached((char far *)page, &frame[i], cache, 15,
                               transparent && i == 14,
                               frame[i].TandyClip);
    }
    return 1;
}

int Screen_TandyCacheTestStarOverlap(const SPRITEPOS *frames,
                                     int frameCount, int frameStride)
{
    static unsigned char page[32768];
    static unsigned char expectedA[168*63];
    static unsigned char expectedB[168*63];
    const SPRITEPOS *frame;
    unsigned char *previous = expectedA;
    unsigned char *current = expectedB;
    unsigned int i;
    int f, ok = 1;

    if (frames == NULL || frameCount <= 0 || frameStride < 15)
        return 0;

    Screen_TandySetStarClip(80, 68, 247, 130);
    memset(page, 0xaa, sizeof(page));
    for (f=0; f<=frameCount && ok; f++)
    {
#ifdef TANDY_CACHE_TEST
        Screen_TandyCacheTestProgress(8, f);
#endif
        frame = frames + (long)(f % frameCount) * frameStride;

        /* This is the transient interval where a star/erase pass runs while
           the previous enlarged foreground is still on the visible page. */
        if (!Screen_TandyTestDrawFrameLayer(page, frame, 0, 10, 0) ||
            !Screen_TandyTestDrawFrameLayer(page, frame, 14, 14, 1))
        {
            ok = 0;
            break;
        }
        if (f == 0)
        {
            for (i=0; i<168U*63U; i++)
                previous[i] = 10;
        }
        if (!Screen_TandyTestClipMatches(page, previous))
        {
            ok = 0;
            break;
        }

        /* Build the independent foreground-only reference in the protected
           rectangle, restore the old visible pixels, then repaint it as the
           runtime does. This validates every retained frame and loop seam. */
        Screen_TandyTestFillClip(page, 10);
        if (!Screen_TandyTestDrawFrameLayer(page, frame, 11, 13, 0))
        {
            ok = 0;
            break;
        }
        Screen_TandyTestReadClip(page, current);
        for (i=0; i<168U*63U; i++)
            Screen_TandyTestWritePixel(page,
                (unsigned short)(80 + i % 168),
                (unsigned short)(68 + i / 168), previous[i]);
        if (!Screen_TandyTestDrawFrameLayer(page, frame, 11, 13, 0) ||
            !Screen_TandyTestClipMatches(page, current))
        {
            ok = 0;
            break;
        }

        {
            unsigned char *swap = previous;
            previous = current;
            current = swap;
        }
    }
    Screen_TandyReleaseCache();
    return ok;
}
#endif

static void Screen_PrintSpriteTandy(const SCREEN *scr, const SPRITEPOS *sprite)
{
    if (sprite == NULL)
        return;

    {
        int pos = 0, stop = 0, i;
        unsigned char scale = (sprite->TandyScale == 3) ? 3 : 1;
        unsigned long egaAdr;
        unsigned short offset;
        unsigned char p0, p1, p2, p3, color0, color1, shift;
        const unsigned char *data = sprite->SpriteAdr;
        TANDY_SPRITE_CACHE *cache = Screen_TandyPackedSprite(data, scale);
        char far *screen = (char far *)MK_FP(0xb800, scr->vpage);

        if (cache != NULL)
        {
            Screen_TandyDrawCached(screen, sprite, cache, 0, 0,
                                   sprite->TandyClip);
            return;
        }

        if (scale == 3)
        {
            Screen_TandyDrawScaledFallback(screen, sprite, 0, 0);
            return;
        }

        egaAdr = sprite->MemAdr;
        offset = Screen_TandyBaseOffset((unsigned short)egaAdr);

        /* Allocation/cache failure fallback reads only source control bytes. */
        while (stop == 0)
        {
            p0 = data[pos++];
            p1 = data[pos++];
            p2 = data[pos++];
            p3 = data[pos++];

            for (i=0; i<4; i++)
            {
                shift = (unsigned char)(7-(2*i));
                color0 = (unsigned char)(((p3 >> shift) & 1) << 3 |
                                         ((p2 >> shift) & 1) << 2 |
                                         ((p1 >> shift) & 1) << 1 |
                                          (p0 >> shift) & 1);
                color1 = (unsigned char)(((p3 >> (shift-1)) & 1) << 3 |
                                         ((p2 >> (shift-1)) & 1) << 2 |
                                         ((p1 >> (shift-1)) & 1) << 1 |
                                          (p0 >> (shift-1)) & 1);
                if (sprite->TandyClip)
                {
                    unsigned short x = (unsigned short)((egaAdr % 40L) * 8L + i*2);
                    unsigned short y = (unsigned short)(egaAdr / 40L);
                    Screen_TandyWritePair(screen, (unsigned short)(offset+i),
                                          x, y,
                                          (unsigned char)((color0 << 4) | color1),
                                          0, 0, 1);
                }
                else
                    screen[offset+i] = (unsigned char)((color0 << 4) | color1);
            }
            egaAdr++;

            if (data[pos] == ENDL)
            {
                unsigned char delta = data[pos+1];
                pos += 2;
                if (delta == 0)
                    stop = 1;
                if (!stop)
                {
                    egaAdr += delta;
                    offset = Screen_TandyBaseOffset(egaAdr);
                }
            }
            else
                offset = Screen_TandyBaseOffset(egaAdr);
        }
    }
}

static void Screen_PrintSpriteTrTandy(const SCREEN *scr, const SPRITEPOS *sprite, unsigned char alpha)
{
    if (sprite == NULL)
        return;

    {
        int pos = 0, stop = 0, i;
        unsigned char scale = (sprite->TandyScale == 3) ? 3 : 1;
        unsigned long egaAdr;
        unsigned short offset;
        unsigned char p0, p1, p2, p3, color0, color1, shift, old, packedPixel;
        const unsigned char *data = sprite->SpriteAdr;
        TANDY_SPRITE_CACHE *cache = Screen_TandyPackedSprite(data, scale);
        char far *screen = (char far *)MK_FP(0xb800, scr->vpage);

        if (cache != NULL)
        {
            Screen_TandyDrawCached(screen, sprite, cache, alpha, 1,
                                   sprite->TandyClip);
            return;
        }

        if (scale == 3)
        {
            Screen_TandyDrawScaledFallback(screen, sprite, alpha, 1);
            return;
        }

        egaAdr = sprite->MemAdr;
        offset = Screen_TandyBaseOffset((unsigned short)egaAdr);

        /* Allocation/cache failure fallback reads only source control bytes. */
        while (stop == 0)
        {
            p0 = data[pos++];
            p1 = data[pos++];
            p2 = data[pos++];
            p3 = data[pos++];

            for (i=0; i<4; i++)
            {
                shift = (unsigned char)(7-(2*i));
                color0 = (unsigned char)(((p3 >> shift) & 1) << 3 |
                                         ((p2 >> shift) & 1) << 2 |
                                         ((p1 >> shift) & 1) << 1 |
                                          (p0 >> shift) & 1);
                color1 = (unsigned char)(((p3 >> (shift-1)) & 1) << 3 |
                                         ((p2 >> (shift-1)) & 1) << 2 |
                                         ((p1 >> (shift-1)) & 1) << 1 |
                                          (p0 >> (shift-1)) & 1);
                if (sprite->TandyClip)
                {
                    unsigned short x = (unsigned short)((egaAdr % 40L) * 8L + i*2);
                    unsigned short y = (unsigned short)(egaAdr / 40L);
                    Screen_TandyWritePair(screen, (unsigned short)(offset+i),
                                          x, y,
                                          (unsigned char)((color0 << 4) | color1),
                                          alpha, 1, 1);
                }
                else
                {
                    old = screen[offset+i];
                    if (color0 == alpha)
                        color0 = (unsigned char)((old >> 4) & 0x0f);
                    if (color1 == alpha)
                        color1 = (unsigned char)(old & 0x0f);
                    screen[offset+i] = (unsigned char)((color0 << 4) | color1);
                }
            }
            egaAdr++;

            if (data[pos] == ENDL)
            {
                unsigned char delta = data[pos+1];
                pos += 2;
                if (delta == 0)
                    stop = 1;
                if (!stop)
                {
                    egaAdr += delta;
                    offset = Screen_TandyBaseOffset(egaAdr);
                }
            }
            else
                offset = Screen_TandyBaseOffset(egaAdr);
        }
    }
}

static void Screen_EraseScreenTandy(SCREEN *scr, unsigned char color)
{
    unsigned char value = (unsigned char)((color << 4) | color);
    char far *screen = (char far *)MK_FP(0xb800, scr->vpage);

    memset(screen, value, scr->pagesize);
    scr->needRefresh = 1;
}

static void Screen_SetPaletteTandy(const unsigned char *palette, const int nbc)
{
    int i;

    for (i=0; i<nbc && i<16; i++)
    {
        outportb(0x03da, 0x10+i);
        outportb(0x03de, palette[i] & 0x0f);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

SCREEN *CreateScreen(VIDEODEVICE device)
{
    SCREEN *scr = NULL;
    /* /tandy is an explicit override; don't probe Tandy ports on other PCs. */
    VIDEODEVICE available = (device == vTandy) ? vTandy : Screen_DetectVideo();

    if (available != vNULL)
        scr = malloc(sizeof(SCREEN));

    if (scr != NULL)
    {
        scr->cpage = 0;
        scr->vpage = 0;
        scr->SetVideoMode = Screen_SetVideoMode;
        scr->ChangeVideoPage = Screen_ChangeVideoPage;

        scr->videoDevice = available;
        if ((device != vNULL) && (scr->videoDevice <= device))
            scr->videoDevice = device;
        switch (scr->videoDevice)
        {
        case vTandy:
            scr->pagesize = 32768;
            scr->SetPalette = Screen_SetPaletteTandy;
            scr->EraseScreen = Screen_EraseScreenTandy;
            scr->PrintSprite = Screen_PrintSpriteTandy;
            scr->PrintSpriteTr = Screen_PrintSpriteTrTandy;
            break;

        case vPlantronicsColorPlus:
            scr->pagesize = 8000;
            scr->SetPalette = Screen_SetPalette16c;
            scr->EraseScreen = Screen_EraseScreenCGA;
            scr->PrintSprite = Screen_PrintSpriteCGA;
            scr->PrintSpriteTr = Screen_PrintSpriteTrCGA;
            break;
        case vEGA:
            scr->pagesize = 8000;
            scr->SetPalette = Screen_SetPalette16c;
            scr->EraseScreen = Screen_EraseScreenEGA;
            scr->PrintSprite = Screen_PrintSpritePlanar;
            scr->PrintSpriteTr = Screen_PrintSpriteTr16cPlanar;
            break;
        case vEGAH:
            scr->pagesize = 28000;
            scr->SetPalette = Screen_SetPalette16c;
            scr->EraseScreen = Screen_EraseScreenEGA;
            scr->PrintSprite = Screen_PrintSpritePlanar;
            scr->PrintSpriteTr = Screen_PrintSpriteTr16cPlanar;
            break;
        case vVGAH:
            scr->pagesize = 32000;
            scr->SetPalette = Screen_SetPalette16c;
            scr->EraseScreen = Screen_EraseScreenEGA;
            scr->PrintSprite = Screen_PrintSpritePlanar;
            scr->PrintSpriteTr = Screen_PrintSpriteTr16cPlanar;
            break;
        case vVGA:
            scr->pagesize = 16000;
            scr->SetPalette = Screen_SetPalette256c;
            scr->EraseScreen = Screen_EraseScreenVGA;
            scr->PrintSprite = Screen_PrintSpritePlanar;
            scr->PrintSpriteTr = Screen_PrintSpriteTr256cPlanar;
            break;
        }
        scr->needRefresh = 1;

        Screen_ChangeVideoPage(scr);
    }

    return scr;
}

SCREEN *DeleteScreen(SCREEN *scr)
{
    if (scr != NULL)
    {
        if (scr->videoDevice == vTandy)
            Screen_TandyReleaseCache();
        free(scr);
    }
    return NULL;
}
