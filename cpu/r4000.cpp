#include "r4000.h"
#include "vfpu.h"
#include "lpsp.h"
#include <math.h>

CPU_BASE R4000;
psp_thread *current_thread;
U64 ticks,start_time;
//-NAN 0xFFFFFF78
//---------------------------------------------------------------------------
static int unknow_ins()
{
   return INS_CYCLES;    //89e61d0
}
//---------------------------------------------------------------------------
static int slti()
{
   R4000.reg[RT(R4000.opcode)] = ((int)R4000.reg[RS(R4000.opcode)] < IMM(R4000.opcode));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sltiu()
{
   R4000.reg[RT(R4000.opcode)] = (R4000.reg[RS(R4000.opcode)] < IMMU(R4000.opcode));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sltu()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] < R4000.reg[RT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int slt()
{
   R4000.reg[RD(R4000.opcode)] = (int)R4000.reg[RS(R4000.opcode)] < (int)R4000.reg[RT(R4000.opcode)] ? 1 : 0;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int j()
{
   R4000.next_pc = JUMP(R4000.pc,R4000.opcode)-4;
   R4000.jump = 2;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int jalr()
{
   R4000.reg[RD(R4000.opcode)] = R4000.pc + 8;
   R4000.next_pc = R4000.reg[RS(R4000.opcode)]-4;
   R4000.jump = 2;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int jal()
{
   R4000.reg[31] = R4000.pc + 8;
   R4000.next_pc = JUMP(R4000.pc,R4000.opcode)-4;
   R4000.jump = 2;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int jr()
{
   R4000.next_pc = R4000.reg[RS(R4000.opcode)]-4;
   R4000.jump = 2;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lwc()
{
   int a;

   a = read_dword(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode));
   R4000.sp_reg[FT(R4000.opcode)] = *((float *)&a);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lw()
{
   R4000.reg[RT(R4000.opcode)] = (unsigned long)((int)read_dword(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode)));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lwl()
{
   unsigned long adr,value,res;

   adr = R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode);
   value = read_dword(adr);
   res = R4000.reg[RT(R4000.opcode)];
   switch(adr & 3){
       case 0:
           res = (res & 0xFFFFFF) | (value << 24);
       break;
       case 1:
           res = (res & 0xFFFF) | (value << 16);
       break;
       case 2:
           res = (res & 0xFF) | (value << 8);
       break;
       case 3:
           res = value;
       break;
   }
   R4000.reg[RT(R4000.opcode)] = res;   
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lwr()
{
   unsigned long adr,value,res;

   adr = R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode);
   value = read_dword(adr);
   res = R4000.reg[RT(R4000.opcode)];
   switch(adr & 3){
       case 0:
           res = value;
       break;
       case 1:
           res = (res & 0xFF000000) | (value >> 8);
       break;
       case 2:
           res = (res & 0xFFFF0000) | (value >> 16);
       break;
       case 3:
           res = (res & 0xFFFFFF00) | (value >> 24);
       break;
   }
   R4000.reg[RT(R4000.opcode)] = res;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lhu()
{
   R4000.reg[RT(R4000.opcode)] = read_word(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lh()
{
   R4000.reg[RT(R4000.opcode)] = (unsigned long)(int)(signed short)read_word(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lbu()
{
   R4000.reg[RT(R4000.opcode)] = (unsigned long)read_byte(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lb()
{
   R4000.reg[RT(R4000.opcode)] = (unsigned long)(int)(signed char)read_byte(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sb()
{
   write_byte(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode),R4000.reg[RT(R4000.opcode)]);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sh()
{
   write_word(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode),R4000.reg[RT(R4000.opcode)]);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sw()
{
   write_dword(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode),R4000.reg[RT(R4000.opcode)]);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int swl()
{
   unsigned long adr,value,reg;

   adr = R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode);
   reg = R4000.reg[RT(R4000.opcode)];
   value = read_dword(adr);
   switch(adr & 3){
       case 0:
           reg = (value & 0xFFFFFF00) | (reg >> 24);
       break;
       case 1:
           reg = (value & 0xFFFF0000) | (reg >> 16);
       break;
       case 2:
           reg = (value & 0xFF000000) | (reg >> 8);
       break;
   }
   write_dword(adr,reg);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int swr()
{
   unsigned long adr,value,reg;

   adr = R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode);
   reg = R4000.reg[RT(R4000.opcode)];
   value = read_dword(adr);
   switch(adr & 3){
       case 1:
           reg = (reg << 8) | (value & 0xFF);
       break;
       case 2:
           reg = (reg << 16) | (value & 0xFFFF);
       break;
       case 3:
           reg = (reg << 24) | (value & 0xFFFFFF);
       break;
   }
   write_dword(adr,reg);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int swc()
{
   write_dword(R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode),
       *((unsigned long *)&R4000.sp_reg[FT(R4000.opcode)]));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int lui()
{
   R4000.reg[RT(R4000.opcode)] = IMM(R4000.opcode) << 16;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int addui()
{
   R4000.reg[RT(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] + IMM(R4000.opcode);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int ori()
{
   R4000.reg[RT(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] | IMMU(R4000.opcode);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int andi()
{
   R4000.reg[RT(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] & IMMU(R4000.opcode);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int xori()
{
   R4000.reg[RT(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] ^ IMMU(R4000.opcode);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int mflo()
{
    R4000.reg[RD(R4000.opcode)] = R4000.lo;
    return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int mfhi()
{
    R4000.reg[RD(R4000.opcode)] = R4000.hi;
    return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int mtlo()
{
    R4000.lo = R4000.reg[RS(R4000.opcode)];
    return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int mult()
{
   S64 value;

   value = (S64)(int)R4000.reg[RS(R4000.opcode)] * (S64)(int)R4000.reg[RT(R4000.opcode)];
   R4000.lo = (unsigned long)value;
   R4000.hi = (unsigned long)(value >> 32);
   return 12;
}
//---------------------------------------------------------------------------
static int multu()
{
   U64 value;

   value = (U64)(unsigned int)R4000.reg[RS(R4000.opcode)] * (U64)(unsigned int)R4000.reg[RT(R4000.opcode)];
   R4000.lo = (unsigned long)value;
   R4000.hi = (unsigned long)(value >> 32);
   return 12;
}
//---------------------------------------------------------------------------
static int sll_imm()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RT(R4000.opcode)] << POS(R4000.opcode);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sll()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RT(R4000.opcode)] << (R4000.reg[RS(R4000.opcode)] & 0x1F);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int srl_imm()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RT(R4000.opcode)] >> POS(R4000.opcode);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int srl()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RT(R4000.opcode)] >> (R4000.reg[RS(R4000.opcode)] & 0x1F);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sra_imm()
{
   R4000.reg[RD(R4000.opcode)] = (int)R4000.reg[RT(R4000.opcode)] >> POS(R4000.opcode);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sra()
{
   R4000.reg[RD(R4000.opcode)] = (int)R4000.reg[RT(R4000.opcode)] >> (R4000.reg[RS(R4000.opcode)] & 0x1F);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int movn()
{
   if(R4000.reg[RT(R4000.opcode)] != 0)
       R4000.reg[RD(R4000.opcode)] = R4000.reg[RS(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int movz()
{
   if(R4000.reg[RT(R4000.opcode)] == 0)
       R4000.reg[RD(R4000.opcode)] = R4000.reg[RS(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int div()
{
   int d;

   d = (int)R4000.reg[RT(R4000.opcode)];
   if(d != 0){
       R4000.lo = (int)R4000.reg[RS(R4000.opcode)] / d;
       R4000.hi = (int)R4000.reg[RS(R4000.opcode)] % d;
   }
   return 75;
}
//---------------------------------------------------------------------------
static int divu()
{
   unsigned long d;

   d = R4000.reg[RT(R4000.opcode)];
   if(d != 0){
       R4000.lo = R4000.reg[RS(R4000.opcode)] / d;
       R4000.hi = R4000.reg[RS(R4000.opcode)] % R4000.reg[RT(R4000.opcode)];
   }
   return 75;
}
//---------------------------------------------------------------------------
static int clz()
{
   unsigned long value;   //8a01ee4
   int i;
                                                        
   value = R4000.reg[RS(R4000.opcode)];
   for(i=31;i>=0;i--){
       if((value >> i) == 1){
           R4000.reg[RD(R4000.opcode)] = 31-i;
           return INS_CYCLES;
       }
   }
   R4000.reg[RT(R4000.opcode)] = 32;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int madd()
{
   S64 c;

   c = (S64)(int)R4000.hi;
   c <<= 32;
   c |= R4000.lo;
   c += ((int)R4000.reg[RS(R4000.opcode)]*(int)R4000.reg[RT(R4000.opcode)]);
   R4000.lo = (unsigned long)c;
   R4000.hi = (unsigned long)(c >> 32);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int add()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] + R4000.reg[RT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int _and()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] & R4000.reg[RT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int _or()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] | R4000.reg[RT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int _xor()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] ^ R4000.reg[RT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int nor()
{
   R4000.reg[RD(R4000.opcode)] = ~(R4000.reg[RS(R4000.opcode)] | R4000.reg[RT(R4000.opcode)]);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int _min()
{
   int a,b;

   if((a = R4000.reg[RS(R4000.opcode)]) > (b = R4000.reg[RT(R4000.opcode)]))
       a = b;
   R4000.reg[RD(R4000.opcode)] =  a;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int _max()
{
   int a,b;

   if((a = R4000.reg[RS(R4000.opcode)]) < (b = R4000.reg[RT(R4000.opcode)]))
       a = b;
   R4000.reg[RD(R4000.opcode)] =  a;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int subu()
{
   R4000.reg[RD(R4000.opcode)] = R4000.reg[RS(R4000.opcode)] - R4000.reg[RT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int blez()
{
   if((int)R4000.reg[RS(R4000.opcode)] <= 0){
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
       R4000.jump = 2;
   }
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int beq()
{
   if(R4000.reg[RT(R4000.opcode)] == R4000.reg[RS(R4000.opcode)]){
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
       R4000.jump = 2;
   }
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int beql()
{
   if(R4000.reg[RT(R4000.opcode)] == R4000.reg[RS(R4000.opcode)]){
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
       R4000.jump = 2;
   }
   else
       R4000.pc += 4;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bne()
{
   if(R4000.reg[RT(R4000.opcode)] != R4000.reg[RS(R4000.opcode)]){
       R4000.jump = 2;
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
   }
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bnel()
{
   if(R4000.reg[RT(R4000.opcode)] != R4000.reg[RS(R4000.opcode)]){
       R4000.jump = 2;
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
   }
   else
       R4000.pc += 4;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bltz()
{
   if((int)R4000.reg[RS(R4000.opcode)] < 0){
       R4000.jump = 2;
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
   }
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bltzl()
{
   if((int)R4000.reg[RS(R4000.opcode)] < 0){
       R4000.jump = 2;
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
   }
   else
       R4000.pc += 4;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bgez()
{
    if((int)R4000.reg[RS(R4000.opcode)] >= 0){
        R4000.jump = 2;
        R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
    }
    return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bgezl()
{
    if((int)R4000.reg[RS(R4000.opcode)] >= 0){
        R4000.jump = 2;
        R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
    }
   else
       R4000.pc += 4;
    return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bgtzl()
{
   if((int)R4000.reg[RS(R4000.opcode)] > 0){
       R4000.jump = 2;
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
   }
   else
       R4000.pc += 4;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int blezl()
{
   if((int)R4000.reg[RS(R4000.opcode)] <= 0){
       R4000.jump = 2;
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
   }
   else
       R4000.pc += 4;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bgtz()
{
   if((int)R4000.reg[RS(R4000.opcode)] > 0){
       R4000.jump = 2;
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
   }
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int mfc()
{
   float a;

   a = R4000.sp_reg[FS(R4000.opcode)];//1c7c
   R4000.reg[RT(R4000.opcode)] = *((unsigned long *)&a);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int mtc()
{
   unsigned long a;

   a = R4000.reg[RT(R4000.opcode)];
   R4000.sp_reg[FS(R4000.opcode)] = *((float *)&a);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int cvtsw()
{
   R4000.sp_reg[FD(R4000.opcode)] = *((int *)&R4000.sp_reg[FS(R4000.opcode)]);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int trunc()
{
   int c;

   c = (int)R4000.sp_reg[FS(R4000.opcode)];
   R4000.sp_reg[FD(R4000.opcode)] = *((float *)&c);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int sqrt()
{
   R4000.sp_reg[FD(R4000.opcode)] = sqrt(R4000.sp_reg[FS(R4000.opcode)]);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int ceqs()
{
   R4000.cc = R4000.sp_reg[FS(R4000.opcode)] == R4000.sp_reg[FT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int clts()
{
   R4000.cc = R4000.sp_reg[FS(R4000.opcode)] < R4000.sp_reg[FT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int cles()
{
   R4000.cc = R4000.sp_reg[FS(R4000.opcode)] <= R4000.sp_reg[FT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bc1t()
{
   if(R4000.cc){
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
       R4000.jump = 2;
   }
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bc1tl()
{
   if(R4000.cc){
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
       R4000.jump = 2;
   }
   else
       R4000.pc += 4;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bc1f()
{
   if(!R4000.cc){
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
       R4000.jump = 2;
   }
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int bc1fl()
{
   if(!R4000.cc){
       R4000.next_pc = R4000.pc + (IMM(R4000.opcode) << 2);
       R4000.jump = 2;
   }
   else
       R4000.pc += 4;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int subs()
{
    R4000.sp_reg[FD(R4000.opcode)] = R4000.sp_reg[FS(R4000.opcode)] - R4000.sp_reg[FT(R4000.opcode)];
    return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int muls()
{
    R4000.sp_reg[FD(R4000.opcode)] = R4000.sp_reg[FS(R4000.opcode)] * R4000.sp_reg[FT(R4000.opcode)];
    return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int divs()
{
   float f;

   f = R4000.sp_reg[FT(R4000.opcode)];
   if(f != 0)
       R4000.sp_reg[FD(R4000.opcode)] = R4000.sp_reg[FS(R4000.opcode)] / f;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int adds()
{
   R4000.sp_reg[FD(R4000.opcode)] = R4000.sp_reg[FS(R4000.opcode)] + R4000.sp_reg[FT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int movs()
{
   R4000.sp_reg[FD(R4000.opcode)] = R4000.sp_reg[FS(R4000.opcode)];//1c7c
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int ext()
{
   R4000.reg[RT(R4000.opcode)] = (R4000.reg[RS(R4000.opcode)] >> POS(R4000.opcode)) &
       MASK((SIZE(R4000.opcode) + 1));
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int ins()
{
   unsigned long mask;
   unsigned char pos;

   pos = POS(R4000.opcode);
   mask = MASK((SIZE(R4000.opcode) - pos + 1));
   R4000.reg[RT(R4000.opcode)] = (R4000.reg[RT(R4000.opcode)] & ~(mask << pos)) |
       ((R4000.reg[RS(R4000.opcode)] & mask) << pos);
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int mfic()
{
   R4000.reg[RT(R4000.opcode)] = R4000.ic;
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int mtic()
{
   R4000.ic = R4000.reg[RT(R4000.opcode)];
   return INS_CYCLES;
}
//---------------------------------------------------------------------------
static int syscall()
{
   int value;

   value = (R4000.opcode & 0x03FFFFFC) >> 6;
//   if(value == 30001)
//       psp.EnterWaitMode();
   return psp.sys_call(value) + 4;
}
//---------------------------------------------------------------------------
int exec_cpu()
{
   int res;

   R4000.opcode = read_dword(R4000.pc);
   res = 0;
exec_cpu_0:
   switch((R4000.opcode >> 24) & 0xFC){
       case 0x00:
           switch(R4000.opcode & 0x3F){
               case 0x00000000:
                   res += sll_imm();
               break;
               case 0x00000002:
                   res += srl_imm();
               break;
               case 0x00000003:
                   res += sra_imm();
               break;
               case 0x00000004:
                   res += sll();
               break;
               case 0x00000006:
                   res += srl();
               break;
               case 0x00000007:
                   res += sra();
               break;
               case 0x00000008:
                   res += jr();
               break;
               case 0x00000009:
                   res += jalr();
               break;
               case 0x0000000C:
                   res += syscall();
               break;
               case 0x0000000A:
                   res += movz();
               break;
               case 0x0000000B:
                   res += movn();
               break;
               case 0x00000010:
                   res += mfhi();
               break;
               case 0x00000012:
                   res += mflo();
               break;
               case 0x00000013:
                   res += mtlo();
               break;
               case 0x00000019:
                   res += multu();
               break;
               case 0x00000018:
                   res += mult();
               break;
               case 0x0000001A:
                   res += div();
               break;
               case 0x0000001B:
                   res += divu();
               break;
               case 0x00000016:
                   res += clz();
               break;
               case 0x0000001C:
                   res += madd();
               break;
               case 0x00000020:
               case 0x00000021:
                   res += add();
               break;
               case 0x00000023:
                   res += subu();
               break;
               case 0x00000024:
                   res += _and();
               break;
               case 0x00000025:
                   res += _or();
               break;
               case 0x00000026:
                   res += _xor();
               break;
               case 0x00000027:
                   res += nor();
               break;
               case 0x0000002A:
                   res += slt();
               break;
               case 0x0000002B:
                   res += sltu();
               break;
               case 0x0000002C:
                   res += _max();
               break;
               case 0x0000002D:
                   res += _min();
               break;
               default:
                   res += unknow_ins();
               break;
           }
       break;
       case 0x4:
           switch((R4000.opcode & 0x001F0000) >> 16){
               case 0x0:
                   res += bltz();
               break;
               case 0x1:
                   res += bgez();
               break;
               case 0x2:
                   res += bltzl();
               break;
               case 0x3:
                   res += bgezl();
               break;
               default:
                   res += unknow_ins();
               break;
           }
       break;
       case 0x8:
           res += j();
       break;
       case 0xC:
           res += jal();
       break;
       case 0x10:
           res += beq();
       break;
       case 0x14:
           res += bne();
       break;
       case 0x18:
           res += blez();
       break;
       case 0x1C:
           res += bgtz();
       break;
       case 0x20:
       case 0x24:
           res += addui();
       break;
       case 0x28:
           res += slti();
       break;
       case 0x2C:
           res += sltiu();
       break;
       case 0x30:
           res += andi();
       break;
       case 0x34:
           res += ori();
       break;
       case 0x38:
           res += xori();
       break;
       case 0x3C:
           res += lui();
       break;
       case 0x44:
           switch(R4000.opcode >> 24){
               case 0x45:
                   switch((R4000.opcode >> 16) & 0xFF){
                       case 0x0:
                           res += bc1f();
                       break;
                       case 0x1:
                           res += bc1t();
                       break;
                       case 0x2:
                           res += bc1fl();
                       break;
                       case 0x3:
                           res += bc1tl();
                       break;
                       default:
                           res += unknow_ins();
                       break;
                   }
               break;
               case 0x46:
                   switch(R4000.opcode & 0x3F){
                       case 0x0:
                           res += adds();
                       break;
                       case 0x1:
                           res += subs();
                       break;
                       case 0x2:
                           res += muls();
                       break;
                       case 0x3:
                           res += divs();
                       break;
                       case 0x4:
                           res += sqrt();
                       break;
                       case 0x5:
                           R4000.sp_reg[FD(R4000.opcode)] = fabs(R4000.sp_reg[FS(R4000.opcode)]);
                           res += 4;
                       break;
                       case 0x6:
                           res += movs();
                       break;
                       case 0x7:
                           R4000.sp_reg[FD(R4000.opcode)] = -R4000.sp_reg[FS(R4000.opcode)];
                           res += 4;
                       break;
                       case 0xD:
                           res += trunc();
                       break;
                       case 0x20: // CVT.S.W
                           res += cvtsw();
                       break;
                       case 0x32:
                           res += ceqs();
                       break;
                       case 0x3C:
                           res += clts();
                       break;
                       case 0x3E:
                           res += cles();
                       break;
                       default:
                           res += unknow_ins();
                       break;
                   }
               break;
               case 0x44:
                   switch((R4000.opcode & 0x00E00000) >> 20){
                       case 0:
                           res += mfc();
                       break;
                       case 8:
                           res += mtc();
                       break;
                       default:
                           res += unknow_ins();
                       break;
                   }
               break;
           }
       break;
       case 0x48:
           switch((R4000.opcode >> 20) & 0x3F){
               case 0x6:
                   res += ins_mfv();
               break;
               case 0xE:
                   res += ins_mtv();
               break;
           }
       break;
       case 0x50:
           res += beql();
       break;
       case 0x54:
           res += bnel();
       break;
       case 0x58:
           res += blezl();
       break;
       case 0x5C:
           res += bgtzl();
       break;
       case 0x60:
           switch((R4000.opcode >> 20) & 0x38){
               case 0x0:
                   res += ins_vadd();
               break;
               case 0x8:
                   res += ins_vsub();
               break;
               case 0x38:
                   res += ins_vdiv();
               break;
           }
       break;                  
       case 0x64:
           switch((R4000.opcode >> 20) & 0x38){
               case 0:
                   res += ins_vmul();
               break;
               case 0x8:
                   res += ins_vdot();
               break;
               case 0x20:
                   res += ins_vhdp();
               break;
           }
       break;
       case 0x6C:
           switch((R4000.opcode >> 20) & 0x38){
               case 0x0:
                   res += ins_vcmp();
               break;
               case 0x18:
                   res += ins_vmax();
               break;
               case 0x10:
                   res += ins_vmin();
               break;
           }
       break;
       case 0x70:
           switch(((R4000.opcode >> 12) & 0xE00) | (R4000.opcode & 0x3f)){
               case 0x24:
                   res += mfic();
               break;
               case 0x26:
                   res += mtic();
               break;
               default:
                   res += unknow_ins();
               break;
           }
       break;
       case 0x7C:
           switch(R4000.opcode & 0x3F){
               case 0x20:
                   switch(R4000.opcode & 0x7FF){
                       case 0x420:
                           R4000.reg[RD(R4000.opcode)] = (unsigned long)(int)(signed char)R4000.reg[RT(R4000.opcode)];
                           res += 4;
                       break;
                       case 0x620:
                           R4000.reg[RD(R4000.opcode)] = (unsigned long)(int)(signed short)R4000.reg[RT(R4000.opcode)];
                           res += 4;
                       break;
                       default:
                           res += unknow_ins();
                       break;
                   }
               break;
               case 0x0:
                   res += ext();
               break;
               case 0x4:
                   res += ins();
               break;
               default:
                   res += unknow_ins();
               break;
           }
       break;
       case 0x80:
           res += lb();
       break;
       case 0x84:
           res += lh();
       break;
       case 0x88:
           res += lwl();                    //8998174
       break;
       case 0x8C:
           res += lw();
       break;
       case 0x90:
           res += lbu();
       break;
       case 0x94:
           res += lhu();
       break;
       case 0x98:
           res += lwr();
       break;
       case 0xA0:
           res += sb();
       break;
       case 0xA4:
           res += sh();
       break;
       case 0xA8:
           res += swl();
       break;
       case 0xAC:
           res += sw();
       break;
       case 0xB8:
           res += swr();
       break;
       case 0xC4:
           res += lwc();
       break;
       case 0xD0:
           switch((R4000.opcode >> 20) & 0x3F){
               case 0:
                   switch((R4000.opcode >> 16) & 0xF){
                       case 0x0:
                       	res += ins_vmov();
                       break;
                       case 0x1:
                           res += ins_vabs();
                       break;
                       case 0x2:
                           res += ins_vneg();
                       break;
                       case 0x3:
                           res += ins_vidt();
                       break;
                       case 0x4:
                       	res += ins_vsat0();
                       break;
                       case 0x5:
                       	res += ins_vsat1();
                       break;
                       case 0x6:
                           res += ins_vzero();
                       break;
                       case 0x7://vone
                           res += ins_vone();
                       break;
                   }
               break;
               case 0x1:
                   switch((R4000.opcode >> 16) & 0xF){
                       case 0x0:
                           res += ins_vrcp();
                       break;
                       case 0x1:
                           res += ins_vrsq();
                       break;
                       case 0x2:
                           res += ins_vsin();
                       break;
                       case 0x3:
                           res += ins_vcos();
                       break;
                       case 0x4:
                           res += ins_vexp2();
                       break;
                       case 0x5:
                           res += ins_vlog2();
                       break;
                       case 0x6:
                           res += ins_vsqrt();
                       break;
                       case 0x7:
                           res += ins_vasin();
                       break;
                       case 0x8:
                           res += ins_vnrcp();
                       break;
                       case 0xA:
                           res += ins_vnsin();
                       break;
                       case 0xC:
                           res += ins_vrexp2();
                       break;
                   }
               break;
               case 0x4:
                   res += ins_vsgn();
               break;
               case 0x6:
                   res += ins_vcst();
               break;
           }
       break;
       case 0xD8:
       	res += ins_lv();
       break;
       case 0xE4:
           res += swc();
       break;
       case 0xF0:
           switch((R4000.opcode >> 20) & 0x38){
               case 0x0:
               	res += ins_vmmul();
               break;
               case 0x38:
                   switch((R4000.opcode >> 16) & 0xf){
                       case 0x3:
                           res += ins_vmidt();
                       break;
                       case 0x6:
                           res += ins_vmzero();
                       break;
                       case 0x7:
                           res += ins_vmone();
                       break;
                   }
               break;
           }
       break;
       case 0xF1:
       	switch((R4000.opcode >> 20) & 0xf){
           	case 0x0:
               break;
               case 0x8:
               	res += ins_vtfm4();
               break;
           }
       break;
       case 0xF2:
       	res += ins_vmscl();
       break;
       case 0xF8:
           res += ins_sv();
       break;
       case 0xFC:
           res += psp.ExecuteFuncs(R4000.opcode & 0x03FFFFFF)*4;
           R4000.opcode = 0x03E00008;       //8a2d788 open(config)
           goto exec_cpu_0;                 //08a2d6d8 ( sceGeEdramGetSize)
       break;                               //8a01bb0 (vblankstart)
       default:                             //8a2d7a0 iolseek
           res += unknow_ins();
       break;
   }
   if(R4000.jump == 1){
       R4000.pc = R4000.next_pc;
       R4000.jump = 0;
   }
   else if(R4000.jump > 1)
       R4000.jump--;
   R4000.pc += 4;
ex_exec_cpu:
   current_thread->time_slice += res;
   ticks += res;
   if(current_thread->time_slice > current_thread->max_time_slice){
       current_thread->Save(&R4000);
       current_thread = psp.switch_thread();
       current_thread->time_slice = 0;
       current_thread->Restore(&R4000);
       res += 333;
   }
   return res;
}
//---------------------------------------------------------------------------
LCPU::LCPU() : LOS()
{
}
//---------------------------------------------------------------------------
LCPU::~LCPU()
{
}
//---------------------------------------------------------------------------
BOOL LCPU::Init()
{
   Reset();
   ticks = 1;
   return TRUE;
}
//---------------------------------------------------------------------------
void LCPU::Reset()
{
   ZeroMemory(&R4000,sizeof(CPU_BASE));
   R4000.reg[29] = 0x09F70000;
   R4000.reg[31] = 0x08000000;
   GetSystemTimeAsFileTime((FILETIME *)&start_time);
}
//---------------------------------------------------------------------------
void LCPU::set_EntryAddress(DWORD value,DWORD gp)
{
   psp_thread *p;

   if((p = new psp_thread("sam_empty",0)) == NULL)
       return;
   threads_list.Add((LPVOID)p); 
   if((p = new psp_thread("sam_main",value)) == NULL)
       return;
   threads_list.Add((LPVOID)p);
   threads_list.active_thread(1);
   current_thread = p;
   p->status = 1;
   p->priority = 0;
   R4000.pc = value;
   p->base.reg[28] = R4000.reg[28] = gp;
}
//---------------------------------------------------------------------------
double LCPU::get_Register(Registers value)
{
   switch(value){
       case PC:
           return (double)R4000.pc;
       case LO:
           return (double)R4000.lo;
       case HI:
           return (double)R4000.hi;
       default:
           if((int)value < 32)
               return (double)R4000.reg[(int)value];
           else if((int)value < 67)
               return (double)R4000.sp_reg[(int)value - 32];
           else
               return (double)R4000.vfpu_reg[(int)value - 67];
   }
}

