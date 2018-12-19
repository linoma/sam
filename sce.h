#include <windows.h>

//---------------------------------------------------------------------------
#ifndef __SCEH__
#define __SCEH__

#ifndef __LONGTYPES__
#define __LONGTYPES__
#ifdef __BORLANDC__
typedef __int64 S64;
typedef unsigned __int64 U64;
#else
typedef signed long long int S64;
typedef unsigned long long int U64;
#endif
#endif

typedef unsigned int   SceUInt32;
typedef unsigned int   SceUInt;
typedef unsigned int   SceSize;
typedef int            SceUID;
//---------------------------------------------------------------------------
typedef struct SceKernelSysClock {
	SceUInt32   low;
	SceUInt32   hi;
} SceKernelSysClock;
//---------------------------------------------------------------------------
typedef struct SceKernelThreadInfo {
	SceSize             size;
	char    	        name[32];
	SceUInt             attr;
	int     	        status;
	SceUInt32           entry;
	SceUInt32  	        stack;
	int     	        stackSize;
	SceUInt32  	        gpReg;
	int     	        initPriority;
	int     	        currentPriority;
	int     	        waitType;
	SceUID  	        waitId;
	int     	        wakeupCount;
	int     	        exitStatus;
	SceKernelSysClock   runClocks;
	SceUInt             intrPreemptCount;
	SceUInt             threadPreemptCount;
	SceUInt             releaseCount;
} SceKernelThreadInfo;
//---------------------------------------------------------------------------
enum PspCtrlButtons
{
	PSP_CTRL_SELECT     = 0x000001,
	PSP_CTRL_START      = 0x000008,
	PSP_CTRL_UP         = 0x000010,
	PSP_CTRL_RIGHT      = 0x000020,
	PSP_CTRL_DOWN      	= 0x000040,
	PSP_CTRL_LEFT      	= 0x000080,
	PSP_CTRL_LTRIGGER   = 0x000100,
	PSP_CTRL_RTRIGGER   = 0x000200,
	PSP_CTRL_TRIANGLE   = 0x001000,
	PSP_CTRL_CIRCLE     = 0x002000,
	PSP_CTRL_CROSS      = 0x004000,
	PSP_CTRL_SQUARE     = 0x008000,
	PSP_CTRL_HOME       = 0x010000,
	PSP_CTRL_HOLD       = 0x020000,
	PSP_CTRL_NOTE       = 0x800000,
	PSP_CTRL_SCREEN     = 0x400000,
	PSP_CTRL_VOLUP      = 0x100000,
	PSP_CTRL_VOLDOWN    = 0x200000,
	PSP_CTRL_WLAN_UP    = 0x040000,
	PSP_CTRL_REMOTE     = 0x080000,
	PSP_CTRL_DISC       = 0x1000000,
	PSP_CTRL_MS         = 0x2000000,
};
//---------------------------------------------------------------------------
enum PspCtrlMode{PSP_CTRL_MODE_DIGITAL = 0,PSP_CTRL_MODE_ANALOG};

typedef struct {
	unsigned int 	TimeStamp;
	unsigned int 	Buttons;
	unsigned char 	Lx;
	unsigned char 	Ly;
	unsigned char 	Rsrv[6];
} SceCtrlData;
//---------------------------------------------------------------------------
typedef struct ScePspDateTime {
	unsigned short	year;
	unsigned short 	month;
	unsigned short 	day;
	unsigned short 	hour;
	unsigned short 	minute;
	unsigned short 	second;
	unsigned int 	microsecond;
} ScePspDateTime;
//---------------------------------------------------------------------------
typedef struct SceIoStat {
	int 		    st_mode;
	unsigned int 	st_attr;
	S64  		st_size;
	ScePspDateTime 	st_ctime;
	ScePspDateTime 	st_atime;
	ScePspDateTime 	st_mtime;
	unsigned int 	st_private[6];
} SceIoStat;
//---------------------------------------------------------------------------
typedef struct SceIoDirent {
	SceIoStat 	d_stat;
	char 		d_name[256];
	void * 		d_private;
	int 		dummy;
} SceIoDirent;
//---------------------------------------------------------------------------
typedef struct PspGeCallbackData
{
	unsigned long signal_func;
	unsigned long signal_arg;
	unsigned long finish_func;
	unsigned long finish_arg;
} PspGeCallbackData;
//---------------------------------------------------------------------------
typedef struct {
   unsigned long sid;
   unsigned long nid;
   char name[80];
   int (*pfn)();
} SO_FUNC, *LPSO_FUNC;
//---------------------------------------------------------------------------

int sceKernelLibcGettimeofday();
//Threads
int sceKernelCreateThread();
int sceKernelStartThread();
int sceKernelDeleteThread();
int sceKernelSleepThread();
int sceKernelGetThreadId();
int sceKernelReferThreadStatus();
int sceKernelDelayThread();
int sceKernelDelayThreadCB();
int sceKernelWakeupThread();
int sceKernelSuspendThread();
int sceKernelResumeThread();

//Semaphores
int sceKernelCreateSema();
int sceKernelWaitSema();
int sceKernelSignalSema();
int sceKernelDeleteSema();

//Events
int sceKernelCreateEventFlag();
int sceKernelWaitEventFlag();
int sceKernelClearEventFlag();
int sceKernelSetEventFlag();
int sceKernelPollEventFlag();

int sceKernelLoadModule();
int sceKernelStartModule();

int sceGeEdramGetAddr();
int sceGeListEnQueue();
int sceGeListSync();
int sceGeListUpdateStallAddr();
int sceGeDrawSync();
int sceGeEdramGetSize();
int sceGeSetCallback();
int sceGeUnsetCallback();

int sceDisplaySetFrameBuf();
int sceDisplaySetMode();
int sceDisplayWaitVblankStart();

int sceCtrlSetSamplingCycle();
int sceCtrlSetSamplingMode();
int sceCtrlReadBufferPositive();
int sceCtrlPeekBufferPositive();

int sceKernelCreateCallback();
int sceKernelRegisterExitCallback();
int sceKernelSleepThreadCB();
int sceKernelAllocPartitionMemory();
int sceKernelGetBlockHeadAddr();
int sceKernelMaxFreeMemSize();
int sceKernelTotalFreeMemSize();

//
int sceRtcGetCurrentTick();
int sceRtcGetTickResolution();
int sceKernelLibcClock();
int sceKernelLibcTime();

//Files
int sceIoGetstat();
int sceIoOpen();
int sceIoLseek();
int sceIoRead();
int sceIoWrite();
int sceIoClose();
int sceIoRemove();
int sceIoLseek32();

//Directory
int sceIoDopen();
int sceIoDread();
int sceIoDclose();

int sceHprmPeekCurrentKey();

//Audio
int sceAudioChReserve();
int sceAudioOutputPannedBlocking();

int sceKernelDcacheWritebackAll();

int sceKernelStdout();
int sceKernelStderr();

int sceKernelUtilsMt19937UInt();
#endif
