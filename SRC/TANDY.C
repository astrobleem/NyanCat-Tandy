/*******************************************************************************
* Tandy 1000 320x200x16 graphics and NCR8496 PSG playback                      *
* Static note data: Tyler, "Nyan Cat", VGMusic MIDI entry (2019-07-25).        *
* The MIDI has a 1536-tick intro followed by an A/B main phrase. The melody's  *
* A/B pair repeats every 12288 ticks; the bass repeats every 6144 ticks.       *
* https://www.vgmusic.com/file/cb0b763c32686a5065b4f0f58d410cb2.html             *
*******************************************************************************/

#include <conio.h>
#include "tandy.h"
#include "lt_score.h"

#ifdef __WATCOMC__
#define outportb outp
#define inportb inp
#endif

#define TANDY_PSG_PORT 0x00c0
#define TANDY_PIT2_PORT 0x0042
#define TANDY_PIT_CONTROL 0x0043
#define TANDY_SPEAKER_PORT 0x0061
#define TANDY_INTRO_END 1536U
#define TANDY_LOOP_START TANDY_INTRO_END
#define TANDY_LOOP_END 13824U
#define TANDY_TEMPO_US 422535UL
#define TANDY_PSG_NOISE_WHITE_FIXED 0xe4

typedef lt_score_note TANDY_NOTE;
typedef lt_score_drum TANDY_DRUM_EVENT;

