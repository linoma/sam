#include "resource.h"
#include "lpsp.h"
#include "util.h"
#include <stdio.h>
#include <ctype.h>
#include "memviewdlg.h"


extern HINSTANCE hInstance;
extern BOOL bQuit;
//---------------------------------------------------------------------------
LMemoryDlg::LMemoryDlg()
{
    m_hWnd = NULL;
    lpszText = NULL;
    nMode = 0;
    yScroll = 0;
}
//---------------------------------------------------------------------------
LMemoryDlg::~LMemoryDlg()
{
   Destroy();
}
//---------------------------------------------------------------------------
BOOL LMemoryDlg::Create(HWND hwnd)
{
   if(m_hWnd == NULL){
       m_hWnd = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_DIALOG2),psp.Handle(),(DLGPROC)DialogProc,(LPARAM)this);
       if(m_hWnd == NULL)
           return FALSE;
       SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
       Update(TRUE);
   }
   else
       BringWindowToTop(m_hWnd);
   return TRUE;
}
//---------------------------------------------------------------------------
void LMemoryDlg::Destroy()
{
   if(m_hWnd != NULL)
       DestroyWindow(m_hWnd);
   if(lpszText != NULL)
       delete lpszText;
   m_hWnd = NULL;
   lpszText = NULL;
}
//---------------------------------------------------------------------------
BOOL LMemoryDlg::OnDialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
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
       case WM_VSCROLL:
           OnVScroll(wParam,lParam);
       break;
       case WM_CLOSE:
           Destroy();
       break;
       case WM_SIZE:
           OnSize((int)LOWORD(lParam),(int)HIWORD(lParam));
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LMemoryDlg::DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LMemoryDlg *cl;

   if(uMsg == WM_INITDIALOG){
       SetWindowLong(hWnd,GWL_USERDATA,lParam);
       lParam = (LPARAM)hWnd;
   }
   cl = (LMemoryDlg *)GetWindowLong(hWnd,GWL_USERDATA);
   if(cl != NULL)
       return cl->OnDialogProc(uMsg,wParam,lParam);
   return FALSE;

}
//---------------------------------------------------------------------------
void LMemoryDlg::OnSize(int width,int height)
{
   RECT rc,rc1;
   HWND hwnd;
   HDWP hdwp;

   SendDlgItemMessage(m_hWnd,IDC_STATUSBAR,WM_SIZE,0,MAKELPARAM(width,height));
   GetWindowRect(GetDlgItem(m_hWnd,IDC_STATUSBAR),&rc1);
   height -= rc1.bottom - rc1.top;
   hdwp = BeginDeferWindowPos(2);
   if(hdwp == NULL)
       return;
   hwnd = GetDlgItem(m_hWnd,IDC_MEMORY_VERTSB);
   GetWindowRect(hwnd,&rc1);
   MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc1,2);
   rc1.left = rc1.right - rc1.left;
   rc1.right = rc1.left;
   rc1.left = (width - 6) - rc1.left;
   hdwp = DeferWindowPos(hdwp,hwnd,NULL,rc1.left,rc1.top,rc1.right,height - 4 - rc1.top,SWP_NOACTIVATE|SWP_NOZORDER);
   if(hdwp == NULL)
       return;
   hwnd = GetDlgItem(m_hWnd,IDC_MEMORY_VIEW);
   GetWindowRect(hwnd,&rc);
   MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   rc.right = rc1.left - 1 - rc.left;
   rc.bottom = height - 4 - rc1.top;
   hdwp = DeferWindowPos(hdwp,hwnd,NULL,0,0,rc.right,height - 4 - rc1.top,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
   if(hdwp == NULL)
       return;
   Update(TRUE);
   EndDeferWindowPos(hdwp);
}
//---------------------------------------------------------------------------
void LMemoryDlg::OnInitDialog()
{
   HDC hdc;

   hFont = (HFONT)SendMessage(m_hWnd,WM_GETFONT,0,0);
   m_hWndMemoryView = GetDlgItem(m_hWnd,IDC_MEMORY_VIEW);
   if((hdc = GetDC(m_hWndMemoryView)) != NULL){
       SelectObject(hdc,hFont);
       GetTextExtentPoint32(hdc,"0",1,&szNumberFont);
       GetTextExtentPoint32(hdc,"W",1,&szLetterFont);
       ReleaseDC(m_hWndMemoryView,hdc);
   }
   nMode = 0;
   UpdateScrollBar(TRUE);
   Update(FALSE);
   SetWindowLong(m_hWndMemoryView,GWL_USERDATA,(LONG)this);
   MemViewWndProc = (WNDPROC)SetWindowLong(m_hWndMemoryView,GWL_WNDPROC,(LONG)WindowProcStaticMemView);
}
//---------------------------------------------------------------------------
void LMemoryDlg::OnCommand(WORD wID)
{
}
//---------------------------------------------------------------------------
void LMemoryDlg::OnItemCommand(WORD wId,WORD wNotifyCode,HWND hWnd)
{
   char s[30],*p;
   DWORD dw;
   POINT pt;

   switch(wId){
       case IDC_MEMORY_GO:
           GetDlgItemText(m_hWnd,IDC_MEMORY_EDIT,s,30);
           strlwr(s);
           if(strchr(s,'r') != NULL){
               dw = (DWORD)psp.get_Register(s);
           }
           else
               dw = StrToHex(s);
           yScroll = (dw-0x08000000);
           UpdateScrollBar(TRUE);
           Update(TRUE);
       break;
       case IDC_MEMORY_VIEW:
           switch(wNotifyCode){
               case STN_DBLCLK:
                   GetCursorPos(&pt);
                   ::ScreenToClient(hWnd,&pt);
                   if(!PointToAddress(pt,&dw))
                       return;
                   wsprintf(s,"0x%08X",dw);
                   SendDlgItemMessage(m_hWnd,IDC_STATUSBAR,SB_SETTEXT,0,(LPARAM)s);
               break;
           }
       break;
   }
}
//---------------------------------------------------------------------------
LRESULT LMemoryDlg::WindowProcStaticMemView(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LMemoryDlg *cl;

   cl = (LMemoryDlg *)GetWindowLong(hWnd,GWL_USERDATA);
   if(cl != NULL)
       return cl->OnWindowProcStaticMemView(uMsg,wParam,lParam);
   return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT LMemoryDlg::OnWindowProcStaticMemView(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   PAINTSTRUCT ps;

   switch(uMsg){
       case WM_ERASEBKGND:
           return 1;
       case WM_PAINT:
           BeginPaint(m_hWndMemoryView,&ps);
           OnPaint(ps.hdc,ps.rcPaint);
           EndPaint(m_hWndMemoryView,&ps);
           return 0;
       case WM_RBUTTONUP:
           if(wParam == 0){
               OnRButtonUp(LOWORD(lParam),HIWORD(lParam));
               return 0;
           }
       break;
   }
   return CallWindowProc(MemViewWndProc,m_hWndMemoryView,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
void LMemoryDlg::OnRButtonUp(int xPos,int yPos)
{
}
//---------------------------------------------------------------------------
void LMemoryDlg::UpdateScrollBar(BOOL bRepos)
{
    SIZE sz1;
    SCROLLINFO si={0};
    HWND hwnd;
    DWORD dw;

    hwnd = GetDlgItem(m_hWnd,IDC_MEMORY_VERTSB);
    get_ItemsPage(&sz1);
    switch(nMode){
        case 1:
            sz1.cx *= 2;
        break;
        case 2:
            sz1.cx *= 4;
        break;
    }
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    dw = ((0x02000000 - 1) / sz1.cx);
    si.nMax = (dw + 1) * sz1.cx;
    si.nPage = si.nMax > 0 ? sz1.cx*sz1.cy : 1;
    SetScrollInfo(hwnd,SB_CTL,&si,FALSE);
    if(!bRepos)
        return;
    SetScrollPos(hwnd,SB_CTL,yScroll,TRUE);
}
//---------------------------------------------------------------------------
void LMemoryDlg::OnPaint(HDC hdc,RECT &rcClip)
{
	char *p,adr[30],*p1;
   int y,x,badr,bsel,btex,x1,x2;
	char c;
   RECT rc;
	HDC hdc1;
   SIZE sz1;

   hdc1 = hdc;
   ::GetClientRect(m_hWndMemoryView,&rc);
	FillRect(hdc1,&rc,GetSysColorBrush(COLOR_WINDOW));
	if(lpszText == NULL)
   	return;
	x = y = 1;
   btex = 0;
   SelectObject(hdc1,hFont);
   SetTextAlign(hdc1,TA_NOUPDATECP);
	SetBkMode(hdc1,OPAQUE);
   get_ItemsPage(&sz1);
   x1 = sz1.cx;
   get_ItemSize(&sz1);
   x1 = x1 * sz1.cx + 11 * szNumberFont.cx + 5;
   x2 = szNumberFont.cx;
   for(p = lpszText,p1 = adr,badr = 0;*p != 0;p++){
   	c = *p;
   	if(c == 13){
       	*p++;
   		y += szNumberFont.cy;
           x = 1;
           x2 = szNumberFont.cx;
           badr = btex = 0;
           if(*p != 0)
				continue;
           else
           	break;
       }
       else if(c == 10){
           btex = 1;
           SetTextColor(hdc1,GetSysColor(COLOR_WINDOWTEXT));
   		SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
           x = x1;
           x2 = szLetterFont.cx;
           continue;
       }
       if(!badr){
           *p1++ = c;
           if(c == ':'){
           	badr = 1;
               *p1++ = *(++p);
               *p1 = 0;
               p1 = adr;
   			SetTextColor(hdc1,RGB(0,0,200));
   			SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
       		::SetRect(&rc,x,y,x+11*szNumberFont.cx,y+szNumberFont.cy);
       		ExtTextOut(hdc1,x,y,ETO_OPAQUE,&rc,p1,lstrlen(p1),NULL);
               x += 11 * x2;
               bsel = 0;
           }
       	continue;
       }
       if(!bsel && !btex){
           switch((c&0xf) - 1){
				case 1:
   				SetTextColor(hdc1,GetSysColor(COLOR_HIGHLIGHTTEXT));
   				SetBkColor(hdc1,GetSysColor(COLOR_HIGHLIGHT));
               break;
               case 2:
               	if((c >> 4)){
           			SetBkColor(hdc1,RGB(0x80,0,0));
           			SetTextColor(hdc1,GetSysColor(COLOR_WINDOW));
               	}
               	else{
           			SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
           			SetTextColor(hdc1,RGB(0xFF,0,0));
               	}
				break;
				case 3:
               	if((c >> 4)){
           			SetBkColor(hdc1,RGB(0,0x80,0));
           			SetTextColor(hdc1,GetSysColor(COLOR_WINDOW));
               	}
               	else{
           			SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
           			SetTextColor(hdc1,RGB(0,0xFF,0));
               	}
				break;
               default:
               	if(c > 1)
                   	c = c;
   				SetTextColor(hdc1,GetSysColor(COLOR_WINDOWTEXT));
   				SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
               break;
           }
           bsel = 1;
           continue;
       }
       if(c == 32)
       	bsel = 0;
       ::SetRect(&rc,x,y,x+szNumberFont.cx,y+szNumberFont.cy);
       ::DrawText(hdc1,p,1,&rc,DT_LEFT|DT_NOCLIP|DT_SINGLELINE|DT_TOP);
       x += x2;
   }
}
//---------------------------------------------------------------------------
void LMemoryDlg::Update(BOOL bInvalidate)
{
   U64 dw,dwMax;
	DWORD dw1,dw2,dwSelStart,dwSelEnd;                      //4ce08
   int i,i1,i3,x;
   char s[31],bRead,cr[]={13,10,0},c,s1[40];
	SIZE sz1;
	BOOL bSel;

   if(m_hWnd == NULL)
       return;
	if(lpszText == NULL && (lpszText = new char[20000]) == NULL)
       return;
	((long *)lpszText)[0] = 0;
	if(!get_ItemsPage(&sz1))
   	return;
   dw = 0x08000000;
	dwMax = dw;
   dw += yScroll;
	i3 = 0x02000000;
	dwMax += i3;
   i3 = sz1.cx;
   bRead = 1;
   for(x=i=0;i<sz1.cy && dw < dwMax;i++){
       x += 11;
       wsprintf(s,"%08X : ",dw);
       lstrcat(lpszText,s);
       *((int *)s1) = 0;
       for(i1=0;i1<i3 && dw < dwMax;i1++){
       	dw2 = (DWORD)dw;
           switch(nMode){
               case 0:
                   if(bRead != 0)
                       dw1 = (DWORD)(BYTE)read_byte(dw2);
                   else
                       dw1 = 0;
                   wsprintf(s+1,"%02X ",dw1);
                   if(isprint(dw1))
                       s1[i1] = (char)dw1;
                   else
                       s1[i1] = '.';
                   s1[i1+1] = 0;
                   x += 3;
                   dw++;
               break;
               case 1:
                   if(bRead != 0)
                       dw1 = (DWORD)(WORD)read_word(dw2);
                   else
                       dw1 = 0;
                   dw += 2;
                   wsprintf(s+1,"%04X ",dw1);
                   x += 5;
               break;
               case 2:
                   if(bRead != 0)
                       dw1 = read_dword(dw2);
                   else
                       dw1 = 0;
                   dw += 4;
                   wsprintf(s+1,"%08X ",dw1);
                   x += 9;
               break;
           }
		    c = 1;
			s[0] = c;
           lstrcat(lpszText,s);
       }
       if(*s1){
           dw1 = 10;
           lstrcat(lpszText,(char *)&dw1);
           lstrcat(lpszText,s1);
       }
       lstrcat(lpszText,cr);
   }
   if(bInvalidate)
   	InvalidateRect(m_hWndMemoryView,NULL,FALSE);
   UpdateWindow(m_hWndMemoryView);
}
//---------------------------------------------------------------------------
BOOL LMemoryDlg::get_ItemSize(LPSIZE s)
{
   if(s == NULL)
   	return FALSE;
   switch(nMode){
   	case 0:
       	s->cx = 3*szNumberFont.cx;
       break;
       case 1:
       	s->cx = 5*szNumberFont.cx;
       break;
       case 2:
       	s->cx = 9*szNumberFont.cx;
       break;
   }
	s->cy = szNumberFont.cy;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LMemoryDlg::get_ItemsPage(LPSIZE s)
{
    RECT rc;
    SIZE sz1;
    int i;

    if(s == NULL || !get_ItemSize(&sz1))
        return FALSE;
    ::GetClientRect(m_hWndMemoryView,&rc);
    rc.right -= 11 * szNumberFont.cx;
    s->cx = rc.right / sz1.cx;
    s->cy = rc.bottom / szNumberFont.cy;
    if(nMode == 0){
        for(;;){
            i = s->cx * szLetterFont.cx;
            if(rc.right - (s->cx*sz1.cx) < i)
                s->cx--;
            else
                break;
        }
    }
    return TRUE;
}
//---------------------------------------------------------------------------
void LMemoryDlg::OnVScroll(WPARAM wParam,LPARAM lParam)
{
   SCROLLINFO si={0};
   HWND hwnd;
   int i;
   SIZE sz1;
   int inc;

   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   if(!GetScrollInfo((hwnd = (HWND)lParam),SB_CTL,&si))
       return;
    get_ItemsPage(&sz1);
    inc = (si.nPage / sz1.cy);
    switch(LOWORD(wParam)){
        case SB_PAGEUP:
            if((i = yScroll - si.nPage) < 0)
                i = 0;
            break;
        case SB_TOP:
            i = 0;
            break;
        case SB_BOTTOM:
            i = (si.nMax - si.nPage) + 1;
            break;
        case SB_PAGEDOWN:
            i = yScroll + si.nPage;
            if(i > (int)((si.nMax - si.nPage)+1))
                i = (si.nMax - si.nPage) + 1;
            break;
        case SB_LINEUP:
            i = yScroll - inc;
            if(i < 0)
                i = 0;
            break;
        case SB_LINEDOWN:
            i = yScroll + inc;
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
    i = (i / inc) * inc;
    if(i == yScroll)
        return;
    yScroll = i;
    si.nPos = i;
    si.fMask = SIF_POS;
    SetScrollInfo(hwnd,SB_CTL,&si,TRUE);
    Update(TRUE);
}
//---------------------------------------------------------------------------
BOOL LMemoryDlg::PointToAddress(POINT &pt,DWORD *address)
{
   SIZE s;
   int x,y,xi,yi;

	if(address == NULL || !get_ItemsPage(&s))
   	return FALSE;
   *address = 0;
   if(pt.x < 11 * szNumberFont.cx)
   	return FALSE;
   x = (pt.x - 11 * szNumberFont.cx) / szNumberFont.cx;
   y = pt.y / szNumberFont.cy;
	switch(nMode){
       case 0:
           xi = 3;
           yi = 1;
       break;
   	case 1:
           xi = 5;
           yi = 2;
       break;
       case 2:
       	xi = 9;
           yi = 4;
       break;
   }
   if(pt.x > (11+(s.cx*xi))*szNumberFont.cx)
       return FALSE;
   x /= xi;
   y *= yi * s.cx;
   *address = 0x08000000 + (x * yi) + y + yScroll;
   pt.x = 11 * szNumberFont.cx + x * xi * szNumberFont.cx;
   pt.y = (y/yi/s.cx) * szNumberFont.cy;
   return TRUE;
}

