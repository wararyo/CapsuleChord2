#include "Tempo.h"
#include "M5Unified.h"
#include <esp_timer.h>
#include <esp_log.h>
#include <climits>

static const char *TEMPO_LOG_TAG = "Tempo";

musical_time_t TempoController::barLengthOf(TimeSignature sig)
{
    // 4分音符1つを480として計算する
    // 分母 (denom) は 4 を基準とし、num 個分の長さを返す
    // 例: 4/4 → 480 * 4 = 1920, 3/4 → 480 * 3 = 1440
    // 6/8 の場合は 480 * (8/8) * 6 / 2 = 1440 となるが、現状は denom=4 前提
    if (sig.denom == 0) return 1920;
    musical_time_t quarterUnit = 480 * 4 / sig.denom;
    return quarterUnit * sig.num;
}

musical_time_t TempoController::barLength() const
{
    return barLengthOf(currentSig);
}

musical_time_t TempoController::intervalOf(size_t idx) const
{
    switch (idx)
    {
    case IDX_BAR:          return barLengthOf(currentSig);
    case IDX_FULL:         return INTERVAL_FULL;
    case IDX_FULL_TRIPLET: return INTERVAL_FULL_TRIPLET;
    case IDX_HALF:         return INTERVAL_HALF;
    case IDX_HALF_TRIPLET: return INTERVAL_HALF_TRIPLET;
    case IDX_QUARTER:      return INTERVAL_QUARTER;
    case IDX_EIGHTH:       return INTERVAL_EIGHTH;
    default:               return INT32_MAX;
    }
}

TempoController::tick_timing_t TempoController::maskOf(size_t idx)
{
    switch (idx)
    {
    case IDX_BAR:          return TICK_TIMING_BAR;
    case IDX_FULL:         return TICK_TIMING_FULL;
    case IDX_FULL_TRIPLET: return TICK_TIMING_FULL_TRIPLET;
    case IDX_HALF:         return TICK_TIMING_HALF;
    case IDX_HALF_TRIPLET: return TICK_TIMING_HALF_TRIPLET;
    case IDX_QUARTER:      return TICK_TIMING_QUARTER;
    case IDX_EIGHTH:       return TICK_TIMING_EIGHTH;
    default:               return 0;
    }
}

musical_time_t TempoController::computeCurrentMusicalTimeLocked() const
{
    if (!isPlaying) return 0;
    int64_t now = esp_timer_get_time();
    int64_t delta = now - realTimeBase;
    if (delta < 0) delta = 0;
    // (delta_us * tempo * 480) / 60_000_000
    int64_t mt = (int64_t)musicalTimeBase + (delta * (int64_t)tempo * 480) / 60000000LL;
    if (mt > INT32_MAX) mt = INT32_MAX;
    return (musical_time_t)mt;
}

musical_time_t TempoController::getMusicalTime() const
{
    portENTER_CRITICAL(&mutex);
    musical_time_t result = computeCurrentMusicalTimeLocked();
    portEXIT_CRITICAL(&mutex);
    return result;
}

musical_time_t TempoController::timeInBar() const
{
    portENTER_CRITICAL(&mutex);
    musical_time_t now = computeCurrentMusicalTimeLocked();
    musical_time_t bl = barLengthOf(currentSig);
    musical_time_t anchor = barAnchor;
    portEXIT_CRITICAL(&mutex);
    if (bl <= 0) return 0;
    musical_time_t d = now - anchor;
    musical_time_t r = d % bl;
    if (r < 0) r += bl;
    return r;
}

musical_time_t TempoController::timeInBar(musical_time_t t) const
{
    portENTER_CRITICAL(&mutex);
    musical_time_t bl = barLengthOf(currentSig);
    musical_time_t anchor = barAnchor;
    portEXIT_CRITICAL(&mutex);
    if (bl <= 0) return 0;
    musical_time_t d = t - anchor;
    musical_time_t r = d % bl;
    if (r < 0) r += bl;
    return r;
}

