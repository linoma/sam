#include <windows.h>
#include "os.h"
#include "bp.h"
#include "memviewdlg.h"
#include "bpdlg.h"

//---------------------------------------------------------------------------
#ifndef __debugdlgH__
#define __debugdlgH__

//---------------------------------------------------------------------------
typedef struct _dispointer{
   DWORD StartAddress;
   DWORD EndAddress;
} DISVIEW,*LPDISVIEW;
//---------------------------------------------------------------------------
enum PAGETYPE {PG_NULL=-1,PG_MEMVIEW=1,PG_BREAKPOINT,PG_MEMBREAKPOINT,PG_SYSTEMCONTROL,PG_CALLSTACK,PG_CONSOLLE};
enum CHANGEREGTYPE {CRT_ZERO,CRT_INCR,CRT_DECR,CRT_CHANGE};
//---------------------------------------------------------------------------
#define DBV_RUN    		0
#define DBV_VIEW   		1
#define DBV_NULL   		0xFF

#define AMM_BYTE           1
#define AMM_WORD           2
#define AMM_DWORD          4

#define AMM_READ           0x10
#define AMM_WRITE          0x20

//---------------------------------------------------------------------------
class LDebugDlg
{
public:
   LDebugDlg();
   ~LDebugDlg();
   BOOL Create(HWND hwnd);
   BOOL OnDialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   static BOOL DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   void Wait();
   static LRESULT WindowProcListBoxDis(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   LRESULT OnWindowProcListBoxDis(UINT uMsg,WPARAM wParam,LPARAM lParam);
   BOOL IsDialogMessage(LPMSG p);
   void EnterWaitMode();
   void OnKeyDown(WPARAM wParam,LPARAM lParam);
   inline BOOL is_Debug(){return bDebug;};
protected:
   BOOL CheckBreakpoints(DWORD adr);
   void OnCommand(WORD wID);
   void OnItemCommand(WORD wID,WORD wNotifyCode,HWND hwnd);
   void Close();
   void Update();
   void Reset();
   void UpdateDissasemblerWindow(DWORD *p,int indexView,int nItem);
   void UpdateRegisterWindow();
   BOOL OnDrawItem(WORD wID,LPDRAWITEMSTRUCT lpDIS);
   void OnVScroll(WPARAM wParam,LPARAM lParam);
   void UpdateVertScrollBarDisDump(DWORD address,int index);
   void OnInitDialog();
   void OnSize(int width,int height);
   void OnNotify(int id,LPNMHDR param);
   void OnChangeRegViewMode(int mode);   
   HWND m_hWnd;
   BOOL bDebug,bPass,bFlag;
   DWORD yScroll,dwBreak;
   DISVIEW views[2];
   int iMaxItem,bTrackMode,iCurrentView,bCurrentTrackMode,nBreakMode,nViewRegMode;
   WNDPROC WndProcOld;
   psp_thread *current_thread;
   LBreakpointDlg brkpointDlg;
   LMemoryDlg memDlg;
   HACCEL accel;
};
#endif
