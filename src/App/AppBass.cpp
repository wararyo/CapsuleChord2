#include <App/AppBass.h>
#include <M5Unified.h>

void AppBass::updateUi()
{
    // 有効/無効スイッチ
    if (isActive) lv_obj_add_state(switchButton, LV_STATE_CHECKED);
    else lv_obj_clear_flag(switchButton, LV_STATE_CHECKED);
}

void AppBass::onCreate()
{
    isActive = false;
    tempoCallbacks.app = this;
    chordFilter.app = this;
}

void AppBass::onActivate()
{
    isActive = true;
    previousTime = Tempo.getMusicalTime();
    Tempo.addListener(&tempoCallbacks);
    context->pipeline->addChordFilter(&chordFilter);
}

void AppBass::onDeactivate()
{
    isActive = false;
    Tempo.removeListener(&tempoCallbacks);
    context->pipeline->removeChordFilter(&chordFilter);
}

void AppBass::onShowGui(lv_obj_t *container)
{
    // アプリタイトル
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 4);

    // 有効/無効スイッチ
    switchButton = lv_switch_create(container);
    lv_obj_set_size(switchButton, 136, 80);
    lv_obj_center(switchButton);
    lv_obj_add_event_cb(switchButton, [](lv_event_t *e) {
        auto *self = (AppBass *)lv_event_get_user_data(e);
        // ここでisActiveを反転させる
        self->isActive = !self->isActive;
        // 以降のisActiveの値は変更後の値
        if (self->isActive)
        {
            self->previousTime = Tempo.getMusicalTime();
            Tempo.addListener(&self->tempoCallbacks);
            self->context->pipeline->addChordFilter(&self->chordFilter);
        }
        else
        {
            Tempo.removeListener(&self->tempoCallbacks);
            self->context->pipeline->removeChordFilter(&self->chordFilter);
        }
        self->updateUi();
    }, LV_EVENT_CLICKED, (void *)this);

    updateUi();
}

void AppBass::onHideGui()
{
    lv_obj_del(titleLabel);
    lv_obj_del(switchButton);
}

void AppBass::onDestroy()
{
}

void AppBass::ChordFilter::onChordOn(Chord chord)
{
    chord.octave = 3;
    chord.inversion = 0;
    
    // Handle slash chord if bass note is specified
    if (chord.bass != Chord::BASS_DEFAULT) {
        // Get the chord notes before making any changes
        std::vector<uint8_t> chordNotes = chord.toMidiNoteNumbers();
        bool bassNoteInChord = false;
        uint8_t bassNoteValue = (chord.octave * 12) + chord.bass;
        
        // Check if the bass note is part of the chord
        for (uint8_t i = 0; i < chordNotes.size(); i++) {
            // Compare ignoring octave (modulo 12)
            if (chordNotes[i] % 12 == chord.bass) {
                bassNoteInChord = true;
                
                // Adjust inversion to make the bass note the bottom note
                chord.inversion = i;
                break;
            }
        }
        
        // If bass note is not part of the chord, ensure all chord notes are higher than the bass note
        if (!bassNoteInChord) {
            // Reset chord settings to calculate properly
            chord.inversion = 0;
            
            // Count how many notes are below the bass note
            chordNotes = chord.toMidiNoteNumbers();
            uint8_t notesBelow = 0;
            
            for (uint8_t note : chordNotes) {
                if (note < bassNoteValue) {
                    notesBelow++;
                }
            }
            
            // Set inversion to make all notes above bass note
            if (notesBelow > 0) {
                // If all notes are below, we need to increase octave
                if (notesBelow >= chordNotes.size()) {
                    chord.octave++;
                    chord.inversion = 0;
                } else {
                    chord.inversion = notesBelow;
                }
            }
            
            // Now convert chord to notes with appropriate inversion
            for (uint8_t note : chord.toMidiNoteNumbers()) {
                // Skip the root note as we'll replace it with the bass note
                if (note % 12 != chord.root) {
                    app->input.push_back(note);
                }
            }
            
            // Add the bass note
            app->input.push_back(bassNoteValue);
        }
    }
    else
    {
        // If not a slash chord, just add the chord notes
        for (uint8_t note : chord.toMidiNoteNumbers()) {
            app->input.push_back(note);
        }
    }
    app->updatePlayingNotes();
}

