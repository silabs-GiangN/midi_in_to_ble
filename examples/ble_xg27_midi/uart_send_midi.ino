#include <MIDI.h>

#define DEBUG_SERIAL Serial
#define MIDI_SERIAL  Serial1

MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL, midiA);

void setup() {
  DEBUG_SERIAL.begin(115200);
  // pinMode(PB0, OUTPUT);

  DEBUG_SERIAL.println("MIDI SEND start");
  // put your setup code here, to run once:
  MIDI_SERIAL.begin(31250, SERIAL_8N1, 16, 17);
  midiA.begin(MIDI_CHANNEL_OMNI);
}

void loop() {
  DEBUG_SERIAL.println("MIDI send()");
  midiA.sendNoteOn(42, 127, 1);    // Send a Note (pitch 42, velo 127 on channel 1)
  // digitalWrite(PB0, HIGH);
  delay(100);
  midiA.sendNoteOff(42, 127, 1);
  // digitalWrite(PB0, LOW);
  delay(100);
}
