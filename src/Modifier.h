#ifndef _MODIFIER_H_
#define _MODIFIER_H_

#include "Chord.h"

void thirdInvert(Chord *c);
void fifthFlat(Chord *c);
void augment(Chord *c);
void sus4(Chord *c);
void seventhInvert(Chord *c);
void ninth(Chord *c);
void thirteenth(Chord *c);
void pitchUp(Chord *c);
void pitchDown(Chord *c);
void inversionUp(Chord *c);
void inversionDown(Chord *c);
void blackAdder(Chord *c);

#endif