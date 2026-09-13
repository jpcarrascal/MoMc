#include <Button.h>
#include <ButtonEventCallback.h>
#include <PushButton.h>
#include <Bounce2.h>
#include <MIDI.h>
//#include "MIDIUSB.h"
#include <USB-MIDI.h>
#include <Wire.h>
// OLED display, text mode:
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE); 

#define POT_COUNT 4 // We have 4 potentiometers/knobs

// Configurationflags:
const bool debug = false;
const bool usbMIDI = true; // Send MIDI via USB?
const bool srlMIDI = true; // Send/receive MIDI via MIDI ports?
bool pickUpMode = true;

PushButton sw_center = PushButton(11, ENABLE_INTERNAL_PULLUP);
PushButton sw_top_left = PushButton(7, ENABLE_INTERNAL_PULLUP);
PushButton sw_top_right = PushButton(8, ENABLE_INTERNAL_PULLUP);
PushButton sw_bottom_left = PushButton(10, ENABLE_INTERNAL_PULLUP);
PushButton sw_bottom_right = PushButton(12, ENABLE_INTERNAL_PULLUP);

const int mainLED =  6;
const int leftLED = 5;
const int rightLED = 9;
int mainLEDState = HIGH;
int leftLEDState = HIGH;
int rightLEDState = HIGH;
const long blinkInterval = 300; // blink interval
int PCchannel[3] = {13, 10, 16};
int nPCchannels = 3;
//const int PCchannelB = 11; // For song switching in computer
const int CCchannel = 1;
const int NoteChannel = 1;
const int INchannel = 16;
const int maxPgm = 63; // Max number of patches. Zoia = 63
int tentativeProgram = 0;
int currentProgram = 0;
int favoriteProgram = 0;
int swapProgram = 0;
int PCmodeTimeout = 0;
unsigned long previousMillis = 0;
int mode = 1; // 1 = Control Change, 2 = Program Change
const int cc_center       = 2;
const int cc_top_left     = 9;
const int cc_top_right    = 10;
const int cc_bottom_left  = 0;
const int cc_bottom_right = 1;

const int note_center        = 11;
const int note_top_left      = 12;
const int note_top_right     = 13;
const int note_bottom_left   = 14;
const int note_bottom_right  = 15;
// MIDI Machine Control messages:
uint8_t mmcStopMsg[] = {0xF0, 0x7F, 0x7F, 0x06, 0x01, 0xF7};
uint8_t mmcStartMsg[] = {0xF0, 0x7F, 0x7F, 0x06, 0x02, 0xF7};


// These CCs are reserved for the Daughterboard:
//const int cc_pot[POT_COUNT] = {3, 4, 5, 6};
const int expPedal = A0;

const char pgm00[] PROGMEM = "LOOOP";
const char pgm01[] PROGMEM = "SADTF";
const char pgm02[] PROGMEM = "LITTLE BOX";
const char pgm03[] PROGMEM = "A VECES";

const char pgm04[] PROGMEM = "EN REMOLINOS";
const char pgm05[] PROGMEM = "FOMO";
const char pgm06[] PROGMEM = "SI TE VAS";
const char pgm07[] PROGMEM = "EL ANSIA";

const char pgm08[] PROGMEM = "PLANEADOR";
const char pgm09[] PROGMEM = "YOU GOT IT";
const char pgm10[] PROGMEM = "DARK PLACES";
const char pgm11[] PROGMEM = "LA SED";

const char pgm12[] PROGMEM = "BREAK CAGE";

const char* const pgmNames[] PROGMEM = {
          pgm00, pgm01, pgm02, pgm03,
          pgm04, pgm05, pgm06, pgm07,
          pgm08, pgm09, pgm10, pgm11,
          pgm12
};

const uint8_t LINE_LEN = 8;              // chars per line with the 2x4 font
char nameBuf[LINE_LEN * 2 + 2];          // 8 + '\n' + 8 + '\0' = 18

