#include <App/AppSequencer.h>
#include <M5Unified.h>
#include <Output/MidiOutput.h>

const auto sequencePiano = std::make_shared<std::list<AppSequencer::SequenceItem>>(std::list<AppSequencer::SequenceItem>{
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
});

const auto sequenceGuitar = std::make_shared<std::list<AppSequencer::SequenceItem>>(std::list<AppSequencer::SequenceItem>{
{0,0x90,0x30,0x69},
{0,0x90,0x32,0x5D},
{0,0x90,0x3C,0x69},
{0,0x90,0x3D,0x69},
{0,0x90,0x3E,0x69},
{0,0x90,0x3F,0x69},
{240,0x90,0x32,0x1B},
{240,0x90,0x3C,0x17},
{240,0x90,0x3D,0x27},
{240,0x90,0x3E,0x22},
{240,0x90,0x3F,0x27},
{480,0x90,0x30,0x51},
{480,0x90,0x32,0x45},
{480,0x90,0x3C,0x43},
{480,0x90,0x3D,0x43},
{480,0x90,0x3E,0x43},
{480,0x90,0x3F,0x43},
{720,0x80,0x30,0x40},
{720,0x80,0x32,0x40},
{720,0x80,0x3C,0x40},
{720,0x80,0x3D,0x40},
{720,0x80,0x3E,0x40},
{720,0x80,0x3F,0x40},
{720,0x90,0x3C,0x32},
{720,0x90,0x3D,0x32},
{720,0x90,0x3E,0x32},
{720,0x90,0x3F,0x32},
{780,0x80,0x3C,0x40},
{780,0x80,0x3D,0x40},
{780,0x80,0x3E,0x40},
{780,0x80,0x3F,0x40},
{840,0x90,0x30,0x6D},
{840,0x90,0x32,0x61},
{840,0x90,0x3C,0x6D},
{840,0x90,0x3D,0x6D},
{840,0x90,0x3E,0x6D},
{840,0x90,0x3F,0x6D},
{1080,0x90,0x3C,0x47},
{1080,0x90,0x3D,0x47},
{1080,0x90,0x3E,0x47},
{1080,0x90,0x3F,0x47},
{1200,0x90,0x30,0x30},
{1200,0x90,0x32,0x24},
{1200,0x90,0x3C,0x30},
{1200,0x90,0x3D,0x30},
{1200,0x90,0x3E,0x30},
{1200,0x90,0x3F,0x30},
{1440,0x90,0x30,0x51},
{1440,0x90,0x32,0x45},
{1440,0x90,0x3C,0x51},
{1440,0x90,0x3D,0x51},
{1440,0x90,0x3E,0x51},
{1440,0x90,0x3F,0x51},
{1680,0x80,0x30,0x40},
{1680,0x80,0x32,0x40},
{1680,0x80,0x3C,0x40},
{1680,0x80,0x3D,0x40},
{1680,0x80,0x3E,0x40},
{1680,0x80,0x3F,0x40},
{1680,0x90,0x30,0x2A},
{1680,0x90,0x32,0x1E},
{1680,0x90,0x3C,0x1F},
{1680,0x90,0x3D,0x1F},
{1680,0x90,0x3E,0x1F},
{1680,0x90,0x3F,0x1F},
{1740,0x80,0x30,0x40},
{1740,0x80,0x32,0x40},
{1740,0x80,0x3C,0x40},
{1740,0x80,0x3D,0x40},
{1740,0x80,0x3E,0x40},
{1740,0x80,0x3F,0x40},
{1800,0x90,0x3C,0x3A},
{1800,0x90,0x3D,0x3A},
{1800,0x90,0x3E,0x3A},
{1800,0x90,0x3F,0x3A},
{1920,0x80,0x3C,0x40},
{1920,0x80,0x3D,0x40},
{1920,0x80,0x3E,0x40},
{1920,0x80,0x3F,0x40}
});

