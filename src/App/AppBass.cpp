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
    Pipeline.addChordFilter(&chordFilter);
}

void AppBass::onDeactivate()
{
    isActive = false;
    Tempo.removeListener(&tempoCallbacks);
    Pipeline.removeChordFilter(&chordFilter);
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
            Pipeline.addChordFilter(&self->chordFilter);
        }
        else
        {
            Tempo.removeListener(&self->tempoCallbacks);
            Pipeline.removeChordFilter(&self->chordFilter);
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
    for (uint8_t note : chord.toMidiNoteNumbers())
    {
        app->input.push_back(note);
    }
    app->updatePlayingNotes();
}

void AppBass::ChordFilter::onChordOff()
{
    app->input.clear();
    for (uint8_t note : app->output)
    {
        Pipeline.sendNote(false, note, 0, 0x1);
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
        Pipeline.sendNote(true, noteNo, item.data2, item.status & 0x0F);
        output.push_back(noteNo);
    }
    else if ((item.status & 0xF0) == 0x80)
    {
        Pipeline.sendNote(false, noteNo, item.data2, item.status & 0x0F);
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

    app->previousTime = timeInBar;
}
