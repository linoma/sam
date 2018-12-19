#include <windows.h>
#ifdef __BORLANDC__
#pragma hdrstop
#include <condefs.h>
#endif
#include "lpsp.h"
#include <stdio.h>
#include <conio.h>

//---------------------------------------------------------------------------
#ifdef __BORLANDC__
USEUNIT("lpsp.cpp");
USEUNIT("lgpu.cpp");
USEUNIT("lmmu.cpp");
USEUNIT("cpu\r4000.cpp");
USEFILE("resource.h");
USERC("sam_ex.rc");
USEUNIT("debugdlg.cpp");
USEUNIT("llist.cpp");
USEUNIT("lslist.cpp");
USEUNIT("lstring.cpp");
USEUNIT("fstream.cpp");
USEUNIT("cpu\r4000dis.cpp");
USEUNIT("util.cpp");
USEUNIT("os.cpp");
USEUNIT("lzari.cpp");
USEUNIT("sce.cpp");
USEUNIT("syscall.cpp");
USEUNIT("ldsp.cpp");
USEUNIT("bp.cpp");
USEUNIT("memviewdlg.cpp");
USEUNIT("bpdlg.cpp");
USEUNIT("cpu\vfpu.cpp");
USEUNIT("lregkey.cpp");
#endif
//---------------------------------------------------------------------------
HINSTANCE hInstance;
LPSP psp;
BOOL bQuit;
//---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   hInstance = hinst;
   psp.Init();
   psp.OnLoop();
   return 0;
}
 
