#include <Button.h>
#include <ButtonEventCallback.h>
#include <PushButton.h>
#include <Bounce2.h>
#include <Wire.h>
#include "MIDIUSB.h"

#define POT_COUNT 4 // We have 4 potentiometers/knobs

// Configurationflags:
const bool debug = false;
const bool usbMIDI = true; // Send MIDI via USB?
const bool i2cMIDI = true; // Send i2c to Motherboard?
const bool sendRelease = true; // Send on release? 
bool pickUpMode = true;

PushButton sw_1 = PushButton(7, ENABLE_INTERNAL_PULLUP);
PushButton sw_2 = PushButton(6, ENABLE_INTERNAL_PULLUP);
PushButton sw_3 = PushButton(5, ENABLE_INTERNAL_PULLUP);
PushButton sw_4 = PushButton(4, ENABLE_INTERNAL_PULLUP);

const int CCchannel = 1;
const int INchannel = 16;
const int PCchannel = 13;
const int note_1 = 28;
const int note_2 = 29;
const int note_3 = 30;
const int note_4 = 31;

const int cc_pot[POT_COUNT] = {3, 4, 5, 6};
const int pot[POT_COUNT] = {A3,A2,A1,A0};
const int leds[4] = {8,9,10,12};
int potval[POT_COUNT];
int potvalIN[POT_COUNT];
bool potPosCorrect[POT_COUNT] = {true, true, true, true};
bool onUSB = false;

void setup() {
  // MIDI baud rate
  if(debug)
    Serial.begin(9600);
  else
    Serial.begin(31250);
  // For brighter LEDs, uncomment these two lines:

  for(int i=0; i<4; i++) {
    potval[i] = analogRead(pot[i]);
  }

  // Footswitch press and release callbacks
  sw_1.onPress(onButtonPressed);
  sw_2.onPress(onButtonPressed);
  sw_3.onPress(onButtonPressed);
  sw_4.onPress(onButtonPressed);

  if(sendRelease) {
    sw_1.onRelease(onButtonReleased);
    sw_2.onRelease(onButtonReleased);
    sw_3.onRelease(onButtonReleased);
    sw_4.onRelease(onButtonReleased);  
  }
  
  Wire.begin();
  onUSB = USBSTA >> VBUS & 1;
  intro();
}

void loop() {
  onUSB = USBSTA >> VBUS & 1;

  sw_1.update();
  sw_3.update();
  sw_2.update();
  sw_4.update();
  midiEventPacket_t rx;
  if(onUSB) {
    rx = MidiUSB.read();
    if (rx.header != 0) {
      if(rx.header == 0xB) { // Controller Change
        // If a CC that corresponds to one of the knobs is received,
        // switch to "pick-up" mode by blocking sending MIDI CC 
        // until the knob position and the controller value match.
        int ccNumber = rx.byte2;
        int index = isPotCC(ccNumber);
        if( index >= 0 && pickUpMode) { // it is a knob CC
          potvalIN[index] = rx.byte3;
          if(abs( potvalIN[index] - potval[index] ) < 2) {
            potPosCorrect[index] = true;
            digitalWrite(leds[index],LOW);
          } else {
            digitalWrite(leds[index],HIGH);
            potPosCorrect[index] = false;          
          }
        }
      } else if(rx.header == 0xC) { // Program Change
  
      }
    }  
  }

  int potvalNew[4];
  for(int i=0; i<4; i++) {
    potvalNew[i] = analogRead(pot[i]);
  }
  
  for(int i=0; i<4; i++) {
    if( abs(potval[i] - potvalNew[i]) > 3) {
      int outval = mapAndClamp(potval[i], i);
      potval[i] = potvalNew[i];
      if(potPosCorrect[i]) {
        ccSend(cc_pot[i], outval, CCchannel);
        i2cSend("cc", cc_pot[i], outval);
      } else {
        // Check if knob position matches value received via MIDI.
        // If so, allow sending MIDI again (i.e. pick-up).
        if( abs( outval - potvalIN[i] ) < 2 ) {
          potPosCorrect[i] = true;
          digitalWrite(leds[i],LOW);
        } else {
          // turn corresponding led on
          digitalWrite(leds[i],HIGH);
        }
      }
    }
  }
}

