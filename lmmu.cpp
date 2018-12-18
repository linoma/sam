#include "lmmu.h"
#include "lpsp.h"
#include <stdio.h>

LPBYTE mem;
LPBYTE video_mem;
static FILE *fp1;
//---------------------------------------------------------------------------
LMMU::LMMU()
{
   mem = NULL;
}
//---------------------------------------------------------------------------
LMMU::~LMMU()
{
   if(mem != NULL)
       GlobalFree(mem);
}
//---------------------------------------------------------------------------
BOOL LMMU::Init()
{
   mem = (LPBYTE)GlobalAlloc(LMEM_FIXED,36*1024*1024);
   if(mem == NULL)
       return FALSE;
   video_mem = mem + 32*1024*1024;
   return TRUE;
}
//---------------------------------------------------------------------------
void LMMU::Reset()
{
   if(mem != NULL)
       ZeroMemory(mem,36*1024*1024);
}
//---------------------------------------------------------------------------
DWORD __fastcall read_byte(DWORD address)
{
   switch((address >> 24) & 0x3F){
       case 0:
           return 0;
       case 8:
       case 9:
           return (DWORD)mem[address & 0x01FFFFFF];
       case 4:
           return (DWORD)video_mem[address & 0x1FFFFF];
       default:
           return 0;
   }
}
//---------------------------------------------------------------------------
DWORD __fastcall read_word(DWORD address)
{
    switch((address >> 24) & 0x3F){
        case 0:
            return 0;
        case 8:
        case 9:
            return (DWORD)*((WORD *)&mem[address & 0x01FFFFFE]);
        case 4:
            return (DWORD)*((WORD *)&video_mem[address & 0x1FFFFE]);
        default:
           return 0;
    }
}
//---------------------------------------------------------------------------
DWORD __fastcall read_dword(DWORD address)
{
    switch((address >> 24) & 0x3F){
        case 0:
            if(address == 0)
               return 0x08000000;
            return 0;
        case 8:
        case 9: //8aa0ceo
            return (DWORD)*((DWORD *)&mem[address & 0x01FFFFFC]);
        case 4:
            return (DWORD)*((DWORD *)&video_mem[address & 0x1FFFFC]);
        default:
           return 0;
    }
}
//---------------------------------------------------------------------------
void __fastcall write_byte(DWORD address,DWORD value)
{
   switch((address >> 24) & 0x3F){
       case 0:
       break;
       case 8:
       case 9:
           mem[address & 0x01FFFFFF] = (BYTE)value;
       break;
       case 4:
           video_mem[address & 0x1FFFFF] = (BYTE)value;
       break;
       default:
           value = 0;
       break;
   }
}
//---------------------------------------------------------------------------
void __fastcall write_word(DWORD address,DWORD value)
{
    switch((address >> 24) & 0x3F){
        case 0:
        break;
        case 8:
        case 9:
           *((WORD *)&mem[address & 0x01FFFFFE]) = (WORD)value;
        break;
        case 4:
            *((WORD *)&video_mem[(address & 0x1FFFFE)]) = (WORD)value;
        break;
        default:
           value = 0;
           break;
    }
}
//---------------------------------------------------------------------------
void __fastcall write_dword(DWORD address,DWORD value)
{
    switch((address >> 24) & 0x3F){
        case 0:
        break;
        case 8:
        case 9:
           *((DWORD *)&mem[address & 0x01FFFFFC]) = value;
        break;
        case 4:
            *((DWORD *)&video_mem[address & 0x1FFFFC]) = value;
        break;
        default:
           value = value;
        break;
    }
}

