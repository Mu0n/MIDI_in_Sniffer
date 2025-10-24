#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <MUMIDI.C>
#include <OPL.C>
#include <KEYBKEY.C>

#define IRQ_NUM 9
#define IRQ_VECTOR 0x08
#define PIT_CONTROL 0x43
#define PIT_CHANNEL2 0x42
#define SPEAKER_CTRL 0x61
#define EVERYMS 32
#define NBPOLY 6

char pressed[128]={0};

const unsigned int delays[7] = {0, 64, 32, 21, 16, 13, 10};
// Lookup table for PIT divisors mapped from MIDI note numbers
// (frequency = 440 * 2^((note - 69)/12), divisor = 1193180 / frequency)
const unsigned int pitDivisors[128] = {
145955, 137741, 130047, 122692, 115843, 109341, 103194, 97402,
91960, 86777, 81865, 77354, 72977, 68870, 65023, 61346,
57921, 54670, 51597, 48701, 45980, 43388, 40932, 38677,
36489, 34435, 32512, 30673, 28961, 27335, 25798, 24351,
22990, 21694, 20466, 19338, 18244, 17218, 16256, 15337,
14480, 13668, 12899, 12175, 11495, 10847, 10242, 9661,
9122, 8609, 8128, 7668, 7240, 6834, 6450, 6088,
5745, 5424, 5119, 4833, 4561, 4304, 4063, 3835,
3620, 3417, 3225, 3044, 2873, 2712, 2559, 2416,
2280, 2152, 2032, 1917, 1810, 1708, 1612, 1522,
1437, 1356, 1280, 1208, 1140, 1076, 1016,  959,
 905,  854,  806,  761,  718,  678,  640,  604,
 570,  538,  508,  452,  427,  403,  380,  359,
 339,  320,  302,  285,  269,  254,  226,  214,
 202,  190,  180,  169,  106,  151,  143,  135,
 127,  113,  107,  101,   95,   90,   85,   80
};

int fuOPL = 5;
int target = 1; //0=PC Speaker 1=MIDI OUT 2=OPL2 at 220h
int currentMIDIinst = 0;

short polyOPLBuffer[9]={0,0,0, 0,0,0, 0,0,0};
oplI currentI;
char str[30];

volatile int everyMSVal = EVERYMS;

volatile unsigned char currentNote = 0;

int polyCount = 0;
int polyOPLCount = 0;
int polyIndex = 0;
int playNote = 0;

volatile unsigned char last_byte=0;
volatile int byte_ready = 0;
short polyBuffer[NBPOLY] = {0,0,0,0,0,0};

void toggleSpeaker(int, unsigned char);

void interrupt (*oldTimerISR)(void);
void toggleSpeaker(int, unsigned char);
int isAlreadyPlaying(unsigned char);
void setupTimer(void);
void restoreTimer(void);
void spkOff(void);
void targetAnnounce(int);
void writeHeader(void);
void put_string_at(int, int, const char *, unsigned char);
void programMIDIinst(void);
void bumpMIDIinst(int);
short findFreeChannel(short *, short);
short liberateChannel(short, short *, short);
void updateOPLText(void);
void randomizeOPL(void);
unsigned char poll_keyboard(void);
void dispatch(int, int);
void restoreOPL(void);
void refreshOPLDefText(void);
void updateSPKText(void);
/*
int findFreePoly(int *, short);
int liberatePoly(int *, unsigned int, short);
  */

