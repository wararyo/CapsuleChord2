#include "KantanChordKeyMap.h"
#include "Modifier.h"
#include "BLEMidi.h"

const uint8_t KantanChordKeyMap::numberKeyMap[] = {
    6, //Custom1
    0, //VII
    0, //Custom2
    3, //IV
    4, //I
    5, //V
    0, //II
    1, //VI
    2};

void KantanChordKeyMap::update() {
  while(Keypad.hasEvent()){
    char event = Keypad.getEvent();
    switch(event >> 7 & 0b1) {
      case KEY_STATE_PRESSED:
        switch(event >> 4 & 0b111) {
          case 0: {// Left Keys Pressed
            uint8_t number = numberKeyMap[(event & 0b1111) - 1]; // Key number starts from 1
            if(0 <= number && number <= 6) {
              Chord c = context->scale->getDiatonic(number,Keypad[KEY_RIGHT_5].isPressed());
              if(Keypad[KEY_RIGHT_8].isPressed())   thirdInvert(&c);
              if(Keypad[KEY_RIGHT_7].isPressed())   fifthFlat(&c);
              if(Keypad[KEY_RIGHT_6].isPressed())   augment(&c);
              if(Keypad[KEY_RIGHT_9].isPressed())   sus4(&c);
              // if(Keypad[KEY_RIGHT_5].isPressed()) thirdInvert(&c);
              if(Keypad[KEY_RIGHT_4].isPressed())   seventhInvert(&c);
              if(Keypad[KEY_RIGHT_2].isPressed())   ninth(&c);
              if(Keypad[KEY_RIGHT_1].isPressed())   thirteenth(&c);
              if(Keypad[KEY_R].isPressed())         pitchUp(&c);
              if(Keypad[KEY_L].isPressed())         pitchDown(&c);
              if(Keypad[KEY_RIGHT_3].isPressed())   blackAdder(&c);
              c.calcInversion(*(uint8_t *)context->centerNoteNo);
              if(Keypad[KEY_RT].isPressed())        inversionUp(&c);
              if(Keypad[KEY_LT].isPressed())        inversionDown(&c);
              currentPressingButton = event & 0b1111111;
              context->pipeline->playChord(c);
            }
          } break;
          case 2: {// Other Keys Pressed
            // if((event & 0b1111111) == KEY_RT) *(context->centerNoteNo) += 4;
            // if((event & 0b1111111) == KEY_LT) *(context->centerNoteNo) -= 4;
          } break;
        }
      break;
      case KEY_STATE_RELEASED:
        switch(event >> 4 & 0b111) {
          case 0: {
            if ((event & 0b1111111) == currentPressingButton)
              context->pipeline->stopChord();
          } break;
          case 2: {
            // if((event & 0b1111111) == KEY_RT) *(context->centerNoteNo) -= 4;
            // if((event & 0b1111111) == KEY_LT) *(context->centerNoteNo) += 4;
          } break;
        }
      break;
    }
  }
}
