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
      case Key_State_Pressed:
        switch(event >> 4 & 0b111) {
          case 0: {// Number Keys Pressed
            uint8_t number = numberKeyMap[(event & 0b1111) - 1]; // Key number starts from 1
            if(0 <= number && number <= 6) {
              Chord c = context->scale->getDiatonic(number,Keypad[Key_Seventh].isPressed());
              if(Keypad[Key_ThirdInvert].isPressed())   thirdInvert(&c);
              if(Keypad[Key_FlatFive].isPressed())     fifthFlat(&c);
              if(Keypad[Key_Augment].isPressed())       augment(&c);
              if(Keypad[Key_Sus4].isPressed())          sus4(&c);
              // if(Keypad[Key_Seventh].isPressed()) thirdInvert(&c);
              if(Keypad[Key_SeventhInvert].isPressed()) seventhInvert(&c);
              if(Keypad[Key_Ninth].isPressed())         ninth(&c);
              if(Keypad[Key_Thirteenth].isPressed())    thirteenth(&c);
              if(Keypad[Key_PitchUp].isPressed()) pitchUp(&c);
              if(Keypad[Key_PitchDown].isPressed()) pitchDown(&c);
              c.calcInversion(*(uint8_t *)context->centerNoteNo);
              if(Keypad[Key_InversionUp].isPressed()) c.inversion++;
              if(Keypad[Key_InversionDown].isPressed()) c.inversion = c.inversion > 0 ? (c.inversion-1) : 0;
              context->playChord(c);
            }
          } break;
          case 2: {// Other Keys Pressed
            // if((event & 0b1111111) == Key_InversionUp) *(context->centerNoteNo) += 4;
            // if((event & 0b1111111) == Key_InversionDown) *(context->centerNoteNo) -= 4;
          } break;
        }
      break;
      case Key_State_Released:
        switch(event >> 4 & 0b111) {
          case 0: {
            context->stopChord();
          } break;
          case 2: {
            // if((event & 0b1111111) == Key_InversionUp) *(context->centerNoteNo) -= 4;
            // if((event & 0b1111111) == Key_InversionDown) *(context->centerNoteNo) += 4;
          } break;
        }
      break;
    }
  }
}
