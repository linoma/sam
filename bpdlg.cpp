#include "resource.h"
#include "lpsp.h"
#include "util.h"
#include <stdio.h>
#include "bpdlg.h"

extern HINSTANCE hInstance;
extern BOOL bQuit;
//---------------------------------------------------------------------------
LBreakpointDlg::LBreakpointDlg() : LList()
{
    m_hWnd = NULL;
}
//---------------------------------------------------------------------------
LBreakpointDlg::~LBreakpointDlg()
{
   Save(NULL,TRUE);
   if(m_hWnd != NULL)
       DestroyWindow(m_hWnd);
   Clear();
}
//---------------------------------------------------------------------------
BOOL LBreakpointDlg::Create(HWND hwnd)
{
   if(m_hWnd == NULL){
       m_hWnd = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_DIALOG3),hwnd,(DLGPROC)DialogProc,(LPARAM)this);
       if(m_hWnd == NULL)
           return FALSE;
       SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
   }
   else
       BringWindowToTop(m_hWnd);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LBreakpointDlg::OnDialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   HWND hwnd;
   UINT i;

   switch(uMsg){
       case WM_INITMENUPOPUP:
           switch(lParam){
               case 0:
                   hwnd = GetDlgItem(m_hWnd,IDC_LV1);
                   if(ListView_GetItemCount(hwnd) < 1)
                       i = MF_GRAYED;
                   else
                       i = MF_ENABLED;
                   i |= MF_BYCOMMAND;
                   EnableMenuItem((HMENU)wParam,ID_BREAKPOINT_SAVE,i);
                   EnableMenuItem((HMENU)wParam,ID_BREAKPOINT_DELETE_ALL,i);
                   EnableMenuItem((HMENU)wParam,ID_BREAKPOINT_DISABLE_ALL,i);
                   EnableMenuItem((HMENU)wParam,ID_BREAKPOINT_ENABLE_ALL,i);
               break;
           }
       break;
       case WM_NOTIFY:
           if(wParam == IDC_LV1)
               OnNotifyListView((LPNMHDR)lParam);
       break;
       case WM_COMMAND:
           if(lParam == 0)
               OnCommand(LOWORD(wParam));
           else
               OnItemCommand(LOWORD(wParam),HIWORD(wParam),(HWND)lParam);
       break;
       case WM_INITDIALOG:
           m_hWnd = (HWND)lParam;
           OnInitDialog();
       break;
       case WM_SIZE:
           hwnd = GetDlgItem(m_hWnd,IDC_LV1);
           SetWindowPos(hwnd,NULL,1,1,LOWORD(lParam)-2,HIWORD(lParam)-2,SWP_NOZORDER);
       break;
       case WM_CLOSE:
           if(m_hWnd != NULL){
               DestroyWindow(m_hWnd);
               m_hWnd = NULL;
           }
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LBreakpointDlg::DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LBreakpointDlg *cl;

   if(uMsg == WM_INITDIALOG){
       SetWindowLong(hWnd,GWL_USERDATA,lParam);
       lParam = (LPARAM)hWnd;
   }
   cl = (LBreakpointDlg *)GetWindowLong(hWnd,GWL_USERDATA);
   if(cl != NULL)
       return cl->OnDialogProc(uMsg,wParam,lParam);
   return FALSE;
}
//---------------------------------------------------------------------------
void LBreakpointDlg::OnInitDialog()
{
   LV_COLUMN lvc={LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_SUBITEM,LVCFMT_LEFT,0,NULL,0,0};
   HWND hwnd;

   hwnd = GetDlgItem(m_hWnd,IDC_LV1);
	ListView_SetExtendedListViewStyle(hwnd,LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT);
   lvc.cx = 100;
   lvc.pszText = "Address";
   ListView_InsertColumn(hwnd,0,&lvc);
   lvc.cx = 200;
   lvc.pszText = "Condition";
   lvc.iSubItem = 1;
   ListView_InsertColumn(hwnd,1,&lvc);
   lvc.cx = 80;
   lvc.pszText = "Pass";
   lvc.iSubItem = 2;
   ListView_InsertColumn(hwnd,2,&lvc);
   lvc.cx = 300;
   lvc.pszText = "Description";
   lvc.iSubItem = 3;
   ListView_InsertColumn(hwnd,3,&lvc);

   Update();
}
//---------------------------------------------------------------------------
void LBreakpointDlg::OnNotifyListView(LPNMHDR p)
{
   NM_LISTVIEW *p1;
   LBreakPoint *pb;
   int item;
   LV_ITEM lvi;
   
   switch(p->code){
       case NM_DBLCLK:
           if((item = ListView_GetNextItem(p->hwndFrom,-1,LVNI_ALL|LVNI_SELECTED)) != -1){
               lvi.mask = LVIF_PARAM;
               lvi.iItem = item;
               ListView_GetItem(p->hwndFrom,&lvi);
               if((pb = (LBreakPoint *)lvi.lParam) != NULL){
                   item = (int)propDlg.Create(m_hWnd,pb);
               }
           }
       break;
		case LVN_KEYDOWN:
           switch(((LV_KEYDOWN *)p)->wVKey){
           	case VK_DELETE:
              		if((item = ListView_GetNextItem(p->hwndFrom,-1,LVNI_ALL|LVNI_SELECTED)) != -1){
						lvi.mask = LVIF_PARAM;
                       lvi.iItem = item;
                       ListView_GetItem(p->hwndFrom,&lvi);
		                if((pb = (LBreakPoint *)lvi.lParam) != NULL){
                       	LList::Delete(pb);
                       	ListView_DeleteItem(p->hwndFrom,lvi.iItem);
                           bModified = TRUE;
                       }
                   }
               break;
           }
       break;
       case LVN_ITEMCHANGED:
           p1 = (NM_LISTVIEW *)p;
       	if((p1->uChanged & LVIF_STATE)){
           	if((p1->uNewState & 0x3000)){
                   if((pb = (LBreakPoint *)p1->lParam) != NULL){
                   	pb->set_Enable(p1->uNewState & 0x2000 ? TRUE : FALSE);
                       bModified = TRUE;
                   }
               }
               if((p1->uNewState & 1) == 0)
                   EnableMenuItem(GetMenu(m_hWnd),1,MF_BYPOSITION|MF_GRAYED);
               else
                   EnableMenuItem(GetMenu(m_hWnd),1,MF_BYPOSITION|MF_ENABLED);
               DrawMenuBar(m_hWnd);
           }

       break;
   }
}
//---------------------------------------------------------------------------
void LBreakpointDlg::OnCommand(WORD wID)
{
   OPENFILENAME opn={0};
   LString s;
   DWORD dwPos;
   LBreakPoint *p;

   switch(wID){
       case ID_BREAKPOINT_DISABLE_ALL:
           p = (LBreakPoint *)GetFirstItem(&dwPos);
           while(p != NULL){
               p->set_Enable(FALSE);
               p = (LBreakPoint *)GetNextItem(&dwPos);
           }
           Update();
       break;
       case ID_BREAKPOINT_ENABLE_ALL:
           p = (LBreakPoint *)GetFirstItem(&dwPos);
           while(p != NULL){
               p->set_Enable(TRUE);
               p = (LBreakPoint *)GetNextItem(&dwPos);
           }
           Update();
       break;
       case ID_BREAKPOINT_DELETE_ALL:
           Clear();
           Update();
       break;
       case ID_BREAKPOINT_EXIT:
           if(m_hWnd != NULL){
               DestroyWindow(m_hWnd);
               m_hWnd = NULL;
           }
       break;
       case ID_BREAKPOINT_OPEN:
           s.Capacity(500);
           opn.lStructSize = sizeof(OPENFILENAME);
           opn.hInstance = hInstance;
           opn.hwndOwner = m_hWnd;
           opn.lpstrFilter = "BreakPoint Files (*.lst)\0*.lst\0All files (*.*)\0*.*\0\0\0\0\0";
           opn.Flags = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
           opn.lpstrFile = s.c_str();
           opn.nMaxFile = 499;
           if(GetOpenFileName(&opn)){
               if(Load(s.c_str()))
                   Update();
           }
       break;
       case ID_BREAKPOINT_SAVE:
           Save(NULL,FALSE);
       break;
       case ID_BREAKPOINT_NEW:
           propDlg.Create(m_hWnd);
       break;
   }
}
//---------------------------------------------------------------------------
void LBreakpointDlg::Update()
{
   int i;
   HWND hwnd;
   LBreakPoint *p;
   DWORD dwPos;
   LV_ITEM lvi={0};
   char s[100];

   if(m_hWnd == NULL || !IsWindow(m_hWnd))
       return; 
   hwnd = GetDlgItem(m_hWnd,IDC_LV1);
   ListView_DeleteAllItems(hwnd);
   i = 0;
   p = (LBreakPoint *)GetFirstItem(&dwPos);
   while(p != NULL){
       lvi.mask = LVIF_TEXT;
       lvi.iItem = i;
       lvi.iSubItem = 0;
       if(p->get_Type() == BT_MEMORY && p->has_Range())
           wsprintf(s,"0x%08X - 0x%08X",p->get_Address(),p->get_Address2());
       else
           wsprintf(s,"0x%08X",p->get_Address());
       lvi.pszText = s;
       lvi.iSubItem = 0;
       ListView_InsertItem(hwnd,&lvi);
       ListView_SetItemState(hwnd,lvi.iItem,INDEXTOSTATEIMAGEMASK(p->is_Enable()?2:1),
           LVIS_STATEIMAGEMASK);
       lvi.mask = LVIF_PARAM;
       lvi.lParam = (LPARAM)p;
       ListView_SetItem(hwnd,&lvi);
       p = (LBreakPoint *)GetNextItem(&dwPos);
   }
}
//---------------------------------------------------------------------------
void LBreakpointDlg::OnItemCommand(WORD wId,WORD wNotifyCode,HWND hWnd)
{
}
//---------------------------------------------------------------------------
LBreakPoint *LBreakpointDlg::Add(unsigned long address,int type)
{
	LBreakPoint *p;

	if(Find(address,type) != NULL)
   	return NULL;
   if((p = new LBreakPoint()) == NULL)
   	return NULL;
	if(!Add(p)){
   	delete p;
       return NULL;
   }
   p->set_Enable();
   p->set_Type(type);
   p->set_Address(address);
	return p;
}
//---------------------------------------------------------------------------
BOOL LBreakpointDlg::Delete(DWORD item,BOOL bFlag)
{
	if(!LList::Delete(item,bFlag))
   	return FALSE;
   bModified = TRUE;
//   currentType = -1;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LBreakpointDlg::Add(LBreakPoint *p)
{
	if(!LList::Add((LPVOID)p))
   	return FALSE;
   bModified = TRUE;
//   currentType = -1;
   return TRUE;
}
//---------------------------------------------------------------------------
LBreakPoint *LBreakpointDlg::Check(unsigned long address,int accessMode)
{
    elem_list *tmp;
    LBreakPoint *p;
    int type;

    type = accessMode == 0 ? BT_PROGRAM : BT_MEMORY;
    tmp = First;
    while(tmp != NULL){
        p = (LBreakPoint *)tmp->Ele;
        if(p->is_Enable()){
            if(p->get_Type() == type && p->Check(address,accessMode))
                return p;
        }
        tmp = tmp->Next;
    }
    return NULL;
}
//---------------------------------------------------------------------------
LBreakPoint *LBreakpointDlg::Find(unsigned long address,int type)
{
	elem_list *tmp;
	LBreakPoint *p;

	tmp = First;
   while(tmp != NULL){
   	p = (LBreakPoint *)tmp->Ele;
       if(p->get_Type() == type && p->get_Address() == address)
       	return p;
   	tmp = tmp->Next;
   }
   return NULL;
}
//---------------------------------------------------------------------------
void LBreakpointDlg::Reset()
{
    elem_list *tmp;
    
    tmp = First;
    while(tmp != NULL){
        ((LBreakPoint *)tmp->Ele)->Reset();
        tmp = tmp->Next;
    }
}
//---------------------------------------------------------------------------
BOOL LBreakpointDlg::Save(const char *lpFileName,BOOL bForce)
{
   OPENFILENAME opn={0};
	LFile *pFile;
	elem_list *tmp;
	LBreakPoint *p;
	LString s;

	if(nCount == 0)
   	return TRUE;
   if(!bForce && !bModified)
   	return TRUE;
   if(lpFileName == NULL || lpFileName[0] == 0)
   	s = fileName;
   else
   	s = (char *)lpFileName;
   if(s.IsEmpty()){
       s.Capacity(500);
       opn.lStructSize = sizeof(OPENFILENAME);
       opn.hInstance = hInstance;
       opn.hwndOwner = m_hWnd;
       opn.lpstrFilter = "BreakPoint Files (*.lst)\0*.lst\0All files (*.*)\0*.*\0\0\0\0\0";
       opn.Flags = OFN_CREATEPROMPT|OFN_OVERWRITEPROMPT;
       opn.lpstrDefExt = "lst";
       opn.lpstrFile = s.c_str();
       opn.nMaxFile = 499;
   	if(!GetSaveFileName(&opn))
       	return FALSE;
   }
   if(s.IsEmpty() || (pFile = new LFile(s.c_str())) == NULL)
   	return FALSE;
   if(!pFile->Open(GENERIC_WRITE,CREATE_ALWAYS)){
   	delete pFile;
       return FALSE;
   }
   s = "VER\1";
   pFile->Write(s.c_str(),4);

	tmp = First;
   while(tmp != NULL){
   	p = (LBreakPoint *)tmp->Ele;
       p->Write(pFile);
   	tmp = tmp->Next;
   }
   delete pFile;
   bModified = FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LBreakpointDlg::Load(const char *lpFileName)
{
	LFile *pFile;
	LBreakPoint *p;
	BOOL res;
   char c[10],ver;

   if(lpFileName == NULL || (pFile = new LFile(lpFileName)) == NULL)
   	return FALSE;
   if(!pFile->Open()){
   	delete pFile;
       return FALSE;
   }
   pFile->Read(c,4);
   ver = c[3];
   c[3] = 0;
   if(lstrcmpi(c,"VER")){
       ver = 0;
       pFile->SeekToBegin();
   }
	Clear();
   res = TRUE;
   while(1){
		if((p = new LBreakPoint()) != NULL){
   		if(!p->Read(pFile,ver)){
				delete p;
				break;
           }
      		if(!Add(p))
           	delete p;
       }
   }
   delete pFile;
   if(res){
   	fileName = (char *)lpFileName;
       bModified = FALSE;
   }
   return res;
}
//---------------------------------------------------------------------------
void LBreakpointDlg::Clear()
{
    LList::Clear();
    fileName = "";
}
//---------------------------------------------------------------------------
void LBreakpointDlg::DeleteElem(LPVOID p)
{
    delete (LBreakPoint *)p;
}
//---------------------------------------------------------------------------
LBreakpointPropertiesDlg::LBreakpointPropertiesDlg()
{
   m_hWnd = NULL;
   breakpoint = NULL;
}
//---------------------------------------------------------------------------
LBreakpointPropertiesDlg::~LBreakpointPropertiesDlg()
{
   m_hWnd = NULL;
   breakpoint = NULL;
}
//---------------------------------------------------------------------------
BOOL LBreakpointPropertiesDlg::Create(HWND hwnd,LBreakPoint *p)
{
   breakpoint = p;
   return (DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DIALOG4),hwnd,
       (DLGPROC)DialogProc,(LPARAM)this) == IDOK ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
BOOL LBreakpointPropertiesDlg::OnDialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   switch(uMsg){
       case WM_INITDIALOG:
           OnInitDialog((HWND)lParam);
       break;
       case WM_CLOSE:
           EndDialog(m_hWnd,IDCANCEL);
       break;
       case WM_COMMAND:
           if(lParam == 0){
           }
           else
               OnItemCommand(LOWORD(wParam),HIWORD(wParam),(HWND)lParam);
       break;
       case WM_DESTROY:
           m_hWnd = NULL;
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LBreakpointPropertiesDlg::DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LBreakpointPropertiesDlg *cl;

   if(uMsg == WM_INITDIALOG){
       SetWindowLong(hWnd,GWL_USERDATA,lParam);
       lParam = (LPARAM)hWnd;
   }
   cl = (LBreakpointPropertiesDlg *)GetWindowLong(hWnd,GWL_USERDATA);
   if(cl != NULL)
       return cl->OnDialogProc(uMsg,wParam,lParam);
   return FALSE;
}
//---------------------------------------------------------------------------
void LBreakpointPropertiesDlg::OnInitDialog(HWND hwnd)
{
   char s[200];

   m_hWnd = hwnd;
   if(breakpoint != NULL){
       if(breakpoint->get_Type() == BT_PROGRAM){
           wsprintf(s,"0x%08X",breakpoint->get_Address());
           SetDlgItemText(m_hWnd,IDC_EDIT1,s);
           SendDlgItemMessage(m_hWnd,IDC_RADIO1,BM_SETCHECK,BST_CHECKED,0);
           EnableWindow(GetDlgItem(m_hWnd,IDC_RADIO2),FALSE);
       }
       else{
           SendDlgItemMessage(m_hWnd,IDC_RADIO2,BM_SETCHECK,BST_CHECKED,0);
           EnableWindow(GetDlgItem(m_hWnd,IDC_RADIO1),FALSE);
       }
       SetDlgItemText(m_hWnd,IDC_EDIT3,
           breakpoint->ConditionToString(breakpoint->get_Type()).c_str());
       wsprintf(s,"%d",breakpoint->get_PassCount());
       SetDlgItemText(m_hWnd,IDC_EDIT4,s);
       breakpoint->get_Description(s);
       SetDlgItemText(m_hWnd,IDC_EDIT5,s);
   }
   else{
       SendDlgItemMessage(m_hWnd,IDC_RADIO1,BM_SETCHECK,BST_CHECKED,0);
       SetDlgItemText(m_hWnd,IDC_EDIT2,"4");
   }
   ::SetFocus(GetDlgItem(m_hWnd,IDC_EDIT1));
}
//---------------------------------------------------------------------------
void LBreakpointPropertiesDlg::OnItemCommand(WORD wID,WORD wNotifyCode,HWND hwnd)
{
   switch(wID){
       case IDCANCEL:
           EndDialog(m_hWnd,IDCANCEL);
       break;
       case IDOK:
           EndDialog(m_hWnd,IDOK);
       break;
       case IDC_RADIO1:
           switch(wNotifyCode){
               case BN_CLICKED:
                   EnableWindow(GetDlgItem(m_hWnd,IDC_EDIT2),FALSE);
               break;
           }
       break;
       case IDC_RADIO2:
           switch(wNotifyCode){
               case BN_CLICKED:
                   EnableWindow(GetDlgItem(m_hWnd,IDC_EDIT2),TRUE);
               break;
           }
       break;
   }
}