const auto sequenceSynth = std::make_shared<std::list<AppSequencer::SequenceItem>>(std::list<AppSequencer::SequenceItem>{
{0,0x90,0x30,0x41},
{0,0x90,0x32,0x41},
{0,0x90,0x3C,0x74},
{0,0x90,0x3D,0x74},
{0,0x90,0x3E,0x74},
{0,0x90,0x3F,0x74},
{360,0x80,0x30,0x40},
{360,0x80,0x32,0x40},
{360,0x80,0x3C,0x40},
{360,0x80,0x3D,0x40},
{360,0x80,0x3E,0x40},
{360,0x80,0x3F,0x40},
{360,0x90,0x30,0x41},
{360,0x90,0x32,0x41},
{360,0x90,0x3C,0x74},
{360,0x90,0x3D,0x74},
{360,0x90,0x3E,0x74},
{360,0x90,0x3F,0x74},
{480,0x80,0x30,0x40},
{480,0x80,0x32,0x40},
{480,0x80,0x3C,0x40},
{480,0x80,0x3D,0x40},
{480,0x80,0x3E,0x40},
{480,0x80,0x3F,0x40},
{720,0x90,0x30,0x41},
{720,0x90,0x32,0x41},
{720,0x90,0x3C,0x74},
{720,0x90,0x3D,0x74},
{720,0x90,0x3E,0x74},
{720,0x90,0x3F,0x74},
{960,0x80,0x30,0x40},
{960,0x80,0x32,0x40},
{960,0x80,0x3C,0x40},
{960,0x80,0x3D,0x40},
{960,0x80,0x3E,0x40},
{960,0x80,0x3F,0x40},
{1200,0x90,0x30,0x41},
{1200,0x90,0x32,0x41},
{1200,0x90,0x3C,0x74},
{1200,0x90,0x3D,0x74},
{1200,0x90,0x3E,0x74},
{1200,0x90,0x3F,0x74},
{1560,0x80,0x30,0x40},
{1560,0x80,0x32,0x40},
{1560,0x80,0x3C,0x40},
{1560,0x80,0x3D,0x40},
{1560,0x80,0x3E,0x40},
{1560,0x80,0x3F,0x40},
{1560,0x90,0x30,0x41},
{1560,0x90,0x32,0x41},
{1560,0x90,0x3C,0x74},
{1560,0x90,0x3D,0x74},
{1560,0x90,0x3E,0x74},
{1560,0x90,0x3F,0x74},
{1680,0x80,0x30,0x40},
{1680,0x80,0x32,0x40},
{1680,0x80,0x3C,0x40},
{1680,0x80,0x3D,0x40},
{1680,0x80,0x3E,0x40},
{1680,0x80,0x3F,0x40}
});

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
    currentSequence = sequencePiano;
}

void AppSequencer::onActivate()
{
    isActive = true;
    previousTime = Tempo.getMusicalTime();
    Tempo.addListener(&tempoCallbacks);
    context->pipeline->addNoteFilter(&noteFilter);
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
            context->pipeline->sendNote(true, n, 120, 0x0, &noteFilter);
        }
    }
    // 入力で発音中でなく出力で発音中のノートをオフにする
    for (uint8_t n : output)
    {
        if (std::find(input.begin(), input.end(), n) == input.end())
        {
            context->pipeline->sendNote(false, n, 0, 0x0, &noteFilter);
        }
    }
    context->pipeline->removeNoteFilter(&noteFilter);
    inputBuffer.clear();
    input.clear();
    output.clear();
}

