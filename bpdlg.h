#include <windows.h>
#include "lstring.h"
#include "llist.h"

//---------------------------------------------------------------------------
#ifndef __bpdlgH__
#define __bpdlgH__
//---------------------------------------------------------------------------
class LBreakpointPropertiesDlg
{
public:
   LBreakpointPropertiesDlg();
   ~LBreakpointPropertiesDlg();
   BOOL Create(HWND hwnd,LBreakPoint *p = NULL);
   BOOL OnDialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   static BOOL DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
protected:
   void OnInitDialog(HWND hwnd);
   void OnItemCommand(WORD wID,WORD wNotifyCode,HWND hwnd);
   LBreakPoint *breakpoint;
   HWND m_hWnd;
};
//---------------------------------------------------------------------------
class LBreakpointDlg : public LList
{
public:
   LBreakpointDlg();
   ~LBreakpointDlg();
   BOOL Create(HWND hwnd);
   BOOL OnDialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   static BOOL DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   LBreakPoint *Add(unsigned long address,int type = BT_PROGRAM);
   BOOL Delete(DWORD item,BOOL bFlag = TRUE);
   BOOL Add(LBreakPoint *p);
   LBreakPoint *Check(unsigned long address,int accessMode);
   LBreakPoint *Find(unsigned long address,int type = BT_PROGRAM);
   void Reset();
   BOOL Save(const char *lpFileName,BOOL bForce = FALSE);
   BOOL Load(const char *lpFileName);
   void Clear();
   inline BOOL is_Modified(){return bModified;};
   inline HWND Handle(){return m_hWnd;};
   void Update();
protected:
   void OnInitDialog();
   void OnNotifyListView(LPNMHDR p);
   void OnCommand(WORD wID);
   void OnItemCommand(WORD wId,WORD wNotifyCode,HWND hWnd);
	void DeleteElem(LPVOID p);

   HWND m_hWnd;
   BOOL bModified;
   int currentType;
   LString fileName;
   LBreakpointPropertiesDlg propDlg;
};

#endif
