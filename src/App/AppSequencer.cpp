#include <App/AppSequencer.h>
#include <M5Unified.h>

void AppSequencer::updateUi()
{
    // 有効/無効スイッチ
    if (isActive) lv_obj_add_state(switchButton, LV_STATE_CHECKED);
    else lv_obj_clear_flag(switchButton, LV_STATE_CHECKED);
}

void AppSequencer::onCreate()
{
    isActive = false;
    tempoCallbacks.app = this;
    noteFilter.app = this;
}

void AppSequencer::onActivate()
{
    isActive = true;
    previousTime = Tempo.getMusicalTime();
    Tempo.addListener(&tempoCallbacks);
    Pipeline.addNoteFilter(&noteFilter);
}

void AppSequencer::onDeactivate()
{
    isActive = false;
    Tempo.removeListener(&tempoCallbacks);
    Pipeline.removeNoteFilter(&noteFilter);
}

void AppSequencer::onShowGui(lv_obj_t *container)
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
        auto *self = (AppSequencer *)lv_event_get_user_data(e);
        // ここでisActiveを反転させる
        self->isActive = !self->isActive;
        // 以降のisActiveの値は変更後の値
        if (self->isActive)
        {
            self->previousTime = Tempo.getMusicalTime();
            Tempo.addListener(&self->tempoCallbacks);
            Pipeline.addNoteFilter(&self->noteFilter);
        }
        else
        {
            Tempo.removeListener(&self->tempoCallbacks);
            Pipeline.removeNoteFilter(&self->noteFilter);
        }
        self->updateUi();
    }, LV_EVENT_CLICKED, (void *)this);

    updateUi();
}

void AppSequencer::onHideGui()
{
    lv_obj_del(titleLabel);
    lv_obj_del(switchButton);
}

void AppSequencer::onDestroy()
{
}

void AppSequencer::NoteFilter::onNoteOn(uint8_t note, uint8_t vel, uint8_t channel)
{
    if (channel == 0)
    {
        app->playingNotes.push_back(note);
        app->updatePlayingNotes();
    }
    else Pipeline.sendNote(true, note, vel, channel, &app->noteFilter);
}

void AppSequencer::NoteFilter::onNoteOff(uint8_t note, uint8_t vel, uint8_t channel)
{
    if (channel == 0)
    {
        app->playingNotes.remove(note);
        app->updatePlayingNotes();
    }
    else Pipeline.sendNote(false, note, vel, channel, &app->noteFilter);
}

const struct AppSequencer::SequenceItem sequence[] = {
{0,0x90,0x3C,0x6B},
{240,0x80,0x3C,0x40},
{240,0x90,0x3D,0x5C},
{480,0x80,0x3D,0x40},
{480,0x90,0x3E,0x64},
{720,0x80,0x3E,0x40},
{720,0x90,0x3C,0x50},
{960,0x80,0x3C,0x40},
{960,0x90,0x3F,0x6F},
{1200,0x80,0x3F,0x40},
{1200,0x90,0x3E,0x64},
{1440,0x80,0x3E,0x40},
{1440,0x90,0x3D,0x52},
{1680,0x80,0x3D,0x40},
{1680,0x90,0x3C,0x4B},
{1920,0x80,0x3C,0x40},
};

// playingNotesの内容を、processItem内で使用できる形に変換する
void AppSequencer::updatePlayingNotes()
{
    if (playingNotes.empty()) return;
    std::list<uint_fast8_t> notes;
    uint_fast8_t octaveOffset = 0;
    do
    {
        for (uint8_t note : playingNotes)
        {
            notes.push_back(note + octaveOffset);
        }
        octaveOffset += 12;
    } while (notes.size() < 12);
    notes.sort();
    for (uint_fast8_t i = 0; i < 12; i++)
    {
        playingNotesProcessed[i] = notes.front();
        notes.pop_front();
    }
}

void AppSequencer::processItem(const AppSequencer::SequenceItem &item)
{
    uint_fast8_t data1 = item.data1;
    uint_fast8_t index = data1 % 12;
    int_fast8_t octave = data1 / 12 - 5;
    uint8_t noteNo = playingNotesProcessed[index] + octave * 12;

    if ((item.status & 0xF0) == 0x90)
    {
        Pipeline.sendNote(true, noteNo, item.data2, item.status & 0x0F, &noteFilter);
    }
    else if ((item.status & 0xF0) == 0x80)
    {
        Pipeline.sendNote(false, noteNo, item.data2, item.status & 0x0F, &noteFilter);
    }
}

void AppSequencer::TempoCallbacks::onTick(TempoController::tick_timing_t timing, musical_time_t time)
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
            for (const AppSequencer::SequenceItem &item : sequence)
            {
                if (previousTime < item.time && item.time <= 1920) app->processItem(item);
                else if (item.time > 1920) break;
            }
        }
        // 小節頭のノートを発音する
        for (const AppSequencer::SequenceItem &item : sequence)
        {
            if (item.time <= 0) app->processItem(item);
            else if (item.time > 0) break;
        }
    }
    else
    {
        // previousTimeより後でcurrentTimeと同じかそれより前のノートを発音する
        for (const AppSequencer::SequenceItem &item : sequence)
        {
            if (previousTime < item.time && item.time <= timeInBar) app->processItem(item);
            else if (item.time > timeInBar) break;
        }
    }

    app->previousTime = timeInBar;
}
