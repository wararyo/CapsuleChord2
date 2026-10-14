#include "Modifier.h"

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
