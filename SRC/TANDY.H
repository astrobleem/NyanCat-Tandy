/*******************************************************************************
* Tandy 1000 graphics and PSG support for NyanCat for DOS                      *
*******************************************************************************/

#ifndef TANDY_H
#define TANDY_H

/* Called from the existing IRQ0 timer while the Tandy mode owns it. */
void TandyMusicStart(unsigned long timerRate);
void TandyMusicTick(void);
void TandyMusicStop(void);

#endif
