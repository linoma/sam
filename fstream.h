#include <windows.h>
#include "llist.h"
//---------------------------------------------------------------------------
#ifndef __fstreamH__
#define __fstreamH__
//---------------------------------------------------------------------------
#ifdef __WIN32__
#define NULL_STREAM INVALID_HANDLE_VALUE
#else
#define NULL_STREAM NULL
#endif

//---------------------------------------------------------------------------
#ifndef __LStream__
#define __LStream__
struct LStream
{
public:
	virtual BOOL Open(DWORD dwStyle = GENERIC_READ,DWORD dwCreation = OPEN_EXISTING,DWORD dwFlags = 0) PURE;
   virtual void Close() PURE;
   virtual DWORD Read(LPVOID lpBuffer,DWORD dwBytes) PURE;
   virtual DWORD Write(LPVOID lpBuffer,DWORD dwBytes) PURE;
   virtual DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN) PURE;
   virtual BOOL SeekToBegin() PURE;
   virtual BOOL SeekToEnd() PURE;
   virtual DWORD Size(LPDWORD lpHigh = NULL) PURE;
   virtual BOOL SetEndOfFile(DWORD dw) PURE;
   virtual DWORD GetCurrentPosition() PURE;
	virtual BOOL IsOpen() PURE;
   virtual void Release() PURE;
};
#endif
//---------------------------------------------------------------------------
class LFile : public LStream
{
public:
   LFile(const char *name);
   virtual ~LFile();
   void Release(){delete this;};
   virtual BOOL Open(DWORD dwStyle=GENERIC_READ,DWORD dwCreation=OPEN_EXISTING,DWORD dwFlags = 0);
   virtual void Close();
   virtual DWORD Read(LPVOID lpBuffer,DWORD dwBytes);
   virtual DWORD Write(LPVOID lpBuffer,DWORD dwBytes);
   virtual DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN);
   virtual inline BOOL SeekToBegin(){if(Seek() != 0xFFFFFFFF) return TRUE; return FALSE;};
   virtual inline BOOL SeekToEnd(){if(Seek(0,FILE_END) != 0xFFFFFFFF) return TRUE; return FALSE;};
   virtual DWORD Size(LPDWORD lpHigh = NULL);
   virtual BOOL SetEndOfFile(DWORD dw);
   virtual inline DWORD GetCurrentPosition(){return Seek(0,FILE_CURRENT);};
   virtual inline HANDLE Handle(){return handle;};
   virtual inline BOOL IsOpen(){return (BOOL)(handle != NULL_STREAM ? TRUE : FALSE);};
   DWORD WriteF(char *mes,...);
#ifdef __WIN32__
   inline BOOL Flush(){return FlushFileBuffers(handle);};
#else
   inline BOOL Flush(){fflush(handle); return TRUE;};
#endif
   DWORD ReadLine(DWORD dwLine,char *lpBuffer,DWORD dwBytes);
protected:
   char fileName[MAX_PATH];
#ifdef __WIN32__
   HANDLE handle;
#else
	FILE *handle;
#endif
};
//---------------------------------------------------------------------------
class LMemoryFile : public LStream, LList
{
public:
   LMemoryFile(DWORD dw = 8192);
   ~LMemoryFile();
   void Release(){delete this;};
   BOOL Open(DWORD dwStyle=GENERIC_READ,DWORD dwCreation=OPEN_EXISTING,DWORD dwFlags = 0);
   void Close();
   DWORD Read(LPVOID lpBuffer,DWORD dwBytes);
   DWORD Write(LPVOID lpBuffer,DWORD dwBytes);
   DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN);
   inline BOOL SeekToBegin(){if(Seek() != 0xFFFFFFFF) return TRUE; return FALSE;};
   inline BOOL SeekToEnd(){if(Seek(0,FILE_END) != 0xFFFFFFFF) return TRUE; return FALSE;};
   DWORD Size(LPDWORD lpHigh = NULL);
   BOOL SetEndOfFile(DWORD dw);
   inline DWORD GetCurrentPosition(){return Seek(0,FILE_CURRENT);};
   inline BOOL IsOpen(){return (BOOL)(nCount > 0 ? TRUE : FALSE);};
   BOOL SaveToFile(const char *lpFileName);
protected:
//---------------------------------------------------------------------------
   struct buffer
   {
       LPBYTE buf;
       DWORD dwSize,dwPos,dwBytesWrite;
   public:
       buffer(DWORD dw = 8192);
   };
//---------------------------------------------------------------------------
   void DeleteElem(LPVOID ele);
   buffer *AddBuffer();
//---------------------------------------------------------------------------
   DWORD dwPos,dwSize,dwIndex,dwBufferSize;
   buffer *currentBuffer;
};
#endif
