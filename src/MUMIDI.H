#ifndef MUMIDI_H
#define MUMIDI_H

#define MIDI_DATA   0x330
#define MIDI_STATUS 0x331


extern const char *midi_instruments[];
void playMIDINote(short);
void stopMIDINote(short);
#endif