MIDI_CREATE_DEFAULT_INSTANCE();
void setup() {
  MIDI.begin(INchannel);
  MIDI.turnThruOn();
  // MIDI baud rate
  if(debug)
    Serial.begin(9600);
  else
    Serial.begin(31250);
  // For brighter LEDs, uncomment these two lines:
  //pinMode(mainLED, OUTPUT);
  //pinMode(mainLED, OUTPUT);

  // Footswitch press and release callbacks
  sw_center.onPress(onButtonPressed);
  sw_top_left.onPress(onButtonPressed);
  sw_top_right.onPress(onButtonPressed);
  sw_bottom_left.onPress(onButtonPressed);
  sw_bottom_right.onPress(onButtonPressed);

  sw_top_left.onHoldRepeat(1500, 800, onButtonHoldRepeat);
  sw_top_right.onHoldRepeat(1500, 800, onButtonHoldRepeat);

  sw_center.onRelease(onButtonReleased);
  sw_top_left.onRelease(onButtonReleased);
  sw_top_right.onRelease(onButtonReleased);
  sw_bottom_left.onRelease(onButtonReleased);
  sw_bottom_right.onRelease(onButtonReleased);
  
  // If center footswitch is held for 1.5 seconds, switch to PC mode
  sw_center.onHold(1500, setPCmode);

  // For rceiving data from Daughterboard
  //Wire.begin(8);
  //Wire.onReceive(receiveEvent);
  Wire.begin();
  u8x8.begin();
  // u8x8.setFont(u8x8_font_inr46_4x8_n); // big numbers
  // u8x8.setFont(u8x8_font_courB24_3x4_r); // Good enough
  u8x8.setFont(u8x8_font_inr21_2x4_r); // best, 2 lines of 8 chars
  //u8x8.print(u8x8_u16toa(0, 2));
  u8x8.print("SPACEBAR\nMAN");
  intro();
  showProgramName(0);
}

void loop() {
  if (MIDI.read()) {
    // Do stuff on MIDI in
  }
  
  midiEventPacket_t rx;
  rx = MidiUSB.read();
  if (rx.header != 0) {
    if(rx.header == 0xB) {
      MIDI.sendControlChange(rx.byte2, rx.byte3, CCchannel);
    } else if(rx.header == 0xC) {
      MIDI.sendProgramChange(rx.byte2, PCchannel);
    }
    // Would be great to make this work instead of the two calls above
    // so not only CC and PC messages are passed through:
    // MIDI.send( rx.header, rx.byte2, rx.byte3, (rx.byte1 >> 4) & 0x0F);
    // Ref:
    // - Arduino MIDIUSB input example
    // - https://arduino.stackexchange.com/questions/41684/midiusb-why-is-the-command-put-twice
    // - https://fortyseveneffects.github.io/arduino_midi_library/a00032.html#ga58454de7d3ee8ee824f955c805151ad2
  }
  
  sw_center.update();
  sw_top_left.update();
  sw_top_right.update();
  sw_bottom_left.update();
  sw_bottom_right.update();
  unsigned long currentMillis = millis();
  if(mode == 2) {
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      rightLEDState = (!rightLEDState);
      analogWrite(rightLED, rightLEDState);
      if(rightLEDState) {
          showProgramName(tentativeProgram);
      } else {
          u8x8.clear();
      }
    } 
  }

  // Auto switch back to CC mode:
  if(mode == 2 && !sw_center.isPressed()) {
    PCmodeTimeout++;
    if(PCmodeTimeout > 30000) {
      if(currentProgram != tentativeProgram){
        swapProgram = currentProgram;
        currentProgram = tentativeProgram;
        pcSend(currentProgram, PCchannel, nPCchannels);
      } else {
        showProgramName(currentProgram);
      }
      setCCmode();
    }
  }

} // main() ends





void configurePushButton(Bounce& bouncedButton){
  bouncedButton.interval(10); //10 is default
}