void restoreOPL()
{
currentI.OP2_TVSKF = opl_instrument_defs[0].OP2_TVSKF;
currentI.OP1_TVSKF = opl_instrument_defs[0].OP1_TVSKF;
currentI.OP2_KSLVOL = opl_instrument_defs[0].OP2_KSLVOL;
currentI.OP1_KSLVOL = opl_instrument_defs[0].OP1_KSLVOL;
currentI.OP2_AD = opl_instrument_defs[0].OP2_AD;
currentI.OP1_AD = opl_instrument_defs[0].OP1_AD;
currentI.OP2_SR = opl_instrument_defs[0].OP2_SR;
currentI.OP1_SR = opl_instrument_defs[0].OP1_SR;
currentI.OP2_WAV = opl_instrument_defs[0].OP2_WAV;
currentI.OP1_WAV = opl_instrument_defs[0].OP1_WAV;

}
unsigned char poll_keyboard()
{
 unsigned char code;
 int is_release;
 int midi;
 unsigned char scancode;
 char str[2];
 short i=0;

 if(inp(0x64)&1)
	{
	code = inp(0x60);
	is_release = code & 0x80;
	scancode = code & 0x7F;

	sprintf(str,"%02x",scancode);
	//put_string_at(10,10,str,0x07);
	if(scancode==0x01) //ESC
		{
		spkOff();
		outp(MIDI_STATUS, 0x3F);
		restoreTimer();

		return 0xFF; //ESC quit the program
		}

	midi = scan_to_midi[scancode];
	if(is_release) //acts on key releases
		{
		if(pressed[midi] && midi !=-1)
			{
			dispatch(midi+12,0);
			pressed[midi]=0;
			}
		}
	else    {     //acts on key presses
		if(scancode==0x3b) //F1
		  {
		  target--;
		  if(target<0) target =2;
		  spkOff();
		  outp(MIDI_STATUS, 0x3F);
		  opl_initialize();
		  writeHeader();
		  polyCount=0;
		  for(i=0;i<6;i++) polyBuffer[i]=0;
		  for(i=0;i<9;i++) polyOPLBuffer[i]=0;

		  if(target==0)
			{
			updateSPKText();
			}
		  if(target==2)
			{
			updateOPLText();
			refreshOPLDefText();
			}
		  }
		if(scancode == 0x3d && target==2) //F3
			randomizeOPL();
		if(target ==1 && scancode==0x4e) bumpMIDIinst(1); //numpad+
		if(target ==1 && scancode==0x4a) bumpMIDIinst(-1); //numpad-


		if(!pressed[midi] && midi !=-1)
			{
			dispatch(midi+12,1);
			pressed[midi]=1;
			}
		}
	}
 return 0;
 }

void put_string_at(int x, int y, const char *str, unsigned char attr)
{
unsigned char far *video = (unsigned char far *)0xB8000000;
int offset = ((y) * 80 + (x)) * 2;

while(*str)
	{
	video[offset] = *str++;
	video[offset+1] = attr;
	offset+=2;
	}
}

void randomizeOPL()
{
char str[35];
 short i=0;
 currentI.OP2_TVSKF = rand()&0xFF;
 currentI.OP1_TVSKF = rand()&0xFF;
 currentI.OP2_KSLVOL = rand()&0xC0;
 currentI.OP1_KSLVOL = rand()&0xC0;
 currentI.OP2_AD = rand()&0xFF;
 currentI.OP1_AD = rand()&0xFF;
 currentI.OP2_SR = rand()&0xFF;
 currentI.OP1_SR = rand()&0xFF;
 currentI.OP2_WAV = rand()&0xFF;
 currentI.OP1_WAV = rand()&0xFF;
 for(i=0;i<9;i++) opl_setInst((oplI *)&currentI,i);
 refreshOPLDefText();
}
void refreshOPLDefText()
{
 put_string_at(0,4," [F3] Randomize Instrument",0x0E);
 sprintf(str,"OP2 %02x %02x %02x %02x %02x",currentI.OP2_TVSKF,
					currentI.OP2_KSLVOL,
					currentI.OP2_AD,
					currentI.OP2_SR,
					currentI.OP2_WAV);
 put_string_at(0,5,str,0x07);
 sprintf(str,"OP1 %02x %02x %02x %02x %02x",currentI.OP1_TVSKF,
					currentI.OP1_KSLVOL,
					currentI.OP1_AD,
					currentI.OP1_SR,
					currentI.OP1_WAV);
 put_string_at(0,6,str,0x07);

}
void updateOPLText()
{
short i=0;
char str[4];

for(i=0;i<9;i++)
{
 sprintf(str,"%2d",i);
 put_string_at(1,7+i,str,0x07);
 sprintf(str,"%02x",polyOPLBuffer[i]);
 put_string_at(4,7+i,str,0x07);
}
}
void targetAnnounce(int which)
{
char str[3];

switch(which)
	{
	case 0:
	  put_string_at(0,2,"PC Speaker",0x0E);
	  break;
	case 1:
	  put_string_at(0,2," MIDI Out ",0x0E);
	  put_string_at(0,4,"+/-",0x07);
	  sprintf(str,"%03d",currentMIDIinst);
	  put_string_at(4,4,str,0x0E);
	  put_string_at(8,4,midi_instruments[currentMIDIinst],0x0E);
	  break;
	case 2:
	  put_string_at(0,2," FM OPL2  ",0x0E);
	  break;
	}
}