static const TANDY_NOTE TandyMelody[] =
{
    {0, 46, 180}, /* MIDI 75 */
    {48, 94, 170}, /* MIDI 76 */
    {96, 190, 151}, /* MIDI 78 */
    {192, 286, 113}, /* MIDI 83 */
    {288, 334, 180}, /* MIDI 75 */
    {336, 382, 170}, /* MIDI 76 */
    {384, 430, 151}, /* MIDI 78 */
    {432, 478, 113}, /* MIDI 83 */
    {480, 526, 101}, /* MIDI 85 */
    {528, 574, 90}, /* MIDI 87 */
    {576, 622, 101}, /* MIDI 85 */
    {624, 670, 120}, /* MIDI 82 */
    {672, 766, 113}, /* MIDI 83 */
    {768, 862, 151}, /* MIDI 78 */
    {864, 910, 180}, /* MIDI 75 */
    {912, 958, 170}, /* MIDI 76 */
    {960, 1054, 151}, /* MIDI 78 */
    {1056, 1150, 113}, /* MIDI 83 */
    {1152, 1198, 101}, /* MIDI 85 */
    {1200, 1246, 120}, /* MIDI 82 */
    {1248, 1294, 113}, /* MIDI 83 */
    {1296, 1342, 101}, /* MIDI 85 */
    {1344, 1390, 85}, /* MIDI 88 */
    {1392, 1438, 90}, /* MIDI 87 */
    {1440, 1486, 85}, /* MIDI 88 */
    {1488, 1534, 101}, /* MIDI 85 */
    {1536, 1630, 302}, /* MIDI 66 */
    {1632, 1726, 269}, /* MIDI 68 */
    {1728, 1774, 404}, /* MIDI 61 */
    {1776, 1870, 360}, /* MIDI 63 */
    {1872, 1918, 453}, /* MIDI 59 */
    {1920, 1966, 381}, /* MIDI 62 */
    {1968, 2014, 404}, /* MIDI 61 */
    {2016, 2110, 453}, /* MIDI 59 */
    {2112, 2206, 453}, /* MIDI 59 */
    {2208, 2302, 404}, /* MIDI 61 */
    {2304, 2398, 381}, /* MIDI 62 */
    {2400, 2446, 381}, /* MIDI 62 */
    {2448, 2494, 404}, /* MIDI 61 */
    {2496, 2542, 453}, /* MIDI 59 */
    {2544, 2590, 404}, /* MIDI 61 */
    {2592, 2638, 360}, /* MIDI 63 */
    {2640, 2686, 302}, /* MIDI 66 */
    {2688, 2734, 269}, /* MIDI 68 */
    {2736, 2782, 360}, /* MIDI 63 */
    {2784, 2830, 302}, /* MIDI 66 */
    {2832, 2878, 404}, /* MIDI 61 */
    {2880, 2926, 381}, /* MIDI 62 */
    {2928, 2974, 453}, /* MIDI 59 */
    {2976, 3022, 404}, /* MIDI 61 */
    {3024, 3070, 453}, /* MIDI 59 */
    {3072, 3166, 360}, /* MIDI 63 */
    {3168, 3262, 302}, /* MIDI 66 */
    {3264, 3310, 269}, /* MIDI 68 */
    {3312, 3358, 360}, /* MIDI 63 */
    {3360, 3406, 302}, /* MIDI 66 */
    {3408, 3454, 404}, /* MIDI 61 */
    {3456, 3502, 360}, /* MIDI 63 */
    {3504, 3550, 453}, /* MIDI 59 */
    {3552, 3598, 381}, /* MIDI 62 */
    {3600, 3646, 360}, /* MIDI 63 */
    {3648, 3694, 381}, /* MIDI 62 */
    {3696, 3742, 404}, /* MIDI 61 */
    {3744, 3790, 453}, /* MIDI 59 */
    {3792, 3838, 404}, /* MIDI 61 */
    {3840, 3934, 381}, /* MIDI 62 */
    {3936, 3982, 453}, /* MIDI 59 */
    {3984, 4030, 404}, /* MIDI 61 */
    {4032, 4078, 360}, /* MIDI 63 */
    {4080, 4126, 302}, /* MIDI 66 */
    {4128, 4174, 404}, /* MIDI 61 */
    {4176, 4222, 381}, /* MIDI 62 */
    {4224, 4270, 404}, /* MIDI 61 */
    {4272, 4318, 453}, /* MIDI 59 */
    {4320, 4414, 404}, /* MIDI 61 */
    {4416, 4510, 453}, /* MIDI 59 */
    {4512, 4606, 404}, /* MIDI 61 */
    {4608, 4702, 302}, /* MIDI 66 */
    {4704, 4798, 269}, /* MIDI 68 */
    {4800, 4846, 404}, /* MIDI 61 */
    {4848, 4942, 360}, /* MIDI 63 */
    {4944, 4990, 453}, /* MIDI 59 */
    {4992, 5038, 381}, /* MIDI 62 */
    {5040, 5086, 404}, /* MIDI 61 */
    {5088, 5182, 453}, /* MIDI 59 */
    {5184, 5278, 453}, /* MIDI 59 */
    {5280, 5374, 404}, /* MIDI 61 */
    {5376, 5470, 381}, /* MIDI 62 */
    {5472, 5518, 381}, /* MIDI 62 */
    {5520, 5566, 404}, /* MIDI 61 */
    {5568, 5614, 453}, /* MIDI 59 */
    {5616, 5662, 404}, /* MIDI 61 */
    {5664, 5710, 360}, /* MIDI 63 */
    {5712, 5758, 302}, /* MIDI 66 */
    {5760, 5806, 269}, /* MIDI 68 */
    {5808, 5854, 360}, /* MIDI 63 */
    {5856, 5902, 302}, /* MIDI 66 */
    {5904, 5950, 404}, /* MIDI 61 */
    {5952, 5998, 381}, /* MIDI 62 */
    {6000, 6046, 453}, /* MIDI 59 */
    {6048, 6094, 404}, /* MIDI 61 */
    {6096, 6142, 453}, /* MIDI 59 */
    {6144, 6238, 360}, /* MIDI 63 */
    {6240, 6334, 302}, /* MIDI 66 */
    {6336, 6382, 269}, /* MIDI 68 */
    {6384, 6430, 360}, /* MIDI 63 */
    {6432, 6478, 302}, /* MIDI 66 */
    {6480, 6526, 404}, /* MIDI 61 */
    {6528, 6574, 360}, /* MIDI 63 */
    {6576, 6622, 453}, /* MIDI 59 */
    {6624, 6670, 381}, /* MIDI 62 */
    {6672, 6718, 360}, /* MIDI 63 */
    {6720, 6766, 381}, /* MIDI 62 */
    {6768, 6814, 404}, /* MIDI 61 */
    {6816, 6862, 453}, /* MIDI 59 */
    {6864, 6910, 404}, /* MIDI 61 */
    {6912, 7006, 381}, /* MIDI 62 */
    {7008, 7054, 453}, /* MIDI 59 */
    {7056, 7102, 404}, /* MIDI 61 */
    {7104, 7150, 360}, /* MIDI 63 */
    {7152, 7198, 302}, /* MIDI 66 */
    {7200, 7246, 404}, /* MIDI 61 */
    {7248, 7294, 381}, /* MIDI 62 */
    {7296, 7342, 404}, /* MIDI 61 */
    {7344, 7390, 453}, /* MIDI 59 */
    {7392, 7486, 404}, /* MIDI 61 */
    {7488, 7582, 453}, /* MIDI 59 */
    {7584, 7678, 453}, /* MIDI 59 */
    {7680, 7774, 453}, /* MIDI 59 */
    {7776, 7822, 605}, /* MIDI 54 */
    {7824, 7870, 539}, /* MIDI 56 */
    {7872, 7966, 453}, /* MIDI 59 */
    {7968, 8014, 605}, /* MIDI 54 */
    {8016, 8062, 539}, /* MIDI 56 */
    {8064, 8110, 453}, /* MIDI 59 */
    {8112, 8158, 404}, /* MIDI 61 */
    {8160, 8206, 360}, /* MIDI 63 */
    {8208, 8254, 453}, /* MIDI 59 */
    {8256, 8302, 339}, /* MIDI 64 */
    {8304, 8350, 360}, /* MIDI 63 */
    {8352, 8398, 339}, /* MIDI 64 */
    {8400, 8446, 302}, /* MIDI 66 */
    {8448, 8542, 453}, /* MIDI 59 */
    {8544, 8638, 453}, /* MIDI 59 */
    {8640, 8686, 605}, /* MIDI 54 */
    {8688, 8734, 539}, /* MIDI 56 */
    {8736, 8782, 453}, /* MIDI 59 */
    {8784, 8830, 605}, /* MIDI 54 */
    {8832, 8878, 339}, /* MIDI 64 */
    {8880, 8926, 360}, /* MIDI 63 */
    {8928, 8974, 404}, /* MIDI 61 */
    {8976, 9022, 453}, /* MIDI 59 */
    {9024, 9070, 605}, /* MIDI 54 */
    {9072, 9118, 719}, /* MIDI 51 */
    {9120, 9166, 679}, /* MIDI 52 */
    {9168, 9214, 605}, /* MIDI 54 */
    {9216, 9310, 453}, /* MIDI 59 */
    {9312, 9358, 605}, /* MIDI 54 */
    {9360, 9406, 539}, /* MIDI 56 */
    {9408, 9502, 453}, /* MIDI 59 */
    {9504, 9550, 605}, /* MIDI 54 */
    {9552, 9598, 539}, /* MIDI 56 */
    {9600, 9646, 453}, /* MIDI 59 */
    {9648, 9694, 453}, /* MIDI 59 */
    {9696, 9742, 404}, /* MIDI 61 */
    {9744, 9790, 360}, /* MIDI 63 */
    {9792, 9838, 453}, /* MIDI 59 */
    {9840, 9886, 605}, /* MIDI 54 */
    {9888, 9934, 539}, /* MIDI 56 */
    {9936, 9982, 605}, /* MIDI 54 */
    {9984, 10078, 453}, /* MIDI 59 */
    {10080, 10126, 453}, /* MIDI 59 */
    {10128, 10174, 480}, /* MIDI 58 */
    {10176, 10222, 453}, /* MIDI 59 */
    {10224, 10270, 605}, /* MIDI 54 */
    {10272, 10318, 539}, /* MIDI 56 */
    {10320, 10366, 453}, /* MIDI 59 */
    {10368, 10414, 339}, /* MIDI 64 */
    {10416, 10462, 360}, /* MIDI 63 */
    {10464, 10510, 339}, /* MIDI 64 */
    {10512, 10558, 302}, /* MIDI 66 */
    {10560, 10654, 453}, /* MIDI 59 */
    {10656, 10750, 480}, /* MIDI 58 */
    {10752, 10846, 453}, /* MIDI 59 */
    {10848, 10894, 605}, /* MIDI 54 */
    {10896, 10942, 539}, /* MIDI 56 */
    {10944, 11038, 453}, /* MIDI 59 */
    {11040, 11086, 605}, /* MIDI 54 */
    {11088, 11134, 539}, /* MIDI 56 */
    {11136, 11182, 453}, /* MIDI 59 */
    {11184, 11230, 404}, /* MIDI 61 */
    {11232, 11278, 360}, /* MIDI 63 */
    {11280, 11326, 453}, /* MIDI 59 */
    {11328, 11374, 339}, /* MIDI 64 */
    {11376, 11422, 360}, /* MIDI 63 */
    {11424, 11470, 339}, /* MIDI 64 */
    {11472, 11518, 302}, /* MIDI 66 */
    {11520, 11614, 453}, /* MIDI 59 */
    {11616, 11710, 453}, /* MIDI 59 */
    {11712, 11758, 605}, /* MIDI 54 */
    {11760, 11806, 539}, /* MIDI 56 */
    {11808, 11854, 453}, /* MIDI 59 */
    {11856, 11902, 605}, /* MIDI 54 */
    {11904, 11950, 339}, /* MIDI 64 */
    {11952, 11998, 360}, /* MIDI 63 */
    {12000, 12046, 404}, /* MIDI 61 */
    {12048, 12094, 453}, /* MIDI 59 */
    {12096, 12142, 605}, /* MIDI 54 */
    {12144, 12190, 719}, /* MIDI 51 */
    {12192, 12238, 679}, /* MIDI 52 */
    {12240, 12286, 605}, /* MIDI 54 */
    {12288, 12382, 453}, /* MIDI 59 */
    {12384, 12430, 605}, /* MIDI 54 */
    {12432, 12478, 539}, /* MIDI 56 */
    {12480, 12574, 453}, /* MIDI 59 */
    {12576, 12622, 605}, /* MIDI 54 */
    {12624, 12670, 539}, /* MIDI 56 */
    {12672, 12718, 453}, /* MIDI 59 */
    {12720, 12766, 453}, /* MIDI 59 */
    {12768, 12814, 404}, /* MIDI 61 */
    {12816, 12862, 360}, /* MIDI 63 */
    {12864, 12910, 453}, /* MIDI 59 */
    {12912, 12958, 605}, /* MIDI 54 */
    {12960, 13006, 539}, /* MIDI 56 */
    {13008, 13054, 605}, /* MIDI 54 */
    {13056, 13150, 453}, /* MIDI 59 */
    {13152, 13198, 453}, /* MIDI 59 */
    {13200, 13246, 480}, /* MIDI 58 */
    {13248, 13294, 453}, /* MIDI 59 */
    {13296, 13342, 605}, /* MIDI 54 */
    {13344, 13390, 539}, /* MIDI 56 */
    {13392, 13438, 453}, /* MIDI 59 */
    {13440, 13486, 339}, /* MIDI 64 */
    {13488, 13534, 360}, /* MIDI 63 */
    {13536, 13582, 339}, /* MIDI 64 */
    {13584, 13630, 302}, /* MIDI 66 */
    {13632, 13726, 453}, /* MIDI 59 */
    {13728, 13822, 404}, /* MIDI 61 */
};

