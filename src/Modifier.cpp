#include "Modifier.h"

void thirdInvert(Chord *c){
  if(c->option & Chord::Minor) c->option &= ~(Chord::Minor);
  else c->option |= Chord::Minor;
}
void fifthFlat(Chord *c){
  c->option |= Chord::FifthFlat;
}
void augment(Chord *c){
  c->option |= Chord::Aug;
}
void sus4(Chord *c){
  c->option |= Chord::Sus4;
}
void seventhInvert(Chord *c){
  if(c->option & Chord::Seventh) {c->option &= ~(Chord::Seventh); c->option |= Chord::MajorSeventh;}
  else if(c->option & Chord::MajorSeventh) {c->option &= ~(Chord::MajorSeventh); c->option |= Chord::Seventh;}
}
void ninth(Chord *c){
  c->option |= Chord::Ninth;
}
void thirteenth(Chord *c){
  c->option |= Chord::Thirteenth;
}

void pitchUp(Chord *c) {
  if(c->root == 11) c->root = 0;
  else c->root++;
}
void pitchDown(Chord *c) {
  if(c->root == 0) c->root = 11;
  else c->root--;
}

void inversionUp(Chord *c) {
  if(c->inversion == c->toMidiNoteNumbers().size() - 1)
  {
    c->inversion = 0;
    c->octave++;
  }
  else c->inversion++;
}

void inversionDown(Chord *c) {
  if(c->inversion == 0)
  {
    c->inversion = c->toMidiNoteNumbers().size() - 1;
    c->octave--;
  }
  else c->inversion--;
}
