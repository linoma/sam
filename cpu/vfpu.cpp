#include "vfpu.h"
#include <math.h>

#define VR(a) (((a & 0x1C) << 2) + ((a & 0x60) >> 3) + (a & 3))

extern CPU_BASE R4000;
//---------------------------------------------------------------------------
static void get_rd(int width,float **rd)
{
   int i,i1;

   i = VD(R4000.opcode);
   if(width == 0){
       i1 = ((i & 0x1C) << 2) + (i & 3) + ((i & 0x60) >> 3);
       *rd = &R4000.vfpu_reg[i1];
   }
   else{
       i1 = ((i & 0x1C) << 2) + ((i & 3) << ((i & 0x20) >> 4));
       if(i & 0x40)
           i1 += 1 << width;
       *rd = &R4000.vfpu_reg[i1];
   }
}
//---------------------------------------------------------------------------
static int get_rs(int width,float **rs)
{
   int i,i1,brow;

   i = VS(R4000.opcode);
   if(width == 0){
       brow = 1;
       i1 = ((i & 0x1C) << 2) + (i & 3) + ((i & 0x60) >> 3);
       *rs = &R4000.vfpu_reg[i1];
   }
   else{
       brow = (i & 0x20) >> 5;
       i1 = ((i & 0x1C) << 2) + ((i & 3) << (brow << 1));
       if(i & 0x40)
           i1 += 1 << width;
       *rs = &R4000.vfpu_reg[i1];
       brow ^= 1;
       brow = brow + brow + brow + 1;
   }
   return brow;
}
//---------------------------------------------------------------------------
static void get_rt(int width,float **rt)
{
   int i,i1;

   i = VT(R4000.opcode);
   if(width == 0){
       i1 = ((i & 0x1C) << 2) + (i & 3) + ((i & 0x60) >> 3);
       *rt = &R4000.vfpu_reg[i1];
   }
   else{
       i1 = ((i & 0x1C) << 2) + ((i & 3) << ((i & 0x20) >> 4));
       if(i & 0x40)
           i1 += 1 << width;
       *rt = &R4000.vfpu_reg[i1];
   }
}
//---------------------------------------------------------------------------
int ins_lv()
{
   int i,rt,rc,value;
   float *f;

   rc = (R4000.opcode & 1);
   rt =((R4000.opcode & 0x1F0000) >> 16);
   f = &R4000.vfpu_reg[((rt & ~3) << 2) + ((rt & 3) << (rc << 1))];
   rc ^= 1;
   rc = (rc << 1) + rc + 1;
   rt = R4000.reg[RS(R4000.opcode)] + (R4000.opcode & 0xFFFC);
   for(i=0;i<4;i++){
       value = read_dword(rt);
       *f = *((float *)&value);
       rt += 4;
       f += rc;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_sv()
{
   int i,rt,rc;
   float *f;

   rc = (R4000.opcode & 1);
   rt =((R4000.opcode & 0x1F0000) >> 16);
   f = &R4000.vfpu_reg[((rt & ~3) << 2) + ((rt & 3) << (rc << 1))];
   rc ^= 1;
   rc = (rc << 1) + rc + 1;
   rt = R4000.reg[RS(R4000.opcode)] + (R4000.opcode & 0xFFFC);
   for(i=0;i<4;i++){
       write_dword(rt,*((int *)f));
       rt += 4;
       f += rc;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_mtv()
{
   float *rd;

   if(R4000.opcode & 0x80)
       R4000.vfpu_cr[R4000.opcode & 0xF] = R4000.reg[RT(R4000.opcode)];   
   else{
       get_rd(0,&rd);
       *rd = *((float *)&R4000.reg[RT(R4000.opcode)]);
   }
   return 12;  //8900c2c
}
//---------------------------------------------------------------------------
int ins_mfv()
{
   float *rd;

   if(R4000.opcode & 0x80)
       R4000.reg[RT(R4000.opcode)] = R4000.vfpu_cr[R4000.opcode & 0xF];
   else{
       get_rd(0,&rd);
       R4000.reg[RT(R4000.opcode)] = *((unsigned long *)rd);
   }
   return 12;  //8900c2c
}
//---------------------------------------------------------------------------
int ins_vdot()
{
   int width,i,brow;
   float *rd,*rt,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(0,&rd);
   brow = get_rs(width,&rs);
   get_rt(width,&rt);
   *rd = 0;
   for(i=0;i<=width;i++){
       *rd += *rt * *rs;
       rd += brow;
       rt += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vhdp()
{
   int width,i,brow;
   float *rd,*rt,*rs;
   
   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(0,&rd);
   brow = get_rs(width,&rs);
   get_rt(width,&rt);
   *rd = 0;
   for(i=0;i<width;i++){
       *rd += *rt * *rs;
       rd += brow;
       rt += brow;
       rs += brow;
   }
   *rd += *rs;
   return 12;
}
//---------------------------------------------------------------------------
int ins_vexp2()
{
   int width,i,brow;
   float *rd,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = pow(2.0f,*rs);
       rs += brow;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vlog2()
{
   int width,i,brow;
   float *rd,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = log(*rs) / M_LN2;
       rs += brow;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vrexp2()
{
   int width,i,brow;
   float *rd,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       f = pow(2.0f,*rs);
       if(*((int *)&f) != 0)
           f = 1.0f / f;
       else
           f = 0;
       *rd = f;
       rs += brow;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vrsq()
{
   int width,i,brow;
   float *rd,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       f = sqrt(*rs);
       if(*((int *)&f) != 0)
           *rd = 1.0f / f;
       else
           *rd = 0;
       rs += brow;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vsqrt()
{
   int width,i,brow;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = sqrt(*rs);
       rs += brow;
       rd += brow;                                                       
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vrcp()
{
   int width,i,brow;
   float *rd,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       f = *rs;
       if(*((int *)&f) != 0)
           *rd = 1.0f / f;
       else
           *rd = 0;
       rs += brow;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vnrcp()
{
   int width,i,brow;
   float *rd,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       f = *rs;
       if(*((int *)&f) != 0)
           *rd = -1.0f / f;
       else
           *rd = 0;
       rs += brow;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmul()
{
   int width,i,brow;
   float *rd,*rt,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   get_rt(width,&rt);
   for(i=0;i<=width;i++){
       *rd = *rt * *rs;
       rd += brow;
       rt += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vcst()
{
   int width,i,brow;
   float *rd,value;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   if(width == 0)
       brow = 1;
   else{
       brow = ((R4000.opcode & 0x20) >> 5) ^ 1;
       brow = brow + brow + brow + 1;
   }
   switch((R4000.opcode >> 16) & 0x1F){
       case 1:
       break;
       case 2:
           value = M_SQRT2;
       break;
       case 3:
           value = 1.0f / M_SQRT2;
       break;
       case 4:
           value = M_2_SQRTPI;
       break;
       case 5:
           value = M_2_PI;
       break;
       case 6:
           value = M_1_PI;
       break;
       case 7:
           value = M_PI_4;
       break;
       case 8:
           value = M_PI_2;
       break;
       case 9:
           value = M_PI;
       break;
       case 10:
           value = M_E;
       break;
       case 11:
           value = M_LOG2E;
       break;
       case 12:
           value = M_LOG10E;
       break;
       case 13:
           value = M_LN2;
       break;
       case 14:
           value = M_LN10;
       break;
       case 15:
           value = 2.0f * M_PI;
       break;
       case 16:
           value = M_PI / 6.0f;
       break;
       case 17:
           value = M_LOG10E / M_LOG2E;
       break;
       case 18:
           value = M_LOG2E / M_LOG10E;
       break;
       case 19:
           value = sqrt(3.0) / 2.0;
       break;
       default:
           value = 0;
       break;
   }
   for(i=0;i<=width;i++){
       *rd = value;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmax()
{
   int width,i,brow;
   float *rd,*rt,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   get_rt(width,&rt);
   for(i=0;i<=width;i++){
       f = *rt;
       if(f < *rs)
           f = *rs;
       *rd = f;
       rd += brow;
       rt += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmin()
{
   int width,i,brow;
   float *rd,*rt,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   get_rt(width,&rt);
   for(i=0;i<=width;i++){
       f = *rt;
       if(f > *rs)
           f = *rs;
       *rd = f;
       rd += brow;
       rt += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vone()
{
   int width,i,brow;
   float *rd;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   if(width == 0)
       brow = 1;
   else{
       brow = ((R4000.opcode & 0x20) >> 5) ^ 1;
       brow = brow + brow + brow + 1;
   }
   for(i=0;i<=width;i++){
       *rd = 1;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vidt()
{
   int width,i,brow;
   float *rd;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = ((R4000.opcode & 0x20) >> 5) ^ 1;
   brow = brow + brow + brow + 1;
   for(i=0;i<width;i++){
       *rd = 0;
       rd += brow;
   }
   *rd = 1.0;
   return 12;
}
//---------------------------------------------------------------------------
int ins_vzero()
{
   int width,i,brow;
   float *rd;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   if(width == 0)
       brow = 1;
   else{
       brow = ((R4000.opcode & 0x20) >> 5) ^ 1;
       brow = brow + brow + brow + 1;
   }
   for(i=0;i<=width;i++){
       *rd = 0;
       rd += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vabs()
{
   int width,i,brow;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = fabs(*rs);
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vneg()
{
   int width,i,brow;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = -*rs;
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vcmp()
{
   int i,width,brow;
   unsigned long cc;
   float *rs,*rt;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   brow = get_rs(width,&rs);
   cc = 0xF;
   switch(R4000.opcode & 0xF){
       case 0x0://VFPU_FL
       break;
       case 0x1://VFPU_EQ
           get_rt(width,&rt);
           for(i=0;i<=width;i++){
           	if(*rt != *rs)
					cc &= ~(1 << i);
           	rs += brow;
               rt += brow;
           }
       break;
       case 0x2://VFPU_LT
           get_rt(width,&rt);
           for(i=0;i<=width;i++){
           	if(*rt >= *rs)
					cc &= ~(1 << i);
           	rs += brow;
               rt += brow;
           }
       break;
       case 0x3://VFPU_LE
           get_rt(width,&rt);
           for(i=0;i<=width;i++){
           	if(*rt > *rs)
					cc &= ~(1 << i);
           	rs += brow;
               rt += brow;
           }
       break;
       case 0x4://VFPU_TR
       break;
       case 0x5://VFPU_NE
           get_rt(width,&rt);
           for(i=0;i<=width;i++){
           	if(*rt == *rs)
					cc &= ~(1 << i);
           	rs += brow;
               rt += brow;
           }
       break;
       case 0x6://VFPU_GE
           get_rt(width,&rt);
           for(i=0;i<=width;i++){
           	if(*rt < *rs)
					cc &= ~(1 << i);
           	rs += brow;
               rt += brow;
           }
       break;
       case 0x7://VFPU_GT
           get_rt(width,&rt);
           for(i=0;i<=width;i++){
           	if(*rt <= *rs)
					cc &= ~(1 << i);
           	rs += brow;
               rt += brow;
           }
       break;
       case 0x8://VFPU_EZ
           for(i=0;i<=width;i++){
               if((*((int *)rs) & 0x7FFFFFFF) != 0)
                   cc &= ~(1 << i);
               rs += brow;
           }
       break;
       case 0x9://VFPU_EN
       break;
       case 0xA://VFPU_EI
       break;
       case 0xB://VFPU_ES
       break;
       case 0xC://VFPU_NZ
           for(i=0;i<=width;i++){
               if((*((int *)rs) & 0x7FFFFFFF) == 0)
                   cc &= ~(1 << i);
               rs += brow;
           }
       break;
       case 0xD://VFPU_NN
       break;
       case 0xE://VFPU_NI
       break;
       case 0xF://VFPU_NS
       break;
   }
   if(cc != 0){
       if(cc == 0xF)
           cc |= 0x20;
       cc |= 0x10;
   }
   R4000.vfpu_cr[4] = cc;
   return 12;
}
//---------------------------------------------------------------------------
int ins_vsgn()
{
   int width,i,brow;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       if(*((int *)rs) & 0x80000000)
           *rd = -1.0f;
       else
           *rd = 1.0f;
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vsat0()
{
   int width,i,brow;
   float *rd,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
		f = *rs;
       if(f > 1.0f)
       	f = 1.0f;
       if(f <= 0.0)
       	f = 0.0f;
       *rd = f;
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vsat1()
{
   int width,i,brow;
   float *rd,*rs,f;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
		f = *rs;
       if(f > 1.0f)
       	f = 1.0f;
       if(f <= -1.0)
       	f = -1.0f;
       *rd = f;
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmov()
{
   int width,i,brow;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = *rs;
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vadd()
{
   int width,i,brow;
   float *rd,*rt,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   get_rt(width,&rt);
   for(i=0;i<=width;i++){
       *rd = *rt + *rs;
       rd += brow;
       rt += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vsub()
{
   int width,i,brow;
   float *rd,*rt,*rs;
                                       
   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);   
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   get_rt(width,&rt);
   for(i=0;i<=width;i++){
       *rd = *rs - *rt;
       rd += brow;
       rt += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vdiv()
{
   int width,i,brow;
   float *rd,*rt,*rs;
   
   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   get_rt(width,&rt);
   for(i=0;i<=width;i++){
       *rd = *rs / *rt;
       rd += brow;
       rt += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vsin()
{
   int width,i,brow;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = sin(*rs * M_PI_2);
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vcos()
{
   int width,i,brow;
   float *rd,*rs;
   
   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = cos(*rs * M_PI_2);
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vasin()
{
   int width,i,brow;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = asin(*rs) * M_2_PI;
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vnsin()
{
   int width,i,brow;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   get_rd(width,&rd);
   brow = get_rs(width,&rs);
   for(i=0;i<=width;i++){
       *rd = -sin(*rs);
       rd += brow;
       rs += brow;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmzero()
{
   int width,i,i1;
   float *rd,*p;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   rd = &R4000.vfpu_reg[(R4000.opcode & 0x1C) << 2];
   for(i=0;i<=width;i++){
       p = rd;
       for(i1=0;i1<=width;i1++)
           *p++ = 0;
       rd += 4;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmone()
{
   int width,i,i1;
   float *rd,*p;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   rd = &R4000.vfpu_reg[(R4000.opcode & 0x1C) << 2];
   for(i=0;i<=width;i++){
       p = rd;
       for(i1=0;i1<=width;i1++)
           *p++ = 1.0f;
       rd += 4;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmidt()
{
   int width,i,i1;
   float *rd,*p;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   rd = &R4000.vfpu_reg[(R4000.opcode & 0x1C) << 2];
   for(i=0;i<=width;i++){
       p = rd;
       for(i1=0;i1<=width;i1++){
           if(i == i1)
               *p = 1.0f;
           else
               *p++ = 0.0f;
       }
       rd += 4;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmmul()
{
   int width,i,i1,i2;
   float *rd,*rs,*rt;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   rd = &R4000.vfpu_reg[(R4000.opcode & 0x1C) << 2];
   for(i=0;i<=width;i++){
		for(i1=0;i1<=width;i1++){
       	rd[i*4+i1] = 0.0f;
           for(i2=0;i2<=width;i2++)
				rd[i*4+i1] += rs[i*4+i2] * rt[i2*4+i1];
       }
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vmscl()
{
   int width,i,i1;
   float *rd,*rs;

   width = ((R4000.opcode >> 7) & 1) | ((R4000.opcode >> 14) & 2);
   rd = &R4000.vfpu_reg[(R4000.opcode & 0x1C) << 2];
   get_rs(width,&rs);
   for(i=0;i<=width;i++){
		for(i1=0;i1<=width;i1++)
       	rd[i*4+i1] *= *rs;
   }
   return 12;
}
//---------------------------------------------------------------------------
int ins_vtfm4()
{
   float *rd;
	int i;

	rd = &R4000.vfpu_reg[(R4000.opcode & 0x1C) << 2];

	return 12;
}


