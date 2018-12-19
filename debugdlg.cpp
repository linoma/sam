#include "debugdlg.h"
#include "resource.h"
#include "lpsp.h"
#include "util.h"
#include <stdio.h>


extern HINSTANCE hInstance;
extern BOOL bQuit;
//---------------------------------------------------------------------------
LDebugDlg::LDebugDlg()
{
   m_hWnd = NULL;
   current_thread = NULL;
   accel = NULL;
}
//---------------------------------------------------------------------------
LDebugDlg::~LDebugDlg()
{
   if(m_hWnd != NULL)
       DestroyWindow(m_hWnd);
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::Create(HWND hwnd)
{
   HWND hWnd;

   if(m_hWnd == NULL){
       m_hWnd = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),hwnd,(DLGPROC)DialogProc,(LPARAM)this);
       if(m_hWnd == NULL)
           return FALSE;
       SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
       SetWindowLong((hWnd = GetDlgItem(m_hWnd,IDC_DEBUG_DIS)),GWL_USERDATA,(LONG)this);
       WndProcOld = (WNDPROC)SetWindowLong(hWnd,GWL_WNDPROC,(LONG)WindowProcListBoxDis);
   }
   else
       BringWindowToTop(m_hWnd);
   bDebug = TRUE;
   return TRUE;
}
//---------------------------------------------------------------------------
void LDebugDlg::EnterWaitMode()
{
    Create(psp.LGPU::Handle());
    dwBreak = 0;
    nBreakMode = 0;
    bPass = FALSE;
}
//---------------------------------------------------------------------------
void LDebugDlg::OnItemCommand(WORD wID,WORD wNotifyCode,HWND hwnd)
{
   char s[30];
   DWORD dw;
   int i;

   switch(wID){
       case IDC_DEBUG_GO:
           GetDlgItemText(m_hWnd,IDC_DEBUG_EDITGO,s,30);
           dw = StrToHex(s);
           UpdateDissasemblerWindow(&dw,DBV_RUN,0);
       break;
       case IDC_THREAD:
#ifdef _DEBPRO
           switch(wNotifyCode){
               case CBN_SELENDOK:
                   i = SendMessage(hwnd,CB_GETCURSEL,0,0);
                   if(i == CB_ERR)
                       return;
                   current_thread = (psp_thread *)SendMessage(hwnd,CB_GETITEMDATA,i,0);
               break;
           }
#endif
       break;
       case IDC_REG_MODE:
           switch(wNotifyCode){
               case CBN_SELENDOK:
                   OnChangeRegViewMode(SendMessage(hwnd,CB_GETCURSEL,0,0));
               break;
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::OnChangeRegViewMode(int mode)
{
   LV_ITEM lvi={0};
   HWND hwnd;
   int i,regs;
   LString s,s1;

   if(mode == nViewRegMode)
       return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_REG);
   nViewRegMode = mode;
   ListView_DeleteAllItems(hwnd);
   lvi.mask = LVIF_TEXT;
   lvi.cchTextMax = 60;
   switch(mode){
       case 0:
           for(i=0;i<32;i++){
               s.Length(60);
               lvi.iItem = i;
               lvi.iSubItem = 0;
               psp.dis_reg((Registers)i,s.c_str(),100);
               s1 = s.NextToken(32);
               lvi.pszText = s1.c_str();
               ListView_InsertItem(hwnd,&lvi);
               s1 = s.NextToken(32);
               lvi.pszText = s1.c_str();
               lvi.iSubItem = 1;
               ListView_InsertItem(hwnd,&lvi);
               ListView_SetItemText(hwnd,i,1,s1.c_str());
           }
           for(i=PC;i<=HI;i++){
               s.Length(60);
               lvi.iItem = i;
               lvi.iSubItem = 0;
               psp.dis_reg((Registers)i,s.c_str(),100);
               s1 = s.NextToken(32);
               lvi.pszText = s1.c_str();
               ListView_InsertItem(hwnd,&lvi);
               s1 = s.NextToken(32);
               lvi.pszText = s1.c_str();
               lvi.iSubItem = 1;
               ListView_InsertItem(hwnd,&lvi);
               ListView_SetItemText(hwnd,i+32-PC,1,s1.c_str());
           }
       break;
       case 1:
           for(i=0;i<32;i++){
               s.Length(60);
               lvi.iItem = i;
               lvi.iSubItem = 0;
               psp.dis_reg((Registers)(i+32),s.c_str(),100);
               s1 = s.NextToken(32);
               lvi.pszText = s1.c_str();
               ListView_InsertItem(hwnd,&lvi);
               s1 = s.NextToken(32);
               lvi.pszText = s1.c_str();
               lvi.iSubItem = 1;
               ListView_InsertItem(hwnd,&lvi);
               ListView_SetItemText(hwnd,i,1,s1.c_str());
           }
       break;
       default:
           mode -= 2;
           for(i=0;i<16;i++){
               s.Length(60);
               lvi.iItem = i;
               lvi.iSubItem = 0;
               psp.dis_reg((Registers)(mode * 16 + i + 67),s.c_str(),100);
               s1 = s.NextToken(32);
               lvi.pszText = s1.c_str();
               ListView_InsertItem(hwnd,&lvi);
               s1 = s.NextToken(32);
               lvi.pszText = s1.c_str();
               lvi.iSubItem = 1;
               ListView_InsertItem(hwnd,&lvi);
               ListView_SetItemText(hwnd,i,1,s1.c_str());
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::OnCommand(WORD wID)
{
   switch(wID){
       case ID_DEBUG_EXIT:
           Close();
       break;
       case ID_DEBUG_OPEN_BREAKPOINTS:
           brkpointDlg.Create(m_hWnd);
           SendMessage(brkpointDlg.Handle(),WM_COMMAND,ID_BREAKPOINT_OPEN,0);
       break;
       case ID_DEBUG_VIEW_BREAKPOINT:
           brkpointDlg.Create(psp.Handle());
       break;
       case ID_DEBUG_VIEW_MEMORY:
           memDlg.Create(psp.Handle());
       break;
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::Close()
{
	bDebug = FALSE;
   bPass = TRUE;
   if(m_hWnd != NULL){
       DestroyWindow(m_hWnd);
       m_hWnd = NULL;
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::OnInitDialog()
{
   LV_COLUMN col={LVCF_FMT|LVCF_WIDTH,LVCFMT_LEFT,40,NULL,0};

	accel = LoadAccelerators(hInstance,MAKEINTRESOURCE(112));

	ListView_SetExtendedListViewStyle(GetDlgItem(m_hWnd,IDC_DEBUG_REG),LVS_EX_FULLROWSELECT);

   ListView_InsertColumn(GetDlgItem(m_hWnd,IDC_DEBUG_REG),0,&col);
   col.cx = 80;
   col.fmt = LVCFMT_LEFT;
   ListView_InsertColumn(GetDlgItem(m_hWnd,IDC_DEBUG_REG),1,&col);

   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"Integer Registers");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"FPU Registers");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"VFPU Registers - 0");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"VFPU Registers - 1");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"VFPU Registers - 2");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"VFPU Registers - 3");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"VFPU Registers - 4");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"VFPU Registers - 5");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"VFPU Registers - 6");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_ADDSTRING,0,(LPARAM)"VFPU Registers - 7");
   SendDlgItemMessage(m_hWnd,IDC_REG_MODE,CB_SETCURSEL,0,0);
   nViewRegMode = -1;
   OnChangeRegViewMode(0);
   Reset();
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::OnDialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   switch(uMsg){
       case WM_INITDIALOG:
           m_hWnd = (HWND)lParam;
           OnInitDialog();
       break;
       case WM_COMMAND:
           if(lParam == 0)
               OnCommand(LOWORD(wParam));
           else
               OnItemCommand(LOWORD(wParam),HIWORD(wParam),(HWND)lParam);
       break;
       case WM_CLOSE:
           Close();
       break;
       case WM_DRAWITEM:
           return OnDrawItem((WORD)wParam,(LPDRAWITEMSTRUCT)lParam);
       break;
       case WM_VSCROLL:
           OnVScroll(wParam,lParam);
       break;
       case WM_SIZE:
           OnSize((int)LOWORD(lParam),(int)HIWORD(lParam));
       break;
       case WM_NOTIFY:
           OnNotify(wParam,(LPNMHDR) lParam);
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
void LDebugDlg::OnNotify(int id,LPNMHDR param)
{
   switch(id){
       case IDC_DEBUG_REG:
           switch(param->code){
               case NM_RCLICK:
                   id = 0;
               break;
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::OnSize(int width,int height)
{
   RECT rc,rc1;
   HWND hwnd;
   HDWP hdwp;
   int x;

   SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,WM_SIZE,0,MAKELPARAM(width,height));

   GetWindowRect(GetDlgItem(m_hWnd,IDC_DEBUG_SB1),&rc1);
   height -= rc1.bottom - rc1.top;
   hdwp = BeginDeferWindowPos(3);
   if(hdwp == NULL)
       return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_REG);
   GetWindowRect(hwnd,&rc1);
   MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc1,2);
   rc1.left = rc1.right - rc1.left;
   rc1.right = rc1.left;
   rc1.left = width - rc1.left - 6;
   hdwp = DeferWindowPos(hdwp,hwnd,NULL,rc1.left,rc1.top,rc1.right,height - 4 - rc1.top,SWP_NOACTIVATE|SWP_NOZORDER);
   if(hdwp == NULL)
       return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_SB);
   GetWindowRect(hwnd,&rc);
   MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   x = rc.right - rc.left;
   rc.bottom = height - 4 - rc.top;
   hdwp = DeferWindowPos(hdwp,hwnd,NULL,rc1.left - x - 5,rc.top,x,rc.bottom,SWP_NOACTIVATE|SWP_NOZORDER);
   if(hdwp == NULL)
       return;
   x = rc1.left - x - 5;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_DIS);
   GetWindowRect(hwnd,&rc);
   MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   rc.right = x - 1 - rc.left;
   rc.bottom = height - rc.top - 4;
   hdwp = DeferWindowPos(hdwp,hwnd,NULL,0,0,rc.right,rc.bottom,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
   if(hdwp == NULL)
       return;
   iMaxItem = (rc.bottom / ::SendMessage(hwnd,LB_GETITEMHEIGHT,0,0));
   UpdateVertScrollBarDisDump(views[DBV_RUN].StartAddress,0);
   UpdateDissasemblerWindow(NULL,DBV_RUN,0);
   EndDeferWindowPos(hdwp);
}
//---------------------------------------------------------------------------
void LDebugDlg::OnVScroll(WPARAM wParam,LPARAM lParam)
{
   SCROLLINFO si={0};
   HWND hwnd;
   int i;
   DWORD dw,dw1;
   LPDISVIEW p1;

   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   if(!GetScrollInfo((hwnd = (HWND)lParam),SB_CTL,&si))
       return;
   switch(GetDlgCtrlID((HWND)lParam)){
       case IDC_DEBUG_SB:
           switch(LOWORD(wParam)){
               case SB_TOP:
                   i = 0;
               break;
               case SB_BOTTOM:
                   i = (si.nMax - si.nPage) + 1;
               break;
               case SB_PAGEDOWN:
                   i = yScroll + 4 * iMaxItem;
                   if(i > (int)((si.nMax - si.nPage)+1))
                       i = (si.nMax - si.nPage) + 1;
               break;
               case SB_PAGEUP:
                   if((i = yScroll - 4 * iMaxItem) < 0)
                       i = 0;
               break;
               case SB_LINEUP:
                   if((i = yScroll - 4) < 0)
                       i = 0;
               break;
               case SB_LINEDOWN:
                   i = yScroll + 4;
                   if(i > (int)((si.nMax - si.nPage)+1))
                       i = (si.nMax - si.nPage) + 1;
               break;
               case SB_THUMBPOSITION:
                   i = si.nPos;
               break;
               case SB_THUMBTRACK:
                   i = si.nTrackPos;
               break;
               default:
                   i = yScroll;
               break;
           }
           i = (i / 4) * 4;
           if(i == (int)yScroll)
               return;
           yScroll = i;
           ::SetScrollPos(hwnd,SB_CTL,i,TRUE);
           p1 = &views[iCurrentView];
           dw1 = p1->StartAddress;
           dw = dw1 & ~(0x02000000 - 1);
           dw |= i;
           UpdateDissasemblerWindow(&dw,DBV_VIEW,0);
       break;
   }
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::OnDrawItem(WORD wID,LPDRAWITEMSTRUCT lpDIS)
{
   char line[200];
   DWORD dwPos;
   RECT rc;
   int i,i1,i2;
   LBreakPoint *p2;
   HBRUSH hBrush,hOldBrush;
   HPEN hOldPen;
   COLORREF color;

	switch(wID){
       case IDC_DEBUG_DIS:
           if(lpDIS->itemID == -1)
               return TRUE;
           *(&rc) = *(&lpDIS->rcItem);
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXT,(WPARAM)lpDIS->itemID,(LPARAM)line);
           sscanf(line,"%08X",&dwPos);
           p2 = brkpointDlg.Find(dwPos);
           if(p2 != NULL){
               color = p2->is_Enable() ? RGB(255,0,0) : RGB(0,255,0);
               hBrush = CreateSolidBrush(color);
           }
           rc.right = rc.left + 12;
           FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_BTNFACE));
           *(&rc) = *(&lpDIS->rcItem);
           rc.left += 12;
           i = 0;
           if((lpDIS->itemState & ODS_SELECTED)){
               if((lpDIS->itemState & ODS_FOCUS) && psp.get_Register(PC) == dwPos){
                   SetBkColor(lpDIS->hDC,GetSysColor(COLOR_HIGHLIGHT));
                   SetTextColor(lpDIS->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
                   FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_HIGHLIGHT));
                   i = 1;
               }                                           //2000dba
           }
           if(i == 0){
               SetTextColor(lpDIS->hDC,GetSysColor(COLOR_WINDOWTEXT));
               if(p2 != NULL){
                   FillRect(lpDIS->hDC,&rc,hBrush);
                   SetBkColor(lpDIS->hDC,color);
               }
               else{
                   SetBkColor(lpDIS->hDC,GetSysColor(COLOR_WINDOW));
                   FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_WINDOW));
               }
               if(lpDIS->itemState & ODS_SELECTED)
                   DrawFocusRect(lpDIS->hDC,&rc);
           }
           if(p2 != NULL){
               *(&rc) = *(&lpDIS->rcItem);
               rc.left += 2;
               rc.right = rc.left + 7;
               rc.top = rc.top + (((rc.bottom - rc.top) - 7) >> 1);
               rc.bottom = rc.top + 7;
               hOldBrush = (HBRUSH)SelectObject(lpDIS->hDC,hBrush);
              	hOldPen = (HPEN)SelectObject(lpDIS->hDC,GetStockObject(BLACK_PEN));
               Ellipse(lpDIS->hDC,rc.left,rc.top,rc.right,rc.bottom);
               ::SelectObject(lpDIS->hDC,hOldBrush);
               ::SelectObject(lpDIS->hDC,hOldPen);
               ::DeleteObject(hBrush);
           }
           psp.dis_ins(dwPos,line,200);
           *(&rc) = *(&lpDIS->rcItem);
           SetBkMode(lpDIS->hDC,TRANSPARENT);
           for(i=0;line[i] != 0;){
               if(line[i++] == 32)
                   break;
           }
           rc.left += 18;
           ::DrawText(lpDIS->hDC,line,i,&rc,DT_NOCLIP|DT_LEFT|DT_SINGLELINE);
           for(i2=i,i1=0;line[i] != 0;i1++){
               if(line[i++] == 32)
                   break;
           }
           rc.left += 60;
           ::DrawText(lpDIS->hDC,&line[i2],i1,&rc,DT_NOCLIP|DT_LEFT|DT_SINGLELINE);
           rc.left += 60;
           ::DrawText(lpDIS->hDC,&line[i],-1,&rc,DT_NOCLIP|DT_LEFT|DT_SINGLELINE);
           return TRUE;
       default:
           return TRUE;
   }
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LDebugDlg *cl;

   if(uMsg == WM_INITDIALOG){
       SetWindowLong(hWnd,GWL_USERDATA,lParam);
       lParam = (LPARAM)hWnd;
   }
   cl = (LDebugDlg *)GetWindowLong(hWnd,GWL_USERDATA);
   if(cl != NULL)
       return cl->OnDialogProc(uMsg,wParam,lParam);
   return FALSE;
}
//---------------------------------------------------------------------------
void LDebugDlg::Update()
{
   UpdateDissasemblerWindow(NULL,DBV_RUN,0);
   UpdateRegisterWindow();
   memDlg.Update(FALSE);
}
//---------------------------------------------------------------------------
void LDebugDlg::Reset()
{
   HWND hwnd;
   RECT rc;

   current_thread = NULL;
   if(m_hWnd == NULL || !IsWindow(m_hWnd))
       return;
   ZeroMemory(views,sizeof(views));
   yScroll = 0;
   bTrackMode = 2;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_DIS);
   ::GetWindowRect(hwnd,&rc);
   iMaxItem = ((rc.bottom - rc.top) / ::SendMessage(hwnd,LB_GETITEMHEIGHT,0,0));
	bDebug = TRUE;
   bPass = FALSE;
   nBreakMode = 0;
   UpdateVertScrollBarDisDump(0x08000000,0);
   Update();
}
//---------------------------------------------------------------------------
void LDebugDlg::UpdateRegisterWindow()
{
   int i,index,cr_index;
   LString s,s1;
   psp_thread_list *list;
   psp_thread *p;
   DWORD dwPos;

   switch(nViewRegMode){
       case 0:
           for(i=0;i<32;i++){
               s.Length(100);
               psp.dis_reg((Registers)i,s.c_str(),100);
               s1 = s.NextToken(32);
               s1 = s.NextToken(32);
               ListView_SetItemText(GetDlgItem(m_hWnd,IDC_DEBUG_REG),i,1,s1.c_str());
           }
           for(i=PC;i<=HI;i++){
               s.Length(100);
               psp.dis_reg((Registers)i,s.c_str(),100);
               s1 = s.NextToken(32);
               s1 = s.NextToken(32);
               ListView_SetItemText(GetDlgItem(m_hWnd,IDC_DEBUG_REG),i+32-PC,1,s1.c_str());
           }
       break;
       case 1:
           for(i=0;i<32;i++){
               s.Length(100);
               psp.dis_reg((Registers)(i+32),s.c_str(),100);
               s1 = s.NextToken(32);
               s1 = s.NextToken(32);
               ListView_SetItemText(GetDlgItem(m_hWnd,IDC_DEBUG_REG),i,1,s1.c_str());
           }
       break;
       default:
           for(i=0;i<16;i++){
               s.Length(100);
               psp.dis_reg((Registers)(i+67+((nViewRegMode - 2) << 4)),s.c_str(),100);
               s1 = s.NextToken(32);
               s1 = s.NextToken(32);
               ListView_SetItemText(GetDlgItem(m_hWnd,IDC_DEBUG_REG),i,1,s1.c_str());
           }
       break;
   }
   SendDlgItemMessage(m_hWnd,IDC_THREAD,CB_RESETCONTENT,0,0);
   list = psp.get_threads();
   if(list == NULL)
       return;
   index = cr_index = CB_ERR;
   p = (psp_thread *)list->GetFirstItem(&dwPos);
   while(p != NULL){
       s = (int)p->res_id;
       s += " - ";
       s += p->name;
       s += " - ";
       s += (int)p->status;
       i = SendDlgItemMessage(m_hWnd,IDC_THREAD,CB_ADDSTRING,0,(LPARAM)s.c_str());
       if(i != CB_ERR){
           SendDlgItemMessage(m_hWnd,IDC_THREAD,CB_SETITEMDATA,i,(LPARAM)p);
           if(current_thread == p)
               cr_index = i;
           if(p == psp.get_current_thread())
               index = i;
       }
       p = (psp_thread *)list->GetNextItem(&dwPos);
   }
   if(cr_index != CB_ERR)
       index = cr_index;
   SendDlgItemMessage(m_hWnd,IDC_THREAD,CB_SETCURSEL,index,0);
}
//---------------------------------------------------------------------------
void LDebugDlg::UpdateDissasemblerWindow(DWORD *p,int indexView,int nItem)
{
   char s[10];
   LPDISVIEW p1;
	DWORD dwAddress;
   int i;

   if(p != NULL)
       dwAddress = *p;
   else
       dwAddress = psp.get_Register(PC);
   p1 = &views[iCurrentView = indexView];
   if(nItem != 0){
       if((dwAddress - p1->EndAddress) >= 4 || !dwAddress){
           p1->StartAddress = dwAddress;
           i = iMaxItem;
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_RESETCONTENT,0,0);
       }
       else{
           i = 1;
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_DELETESTRING,0,0);
           if(indexView == DBV_RUN)
               p1->StartAddress += 4;
       }
   }
   else{
       p1->StartAddress = dwAddress;
       i = iMaxItem;
       SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_RESETCONTENT,0,0);
   }
   for(;i > 0;i--,dwAddress+=4){
       wsprintf(s,"%08X",dwAddress);
       SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_INSERTSTRING,(WPARAM)-1,(LPARAM)s);
   }
   p1->EndAddress = dwAddress - 4;
   if(indexView == DBV_RUN)
		*(&views[DBV_VIEW]) = *p1;
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::CheckBreakpoints(DWORD adr)
{
   DWORD dw;

   if(!bPass)
       return FALSE;
   dw = psp.get_Register(PC);
   if(nBreakMode){
       if(dw == dwBreak){
           dwBreak = 0;
           nBreakMode = 0;
           bPass = FALSE;
           return TRUE;
       }
       return FALSE;
   }
   if(brkpointDlg.Check(adr,BT_PROGRAM))
       return TRUE;
   return FALSE;
}
//---------------------------------------------------------------------------
void LDebugDlg::Wait()
{
   LPDISVIEW p;
   DWORD dw;
   int item,i;
   char s[200];
   MSG msg;

   if(!bDebug)
       return;
   dw = psp.get_Register(PC);
   if(bPass){
       if(!CheckBreakpoints(dw))
           return;
       bPass = FALSE;
       current_thread = psp.get_current_thread();
   }
#ifdef _DEBPRO
   if(current_thread != NULL && psp.get_current_thread() != current_thread)
       return;
#endif
   bFlag = FALSE;
   p = &views[DBV_RUN];
   if(bTrackMode != 2){
       bTrackMode = 2;
       bFlag = TRUE;
   }
   else if(p->StartAddress == 0)
       bFlag = TRUE;
   else if(iCurrentView != DBV_RUN){
       /*       i = ((int)p->StartAddress - (int)dbg.views[DBV_VIEW].StartAddress);
       if(abs(i) < (dbg.iMaxItem >> 1)){
           if(i < 0){
               CopyMemory(p,&dbg.views[DBV_VIEW],sizeof(DISVIEW));*/
               iCurrentView = DBV_RUN;
           /*           }
       }
       else*/

           bFlag = TRUE;
   }
   else if(dw < p->StartAddress || dw > p->EndAddress)
       bFlag = TRUE;
   if(bFlag)
       UpdateDissasemblerWindow(NULL,DBV_RUN,1);
   UpdateRegisterWindow();
   memDlg.Update(TRUE);
   SetFocus(GetDlgItem(m_hWnd,IDC_DEBUG_DIS));
   item = SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETCOUNT,0,0);
   for(i=0;i<item;i++){
       SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXT,i,(LPARAM)s);
       if(StrToHex(s) == dw){
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_SETCURSEL,i,0);
           break;
       }
   }
   UpdateVertScrollBarDisDump(p->StartAddress,0);
   bFlag = TRUE;
   while(!bPass && bFlag && !bQuit){
       GetMessage(&msg,NULL,0,0);
       if(!IsDialogMessage(&msg)){
           TranslateMessage(&msg);
           DispatchMessage(&msg);
       }
       if(msg.message == WM_QUIT){
           return;
       }
       else if(msg.message != WM_KEYDOWN)
           continue;
       OnKeyDown(msg.wParam,msg.lParam);
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::OnKeyDown(WPARAM wParam,LPARAM lParam)
{
   DWORD dw;
   char s[20];
   int i;
   LBreakPoint *p;

   if(!bDebug || m_hWnd == NULL || !::IsWindow(m_hWnd))
   	return;
   switch(wParam){
       case VK_F4:
           bPass = FALSE;
       break;
       case VK_F5:
           bPass = TRUE;
           nBreakMode = 0;
       break;
       case VK_F6:
           i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXT,i,(LPARAM)s);
           dwBreak = StrToHex(s);
           nBreakMode = 1;
           bPass = TRUE;
       break;
#ifdef _DEBPRO
       case VK_F9:
           i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXT,i,(LPARAM)s);
           dw = StrToHex(s);
           p = brkpointDlg.Find(dw,BT_PROGRAM);
           if(p != NULL)
               p->set_Enable(!p->is_Enable());
           else
               p = brkpointDlg.Add(dw);
           if(p != NULL)
               brkpointDlg.Update();
           InvalidateRect(GetDlgItem(m_hWnd,IDC_DEBUG_DIS),NULL,TRUE);
           UpdateWindow(GetDlgItem(m_hWnd,IDC_DEBUG_DIS));
       break;
       case VK_F11:
           i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXT,i,(LPARAM)s);
           dwBreak = StrToHex(s);
           dwBreak += 8;
           nBreakMode = 1;
           bPass = TRUE;
       break;
