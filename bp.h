#include <windows.h>
#include "fstream.h"
#include "lstring.h"
//---------------------------------------------------------------------------
#ifndef __bpH__
#define __bpH__

#define BT_PROGRAM     	0
#define BT_MEMORY			1

#define CC_EQ      1
#define CC_NE      2
#define CC_GT      3
#define CC_GE      4
#define CC_LT      5
#define CC_LE      6

//---------------------------------------------------------------------------
class LBreakPoint
{
public:
	LBreakPoint();
   ~LBreakPoint();
   inline BOOL is_Enable(){return Enable;};
   inline unsigned long get_Address(){return Address;};
   inline int get_Type(){return Type;};
   void get_Condition(char *c){CopyMemory(c,Condition,sizeof(Condition));};
   void set_Condition(char *c){CopyMemory(Condition,c,sizeof(Condition));};
   void get_Description(char *c){lstrcpy(c,Description);};
   void set_Description(char *c){lstrcpy(Description,c);};
   void set_Type(int type){Type = (unsigned char )type;};
   BOOL Read(LFile *pFile,int ver = 0);
   BOOL Write(LFile *pFile);
   inline BOOL is_Write(){return Condition[0] & 1 ? TRUE : FALSE;};
   inline BOOL is_Read(){return Condition[0] & 2 ? TRUE : FALSE;};
	inline BOOL is_Modify(){return Condition[0] & 4 ? TRUE : FALSE;};
   inline BOOL is_Break(){return Condition[0] & 8 ? TRUE : FALSE;};
   inline int get_Access(){return Condition[0] >> 4;};
	void set_Write(BOOL b = TRUE){if(b) Condition[0] |= 1; else Condition[0] &= ~1;};
	void set_Read(BOOL b = TRUE){if(b) Condition[0] |= 2; else Condition[0] &= ~2;};
	void set_Modify(BOOL b = TRUE){if(b) Condition[0] |= 4; else Condition[0] &= ~4;};
	void set_Break(BOOL b = TRUE){if(b) Condition[0] |= 8; else Condition[0] &= ~8;};
	void set_Access(char f){Condition[0] &= (char)~0xF0;Condition[0] |= (char)(f << 4);};
   void set_Flags(char f){Condition[0] = f;};
   inline void set_Address(unsigned long l){Address = l;};
   inline void set_Address2(unsigned long l){*((unsigned long *)(Condition+4)) = l;Condition[1] |= 0x80;};
   inline unsigned long get_Address2(){return *((unsigned long *)(Condition+4));};
   inline void set_Enable(BOOL b = TRUE){Enable = (unsigned char)b;};
   inline BOOL has_Range(){if(Type == BT_MEMORY && (Condition[1] & 0x80)) return TRUE; return FALSE;};
   inline unsigned long get_PassCount(){return PassCount;};
   void set_PassCount(unsigned long p){PassCount = p;int_PassCount = 0;};
   BOOL Check(unsigned long a0,int accessMode);
	LString ConditionToString(unsigned char type);
   void StringToCondition(char *sm,unsigned char type = BT_PROGRAM);
   void Reset(){int_PassCount= 0;};
protected:
	BYTE ConditionToValue(int *rd,char *cond,DWORD *value,unsigned char type = BT_PROGRAM);
	unsigned char Type,Enable;
   unsigned long Address,PassCount,int_PassCount;
	char Condition[30],Description[100];
};


#endif