void AppBass::ChordFilter::onChordOff()
{
    app->input.clear();
    for (uint8_t note : app->output)
    {
        app->context->pipeline->sendNote(false, note, 0, 0x01);
    }
}

const struct AppBass::SequenceItem sequence[] = {
{0,0x91,0x30,0x7F},
{240,0x81,0x30,0x40},
{240,0x91,0x30,0x75},
{480,0x81,0x30,0x40},
{480,0x91,0x30,0x73},
{720,0x81,0x30,0x40},
{720,0x91,0x30,0x6F},
{960,0x81,0x30,0x40},
{960,0x91,0x30,0x6C},
{1200,0x81,0x30,0x40},
{1200,0x91,0x30,0x64},
{1440,0x81,0x30,0x40},
{1440,0x91,0x30,0x72},
{1680,0x81,0x30,0x40},
{1680,0x91,0x30,0x69},
{1920,0x81,0x30,0x40},
};

// inputの内容を、processItem内で使用できる形に変換する
void AppBass::updatePlayingNotes()
{
    if (input.empty()) return;
    std::list<uint_fast8_t> notes;
    uint_fast8_t octaveOffset = 0;
    do
    {
        for (uint8_t note : input)
        {
            notes.push_back(note + octaveOffset);
        }
        octaveOffset += 12;
    } while (notes.size() < 12);
    notes.sort();
    for (uint_fast8_t i = 0; i < 12; i++)
    {
        inputProcessed[i] = notes.front();
        notes.pop_front();
    }
}

void AppBass::processItem(const AppBass::SequenceItem &item)
{
    uint_fast8_t data1 = item.data1;
    uint_fast8_t index = data1 % 12;
    int_fast8_t octave = data1 / 12 - 5;
    uint8_t noteNo = inputProcessed[index] + octave * 12;

    if ((item.status & 0xF0) == 0x90)
    {
        context->pipeline->sendNote(true, noteNo, item.data2, item.status & 0x0F);
        output.push_back(noteNo);
        tempoCallbacks.shouldKnock = true;
    }
    else if ((item.status & 0xF0) == 0x80)
    {
        context->pipeline->sendNote(false, noteNo, item.data2, item.status & 0x0F);
        output.remove(noteNo);
    }
}

void AppBass::TempoCallbacks::onTick(TempoController::tick_timing_t timing, musical_time_t time)
{
    const musical_time_t timeInBar = time_in_bar(time);
    if (!app->isPlayingNotes())
    {
        app->previousTime = timeInBar;
        return;
    }
    const musical_time_t previousTime = app->previousTime;
    if (timeInBar == 0) {
        // 小節終わりのノート(ノートオフ想定)を発音する
        if (previousTime > 0) {
            for (const AppBass::SequenceItem &item : sequence)
            {
                if (previousTime < item.time && item.time <= 1920) app->processItem(item);
                else if (item.time > 1920) break;
            }
        }
        // 小節頭のノートを発音する
        for (const AppBass::SequenceItem &item : sequence)
        {
            if (item.time <= 0) app->processItem(item);
            else if (item.time > 0) break;
        }
    }
    else
    {
        // previousTimeより後でcurrentTimeと同じかそれより前のノートを発音する
        for (const AppBass::SequenceItem &item : sequence)
        {
            if (previousTime < item.time && item.time <= timeInBar) app->processItem(item);
            else if (item.time > timeInBar) break;
        }
    }

    // 必要ならばノックを行う
    if (shouldKnock)
    {
        app->context->knock(app);
        shouldKnock = false;
    }

    app->previousTime = timeInBar;
}
