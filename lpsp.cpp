#include "lpsp.h"
#include "lstring.h"
#include "resource.h"

//---------------------------------------------------------------------------
extern HINSTANCE hInstance;
extern BOOL bQuit;
//---------------------------------------------------------------------------
LPSP::LPSP() : LGPU(),LDSP(),LMMU(),LCPU(),LDebugDlg()
{
   bQuit = FALSE;
}
//---------------------------------------------------------------------------
LPSP::~LPSP()
{
   recentFiles.Save("Settings\\Sam\\Settings\\RecentFile");
   OleUninitialize();
}
//---------------------------------------------------------------------------
BOOL LPSP::Init()
{
   INITCOMMONCONTROLSEX ice={0};
	WNDCLASS wc={0};

   OleInitialize(NULL);
   ZeroMemory(&ice,sizeof(INITCOMMONCONTROLSEX));
   ice.dwSize = sizeof(INITCOMMONCONTROLSEX);
   ice.dwICC = ICC_WIN95_CLASSES|ICC_INTERNET_CLASSES;
	InitCommonControlsEx(&ice);

	wc.style 			= CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
   wc.lpfnWndProc 		= DefWindowProc;
   wc.hInstance 		= hInstance;
   wc.hCursor 			= ::LoadCursor(NULL,IDC_ARROW);
   wc.lpszClassName 	= "SAM";
   wc.hbrBackground	= (HBRUSH)::GetStockObject(BLACK_BRUSH);
   wc.hIcon			= ::LoadIcon(hInstance,MAKEINTRESOURCE(2));
   if((classId = RegisterClass(&wc)) == 0)
   	return FALSE;
   if(!LCPU::Init())
       return FALSE;
   if(!LGPU::Init())
       return FALSE;
   if(!LDSP::Init())
       return FALSE;
   if(!LMMU::Init())
       return FALSE;
   if(!LOS::Init())
       return FALSE;
   recentFiles.set_MaxElem(10);
   recentFiles.Load("Settings\\Sam\\Settings\\RecentFile");
   ShowWindow(LGPU::m_hWnd,SW_SHOW);
   return TRUE;
}
//---------------------------------------------------------------------------
void LPSP::Reset()
{
   bStart = bRom = FALSE;
   LCPU::Reset();
   LGPU::Reset();
   LDSP::Reset();
   LMMU::Reset();
   LOS::Reset();
   LDebugDlg::Reset();
}
//---------------------------------------------------------------------------
void LPSP::OnLoop()
{
   MSG msg;
   char s[100];

   while(!bQuit){
       if(bRom && bStart){
           status = 0;
           cycles = 19425;
           for(lines=0;lines<272;lines++){
               status &= ~2;
               for(cycles -= 19425;cycles<17384;){
                   if(bDebug) Wait();
                   cycles += exec_cpu();
               }
               status |= 2;
               for(;cycles<19425;){
                   if(bDebug) Wait();
                   cycles += exec_cpu();
               }
           }
           BitBlt();
           status |= 1;
           for(;lines<286;lines++){
               status &= ~2;
               for(cycles -= 19425;cycles<17384;){
                   if(bDebug) Wait();
                   cycles += exec_cpu();
               }
               status |= 2;
               for(;cycles<19425;){
                   if(bDebug) Wait();
                   cycles += exec_cpu();
               }
           }
       }
       else{
           WaitMessage();
       }
   	while(::PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
           if(GetActiveWindow() != LGPU::m_hWnd || !TranslateAccelerator(LGPU::m_hWnd,LGPU::accel,&msg)){
               if(!IsDialogMessage(&msg)){
                   if(!::TranslateMessage(&msg))
                       ::DispatchMessage(&msg);
               }
           }
           if(msg.message == WM_KEYDOWN)
               OnKeyDown(msg.wParam,msg.lParam);
           else if(msg.message == WM_KEYUP)
               OnKeyUp(msg.wParam,msg.lParam);
       }
   }
}
//---------------------------------------------------------------------------
BOOL LPSP::OnOpenFile(char *lpFileName)
{
   OPENFILENAME ofn={0};
	char *c;
	BOOL res;
   LString s;
   DWORD dw,gp;

	Reset();
   res = FALSE;
  	if((c = (char *)LocalAlloc(LPTR,5000)) == NULL)
      	goto ex_OnOpenFile;
   if(lpFileName == NULL || lstrlen(lpFileName) == 0){
       ofn.lStructSize = sizeof(OPENFILENAME);
       ofn.hwndOwner = LGPU::m_hWnd;
       ofn.lpstrFile = c;
       ofn.nMaxFile = 5000;
       ofn.lpstrFilter = "PBP File (*.pbp)\0*.pbp\0All files (*.*)\0*.*\0\0\0\0\0";
       ofn.nFilterIndex = 1;
       ofn.Flags = OFN_FILEMUSTEXIST|OFN_NOREADONLYRETURN|OFN_HIDEREADONLY|OFN_EXPLORER;
       ofn.lpstrInitialDir = lastPath.c_str();
       if(!GetOpenFileName(&ofn))
           goto ex_OnOpenFile_1;
   }
   else
   	lstrcpy(c,lpFileName);
   dw = LoadFile(c,&gp);
   if(dw == 0xFFFFFFFF)
       res = FALSE;
   else{
       lastPath = LString(c).Path();
       lastFileName = c;
   	if(recentFiles.Find(c) == NULL)
   		recentFiles.Add(c);
       set_EntryAddress(dw,gp);
       res = TRUE;
   }
ex_OnOpenFile:
	if(!res){
       s = "Unable to read ";
       s += c;
		MessageBox(LGPU::m_hWnd,s.c_str(),"Sam Emulator",MB_OK|MB_ICONERROR);
   }
   else{
       bRom = TRUE;
       bStart = FALSE;
       UpdateControls();
   }
ex_OnOpenFile_1:
   if(c != NULL)
   	LocalFree(c);
   return bRom;
}
//---------------------------------------------------------------------------
void LPSP::UpdateControls()
{
   UINT cmd;

   cmd = bRom && !bStart ? MF_ENABLED : MF_GRAYED;
   EnableMenuItem(GetMenu(LGPU::m_hWnd),ID_FILE_START,MF_BYCOMMAND|cmd);
   cmd = bRom && bStart ? MF_ENABLED : MF_GRAYED;
   EnableMenuItem(GetMenu(LGPU::m_hWnd),ID_FILE_PAUSE,MF_BYCOMMAND|cmd);
   EnableMenuItem(GetMenu(LGPU::m_hWnd),ID_FILE_STOP,MF_BYCOMMAND|cmd);
   EnableMenuItem(GetMenu(LGPU::m_hWnd),ID_FILE_RESET,MF_BYCOMMAND|cmd);
}
//---------------------------------------------------------------------------
void LPSP::OnInitMenuPopup(HMENU hMenu,UINT uPos)
{
   HMENU menu;
   int i;
   LString s;
   MENUITEMINFO mi;
   
   if(uPos != 9)
       return;
   menu = GetMenu(Handle());
   if(menu == NULL)
       return;
   if(GetSubMenu(GetSubMenu(menu,0),9) != hMenu)
       return;
   for(i=GetMenuItemCount(hMenu)-1;i>0;i--)
       DeleteMenu(hMenu,i,MF_BYPOSITION);
   if(recentFiles.Count() < 1)
       return;
   mi.cbSize = sizeof(mi);
   mi.fMask = MIIM_TYPE;
   mi.fType = MFT_SEPARATOR;
   InsertMenuItem(hMenu,1,TRUE,&mi);
   for(i=recentFiles.Count();i>0;i--){
       mi.cbSize = sizeof(mi);
       mi.fMask = MIIM_ID|MIIM_TYPE;
       mi.fType = MFT_STRING;
       mi.wID = ID_FILE_RECENT_FILE0 + i - 1;
       mi.dwTypeData = (char *)((LString *)recentFiles.GetItem(i))->c_str();
       InsertMenuItem(hMenu,mi.wID,FALSE,&mi);
   }
}
//---------------------------------------------------------------------------
void LPSP::OnCommand(WORD wID)
{
   MSGBOXPARAMS mp={0};

   if(wID >= ID_FILE_RECENT_FILE0 && wID < ID_FILE_RECENT_FILE0 + 11){
       OnOpenFile((char *)((LString *)recentFiles.GetItem(wID - ID_FILE_RECENT_FILE0 + 1))->c_str());
       return;
   }
   switch(wID){
       case ID_FILE_EXIT:
           bQuit = TRUE;
       break;
       case ID_INFO_INFO:
           mp.cbSize = sizeof(MSGBOXPARAMS);
           mp.hwndOwner = Handle();
           mp.hInstance = hInstance;
           mp.lpszText = new char[100];
			lstrcpy((char *)mp.lpszText,"Sam Emulator\r\nVersion 1.0.0.2\r\n");
           mp.lpszCaption = "Sam Emulator";
           mp.lpszIcon = MAKEINTRESOURCE(2);
           mp.dwStyle = MB_USERICON|MB_OK|MB_APPLMODAL;
      		::MessageBoxIndirect(&mp);
			delete mp.lpszText;
       break;
       case ID_FILE_DEBUGGER:
           Create(LGPU::m_hWnd);
       break;
       case ID_FILE_OPEN:
           OnOpenFile(NULL);
       break;
       case ID_FILE_START:
           bStart = TRUE;
           UpdateControls();
       break;
       case ID_FILE_PAUSE:
           bStart = FALSE;
           UpdateControls();
       break;
       case ID_FILE_STOP:
           bStart = FALSE;
           UpdateControls();
       break;
       case ID_FILE_RESET:
           Reset();
           if(!lastFileName.IsEmpty())
               OnOpenFile(lastFileName.c_str());
           UpdateControls();
       break;

   }
}
//---------------------------------------------------------------------------
void LPSP::OnKeyDown(WPARAM wParam,LPARAM lParam)
{
   LDebugDlg::OnKeyDown(wParam,lParam);
}
//---------------------------------------------------------------------------
void LPSP::OnKeyUp(WPARAM wParam,LPARAM lParam)
{
}
//---------------------------------------------------------------------------
unsigned long LPSP::read_controller(unsigned long adr,int mode)
{
   LPDWORD p;
   BYTE keys[300],key;
   CONTROLLER ctrl={0};

   GetKeyboardState(keys);
   ctrl.data.Lx = 0x7F;
   ctrl.data.Ly = 0x7F;
   if(keys[VK_LEFT] & 0x80){
       ctrl.data.Lx = 0;
       ctrl.data.Buttons |= PSP_CTRL_LEFT;
   }
   if(keys[VK_RIGHT] & 0x80){
       ctrl.data.Lx = 255;
       ctrl.data.Buttons |= PSP_CTRL_RIGHT;
   }
   if(keys[VK_UP] & 0x80){
       ctrl.data.Ly = 0;
       ctrl.data.Buttons |= PSP_CTRL_UP;
   }
   if(keys[VK_DOWN] & 0x80){
       ctrl.data.Ly = 255;
       ctrl.data.Buttons |= PSP_CTRL_DOWN;
   }
   if(keys[VK_RETURN] & 0x80)
       ctrl.data.Buttons |= PSP_CTRL_START;
   if(keys[VK_SHIFT] & 0x80)
       ctrl.data.Buttons |= PSP_CTRL_SELECT;
   if(keys['A'] & 0x80)
       ctrl.data.Buttons |= PSP_CTRL_TRIANGLE;
   if(keys['X'] & 0x80)
       ctrl.data.Buttons |= PSP_CTRL_CIRCLE;
   if(keys['D'] & 0x80)
       ctrl.data.Buttons |= PSP_CTRL_CROSS;
   if(keys['W'] & 0x80)
       ctrl.data.Buttons |= PSP_CTRL_SQUARE;
   if(keys['L'] & 0x80)
       ctrl.data.Buttons |= PSP_CTRL_LTRIGGER;
   if(keys['R'] & 0x80)
       ctrl.data.Buttons |= PSP_CTRL_RTRIGGER;
   p = (LPDWORD)&ctrl.data;
   for(int i = 0;i<sizeof(SceCtrlData);i += 4,adr += 4)
       ::write_dword(adr,*p++);
   return adr;
}
