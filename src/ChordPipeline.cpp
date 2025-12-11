#include "ChordPipeline.h"
#include "Output/MidiOutput.h"
#include <M5Unified.h>

// ノートを発音する
// 呼び出し元がコードフィルターでない場合は、最初のフィルターに受け渡す
// 呼び出し元がコードフィルターである場合は、次のフィルターに受け渡す
// 次のフィルターが存在しない場合は、出力に送る
void ChordPipeline::sendNote(bool isNoteOn, uint8_t note, uint8_t vel, uint8_t channel, NoteFilter *filter)
{
  // 次のフィルターのonNoteOnを呼び出す
  // ただし次のフィルターがコードを変更しないものである場合はさらに次も呼び出す
  for (auto itr = filter == nullptr ? noteFilters.begin() : std::next(filter->getHandler()), e = noteFilters.end(); itr != e; ++itr) {
    if (isNoteOn) (*itr)->onNoteOn(note, vel, channel);
    else (*itr)->onNoteOff(note, vel, channel);
    if ((*itr)->modifiesNote()) return;
  }

  // 最後のフィルターを通過したので、出力に送る
  if (isNoteOn) sendNoteOnRaw(note, vel, channel);
  else sendNoteOffRaw(note, vel, channel);
}

// 複数のノートを発音する
// 呼び出し元がコードフィルターでない場合は、最初のフィルターに受け渡す
// 呼び出し元がコードフィルターである場合は、次のフィルターに受け渡す
// 次のフィルターが存在しない場合は、出力に送る
void ChordPipeline::sendNotes(bool isNoteOn, std::vector<uint8_t> notes, uint8_t vel, uint8_t channel, NoteFilter *filter)
{
  // 次のフィルターのonNoteOnを呼び出す
  // ただし次のフィルターがコードを変更しないものである場合はさらに次も呼び出す
  for (auto itr = filter == nullptr ? noteFilters.begin() : std::next(filter->getHandler()), e = noteFilters.end(); itr != e; ++itr)
  {
    if (isNoteOn)
    {
      for (uint8_t n : notes)
      {
        (*itr)->onNoteOn(n, vel, channel);
      }
    }
    else
    {
      for (uint8_t n : notes)
      {
        (*itr)->onNoteOff(n, vel, channel);
      }
    }
    if ((*itr)->modifiesNote()) return;
  }

  if (isNoteOn)
  {
    for (uint8_t n : notes)
    {
      sendNoteOnRaw(n, vel, channel);
    }
  }
  else
  {
    for (uint8_t n : notes)
    {
      sendNoteOffRaw(n, vel, channel);
    }
  }
}

void ChordPipeline::sendNoteOnRaw(uint8_t note, uint8_t vel, uint8_t channel)
{
  Output.getCurrentOutput()->noteOn(note, vel, channel);
}

void ChordPipeline::sendNoteOffRaw(uint8_t note, uint8_t vel, uint8_t channel)
{
  Output.getCurrentOutput()->noteOff(note, vel, channel);
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