void onButtonHoldRepeat(Button& btn, uint16_t duration, uint8_t repeat_count){
  debugThis("hold and repeat", 1,1);
  if(mode == 2) {
    if(btn.is(sw_top_left)) {
      decrementProgram();
    } else if (btn.is(sw_top_right)) {
      incrementProgram();
    }
  }
  // btn is a reference to the button that was held
  // duration is how long the button has been held for
  // repeat_count is the number of times the callback has been called
}


void decrementProgram() {
    PCmodeTimeout = 0;
  if(tentativeProgram > 0)
    tentativeProgram--;
  else
    tentativeProgram = maxPgm;
}

void incrementProgram() {
    PCmodeTimeout = 0;
  if(tentativeProgram < maxPgm)
    tentativeProgram++;
  else
    tentativeProgram = 0;
}

void onButtonPressed(Button& btn){
  if(btn.is(sw_top_left)) {
    if(mode == 2) {
      decrementProgram();
      //pcSend(tentativeProgram, PCchannel, nPCchannels);
    }
    else {
      ccSend(cc_top_left, 127, CCchannel);
      mmcStart();
      noteOnSend(note_top_left, 127, NoteChannel);
    }
  } else if (btn.is(sw_top_right)){
    if(mode == 2) {
      incrementProgram();
      //pcSend(tentativeProgram, PCchannel, nPCchannels);
    }
    else {
      ccSend(cc_top_right, 127, CCchannel);
      mmcStop();
      noteOnSend(note_top_right, 127, NoteChannel);
    }
  } else if (btn.is(sw_center)) {
    if(mode == 2) {
      swapProgram = currentProgram;
      currentProgram = tentativeProgram;
      pcSend(currentProgram, PCchannel, nPCchannels);
      setCCmode();
    }
    else {
      ccSend(cc_center, 127, CCchannel);
      noteOnSend(note_center, 127, NoteChannel);
    }
  } else if (btn.is(sw_bottom_left)){
    if(mode == 2) {
      swapProgram = currentProgram;
      currentProgram = favoriteProgram;
      pcSend(currentProgram, PCchannel, nPCchannels);
      setCCmode();
    } else {
      ccSend(cc_bottom_left, 127, CCchannel);
      noteOnSend(note_bottom_left, 127, NoteChannel);
    }
  } else if (btn.is(sw_bottom_right)){
    if(mode == 2) {
      tentativeProgram = swapProgram;
      swapProgram = currentProgram;
      currentProgram = tentativeProgram;
      pcSend(currentProgram, PCchannel, nPCchannels);
      setCCmode();
    } else {
      ccSend(cc_bottom_right, 127, CCchannel);
      noteOnSend(note_bottom_right, 127, NoteChannel);
    }
  }
}

void onButtonReleased(Button& btn, uint16_t duration){
  if(btn.is(sw_top_left)) {
    if(mode == 1)
      ccSend(cc_top_left, 0, CCchannel);
  } else if (btn.is(sw_top_right)){
    if(mode == 1)
      ccSend(cc_top_right, 0, CCchannel);
  } else if (btn.is(sw_center)) {
    if(mode == 1)
      ccSend(cc_center, 0, CCchannel);
  } else if (btn.is(sw_bottom_right)){
      ccSend(cc_bottom_right, 0, CCchannel);
  } else if (btn.is(sw_bottom_left)){
      ccSend(cc_bottom_left, 0, CCchannel);
  }
}

void setCCmode() {
  if(debug) Serial.println("CC mode");
  mode = 1;
  digitalWrite(rightLED, LOW);
  PCmodeTimeout = 0;
  showProgramName(currentProgram);
  tentativeProgram = currentProgram;
}

void setPCmode() {
  if(debug) Serial.println("PC mode");
  ccSend(cc_center, 0, CCchannel);
  mode = 2;
}