void writeHeader()
{
    put_string_at(0,0," ",0x07);
    clrscr();
    put_string_at(0,0,"MIDI Sniffer v0.3 Oct 2025 by 1BitFeverDreams",0x1E);
    put_string_at(0,1,"Listening on MIDI controller UART 0x330 & keystrokes on regular keyboard",0x07);
    targetAnnounce(target);
    put_string_at(11,2,"is the current target",0x07);
    put_string_at(0,3,"[F1] Cycle target - [ESC] to quit",0x0E);

}
/*
int isAlreadyPlaying(unsigned char note)
{
int i;
for(i=0; i< NBPOLY; i++) if(polyBuffer[i] == note) return i;
return -1;
}
*/

void interrupt newTimerISR(void)
{
int startIndex=polyIndex;
//int found =0;

  if(everyMSVal == 0)
  {
    everyMSVal = delays[polyCount]; //reset the delay
    if(polyCount>0) //switch the next valid note in the buffer
      {
      startIndex = polyIndex;
      //found =0;
      do {
	 polyIndex = (polyIndex + 1 ) % NBPOLY;
	 if(polyBuffer[polyIndex] !=0)
		{
		//found = 1;
		break;
		}
	 } while(polyIndex != startIndex);
      }
  }
  else everyMSVal--;
//oldTimerISR();
  outp(0x20,0x20);
}
void setupTimer()
{
disable();
oldTimerISR = getvect(IRQ_VECTOR);
setvect(IRQ_VECTOR, newTimerISR);
enable();

outp(0x43, 0x36);
outp(0x40, 1193 & 0xFF);
outp(0x40, (1193 >> 8) &0xFF);
}

void restoreTimer()
{
disable();
setvect(IRQ_VECTOR, oldTimerISR);
enable();

outp(0x43, 0x36);
outp(0x40, 0);
outp(0x40, 0);
}

void dispatch(int note, int onOrOff)
{
short j=0;
short foundFreeChan=0, result=0;
switch(target)
	{
	case 0: //pc spk

	  if(onOrOff){
	   foundFreeChan = findFreeChannel((short *)polyBuffer, 6);
	   if(foundFreeChan != -1)
	    {
	    polyBuffer[foundFreeChan]=note;
	    polyCount++;
	    sprintf(str,"0x%02x", note);
	    put_string_at(1,20,str,0x07);
	    updateSPKText();
	    }

	   }
	   else
	     {
	     foundFreeChan = liberateChannel(note,polyBuffer,6);
	     if(foundFreeChan != -1)
	      {
	      polyBuffer[foundFreeChan] = 0;
	      polyCount--;
	      if(polyCount < 0) polyCount =0;
	      updateSPKText();
	      }
	     }
	  break;

	case 1: //midi
	  if(onOrOff) playMIDINote(note);
	  else stopMIDINote(note);
	  break;
	case 2: //opl2
	  if(onOrOff){   //note on OPL
	   foundFreeChan = findFreeChannel(polyOPLBuffer, 9);
	   if(foundFreeChan != -1)
	    {
	    opl_note(foundFreeChan,
		    opl_fnums[(note+fuOPL)%12],
		    (note+fuOPL)/12-2,
		    1);
	    polyOPLBuffer[foundFreeChan]=note;
	    polyOPLCount++;
	    sprintf(str,"0x%02x", note);
	    put_string_at(1,20,str,0x07);
	    updateOPLText();
	    }

	   } //end if note on OPL
	   else    //note off OPL
	     {
	     foundFreeChan = liberateChannel(note,polyOPLBuffer,9);
	     if(foundFreeChan != -1)
	      {
		opl_note(foundFreeChan,
			 opl_fnums[(note+fuOPL)%12],
			 (note+fuOPL)/12-2,
			 0);
		polyOPLBuffer[foundFreeChan] = 0;
	      polyOPLCount--;
	      if(polyOPLCount < 0) polyOPLCount =0;
	      updateOPLText();
	      }
	     } //end else note off OPL
	  break;
	}  // end switch
}
void spkOff()
{
	unsigned char tmp;

	disable();
	tmp = inp(SPEAKER_CTRL);
	outp(SPEAKER_CTRL, tmp & ~0x03);            // Turn speaker off
	enable();

}
void toggleSpeaker(int on, unsigned char midiNote) {
    unsigned char tmp;
    unsigned int divisor;

    if (on) {
	if (midiNote > 127) return; // Sanity check

	divisor = pitDivisors[midiNote];
	disable();
	outp(PIT_CONTROL, 0xB6);
	outp(PIT_CHANNEL2, divisor & 0xFF);         // Low byte
	outp(PIT_CHANNEL2, (divisor >> 8) & 0xFF);  // High byte

	tmp = inp(SPEAKER_CTRL);
	outp(SPEAKER_CTRL, tmp | 0x03);             // Turn speaker on
	enable();
    } else {
	spkOff();
   }
}

