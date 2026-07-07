#ifndef _MODIFIER_H_
#define _MODIFIER_H_

#include "Chord.h"

// root/option/bassにのみ作用する修飾はChord/DegreeChordの両方に適用できるためテンプレートにしている。
// ChordLikeはroot(0-11)・option・setBass()を持つこと。

template <typename ChordLike>
void thirdInvert(ChordLike *c){
  if(c->option & Chord::Minor) c->option &= ~(Chord::Minor);
  else c->option |= Chord::Minor;
}
template <typename ChordLike>
void fifthFlat(ChordLike *c){
  c->option |= Chord::FifthFlat;
}
template <typename ChordLike>
void augment(ChordLike *c){
  c->option |= Chord::Aug;
}
template <typename ChordLike>
void sus4(ChordLike *c){
  c->option |= Chord::Sus4;
}
template <typename ChordLike>
void seventhInvert(ChordLike *c){
  if(c->option & Chord::Seventh) {c->option &= ~(Chord::Seventh); c->option |= Chord::MajorSeventh;}
  else if(c->option & Chord::MajorSeventh) {c->option &= ~(Chord::MajorSeventh); c->option |= Chord::Seventh;}
}
template <typename ChordLike>
void ninth(ChordLike *c){
  c->option |= Chord::Ninth;
}
template <typename ChordLike>
void thirteenth(ChordLike *c){
  c->option |= Chord::Thirteenth;
}

template <typename ChordLike>
void pitchUp(ChordLike *c) {
  if(c->root == 11) c->root = 0;
  else c->root++;
}
template <typename ChordLike>
void pitchDown(ChordLike *c) {
  if(c->root == 0) c->root = 11;
  else c->root--;
}

template <typename ChordLike>
void blackAdder(ChordLike *c) {
  // First make the chord an augmented chord
  c->option &= ~(Chord::Minor | Chord::Dimish | Chord::Sus4 | Chord::Sus2 | Chord::FifthFlat); // Clear modifiers that would conflict with augmented
  c->option |= Chord::Aug; // Set it as augmented

  // Set the bass note to be 2 semitones higher than the root
  uint8_t bassNote = (c->root + 2) % 12;
  c->setBass(bassNote);
}

// 転回操作はoctave/toMidiNoteNumbers()に依存するためChord専用。
// calcInversion()の後にのみ使用する。
void inversionUp(Chord *c);
void inversionDown(Chord *c);

#endif
