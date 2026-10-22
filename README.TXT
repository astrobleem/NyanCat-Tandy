                            *******************
                            * NyanCat for DOS *
                            *******************

After the Thomson TO8 release, here is the TO16 version of the famous NyanCat.
The Thomson TO16 is a French PC XT computer from the late eighties.

This software should work on any PC XT with a Plantronics CGA graphic card
or an EGA graphic card. 
A SoundBlaster is required for the sound playback. The BLASTER environment 
variable in AUTOEXEC.BAT must be set with the proper values.

You can change the following parameters :
    /tandy (Tandy 1000/PCjr 320x200x16 graphics and PSG audio; 3x pixel art)
    /tandy1x (use the original-size Tandy animation)
    /fps:15 (with a value from 1 to 30)
    /dsp:201 (with an hexa value for the DSP version)
    /dma:4096 (with a value from 1 to 32768 for the dma buffer size)
    /wav:filename.wav (with the name of another WAV file)
If there's not enough conventional memory, XMS memory will be used if 
available.

Use /tandy on a Tandy 1000 or PCjr-compatible system to select 320x200
16-color graphics, enlarge the cat and rainbow 3x with crisp pixel replication,
and play a score-based Nyan Cat arrangement on its PSG. Use /tandy1x to keep
the original animation size.
This mode does not require a Sound Blaster. The normal video selection and
Sound Blaster WAV playback remain the default.

The Tandy renderer preserves the original source palette indices and maps them
to the reference-guided RGBI colors, including the gray cat, yellow crust, red
sprinkles, and six distinct rainbow bands.

The Tandy arrangement uses static note and percussion events from Tyler's
Nyan Cat MIDI on VGMusic (credited in SRC/TANDY.C). It plays the 1536-tick
intro once, then loops the score's repeating A/B phrase (12288 ticks at the
MIDI tempo). The lead keeps its MIDI pitches; the low bass line is voiced one
octave higher for the SN76496 divider range. A quiet PSG tone adds bass-root
and fifth offbeats when the lead has room. PSG white-noise percussion follows
the MIDI kick, snare, crash, and its single high-hat event. The restrained
offbeat high-hat pattern is an added arrangement, not part of that MIDI data.
Short PC-speaker kick accents follow the MIDI bass-drum events when the
speaker is free. The speaker is acquired only if port 61's speaker gate bits
are clear at startup. A program that has configured PIT channel 2 while leaving
those gates clear cannot be detected. DOS exposes the live PIT count but not
its prior mode or reload value, so shutdown mutes the speaker gate and leaves
channel 2 in the mode used for the accents rather than claiming to restore
unreadable timer state. Tandy mode runs its musical scheduler at 60 Hz while
keeping the requested video frame rate; generic PC/Sound Blaster timing is
unchanged.

This release is derived from the original NyanCat-XT project at
https://github.com/OlivierP-To8/NyanCat-XT. The original source tree and
GPL-3.0 license are retained. The Tandy score uses the included LibTandy
static-score/PSG modules under MIT; those sources are compiled into the DOS
executable, so there is no separate runtime library or DLL to install. The
included third_party/libtandy/README.md and LICENSE identify that subset.

To build from the project root with Open Watcom C16 1.9 installed and on PATH:
    wmake -f MAKEFILE.WCC
This creates NYANCAT.EXE. Run NYANCAT.EXE /tandy on a Tandy 1000 or PCjr
compatible machine for the 320x200x16 PSG mode. The Tandy mode does not require
a Sound Blaster. The default non-Tandy mode keeps the original Sound Blaster
WAV path and requirements above.

