/*******************************************************************************
*                               NyanCat for DOS                                *
********************************************************************************
* Author  : OlivierP                                                           *
* Date    : march, july, october & november 2012                               *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <dos.h> /* MK_FP */
#include <conio.h> /* outportb */
#include <string.h>
#include <stdlib.h>

#ifdef __WATCOMC__
#define outportb outp
#define inportb inp
#define disable _disable
#define enable _enable
#define getvect _dos_getvect
#define setvect _dos_setvect
#endif

#include "screen.h"
#include "nyananim.h"
#include "irq.h"
#include "wav.h"
#include "tandy.h"

#define lo(value) (unsigned char)( value & 0x00FF )
#define hi(value) (unsigned char)((value & 0xFF00) >> 8)

/*
http://www.reenigne.org/blog/why-the-ega-can-only-use-16-of-its-64-colours-in-200-line-modes/
http://elongshine.com/269
*/

/* 320x200x16c */
unsigned char PALET_EGA[] = {63, 62, 60, 58, 61, 60, 6, 56, 59, 5, 1, 4, 0, 57, 0, 1, 1};

/* 640x350x16c */
unsigned char PALET_EGAH[] = {63, 54, 46, 50, 61, 60, 52, 56, 11, 60, 8, 36, 0, 41, 0, 8, 8};

/* 320x200x256c */
unsigned char PALET_VGA[] =
{
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00,
    0xFF, 0xCC, 0x99,
    0x33, 0xFF, 0x00,
    0xFF, 0x99, 0xFF,
    0xFF, 0x99, 0x99,
    0xFF, 0x99, 0x00,
    0x99, 0x99, 0x99,
    0x00, 0x99, 0xFF,
    0xFF, 0x33, 0x99,
    0x00, 0x33, 0x66,
    0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x66, 0x33, 0xFF,
    0x00, 0x00, 0x00,
    0x00, 0x33, 0x66
};

SCREEN *scr = NULL;
DMA_BUFFER *dma_buffer = NULL;
SB_SAMPLE *sb_sample = NULL;
SB_CARD *sb_card = NULL;
static SPRITEPOS TandyAnimation[NBANIM][NBSPRITE];

#define TANDY_OBJECT_FIRST 11
#define TANDY_OBJECT_LAST 13
#define TANDY_OBJECT_X_SHIFT_GROUPS (-38)
#define TANDY_OBJECT_Y_SHIFT_LINES (-160)

/* Keep the original animation table untouched for every non-Tandy mode. */
static void BuildTandyAnimation(int scale)
{
    int frame, sprite;
    unsigned short sourceAdr;
    int sourceX, sourceY, targetX, targetY;

    for (frame=0; frame<NBANIM; frame++)
    {
        for (sprite=0; sprite<NBSPRITE; sprite++)
        {
            TandyAnimation[frame][sprite] = ANIM_EGA[frame][sprite];
            TandyAnimation[frame][sprite].TandyScale = 1;
            TandyAnimation[frame][sprite].TandyClip =
                (sprite <= 10 || sprite == NBSPRITE-1) ? 1 : 0;
            if (scale != 3 || sprite < TANDY_OBJECT_FIRST ||
                sprite > TANDY_OBJECT_LAST)
                continue;

            sourceAdr = ANIM_EGA[frame][sprite].MemAdr;
            sourceX = sourceAdr % 40;
            sourceY = sourceAdr / 40;
            targetX = sourceX * 3 + TANDY_OBJECT_X_SHIFT_GROUPS;
            targetY = sourceY * 3 + TANDY_OBJECT_Y_SHIFT_LINES;
            while (targetX < 0)
            {
                targetX += 40;
                targetY--;
            }
            while (targetX >= 40)
            {
                targetX -= 40;
                targetY++;
            }
            TandyAnimation[frame][sprite].MemAdr =
                (unsigned short)(targetY * 40 + targetX);
            TandyAnimation[frame][sprite].TandyScale = 3;
        }
    }
}

/***********************************TIMER**************************************/

INTERRUPT_HANDLER BIOStimer = NULL;
static int TimerOK = 0;
unsigned long nbTicks = 0, nbTicksFps = 0;
static unsigned long TandyBiosCycles = 0;
static unsigned short TandyTimerDivisor = 0;