static const TANDY_NOTE TandyBass[] =
{
    /* Source MIDI pitches 35-44 are voiced +12 semitones for the PSG. */
    {1536, 1728, 679}, /* MIDI 52 */
    {1728, 1920, 605}, /* MIDI 54 */
    {1920, 2112, 719}, /* MIDI 51 */
    {2112, 2304, 539}, /* MIDI 56 */
    {2304, 2496, 807}, /* MIDI 49 */
    {2496, 2688, 605}, /* MIDI 54 */
    {2688, 2880, 906}, /* MIDI 47 */
    {2880, 2976, 807}, /* MIDI 49 */
    {2976, 3072, 719}, /* MIDI 51 */
    {3072, 3264, 679}, /* MIDI 52 */
    {3264, 3456, 605}, /* MIDI 54 */
    {3456, 3648, 719}, /* MIDI 51 */
    {3648, 3840, 539}, /* MIDI 56 */
    {3840, 4032, 807}, /* MIDI 49 */
    {4032, 4128, 605}, /* MIDI 54 */
    {4128, 4224, 906}, /* MIDI 47 */
    {4224, 4416, 906}, /* MIDI 47 */
    {4416, 4608, 906}, /* MIDI 47 */
    {4608, 4800, 679}, /* MIDI 52 */
    {4800, 4992, 605}, /* MIDI 54 */
    {4992, 5184, 719}, /* MIDI 51 */
    {5184, 5376, 539}, /* MIDI 56 */
    {5376, 5568, 807}, /* MIDI 49 */
    {5568, 5760, 605}, /* MIDI 54 */
    {5760, 5952, 906}, /* MIDI 47 */
    {5952, 6048, 807}, /* MIDI 49 */
    {6048, 6144, 719}, /* MIDI 51 */
    {6144, 6336, 679}, /* MIDI 52 */
    {6336, 6528, 605}, /* MIDI 54 */
    {6528, 6720, 719}, /* MIDI 51 */
    {6720, 6912, 539}, /* MIDI 56 */
    {6912, 7104, 807}, /* MIDI 49 */
    {7104, 7200, 605}, /* MIDI 54 */
    {7200, 7296, 906}, /* MIDI 47 */
    {7296, 7488, 906}, /* MIDI 47 */
    {7488, 7680, 906}, /* MIDI 47 */
    {7680, 7872, 679}, /* MIDI 52 */
    {7872, 8064, 605}, /* MIDI 54 */
    {8064, 8256, 719}, /* MIDI 51 */
    {8256, 8448, 539}, /* MIDI 56 */
    {8448, 8640, 807}, /* MIDI 49 */
    {8640, 8832, 605}, /* MIDI 54 */
    {8832, 9024, 906}, /* MIDI 47 */
    {9024, 9120, 807}, /* MIDI 49 */
    {9120, 9216, 719}, /* MIDI 51 */
    {9216, 9408, 679}, /* MIDI 52 */
    {9408, 9600, 605}, /* MIDI 54 */
    {9600, 9792, 719}, /* MIDI 51 */
    {9792, 9984, 539}, /* MIDI 56 */
    {9984, 10176, 807}, /* MIDI 49 */
    {10176, 10272, 605}, /* MIDI 54 */
    {10272, 10368, 906}, /* MIDI 47 */
    {10368, 10560, 906}, /* MIDI 47 */
    {10560, 10752, 906}, /* MIDI 47 */
    {10752, 10944, 679}, /* MIDI 52 */
    {10944, 11136, 605}, /* MIDI 54 */
    {11136, 11328, 719}, /* MIDI 51 */
    {11328, 11520, 539}, /* MIDI 56 */
    {11520, 11712, 807}, /* MIDI 49 */
    {11712, 11904, 605}, /* MIDI 54 */
    {11904, 12096, 906}, /* MIDI 47 */
    {12096, 12192, 807}, /* MIDI 49 */
    {12192, 12288, 719}, /* MIDI 51 */
    {12288, 12480, 679}, /* MIDI 52 */
    {12480, 12672, 605}, /* MIDI 54 */
    {12672, 12864, 719}, /* MIDI 51 */
    {12864, 13056, 539}, /* MIDI 56 */
    {13056, 13248, 807}, /* MIDI 49 */
    {13248, 13344, 605}, /* MIDI 54 */
    {13344, 13440, 906}, /* MIDI 47 */
    {13440, 13632, 906}, /* MIDI 47 */
    {13632, 13824, 906}, /* MIDI 47 */
};

