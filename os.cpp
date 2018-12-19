#include "os.h"
#include "lzari.h"
#include "resource.h"
#include "lstring.h"
#include <stdio.h>
#include <time.h>
#include "lpsp.h"
#include "syscall.h"

extern CPU_BASE R4000;
extern U64 ticks;
//---------------------------------------------------------------------------
LOS::LOS()
{
}
//---------------------------------------------------------------------------
LOS::~LOS()
{
   Reset();
}
//---------------------------------------------------------------------------
void LOS::Reset()
{
   threads_list.reset();
   segments_list.Clear();
   callback_list.Clear();
   partition_list.Clear();
   files_list.Clear();
   dir_list.Clear();
   sema_list.Clear();
   event_list.Clear();
   module_list.Clear();
}
//---------------------------------------------------------------------------
BOOL LOS::Init()
{
    return TRUE;
}
//---------------------------------------------------------------------------
int LOS::IndexOfFunc(DWORD nid)
{
    int i;

    for(i=0;i<sizeof(syscall_func) / sizeof(SO_FUNC);i++){
        if(syscall_func[i].nid == nid)
           return i;
    }
    return -1;
}
//---------------------------------------------------------------------------
int LOS::ExecuteFuncs(int index)
{
   LPSO_FUNC p;     

   p = &syscall_func[index];
   if(p != NULL && p->pfn != NULL)
       return p->pfn();
   return 0;
}
//---------------------------------------------------------------------------
int LOS::sys_call(DWORD sid)
{
    int i;

    for(i=0;i<sizeof(syscall_func) / sizeof(SO_FUNC);i++){
        if(syscall_func[i].sid == sid){
           if(syscall_func[i].pfn != NULL)
               return syscall_func[i].pfn();
           break;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
static int sortThreads(void *a0,void *a1)
{
   psp_thread *p,*p1;

   p = (psp_thread *)a0;
   p1 = (psp_thread *)a1;
   if(p->priority > p1->priority)
       return 1;
   else if(p->priority < p1->priority)
       return -1;
   return 0;
}
//---------------------------------------------------------------------------
int LOS::create_thread(const char *name,unsigned long pc,unsigned long s_size,int prio)
{
   psp_thread *p;

   p = new psp_thread(name,pc,s_size,prio);
   if(p == NULL)
       return -1;
   if(threads_list.Add((LPVOID)p) < 0){
       delete p;
       return -1;
   }
   return p->res_id;
}
//---------------------------------------------------------------------------
int LOS::start_thread(unsigned long index,int len,unsigned long mem)
{
   psp_thread *p;
   int i;

   p = (psp_thread *)threads_list.get_ObjectFromID(index);
   if(p == NULL)
       return -1;
   p->Start();
   if(len != 0){
       p->base.reg[5] = p->base.reg[29];
       for(i = 0;i<len;i++)
           write_byte(p->base.reg[5]+i,read_byte(mem+i));
   }
   p->base.reg[28] = R4000.reg[28];
   threads_list.Sort(0,threads_list.Count(),sortThreads);
//   threads_list.get_current_thread()->time_slice = MAX_TIMESLICE;
   return 1;
}
//---------------------------------------------------------------------------
void LOS::delete_thread(int index)
{
   psp_thread *p;

   p = (psp_thread *)threads_list.get_ObjectFromID(index);               //89f55dc
   if(p != NULL)
       p->status |= 2;
}
//---------------------------------------------------------------------------
unsigned long LOS::alloc_mem(unsigned long size)
{
   LPMEM_SEGMENT p;

   p = segments_list.alloc(size);
   if(p != NULL)
       return p->adr;
   return 0;
}
//---------------------------------------------------------------------------
void LOS::free_mem(unsigned long adr)
{
   segments_list.free(adr);
}
//---------------------------------------------------------------------------
void LOS::use_mem(unsigned long adr,unsigned long size)
{
   segments_list.use(adr,size);
}
//---------------------------------------------------------------------------
unsigned long LOS::get_freemem()
{
   unsigned long res;
   DWORD dwPos;
   LPMEM_SEGMENT p;

   res = 32*1024*104;
   p = (LPMEM_SEGMENT)segments_list.GetFirstItem(&dwPos);
   while(p != NULL){
       res -= p->size;
       p = (LPMEM_SEGMENT)segments_list.GetNextItem(&dwPos);
   }
   return res;
}
//---------------------------------------------------------------------------
unsigned long LOS::get_maxfreemem()
{
   psp_segment_list *unused;
   unsigned long res;
   DWORD dwPos;
   LPMEM_SEGMENT p;

   unused = segments_list.get_free_segments();
   if(unused == NULL)
       return 0;
   res = 0;
   p = (LPMEM_SEGMENT)unused->GetFirstItem(&dwPos);
   while(p != NULL){
       if(res < p->size)
           res = p->size;
       p = (LPMEM_SEGMENT)unused->GetNextItem(&dwPos);
   }
   delete unused;
   return res;
}
//---------------------------------------------------------------------------
int LOS::create_callback(unsigned long name,unsigned long adr,unsigned long arg)
{
   LPPSP_CALLBACK p;
   char c;
   int i;

   p = new PSP_CALLBACK[1];
   if(p == NULL)
       return -1;
   ZeroMemory(p,sizeof(PSP_CALLBACK));
   if(name != 0){
       c = 1;
       for(i=0;i<60 && c != 0;i++){
           c = read_byte(name++);
           p->hdr.name[i] = c;
       }
   }
   p->adr = adr;
   p->arg = arg;
   if((i = callback_list.Add((LPVOID)p)) < 0){
       delete p;
       return -1;
   }
   return i;
}
//---------------------------------------------------------------------------
int LOS::create_partition(unsigned long id,unsigned long name,int type, unsigned long size,unsigned long base)
{
   MEM_PARTITION *p;
   int i;
   char c;

   p = new MEM_PARTITION[1];
   if(p == NULL)
       return -1;
   ZeroMemory(p,sizeof(MEM_PARTITION));
   if(name != 0){
       c = 1;
       for(i=0;i<60 && c != 0;i++){
           c = read_byte(name++);
           p->hdr.name[i] = c;
       }
   }
   p->type = type;
   p->id = id;
   p->addr = alloc_mem(size);
   p->size = size;
   if((i = partition_list.Add((LPVOID)p)) < 0){
       delete p;
       return -1;
   }
   return i;
}
//---------------------------------------------------------------------------
int LOS::psp_open_dir(unsigned long adr)
{
   LString s,s1;
   char c;
   PSP_DIR *p;
   int i;
   
   if(adr == 0)
       return -1;
   s = "";
   do{
       c = (char)read_byte(adr++);
       if(c == 0)
           break;
       if(c == '/')
           c = '\\';
       s += (char)c;
   }while(true);
   p = new PSP_DIR[1];
   if(p == NULL)
       return -1;
   if(s[1] == '\\')
       s = s.SubString(2,s.Length()-1);
   else if((i = s.Pos(":")) > 0){
       s = s.SubString(i+2,s.Length()-i-1);
   }
   s1 = ".\\";
   if(s.Length() < 1)
       s += "*.*";
   s1 += s;
   p->handle = FindFirstFile(s1.c_str(),&p->wf);
   if(p->handle == INVALID_HANDLE_VALUE){
       delete p;
       return -1;
   }
   p->status = TRUE;
   if(dir_list.Add(p) < 0){
       FindClose(p->handle);
       delete p;
       return -1;
   }
   return p->hdr.res_id;
}
//---------------------------------------------------------------------------
int LOS::psp_close_dir(int fd)
{
   PSP_DIR *p;
   int res;

   res = -1;
   p = get_dir(fd);
   if(p == NULL)
       return res;
   if(p->handle != INVALID_HANDLE_VALUE){
       FindClose(p->handle);
       res = 0;
   }
   dir_list.Delete(dir_list.IndexFromEle(p),TRUE);
   return res;
}
//---------------------------------------------------------------------------
int LOS::psp_open_file(unsigned long adr,unsigned long flags,unsigned long mode)
{
   LString s,s1;
   char c;
   LFile *pFile;
   DWORD dwStyle,dwCreation;
   int i;
   LPPSP_FILE p;

   if(adr == 0)
       return -1;
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
   else if((i = s.Pos(":")) > 0){
       s = s.SubString(i+2,s.Length()-i-1);
   }
   s1 = ".\\";
   s1 += s;
   p = new PSP_FILE[1];
   if(p == NULL)
       return -1;
   pFile = new LFile(s1.c_str());
   if(pFile == NULL){
       delete p;
       return -1;
   }
   dwStyle = 0;
   if(flags & 1)
       dwStyle |= GENERIC_READ;
   if(flags & 2)
       dwStyle |= GENERIC_WRITE;
   dwCreation = 0;
   if(flags & 0x200)
       dwCreation = CREATE_ALWAYS;
   else
       dwCreation = OPEN_EXISTING;
   if(!pFile->Open(dwStyle,dwCreation)){
       delete pFile;
       delete p;
       return -1;
   }
   if((i = files_list.Add(p)) < 0){
       delete pFile;
       delete p;
       return -1;
   }
   p->pFile = pFile;
   return i;
}
//---------------------------------------------------------------------------
int LOS::psp_close_file(unsigned long index)
{
   LPPSP_FILE p;

   p = (LPPSP_FILE)files_list.get_ObjectFromID(index);
   if(p == NULL || p->pFile == NULL)
       return -1;
   p->pFile->Close();
   files_list.Delete(files_list.IndexFromEle(p),TRUE);
   return 1;
}
//---------------------------------------------------------------------------
psp_object_list::psp_object_list() : LList()
{
   id = 1;
}
//---------------------------------------------------------------------------
psp_object_list::~psp_object_list()
{
    Clear();
}
//---------------------------------------------------------------------------
void *psp_object_list::get_ObjectFromID(int value)
{
   elem_list *tmp;

   tmp = First;
   while(tmp != NULL){
       if(((LPPSP_OBJECT_BASE)tmp->Ele)->res_id == value)
           return tmp->Ele;
       tmp = tmp->Next;
   }
   return NULL;
}
//---------------------------------------------------------------------------
int psp_object_list::Add(void *object)
{
   if(!LList::Add(object))
       return -1;
   ((LPPSP_OBJECT_BASE)object)->res_id = id;
   return id++;
}
//---------------------------------------------------------------------------
psp_thread::psp_thread(const char *n,unsigned long pc,unsigned long s_size,int prio) : LList()
{
   ZeroMemory(&base,sizeof(CPU_BASE));
   if(n != NULL)
       lstrcpy(name,n);
   entry = base.pc = pc;
   stack_size = s_size;
   priority = prio;
   time_slice = 0;
   status = 0;
   max_time_slice = MAX_TIMESLICE * (64 - prio) / 32;   
}
//---------------------------------------------------------------------------
psp_thread::~psp_thread()
{
   Clear();
}
//---------------------------------------------------------------------------
void psp_thread::DeleteElem(LPVOID p)
{
   delete (LPWAIT_OBJECT)p;
}
//---------------------------------------------------------------------------
void psp_thread::Save(LPCPU_BASE p)
{
   CopyMemory(&base,p,sizeof(CPU_BASE));
}
//---------------------------------------------------------------------------
void psp_thread::Restore(LPCPU_BASE p)
{
   CopyMemory(p,&base,sizeof(CPU_BASE));
}
//---------------------------------------------------------------------------
void psp_thread::Suspend()
{
   status |= 0x10;
   time_slice = max_time_slice;
}
//---------------------------------------------------------------------------
void psp_thread::Resume()
{
   status &= ~0x10;
   time_slice = 0;
}
//---------------------------------------------------------------------------
void psp_thread::Start()
{
   unsigned long size;

   if(!(status & 1)){
       size = stack_size;
       if(size & 3)
           size = ((size >> 2) + 1) << 2;
       base.reg[29] = psp.alloc_mem(size);
       base.reg[29] += size;
       base.reg[31] = 0x08000000;
       status |= 1;
   }
}
//---------------------------------------------------------------------------
void psp_thread::Sleep(int mode)
{
   status |= 4;
   time_slice = max_time_slice;   
}
//---------------------------------------------------------------------------
void psp_thread::wait(int type,void *p,unsigned long timeout,...)
{
    LPWAIT_OBJECT p1;
    DWORD dwPos;
    va_list ap;
    unsigned long arg,arg0,arg1,clear;
    int i;

    p1 = (LPWAIT_OBJECT)GetFirstItem(&dwPos);
    while(p1 != NULL){
        if(p1->type == type){
            if(p1->object == p)
                return;
        }
        p1 = (LPWAIT_OBJECT)GetNextItem(&dwPos);
    }
    va_start(ap,timeout);
    switch(type){
        case 1:
            if(((LPPSP_SEMAPHORE)p)->current_value > 0){
                ((LPPSP_SEMAPHORE)p)->current_value--;
                R4000.reg[2] = 0;
                status &= ~8;
                return;
            }
        break;
        case 3:
            clear = 0;
            arg0 = va_arg(ap,unsigned long);
            arg1 = va_arg(ap,unsigned long);
            switch(arg1 & 1){
                case 0://AND
                    if((((LPPSP_EVENT)p)->bits & arg0) == arg0)
                        clear = arg0;
                break;
                case 1:
                    if((((LPPSP_EVENT)p)->bits & arg0))
                        clear = ((LPPSP_EVENT)p)->bits & arg0;
                break;
            }
            if(clear){
                if(arg1 & 0x20)
                    ((LPPSP_EVENT)p)->bits &= ~clear;
                status &= ~8;
                R4000.reg[2] = 0;
                return;
            }
        break;
    }
    va_start(ap,timeout);
    status |= 0x8;
    stop_ticks = ticks;
    p1 = new WAIT_OBJECT[1];
    p1->type = type;
    p1->sleep_ticks = timeout*333;
    p1->object = p;
    for(i=0;(arg = va_arg(ap,unsigned long)) != 0 && i < 4;i++)
        p1->reserved[i] = arg;
    va_end(ap);
    Add(p1);
    time_slice = max_time_slice;
}
//---------------------------------------------------------------------------
BOOL psp_thread::check_wait(int value)
{
   LPWAIT_OBJECT p1;
   DWORD dwPos;
   LPPSP_SEMAPHORE sema;
   LPPSP_EVENT event;
   unsigned long clear;

   p1 = (LPWAIT_OBJECT)GetFirstItem(&dwPos);
   while(p1 != NULL){
       if(value == 0 || (value != 0 && p1->type == value)){
           switch(p1->type){
               case 1:
                   sema = (LPPSP_SEMAPHORE)p1->object;
                   if(sema != NULL && sema->current_value > 0){
                       sema->current_value--;
                       Delete(p1,TRUE);
                       time_slice = 0;
                       status &= ~0x8;
                       base.reg[2] = 0;
                       return TRUE;
                   }
                   break;
               case 2:
                   if((ticks-stop_ticks) > (unsigned long)p1->sleep_ticks){
                       Delete(p1,TRUE);
                       time_slice = 0;
                       status &= ~0x8;
                       base.reg[2] = 0;
                       return TRUE;
                   }
                   break;
               case 3://Event
                   event = (LPPSP_EVENT)p1->object;
                   clear = 0;
                   switch(p1->reserved[1] & 1){
                       case 0://AND
                           if((event->bits & p1->reserved[0]) == p1->reserved[0]){
                               clear = p1->reserved[0];
                           }
                       break;
                       case 1:
                           if((event->bits & p1->reserved[0])){
                               clear = event->bits & p1->reserved[0];
                           }
                       break;
                   }
                   if(clear){
                       if(p1->reserved[1] & 0x20){
                           event->bits &= ~clear;
                       }
                       Delete(p1,TRUE);
                       time_slice = 0;
                       status &= ~0x8;
                       base.reg[2] = 0;
                       return TRUE;
                   }
                   if(p1->sleep_ticks != 0){
                       if((ticks-stop_ticks) > (unsigned long)p1->sleep_ticks){
                           Delete(p1,TRUE);
                           time_slice = 0;
                           status &= ~0x8;
                           base.reg[2] = 0;
                           return TRUE;
                       }
                   }
               break;
               case 4:
                   clear = (int)p1->object;
                   if(clear){
                       if((psp.get_Status() & 1) == 0)
                           p1->object = 0;
                   }
                   else{
                       if((psp.get_Status() & 1)){
                           Delete(p1,TRUE);
                           time_slice = 0;
                           status &= ~0x8;
                           return TRUE;
                       }
                   }
               break;
               default:
               break;
           }
       }
       p1 = (LPWAIT_OBJECT)GetNextItem(&dwPos);
   }
   return FALSE;
}
//---------------------------------------------------------------------------
psp_thread_list::psp_thread_list() : psp_object_list()
{
   reset();
}
//---------------------------------------------------------------------------
psp_thread_list::~psp_thread_list()
{
    Clear();
}
//---------------------------------------------------------------------------
void *psp_thread_list::get_ObjectFromID(int value)
{
   elem_list *tmp;

   tmp = First;
   while(tmp != NULL){
       if(((psp_thread *)tmp->Ele)->res_id == value)
           return tmp->Ele;
       tmp = tmp->Next;
   }
   return NULL;
}
//---------------------------------------------------------------------------
int psp_thread_list::Add(void *object)
{
   if(!LList::Add(object))
       return -1;
   ((psp_thread *)object)->res_id = id;
   return id++;
}
//---------------------------------------------------------------------------
void psp_thread_list::reset()
{
   Clear();
   current = NULL;
   dwCurrentIndex = (DWORD)-1;   
}
//---------------------------------------------------------------------------
void psp_thread_list::check_wait_condition(int type,void *object)
{
   psp_thread *p;
   DWORD dwPos;
   
   p = (psp_thread *)GetFirstItem(&dwPos);
   while(p != NULL){
       p->check_wait(type);
       p = (psp_thread *)GetNextItem(&dwPos);
   }
}
//---------------------------------------------------------------------------
psp_thread *psp_thread_list::switch_thread()
{
   psp_thread *p;
   DWORD dw,dw1;

   p = current;
   if(p == NULL){
       if(First == NULL)
           return NULL;
       p = (psp_thread *)First->Ele;
       dw = 0;
   }
   else{
       dw = ++dwCurrentIndex;
       if(dw >= nCount)
           dw = 0;
       p = (psp_thread *)GetItem(dw+1);
   }
   dw1 = dw;
   while(p != NULL){
       if((p->status & 0x1F) == 1){
           current = p;
           dwCurrentIndex = dw;
           return p;
       }
       else if((p->status & 0x1F) == 0x9){
           if(p->check_wait()){
               current = p;
               dwCurrentIndex = dw;
               return p;
           }
       }
       p = (psp_thread *)GetItem(++dw + 1);
   }
   for(dw=1;dw<=dw1;dw++){
       p = (psp_thread *)GetItem(dw);
       if((p->status & 0x1F) == 1){
           current = p;
           dwCurrentIndex = dw;
           return p;
       }
       else if((p->status & 0x1F) == 0x9){
           if(p->check_wait()){
               current = p;
               dwCurrentIndex = dw;
               return p;
           }
       }
   }
/*   p = (psp_thread *)GetFirstItem(&dw);
   while(p != NULL){
       if(p->status & 2)
           RemoveInternal(p,TRUE);
       p = (psp_thread *)GetNextItem(&dw);
   }*/
   current = (psp_thread *)GetItem(1);
   dwCurrentIndex = 1;
   return current;
}
//---------------------------------------------------------------------------
void psp_thread_list::active_thread(DWORD index)
{
   psp_thread *p;

   p = (psp_thread *)GetItem(index+1);
   if(p == NULL)
       return;
   current = p;
   dwCurrentIndex = index;
}
//---------------------------------------------------------------------------
void psp_thread_list::DeleteElem(void *p)
{
   delete (psp_thread*)p;
}
//---------------------------------------------------------------------------
psp_segment_list::psp_segment_list() : LList()
{
}
//---------------------------------------------------------------------------
psp_segment_list::~psp_segment_list()
{
   Clear();
}
//---------------------------------------------------------------------------
void psp_segment_list::free(unsigned long adr)
{
   LPMEM_SEGMENT p;
   struct elem_list *ele;

   ele = First;
   while(ele != NULL){
       p = (LPMEM_SEGMENT)ele->Ele;
       if(p->adr == adr){
           RemoveInternal(p,TRUE);
           return;
       }
       ele = ele->Next;
   }
}
//---------------------------------------------------------------------------
static int sortMemSegments(void *a0,void *a1)
{
   return (int)((LPMEM_SEGMENT)a0)->adr - (int)((LPMEM_SEGMENT)a1)->adr;
}
//---------------------------------------------------------------------------
void psp_segment_list::use(unsigned long adr,unsigned long size)
{
   LPMEM_SEGMENT p;
   struct elem_list *ele;

   ele = First;
   while(ele != NULL){
       p = (LPMEM_SEGMENT)ele->Ele;
       if(p->adr == adr){
           if(size > p->size){
               p->size = size;
               return;
           }
       }
       ele = ele->Next;
   }
   p = new MEM_SEGMENT[1];
   if(p != NULL){
       p->adr = adr;
       p->size = size;
   }
   if(!Add((LPVOID)p)){
       delete p;
       return;
   }
   Sort(0,nCount,sortMemSegments);
}
//---------------------------------------------------------------------------
psp_segment_list *psp_segment_list::get_free_segments()
{
   psp_segment_list *unused;
   unsigned long m_s,m_e,m_l;
   LPMEM_SEGMENT p,p1;
   struct elem_list *ele;

   if((unused = new psp_segment_list()) == NULL)
       return NULL;
   m_s = 0x08000000 + 0x100000;
   m_e = 0x09FFFFFF - 0x8FFFF;
   if(First == NULL){
       p1 = new MEM_SEGMENT[1];
       p1->adr = m_s;
       p1->size = m_e-m_s;
       unused->Add((LPVOID)p1);
   }
   else{
       ele = First;
       p = (LPMEM_SEGMENT)ele->Ele;
       p1 = new MEM_SEGMENT[1];
       p1->adr = m_s;
       p1->size = p->adr - m_s;
       unused->Add((LPVOID)p1);
       m_l = p->adr + p->size;
       ele = ele->Next;
       while(ele != NULL){
           p = (LPMEM_SEGMENT)ele->Ele;
           if((p->adr - m_l) > 0){
               p1 = new MEM_SEGMENT[1];
               p1->adr = m_l;
               p1->size = p->adr - m_l;
               unused->Add((LPVOID)p1);
           }
           m_l = p->adr + p->size;
           ele = ele->Next;
       }
   }
   if(m_e > m_l){
       p1 = new MEM_SEGMENT[1];
       p1->adr = m_l;
       p1->size = m_e - m_l;
       unused->Add((LPVOID)p1);
   }
   return unused;
}
//---------------------------------------------------------------------------
LPMEM_SEGMENT psp_segment_list::alloc(unsigned long size)
{
   psp_segment_list *unused;
   LPMEM_SEGMENT p,p1;
   DWORD dwPos;

   unused = get_free_segments();
   if(unused == NULL)
       return NULL;
   p = p1 = NULL;
   p = (LPMEM_SEGMENT)unused->GetFirstItem(&dwPos);
   while(p != NULL){
       if(p->size >= size && p1 == NULL)
           p1 = p;
       else if(p->size >= size && p1->size > p->size)
           p1 = p;
       p = (LPMEM_SEGMENT)unused->GetNextItem(&dwPos);
   }
   if(p1 != NULL){
       p = new MEM_SEGMENT[1];
       p->adr = p1->adr;
       p->size = size;
       Add((LPVOID)p);
       Sort(0,nCount,sortMemSegments);
   }
   else
       p = NULL;
   delete unused;
   return p;
}
//---------------------------------------------------------------------------
gpu_list_list::gpu_list_list() : LList()
{
}
//---------------------------------------------------------------------------
gpu_list_list::~gpu_list_list()
{
   Clear();
}
//---------------------------------------------------------------------------
void gpu_list_list::DeleteElem(LPVOID p)
{
    delete [](LPGPU_LIST)p;
}
//---------------------------------------------------------------------------
void GPU_LIST::read_color()
{
   int n;

   switch(color){
       case 1:
       case 2:
       case 3:
           n = (int)(signed char)read_byte(mem_adr++);
       break;
       case 4:
       case 5:
       case 6:
           n = (int)(signed short)read_word(mem_adr);
           mem_adr += 2;
       break;
       case 7:
           if(mem_adr & 3)
               mem_adr = ((mem_adr >> 2) + 1) << 2;
           n = (int)(signed int)read_dword(mem_adr);
           mem_adr += 4;
       break;
   }
   switch(color){
       case 5:
       break;
       case 6:
       break;
       case 7:
           col[0] = (n & 0xFF) / 255.0f;
           col[1] = ((n >> 8) & 0xFF) / 255.0f;
           col[2] = ((n >> 16) & 0xFF) / 255.0f;
           col[3] = ((n >> 24) & 0xFF) / 255.0f;
       break;
       default:
           col[0] = col[1] = col[2] = 0;
           col[3] = 1;
       break;
   }
}
//---------------------------------------------------------------------------
void GPU_LIST::move_index()
{
   int i;

   switch(index){
       case 0:
//           if(mem_adr & 3)
//               mem_adr = ((mem_adr >> 2) + 1) << 2;
           return;
       case 1:
           i = read_byte(index_adr++);
       break;
       case 2:
           i = read_word(index_adr);
           index_adr += 2;
       break;
       case 3:
           i = read_dword(index_adr);
           index_adr += 4;
       break;
   }
   mem_adr = vertex_adr + i * vertex_size;
}
//---------------------------------------------------------------------------
void GPU_LIST::get_value(int size,int len,float *v)
{
   int n,value;

   switch (size) {
       case 1:
           for (n = 0; n < len; n++)
               v[n] = (float)(signed char)read_byte(mem_adr++);
       break;
       case 2:
           for (n = 0; n < len; n++){
               v[n] = (float)(signed short)read_word(mem_adr);
               mem_adr += 2;
           }
       break;
       case 3:
           for (n = 0; n < len; n++){
               value = (int)read_dword(mem_adr);
               v[n] = *((float *)&value);
               mem_adr += 4;
           }
       break;
   }
}
//---------------------------------------------------------------------------
psp_file_list::psp_file_list() : psp_object_list()
{
}
//---------------------------------------------------------------------------
psp_file_list::~psp_file_list()
{
    Clear();
}
//---------------------------------------------------------------------------
void psp_file_list::DeleteElem(void *p)
{
   if(p == NULL)
       return;
   if(((LPPSP_FILE)p)->pFile != NULL)
       delete ((LPPSP_FILE)p)->pFile;
   delete (LPPSP_FILE)p;
}
//---------------------------------------------------------------------------
int LOS::psp_create_sema(unsigned long name,unsigned long attr,int initValue,int maxValue,unsigned long arg)
{
   LPPSP_SEMAPHORE p;
   char c;
   LString s;
   int i;

   p = new PSP_SEMAPHORE[1];
   if(p == NULL)
       return -1;
   s = "";
   do{
       c = (char)read_byte(name++);
       if(c == 0)
           break;
       if(c == '/')
           c = '\\';
       s += (char)c;
   }while(true);
   lstrcpy(p->hdr.name,s.c_str());
   p->init_value = initValue;
   p->max_value = maxValue;
   p->current_value = initValue;
   if((i = sema_list.Add((LPVOID)p)) < 0){
       delete p;
       return -1;
   }
   return i;
}
//---------------------------------------------------------------------------
int LOS::psp_delete_sema(unsigned long index)
{
   LPPSP_SEMAPHORE p;
   psp_thread *p1;
   DWORD dwPos,dwPos1;
   LPWAIT_OBJECT p2;

   p = (LPPSP_SEMAPHORE)sema_list.get_ObjectFromID(index);
   if(p == NULL)
       return -1;
   p1 = (psp_thread *)threads_list.GetFirstItem(&dwPos);
   while(p1 != NULL){
       p2 = (LPWAIT_OBJECT)p1->GetFirstItem(&dwPos1);
       while(p2 != NULL){
           if(p2->type == 1 && p2->object == p){
               p1->Delete(p2,TRUE);
               break;
           }
           p2 = (LPWAIT_OBJECT)p1->GetNextItem(&dwPos1);
       }
       p1 = (psp_thread *)threads_list.GetNextItem(&dwPos);
   }
   return 0;
}
//---------------------------------------------------------------------------
int LOS::psp_create_event(unsigned long name,int attr,int bits,unsigned long opts)
{
   LPPSP_EVENT p;
   char c;
   LString s;
   int res;

   p = new PSP_EVENT[1];
   if(p == NULL)
       return -1;
   s = "";
   do{
       c = (char)read_byte(name++);
       if(c == 0)
           break;
       if(c == '/')
           c = '\\';
       s += (char)c;
   }while(true);
   lstrcpy(p->hdr.name,s.c_str());
   p->attr = attr;
   p->bits = bits;
   p->opts = opts;
   if((res = event_list.Add(p)) < 0){
       delete p;
       return -1;
   }
   return res;
}
//---------------------------------------------------------------------------
int LOS::psp_load_module(unsigned long name,int flags,unsigned long opt)
{
   LString s;
   int res,n;
   LPPSP_MODULE p;
   char c;
   DWORD dw,dwPos,base;
   LFile *pFile;
   Elf32_Shdr *secs;
   PBP_Header *header;
   Elf32_Ehdr *elf_hdr;
   psp_segment_list *list;
   LPMEM_SEGMENT seg;

   s = "";
   do{
       c = (char)read_byte(name++);
       if(c == 0)
           break;
       if(c == '/')
           c = '\\';
       s += (char)c;
   }while(true);
   pFile = new LFile(s.c_str());
   pFile->Open();

   if(!get_ModuleSection(pFile,&elf_hdr,&header,&secs,&res)){
       delete p;
       return -1;
   }
   dw = 0;
   for(n=0;n<res;n++){
       if(secs[n]._flags & Allocate){
           switch(secs[n]._type){
               case PROGBITS:
               case NOBITS:
                   dw += secs[n]._size;
               break;
           }
       }
   }
   list = segments_list.get_free_segments();
   base = 0;
   seg = (LPMEM_SEGMENT)list->GetFirstItem(&dwPos);
   while(p != NULL){
       if(dw < seg->size){
           base = seg->adr;
           break;
       }
       seg = (LPMEM_SEGMENT)list->GetNextItem(&dwPos);
   }

   LoadModule(pFile,secs,header,elf_hdr,base,(DWORD *)&res);

   delete list;
   LocalFree(header);
   LocalFree(elf_hdr);
   LocalFree(secs);

   p = new PSP_MODULE[1];
   p->adr = base;
   if((res = module_list.Add(p)) < 0){
       delete p;
       return -1;
   }
   return res;
}
//---------------------------------------------------------------------------
DWORD LOS::LoadModule(LFile *pFile,Elf32_Shdr *shdrs,PBP_Header *header,Elf32_Ehdr *elf_hdr,DWORD baseAddress,DWORD *pgp)
{
   Elf32_Rel reloc;
   PspModuleInfo moduleInfo;
   PspModuleImport *im;
   PspModuleExport *ex;
   Elf32_Phdr offsetHdr,valueHdr;
	BOOL needsRelocation,reserved;
	DWORD defaultLoad,dw,dwPos;
   LString s;
   int n,i,IndexStringTab,m,l,o;
   unsigned char c;
   LPDWORD p_id,p_adr;
   DWORD nid,data,offset,basea,addr;
   LVector<DWORD> hiReloc;

   if(header->pmagic == 0x50425000)
       dwPos = header->offset_psp_data;
   else
       dwPos = 0;
   IndexStringTab = -1;
   for(n=0;n<elf_hdr->_shnum;n++){
       if(shdrs[n]._type == STRTAB && IndexStringTab == -1)
           IndexStringTab = n;
   }
   for(n=0;n<elf_hdr->_shnum;n++){
       shdrs[n]._addr += baseAddress;
       if(shdrs[n]._flags & Allocate){
           reserved = TRUE;
           switch(shdrs[n]._type){
               case PROGBITS:
                   dw = pFile->GetCurrentPosition();
                   pFile->Seek(dwPos+shdrs[n]._offset);
                   for(i=0;i<shdrs[n]._size;i++){
                       pFile->Read(&c,1);
                       write_byte(shdrs[n]._addr+i,c);
                   }
                   pFile->Seek(dw);
               break;
               case NOBITS:
                   for(i=0;i<shdrs[n]._size;i++)
                       write_byte(shdrs[n]._addr+i,0);
               break;
               default:
                   reserved = FALSE;
               break;
           }
           if(reserved)
               use_mem(shdrs[n]._addr,shdrs[n]._size);
       }
   }
   if(needsRelocation){
       for(n=0;n<elf_hdr->_shnum;n++){
           if (shdrs[n]._type != REL && shdrs[n]._type != PRXRELOC)
               continue;
           i = shdrs[n]._size / sizeof(Elf32_Rel);
           pFile->Seek(dwPos + shdrs[n]._offset);
           for(;i>0;i--){
               pFile->Read(&reloc,sizeof(reloc));
               c = (unsigned char)reloc._info;
               if(c == 0)
                   continue;
			    basea = baseAddress;
				offset = reloc._offset + baseAddress;
               m = (int)(reloc._info >> 8);
               if(shdrs[n]._type == REL){
                   if(m != 0){
                       m = 0;
                   }
               }
               else{
                   l = m & 0xFF;
                   o = m >> 8;
                   if(o != 0 || l != 0){
                       dw = pFile->GetCurrentPosition();
                       pFile->Seek(dwPos+elf_hdr->_phoff + m * elf_hdr->_phentsize);
                       pFile->Read(&offsetHdr,sizeof(offsetHdr));
                       pFile->Seek(dwPos+elf_hdr->_phoff + o * elf_hdr->_phentsize);
                       pFile->Read(&valueHdr,sizeof(valueHdr));
                       pFile->Seek(dw);
                   }
               }
               switch(c){
                   case Mips26:
                       data = ::read_dword(offset);
                       addr = (data & 0x03FFFFFF) << 2;
                       addr += basea;
                       addr = (data & ~0x3FFFFFF) | (addr >> 2);
                       ::write_dword(offset,addr);
                   break;
                   case MipsHi16:
                       hiReloc.push(offset);
                   break;
                   case MipsLo16:
                       DWORD value2;

                       data = ::read_dword(offset);
                       data = ((data & 0xFFFF ) ^ 0x8000 ) - 0x8000;
                       while(hiReloc.count()){
                           offset = hiReloc.pop();
                           value2 = ::read_dword(offset);
                           dw = ((value2 & 0xFFFF) << 16) + data;
						    dw += basea;
							dw = ((dw >> 16 ) + (((dw & 0x00008000) != 0) ? 1 : 0)) & 0x0000FFFF;
                           value2 = ((value2 & ~0x0000FFFF)|dw);
                           ::write_dword(offset,value2);
                       }
                       hiReloc.free();
                   break;
                   case Mips32:
                       data = ::read_dword(offset);
                       data += basea;
                       ::write_dword(offset,data);
                   break;
               }
           }
       }
   }
   for(i=n=0;n<shdrs[IndexStringTab]._size;i++){
       pFile->Seek(dwPos + shdrs[IndexStringTab]._offset + n);
       s = "";
       l = -1;
       do{
           if(pFile->Read(&c,1) != 1)
               break;
           n++;
           s += (char)c;
           l++;
       }while(c != 0);
       if(s == ".rodata.sceModuleInfo"){
           break;
       }
   }
   pFile->Seek(dwPos + shdrs[i]._offset);
   pFile->Read(&moduleInfo,sizeof(moduleInfo));

   n = (moduleInfo.exp_end - moduleInfo.exports) / sizeof(PspModuleExport);
   ex = (PspModuleExport *)&mem[moduleInfo.exports - 0x08000000 + baseAddress];
   for(i=0;i<n;i++){
   }
   n = (moduleInfo.imp_end - moduleInfo.imports) / sizeof(PspModuleImport);
   im = (PspModuleImport *)&mem[moduleInfo.imports - 0x08000000 + baseAddress];
   for(i=0;i<n;i++){
       p_id = (LPDWORD)&mem[im[i].nids - 0x08000000];
       p_adr = (LPDWORD)&mem[im[i].funcs - 0x08000000];
       for(m=0;m<im[i].func_count;m++){
           nid = p_id[m];
           l = IndexOfFunc(nid);
           if(l != -1)
               p_adr[m*2] = 0xFC000000|l;
           else
               l = -1;
       }
   }
   *pgp = baseAddress + moduleInfo.gp;
   return elf_hdr->_entry + baseAddress;
}
//---------------------------------------------------------------------------
BOOL LOS::get_ModuleSection(LFile *pFile,Elf32_Ehdr **elf_hdr,PBP_Header **header,Elf32_Shdr **secs,int *nsecs)
{
    Elf32_Shdr *shdrs;
   int n;
   DWORD dwPos;

    if(pFile == NULL || !pFile->IsOpen())
        return FALSE;
    *secs = NULL;
    *header = (PBP_Header *)LocalAlloc(LPTR,sizeof(PBP_Header));
    *elf_hdr = (Elf32_Ehdr *)LocalAlloc(LPTR,sizeof(Elf32_Ehdr));

    pFile->Read(*header,sizeof(PBP_Header));
    if((*header)->pmagic == 0x50425000)
        dwPos = (*header)->offset_psp_data;
    else
        dwPos = 0;
    pFile->Seek(dwPos);
    if(pFile->Read(*elf_hdr,sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr))
        return FALSE;
    if(*((DWORD *)(*elf_hdr)->_magic) != 0x464C457F){
        return FALSE;
    }
    if((*elf_hdr)->_machine != 8)
       return FALSE;
   shdrs = (Elf32_Shdr *)LocalAlloc(LPTR,sizeof(Elf32_Shdr)* (*elf_hdr)->_shnum);
   for(n=0;n<(*elf_hdr)->_shnum;n++){
       pFile->Seek((*elf_hdr)->_shoff + ((*elf_hdr)->_shentsize * n) + dwPos);
       pFile->Read(&shdrs[n],sizeof(Elf32_Shdr));
    }
    *secs = shdrs;
    if(nsecs != NULL)
       *nsecs = (*elf_hdr)->_shnum;
   return TRUE;
}
//---------------------------------------------------------------------------
DWORD LOS::LoadFile(char *fileName,DWORD *pgp)
{
   PBP_Header header;
   Elf32_Ehdr elf_hdr;
   Elf32_Shdr *shdrs;
   Elf32_Rel reloc;
   PspModuleInfo moduleInfo;
   PspModuleImport *im;
   PspModuleExport *ex;
   Elf32_Phdr offsetHdr,valueHdr;
	BOOL needsRelocation,reserved;
	DWORD defaultLoad,baseAddress,dw,dwPos;
   LString s;
   int n,i,IndexStringTab,m,l,o;
   unsigned char c;
   LPDWORD p_id,p_adr;
   DWORD nid,data,offset,basea,addr;
   LVector<DWORD> hiReloc;
   LFile *pFile;

   if(fileName == NULL || *fileName == 0)
       return 0xFFFFFFFF;
   if((pFile = new LFile(fileName)) == NULL)
       return 0xFFFFFFFF;
   if(!pFile->Open()){
       delete pFile;
       pFile = NULL;
       return 0xFFFFFFFF;
   }
   pFile->Read(&header,sizeof(PBP_Header));
   if(header.pmagic == 0x50425000){
       dwPos = header.offset_psp_data;
   }
   else
       dwPos = 0;
   IndexStringTab = -1;
   pFile->Seek(dwPos);
   if(pFile->Read(&elf_hdr,sizeof(elf_hdr)) != sizeof(elf_hdr))
       return 0xFFFFFFFF;
   if(*((DWORD *)elf_hdr._magic) != 0x464C457F)
       return 0xFFFFFFFF;
   if(elf_hdr._machine != 8)
       return 0xFFFFFFFF;
   needsRelocation = elf_hdr._entry < 0x08000000 || elf_hdr._type == Prx;
	defaultLoad = 0x08900000;
	if (needsRelocation)
       baseAddress = 0x08900000;
   else
       baseAddress = 0;
   shdrs = (Elf32_Shdr *)LocalAlloc(LPTR,sizeof(Elf32_Shdr)*elf_hdr._shnum);
   for(n=0;n<elf_hdr._shnum;n++){
       pFile->Seek(elf_hdr._shoff + (elf_hdr._shentsize * n) + dwPos);
       pFile->Read(&shdrs[n],sizeof(Elf32_Shdr));
       if(shdrs[n]._type == STRTAB && IndexStringTab == -1)
           IndexStringTab = n;
   }
   for(n=0;n<elf_hdr._shnum;n++){
       shdrs[n]._addr += baseAddress;
       if(shdrs[n]._flags & Allocate){
           reserved = TRUE;
           switch(shdrs[n]._type){
               case PROGBITS:
                   dw = pFile->GetCurrentPosition();
                   pFile->Seek(dwPos+shdrs[n]._offset);
                   for(i=0;i<shdrs[n]._size;i++){
                       pFile->Read(&c,1);
                       write_byte(shdrs[n]._addr+i,c);
                   }
                   pFile->Seek(dw);
               break;
               case NOBITS:
                   for(i=0;i<shdrs[n]._size;i++)
                       write_byte(shdrs[n]._addr+i,0);
               break;
               default:
                   reserved = FALSE;
               break;
           }
           if(reserved)
               use_mem(shdrs[n]._addr,shdrs[n]._size);
       }
   }
   if(needsRelocation){
       for(n=0;n<elf_hdr._shnum;n++){
           if (shdrs[n]._type != REL && shdrs[n]._type != PRXRELOC)
               continue;
           i = shdrs[n]._size / sizeof(Elf32_Rel);
           pFile->Seek(dwPos + shdrs[n]._offset);
           for(;i>0;i--){
               pFile->Read(&reloc,sizeof(reloc));
               c = (unsigned char)reloc._info;
               if(c == 0)
                   continue;
			    basea = baseAddress;
				offset = reloc._offset + baseAddress;
               m = (int)(reloc._info >> 8);
               if(shdrs[n]._type == REL){
                   if(m != 0){
                       m = 0;
                   }
               }
               else{
                   l = m & 0xFF;
                   o = m >> 8;
                   if(o != 0 || l != 0){
                       dw = pFile->GetCurrentPosition();
                       pFile->Seek(dwPos+elf_hdr._phoff + m * elf_hdr._phentsize);
                       pFile->Read(&offsetHdr,sizeof(offsetHdr));
                       pFile->Seek(dwPos+elf_hdr._phoff + o * elf_hdr._phentsize);
                       pFile->Read(&valueHdr,sizeof(valueHdr));
                       pFile->Seek(dw);
                   }
               }
               switch(c){
                   case Mips26:
                       data = ::read_dword(offset);
                       addr = (data & 0x03FFFFFF) << 2;
                       addr += basea;
                       addr = (data & ~0x3FFFFFF) | (addr >> 2);
                       ::write_dword(offset,addr);
                   break;
                   case MipsHi16:
                       hiReloc.push(offset);
                   break;
                   case MipsLo16:
                       DWORD value2;

                       data = ::read_dword(offset);
                       data = ((data & 0xFFFF ) ^ 0x8000 ) - 0x8000;
                       while(hiReloc.count()){
                           offset = hiReloc.pop();
                           value2 = ::read_dword(offset);
                           dw = ((value2 & 0xFFFF) << 16) + data;
						    dw += basea;
							dw = ((dw >> 16 ) + (((dw & 0x00008000) != 0) ? 1 : 0)) & 0x0000FFFF;
                           value2 = ((value2 & ~0x0000FFFF)|dw);
                           ::write_dword(offset,value2);
                       }
                       hiReloc.free();
                   break;
                   case Mips32:
                       data = ::read_dword(offset);
                       data += basea;
                       ::write_dword(offset,data);
                   break;
               }
           }
       }
   }
   for(i=n=0;n<shdrs[IndexStringTab]._size;i++){
       pFile->Seek(dwPos + shdrs[IndexStringTab]._offset + n);
       s = "";
       l = -1;
       do{
           if(pFile->Read(&c,1) != 1)
               break;
           n++;
           s += (char)c;
           l++;
       }while(c != 0);
       if(s == ".rodata.sceModuleInfo"){
           break;
       }
   }
   pFile->Seek(dwPos + shdrs[i]._offset);
   pFile->Read(&moduleInfo,sizeof(moduleInfo));

   n = (moduleInfo.exp_end - moduleInfo.exports) / sizeof(PspModuleExport);
   ex = (PspModuleExport *)&mem[moduleInfo.exports - 0x08000000 + baseAddress];
   for(i=0;i<n;i++){
   }
   n = (moduleInfo.imp_end - moduleInfo.imports) / sizeof(PspModuleImport);
   im = (PspModuleImport *)&mem[moduleInfo.imports - 0x08000000 + baseAddress];
   for(i=0;i<n;i++){
       p_id = (LPDWORD)&mem[im[i].nids - 0x08000000];
       p_adr = (LPDWORD)&mem[im[i].funcs - 0x08000000];
       for(m=0;m<im[i].func_count;m++){
           nid = p_id[m];
           l = IndexOfFunc(nid);
           if(l != -1)
               p_adr[m*2] = 0xFC000000|l;
           else
               l = -1;
       }
   }
   LocalFree(shdrs);
   *pgp = baseAddress + moduleInfo.gp;
   ::write_dword(0x08000000,0xFC00000C);
   return elf_hdr._entry + baseAddress;
}

