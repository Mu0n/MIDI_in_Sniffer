#include "opl.h"


const oplI opl_instrument_defs[]={

 {0x21,0xC0, 0x01,0x15,
  0xF4,0xF2, 0x25,0x40,
  0x04,0x07,  0x30}
};


const int opl_fnums[] = { 0x205, 0x223, 0x244, 0x267, 0x28B, 0x2B2,
			  0x2DB, 0x306, 0x334, 0x365, 0x399, 0x3CF};

void opl_initialize(void)
{
int i=0;
opl_write(OPL_EN, 0x20); //chip wide reset
opl_write(OPL_T1, 0x00);
opl_write(OPL_T2, 0x00);
opl_write(OPL_CSW, 0x00);

for(i=0;i<9;i++) opl_write(OPL_CH_FEED + i, 0x30);
opl_quietAll();
}

void opl_quietAll(void)
{
int channel =0;
for(channel=0;channel<9; channel++) opl_write(OPL_CH_KBF_HI | channel, 0x00);
}

void opl_write(int addr, short value)
{
outp(OPL_ADDR, addr);
outp(OPL_DATA, value);
}

void opl_note(short channel, int fnum, short block, short onOrOff)
{
opl_write(OPL_CH_F_LO | channel, fnum & 0x00FF);
opl_write(OPL_CH_KBF_HI | channel,
          ((fnum >> 8) & 0x03) | ((int)block<<2) | (onOrOff==1?0x20:0x00));

}

void opl_setInst(struct oplInst *inst, short chan)
{
short offset = chan+0x00;
if(chan>2) offset += 0x05;
if(chan>5) offset += 0x05;

opl_write(OPL_OP_TVSKF | offset,        inst->OP1_TVSKF);
opl_write(OPL_OP_TVSKF | (offset+0x03), inst->OP2_TVSKF);

opl_write(OPL_OP_KSLVOL | offset,        inst->OP1_KSLVOL);
opl_write(OPL_OP_KSLVOL | (offset+0x03), inst->OP2_KSLVOL);

opl_write(OPL_OP_AD | offset,        inst->OP1_AD);
opl_write(OPL_OP_AD | (offset+0x03), inst->OP2_AD);

opl_write(OPL_OP_SR | offset,        inst->OP1_SR);
opl_write(OPL_OP_SR | (offset+0x03), inst->OP2_SR);

opl_write(OPL_OP_WAV | offset,        inst->OP1_WAV);
opl_write(OPL_OP_WAV | (offset+0x03), inst->OP2_WAV);
}

void opl_setInstAll(short which)
{
short c=0;
for(c=0;c<9;c++)
	{
	opl_setInst((oplI *)&opl_instrument_defs[which],c);
	opl_write(OPL_CH_FEED + c, opl_instrument_defs[which].CHAN_FEED);
	}
}