void ccSend(int cc, int value, int channel) {
  if(debug) {
    debugThis("cc", cc, value);
  } else {
    if(srlMIDI) {
      MIDI.sendControlChange(cc, value, channel);
    }
    if(usbMIDI) {
      midiEventPacket_t event = {0x0B, 0xB0 | channel, cc, value};
      MidiUSB.sendMIDI(event);
      MidiUSB.flush();
    }
  }
}

void mmcStart() {
  if(debug) {
    debugThis("MMC START", 0, 0);
  } else {
    if(srlMIDI) {
      MIDI.sendSysEx(6, mmcStartMsg, true);
    }
    if(usbMIDI) {
      MidiUSB_sendSysEx(mmcStartMsg, 6);
      //MidiUSB.flush();
    }
  }
}

void mmcStop() {
  if(debug) {
    debugThis("MMC STOP", 0, 0);
  } else {
    if(srlMIDI) {
      MIDI.sendSysEx(6, mmcStopMsg, true);
    }
    if(usbMIDI) {
      MidiUSB_sendSysEx(mmcStopMsg, 6);
      //MidiUSB.flush();
    }
  }
}


void pcSend(int value, int channel) {
  if(debug) {
    debugThis("pc", -1, value);
  } else {
    if(srlMIDI) {
      MIDI.sendProgramChange(value, channel);
    }
    if(usbMIDI) {
      midiEventPacket_t event = {0x0C, 0xC0 | channel, value, 0};
      MidiUSB.sendMIDI(event);
      MidiUSB.flush();
    }
  }
}

void pcSend(int value, int channel[], int nChannels) {
  showProgramName(value);
  if(debug) {
    debugThis("pc", -1, value);
  } else {
    for(int i=0; i<nChannels; i++)
    {
      int ch = channel[i];
      if(srlMIDI) {
        MIDI.sendProgramChange(value, ch);
      }
      if(usbMIDI) {
        midiEventPacket_t event = {0x0C, 0xC0 | ch, value, 0};
        MidiUSB.sendMIDI(event);
        MidiUSB.flush();
      }
    }
  }
}

void noteOnSend(int note, int vel, int channel) {
  if(debug) {
    debugThis("noteOn", note, vel);
  } else {
    if(srlMIDI) {
      MIDI.sendNoteOn(note, vel, channel);
    }
    if(usbMIDI) {
      midiEventPacket_t event = {0x09, 0x90 | channel, note, vel};
      MidiUSB.sendMIDI(event);
      MidiUSB.flush();
    }
  }
}

void noteOffSend(int note, int vel, int channel) {
  if(debug) {
    debugThis("noteOff", note, vel);
  } else {
    if(srlMIDI) {
      MIDI.sendNoteOff(note, vel, channel);
    }
    if(usbMIDI) {
      midiEventPacket_t event = {0x08, 0x80 | channel, note, vel};
      MidiUSB.sendMIDI(event);
      MidiUSB.flush();
    }
  }
}

void receiveEvent(int howMany) {
  digitalWrite(leftLED, HIGH);
  int byte1 = Wire.read();
  int byte2 = Wire.read();
  int byte3 = Wire.read();
  if(byte1 == 129) ccSend(byte2, byte3, CCchannel); //cc
  else if(byte1 == 117) noteOnSend (byte2, byte3, NoteChannel); //note on
  else if(byte1 == 109) noteOffSend(byte2, byte3, NoteChannel); // note off
  digitalWrite(leftLED, LOW);
}

void debugThis(const char* name, int i, int value) {
  if(debug) {
    Serial.print(name);
    Serial.print("[");
    Serial.print(i);
    Serial.print("]");
    Serial.print(": ");
    Serial.println(value);
  }
}

// LED sequence at power-up
void intro() {
  int delayTime = 2;
  for(int i=0;i<2;i++){
    fadeLed(leftLED);
    fadeLed(mainLED);
    fadeLed(rightLED);
    fadeLed(mainLED);
  }
  for(int i=0;i<100;i++) {
    analogWrite(mainLED,i/4);
    delay(delayTime*2);
  }
}

