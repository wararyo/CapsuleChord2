#include "ChordPipeline.h"
#include "Output/MidiOutput.h"

void ChordPipeline::sendNote(bool isNoteOn, uint8_t note, uint8_t vel, uint8_t channel)
{
  if (isNoteOn)
  {
    // Midi.sendNote(0x90, note, vel);
    Output.Internal.NoteOn(note, vel, channel);
  }
  else
  {
    // Midi.sendNote(0x80, note, 0);
    Output.Internal.NoteOff(note, 0, channel);
  }
}

void ChordPipeline::sendNotes(bool isNoteOn, std::vector<uint8_t> notes, uint8_t vel, uint8_t channel)
{
  if (isNoteOn)
  {
    for (uint8_t n : notes)
    {
      // Midi.sendNote(0x90, n, vel);
      Output.Internal.NoteOn(n, vel, channel);
    }
  }
  else
  {
    for (uint8_t n : notes)
    {
      // Midi.sendNote(0x80, n, 0);
      Output.Internal.NoteOff(n, 0, channel);
    }
  }
}

void ChordPipeline::playChord(Chord chord)
{
  auto notes = chord.toMidiNoteNumbers();
  sendNotes(false, playingNotes, 120);
  playingNotes.clear();
  playingNotes.insert(playingNotes.end(), notes.begin(), notes.end());
  sendNotes(true, notes, 120);
  for (PipelineCallbacks *listener : listeners)
  {
    listener->onChordChanged(chord);
  }
}

void ChordPipeline::stopChord()
{
  sendNotes(false, playingNotes, 120);
  playingNotes.clear();
}

ChordPipeline Pipeline;
