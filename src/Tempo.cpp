#include "Tempo.h"
#include "M5Unified.h"
#include <set>

void TempoController::start()
{
    if (isActive) return;
    isActive = true;
    if (timer != nullptr)
    {
        xTimerDelete(timer, 0);
    }
    timer = xTimerCreate("Tempo", pdMS_TO_TICKS(1), pdTRUE, this, timerWork);
    xTimerStart(timer, 0);

    // 各intervalを計算する
    intervalBar = (60000.0 * 4) / tempo; // TODO: 現状では4/4拍子固定としている
    intervalFullBeat = 60000.0 / tempo;
    intervalFullBeatTriplet = (60000.0 / 3) / tempo;
    intervalHalfBeat = (60000.0 / 2) / tempo;
    intervalHalfBeatTriplet = (60000.0 / 6) / tempo;
    intervalQuarterBeat = (60000.0 / 4) / tempo;
    intervalEighthBeat = (60000.0 / 8) / tempo;

    // 各変数の初期化
    nextTimeToTickBar = 0;
    nextTimeToTickFullBeat = 0;
    nextTimeToTickFullBeatTriplet = 0;
    nextTimeToTickHalfBeat = 0;
    nextTimeToTickHalfBeatTriplet = 0;
    nextTimeToTickQuarterBeat = 0;
    nextTimeToTickEighthBeat = 0;
    nearestNextTimeToTick = 0;
}

void TempoController::stop() {
    if (!isActive) return;
    isActive = false;
    xTimerDelete(timer, 0);
    timer = nullptr;
}

void TempoController::timerWorkInner()
{
    elapsedTime++;
    // nearestNextTimeToTickまでは処理をスキップする
    if (elapsedTime <= nearestNextTimeToTick) return;

    // Tickのタイミングを迎えたビートを算出する
    tick_timing_t beatToNotify = 0;
    // 1/8拍
    if (elapsedTime >= nextTimeToTickEighthBeat)
    {
        beatToNotify |= TICK_TIMING_EIGHTH;
        nextTimeToTickEighthBeat += intervalEighthBeat;
    }
    // 1/4拍
    if (elapsedTime >= nextTimeToTickQuarterBeat)
    {
        beatToNotify |= TICK_TIMING_QUARTER;
        nextTimeToTickQuarterBeat += intervalQuarterBeat;
    }
    // 6連符
    if (elapsedTime >= nextTimeToTickHalfBeatTriplet)
    {
        beatToNotify |= TICK_TIMING_HALF_TRIPLET;
        nextTimeToTickHalfBeatTriplet += intervalHalfBeatTriplet;
    }
    // 1/2拍
    if (elapsedTime >= nextTimeToTickHalfBeat)
    {
        beatToNotify |= TICK_TIMING_HALF;
        nextTimeToTickHalfBeat += intervalHalfBeat;
    }
    // 3連符
    if (elapsedTime >= nextTimeToTickFullBeatTriplet)
    {
        beatToNotify |= TICK_TIMING_FULL_TRIPLET;
        nextTimeToTickFullBeatTriplet += intervalFullBeatTriplet;
    }
    // 1拍
    if (elapsedTime >= nextTimeToTickFullBeat)
    {
        beatToNotify |= TICK_TIMING_FULL;
        nextTimeToTickFullBeat += intervalFullBeat;
    }
    // 1小節
    if (elapsedTime >= nextTimeToTickBar)
    {
        beatToNotify |= TICK_TIMING_BAR;
        // elapsedTimeをリセット
        elapsedTime = 0;
        nextTimeToTickBar = intervalBar;
        nextTimeToTickFullBeat = intervalFullBeat;
        nextTimeToTickFullBeatTriplet = intervalFullBeatTriplet;
        nextTimeToTickHalfBeat = intervalHalfBeat;
        nextTimeToTickHalfBeatTriplet = intervalHalfBeatTriplet;
        nextTimeToTickQuarterBeat = intervalQuarterBeat;
        nextTimeToTickEighthBeat = intervalEighthBeat;
    }

    // Tickを通知する
    for (TempoCallbacks *listener : listeners)
    {
        if (listener->getTimingMask() & beatToNotify)
        {
            listener->onTick(beatToNotify);
        }
    }

    // 最も近い次のTickをセットする
    nearestNextTimeToTick = (uint32_t)(std::min(nextTimeToTickHalfBeatTriplet, nextTimeToTickEighthBeat) + 0.5);
}

void TempoController::timerWork(TimerHandle_t t)
{
    static_cast<TempoController *>(pvTimerGetTimerID(t))->timerWorkInner();
}

TempoController Tempo;