static const TANDY_NOTE TandyHarmony[] =
{
    {1632, 1696, 339}, /* MIDI 64 */
    {1824, 1888, 202}, /* MIDI 73 */
    {2016, 2080, 360}, /* MIDI 63 */
    {2208, 2272, 180}, /* MIDI 75 */
    {2400, 2464, 404}, /* MIDI 61 */
    {3168, 3232, 226}, /* MIDI 71 */
    {3936, 4000, 404}, /* MIDI 61 */
    {4320, 4384, 302}, /* MIDI 66 */
    {4512, 4576, 453}, /* MIDI 59 */
    {4704, 4768, 226}, /* MIDI 71 */
    {4896, 4960, 302}, /* MIDI 66 */
    {5088, 5152, 240}, /* MIDI 70 */
    {5280, 5344, 269}, /* MIDI 68 */
    {5472, 5536, 269}, /* MIDI 68 */
    {6240, 6304, 339}, /* MIDI 64 */
    {7008, 7072, 269}, /* MIDI 68 */
    {7392, 7456, 453}, /* MIDI 59 */
    {7584, 7648, 302}, /* MIDI 66 */
    {7776, 7840, 339}, /* MIDI 64 */
    {7968, 8032, 202}, /* MIDI 73 */
    {8544, 8608, 404}, /* MIDI 61 */
    {9312, 9376, 226}, /* MIDI 71 */
    {9504, 9568, 302}, /* MIDI 66 */
    {10080, 10144, 269}, /* MIDI 68 */
    {10656, 10720, 453}, /* MIDI 59 */
    {10848, 10912, 226}, /* MIDI 71 */
    {11040, 11104, 302}, /* MIDI 66 */
    {11616, 11680, 269}, /* MIDI 68 */
    {12384, 12448, 339}, /* MIDI 64 */
    {12576, 12640, 202}, /* MIDI 73 */
    {13152, 13216, 404}, /* MIDI 61 */
    {13728, 13792, 302}, /* MIDI 66 */
};

