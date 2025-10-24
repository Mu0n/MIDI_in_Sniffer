#ifndef MUOPL_H
#define MUOPL_H


#define OPL_ADDR 0x388
#define OPL_DATA 0x389


//scope OPL2
#define OPL_EN   0x01
#define OPL_T1   0x02
#define OPL_T2   0x03
#define OPL_IRQ  0x04
#define OPL_CSW  0x08
#define OPL_PERC 0xBD

//scope channel
#define OPL_CH_F_LO    0xA0
#define OPL_CH_KBF_HI  0xB0
#define OPL_CH_FEED    0xC0

//scope operator
#define OPL_OP_TVSKF  0x20
#define OPL_OP_KSLVOL 0x40
#define OPL_OP_AD     0x60
#define OPL_OP_SR     0x80
#define OPL_OP_WAV    0xE0

typedef struct oplInst {
  short OP2_TVSKF,  OP1_TVSKF;
  short OP2_KSLVOL, OP1_KSLVOL;
  short OP2_AD,     OP1_AD;
  short OP2_SR,     OP1_SR;
  short OP2_WAV,    OP1_WAV;
  short CHAN_FEED;
} oplI;


void opl_initialize(void);
void opl_quietAll(void);
void opl_write(int, short);
void opl_note(short, int, short,short);
void opl_setInst(struct oplInst *, short);
void opl_setInstAll(short);

extern const int opl_fnums[];
extern const oplI opl_instrument_defs[];

#endif