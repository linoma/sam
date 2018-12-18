#include "ldsp.h"

//---------------------------------------------------------------------------
LDSP::LDSP()
{
}
//---------------------------------------------------------------------------
LDSP::~LDSP()
{
}
//---------------------------------------------------------------------------
BOOL LDSP::Init()
{
   return TRUE;
}
//---------------------------------------------------------------------------
void LDSP::Reset()
{
   ZeroMemory(channels,sizeof(channels));
}
//---------------------------------------------------------------------------
int LDSP::allocate_channel(int channel,int samples,int format)
{
   int i;

   if(channel == -1){
       for(i=0;i<8;i++){
           if(channels[i].allocate == 0)
               break;
       }
   }
   else
       i = channel;
   if(i < 0 || i > 7)
       return -1;
   channels[i].allocate = 1;
   channels[i].samples = samples;
   channels[i].format = 0;
   return i;
}
//---------------------------------------------------------------------------
int LDSP::free_channel(int channel)
{
   if(channel < 0 || channel > 7)
       return -1;
   channels[channel].allocate = 0;
   return 0;
}
//---------------------------------------------------------------------------
LPPSP_AUDIO_CHANNEL LDSP::get_channel(int channel)
{
   if(channel < 0 || channel > 7)
       return NULL;
   return &channels[channel];
}
