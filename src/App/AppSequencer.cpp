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
    // 入力で発音中かつ出力で発音中でないノートを発音する
    for (uint8_t n : input)
    {
        if (std::find(output.begin(), output.end(), n) == output.end())
        {
            Pipeline.sendNote(true, n, 120, 0x0, &noteFilter);
        }
    }
    // 入力で発音中でなく出力で発音中のノートをオフにする
    for (uint8_t n : output)
    {
        if (std::find(input.begin(), input.end(), n) == input.end())
        {
            Pipeline.sendNote(false, n, 0, 0x0, &noteFilter);
        }
    }
    Pipeline.removeNoteFilter(&noteFilter);
    inputBuffer.clear();
    input.clear();
    output.clear();
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

        // 変更後のisActiveに応じた処理
        bool isActive = !self->isActive;
        if (isActive) self->onActivate();
        else self->onDeactivate();
    
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
        app->inputBuffer.push_back(note);
    }
    else Pipeline.sendNote(true, note, vel, channel, &app->noteFilter);
}

void AppSequencer::NoteFilter::onNoteOff(uint8_t note, uint8_t vel, uint8_t channel)
{
    if (channel == 0)
    {
        app->inputBuffer.remove(note);
    }
    else Pipeline.sendNote(false, note, vel, channel, &app->noteFilter);
}

const struct AppSequencer::SequenceItem sequence[] = {
{0,0x90,0x3C,0x6B},
{240,0x90,0x3D,0x5C},
{480,0x90,0x3E,0x64},
{720,0x80,0x3C,0x40},
{720,0x90,0x3C,0x50},
{960,0x90,0x3F,0x6F},
{1200,0x80,0x3E,0x40},
{1200,0x90,0x3E,0x64},
{1440,0x80,0x3D,0x40},
{1440,0x90,0x3D,0x52},
{1680,0x80,0x3C,0x40},
{1680,0x90,0x3C,0x4B},
{1920,0x80,0x3F,0x40},
{1920,0x80,0x3E,0x40},
{1920,0x80,0x3D,0x40},
{1920,0x80,0x3C,0x40}
};

// inputの内容を、processItem内で使用できる形に変換する
void AppSequencer::updatePlayingNotes()
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

void AppSequencer::processItem(const AppSequencer::SequenceItem &item)
{
    uint_fast8_t data1 = item.data1;
    uint_fast8_t index = data1 % 12;
    int_fast8_t octave = data1 / 12 - 5;
    uint8_t noteNo = inputProcessed[index] + octave * 12;

    if ((item.status & 0xF0) == 0x90)
    {
        Pipeline.sendNote(true, noteNo, item.data2, item.status & 0x0F, &noteFilter);
        output.push_back(noteNo);
    }
    else if ((item.status & 0xF0) == 0x80)
    {
        Pipeline.sendNote(false, noteNo, item.data2, item.status & 0x0F, &noteFilter);
        output.remove(noteNo);
    }
}

void AppSequencer::TempoCallbacks::onTick(TempoController::tick_timing_t timing, musical_time_t time)
{
    const musical_time_t timeInBar = time_in_bar(time);
    if (timing & TempoController::TICK_TIMING_FULL)
    {
        app->input = app->inputBuffer;
        app->updatePlayingNotes();
        // 入力から発音中のノートがなくなったら全ての出力中のノートをオフにする
        if (app->input.empty() && !app->output.empty())
        {
            for (uint8_t n : app->output)
            {
                Pipeline.sendNote(false, n, 0, 0x0, &app->noteFilter);
            }
            app->output.clear();
        }
    }
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