void TempoController::setTempo(tempo_t newTempo)
{
    std::list<TempoCallbacks *> listenersCopy;
    portENTER_CRITICAL(&mutex);
    if (isPlaying)
    {
        // アンカーを現在時刻に更新してからテンポを変更することで
        // 再生中のテンポ変更をシームレスに反映する
        int64_t now = esp_timer_get_time();
        int64_t delta = now - realTimeBase;
        if (delta < 0) delta = 0;
        int64_t advance = (delta * (int64_t)tempo * 480) / 60000000LL;
        musicalTimeBase = (musical_time_t)((int64_t)musicalTimeBase + advance);
        realTimeBase = now;
    }
    tempo = newTempo;
    listenersCopy = listeners;
    portEXIT_CRITICAL(&mutex);

    for (TempoCallbacks *listener : listenersCopy)
    {
        listener->onTempoChanged(newTempo);
    }
}

void TempoController::begin()
{
    // 呼び出し元はシングルスレッド (setup() およびメインループ経由の play()) であることを前提とする。
    // 複数スレッドから同時に呼ぶとキュー／タスクを二重生成する TOCTOU があるため、
    // 並行呼び出しが必要になった場合はここをロックで保護すること。
    // 既に初期化済みなら何もしない (多重呼び出し・遅延初期化の両方に対応)
    if (tickQueue != nullptr && dispatchTask != nullptr) return;

    if (tickQueue == nullptr)
    {
        // 1ms ごとに最大 MAX_FIRES(32) 個発火しうるが、配送タスクが追従できる前提で
        // バースト耐性として余裕をもったキュー長を確保する
        tickQueue = xQueueCreate(64, sizeof(TickInfo));
        if (tickQueue == nullptr)
        {
            ESP_LOGE(TEMPO_LOG_TAG, "Failed to create tick queue");
            return;
        }
    }

    if (dispatchTask == nullptr)
    {
        // 発音処理 (Pipeline.sendNotes → サンプラー NoteOn) を安全に実行できるよう
        // 十分なスタック (8192) を与える。Tmr Svc(2048) ではオーバーフローしていた。
        BaseType_t ok = xTaskCreate(
            dispatchTaskEntry,
            "TempoTick",
            8192,
            this,
            2, // 発音の遅延を抑えるため Arduino loopTask/audioLoop(prio1) より一段高く
            &dispatchTask);
        if (ok != pdPASS)
        {
            ESP_LOGE(TEMPO_LOG_TAG, "Failed to create dispatch task");
            dispatchTask = nullptr;
        }
    }
}

void TempoController::dispatchTaskEntry(void *arg)
{
    static_cast<TempoController *>(arg)->dispatchLoop();
}

void TempoController::dispatchLoop()
{
    TickInfo info;
    for (;;)
    {
        if (xQueueReceive(tickQueue, &info, portMAX_DELAY) != pdTRUE) continue;

        // デキュー後・配送前に stop() が走った場合、stop() の xQueueReset では
        // 既に取り出したこの info は止められない。再生中でなければ配送しない。
        if (!isPlaying) continue;

        std::list<TempoCallbacks *> listenersCopy;
        portENTER_CRITICAL(&mutex);
        listenersCopy = listeners;
        portEXIT_CRITICAL(&mutex);

        for (TempoCallbacks *listener : listenersCopy)
        {
            if (listener->getTimingMask() & info.timing)
            {
                listener->onTick(info);
            }
        }
    }
}

void TempoController::play()
{
    if (isPlaying) return;
    // ディスパッチ機構が未初期化なら遅延初期化する
    begin();
    // 前回再生時の残り tick があれば破棄してクリーンな状態で開始する
    if (tickQueue != nullptr) xQueueReset(tickQueue);
    std::list<TempoCallbacks *> listenersCopy;
    portENTER_CRITICAL(&mutex);
    isPlaying = true;
    if (timer != nullptr) xTimerDelete(timer, 0);

    realTimeBase = esp_timer_get_time();
    musicalTimeBase = 0;
    barAnchor = 0;
    currentBar = 0;

    // 各 tick 種別の最終発火位置を -interval で初期化することで
    // 最初のポーリングで mt=0 の tick が発火する
    for (size_t i = 0; i < 7; i++)
    {
        lastFiredMt[i] = -intervalOf(i);
    }

    timer = xTimerCreate("Tempo", pdMS_TO_TICKS(1), pdTRUE, this, timerWork);
    xTimerStart(timer, 0);

    listenersCopy = listeners;
    portEXIT_CRITICAL(&mutex);

    for (TempoCallbacks *listener : listenersCopy)
    {
        listener->onPlayingStateChanged(true);
    }
}