short findFreeChannel(short *ptr, short howMany)
{
 int i=0;
 for(i=0;i<howMany;i++)
	{
	if(ptr[i]==0) return i;
	}
 return -1;
}

short liberateChannel(short note, short *ptr, short howMany)
{
 short i=0;
 for(i=0;i<howMany;i++)
	{
	if(ptr[i] == note) return i;

	}
 return -1;
}

void programMIDIinst()
{
outp(MIDI_DATA,0xC0);
outp(MIDI_DATA,currentMIDIinst);
}

void bumpMIDIinst(int upDown)
{
if(upDown == 1)
	{
	currentMIDIinst++;
	if(currentMIDIinst == 128) currentMIDIinst = 0;
	}
else
	{
		if(currentMIDIinst == 0)
		{
		currentMIDIinst=127;
		}
		else currentMIDIinst--;
	}
programMIDIinst();
writeHeader();
}
void main() {
    int ch =0; //for getch
    int oplOn = 0;
    int needVeloc =0;
    int writeChange = 0; //will be 1 when we need to rewrite the screen
    int noteOn = 0;
    int noteOff = 0;
    int result = 0;
    int firstMIDIb = 0, secondMIDIb = 0, thirdMIDIb = 0;
    int j=0;


    //int timebuf=10000;
    textmode(C80);
    clrscr();
    writeHeader();

    // Wake up MPU-401 into UART mode
    opl_initialize();
    opl_setInstAll(0);
    restoreOPL();

    outp(MIDI_STATUS, 0x3F);
    delay(100);

    for(polyIndex = 0 ; polyIndex < NBPOLY; polyIndex++)
	{
	polyBuffer[polyIndex] = 0;
	}
setupTimer();

while (1){ // ESC to quit
	if(!(inp(MIDI_STATUS) & 0x80)) {
	last_byte = inp(MIDI_DATA);
	byte_ready = 1;
		}
	if(byte_ready)
		{
		byte_ready = 0;
		if(last_byte == 0xFE) continue;

		if(needVeloc)
			{
			needVeloc = 0;
			thirdMIDIb = last_byte;

			firstMIDIb = secondMIDIb = thirdMIDIb = 0;
			}

		if(noteOn)
		  {

		  needVeloc = 1;
		  noteOn = 0;
		  secondMIDIb = last_byte;
		  dispatch(last_byte, 1);
		  }
		if(noteOff)
		  {
		  noteOff = 0;
		  needVeloc = 1;
		  secondMIDIb = last_byte;
		  dispatch(last_byte,0);

		  }
		if((last_byte & 0xF0) == 0x90 && noteOn == 0 && noteOff == 0)
		  {
		  //if(target==0 && polyCount == NBPOLY) continue;
		  firstMIDIb = last_byte; //keep score of midi command
		  noteOn = 1;
		  }
		else if((last_byte & 0xF0) == 0x80 && noteOn == 0 && noteOff ==0)
		  {
		  //if(target==0) continue;
		  firstMIDIb = last_byte; //keep score of midi command
		  noteOff = 1;
		  }
		}

	if(polyCount > 0 && target==0) //time to switch notes
		{

		if(polyBuffer[polyIndex] != 0 && polyBuffer[polyIndex] != currentNote)
		  {
		  toggleSpeaker(1, polyBuffer[polyIndex]);
		  currentNote = polyBuffer[polyIndex];
		  }
		}//enf if polyCount >0
	  else if(polyCount==0 && target==0)
		{

		if(currentNote !=0)
			{
			toggleSpeaker(0,0);
			currentNote=0;
			}

		}//end else polycount >0
	if(poll_keyboard()==0xFF) return;
	}           //end of while


}//end of main

void updateSPKText()
{
short j=0;

for(j=0;j<NBPOLY;j++)
    {
    sprintf(str,"%d: %02x %d",j,polyBuffer[j],pitDivisors[polyBuffer[j]]);
    put_string_at(1,6+j,str,0x07);
    }
sprintf(str,"polycount %02d",polyCount);
put_string_at(1,6+NBPOLY,str,0x07);

}