void fadeLed(int led) {
  int maxVal = 50;
  int delayTime = 2;
  for(int i=0;i<maxVal;i++) {
    int val;
    if(i<maxVal/2) val = i;
    if(i>=maxVal/2) val = (maxVal-1)-i;
    analogWrite(led,val/2);
    delay(delayTime);
  }
}


// Source for this function: https://github.com/arduino-libraries/MIDIUSB/issues/19
// TODO: use the USB transport of FortySevenEffects Arduino MIDI Library
//       info here: https://github.com/lathoub/Arduino-USBMIDI
void MidiUSB_sendSysEx(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0) return;

    size_t midiDataSize = (size+2)/3*4;
    uint8_t midiData[midiDataSize];
    const uint8_t *d = data;
    uint8_t *p = midiData;
    size_t bytesRemaining = size;

    while (bytesRemaining > 0) {
        switch (bytesRemaining) {
        case 1:
            *p++ = 5;   // SysEx ends with following single byte
            *p++ = *d;
            *p++ = 0;
            *p = 0;
            bytesRemaining = 0;
            break;
        case 2:
            *p++ = 6;   // SysEx ends with following two bytes
            *p++ = *d++;
            *p++ = *d;
            *p = 0;
            bytesRemaining = 0;
            break;
        case 3:
            *p++ = 7;   // SysEx ends with following three bytes
            *p++ = *d++;
            *p++ = *d++;
            *p = *d;
            bytesRemaining = 0;
            break;
        default:
            *p++ = 4;   // SysEx starts or continues
            *p++ = *d++;
            *p++ = *d++;
            *p++ = *d++;
            bytesRemaining -= 3;
            break;
        }
    }
    MidiUSB.write(midiData, midiDataSize);
}

void formatProgramName(uint8_t index) {
  char raw[LINE_LEN * 2 + 2];

  raw[0] = '0' + (index / 10);
  raw[1] = '0' + (index % 10);
  raw[2] = '.';
  strncpy_P(raw + 3, (char*)pgm_read_word(&pgmNames[index]), sizeof(raw) - 4);
  raw[sizeof(raw) - 1] = '\0';

  uint8_t len = strlen(raw);

  if (len <= LINE_LEN) {                 // fits on one line, nothing to do
    strcpy(nameBuf, raw);
    return;
  }

  uint8_t split;                         // index in raw where line 2 starts
  bool    drop;                          // is the char at the split consumed?

  char* nl = strchr(raw, '\n');          // 1. honour an explicit newline
  if (nl) {
    split = nl - raw;
    drop  = true;
  } else {
    char* sp = strchr(raw + 3, ' ');     // 2. first space
    if (sp) {
      split = sp - raw;
      drop  = true;
    } else {                             // 3. hard split
      split = LINE_LEN;
      drop  = false;
    }
  }

  if (split > LINE_LEN) {                // break point past the edge -> hard split
    split = LINE_LEN;
    drop  = false;
  }

  memcpy(nameBuf, raw, split);
  nameBuf[split] = '\n';

  const char* rest = raw + split + (drop ? 1 : 0);
  uint8_t n2 = strlen(rest);
  if (n2 > LINE_LEN) n2 = LINE_LEN;      // truncate line 2
  memcpy(nameBuf + split + 1, rest, n2);
  nameBuf[split + 1 + n2] = '\0';

  for (uint8_t i = split + 1; nameBuf[i]; i++) {   // no stray breaks on line 2
    if (nameBuf[i] == '\n') nameBuf[i] = ' ';
  }
}

void showProgramName(uint8_t index) {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  if(index < sizeof(pgmNames)) {
    formatProgramName(index);
    u8x8.print(nameBuf);
  } else {
    u8x8.print(u8x8_u16toa(index, 2));
  }
}