#pragma once

#include "tusb.h"

// Interface numbers for composite device
enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_MIDI,
    ITF_NUM_MIDI_STREAMING,
    ITF_NUM_TOTAL
};

// Endpoint numbers (avoid conflicts between CDC and MIDI)
#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82
#define EPNUM_MIDI_OUT    0x04
#define EPNUM_MIDI_IN     0x84

// Total configuration descriptor length
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MIDI_DESC_LEN)

// String descriptor indices
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_CDC,
    STRID_MIDI,
};

// String descriptors
static const char* usb_string_descriptor[] = {
    (char[]){0x09, 0x04},    // 0: Language (English)
    "CapsuleChord",          // 1: Manufacturer
    "CapsuleChord2",         // 2: Product
    "000001",                // 3: Serial
    "CapsuleChord2 CDC",     // 4: CDC Interface
    "CapsuleChord2 MIDI",    // 5: MIDI Interface
};

// Configuration descriptor for CDC + MIDI composite device
static const uint8_t usb_configuration_descriptor[] = {
    // Configuration descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),
    // CDC descriptor
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, STRID_CDC, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
    // MIDI descriptor
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, STRID_MIDI, EPNUM_MIDI_OUT, EPNUM_MIDI_IN, 64),
};
