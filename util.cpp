#include "util.h"
#include <stdio.h>

DWORD StrToHex(char *string)
{
   DWORD iValue,iValue2;
   int i,i1;
   char *p,car,*p1;

   strlwr(string);
   while(*string == 32)
   	string++;
   if((string[0] == '0' || strpbrk(string,"abcdefx") !=  NULL)){
       if((p = strchr(string,'x')) != NULL){
			if((p1 = strpbrk(string,"+-*/")) != NULL){
           	if(p1 < p)
               	p = string;
               else
               	p++;
           }
           else
           	p++;
       }
       else
           p = string;
       i = lstrlen(p);
       if(i > 8){
       	i = 8;
       	if((p1 = strpbrk(string,"+-*/")) != NULL)
           	i = (int)((DWORD)p1 - (DWORD)p);
           while(p[i-1] == 32)	i--;
       }
       for(iValue = 0;i>0;i--){
           car = *p++;
           if((car >= 'a' && car < 'g'))
               i1 = (car - 87);
           else if(car > 47 && car < 58)
               i1 = car - 48;
           else
               i1 = 0;
           iValue = (iValue << 4) + i1;
       }
       if((p = strpbrk(string,"+-*/")) != NULL){
			iValue2 = StrToHex(p+1);
           switch(p[0]){
           	case '+':
               	iValue += iValue2;
               break;
               case '-':
               	iValue -= iValue2;
               break;
               case '*':
               	iValue *= iValue2;
               break;
               case '/':
               	iValue /= iValue2;
               break;
           }
       }
   }
   else
       iValue = atoi(string);
   return iValue;
}

