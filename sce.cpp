#include "sce.h"
#include "os.h"
#include "lpsp.h"
#include <time.h>

extern CPU_BASE R4000;
extern unsigned __int64 ticks;
extern unsigned __int64 start_time;
//---------------------------------------------------------------------------
int sceKernelCreateThread()
{
   R4000.reg[2] = psp.create_thread((const char *)&mem[R4000.reg[4] - 0x08000000],
                       R4000.reg[5],R4000.reg[7],R4000.reg[6]);
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelStartThread()
{
   psp.start_thread(R4000.reg[4],(int)R4000.reg[5],R4000.reg[6]);
   R4000.reg[2] = 0;
   return 100;
 }
//---------------------------------------------------------------------------
int sceKernelDeleteThread()
{
   psp.delete_thread((int)R4000.reg[4]);
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelSleepThread()
{
   psp_thread *p;

   p = psp.get_current_thread();
   p->Sleep();
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelSleepThreadCB()
{
   psp_thread *p;

   p = psp.get_current_thread();
   p->Sleep(1);
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelWakeupThread()
{
   psp_thread *p;

   p = psp.get_thread(R4000.reg[4]);
   if(p == NULL)
       return 50;
   p->status &= ~4;
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelDelayThread()
{
   psp_thread *p;

   p = psp.get_current_thread();
   p->wait(2,NULL,R4000.reg[4],NULL);
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelDelayThreadCB()
{
   psp_thread *p;

   p = psp.get_current_thread();
   p->wait(2,NULL,R4000.reg[4],NULL);
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelSuspendThread()
{
   psp_thread *p;

   p = psp.get_thread(R4000.reg[4]);
   if(p != NULL)
       p->Suspend();
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelResumeThread()
{
   psp_thread *p;

   p = psp.get_thread(R4000.reg[4]);
   if(p != NULL)
       p->Resume();
}
//---------------------------------------------------------------------------
int sceKernelGetThreadId()
{
   psp_thread *p;

   p = psp.get_current_thread();
   R4000.reg[2] = p->res_id;
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelReferThreadStatus()             //8a2d8c0
{
   psp_thread *p;
   SceKernelThreadInfo *info;

   info = (SceKernelThreadInfo *)&mem[R4000.reg[5] - 0x08000000];
   p = psp.get_thread(R4000.reg[4]);
   lstrcpy(info->name,p->name);
   info->initPriority = p->priority;
   info->currentPriority = p->priority;
   info->entry = p->entry;
   info->status = 0;
   if((p->status & 0xC) == 0xC)
       info->status = 4;
   if((p->status & 1))
       info->status = 1;
   R4000.reg[2] = 0;
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelWaitSema()
{
   LPPSP_SEMAPHORE p;
   psp_thread *p1;

   p = psp.get_sema(R4000.reg[4]);
   p1 = psp.get_current_thread();
   p1->wait(1,p,R4000.reg[5],NULL);
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelCreateSema()
{
   R4000.reg[2] = psp.psp_create_sema(R4000.reg[4],R4000.reg[5],R4000.reg[6],
       R4000.reg[7],R4000.reg[8]);
   return 200;
}
//---------------------------------------------------------------------------
int sceKernelSignalSema()
{
   LPPSP_SEMAPHORE p;
   psp_thread *thread;
   DWORD dwPos;

   p = psp.get_sema(R4000.reg[4]);
   if(p == NULL)
       R4000.reg[2] = (DWORD)-1;
   else{
       p->current_value += R4000.reg[5];
       psp.check_wait_condition(1,p);
       R4000.reg[2] = 0;
   }
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelDeleteSema()
{
   R4000.reg[2] = psp.psp_delete_sema(R4000.reg[4]);
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelCreateEventFlag()
{
   R4000.reg[2] = psp.psp_create_event(R4000.reg[4],(int)R4000.reg[5],(int)R4000.reg[6],R4000.reg[7]);
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelWaitEventFlag()
{
   psp_thread *thread;
   LPPSP_EVENT event;
   unsigned long timeout,clear;

   R4000.reg[2] = (DWORD)-1;
   thread = psp.get_current_thread();
   if(thread != NULL){
       event = psp.get_event(R4000.reg[4]);
       if(event != NULL){
           if(R4000.reg[8] != 0)
               timeout = read_dword(R4000.reg[8]);
           else
               timeout = 0;
           thread->wait(3,event,timeout,R4000.reg[5],R4000.reg[6]);
       }
   }
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelClearEventFlag()
{
   LPPSP_EVENT event;

   event = psp.get_event(R4000.reg[4]);
   if(event != NULL){
       event->bits &= ~R4000.reg[5];
       R4000.reg[2] = 0;
   }
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelSetEventFlag()
{
   LPPSP_EVENT event;
   psp_thread *thread;
   DWORD dwPos;

   event = psp.get_event(R4000.reg[4]);
   if(event != NULL){
       event->bits |= R4000.reg[5];
       psp.check_wait_condition(3,event);
       R4000.reg[2] = 0;
   }
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelPollEventFlag()
{
}
//---------------------------------------------------------------------------
int sceKernelLoadModule()
{
   R4000.reg[2] = psp.psp_load_module(R4000.reg[4],R4000.reg[5],R4000.reg[6]);
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelStartModule()
{
}
//---------------------------------------------------------------------------
int sceGeEdramGetAddr()
{
   R4000.reg[2] = 0x04000000;
   return 20;
}
//---------------------------------------------------------------------------
int sceGeEdramGetSize()
{
   R4000.reg[2] = 0x001FFFFF;
   return 20;
}
//---------------------------------------------------------------------------
int sceGeListEnQueue()
{
   LPGPU_LIST p;
   int res;

   p = new GPU_LIST[1];
   if(p == NULL){
       R4000.reg[2] = (DWORD)-1;
       return 100;
   }
   ZeroMemory(p,sizeof(GPU_LIST));
   p->adr = R4000.reg[4];
   p->stall_adr = R4000.reg[5];
   p->callback_id = R4000.reg[6];
   p->arg = R4000.reg[7];
   R4000.reg[2] = (res = psp.add_DisplayList(p));
   if(res < 0)
       delete p;
   return 100;
}
//---------------------------------------------------------------------------
int sceGeListSync()
{
/*
PSP_GE_LIST_DONE = 0,
	PSP_GE_LIST_QUEUED,
	PSP_GE_LIST_DRAWING_DONE,
	PSP_GE_LIST_STALL_REACHED,
	PSP_GE_LIST_CANCEL_DONE
*/
   int qid,syncType;

   qid = (int)R4000.reg[4];
   syncType = (int)R4000.reg[5];
   R4000.reg[2] = 0;
   return 100;
}
//---------------------------------------------------------------------------
int sceGeListUpdateStallAddr()
{
   LPGPU_LIST p;

   p = psp.get_DisplayList(R4000.reg[4]);
   if(p != NULL){
       p->stall_adr = R4000.reg[5];
       p->stalled = 0;
//       psp.draw_DisplayList((int)R4000.reg[4]);
   }
   R4000.reg[2] = 0;
   return 50;      //8a18Acc
}
//---------------------------------------------------------------------------
int sceGeDrawSync()
{
   psp.draw_DisplayList((int)R4000.reg[4]);
   R4000.reg[2] = 0;
   return 100;
}
//---------------------------------------------------------------------------
int sceGeSetCallback()
{
   psp.set_CallbackData(R4000.reg[4]);
   R4000.reg[2] = 1;
   return 100;
}
//---------------------------------------------------------------------------
int sceGeUnsetCallback()
{
   R4000.reg[2] = 1;
   return 100;
}
//---------------------------------------------------------------------------
int sceDisplaySetFrameBuf()
{
   psp.ge_SwapBuffer(R4000.reg[4],R4000.reg[5],R4000.reg[6],R4000.reg[7]);
   return 100;
}
//---------------------------------------------------------------------------
int sceDisplaySetMode()
{
   psp.ge_SetMode(R4000.reg[4],R4000.reg[5],R4000.reg[6]);
}
//---------------------------------------------------------------------------
int sceDisplayWaitVblankStart()
{
   psp_thread *p;
   int status;

   p = psp.get_current_thread();
   if(p != NULL){
       status = psp.get_Status() & 1;
       p->wait(4,(void *)status,0);
   }
   return 50;
}
//---------------------------------------------------------------------------
int sceCtrlSetSamplingCycle()
{
   return 20;
}
//---------------------------------------------------------------------------
int sceCtrlSetSamplingMode()
{
   return 20;
}
//---------------------------------------------------------------------------
int sceCtrlReadBufferPositive()
{
   int res,count,i;
   unsigned long adr;
   psp_thread *p;
   int status;

   count = (int)R4000.reg[5];
   adr = R4000.reg[4];
   for(i=res=0;i<count;i++){
       adr = psp.read_controller(adr,0);
       res += 50;
   }
   R4000.reg[2] = count;
   p = psp.get_current_thread();
   if(p != NULL){
       status = psp.get_Status() & 1;
       p->wait(4,(void *)status,0);
   }

   return res;
}
//---------------------------------------------------------------------------
int sceCtrlPeekBufferPositive()
{
   int res,count,i;
   unsigned long adr;

   count = (int)R4000.reg[5];
   adr = R4000.reg[4];
   for(i=res=0;i<count;i++){
       adr = psp.read_controller(adr,1);
       res += 50;
   }
   R4000.reg[2] = count;
   return res;
}
//---------------------------------------------------------------------------
int sceKernelCreateCallback()
{
   int i;

   R4000.reg[2] = psp.create_callback(R4000.reg[4],R4000.reg[5],R4000.reg[6]);
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelRegisterExitCallback()
{
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelAllocPartitionMemory()
{
   R4000.reg[2] = psp.create_partition(R4000.reg[4],R4000.reg[5],R4000.reg[6],R4000.reg[7],R4000.reg[8]);
   return 100;
}
//---------------------------------------------------------------------------
int sceKernelGetBlockHeadAddr()
{
   LPMEM_PARTITION p;

   p = psp.get_partition(R4000.reg[4]);
   if(p != NULL){
       R4000.reg[2] = p->addr;
   }
   else
       R4000.reg[2] = 0;
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelMaxFreeMemSize()
{
   R4000.reg[2] = psp.get_maxfreemem();
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelTotalFreeMemSize()
{
   R4000.reg[2] = psp.get_freemem();
   return 50;
}
//---------------------------------------------------------------------------
int sceIoOpen()
{
   R4000.reg[2] = psp.psp_open_file(R4000.reg[4],R4000.reg[5],R4000.reg[6]);
   return 300;
}
//---------------------------------------------------------------------------
int sceIoLseek32()
{
   LPPSP_FILE p;

   p = psp.get_file(R4000.reg[4]);
   if(p == NULL || !p->pFile->IsOpen()){
       R4000.reg[2] = (DWORD)-1;
       return 100;
   }
   R4000.reg[2] = p->pFile->Seek(R4000.reg[5],R4000.reg[6]);
   return 100;
}
//---------------------------------------------------------------------------
int sceIoLseek()
{
   LPPSP_FILE p;

   p = psp.get_file(R4000.reg[4]);
   if(p == NULL || !p->pFile->IsOpen()){
       R4000.reg[2] = (DWORD)-1;
       R4000.reg[3] = (DWORD)-1;
       return 100;
   }
   R4000.reg[2] = p->pFile->Seek(R4000.reg[6],R4000.reg[8]);
   R4000.reg[3] = 0;
   return 100;
}
//---------------------------------------------------------------------------
int sceIoRead()
{
   LPPSP_FILE p;
   unsigned char *buffer;
   unsigned long size,l,adr;

   p = psp.get_file(R4000.reg[4]); //890bffc
   if(p == NULL){
       R4000.reg[2] = (DWORD)-1;
       return 100;
   }
   size = R4000.reg[6];
   buffer = (unsigned char *)LocalAlloc(LMEM_FIXED,size);
   if(buffer == NULL){
       R4000.reg[2] = (DWORD)-1;
       return 100;
   }
   size = p->pFile->Read(buffer,size);
   adr = R4000.reg[5];
   for(l=0;l<size;l++)
       write_byte(adr++,buffer[l]);
   LocalFree(buffer);
   R4000.reg[2] = size;
   return 200;
}
//---------------------------------------------------------------------------
int sceIoWrite()
{
   LPPSP_FILE p;
   unsigned char *buffer;
   unsigned long size,l,adr;

   p = psp.get_file(R4000.reg[4]); //890bffc
   if(p == NULL){
       R4000.reg[2] = (DWORD)-1;
       return 100;
   }
   buffer = &mem[R4000.reg[5] - 0x08000000];
   R4000.reg[2] = p->pFile->Write(buffer,R4000.reg[6]);
   return 200;
}
//---------------------------------------------------------------------------
int sceIoClose()
{
   R4000.reg[2] = psp.psp_close_file(R4000.reg[4]);
   return 200;
}
//---------------------------------------------------------------------------
int sceIoRemove()
{
   LString s,s1;
   unsigned long adr;
   char c;

   adr = R4000.reg[4];
   s = "";
   do{
       c = (char)read_byte(adr++);
       if(c == 0)
           break;
       if(c == '/')
           c = '\\';
       s += (char)c;
   }while(true);
   if(s[1] == '\\')
       s = s.SubString(2,s.Length()-1);
   s1 = ".\\";
   s1 += s;
   DeleteFile(s1.c_str());
   R4000.reg[2] = 1;
   return 300;
}
//---------------------------------------------------------------------------
int sceIoDopen()
{
   R4000.reg[2] = psp.psp_open_dir(R4000.reg[4]);
   return 100;
}
//---------------------------------------------------------------------------
int sceIoDread()
{
   PSP_DIR *p;
   SceIoDirent *p1;

   p = psp.get_dir(R4000.reg[4]);
   if(p != NULL && p->status){
       p1 = (SceIoDirent *)&mem[R4000.reg[5]-0x08000000];
       lstrcpyn(p1->d_name,p->wf.cFileName,255);
       p->status = FindNextFile(p->handle,&p->wf);
       R4000.reg[2] = 1;
   }
   else
       R4000.reg[2] = (DWORD)-1;
   return 100;
}
//---------------------------------------------------------------------------
int sceIoDclose()
{
   psp.psp_close_dir(R4000.reg[4]);
   R4000.reg[2] = 1;
   return 100;
}
//---------------------------------------------------------------------------
int sceIoGetstat()
{
   LString s,s1;
   unsigned long adr;
   WIN32_FILE_ATTRIBUTE_DATA wf;
   char c;
   SceIoStat *p;
   SYSTEMTIME st;

   adr = R4000.reg[4];
   if(adr == 0){
       R4000.reg[2] = (DWORD)-1;
       return 200;
   }
   s = "";
   do{
       c = (char)read_byte(adr++);
       if(c == 0)
           break;
       if(c == '/')
           c = '\\';
       s += (char)c;
   }while(true);
   if(s[1] == '\\')
       s = s.SubString(2,s.Length()-1);
   s1 = ".\\";
   s1 += s;
   if(!GetFileAttributesEx(s1.c_str(),GetFileExInfoStandard,&wf)){
       R4000.reg[2] = (DWORD)-1;
       return 200;
   }
   p = (SceIoStat *)&mem[R4000.reg[5]- 0x08000000];
   p->st_attr = 0x26;
   p->st_mode = 0x1DE;

   FileTimeToSystemTime(&wf.ftCreationTime,&st);
   p->st_ctime.year = st.wYear;
   p->st_ctime.month = st.wMonth;
   p->st_ctime.day = st.wDay;
   p->st_ctime.hour = st.wHour;
   p->st_ctime.minute = st.wMinute;
   p->st_ctime.second = st.wSecond;
   p->st_ctime.microsecond = st.wMilliseconds * 1000;;

   FileTimeToSystemTime(&wf.ftLastAccessTime,&st);
   p->st_atime.year = st.wYear;
   p->st_atime.month = st.wMonth;
   p->st_atime.day = st.wDay;
   p->st_atime.hour = st.wHour;
   p->st_atime.minute = st.wMinute;
   p->st_atime.second = st.wSecond;
   p->st_atime.microsecond = st.wMilliseconds * 1000;;

   FileTimeToSystemTime(&wf.ftLastWriteTime,&st);
   p->st_mtime.year = st.wYear;
   p->st_mtime.month = st.wMonth;
   p->st_mtime.day = st.wDay;
   p->st_mtime.hour = st.wHour;
   p->st_mtime.minute = st.wMinute;
   p->st_mtime.second = st.wSecond;
   p->st_mtime.microsecond = st.wMilliseconds * 1000;;

   p->st_size = wf.nFileSizeLow;

   R4000.reg[2] = 1;
   return 200;
}
//---------------------------------------------------------------------------
int sceKernelLibcGettimeofday()
{
   unsigned long adr;
   U64 t;

   adr = R4000.reg[4];
   if(adr != 0){
//       t = start_time + (ticks / 33);
       GetSystemTimeAsFileTime((FILETIME *)&t);
       write_dword(adr+4,t);
       write_dword(adr,(t>>32));
   }
   R4000.reg[2] = 0;
   return 50;
}
//---------------------------------------------------------------------------
int sceRtcGetCurrentTick()
{
   U64 value;

   value = ticks;
   ::write_dword(R4000.reg[4]+4,(unsigned long)value); //1ca0
   ::write_dword(R4000.reg[4],(unsigned long)(value >> 32));
   R4000.reg[2] = 0;
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelLibcClock()
{
   R4000.reg[2] = ticks / 333000;
}
//---------------------------------------------------------------------------
int sceRtcGetTickResolution()
{
   R4000.reg[2] = 333000;
    return 50;
}
//---------------------------------------------------------------------------
int sceHprmPeekCurrentKey()
{
   ::write_dword(R4000.reg[4],0);
   R4000.reg[2] = 0;
   return 50;
}
//---------------------------------------------------------------------------
int sceAudioChReserve()
{
   R4000.reg[2] = psp.allocate_channel(R4000.reg[4],R4000.reg[5],R4000.reg[6]);
   return 100;
}
//---------------------------------------------------------------------------
int sceAudioOutputPannedBlocking()
{
   psp_thread *p;
   LPPSP_AUDIO_CHANNEL p1;

   p = psp.get_current_thread();
   p1 = psp.get_channel(R4000.reg[4]);
   if(p == NULL || p1 == NULL){
       R4000.reg[2] = (DWORD)-1;
       return 100;
   }
   p->wait(2,NULL,(p1->samples / 44.1f) * 1000.0f,NULL);

   return 100;
}
//---------------------------------------------------------------------------
int sceKernelDcacheWritebackAll()
{
   return 20;
}
//---------------------------------------------------------------------------
int sceKernelLibcTime()
{
   time_t value;

   R4000.reg[2] = time(&value);
   return 50;
}
//---------------------------------------------------------------------------
int sceKernelStdout()
{
   R4000.reg[2] = 0x07000001;
    return 20;
}
//---------------------------------------------------------------------------
int sceKernelStderr()
{
   R4000.reg[2] = 0x07000002;
    return 20;
}
//---------------------------------------------------------------------------
int sceKernelUtilsMt19937UInt()
{
   R4000.reg[2] = rand();
   return 100;

}