void AppSequencer::onShowGui(lv_obj_t *container)
{
    // アプリタイトル
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 0, 4);

    // 有効/無効スイッチ
    switchButton = lv_switch_create(container);
    lv_obj_set_size(switchButton, 136, 80);
    lv_obj_align(switchButton, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(switchButton, [](lv_event_t *e) {
        auto *self = (AppSequencer *)lv_event_get_user_data(e);

        // 変更後のisActiveに応じた処理
        bool isActive = !self->isActive;
        if (isActive) self->onActivate();
        else self->onDeactivate();
    
        self->updateUi();
    }, LV_EVENT_CLICKED, (void *)this);

    // 音色ラベル
    lv_obj_t *timbreLabel = lv_label_create(container);
    lv_label_set_text(timbreLabel, "音色変更ボタン(仮)");
    lv_obj_align(timbreLabel, LV_ALIGN_TOP_MID, 0, 94);

    // ピアノボタン
    lv_obj_t *pianoButton = lv_btn_create(container);
    lv_obj_set_size(pianoButton, 200, 40);
    lv_obj_align(pianoButton, LV_ALIGN_BOTTOM_MID, 0, -132);
    lv_obj_add_event_cb(pianoButton, [](lv_event_t *e) {
        auto *self = (AppSequencer *)lv_event_get_user_data(e);
        Output.Internal.loadPiano();
        self->currentSequence = sequencePiano;
    }, LV_EVENT_CLICKED, (void *)this);
    lv_obj_t *pianoLabel = lv_label_create(pianoButton);
    lv_label_set_text(pianoLabel, "ピアノ");
    lv_obj_center(pianoLabel);

    // AGuitarボタン
    lv_obj_t *aGuitarButton = lv_btn_create(container);
    lv_obj_set_size(aGuitarButton, 200, 40);
    lv_obj_align(aGuitarButton, LV_ALIGN_BOTTOM_MID, 0, -88);
    lv_obj_add_event_cb(aGuitarButton, [](lv_event_t *e) {
        auto *self = (AppSequencer *)lv_event_get_user_data(e);
        Output.Internal.loadAGuitar();
        self->currentSequence = sequenceGuitar;
    }, LV_EVENT_CLICKED, (void *)this);
    lv_obj_t *aGuitarLabel = lv_label_create(aGuitarButton);
    lv_label_set_text(aGuitarLabel, "アコギ");
    lv_obj_center(aGuitarLabel);

    // EPianoボタン
    lv_obj_t *ePianoButton = lv_btn_create(container);
    lv_obj_set_size(ePianoButton, 200, 40);
    lv_obj_align(ePianoButton, LV_ALIGN_BOTTOM_MID, 0, -44);
    lv_obj_add_event_cb(ePianoButton, [](lv_event_t *e) {
        auto *self = (AppSequencer *)lv_event_get_user_data(e);
        Output.Internal.loadEPiano();
        self->currentSequence = sequenceSynth;
    }, LV_EVENT_CLICKED, (void *)this);
    lv_obj_t *ePianoLabel = lv_label_create(ePianoButton);
    lv_label_set_text(ePianoLabel, "エレピ");
    lv_obj_center(ePianoLabel);

    // SuperSawボタン
    lv_obj_t *superSawButton = lv_btn_create(container);
    lv_obj_set_size(superSawButton, 200, 40);
    lv_obj_align(superSawButton, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(superSawButton, [](lv_event_t *e) {
        auto *self = (AppSequencer *)lv_event_get_user_data(e);
        Output.Internal.loadSuperSaw();
        self->currentSequence = sequenceSynth;
    }, LV_EVENT_CLICKED, (void *)this);
    lv_obj_t *superSawLabel = lv_label_create(superSawButton);
    lv_label_set_text(superSawLabel, "スパソ");
    lv_obj_center(superSawLabel);

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
    else app->context->pipeline->sendNote(true, note, vel, channel, &app->noteFilter);
}

void AppSequencer::NoteFilter::onNoteOff(uint8_t note, uint8_t vel, uint8_t channel)
{
    if (channel == 0)
    {
        app->inputBuffer.remove(note);
    }
    else app->context->pipeline->sendNote(false, note, vel, channel, &app->noteFilter);
}

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
        context->pipeline->sendNote(true, noteNo, item.data2, item.status & 0x0F, &noteFilter);
        output.push_back(noteNo);
        tempoCallbacks.shouldKnock = true;
    }
    else if ((item.status & 0xF0) == 0x80)
    {
        context->pipeline->sendNote(false, noteNo, item.data2, item.status & 0x0F, &noteFilter);
        output.remove(noteNo);
    }
}

void AppSequencer::TempoCallbacks::onTick(TempoController::tick_timing_t timing, musical_time_t time)
{
    const musical_time_t timeInBar = time_in_bar(time);
    // コードを切り替えるときに音が途切れないようにするために、4分音符の間隔で入力を更新する
    // ただし直前が無音だった場合は即座に鳴らす
    if ((timing & TempoController::TICK_TIMING_FULL) ||
        (app->output.empty() && !app->inputBuffer.empty()))
    {
        app->input = app->inputBuffer;
        app->updatePlayingNotes();
        // 入力から発音中のノートがなくなったら全ての出力中のノートをオフにする
        if (app->input.empty() && !app->output.empty())
        {
            for (uint8_t n : app->output)
            {
                app->context->pipeline->sendNote(false, n, 0, 0x0, &app->noteFilter);
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
            for (const AppSequencer::SequenceItem &item : *(app->currentSequence))
            {
                if (previousTime < item.time && item.time <= 1920) app->processItem(item);
                else if (item.time > 1920) break;
            }
        }
        // 小節頭のノートを発音する
        for (const AppSequencer::SequenceItem &item : *(app->currentSequence))
        {
            if (item.time <= 0) app->processItem(item);
            else if (item.time > 0) break;
        }
    }
    else
    {
        // previousTimeより後でcurrentTimeと同じかそれより前のノートを発音する
        for (const AppSequencer::SequenceItem &item : *(app->currentSequence))
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
