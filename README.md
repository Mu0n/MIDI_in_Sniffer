# MIDI_in_Sniffer
MSDOS compatible program coded in Borland C to test your vintage PC's MIDI in capability through gameport pin 15.

## Hardware required
* Vintage PC that can run MSDOS
* A 15-pin gameport like the ones on Sound Blaster
* A way to route opto-isolated MIDI in signals through pin 15 of that gameport. See the PC Gameport Party project https://github.com/Mu0n/PCGameportParty I have to fill that need, a video describing it https://youtu.be/SGnbr5O1Wa0 and a storefront page to buy pre-assembled: https://jcm-1.com/product/pc-gameport-party/
* a MIDI capable controller (ie piano)

## What it can do
* Read incoming MIDI signals from your MIDI controller
* Patch it to MIDI out (pin 12 of the gameport) at port 330h
* Patch it to the PC speaker and let you play up to 6 note polyphony using "time sharing" aka arpeggiato
* Patch it to your Sound Blaster's OPL2 playing capability at port 388h
* Generate a randomized "instrument definition" in OPL2 to allow you to find new sounds