#endif       
       case VK_F8:
           if(!bPass)
               bFlag = FALSE;
       break;
   }
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::IsDialogMessage(LPMSG p)
{
   if(p == NULL || m_hWnd == NULL || !IsWindow(m_hWnd))
       return FALSE;
 	if(GetActiveWindow() == m_hWnd && TranslateAccelerator(m_hWnd,accel,p))
		return TRUE;
  	if(::IsDialogMessage(m_hWnd,p))
  		return TRUE;
   return FALSE;
}
//---------------------------------------------------------------------------
void LDebugDlg::UpdateVertScrollBarDisDump(DWORD address,int index)
{
   SCROLLINFO si={0};
   HWND hwnd;
   int i;

   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_SB);
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   si.nMax = (32*1024*1024)-1;
   if(si.nMax != 0){
       i = 4;
       si.nPage = (iMaxItem * i);
       yScroll = (((address - 0x08000000) / i) * i) & (0x02000000 -1);
   }
   else{
       si.nPage = 1;
       yScroll = 0;
   }
   SetScrollInfo(hwnd,SB_CTL,&si,FALSE);
   SetScrollPos(hwnd,SB_CTL,yScroll,TRUE);
}
//---------------------------------------------------------------------------
LRESULT LDebugDlg::WindowProcListBoxDis(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LDebugDlg *cl;

   cl = (LDebugDlg *)GetWindowLong(hWnd,GWL_USERDATA);
   if(cl != NULL)
       return cl->OnWindowProcListBoxDis(uMsg,wParam,lParam);
   return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT LDebugDlg::OnWindowProcListBoxDis(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	HWND hWnd;
   int i;

   hWnd = GetDlgItem(m_hWnd,IDC_DEBUG_DIS);
	switch(uMsg){
   	case WM_KEYDOWN:
			switch(wParam){
       		case VK_NEXT:
           		::SendMessage(m_hWnd,WM_VSCROLL,MAKEWPARAM(SB_PAGEDOWN,0),
               			(LPARAM)GetDlgItem(m_hWnd,IDC_DEBUG_SB));
           		return 0;
           	case VK_PRIOR:
           		::SendMessage(m_hWnd,WM_VSCROLL,MAKEWPARAM(SB_PAGEUP,0),
               			(LPARAM)GetDlgItem(m_hWnd,IDC_DEBUG_SB));
           		return 0;
               case VK_UP:
               	if(::SendMessage(hWnd,LB_GETCURSEL,0,0) == 0){
           			::SendMessage(m_hWnd,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),
               			(LPARAM)GetDlgItem(m_hWnd,IDC_DEBUG_SB));
					   	::SendMessage(hWnd,LB_SETCURSEL,0,0);
               		return 0;
                   }
               break;
       		case VK_DOWN:
                   i = ::SendMessage(hWnd,LB_GETCOUNT,0,0);
               	if(::SendMessage(hWnd,LB_GETCURSEL,0,0) == i-1){
           			::SendMessage(m_hWnd,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),
               			(LPARAM)GetDlgItem(m_hWnd,IDC_DEBUG_SB));
					   	::SendMessage(hWnd,LB_SETCURSEL,i-1,0);
                       return 0;
               	}
               break;
           }
       break;
   }
   return CallWindowProc(WndProcOld,hWnd,uMsg,wParam,lParam);
}

