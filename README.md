# MoMc: Modular MIDI Controller

![Controller image](https://raw.githubusercontent.com/jpcarrascal/MoMc/main/MoMc.png)

Arduino-based modular MIDI pedal controller.

* Motherboard: 5 footswitch MIDI* controller with external expression pedal input and USB-MIDI.
* Daughterboard: 4-knob, 4-button general purpose controller. Can work as an expansion for the Motherboard or as a standalone USB-MIDI controller.
* MIDI Class-compliant. Works as USB plug-and-play controller for computer software, no driver required.
* Motherboard MIDI-USB passthrough: it passes PC and CC messages from USB-MIDI through to USB ports. Acts (almost) like a USB MIDI interface, so it is possible to control pedals from a computer with no additional hardware.

(*) _The MIDI ports are 1/8" TRS jack, conforming to the MIDI standard described in [the MIDI.org website](https://www.midi.org/specifications/midi-transports-specifications/specification-for-use-of-trs-connectors-with-midi-devices-2) (sometimes referred as "TRS-A")._

### Dependencies
- r89m Buttons: https://github.com/r89m/Button
- r89m PushButton: https://github.com/r89m/PushButton
- Bounce2: https://github.com/thomasfredericks/Bounce2
- 47 effects Arduino MIDI library: https://github.com/FortySevenEffects/arduino_midi_library
