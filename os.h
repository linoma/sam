#include <windows.h>
#include "llist.h"
#include "lvec.hpp"
#include "sce.h"
#include "fstream.h"
#include "lmmu.h"

//---------------------------------------------------------------------------
#ifndef __osH__
#define __osH__
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
typedef struct {
   unsigned long opcode;
   unsigned long pc,next_pc;
   unsigned long hi,lo;
   unsigned long ic;
   int jump,stop;
   unsigned long reg[32];
   float sp_reg[32];
   float vfpu_reg[128];
   BOOL cc;
   unsigned long vfpu_cr[16];
} CPU_BASE,*LPCPU_BASE;
//---------------------------------------------------------------------------
typedef struct
{
   int res_id;
   char name[33];
} PSP_OBJECT_BASE,*LPPSP_OBJECT_BASE;
//---------------------------------------------------------------------------
class psp_object_list : public LList
{
public:
   psp_object_list();
   ~psp_object_list();
   virtual void *get_ObjectFromID(int value);
   virtual int Add(void *object);
   void Clear(){LList::Clear();id=1;};
protected:
   int id;
};
//---------------------------------------------------------------------------
class psp_thread;
typedef struct
{
   PSP_OBJECT_BASE hdr;
   int attr;
   int bits;
   unsigned long opts;
   psp_thread *thread;
} PSP_EVENT,*LPPSP_EVENT;
//---------------------------------------------------------------------------
typedef struct
{
   PSP_OBJECT_BASE hdr;
   int max_value;
   int init_value;
   int current_value;
} PSP_SEMAPHORE,*LPPSP_SEMAPHORE;
//---------------------------------------------------------------------------
typedef struct
{
   PSP_OBJECT_BASE hdr;
   unsigned long adr;
} PSP_MODULE,*LPPSP_MODULE;
//---------------------------------------------------------------------------
typedef struct
{
   unsigned long adr;
	unsigned long size;
} MEM_SEGMENT,*LPMEM_SEGMENT;
//---------------------------------------------------------------------------
typedef struct
{
   PSP_OBJECT_BASE hdr;
   unsigned long id;
	unsigned long addr;
	unsigned long raddr;
	int size;
	unsigned long type;
} MEM_PARTITION,*LPMEM_PARTITION;
//---------------------------------------------------------------------------
typedef struct
{
   PSP_OBJECT_BASE hdr;
   int id;
	unsigned long adr;
	unsigned long arg;
} PSP_CALLBACK,*LPPSP_CALLBACK;
//---------------------------------------------------------------------------
typedef struct
{
   PSP_OBJECT_BASE hdr;
   LFile *pFile;
} PSP_FILE,*LPPSP_FILE;
//---------------------------------------------------------------------------
typedef struct
{
   PSP_OBJECT_BASE hdr;
   HANDLE handle;
   WIN32_FIND_DATA wf;
   BOOL status;
} PSP_DIR,*LPPSP_DIR;
//---------------------------------------------------------------------------
typedef struct _GPU_LIST
{
public:
   void read_color();
   void move_index();
   void get_value(int size,int len,float *v);

	unsigned long adr;
   unsigned long stall_adr;
   unsigned long base;
   unsigned long vertex_adr;
   unsigned long index_adr;
   unsigned long arg;
   unsigned long callback_id;
   unsigned char end,stalled,draw;
   unsigned char texture,color,normal,position,weight,index;
   unsigned char skinningWeightCount,morphingVertexCount,transform2D;
   unsigned long vertex_size;
   unsigned long mem_adr;
   LVector<unsigned long> stack;
   float col[4];
} GPU_LIST,*LPGPU_LIST;

