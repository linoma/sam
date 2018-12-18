#include "bp.h"
#include "util.h"
#include "lpsp.h"

//---------------------------------------------------------------------------
LBreakPoint::LBreakPoint()
{
	Enable = FALSE;
	Type = BT_PROGRAM;
   Address = 0;
   ZeroMemory(Condition,sizeof(Condition));
   ZeroMemory(Description,sizeof(Description));
   PassCount = 0;
	int_PassCount = 0;
}
//---------------------------------------------------------------------------
LBreakPoint::~LBreakPoint()
{
}
//---------------------------------------------------------------------------
BOOL LBreakPoint::Read(LFile *pFile,int ver)
{
	if(pFile == NULL)
   	return FALSE;
	if(pFile->Read(&Address,sizeof(Address)) != sizeof(Address))
   	return FALSE;
	if(pFile->Read(&Enable,sizeof(Enable)) != sizeof(Enable))
   	return FALSE;
   if(pFile->Read(&PassCount,sizeof(PassCount)) != sizeof(PassCount))
   	return FALSE;
   if(pFile->Read(&int_PassCount,sizeof(int_PassCount)) != sizeof(int_PassCount))
   	return FALSE;
	if(pFile->Read(Condition,30) != 30)
   	return FALSE;
	if(pFile->Read(&Type,sizeof(Type)) != sizeof(Type))
   	return FALSE;
   if(ver != 0){
       if(pFile->Read(Description,100) != 100)
   	    return FALSE;
   }
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LBreakPoint::Write(LFile *pFile)
{
	if(pFile == NULL)
   	return FALSE;
	pFile->Write(&Address,sizeof(Address));
	pFile->Write(&Enable,sizeof(Enable));
   pFile->Write(&PassCount,sizeof(PassCount));
   pFile->Write(&int_PassCount,sizeof(int_PassCount));
	pFile->Write(Condition,30);
	pFile->Write(&Type,sizeof(Type));
	pFile->Write(Description,100);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LBreakPoint::Check(unsigned long a0,int accessMode)
{
   unsigned long a1;
   DWORD value,value1;

	switch(Type){
   	case BT_PROGRAM:
       	if(Address != a0)
           	return FALSE;
           if(PassCount == 0 && Condition[0] == 0)
				return TRUE;
           else if(PassCount && ++int_PassCount >= PassCount)
           	return TRUE;
   		if(Condition[0] == 0)
           	return FALSE;
           switch(Condition[0]){
               case 'h':
                   value1 = read_word(value1);
               break;
               case 'b':
                   value1 = read_byte(value1);
               break;
               case 'w':
                   value1 = read_dword(value1);
               break;
               default:
           		value1 = psp.get_Register((Registers)Condition[1]);
               break;
           }
       	if(Condition[3] == 'r')
           	value = psp.get_Register((Registers)Condition[4]);
       	else if(Condition[3] == 'v')
           	value = *((DWORD *)&Condition[4]);
       	switch(Condition[2]){
           	case CC_EQ:
              		if(value1 == value)
                  		return TRUE;
				break;
              	case CC_NE:
              		if(value1 != value)
                   	return TRUE;
               break;
               case CC_GT:
               	if(value1 > value)
                  		return TRUE;
               break;
               case CC_GE:
               	if(value1 >= value)
                  		return TRUE;
               break;
               case CC_LT:
               	if(value1 < value)
                  		return TRUE;
               break;
               case CC_LE:
               	if(value1 <= value)
                  		return TRUE;
               break;
           }
			return FALSE;
       case BT_MEMORY:
   		a1 = a0;
   		switch(accessMode & 0xF){
       		case AMM_WORD:
					a1 += 1;
       		break;
       		case AMM_DWORD:
       			a1 += 3;
       		break;
   		}
           if(has_Range()){
           	if(!((a0 >= Address && a0 <= get_Address2()) ||
                  (a1 >= Address && a1 <= get_Address2())))
                  		return FALSE;
           }
           else{
				if(Address < a0 || Address > a1)
           		return FALSE;
           }
      		if(((accessMode & AMM_WRITE) && is_Write()))
          		goto Check_1;
          	if(((accessMode & AMM_READ) && is_Read()))
          		goto Check_1;
           return FALSE;
Check_1:
           if(Condition[10] == 0)
               return TRUE;
   		switch(accessMode & 0xF){
       		case AMM_WORD:
                   value1 = read_word(a0);
       		break;
       		case AMM_DWORD:
                   value1 = read_dword(a0);
       		break;
               case AMM_BYTE:
                   value1 = read_byte(a0);
               break;
   		}
           value = *((DWORD *)&Condition[12]);
       	switch(Condition[10]){
               case CC_EQ:
                   if(value1 == value)
                  	    return TRUE;
               break;
              	case CC_NE:
                   if(value1 != value)
                       return TRUE;
               break;
               case CC_GT:
                   if(value1 > value)
                  	    return TRUE;
               break;
               case CC_GE:
                   if(value1 >= value)
                  	    return TRUE;
               break;
               case CC_LT:
                   if(value1 < value)
                  	    return TRUE;
               break;
               case CC_LE:
                   if(value1 <= value)
                  	    return TRUE;
               break;
           }
			return FALSE;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BYTE LBreakPoint::ConditionToValue(int *rd,char *cond,DWORD *value,unsigned char type)
{
   if(type == BT_PROGRAM){
       if(Condition[0] == 0)
           return 0;
       *rd = Condition[1];
       if(Condition[0] != 'r'){
       	switch(Condition[0]){
				case 'b':
               	*rd |= 0x20;
               break;
				case 'h':
               	*rd |= 0x40;
               break;
				case 'w':
               	*rd |= 0x10;
               break;
               default:
               	*rd |= 0x80;
               break;
           }
       }
       *cond = Condition[2];
       if(Condition[3] == 'r')
           *value = Condition[4];
       else if(Condition[3] == 'v')
           *value = *((DWORD *)&Condition[4]);
       if(Condition[3] == 'r')
           return 2;
       return 1;
   }
   if(Condition[10] == 0)
       return 0;
   *cond = Condition[10];
   *value = *((DWORD *)&Condition[12]);
   return 1;
}
//---------------------------------------------------------------------------
void LBreakPoint::StringToCondition(char *s,unsigned char type)
{
	char *p,c[50],*p1,*p2,c1[10];
	int i,i1;

	if(s == NULL || s[0] == 0){
       if(type == BT_PROGRAM)
   	    ZeroMemory(Condition,sizeof(Condition));
       else
           ZeroMemory(&Condition[10],sizeof(Condition) - 10);
		return;
   }
   ((long *)c)[0] = 0;
   i1 = 0;
   if(type == BT_PROGRAM){
       p = strpbrk(s,"rcvnz");
       if(p == NULL)
   	    return;
       c[i1] = *p;
       if(p != s){
           i = (int)((DWORD)p - (DWORD)s);
           if(*(p-i) == '*'){
               c[i1] = 'w';
               if(i > 1){
                   if(*(p-i+1) == 'b' || *(p-i+1) =='B')
                       c[i1] = 'b';
                   else if(*(p-i+1) == 'h' || *(p-i+1) == 'H')
                       c[i1] = 'h';
               }
           }
       }
       i1++;
       c[i1++] = (char)atoi(p+1);
   }
   p = strpbrk(s,"!=<>");
   if(p == 0)
   	return;
   p2 = p + 1;
   do{
   	p1 = strpbrk(p2,"!=<>");
		if(p1 == NULL)
       	break;
		p2 = p1 + 1;
   }while(1);
	i = (int)p - (int)s;
   while(*(p-1) == 32){
   	i--;
       p--;
   }
	while(s[i] == 32)
   	i++;
   p1 = &s[i];
   i = (int)p2 - (int)p;
   lstrcpyn(c1,p1,i+1);
   i = 1;
   if(c1[0] == '!')
   	i = CC_NE;
	else if(c1[0] == '>'){
       i = c1[1] == '=' ? CC_GE : CC_GT;
   }
	else if(c1[0] == '<'){
   	i = c1[1] == '=' ? CC_LE : CC_LT;
   }
	c[i1++] = (char)i;
   while(*p2 == 32)
   	p2++;
   if(*p2 == 'r'){
   	c[i1++] = *p2;
       c[i1++] = (char)atoi(p2+1);
   }
   else{
   	c[i1++] = 'v';
		*((int *)&c[i1]) = StrToHex(p2);
       i1 += 4;
   }
   c[i1++] = 0;
   i = type == BT_PROGRAM ? 0 : 10;
   CopyMemory(&Condition[i],c,i1);
}
//---------------------------------------------------------------------------
LString LBreakPoint::ConditionToString(unsigned char type)
{
   LString condition;
   BYTE cond,res;
   char s[50];
   int rd;
   DWORD value;

   condition = "";
   if((res = ConditionToValue(&rd,(char *)&cond,&value,type)) != 0){
       if(type == BT_PROGRAM){
           if(rd & 0x70){
               condition = "*";
               if(rd & 0x20)
                   condition += "b";
               else if(rd & 0x40)
                   condition += "h";
           }
           condition += Condition[0];
           if(!(rd & 0x80))
           	condition += (int)(rd & 0xF);
       }
       switch(cond){
           case CC_EQ:
               condition += " == ";
           break;
           case CC_NE:
               condition += " != ";
           break;
           case CC_GT:
               condition += " >  ";
           break;
           case CC_GE:
               condition += " >= ";
           break;
           case CC_LT:
               condition += " <  ";
           break;
           case CC_LE:
               condition += " <= ";
           break;
       }
       if(res == 1)
       	wsprintf(s,"0x%08X",value);
       else
			wsprintf(s,"r%d",value);
       condition += s;
   }
   return condition;
}

