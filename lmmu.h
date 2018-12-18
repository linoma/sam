#include <windows.h>
#include "fstream.h"
#include "lstring.h"
#include "lvec.hpp"

//---------------------------------------------------------------------------
#ifndef __lmmuH__
#define __lmmuH__
//---------------------------------------------------------------------------
enum RelType {Mips16 = 1,Mips32 = 2,MipsRel32 = 3,Mips26 = 4,
		MipsHi16 = 5,MipsLo16 = 6,MipsGpRel16 = 7,MipsLiteral = 8,
		MipsGot16 = 9,MipsPc16 = 10,MipsCall16 = 11,MipsGpRel32 = 12};

enum ElfType {Executable = 0x0002, Prx = 0xFFA0 };

enum ShType {Null = 0,PROGBITS = 1,SYMTAB = 2,STRTAB = 3,RELA = 4,HASH = 5,DYNAMIC = 6,
		NOTE = 7,NOBITS = 8,REL = 9,SHLIB = 10,DYNSYM = 11,LOPROC = 0x70000000, HIPROC = 0x7FFFFFFF,
		LOUSER = 0x80000000, HIUSER = 0xFFFFFFFF,PRXRELOC = 0x700000A0};

enum ShFlags {Write = 1, Allocate = 2, Execute = 4 };

enum PspModuleFlags {User = 0x0000,Kernel = 0x1000};

enum PspLibFlags {SysLib = 0x8000,DirectJump = 0x0001,Syscall = 0x4000};

#pragma pack(1)
   typedef struct {
		unsigned int pmagic;
		unsigned int pversion;
		unsigned int offset_param_sfo;
		unsigned int offset_icon0_png;
		unsigned int offset_icon1_pmf;
		unsigned int offset_pic0_png;
		unsigned int offset_pic1_png;
		unsigned int offset_snd0_at3;
		unsigned int offset_psp_data;
		unsigned int offset_psar_data;
	} PBP_Header;

   typedef struct { // ELF Header
		unsigned char _magic[4];
		unsigned char _class;     //
		unsigned char _data;      //
		unsigned char _idver;     //
		unsigned char _pad[9];       //
		unsigned short  _type;      // Module type
		unsigned short   _machine;
		unsigned int     _version;   //
		unsigned int     _entry;     // Module EntryPoint
		unsigned int     _phoff;     // Program Header Offset
		unsigned int     _shoff;     // Section Header Offset
		unsigned int     _flags;     // Flags
		unsigned short   _ehsize;    //
		unsigned short   _phentsize; //
		unsigned short   _phnum;     //
		unsigned short   _shentsize; //
		unsigned short   _shnum;     // Section Header Num
		unsigned short   _shstrndx;
	} Elf32_Ehdr;

	typedef struct { // ELF Section Header
		unsigned int    _name;
		ShType  _type;
		ShFlags _flags;
		unsigned int    _addr;
		unsigned int    _offset;
		unsigned int    _size;
		unsigned int    _link;
		unsigned int    _info;
		unsigned int    _addralign;
		unsigned int    _entsize;
	} Elf32_Shdr;

	enum ModuleNids {
		MODULE_INFO = 0xF01D73A7,
		MODULE_BOOTSTART = 0xD3744BE0,
		MODULE_REBOOT_BEFORE = 0x2F064FA6,
		MODULE_START = 0xD632ACDB,
		MODULE_START_THREAD_PARAMETER = 0x0F7C276C,
		MODULE_STOP = 0xCEE8593C,
		MODULE_STOP_THREAD_PARAMETER = 0xCF0CC697,
	};

	typedef struct {
		unsigned int   name;
		unsigned short _version;
		unsigned short flags;
		char   entry_size;
		char   var_count;
		unsigned short func_count;
		unsigned int   exports;
	} PspModuleExport;

	typedef struct {
		unsigned int   name;
		unsigned short _version;
		unsigned short flags;
		char   entry_size;
		char   var_count;
		unsigned short func_count;
		unsigned int   nids;
		unsigned int   funcs;
	} PspModuleImport;

	typedef struct {
		unsigned int flags;
		char name[28];
		unsigned int gp;
		unsigned int exports;
		unsigned int exp_end;
		unsigned int imports;
		unsigned int imp_end;
	} PspModuleInfo;

	typedef struct {
		unsigned int p_type;
		unsigned int p_offset;
		unsigned int p_vaddr;
		unsigned int p_paddr;
		unsigned int p_filesz;
		unsigned int p_memsz;
		unsigned int p_flags;
		unsigned int p_align;
	} Elf32_Phdr;

	typedef struct  {
		unsigned int _offset;
		unsigned int _info;
	} Elf32_Rel;

	typedef struct {
		unsigned int st_name;
		unsigned int st_value;
		unsigned int st_size;
		char st_info;
		char st_other;
		unsigned short st_shndx;
	} Elf32_Sym;
#pragma pack(4)
//---------------------------------------------------------------------------
class LMMU
{
public:
   LMMU();
   ~LMMU();
   BOOL Init();
   void Reset();
protected:
   LString lastFileName,lastPath;
};

extern LPBYTE mem;
extern LPBYTE video_mem;

DWORD __fastcall read_byte(DWORD address);
DWORD __fastcall read_word(DWORD address);
DWORD __fastcall read_dword(DWORD address);
void __fastcall write_byte(DWORD address,DWORD value);
void __fastcall write_word(DWORD address,DWORD value);
void __fastcall write_dword(DWORD address,DWORD value);

#endif