static void interrupt MyTimer()
{
    disable(); /* asm cli */

    if (nbTicksFps <= 1)
    {
        scr->needRefresh = 1;
    }
    else
    {
        /* allow screen refresh */
        nbTicks++;
        if (nbTicks >= nbTicksFps)
        {
            nbTicks = 0;
            scr->needRefresh = 1;
        }
    }

    /* tells 8259 in the PC that the interrupt has been processed */
    if (scr != NULL && scr->videoDevice == vTandy)
    {
        TandyMusicTick();

        /* Keep DOS time and BIOS timer callbacks advancing on the Tandy path.
           At the high-rate IRQ, chain the original IRQ0 handler once per
           accumulated 65,536 PIT clocks; it performs the PIC acknowledgement. */
        TandyBiosCycles += TandyTimerDivisor;
        if (TandyBiosCycles >= 65536L)
        {
            TandyBiosCycles -= 65536L;
            _chain_intr(BIOStimer);
            return;
        }
    }

    outportb(0x20, 0x20);

    enable(); /* asm sti */
}

void KillMyTimer()
{
    if (TimerOK)
    {
        disable(); /* asm cli */

        if (scr != NULL && scr->videoDevice == vTandy)
            TandyMusicStop();

        /* Set the timer back to its original settings (18.2Hz) */
        outportb(0x43, 0x36);
        outportb(0x40, 0xFF);
        outportb(0x40, 0xFF);

        /* restore original timer */
        setvect(8, BIOStimer);
        TimerOK = 0;

        enable(); /* asm sti */
    }
}

void InitMyTimer(unsigned long fps)
{
    int countdown;
    unsigned long Frequency;
    /* Frequency must be >= 19 (1193180/65535),
       otherwise countdown is larger than 4 bytes */
    for (Frequency = fps; Frequency < 19; Frequency *= 2)
    {
        ;
    }

    /* The Tandy note and percussion scheduler needs a 60 Hz IRQ. Keep the
       requested frame rate by refreshing only every Frequency/fps ticks. */
    if (scr != NULL && scr->videoDevice == vTandy && Frequency < 60)
        Frequency = 60;

    nbTicksFps = Frequency/fps;
    countdown = (int)(1193180L / Frequency);
    nbTicks = 0;

    if (TimerOK)
        KillMyTimer();

    disable(); /* asm cli */

    BIOStimer = getvect(8);
    TandyTimerDivisor = (unsigned short)countdown;
    TandyBiosCycles = 0;

    /* Set timer 0 up with the specified playback frequency */
    outportb(0x43, 0x36);
    outportb(0x40, lo(countdown));
    outportb(0x40, hi(countdown));

    setvect(8, MyTimer);
    TimerOK = 1;

    if (scr != NULL && scr->videoDevice == vTandy)
        TandyMusicStart(Frequency);

    enable(); /* asm sti */
}

/******************************************************************************/

int CreateSoundBlasterSample(char *filename, int forcedspversion, unsigned short dmasize)
{
    int retval = FALSE;
    SB_SAMPLE *sb_presample = PreLoadWAV(filename);

    if (sb_presample != NULL)
    {
        dma_buffer = createDmaTransfer(dmasize, sb_presample->bps);
        if (dma_buffer == NULL)
        {
            printf("error creating DMA transfer\n");
        }
        else
        {
            sb_card = createSBCard(forcedspversion);
            if (sb_card == NULL)
            {
                printf("error setting up SB card\n");
            }
            else
            {
                if (testCardCapabilities(sb_card, sb_presample) == FALSE)
                {
                    printf("Can't play a %s bits %s %u Hz sample\non a %s bits %s [%u,%u] Hz card\n",
                           (sb_presample->bps==2)?"16":"8", (sb_presample->channels==2)?"stereo":"mono", sb_presample->samplingrate,
                           (sb_card->support16bits)?"16":"8", (sb_card->supportStereo)?"stereo":"mono",
                           (sb_presample->channels==2)?sb_card->supportMinStereo:sb_card->supportMinMono,
                           (sb_presample->channels==2)?sb_card->supportMaxStereo:sb_card->supportMaxMono);

                    dma_buffer->freeDmaBuffer(dma_buffer);
                }
                else
                {
                    sb_sample = LoadWAV(filename, 2*dmasize, dma_buffer->buf_part[0]);
                    if (sb_sample != NULL)
                    {
                        sb_sample->loop = TRUE;
                        printf("Playing a %s bits %s %u Hz sample\n",
                               (sb_sample->bps==2)?"16":"8", (sb_sample->channels==2)?"stereo":"mono", sb_sample->samplingrate);

                        sb_card->init(sb_card, sb_sample, dma_buffer);
                        sb_card->play();
                        retval = TRUE;
                    }
                }
            }
        }
        free(sb_presample);
    }
    return retval;
}

