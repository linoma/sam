#include "r4000.h"
#include "lmmu.h"
#include "syscall.h"
#include <stdio.h>

enum ADDR_TYPE {NUL = 0,T16 = 1,T26 = 2,REG = 3};
enum INSTR_TYPE {PSP  = 1,B    = 2,JUMP = 4,JAL  = 8};
static const char regName[][4] = {
   "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
   "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
   "f8", "f9", "f10","f11","f12","f13","f14","f15",
   "f16","f17","f18","f19","f20","f21","f22","f23",
   "f24","f25","f26","f27","f28","f29","f30","f31",
   "pc","lo","hi"
};

static const Instruction g_inst[] = {
		/* MIPS instructions */
		{ "add",		0x00000020, 0xFC0007FF, "%d, %s, %t",  NUL, 0 },
		{ "addi",		0x20000000, 0xFC000000, "%t, %s, %i",  NUL, 0 },
		{ "addiu",		0x24000000, 0xFC000000, "%t, %s, %i",  NUL, 0 },
		{ "addu",		0x00000021, 0xFC0007FF, "%d, %s, %t",  NUL, 0 },
		{ "and",		0x00000024, 0xFC0007FF,	"%d, %s, %t",  NUL, 0 },
		{ "andi",		0x30000000, 0xFC000000,	"%t, %s, %I",  NUL, 0 },
		{ "beq",		0x10000000, 0xFC000000,	"%s, %t, %O",  T16,  B },
		{ "beql",		0x50000000, 0xFC000000,	"%s, %t, %O",  T16,  B },
		{ "bgez",		0x04010000, 0xFC1F0000,	"%s, %O",  T16,  B },
		{ "bgezal",		0x04110000, 0xFC1F0000,	"%s, %0",  T16,  JAL },
		{ "bgezl",		0x04030000, 0xFC1F0000,	"%s, %O",  T16,  B },
		{ "bgtz",		0x1C000000, 0xFC1F0000,	"%s, %O",  T16,  B },
		{ "bgtzl",		0x5C000000, 0xFC1F0000,	"%s, %O",  T16,  B },
		{ "bitrev",		0x7C000520, 0xFFE007FF, "%d, %t",  NUL,  PSP },
		{ "blez",		0x18000000, 0xFC1F0000,	"%s, %O",  T16,  B },
		{ "blezl",		0x58000000, 0xFC1F0000,	"%s, %O",  T16,  B },
		{ "bltz",		0x04000000, 0xFC1F0000,	"%s, %O",  T16,  B },
		{ "bltzl",		0x04020000, 0xFC1F0000,	"%s, %O",  T16,  B },
		{ "bltzal",		0x04100000, 0xFC1F0000,	"%s, %O",  T16,  JAL },
		{ "bltzall",	0x04120000, 0xFC1F0000,	"%s, %O",  T16,  JAL },
		{ "bne",		0x14000000, 0xFC000000,	"%s, %t, %O",  T16,  B },
		{ "bnel",		0x54000000, 0xFC000000,	"%s, %t, %O",  T16,  B },
		{ "break",		0x0000000D, 0xFC00003F,	"%c",  NUL, 0 },
		{ "cache",		0xbc000000, 0xfc000000, "%k, %o",  NUL, 0 },
		{ "cfc0",		0x40400000, 0xFFE007FF,	"%t, %p",  NUL,  PSP },
		{ "clo",		0x00000017, 0xFC1F07FF, "%d, %s",  NUL,  PSP },
		{ "clz",		0x00000016, 0xFC1F07FF, "%d, %s",  NUL,  PSP },
		{ "ctc0",		0x40C00000, 0xFFE007FF,	"%t, %p",  NUL,  PSP },
		{ "max",		0x0000002C, 0xFC0007FF, "%d, %s, %t",  NUL,  PSP },
		{ "min",		0x0000002D, 0xFC0007FF, "%d, %s, %t",  NUL,  PSP },
		{ "dbreak",		0x7000003F, 0xFFFFFFFF,	"",  NUL,  PSP },
		{ "div",		0x0000001A, 0xFC00FFFF, "%s, %t",  NUL, 0 },
		{ "divu",		0x0000001B, 0xFC00FFFF, "%s, %t",  NUL, 0 },
		{ "dret",		0x7000003E, 0xFFFFFFFF,	"",  NUL,  PSP },
		{ "eret",		0x42000018, 0xFFFFFFFF, "",  NUL, 0 },
		{ "ext",		0x7C000000, 0xFC00003F, "%t, %s, %a, %ne",  NUL,  PSP },
		{ "ins",		0x7C000004, 0xFC00003F, "%t, %s, %a, %ni",  NUL,  PSP },
		{ "j",			0x08000000, 0xFC000000,	"%j",  T26,  JUMP },
		{ "jr",			0x00000008, 0xFC1FFFFF,	"%J",  REG,  JUMP },
		{ "jalr",		0x00000009, 0xFC1F07FF,	"%J, %d",  REG,  JAL },
		{ "jal",		0x0C000000, 0xFC000000,	"%j",  T26,  JAL },
		{ "lb",			0x80000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "lbu",		0x90000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "lh",			0x84000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "lhu",		0x94000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "ll",			0xC0000000, 0xFC000000,	"%t, %O",  NUL, 0 },
		{ "lui",		0x3C000000, 0xFFE00000,	"%t, %I",  NUL, 0 },
		{ "lw",			0x8C000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "lwl",		0x88000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "lwr",		0x98000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "madd",		0x0000001C, 0xFC00FFFF, "%s, %t",  NUL,  PSP },
		{ "maddu",		0x0000001D, 0xFC00FFFF, "%s, %t",  NUL,  PSP },
		{ "mfc0",		0x40000000, 0xFFE007FF,	"%t, %0",  NUL, 0 },
		{ "mfdr",		0x7000003D, 0xFFE007FF,	"%t, %r",  NUL,  PSP },
		{ "mfhi",		0x00000010, 0xFFFF07FF, "%d",  NUL, 0 },
		{ "mfic",		0x70000024, 0xFFE007FF, "%t, %p",  NUL,  PSP },
		{ "mflo",		0x00000012, 0xFFFF07FF, "%d",  NUL, 0 },
		{ "movn",		0x0000000B, 0xFC0007FF, "%d, %s, %t",  NUL,  PSP },
		{ "movz",		0x0000000A, 0xFC0007FF, "%d, %s, %t",  NUL,  PSP },
		{ "msub",		0x0000002e, 0xfc00ffff, "%d, %t",  NUL,  PSP },
		{ "msubu",		0x0000002f, 0xfc00ffff, "%d, %t",  NUL,  PSP },
		{ "mtc0",		0x40800000, 0xFFE007FF,	"%t, %0",  NUL, 0 },
		{ "mtdr",		0x7080003D, 0xFFE007FF,	"%t, %r",  NUL,  PSP },
		{ "mtic",		0x70000026, 0xFFE007FF, "%t, %p",  NUL,  PSP },
		{ "halt",       0x70000000, 0xFFFFFFFF, "" ,  NUL,  PSP },
		{ "mthi",		0x00000011, 0xFC1FFFFF,	"%s",  NUL, 0 },
		{ "mtlo",		0x00000013, 0xFC1FFFFF,	"%s",  NUL, 0 },
		{ "mult",		0x00000018, 0xFC00FFFF, "%s, %t",  NUL, 0 },
		{ "multu",		0x00000019, 0xFC0007FF, "%s, %t",  NUL, 0 },
		{ "nor",		0x00000027, 0xFC0007FF,	"%d, %s, %t",  NUL, 0 },
		{ "or",			0x00000025, 0xFC0007FF,	"%d, %s, %t",  NUL, 0 },
		{ "ori",		0x34000000, 0xFC000000,	"%t, %s, %I",  NUL, 0 },
		{ "rotr",		0x00200002, 0xFFE0003F, "%d, %t, %a",  NUL, 0 },
		{ "rotv",		0x00000046, 0xFC0007FF, "%d, %t, %s",  NUL, 0 },
		{ "seb",		0x7C000420, 0xFFE007FF,	"%d, %t",  NUL, 0 },
		{ "seh",		0x7C000620, 0xFFE007FF,	"%d, %t",  NUL, 0 },
		{ "sb",			0xA0000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "sh",			0xA4000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "sllv",		0x00000004, 0xFC0007FF,	"%d, %t, %s",  NUL, 0 },
		{ "sll",		0x00000000, 0xFFE0003F,	"%d, %t, %a",  NUL, 0 },
		{ "slt",		0x0000002A, 0xFC0007FF,	"%d, %s, %t",  NUL, 0 },
		{ "slti",		0x28000000, 0xFC000000,	"%t, %s, %i",  NUL, 0 },
		{ "sltiu",		0x2C000000, 0xFC000000,	"%t, %s, %i",  NUL, 0 },
		{ "sltu",		0x0000002B, 0xFC0007FF,	"%d, %s, %t",  NUL, 0 },
		{ "sra",		0x00000003, 0xFFE0003F,	"%d, %t, %a",  NUL, 0 },
		{ "srav",		0x00000007, 0xFC0007FF,	"%d, %t, %s",  NUL, 0 },
		{ "srlv",		0x00000006, 0xFC0007FF,	"%d, %t, %s",  NUL, 0 },
		{ "srl",		0x00000002, 0xFFE0003F,	"%d, %t, %a",  NUL, 0 },
		{ "sw",			0xAC000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "swl",		0xA8000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "swr",		0xB8000000, 0xFC000000,	"%t, %o",  NUL, 0 },
		{ "sub",		0x00000022, 0xFC0007FF,	"%d, %s, %t",  NUL, 0 },
		{ "subu",		0x00000023, 0xFC0007FF,	"%d, %s, %t",  NUL, 0 },
		{ "sync",		0x0000000F, 0xFFFFFFFF,	"",  NUL, 0 },
		{ "syscall",	0x0000000C, 0xFC00003F,	"%C",  NUL, 0 },
		{ "xor",		0x00000026, 0xFC0007FF,	"%d, %s, %t",  NUL, 0 },
		{ "xori",		0x38000000, 0xFC000000,	"%t, %s, %I",  NUL, 0 },
		{ "wsbh",		0x7C0000A0, 0xFFE007FF,	"%d, %t",  NUL,  PSP },
		{ "wsbw",		0x7C0000E0, 0xFFE007FF, "%d, %t",  NUL,  PSP }, 

		/* FPU instructions */
		{"abs.s",	0x46000005, 0xFFFF003F, "%D, %S",  NUL, 0 },
		{"add.s",	0x46000000, 0xFFE0003F,	"%D, %S, %T",  NUL, 0 },
		{"bc1f",	0x45000000, 0xFFFF0000,	"%O",  T16,  B },
		{"bc1fl",	0x45020000, 0xFFFF0000,	"%O",  T16,  B },
		{"bc1t",	0x45010000, 0xFFFF0000,	"%O",  T16,  B },
		{"bc1tl",	0x45030000, 0xFFFF0000,	"%O",  T16,  B },
		{"c.f.s",	0x46000030, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.un.s",	0x46000031, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.eq.s",	0x46000032, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.ueq.s",	0x46000033, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.olt.s",	0x46000034, 0xFFE007FF,	"%S, %T",  NUL, 0 },
		{"c.ult.s",	0x46000035, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.ole.s",	0x46000036, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.ule.s",	0x46000037, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.sf.s",	0x46000038, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.ngle.s",0x46000039, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.seq.s",	0x4600003A, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.ngl.s",	0x4600003B, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.lt.s",	0x4600003C, 0xFFE007FF,	"%S, %T",  NUL, 0 },
		{"c.nge.s",	0x4600003D, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"c.le.s",	0x4600003E, 0xFFE007FF,	"%S, %T",  NUL, 0 },
		{"c.ngt.s",	0x4600003F, 0xFFE007FF, "%S, %T",  NUL, 0 },
		{"ceil.w.s",0x4600000E, 0xFFFF003F, "%D, %S",  NUL, 0 },
		{"cfc1",	0x44400000, 0xFFE007FF, "%t, %p",  NUL, 0 },
		{"ctc1",	0x44c00000, 0xFFE007FF, "%t, %p",  NUL, 0 },
		{"cvt.s.w",	0x46800020, 0xFFFF003F, "%D, %S",  NUL, 0 },
		{"cvt.w.s",	0x46000024, 0xFFFF003F, "%D, %S",  NUL, 0 },
		{"div.s",	0x46000003, 0xFFE0003F, "%D, %S, %T",  NUL, 0 },
		{"floor.w.s",0x4600000F, 0xFFFF003F,"%D, %S",  NUL, 0 },
		{"lwc1",	0xc4000000, 0xFC000000, "%T, %o",  NUL, 0 },
		{"mfc1",	0x44000000, 0xFFE007FF, "%t, %1",  NUL, 0 },
		{"mov.s",	0x46000006, 0xFFFF003F, "%D, %S",  NUL, 0 },
		{"mtc1",	0x44800000, 0xFFE007FF, "%t, %1",  NUL, 0 },
		{"mul.s",	0x46000002, 0xFFE0003F, "%D, %S, %T",  NUL, 0 },
		{"neg.s",	0x46000007, 0xFFFF003F, "%D, %S",  NUL, 0 },
		{"round.w.s",0x4600000C, 0xFFFF003F,"%D, %S",  NUL, 0 },
		{"sqrt.s",	0x46000004, 0xFFFF003F, "%D, %S",  NUL, 0 },
		{"sub.s",	0x46000001, 0xFFE0003F, "%D, %S, %T",  NUL, 0 },
		{"swc1",	0xe4000000, 0xFC000000, "%T, %o",  NUL, 0 },
		{"trunc.w.s",0x4600000D, 0xFFFF003F,"%D, %S",  NUL, 0 },

		/* VPU instructions */
		{ "bvf",	 0x49000000, 0xFFE30000, "%Zc, %O" ,  T16,  PSP |  B }, // [hlide] %Z -> %Zc
		{ "bvfl",	 0x49020000, 0xFFE30000, "%Zc, %O" ,  T16,  PSP |  B }, // [hlide] %Z -> %Zc
		{ "bvt",	 0x49010000, 0xFFE30000, "%Zc, %O" ,  T16,  PSP |  B }, // [hlide] %Z -> %Zc
		{ "bvtl",	 0x49030000, 0xFFE30000, "%Zc, %O" ,  T16,  PSP |  B }, // [hlide] %Z -> %Zc
		{ "lv.q",	 0xD8000000, 0xFC000002, "%Xq, %Y" ,  NUL,  PSP },
		{ "lv.s",	 0xC8000000, 0xFC000000, "%Xs, %Y" ,  NUL,  PSP },
		{ "lvl.q",	 0xD4000000, 0xFC000002, "%Xq, %Y" ,  NUL,  PSP },
		{ "lvr.q",	 0xD4000002, 0xFC000002, "%Xq, %Y" ,  NUL,  PSP },
		{ "mfv",	 0x48600000, 0xFFE0FF80, "%t, %zs" ,  NUL,  PSP }, // [hlide] added "%t, %zs"
		{ "mfvc",	 0x48600000, 0xFFE0FF00, "%t, %2d" ,  NUL,  PSP }, // [hlide] added "%t, %2d"
		{ "mtv",	 0x48E00000, 0xFFE0FF80, "%t, %zs" ,  NUL,  PSP }, // [hlide] added "%t, %zs"
		{ "mtvc",	 0x48E00000, 0xFFE0FF00, "%t, %2d" ,  NUL,  PSP }, // [hlide] added "%t, %2d"
		{ "sv.q",	 0xF8000000, 0xFC000002, "%Xq, %Y" ,  NUL,  PSP },
		{ "sv.s",	 0xE8000000, 0xFC000000, "%Xs, %Y" ,  NUL,  PSP },
		{ "svl.q",	 0xF4000000, 0xFC000002, "%Xq, %Y" ,  NUL,  PSP },
		{ "svr.q",	 0xF4000002, 0xFC000002, "%Xq, %Y" ,  NUL,  PSP },
		{ "vabs.p",	 0xD0010080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vabs.q",	 0xD0018080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vabs.s",	 0xD0010000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vabs.t",	 0xD0018000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vadd.p",	 0x60000080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vadd.q",	 0x60008080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vadd.s",	 0x60000000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP }, // [hlide] %yz -> %ys
		{ "vadd.t",	 0x60008000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vasin.p", 0xD0170080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vasin.q", 0xD0178080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vasin.s", 0xD0170000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vasin.t", 0xD0178000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vavg.p",	 0xD0470080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vavg.q",	 0xD0478080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vavg.t",	 0xD0478000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vbfy1.p",    0xD0420080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vbfy1.q",    0xD0428080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vbfy2.q",    0xD0438080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vcmovf.p",   0xD2A80080, 0xFFF88080, "%zp, %yp, %v3" ,  NUL,  PSP }, // [hlide] added "%zp, %yp, %v3"
		{ "vcmovf.q",   0xD2A88080, 0xFFF88080, "%zq, %yq, %v3" ,  NUL,  PSP }, // [hlide] added "%zq, %yq, %v3"
		{ "vcmovf.s",   0xD2A80000, 0xFFF88080, "%zs, %ys, %v3" ,  NUL,  PSP }, // [hlide] added "%zs, %ys, %v3"
		{ "vcmovf.t",   0xD2A88000, 0xFFF88080, "%zt, %yt, %v3" ,  NUL,  PSP }, // [hlide] added "%zt, %yt, %v3"
		{ "vcmovt.p",   0xD2A00080, 0xFFF88080, "%zp, %yp, %v3" ,  NUL,  PSP }, // [hlide] added "%zp, %yp, %v3"
		{ "vcmovt.q",   0xD2A08080, 0xFFF88080, "%zq, %yq, %v3" ,  NUL,  PSP }, // [hlide] added "%zq, %yq, %v3"
		{ "vcmovt.s",   0xD2A00000, 0xFFF88080, "%zs, %ys, %v3" ,  NUL,  PSP }, // [hlide] added "%zs, %ys, %v3"
		{ "vcmovt.t",   0xD2A08000, 0xFFF88080, "%zt, %yt, %v3" ,  NUL,  PSP }, // [hlide] added "%zt, %yt, %v3"
		{ "vcmp.p",	    0x6C000080, 0xFF8080F0, "%Zn, %yp, %xp" ,  NUL,  PSP }, // [hlide] added "%Zn, %zp, %xp"
		{ "vcmp.p",	    0x6C000080, 0xFFFF80F0, "%Zn, %yp" ,  NUL,  PSP }, // [hlide] added "%Zn, %xp"
		{ "vcmp.p",	    0x6C000080, 0xFFFFFFF0, "%Zn" ,  NUL,  PSP }, // [hlide] added "%Zn"
		{ "vcmp.q",	    0x6C008080, 0xFF8080F0, "%Zn, %yq, %xq" ,  NUL,  PSP }, // [hlide] added "%Zn, %yq, %xq"
		{ "vcmp.q",	    0x6C008080, 0xFFFF80F0, "%Zn, %yq" ,  NUL,  PSP }, // [hlide] added "%Zn, %yq"
		{ "vcmp.q",	    0x6C008080, 0xFFFFFFF0, "%Zn" ,  NUL,  PSP }, // [hlide] added "%Zn"
		{ "vcmp.s",	    0x6C000000, 0xFF8080F0, "%Zn, %ys, %xs" ,  NUL,  PSP }, // [hlide] added "%Zn, %ys, %xs"
		{ "vcmp.s",	    0x6C000000, 0xFFFF80F0, "%Zn, %ys" ,  NUL,  PSP }, // [hlide] added "%Zn, %ys"
		{ "vcmp.s",	    0x6C000000, 0xFFFFFFF0, "%Zn" ,  NUL,  PSP }, // [hlide] added "%Zn"
		{ "vcmp.t",	    0x6C008000, 0xFF8080F0, "%Zn, %yt, %xt" ,  NUL,  PSP }, // [hlide] added "%Zn, %yt, %xt"
		{ "vcmp.t",	    0x6C008000, 0xFFFF80F0, "%Zn, %yt" ,  NUL,  PSP }, // [hlide] added "%Zn, %yt"
		{ "vcmp.t",	    0x6C008000, 0xFFFFFFF0, "%Zn" ,  NUL,  PSP }, // [hlide] added "%zp"
		{ "vcos.p",	    0xD0130080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vcos.q",	    0xD0138080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vcos.s",	    0xD0130000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vcos.t",	    0xD0138000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vcrs.t",	    0x66808000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vcrsp.t",    0xF2808000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vcst.p",	    0xD0600080, 0xFFE0FF80, "%zp, %vk" ,  NUL,  PSP }, // [hlide] "%zp, %yp, %xp" -> "%zp, %vk"
		{ "vcst.q",	    0xD0608080, 0xFFE0FF80, "%zq, %vk" ,  NUL,  PSP }, // [hlide] "%zq, %yq, %xq" -> "%zq, %vk"
		{ "vcst.s",	    0xD0600000, 0xFFE0FF80, "%zs, %vk" ,  NUL,  PSP }, // [hlide] "%zs, %ys, %xs" -> "%zs, %vk"
		{ "vcst.t",	    0xD0608000, 0xFFE0FF80, "%zt, %vk" ,  NUL,  PSP }, // [hlide] "%zt, %yt, %xt" -> "%zt, %vk"
		{ "vdet.p",	    0x67000080, 0xFF808080, "%zs, %yp, %xp" ,  NUL,  PSP },
		{ "vdiv.p",	    0x63800080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vdiv.q",	    0x63808080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vdiv.s",	    0x63800000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP }, // [hlide] %yz -> %ys
		{ "vdiv.t",	    0x63808000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vdot.p",	    0x64800080, 0xFF808080, "%zs, %yp, %xp" ,  NUL,  PSP },
		{ "vdot.q",	    0x64808080, 0xFF808080, "%zs, %yq, %xq" ,  NUL,  PSP },
		{ "vdot.t",	    0x64808000, 0xFF808080, "%zs, %yt, %xt" ,  NUL,  PSP },
		{ "vexp2.p",    0xD0140080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vexp2.q",    0xD0148080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vexp2.s",    0xD0140000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vexp2.t",    0xD0148000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vf2h.p",	    0xD0320080, 0xFFFF8080, "%zs, %yp" ,  NUL,  PSP }, // [hlide] %zp -> %zs
		{ "vf2h.q",	    0xD0328080, 0xFFFF8080, "%zp, %yq" ,  NUL,  PSP }, // [hlide] %zq -> %zp
		{ "vf2id.p",    0xD2600080, 0xFFE08080, "%zp, %yp, %v5" ,  NUL,  PSP }, // [hlide] added "%zp, %yp, %v5"
		{ "vf2id.q",    0xD2608080, 0xFFE08080, "%zq, %yq, %v5" ,  NUL,  PSP }, // [hlide] added "%zq, %yq, %v5"
		{ "vf2id.s",    0xD2600000, 0xFFE08080, "%zs, %ys, %v5" ,  NUL,  PSP }, // [hlide] added "%zs, %ys, %v5"
		{ "vf2id.t", 0xD2608000, 0xFFE08080, "%zt, %yt, %v5" ,  NUL,  PSP }, // [hlide] added "%zt, %yt, %v5"
		{ "vf2in.p", 0xD2000080, 0xFFE08080, "%zp, %yp, %v5" ,  NUL,  PSP }, // [hlide] added "%zp, %yp, %v5"
		{ "vf2in.q", 0xD2008080, 0xFFE08080, "%zq, %yq, %v5" ,  NUL,  PSP }, // [hlide] added "%zq, %yq, %v5"
		{ "vf2in.s", 0xD2000000, 0xFFE08080, "%zs, %ys, %v5" ,  NUL,  PSP }, // [hlide] added "%zs, %ys, %v5"
		{ "vf2in.t", 0xD2008000, 0xFFE08080, "%zt, %yt, %v5" ,  NUL,  PSP }, // [hlide] added "%zt, %yt, %v5"
		{ "vf2iu.p", 0xD2400080, 0xFFE08080, "%zp, %yp, %v5" ,  NUL,  PSP }, // [hlide] added "%zp, %yp, %v5"
		{ "vf2iu.q", 0xD2408080, 0xFFE08080, "%zq, %yq, %v5" ,  NUL,  PSP }, // [hlide] added "%zq, %yq, %v5"
		{ "vf2iu.s", 0xD2400000, 0xFFE08080, "%zs, %ys, %v5" ,  NUL,  PSP }, // [hlide] added "%zs, %ys, %v5"
		{ "vf2iu.t", 0xD2408000, 0xFFE08080, "%zt, %yt, %v5" ,  NUL,  PSP }, // [hlide] added "%zt, %yt, %v5"
		{ "vf2iz.p", 0xD2200080, 0xFFE08080, "%zp, %yp, %v5" ,  NUL,  PSP }, // [hlide] added "%zp, %yp, %v5"
		{ "vf2iz.q", 0xD2208080, 0xFFE08080, "%zq, %yq, %v5" ,  NUL,  PSP }, // [hlide] added "%zq, %yq, %v5"
		{ "vf2iz.s", 0xD2200000, 0xFFE08080, "%zs, %ys, %v5" ,  NUL,  PSP }, // [hlide] added "%zs, %ys, %v5"
		{ "vf2iz.t", 0xD2208000, 0xFFE08080, "%zt, %yt, %v5" ,  NUL,  PSP }, // [hlide] added "%zt, %yt, %v5"
		{ "vfad.p",	 0xD0460080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vfad.q",	 0xD0468080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vfad.t",	 0xD0468000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vfim.s",	 0xDF800000, 0xFF800000, "%xs, %vh" ,  NUL,  PSP }, // [hlide] added "%xs, %vh"
		{ "vflush",	 0xFFFF040D, 0xFFFFFFFF, "" ,  NUL,  PSP },
		{ "vh2f.p",	 0xD0330080, 0xFFFF8080, "%zq, %yp" ,  NUL,  PSP }, // [hlide] %zp -> %zq
		{ "vh2f.s",	 0xD0330000, 0xFFFF8080, "%zp, %ys" ,  NUL,  PSP }, // [hlide] %zs -> %zp
		{ "vhdp.p",	 0x66000080, 0xFF808080, "%zs, %yp, %xp" ,  NUL,  PSP }, // [hlide] added "%zs, %yp, %xp"
		{ "vhdp.q",	 0x66008080, 0xFF808080, "%zs, %yq, %xq" ,  NUL,  PSP }, // [hlide] added "%zs, %yq, %xq"
		{ "vhdp.t",	 0x66008000, 0xFF808080, "%zs, %yt, %xt" ,  NUL,  PSP }, // [hlide] added "%zs, %yt, %xt"
		{ "vhtfm2.p", 0xF0800000, 0xFF808080, "%zp, %ym, %xp" ,  NUL,  PSP }, // [hlide] added "%zp, %ym, %xp"
		{ "vhtfm3.t",0xF1000080, 0xFF808080, "%zt, %yn, %xt" ,  NUL,  PSP }, // [hlide] added "%zt, %yn, %xt"
		{ "vhtfm4.q",0xF1808000, 0xFF808080, "%zq, %yo, %xq" ,  NUL,  PSP }, // [hlide] added "%zq, %yo, %xq"
		{ "vi2c.q",	 0xD03D8080, 0xFFFF8080, "%zs, %yq" ,  NUL,  PSP }, // [hlide] added "%zs, %yq"
		{ "vi2f.p",	 0xD2800080, 0xFFE08080, "%zp, %yp, %v5" ,  NUL,  PSP }, // [hlide] added "%zp, %yp, %v5"
		{ "vi2f.q",	 0xD2808080, 0xFFE08080, "%zq, %yq, %v5" ,  NUL,  PSP }, // [hlide] added "%zq, %yq, %v5"
		{ "vi2f.s",	 0xD2800000, 0xFFE08080, "%zs, %ys, %v5" ,  NUL,  PSP }, // [hlide] added "%zs, %ys, %v5"
		{ "vi2f.t",	 0xD2808000, 0xFFE08080, "%zt, %yt, %v5" ,  NUL,  PSP }, // [hlide] added "%zt, %yt, %v5"
		{ "vi2s.p",	 0xD03F0080, 0xFFFF8080, "%zs, %yp" ,  NUL,  PSP }, // [hlide] added "%zs, %yp"
		{ "vi2s.q",	 0xD03F8080, 0xFFFF8080, "%zp, %yq" ,  NUL,  PSP }, // [hlide] added "%zp, %yq"
		{ "vi2uc.q", 0xD03C8080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP }, // [hlide] %zp -> %zq
		{ "vi2us.p", 0xD03E0080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP }, // [hlide] %zp -> %zq
		{ "vi2us.q", 0xD03E8080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP }, // [hlide] %zp -> %zq
		{ "vidt.p",	 0xD0030080, 0xFFFFFF80, "%zp" ,  NUL,  PSP },
		{ "vidt.q",	 0xD0038080, 0xFFFFFF80, "%zq" ,  NUL,  PSP },
		{ "viim.s",	 0xDF000000, 0xFF800000, "%xs, %vi" ,  NUL,  PSP }, // [hlide] added "%xs, %vi"
		{ "vlgb.s",	 0xD0370000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vlog2.p", 0xD0150080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vlog2.q", 0xD0158080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vlog2.s", 0xD0150000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vlog2.t", 0xD0158000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vmax.p",	 0x6D800080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vmax.q",	 0x6D808080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vmax.s",	 0x6D800000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP },
		{ "vmax.t",	 0x6D808000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vmfvc",	 0xD0500000, 0xFFFF0080, "%zs, %2s" ,  NUL,  PSP }, // [hlide] added "%zs, %2s"
		{ "vmidt.p", 0xF3830080, 0xFFFFFF80, "%zm" ,  NUL,  PSP }, // [hlide] %zp -> %zm
		{ "vmidt.q", 0xF3838080, 0xFFFFFF80, "%zo" ,  NUL,  PSP }, // [hlide] %zq -> %zo
		{ "vmidt.t", 0xF3838000, 0xFFFFFF80, "%zn" ,  NUL,  PSP }, // [hlide] %zt -> %zn
		{ "vmin.p",	 0x6D000080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vmin.q",	 0x6D008080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vmin.s",	 0x6D000000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP },
		{ "vmin.t",	 0x6D008000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vmmov.p", 0xF3800080, 0xFFFF8080, "%zm, %ym" ,  NUL,  PSP }, // [hlide] added "%zm, %ym"
		{ "vmmov.q", 0xF3808080, 0xFFFF8080, "%zo, %yo" ,  NUL,  PSP },
		{ "vmmov.t", 0xF3808000, 0xFFFF8080, "%zn, %yn" ,  NUL,  PSP }, // [hlide] added "%zn, %yn"
		{ "vmmul.p", 0xF0000080, 0xFF808080, "%?%zm, %ym, %xm" ,  NUL,  PSP }, // [hlide] added "%?%zm, %ym, %xm"
		{ "vmmul.q", 0xF0008080, 0xFF808080, "%?%zo, %yo, %xo" ,  NUL,  PSP },
		{ "vmmul.t", 0xF0008000, 0xFF808080, "%?%zn, %yn, %xn" ,  NUL,  PSP }, // [hlide] added "%?%zn, %yn, %xn"
		{ "vmone.p", 0xF3870080, 0xFFFFFF80, "%zp" ,  NUL,  PSP },
		{ "vmone.q", 0xF3878080, 0xFFFFFF80, "%zq" ,  NUL,  PSP },
		{ "vmone.t", 0xF3878000, 0xFFFFFF80, "%zt" ,  NUL,  PSP },
		{ "vmov.p",	 0xD0000080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vmov.q",	 0xD0008080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vmov.s",	 0xD0000000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vmov.t",	 0xD0008000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vmscl.p", 0xF2000080, 0xFF808080, "%zm, %ym, %xs" ,  NUL,  PSP }, // [hlide] %zp, %yp, %xp -> %zm, %ym, %xs
		{ "vmscl.q", 0xF2008080, 0xFF808080, "%zo, %yo, %xs" ,  NUL,  PSP }, // [hlide] %zq, %yq, %xp -> %zo, %yo, %xs
		{ "vmscl.t", 0xF2008000, 0xFF808080, "%zn, %yn, %xs" ,  NUL,  PSP }, // [hlide] %zt, %yt, %xp -> %zn, %yn, %xs
		{ "vmtvc",	 0xD0510000, 0xFFFF8000, "%2d, %ys" ,  NUL,  PSP }, // [hlide] added "%2d, %ys"
		{ "vmul.p",	 0x64000080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vmul.q",	 0x64008080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vmul.s",	 0x64000000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP },
		{ "vmul.t",	 0x64008000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vmzero.p", 0xF3860080, 0xFFFFFF80, "%zm" ,  NUL,  PSP }, // [hlide] %zp -> %zm
		{ "vmzero.q",0xF3868080, 0xFFFFFF80, "%zo" ,  NUL,  PSP }, // [hlide] %zq -> %zo
		{ "vmzero.t",0xF3868000, 0xFFFFFF80, "%zn" ,  NUL,  PSP }, // [hlide] %zt -> %zn
		{ "vneg.p",	 0xD0020080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vneg.q",	 0xD0028080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vneg.s",	 0xD0020000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vneg.t",	 0xD0028000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vnop",	 0xFFFF0000, 0xFFFFFFFF, "" ,  NUL,  PSP },
		{ "vnrcp.p", 0xD0180080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vnrcp.q", 0xD0188080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vnrcp.s", 0xD0180000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vnrcp.t", 0xD0188000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vnsin.p", 0xD01A0080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vnsin.q", 0xD01A8080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vnsin.s", 0xD01A0000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vnsin.t", 0xD01A8000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vocp.p",	 0xD0440080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vocp.q",	 0xD0448080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vocp.s",	 0xD0440000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vocp.t",	 0xD0448000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vone.p",	 0xD0070080, 0xFFFFFF80, "%zp" ,  NUL,  PSP },
		{ "vone.q",	 0xD0078080, 0xFFFFFF80, "%zq" ,  NUL,  PSP },
		{ "vone.s",	 0xD0070000, 0xFFFFFF80, "%zs" ,  NUL,  PSP },
		{ "vone.t",	 0xD0078000, 0xFFFFFF80, "%zt" ,  NUL,  PSP },
		{ "vpfxd",	 0xDE000000, 0xFF000000, "[%vp4, %vp5, %vp6, %vp7]" ,  NUL,  PSP }, // [hlide] added "[%vp4, %vp5, %vp6, %vp7]"
		{ "vpfxs",	 0xDC000000, 0xFF000000, "[%vp0, %vp1, %vp2, %vp3]" ,  NUL,  PSP }, // [hlide] added "[%vp0, %vp1, %vp2, %vp3]"
		{ "vpfxt",	 0xDD000000, 0xFF000000, "[%vp0, %vp1, %vp2, %vp3]" ,  NUL,  PSP }, // [hlide] added "[%vp0, %vp1, %vp2, %vp3]"
		{ "vqmul.q", 0xF2808080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP }, // [hlide] added "%zq, %yq, %xq"
		{ "vrcp.p",	 0xD0100080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vrcp.q",	 0xD0108080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vrcp.s",	 0xD0100000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vrcp.t",	 0xD0108000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vrexp2.p",0xD01C0080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vrexp2.q",0xD01C8080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vrexp2.s", 0xD01C0000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vrexp2.t",0xD01C8000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vrndf1.p", 0xD0220080, 0xFFFFFF80, "%zp" ,  NUL,  PSP },
		{ "vrndf1.q",0xD0228080, 0xFFFFFF80, "%zq" ,  NUL,  PSP },
		{ "vrndf1.s", 0xD0220000, 0xFFFFFF80, "%zs" ,  NUL,  PSP },
		{ "vrndf1.t",0xD0228000, 0xFFFFFF80, "%zt" ,  NUL,  PSP },
		{ "vrndf2.p", 0xD0230080, 0xFFFFFF80, "%zp" ,  NUL,  PSP },
		{ "vrndf2.q",0xD0238080, 0xFFFFFF80, "%zq" ,  NUL,  PSP },
		{ "vrndf2.s", 0xD0230000, 0xFFFFFF80, "%zs" ,  NUL,  PSP },
		{ "vrndf2.t",0xD0238000, 0xFFFFFF80, "%zt" ,  NUL,  PSP },
		{ "vrndi.p", 0xD0210080, 0xFFFFFF80, "%zp" ,  NUL,  PSP },
		{ "vrndi.q", 0xD0218080, 0xFFFFFF80, "%zq" ,  NUL,  PSP },
		{ "vrndi.s", 0xD0210000, 0xFFFFFF80, "%zs" ,  NUL,  PSP },
		{ "vrndi.t", 0xD0218000, 0xFFFFFF80, "%zt" ,  NUL,  PSP },
		{ "vrnds.s", 0xD0200000, 0xFFFF80FF, "%ys" ,  NUL,  PSP },
		{ "vrot.p",	 0xF3A00080, 0xFFE08080, "%zp, %ys, %vr" ,  NUL,  PSP }, // [hlide] added "%zp, %ys, %vr"
		{ "vrot.q",	 0xF3A08080, 0xFFE08080, "%zq, %ys, %vr" ,  NUL,  PSP }, // [hlide] added "%zq, %ys, %vr"
		{ "vrot.t",	 0xF3A08000, 0xFFE08080, "%zt, %ys, %vr" ,  NUL,  PSP }, // [hlide] added "%zt, %ys, %vr"
		{ "vrsq.p",	 0xD0110080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vrsq.q",	 0xD0118080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vrsq.s",	 0xD0110000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vrsq.t",	 0xD0118000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vs2i.p",	 0xD03B0080, 0xFFFF8080, "%zq, %yp" ,  NUL,  PSP }, // [hlide] %zp -> %zq
		{ "vs2i.s",	 0xD03B0000, 0xFFFF8080, "%zp, %ys" ,  NUL,  PSP }, // [hlide] %zs -> %zp
		{ "vsat0.p", 0xD0040080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vsat0.q", 0xD0048080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsat0.s", 0xD0040000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vsat0.t", 0xD0048000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vsat1.p", 0xD0050080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vsat1.q", 0xD0058080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsat1.s", 0xD0050000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vsat1.t", 0xD0058000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vsbn.s",	 0x61000000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP },
		{ "vsbz.s",	 0xD0360000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vscl.p",	 0x65000080, 0xFF808080, "%zp, %yp, %xs" ,  NUL,  PSP }, // [hlide] %xp -> %xs
		{ "vscl.q",	 0x65008080, 0xFF808080, "%zq, %yq, %xs" ,  NUL,  PSP }, // [hlide] %xq -> %xs
		{ "vscl.t",	 0x65008000, 0xFF808080, "%zt, %yt, %xs" ,  NUL,  PSP }, // [hlide] %xt -> %xs
		{ "vscmp.p", 0x6E800080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vscmp.q", 0x6E808080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vscmp.s", 0x6E800000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP },
		{ "vscmp.t", 0x6E808000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vsge.p",	 0x6F000080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vsge.q",	 0x6F008080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vsge.s",	 0x6F000000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP },
		{ "vsge.t",	 0x6F008000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vsgn.p",	 0xD04A0080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vsgn.q",	 0xD04A8080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsgn.s",	 0xD04A0000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vsgn.t",	 0xD04A8000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vsin.p",	 0xD0120080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vsin.q",	 0xD0128080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsin.s",	 0xD0120000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vsin.t",	 0xD0128000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vslt.p",	 0x6F800080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vslt.q",	 0x6F808080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vslt.s",	 0x6F800000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP },
		{ "vslt.t",	 0x6F808000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vsocp.p", 0xD0450080, 0xFFFF8080, "%zq, %yp" ,  NUL,  PSP }, // [hlide] %zp -> %zq
		{ "vsocp.s", 0xD0450000, 0xFFFF8080, "%zp, %ys" ,  NUL,  PSP }, // [hlide] %zs -> %zp
		{ "vsqrt.p", 0xD0160080, 0xFFFF8080, "%zp, %yp" ,  NUL,  PSP },
		{ "vsqrt.q", 0xD0168080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsqrt.s", 0xD0160000, 0xFFFF8080, "%zs, %ys" ,  NUL,  PSP },
		{ "vsqrt.t", 0xD0168000, 0xFFFF8080, "%zt, %yt" ,  NUL,  PSP },
		{ "vsrt1.q", 0xD0408080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsrt2.q", 0xD0418080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsrt3.q", 0xD0488080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsrt4.q", 0xD0498080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP },
		{ "vsub.p",	 0x60800080, 0xFF808080, "%zp, %yp, %xp" ,  NUL,  PSP },
		{ "vsub.q",	 0x60808080, 0xFF808080, "%zq, %yq, %xq" ,  NUL,  PSP },
		{ "vsub.s",	 0x60800000, 0xFF808080, "%zs, %ys, %xs" ,  NUL,  PSP },
		{ "vsub.t",	 0x60808000, 0xFF808080, "%zt, %yt, %xt" ,  NUL,  PSP },
		{ "vsync",	 0xFFFF0000, 0xFFFF0000, "%I" ,  NUL,  PSP },
		{ "vsync",	 0xFFFF0320, 0xFFFFFFFF, "" ,  NUL,  PSP },
		{ "vt4444.q",0xD0598080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP }, // [hlide] %zq -> %zp
		{ "vt5551.q",0xD05A8080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP }, // [hlide] %zq -> %zp
		{ "vt5650.q",0xD05B8080, 0xFFFF8080, "%zq, %yq" ,  NUL,  PSP }, // [hlide] %zq -> %zp
		{ "vtfm2.p", 0xF0800080, 0xFF808080, "%zp, %ym, %xp" ,  NUL,  PSP }, // [hlide] added "%zp, %ym, %xp"
		{ "vtfm3.t", 0xF1008000, 0xFF808080, "%zt, %yn, %xt" ,  NUL,  PSP }, // [hlide] added "%zt, %yn, %xt"
		{ "vtfm4.q", 0xF1808080, 0xFF808080, "%zq, %yo, %xq" ,  NUL,  PSP }, // [hlide] added "%zq, %yo, %xq"
		{ "vus2i.p", 0xD03A0080, 0xFFFF8080, "%zq, %yp" ,  NUL,  PSP }, // [hlide] added "%zq, %yp"
		{ "vus2i.s", 0xD03A0000, 0xFFFF8080, "%zp, %ys" ,  NUL,  PSP }, // [hlide] added "%zp, %ys"
		{ "vwb.q",	 0xF8000002, 0xFC000002, "%Xq, %Y" ,  NUL,  PSP },
		{ "vwbn.s",	 0xD3000000, 0xFF008080, "%zs, %xs, %I" ,  NUL,  PSP },
		{ "vzero.p", 0xD0060080, 0xFFFFFF80, "%zp" ,  NUL,  PSP },
		{ "vzero.q", 0xD0068080, 0xFFFFFF80, "%zq" ,  NUL,  PSP },
		{ "vzero.s", 0xD0060000, 0xFFFFFF80, "%zs" ,  NUL,  PSP },
		{ "vzero.t", 0xD0068000, 0xFFFFFF80, "%zt" ,  NUL,  PSP },
		{ "mfvme", 0x68000000, 0xFC000000, "%t, %i",  NUL, 0 },
		{ "mtvme", 0xb0000000, 0xFC000000, "%t, %i",  NUL, 0 },
};
//---------------------------------------------------------------------------
double LCPU::get_Register(char *name)
{
   int i;

   if(*name == 'r')
       name++;
   for(i=0;i<sizeof(regName)/4;i++){
       if(strcmp(name,regName[i]) == 0)
           return get_Register((Registers)i);
   }
   return 0;
}
//---------------------------------------------------------------------------
void LCPU::dis_reg(Registers reg,char *out,int maxLen)
{
   int i;

   if((int)reg < 32)
       wsprintf(out,"%s 0x%08X",regName[(int)reg],(int)get_Register(reg));
   else if(reg < 64)
       sprintf(out,"fr%02d %.5f",(int)reg - 32,(float)get_Register(reg));
   else if(reg < 67)
       wsprintf(out,"%s 0x%08X",regName[reg],(int)get_Register(reg));
   else{
       i = reg - 67;
       sprintf(out,"S%1d%1d%1d %.5f",(i >> 4),(i % 16) / 4,(i % 16) & 3,(float)get_Register(reg));
   }
}
//---------------------------------------------------------------------------
static int print_vfpusingle(int reg, char *output)
{
   int len;

   len = sprintf(output, "S%d%d%d", (reg >> 2) & 7, reg & 3, (reg >> 5) & 3);
   return len;
}
//---------------------------------------------------------------------------
static int print_vfpu_reg(int reg, int offset, char one, char two, char *output)
{
   int len;

   if((reg >> 5) & 1)
       len = sprintf(output, "%c%d%d%d", two, (reg >> 2) & 7, offset, reg & 3);
   else
       len = sprintf(output, "%c%d%d%d", one, (reg >> 2) & 7, reg & 3, offset);
   return len;
}
//---------------------------------------------------------------------------
static int print_vfpuquad(int reg, char *output)
{
	return print_vfpu_reg(reg, 0, 'C', 'R', output);
}
//---------------------------------------------------------------------------
static int print_vfpupair(int reg, char *output)
{
	if((reg >> 6) & 1)
	{
		return print_vfpu_reg(reg, 2, 'C', 'R', output);
	}
	else
	{
		return print_vfpu_reg(reg, 0, 'C', 'R', output);
	}
}
//---------------------------------------------------------------------------
static int print_vfputriple(int reg, char *output)
{
	if((reg >> 6) & 1)
	{
		return print_vfpu_reg(reg, 1, 'C', 'R', output);
	}
	else
	{
		return print_vfpu_reg(reg, 0, 'C', 'R', output);
	}
}
//---------------------------------------------------------------------------
static int print_vfpumpair(int reg, char *output)
{
	if((reg >> 6) & 1)
	{
		return print_vfpu_reg(reg, 2, 'M', 'E', output);
	}
	else
	{
		return print_vfpu_reg(reg, 0, 'M', 'E', output);
	}
}
//---------------------------------------------------------------------------
static int print_vfpumtriple(int reg, char *output)
{
	if((reg >> 6) & 1)
	{
		return print_vfpu_reg(reg, 1, 'M', 'E', output);
	}
	else
	{
		return print_vfpu_reg(reg, 0, 'M', 'E', output);
	}
}
//---------------------------------------------------------------------------
static int print_vfpumatrix(int reg, char *output)
{
   return print_vfpu_reg(reg, 0, 'M', 'E', output);
}
//---------------------------------------------------------------------------
static int print_vfpureg(int reg, char type, char *output)
{
   switch(type){
       case 's':
           return print_vfpusingle(reg, output);
		case 'q':
           return print_vfpuquad(reg, output);
		case 'p':
           return print_vfpupair(reg, output);
		case 't':
           return print_vfputriple(reg, output);
		case 'm':
           return print_vfpumpair(reg, output);
		case 'n':
           return print_vfpumtriple(reg, output);
		case 'o':
           return print_vfpumatrix(reg, output);
		default:
           return 0;
   }
}
//---------------------------------------------------------------------------
void LCPU::dis_ins(DWORD address,char *out,int maxLen)
{
   Instruction *p;
   DWORD opcode;
   int i,len,n,m;
   char c,s[40];

   opcode = read_dword(address);
   wsprintf(out,"%08X %08X ",address,opcode);
/*   for(p = (Instruction *)g_macro,i=0;i<sizeof(g_macro)/sizeof(Instruction);i++,p++){
       if((opcode & p->mask) == p->opcode){
           lstrcat(out,p->name);
           i = -1;
           break;
       }
   }*/
   i = 0;
   if(i != -1){
       for(p = (Instruction *)g_inst,i=0;i<sizeof(g_inst)/sizeof(Instruction);i++,p++){
           if((opcode & p->mask) == p->opcode){
               lstrcat(out,p->name);
               i =-1;
               break;
           }
       }
   }
   if(i != -1){
       if((opcode & 0xFF000000) == 0xFC000000){
           i = opcode & 0x03FFFFFF;
           if(i < 494){
               lstrcat(out,"jr ");
               lstrcat(out,syscall_func[i].name);
               return;
           }
       }
       lstrcat(out,"unknow");
       return;
   }
   lstrcat(out," ");
   m = lstrlen(out);
   len = lstrlen(p->fmt);
   for(n=0;n<len;n++){
       c = p->fmt[n];
       if(c != '%'){
           out[m++] = c;
           out[m] = 0;
           continue;
       }
       c = p->fmt[++n];
       switch(c){
           case 'd':
               lstrcat(out,regName[RD(opcode)]);
               m = lstrlen(out);
           break;
           case 't':
               lstrcat(out,regName[RT(opcode)]);
               m = lstrlen(out);
           break;
           case 's':
               lstrcat(out,regName[RS(opcode)]);
               m = lstrlen(out);
           break;
           case 'D':
               lstrcat(out,regName[32+FD(opcode)]);
               m = lstrlen(out);
           break;
           case 'o':
               wsprintf(s,"%s[%d]",regName[RS(opcode)],IMM(opcode));
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           case 'O':
               wsprintf(s,"0x%08X",address + (IMM(opcode) << 2) + 4);
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           case 'i':
               wsprintf(s,"%d", IMM(opcode));
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           case 'I':
               wsprintf(s,"0x%04X", IMMU(opcode));
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           case 'a':
               wsprintf(s,"%d",SA(opcode));
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           /*			case 'c':
               params ~= std.string.format("0x%05X ; %s", CODE, sbreak(CODE)); i_params ~= CODE;
           break;
           case 'C':
               params ~= std.string.format("0x%05X ; %s", CODE, syscall(CODE)); i_params ~= CODE;
           break;*/
           case 'n':
               switch (p->fmt[++n]) {
                   case 'e':
                       wsprintf(s,"%d", RD(opcode)+1);
                       m += lstrlen(s);
                       lstrcat(out,s);
                   break;
                   case 'i':
                       wsprintf(s,"%d", RD(opcode) - SA(opcode) +1);
                       m += lstrlen(s);
                       lstrcat(out,s);
                   break;
                   default:
                       n--;
                   break;
               }
           break;
           case 'j':
               wsprintf(s,"0x%08X", JUMP(address+4,opcode));
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           case 'J':
               lstrcat(out,regName[RS(opcode)]);
               m = lstrlen(out);
           break;
           case 'T':
               lstrcat(out,regName[32+FT(opcode)]);
               m = lstrlen(out);
           break;
           case 'S':
           case '1':
               lstrcat(out,regName[32+FS(opcode)]);
               m = lstrlen(out);
           break;
           case 'C':
               wsprintf(s,"0x%05X", CODE(opcode));
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           case 'x':
               print_vfpureg(VT(opcode),p->fmt[++n],s);
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           case 'y':
               i = VS(opcode);
               if(i & 0x20)
                   i &= 0x5F;
               else
                   i |= 0x20;
               print_vfpureg(i,p->fmt[++n],s);
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           case 'z'://33c
               print_vfpureg(VD(opcode),p->fmt[++n],s);
               m += lstrlen(s);
               lstrcat(out,s);
           break;
           default:
               out[m++] = '%';
               out[m++] = c;
               out[m] = 0;
           break;
       }
   }
}