enum
{
    TANDY_DRUM_HAT = 1,
    TANDY_DRUM_SNARE = 2,
    TANDY_DRUM_CRASH = 3,
    TANDY_DRUM_ADDED_HAT = 4
};

static const TANDY_DRUM_EVENT TandyDrums[] =
{
    {0, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {192, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {576, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {960, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {1152, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {1248, TANDY_DRUM_HAT}, /* MIDI percussion */
    {1344, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {1440, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {1488, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {1536, TANDY_DRUM_CRASH}, /* MIDI percussion */
    {1632, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {1728, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {1824, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {2016, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {2112, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {2208, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {2400, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {2496, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {2592, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {2784, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {2880, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {2976, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {3168, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {3264, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {3360, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {3552, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {3648, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {3744, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {3936, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {4032, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {4128, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {4320, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {4416, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {4512, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {4704, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {4800, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {4896, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {5088, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {5184, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {5280, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {5472, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {5568, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {5664, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {5856, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {5952, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {6048, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {6240, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {6336, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {6432, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {6624, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {6720, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {6816, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {7008, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {7104, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {7200, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {7392, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {7488, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {7584, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {7680, TANDY_DRUM_CRASH}, /* MIDI percussion */
    {7776, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {7872, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {7968, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {8160, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {8256, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {8352, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {8544, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {8640, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {8736, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {8928, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {9024, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {9120, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {9312, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {9408, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {9504, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {9696, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {9792, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {9888, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {10080, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {10176, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {10272, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {10464, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {10560, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {10656, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {10848, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {10944, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {11040, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {11232, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {11328, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {11424, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {11616, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {11712, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {11808, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {12000, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {12096, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {12192, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {12384, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {12480, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {12576, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {12768, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {12864, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {12960, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {13152, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {13248, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {13344, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {13536, TANDY_DRUM_ADDED_HAT}, /* added offbeat hat */
    {13632, TANDY_DRUM_SNARE}, /* MIDI percussion */
    {13728, TANDY_DRUM_SNARE}, /* MIDI percussion */
};

static const lt_score_kick TandyKickTicks[] =
{
    { 384 },
    { 768 },
    { 960 },
    { 1152 },
    { 1536 },
    { 1920 },
    { 2304 },
    { 2688 },
    { 3072 },
    { 3456 },
    { 3840 },
    { 4224 },
    { 4608 },
    { 4992 },
    { 5376 },
    { 5760 },
    { 6144 },
    { 6528 },
    { 6912 },
    { 7296 },
    { 7680 },
    { 8064 },
    { 8448 },
    { 8832 },
    { 9216 },
    { 9600 },
    { 9984 },
    { 10368 },
    { 10752 },
    { 11136 },
    { 11520 },
    { 11904 },
    { 12288 },
    { 12672 },
    { 13056 },
    { 13440 }
};

#define TANDY_MELODY_COUNT ((unsigned short)(sizeof(TandyMelody) / sizeof(TandyMelody[0])))
#define TANDY_BASS_COUNT ((unsigned short)(sizeof(TandyBass) / sizeof(TandyBass[0])))
#define TANDY_HARMONY_COUNT ((unsigned short)(sizeof(TandyHarmony) / sizeof(TandyHarmony[0])))
#define TANDY_DRUM_COUNT ((unsigned short)(sizeof(TandyDrums) / sizeof(TandyDrums[0])))
#define TANDY_KICK_COUNT ((unsigned short)(sizeof(TandyKickTicks) / sizeof(TandyKickTicks[0])))

static unsigned char TandySpeakerOwned = 0;

static void TandySpeakerSetPeriod(unsigned short divisor)
{
    outportb(TANDY_PIT_CONTROL, 0xb6); /* channel 2, LSB/MSB, mode 3 */
    outportb(TANDY_PIT2_PORT, (unsigned char)(divisor & 0xff));
    outportb(TANDY_PIT2_PORT, (unsigned char)(divisor >> 8));
}

static unsigned char TandySpeakerPortValue(unsigned char current,
                                          unsigned char enabled)
{
    /* Preserve the live, unrelated port 61 bits; we own only bits 0 and 1. */
    return (unsigned char)((current & 0xfc) | (enabled ? 0x03 : 0x00));
}

static void TandySpeakerGate(unsigned char enabled)
{
    unsigned char current = inportb(TANDY_SPEAKER_PORT);
    /* Callers run with IRQs masked (startup/shutdown) or inside IRQ0. */
    outportb(TANDY_SPEAKER_PORT,
             TandySpeakerPortValue(current, enabled));
}

static void TandySpeakerAcquire(void)
{
    unsigned char current;

    current = inportb(TANDY_SPEAKER_PORT);
    /*
     * Do not take PIT2 when either speaker gate is active. A PIT2 program
     * with both gates disabled is not observable here. The 8253 exposes its
     * current count, not the original reload/mode, so that state cannot be
     * faithfully restored after using PIT2 for the short kick accents.
     */
    if (current & 0x03)
    {
        TandySpeakerOwned = 0;
        return;
    }

    TandySpeakerOwned = 1;
}

static void TandySpeakerRestore(void)
{
    if (!TandySpeakerOwned)
        return;

    TandySpeakerGate(0);
    TandySpeakerOwned = 0;
}

#ifdef NYANCAT_CAPTURE_TEST
int TandyCapturePort61Regression(void)
{
    unsigned int sample;
    unsigned char acquired;
    unsigned char live;
    unsigned char enabled;
    unsigned char muted;

    for (sample = 0; sample < 64; ++sample)
    {
        acquired = (unsigned char)(sample << 2);
        /* Model other upper-bit state changing while our lease is active. */
        live = (unsigned char)(((acquired ^ 0xa4) & 0xfc) | (sample & 3));
        if ((acquired & 0xfc) == (live & 0xfc))
            return 0;

        enabled = TandySpeakerPortValue(live, 1);
        muted = TandySpeakerPortValue(live, 0);
        if (enabled != (unsigned char)((live & 0xfc) | 3))
            return 0;
        if (muted != (unsigned char)(live & 0xfc))
            return 0;
    }
    return 1;
}
#endif


/* Nyan owns one LTSCORE player and one LTA handle for PSG channels 0..3. */
static lt_chip TandyChip;
static lta_handle TandyAudio;
static lt_score_player TandyPlayer;
static unsigned short TandyPsgPort = TANDY_PSG_PORT;
static unsigned char TandyAudioInitialized = 0;
static unsigned char TandyMusicEnabled = 0;

extern void lt_dos_emit(void *context, lt_u8 byte);

static const lt_score TandyNyanScore =
{
    LTSCORE_VERSION,
    (unsigned char)(TANDY_PSG_NOISE_WHITE_FIXED & 7),
    192U,
    TANDY_LOOP_START,
    TANDY_LOOP_END,
    TANDY_TEMPO_US,
    { TandyMelody, TandyBass, TandyHarmony },
    { TANDY_MELODY_COUNT, TANDY_BASS_COUNT, TANDY_HARMONY_COUNT },
    { 2U, 7U, 10U },
    { 6U, 10U, 8U },
    TandyDrums,
    TANDY_DRUM_COUNT,
    TandyKickTicks,
    TANDY_KICK_COUNT
};

static int TandyLibtandyInitialize(void)
{
    if (TandyAudioInitialized)
        return 1;
    lt_init(&TandyChip, lt_dos_emit, &TandyPsgPort);
    if (lta_init(&TandyAudio, &TandyChip, 1U) != LTA_OK)
        return 0;
    if (lt_score_init(&TandyPlayer, &TandyAudio) != LTA_OK)
        return 0;
    TandyAudioInitialized = 1;
    return 1;
}

static void TandySpeakerSink(void *context, lt_u16 divisor, lt_u8 gate)
{
    (void)context;
    if (!TandySpeakerOwned)
        return;

    if (gate == LTSCORE_SPEAKER_GATE_ON)
    {
        TandySpeakerSetPeriod(divisor);
        TandySpeakerGate(1);
    }
    else if (gate == LTSCORE_SPEAKER_KEEP_GATE)
        TandySpeakerSetPeriod(divisor);
    else if (gate == LTSCORE_SPEAKER_GATE_OFF)
        TandySpeakerGate(0);
}

void TandyMusicStart(unsigned long timerRate)
{
    lta_request request;
    lta_status status;

    TandyMusicEnabled = 0;
    if (TandyPlayer.active || TandyAudio.active)
        TandyMusicStop();
    TandySpeakerRestore();

    if (!TandyLibtandyInitialize())
        return;

    lta_request_init(&request);
    request.op = LTA_OP_ACQUIRE;
    request.mask = 15U;
    status = lta_command(&TandyAudio, &request);
    if (status != LTA_OK)
        return;

    TandySpeakerAcquire();
    status = lt_score_start(&TandyPlayer, &TandyNyanScore,
                            (lta_u32)timerRate, TandySpeakerSink, 0);
    if (status != LTA_OK)
    {
        TandyMusicStop();
        return;
    }
    TandyMusicEnabled = 1;
}

void TandyMusicTick(void)
{
    if (!TandyMusicEnabled)
        return;
    if (lt_score_tick(&TandyPlayer) != LTA_OK)
        TandyMusicStop();
}

void TandyMusicStop(void)
{
    lta_request request;
    lta_status status = LTA_OK;
    unsigned char playerWasActive = 0;

    TandyMusicEnabled = 0;
    if (TandyAudioInitialized && TandyPlayer.active)
    {
        playerWasActive = 1;
        status = lt_score_stop(&TandyPlayer);
    }

    /* score_stop already closes an owned speaker gate through this sink. */
    if (playerWasActive && status == LTA_OK)
        TandySpeakerOwned = 0;
    else
        TandySpeakerRestore();

    if (TandyAudioInitialized && TandyAudio.active)
    {
        lta_request_init(&request);
        request.op = LTA_OP_RELEASE;
        (void)lta_command(&TandyAudio, &request);
    }
}
