#include <windows.h>

//---------------------------------------------------------------------------
#ifndef __ldspH__
#define __ldspH__

typedef struct
{
   int index;
   int format;
   int samples;
   int allocate;
   WAVEFORMAT wf;
} PSP_AUDIO_CHANNEL,*LPPSP_AUDIO_CHANNEL;
//---------------------------------------------------------------------------
class LDSP
{
public:
   LDSP();
   ~LDSP();
   BOOL Init();
   void Reset();
   int allocate_channel(int channel,int samples,int format);
   int free_channel(int channel);
   LPPSP_AUDIO_CHANNEL get_channel(int channel);
protected:
   PSP_AUDIO_CHANNEL channels[8];
};

#endif