void FreeSoundBlasterSample()
{
    if (dma_buffer != NULL)
        dma_buffer->stopDmaTransfer(dma_buffer);
    if (sb_card != NULL)
        sb_card->stop();
    if (dma_buffer != NULL)
        dma_buffer->freeDmaBuffer(dma_buffer);
    if (sb_sample != NULL)
        freeRamBuffer(sb_sample->sample);
}

int get_scancode()
{
    union REGS regs;

    while (!kbhit())
    {
        ;
    }

    regs.h.ah = 0;
    int86(0x16, &regs, &regs);
    return regs.h.ah;
}

VIDEODEVICE askVideoDevice(VIDEODEVICE videoDevice)
{
    VIDEODEVICE retval = vNULL;
    int video;

    printf("\nPlease select a video device\n");
    switch (videoDevice)
    {
    case vVGA:
    case vVGAH:
        printf("\t4 : VGA 320x200x256c\n");
        printf("\t3 : VGA 640x400x16c\n");
    case vEGAH:
        printf("\t2 : EGA 640x350x16c\n");
    case vEGA:
        printf("\t1 : EGA 320x200x16c\n");
    case vPlantronicsColorPlus:
        printf("\t0 : Plantronics ColorPlus 320x200x16c\n");
        break;
    }
    video = get_scancode();
    switch (video)
    {
    case 0x05:
    case 0x4b:
        retval = vVGA;
        break;
    case 0x04:
    case 0x51:
        retval = vVGAH;
        break;
    case 0x03:
    case 0x50:
        retval = vEGAH;
        break;
    case 0x02:
    case 0x4f:
        retval = vEGA;
        break;
    case 0x0b:
    case 0x52:
        retval = vPlantronicsColorPlus;
        break;
    }

    return retval;
}