void configurePushButton(Bounce& bouncedButton){
  bouncedButton.interval(10); //10 is default
}

void onButtonPressed(Button& btn){
   if(btn.is(sw_1)) {
      noteOnSend(note_1, 127, CCchannel);
      i2cSend("noteOn",note_1, 127);
  } else if (btn.is(sw_2)) {
      noteOnSend(note_2, 127, CCchannel);
      i2cSend("noteOn",note_2, 127);
  } else if (btn.is(sw_3)) {
      noteOnSend(note_3, 127, CCchannel);
      i2cSend("noteOn",note_3, 127);
  } else if (btn.is(sw_4)) {
      noteOnSend(note_4, 127, CCchannel);
      i2cSend("noteOn",note_4, 127);
  }
}

void onButtonReleased(Button& btn, uint16_t duration){
  if(btn.is(sw_1)) {
      noteOffSend(note_1, 0, CCchannel);
      i2cSend("noteOff",note_1, 0);
  } else if (btn.is(sw_2)) {
      noteOffSend(note_2, 0, CCchannel);
      i2cSend("noteOff",note_2, 0);
  } else if (btn.is(sw_3)) {
      noteOffSend(note_3, 0, CCchannel);
      i2cSend("noteOff",note_3, 0);
  } else if (btn.is(sw_4)) {
      noteOffSend(note_4, 0, CCchannel);
      i2cSend("noteOff",note_4, 0);
  }
}

void ccSend(int cc, int value, int channel) {
  if(debug) {
    debugThis("cc", cc, value);
  } else {
    if(usbMIDI) {
      midiEventPacket_t event = {0x0B, 0xB0 | channel, cc, value};
      MidiUSB.sendMIDI(event);
      MidiUSB.flush();
    }
  }
}

void noteOnSend(int note, int vel, int channel) {
  if(debug) {
    debugThis("noteOn", note, vel);
  } else {
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
    if(usbMIDI) {
      midiEventPacket_t event = {0x08, 0x80 | channel, note, vel};
      MidiUSB.sendMIDI(event);
      MidiUSB.flush();
    }
  }
}

void i2cSend(char type, int cc, int value) {
  int eventType = 0x09;
  if(type == "noteOff") eventType = 0x08;
  else if(type == "cc") eventType = 0x0B;
  if(i2cMIDI) {
    Wire.beginTransmission(8);
    Wire.write(type);
    Wire.write(cc);
    Wire.write(value);
    Wire.endTransmission(); 
  }
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

int mapAndClamp(int input, int i) {
  int inMin[POT_COUNT] = {0,0,0,0};
  int inMax[POT_COUNT] = {1023,1023,1023,1023};
  int outval = map(input, inMin[i], inMax[i], 0, 127);
  if(outval < 0) outval = 0;
  if(outval>127) outval = 127;
  return (outval);
}

int isPotCC(int val) {
  for(int i=0; i<POT_COUNT; i++) {
    if(cc_pot[i] == val)
      return (i);
  }
  return (-1);
}

void intro() {
  int delayTime = 200;
  int i;
  for(i=0;i<4;i++) {
    digitalWrite(leds[i],HIGH);
    delay(delayTime);
  }
  for(i=0;i<4;i++) {
    digitalWrite(leds[i],LOW);
  }
  delay(delayTime * 2);
  for(i=0;i<4;i++) {
    digitalWrite(leds[i],HIGH);
  }
  delay(delayTime);
  for(i=0;i<4;i++) {
    digitalWrite(leds[i],LOW);
  }

}
