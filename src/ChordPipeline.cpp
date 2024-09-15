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

// コードを再生する
// 呼び出し元がコードフィルターでない場合は、最初のフィルターに受け渡す
// 呼び出し元がコードフィルターである場合は、次のフィルターに受け渡す
// 次のフィルターが存在しない場合は、ノートに変換する
void ChordPipeline::playChord(Chord chord, ChordFilter *filter)
{
  // 次のフィルターのonChordOnを呼び出す
  // ただし次のフィルターがコードを変更しないものである場合はさらに次も呼び出す
  for (auto itr = filter == nullptr ? chordFilters.begin() : std::next(filter->getHandler()), e = chordFilters.end(); itr != e; ++itr) {
    (*itr)->onChordOn(chord);
    if ((*itr)->modifiesChord()) return;
  }

  // 最後のフィルターを通過したので、ノートに変換する
  auto notes = chord.toMidiNoteNumbers();
  sendNotes(false, playingNotes, 120);
  playingNotes.clear();
  playingNotes.insert(playingNotes.end(), notes.begin(), notes.end());
  sendNotes(true, notes, 120);
}

// コードを停止する
// 呼び出し元がコードフィルターでない場合は、最初のフィルターに受け渡す
// 呼び出し元がコードフィルターである場合は、次のフィルターに受け渡す
// 次のフィルターが存在しない場合はコードを停止する
void ChordPipeline::stopChord(ChordFilter *filter)
{
  // 次のフィルターのonChordOffを呼び出す
  // ただし次のフィルターがコードを変更しないものである場合はさらに次も呼び出す
  for (auto itr = filter == nullptr ? chordFilters.begin() : std::next(filter->getHandler()), e = chordFilters.end(); itr != e; ++itr) {
    (*itr)->onChordOff();
    if ((*itr)->modifiesChord()) return;
  }

  // 最後のフィルターを通過したのでコードを停止する
  sendNotes(false, playingNotes, 120);
  playingNotes.clear();
}

ChordPipeline Pipeline;
