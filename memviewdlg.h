#include <windows.h>

//---------------------------------------------------------------------------
#ifndef __memviewdlgH__
#define __memviewdlgH__

//---------------------------------------------------------------------------
class LMemoryDlg
{
public:
   LMemoryDlg();
   ~LMemoryDlg();
   inline HWND Handle(){return m_hWnd;};
   BOOL Create(HWND hwnd);
   BOOL OnDialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   static BOOL DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   static LRESULT WindowProcStaticMemView(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   LRESULT OnWindowProcStaticMemView(UINT uMsg,WPARAM wParam,LPARAM lParam);
   void Update(BOOL bInvalidate);
protected:
   void OnInitDialog();
   void Destroy();
   void OnSize(int width,int height);
   void OnCommand(WORD wID);
   void OnItemCommand(WORD wId,WORD wNotifyCode,HWND hWnd);
   void UpdateScrollBar(BOOL bRepos);
   void OnPaint(HDC hdc,RECT &rcClip);
   BOOL get_ItemSize(LPSIZE s);
   BOOL get_ItemsPage(LPSIZE s);
   void OnVScroll(WPARAM wParam,LPARAM lParam);
   void OnRButtonUp(int xPos,int yPos);
   BOOL PointToAddress(POINT &pt,DWORD *address);  
   DWORD yScroll;
   int nMode;
   HWND m_hWnd,m_hWndMemoryView;
   HFONT hFont;
   SIZE szNumberFont,szLetterFont;
   char *lpszText;
   WNDPROC MemViewWndProc;
};

#endif
