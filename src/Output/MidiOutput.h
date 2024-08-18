#pragma once
#include "OutputInternal.h"

// いくつかの出力を総合的に扱うクラス
class MidiOutput {
public:
    OutputInternal Internal; //内蔵音源
    // BleOutput Ble; //BLE
    // UsbOutput Usb; //USB
};

extern MidiOutput Output;
