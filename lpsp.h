#include <windows.h>
#include <commctrl.h>
#include "lgpu.h"
#include "lmmu.h"
#include "r4000.h"
#include "ldsp.h"
#include "lslist.h"

//---------------------------------------------------------------------------
#ifndef lpspH
#define lpspH
//---------------------------------------------------------------------------
class LPSP : public LGPU,public LDSP,LMMU,public LCPU,public LDebugDlg
{
public:
   LPSP();
   ~LPSP();
   BOOL Init();
   void OnLoop();
   void Reset();
   BOOL OnOpenFile(char *lpFileName);
   void OnCommand(WORD wID);
   void OnInitMenuPopup(HMENU hMenu,UINT uPos);
   unsigned long read_controller(unsigned long adr,int mode);
protected:
   void OnKeyDown(WPARAM wParam,LPARAM lParam);
   void OnKeyUp(WPARAM wParam,LPARAM lParam);
   void UpdateControls();
   ATOM classId;
   LStringList recentFiles;
};

extern LPSP psp;
#endif

