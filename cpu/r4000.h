#include <windows.h>
#include "os.h"
//---------------------------------------------------------------------------
#ifndef __r4000H__
#define __r4000H__
//---------------------------------------------------------------------------
typedef enum {zr=0, at, v0, v1, a0, a1, a2, a3,
		t0, t1, t2, t3, t4, t5, t6, t7,s0, s1, s2, s3, s4, s5, s6, s7,
		t8, t9, k0, k1, gp, sp, fp, ra,f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,
       f11,f12,f13,f14,f15,f16,f17,f18,f19,f20,f21,f22,f23,f24,f25,f26,
       f27,f28,f29,f30,f31,PC,LO,HI} Registers;

#define RT(OP)         ((OP >> 16) & 0x1F)
#define RS(OP)         ((OP >> 21) & 0x1F)
#define RD(OP)         ((OP >> 11) & 0x1F)
#define FT(OP)         ((OP >> 16) & 0x1F)
#define FS(OP)         ((OP >> 11) & 0x1F)
#define FD(OP)         ((OP >> 6)  & 0x1F)
#define SA(OP)         ((OP >> 6)  & 0x1F)
#define IMM(OP)        (int)(short)OP
#define IMMU(OP)       (unsigned long)(unsigned short)OP
#define JUMP(PC,OP)    ((PC & 0xF0000000) | ((OP & 0x3FFFFFF) << 2))
#define CODE(OP)       ((OP >> 6) & 0xFFFFF)
#define SIZE(OP)       ((OP >> 11) & 0x1F)
#define POS(OP)        ((OP >> 6) & 0x1F)
#define MASK(a)        ((1 << a) - 1)
#define VO(OP)         (((OP & 3) << 5) | ((OP >> 16) & 0x1F))
#define VCC(OP)        ((OP >> 18) & 7)
#define VD(OP)         (OP & 0x7F)
#define VS(OP)         ((OP >> 8) & 0x7F)
#define VT(OP)         ((OP >> 16) & 0x7F)
#define VED(OP)        (OP & 0xFF)
#define VES(OP)        ((OP >> 8) & 0xFF)
#define VCN(OP)        (OP & 0x0F)
#define VI3(OP)        ((OP >> 16) & 0x07)
#define VI5(OP)        ((OP >> 16) & 0x1F)
#define VI8(OP)        ((OP >> 16) & 0xFF)

#define MAX_TIMESLICE  10*333
#define INS_CYCLES     16

typedef struct {
   char            name[10];
	unsigned int    opcode;
	unsigned int    mask;
	char            fmt[30];
	int             addrtype;
	int             type;
} Instruction;

class LCPU : public LOS
{
public:
   LCPU();
   ~LCPU();
   BOOL Init();
   void Reset();
   void set_EntryAddress(DWORD value,DWORD gp);
   void dis_ins(DWORD address,char *out,int maxLen);
   void dis_reg(Registers reg,char *out,int maxLen);
   double get_Register(Registers value);
   double get_Register(char *name);
protected:
};

int exec_cpu();

typedef int (*LPCPUFUNC)(void);

#endif
