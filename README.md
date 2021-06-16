# MoMc: Modular MIDI Controller

![Controller image](https://raw.githubusercontent.com/jpcarrascal/MoMc/main/MoMc.png)

Arduino-based modular MIDI pedal controller.

* *Motherboard*: 5 footswitch MIDI* controller with external expression pedal input and USB-MIDI.
* *Daughterboard*: 4-knob, 4-button general purpose controller. Can work as an expansion for the Motherboard or as a standalone USB-MIDI controller.
* MIDI Class-compliant. Works as USB plug-and-play controller for computer software, no driver required.
* Motherboard MIDI-USB passthrough: it passes PC and CC messages from USB-MIDI through to USB ports. Acts (almost) like a USB MIDI interface, so it allows bidirectional MIDI between external devices and a computer.

![Block diagram](https://raw.githubusercontent.com/jpcarrascal/MoMc/main/MoMc-BlockDiagram.png)

(*) _MIDI ports are 1/8" TRS jack, conforming to the MIDI standard described in [the MIDI.org website](https://www.midi.org/specifications/midi-transports-specifications/specification-for-use-of-trs-connectors-with-midi-devices-2) (sometimes referred as "TRS-A")._

### Latest changes
- 2021-06-16: Added MMC Start (top-left footswitch) and Stop (top-right footswitch).

### Dependencies
- r89m Buttons: https://github.com/r89m/Button
- r89m PushButton: https://github.com/r89m/PushButton
- Bounce2: https://github.com/thomasfredericks/Bounce2
- 47 effects Arduino MIDI library: https://github.com/FortySevenEffects/arduino_midi_library

### TODO
- Extend passthrough to support any MIDI message type.
- MMC commands are implemented using a workaround (MidiUSB_sendSysEx()) described here: https://github.com/arduino-libraries/MIDIUSB/issues/19
  This should be replaced by a proper implementation of SysEx/MMC using the USB transport of FortySevenEffects Arduino MIDI Library
  info here: https://github.com/lathoub/Arduino-
