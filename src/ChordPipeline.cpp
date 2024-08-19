#include "ChordPipeline.h"
#include "Output/MidiOutput.h"

void ChordPipeline::sendNotes(bool isNoteOn, std::vector<uint8_t> notes, int vel) {
  if(isNoteOn) {
    for(uint8_t n : notes) {
      // Midi.sendNote(0x90, n, vel);
      Output.Internal.NoteOn(n, vel, 0);
    }
    playingNotes.insert(playingNotes.end(),notes.begin(),notes.end());
  }
  else {
    for(uint8_t n : playingNotes) {
      // Midi.sendNote(0x80, n, 0);
      Output.Internal.NoteOff(n, 0, 0);
    }
    playingNotes.clear();
  }
}

void ChordPipeline::playChord(Chord chord) {
  sendNotes(true,chord.toMidiNoteNumbers(),120);
  for(PipelineCallbacks* listener : listeners) {
    listener->onChordChanged(chord);
  }
}

ChordPipeline Pipeline;