int main(int argc, char **argv)
{
    int fps = 15;
    int tandyMode = FALSE;
    int tandyScale = 3;
    unsigned char *samplename = "NYANLOOP.WAV";
    int dspversion = 0;
    unsigned short dmasize = 4096;
    int i;

    for (i=1; i<argc; i++)
    {
        if (strcmp(argv[i], "/tandy") == 0)
            tandyMode = TRUE;
        if (strcmp(argv[i], "/tandy1x") == 0)
        {
            tandyMode = TRUE;
            tandyScale = 1;
        }
        if ((strlen(argv[i]) > 5) && (strncmp(argv[i], "/fps:", 5) == 0))
        {
            int f = strtol(&argv[i][5], NULL, 10);
            if ((f>0) && (f<=30))
                fps = f;
        }
        if ((strlen(argv[i]) > 5) && (strncmp(argv[i], "/dma:", 5) == 0))
        {
            long f = strtol(&argv[i][5], NULL, 10);
            if ((f>0) && (f<=32768))
                dmasize = f;
        }
        if ((strlen(argv[i]) > 5) && (strncmp(argv[i], "/dsp:", 5) == 0))
        {
            dspversion = strtol(&argv[i][5], NULL, 16);
        }
        if ((strlen(argv[i]) > 5) && (strncmp(argv[i], "/wav:", 5) == 0))
        {
            samplename = &argv[i][5];
        }
    }

    printf("********************************************************************************");
    printf("*                               NyanCat for DOS                                *");
    printf("********************************************************************************");
    printf("* Graphics: prguitarman                                                        *");
    printf("* Music: daniwell                                                              *");
    printf("* Code: OlivierP                                                               *");
    printf("********************************************************************************");

    scr = CreateScreen(tandyMode ? vTandy : vNULL);
    if (scr == NULL)
    {
        printf("requires an EGA/VGA graphic card\n");
    }
    else
    {
        SPRITEPOS (*ANIM_ptr)[NBANIM][NBSPRITE] = NULL;
        VIDEODEVICE videoMode = tandyMode ? vTandy : askVideoDevice(scr->videoDevice);
        unsigned char alpha = 0x0f, back = 10;

        if (videoMode != scr->videoDevice)
        {
            scr = DeleteScreen(scr);
            scr = CreateScreen(videoMode);
        }

        if (!tandyMode)
            CreateSoundBlasterSample(samplename, dspversion, dmasize);

        Screen_SetGraphicMode(scr);
        switch (scr->videoDevice)
        {
        case vTandy:
            {
                /* The Tandy blitter restores original VGA palette indices
                   from the low-plane-first EGA source stream. Map those
                   indices to the user's RGBI reference palette. */
                static const unsigned char PALET_TANDY[16] =
                    {15, 2, 14, 3, 7, 7, 14, 7, 5, 12, 1, 12, 0, 9, 0, 1};
                scr->SetPalette(PALET_TANDY, 16);
                BuildTandyAnimation(tandyScale);
                if (tandyScale == 3)
                    Screen_TandySetStarClip(80, 68, 247, 130);
                else
                    Screen_TandySetStarClip(128, 76, 183, 96);
                ANIM_ptr = &TandyAnimation;
                break;
            }

        case vPlantronicsColorPlus:
            Screen_SetPalette16c(PALET_EGA, 16);
            ANIM_ptr = &ANIM_CGA;
            back = alpha = 0x02; /* no palette, RGBI value */
            break;
        case vEGA:
            Screen_SetPalette16c(PALET_EGA, 16);
            ANIM_ptr = &ANIM_EGA;
            break;
        case vEGAH:
            Screen_SetPalette16c(PALET_EGAH, 16);
            ANIM_ptr = &ANIM_EGAH;
            break;
        case vVGAH:
            Screen_SetPalette16c(PALET_EGAH, 16);
            ANIM_ptr = &ANIM_EGAH;
            break;
        case vVGA:
            Screen_SetPalette256c(PALET_VGA, sizeof(PALET_VGA)/3);
            ANIM_ptr = &ANIM_VGA;
            break;
        }

        if (ANIM_ptr != NULL)
        {
            int i, j;

            scr->EraseScreen(scr, back);
            Screen_ChangeVideoPage(scr);
            scr->EraseScreen(scr, back);
            Screen_ChangeVideoPage(scr);

            InitMyTimer(fps);
            while (!kbhit())
            {
                for (i=0; i<NBANIM; i++)
                {
                    if (scr->videoDevice == vTandy)
                    {
                        /* Star/erase layers are clipped to the stable union
                           of foreground pixels, then the white overlay stars
                           are composited behind the enlarged cat and rainbow.
                           This prevents a live-page erase from showing through
                           the slower 3x foreground redraw. */
                        for (j=0; j<=10; j++)
                            scr->PrintSprite(scr, &(*ANIM_ptr)[i][j]);
                        scr->PrintSpriteTr(scr, &(*ANIM_ptr)[i][NBSPRITE-1], alpha);
                        for (j=TANDY_OBJECT_FIRST; j<=TANDY_OBJECT_LAST; j++)
                            scr->PrintSprite(scr, &(*ANIM_ptr)[i][j]);
                    }
                    else
                    {
                        for (j=0; j<NBSPRITE-1; j++)
                            scr->PrintSprite(scr, &(*ANIM_ptr)[i][j]);
                        scr->PrintSpriteTr(scr, &(*ANIM_ptr)[i][NBSPRITE-1], alpha);
                    }
                    Screen_ChangeVideoPage(scr);
                }
            }
            get_scancode();
            KillMyTimer();
        }

        Screen_SetVideoMode(0x0003);

        FreeSoundBlasterSample();

        scr = DeleteScreen(scr);
    }

    return 0;
}
