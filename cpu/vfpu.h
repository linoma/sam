#include "r4000.h"

//---------------------------------------------------------------------------
#ifndef __vfpuH__
#define __vfpuH__
//---------------------------------------------------------------------------
int ins_mtv();
int ins_mfv();
int ins_sv();
int ins_lv();

int ins_vmov();
int ins_vadd();
int ins_vsub();
int ins_vmul();
int ins_vdiv();
int ins_vsat0();
int ins_vsat1();

int ins_vzero();
int ins_vone();
int ins_vidt();
int ins_vabs();
int ins_vneg();
int ins_vcst();
int ins_vmax();
int ins_vmin();
int ins_vcmp();

int ins_vdot();
int ins_vhdp();
int ins_vrsq();
int ins_vrcp();
int ins_vnrcp();
int ins_vsgn();
int ins_vsqrt();
int ins_vexp2();
int ins_vlog2();
int ins_vrexp2();

int ins_vsin();
int ins_vcos();
int ins_vasin();
int ins_vnsin();

int ins_vmzero();
int ins_vmone();
int ins_vmidt();
int ins_vmmul();
int ins_vmscl();
int ins_vtfm4();
#endif