void TempoController::stop()
{
    if (!isPlaying) return;
    std::list<TempoCallbacks *> listenersCopy;
    portENTER_CRITICAL(&mutex);
    isPlaying = false;
    if (timer != nullptr)
    {
        xTimerDelete(timer, 0);
        timer = nullptr;
    }
    musicalTimeBase = 0;
    realTimeBase = 0;
    barAnchor = 0;
    currentBar = 0;
    for (size_t i = 0; i < 7; i++) lastFiredMt[i] = 0;

    listenersCopy = listeners;
    portEXIT_CRITICAL(&mutex);

    // 停止後にキューへ残った古い tick が配送されないよう破棄する。
    // (この時点で timerWorkInner は isPlaying=false により新規 enqueue しない)
    if (tickQueue != nullptr) xQueueReset(tickQueue);

    for (TempoCallbacks *listener : listenersCopy)
    {
        listener->onPlayingStateChanged(false);
    }
}

void TempoController::timerWorkInner()
{
    // 発火した tick を溜めるスタックバッファ (暴走防止のための上限付き)
    constexpr size_t MAX_FIRES = 32;
    TickInfo fires[MAX_FIRES];
    size_t numFires = 0;
    bool overflow = false;

    portENTER_CRITICAL(&mutex);
    if (!isPlaying)
    {
        portEXIT_CRITICAL(&mutex);
        return;
    }

    musical_time_t now = computeCurrentMusicalTimeLocked();

    while (numFires < MAX_FIRES)
    {
        // 全種別で次の発火時刻が最小のものを特定する
        musical_time_t nextMt = INT32_MAX;
        for (size_t i = 0; i < 7; i++)
        {
            musical_time_t n = lastFiredMt[i] + intervalOf(i);
            if (n < nextMt) nextMt = n;
        }
        if (nextMt > now) break;

        // 同じ musical_time で同時発火する種別をまとめる
        tick_timing_t mask = 0;
        for (size_t i = 0; i < 7; i++)
        {
            if (lastFiredMt[i] + intervalOf(i) == nextMt)
            {
                mask |= maskOf(i);
                lastFiredMt[i] = nextMt;
            }
        }

        if (mask & TICK_TIMING_BAR)
        {
            if (nextMt > 0) currentBar++;
            barAnchor = nextMt;
        }

        TickInfo &info = fires[numFires++];
        info.timing = mask;
        info.time = nextMt;
        info.timeInBar = nextMt - barAnchor;
        info.bar = currentBar;
    }

    // バッファ溢れ時のフォールバック: lastFiredMt を now まで強制同期
    if (numFires >= MAX_FIRES)
    {
        musical_time_t bl = barLengthOf(currentSig);
        for (size_t i = 0; i < 7; i++)
        {
            musical_time_t iv = intervalOf(i);
            if (iv > 0) lastFiredMt[i] = (now / iv) * iv;
        }
        if (bl > 0)
        {
            currentBar = (uint32_t)(now / bl);
            barAnchor = (musical_time_t)currentBar * bl;
        }
        overflow = true;
    }

    portEXIT_CRITICAL(&mutex);

    if (overflow)
    {
        ESP_LOGW(TEMPO_LOG_TAG, "Tick poll overflow, forced resync to mt=%d", (int)now);
    }

    // 発火した TickInfo をキューへ積むだけにする。
    // 実際の onTick 配送 (発音などの重い処理) は専用タスク dispatchLoop が
    // 十分なスタック上で行う。これにより Tmr Svc タスクのスタックを消費しない。
    if (tickQueue != nullptr)
    {
        for (size_t j = 0; j < numFires; j++)
        {
            if (xQueueSend(tickQueue, &fires[j], 0) != pdTRUE)
            {
                // 配送タスクが追従できずキューが溢れた場合は破棄する。
                // (ここでブロックすると Tmr Svc を止めてしまうため待たない)
                droppedTicks++;
            }
        }
    }
}

void TempoController::timerWork(TimerHandle_t t)
{
    static_cast<TempoController *>(pvTimerGetTimerID(t))->timerWorkInner();
}

TempoController Tempo;