//---------------------------------------------------------------------------
typedef struct
{
   SceCtrlData data;
   int mode;
} CONTROLLER,*LPCONTROLLER;
//---------------------------------------------------------------------------
class gpu_list_list : public LList
{
public:
   gpu_list_list();
   ~gpu_list_list();
protected:
   void DeleteElem(LPVOID p);
};
//---------------------------------------------------------------------------
typedef struct
{
   int type;
   void *object;
   unsigned long sleep_ticks;
   unsigned long reserved[4];
} WAIT_OBJECT,*LPWAIT_OBJECT;
//---------------------------------------------------------------------------
class psp_thread : public LList
{
public:
   psp_thread(const char *name,unsigned long pc,unsigned long s_size = 0x10000,int prio = 0x18);
   ~psp_thread();
   void Save(LPCPU_BASE p);
   void Restore(LPCPU_BASE p);
   void Suspend();
   void Resume();
   void Sleep(int mode = 0);
   void Start();
   void wait(int object,void *p,unsigned long timeout,...);
   BOOL check_wait(int value = 0);
   //Variables
   CPU_BASE base;
   unsigned long status,time_slice,stack_size,entry,max_time_slice;
   unsigned __int64 stop_ticks;
   int priority;
   char name[33];
   int res_id;
protected:
   void DeleteElem(LPVOID p);
};
//---------------------------------------------------------------------------
class psp_thread_list : public psp_object_list
{
public:
   psp_thread_list();
   ~psp_thread_list();
   psp_thread *switch_thread();
   void active_thread(DWORD index);
   inline psp_thread *get_current_thread(){return current;};
   void reset();
   void *get_ObjectFromID(int value);
   int Add(void *object);
   inline void set_current_index(DWORD value){dwCurrentIndex=(DWORD)-1;};
   void check_wait_condition(int type,void *object);
protected:
   void DeleteElem(void *p);
   psp_thread *current;
   DWORD dwCurrentIndex;
};
//---------------------------------------------------------------------------
class psp_segment_list : public LList
{
public:
   psp_segment_list();
   ~psp_segment_list();
   LPMEM_SEGMENT alloc(unsigned long size);
   void free(unsigned long adr);
   void use(unsigned long adr,unsigned long size);
   psp_segment_list *get_free_segments();
};
//---------------------------------------------------------------------------
class psp_file_list : public psp_object_list
{
public:
   psp_file_list();
   ~psp_file_list();
protected:
   void DeleteElem(void *p);
};
//---------------------------------------------------------------------------
class psp_dir_list : public psp_object_list
{
public:
   psp_dir_list() : psp_object_list() {};
   ~psp_dir_list(){Clear();};
protected:
   void DeleteElem(void *p){if(p != NULL) FindClose(p);};
};

//---------------------------------------------------------------------------
class LOS
{
public:
   LOS();
   ~LOS();
   BOOL Init();
   void Reset();
   int IndexOfFunc(DWORD nid);
   int ExecuteFuncs(int index);
   int sys_call(DWORD sid);
   DWORD LoadFile(char *fileName,DWORD *pgp);
   BOOL get_ModuleSection(LFile *pFile,Elf32_Ehdr **elf_hdr,PBP_Header **header,Elf32_Shdr **secs,int *nsecs);
   DWORD LoadModule(LFile *pFile,Elf32_Shdr *shdrs,PBP_Header *header,Elf32_Ehdr *elf_hdr,DWORD baseAddress,DWORD *pgp);
   
   inline psp_thread *switch_thread(){return threads_list.switch_thread();};
   int create_thread(const char *name,unsigned long pc,unsigned long s_size = 0x10000,int prio = 0x18);
   int start_thread(unsigned long index,int len,unsigned long mem);
   void delete_thread(int index);
   psp_thread *get_thread(unsigned long index){return (psp_thread *)threads_list.get_ObjectFromID(index);};
   psp_thread *get_current_thread(){return threads_list.get_current_thread();};
   psp_thread_list *get_threads(){return &threads_list;};
   inline void reset_thread_index(){threads_list.set_current_index(0);};

   unsigned long alloc_mem(unsigned long size);
   void free_mem(unsigned long adr);
   void use_mem(unsigned long adr,unsigned long size);
   unsigned long get_freemem();
   unsigned long get_maxfreemem();

   int create_callback(unsigned long name,unsigned long adr,unsigned long arg);

   int create_partition(unsigned long id,unsigned long name,int type, unsigned long size,unsigned long base);
   LPMEM_PARTITION get_partition(unsigned long id){return (LPMEM_PARTITION)partition_list.get_ObjectFromID(id);};

   int psp_open_file(unsigned long adr,unsigned long flags,unsigned long mode);
   LPPSP_FILE get_file(unsigned long index){return (LPPSP_FILE)files_list.get_ObjectFromID(index);};
   int psp_close_file(unsigned long index);

   int psp_open_dir(unsigned long adr);
   LPPSP_DIR get_dir(unsigned long index){return (LPPSP_DIR)dir_list.get_ObjectFromID(index);};
   int psp_close_dir(int fd);

   int psp_create_sema(unsigned long name,unsigned long attr,int initValue,int maxValue,unsigned long arg);
   LPPSP_SEMAPHORE get_sema(unsigned long index){return (LPPSP_SEMAPHORE)sema_list.get_ObjectFromID(index);};
   int psp_delete_sema(unsigned long index);

   int psp_create_event(unsigned long name,int attr,int bits,unsigned long opts);
   LPPSP_EVENT get_event(unsigned long index){return (LPPSP_EVENT)event_list.get_ObjectFromID(index);};

   int psp_load_module(unsigned long name,int flags,unsigned long opt);

   void check_wait_condition(int type,void *object){threads_list.check_wait_condition(type,object);};
protected:
   psp_thread_list threads_list;
   psp_segment_list segments_list;
   psp_object_list partition_list;
   psp_object_list callback_list;
   psp_file_list files_list;
   psp_object_list sema_list;
   psp_object_list event_list;
   psp_object_list module_list;
   psp_dir_list dir_list;
};
#endif
