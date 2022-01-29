#include <Button.h>
#include <ButtonEventCallback.h>
#include <PushButton.h>
#include <Bounce2.h>
#include <MIDI.h>
//#include "MIDIUSB.h"
#include <USB-MIDI.h>
#include <Wire.h>

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
int PCchannel[] = {13, 10};
const int PCchannelB = 11; // For song switching in computer
const int CCchannel = 1;
const int NoteChannel = 1;
const int INchannel = 16;
const int maxPgm = 63; // Max number of patches. Zoia = 63
int currentProgram = 0;
int PCmodeTimeout = 0;
unsigned long previousMillis = 0;
String mode = "CC"; // "CC" = Control Change, PC" = Program Change
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
  intro();

  // Footswitch press and release callbacks
  sw_center.onPress(onButtonPressed);
  sw_top_left.onPress(onButtonPressed);
  sw_top_right.onPress(onButtonPressed);  
  sw_bottom_left.onPress(onButtonPressed);
  sw_bottom_right.onPress(onButtonPressed);

  sw_center.onRelease(onButtonReleased);
  sw_top_left.onRelease(onButtonReleased);
  sw_top_right.onRelease(onButtonReleased);
  sw_bottom_left.onRelease(onButtonReleased);
  sw_bottom_right.onRelease(onButtonReleased);
  
  // If center footswitch is held for 1.5 seconds, switch to PC mode
  sw_center.onHold(1500, setPCmode);

  // For rceiving data from Daughterboard
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
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
  if(mode == "PC") {
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      rightLEDState = (!rightLEDState);
      analogWrite(rightLED, rightLEDState);
    } 
  }

  // Auto switch back to CC mode:
  if(mode == "PC" && !sw_center.isPressed()) {
    PCmodeTimeout++;
    if(PCmodeTimeout > 30000) {
      setCCmode();
    }
  }
}

void configurePushButton(Bounce& bouncedButton){
  bouncedButton.interval(10); //10 is default
}

void onButtonPressed(Button& btn){
  if(btn.is(sw_top_left)) {
    if(mode == "PC") {
      PCmodeTimeout = 0;
      if(currentProgram > 0)
        currentProgram--;
      else
        currentProgram = maxPgm;
      pcSend(currentProgram, PCchannel);
    }
    else {
      ccSend(cc_top_left, 127, CCchannel);
      mmcStart();
      noteOnSend(note_top_left, 127, NoteChannel);
    }
  } else if (btn.is(sw_top_right)){
    if(mode == "PC") {
      PCmodeTimeout = 0;
      if(currentProgram < maxPgm)
        currentProgram++;
      else
        currentProgram = 0;
      pcSend(currentProgram, PCchannel);
    }
    else {
      ccSend(cc_top_right, 127, CCchannel);
      mmcStop();
      noteOnSend(note_top_right, 127, NoteChannel);
    }
  } else if (btn.is(sw_center)) {
    if(mode == "PC") {
      pcSend(currentProgram, PCchannelB);
      setCCmode();
    }
    else {
      ccSend(cc_center, 127, CCchannel);
      noteOnSend(note_center, 127, NoteChannel);
    }
  } else if (btn.is(sw_bottom_left)){
      ccSend(cc_bottom_left, 127, CCchannel);
      noteOnSend(note_bottom_left, 127, NoteChannel);
  } else if (btn.is(sw_bottom_right)){
      ccSend(cc_bottom_right, 127, CCchannel);
      noteOnSend(note_bottom_right, 127, NoteChannel);
  }
}

void onButtonReleased(Button& btn, uint16_t duration){
  if(btn.is(sw_top_left)) {
    if(mode == "CC")
      ccSend(cc_top_left, 0, CCchannel);
  } else if (btn.is(sw_top_right)){
    if(mode == "CC")
      ccSend(cc_top_right, 0, CCchannel);
  } else if (btn.is(sw_center)) {
    if(mode == "CC")
      ccSend(cc_center, 0, CCchannel);
  } else if (btn.is(sw_bottom_right)){
      ccSend(cc_bottom_right, 0, CCchannel);
  } else if (btn.is(sw_bottom_left)){
      ccSend(cc_bottom_left, 0, CCchannel);
  }
}

void setCCmode() {
  if(debug) Serial.println("CC mode");
  mode = "CC";
  digitalWrite(rightLED, LOW);
  PCmodeTimeout = 0;
}

void setPCmode() {
  if(debug) Serial.println("PC mode");
  ccSend(cc_center, 0, CCchannel);
  mode = "PC";
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

void pcSend(int value, int channel[]) {
  if(debug) {
    debugThis("pc", -1, value);
  } else {
    for(int i=0; i<sizeof(channel); i++)
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

void debugThis(String name, int i, int value) {
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
  int delayTime = 4;
  fadeLed(leftLED);
  fadeLed(mainLED);
  fadeLed(rightLED);
  fadeLed(mainLED);
  fadeLed(leftLED);
  delay(300);
  for(int i=0;i<100;i++) {
    analogWrite(mainLED,i/4);
    delay(delayTime*2);
  }
}

void fadeLed(int led) {
  int maxVal = 50;
  int delayTime = 4;
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
