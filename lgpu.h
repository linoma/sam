#include <windows.h>
#include <f:\borland\include\gl\gl.h>
#include <f:\borland\include\gl\glext.h>
#include <f:\borland\include\gl\wglext.h>
#include "debugdlg.h"
#include "os.h"
//---------------------------------------------------------------------------
#ifndef lgpuH
#define lgpuH
//---------------------------------------------------------------------------

class LGPU
{
public:
   LGPU();
   ~LGPU();
   BOOL Init();
   void Reset();
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   static LRESULT WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   inline HWND Handle(){return m_hWnd;};
   int add_DisplayList(LPGPU_LIST ls);
   int draw_DisplayList(int syncType);
   LPGPU_LIST get_DisplayList(DWORD index){return (LPGPU_LIST)display_lists.GetItem(index);};
   int ge_SwapBuffer(unsigned long adr,int _width,int _mode,int ukn);
   int ge_SetMode(int m,int w,int h);
   inline int get_Status(){return status;};
   inline BOOL get_VideoMemoryStatus(){return bModifiedVideoMemory;};
   inline void set_VideoMemoryStatus(){bModifiedVideoMemory = TRUE;};
   void BitBlt();
   inline LPBYTE get_VideoMemory(){return screen;};
   void set_CallbackData(unsigned long adr);
protected:
   void OnCommand(WORD wID);
   void read_color();
   void move_index();
   void get_value(int size,int len,float *v);
   void gen_texture(int index);
   void gen_clut();
   HWND m_hWnd;
   HBITMAP hBitmap;
   HDC hDC;
   HGLRC hRC;
   WNDPROC oldWndProc;
   BITMAPINFO bminfo;
   LPBYTE screen,out_buffer;
   BOOL bRom,bStart,bModifiedVideoMemory;
   int cycles,lines,status,mode,width,height;
   gpu_list_list display_lists;
   unsigned long base_adr;
   struct{
       char swizz;
       char mipmap_level;
       int format;
       float scale_u,scale_v;
       float off_u,off_v;
       float env_color[4];
       int env_mode;
       int wrapS,wrapT;
       int filter_min,filter_mag;
       struct{
           unsigned long adr;
           char mipmap_level;
           int width,height;
           int width_bytes;
           int format;
           unsigned int id;
           char in_memory;
       } textures[8];
   } textures;
   struct{
       int type;
       int kind;
       float pos[4];
       float dir[4];
       float ambient_color[4];
       float specular_color[4];
       float diffuse_color[4];
       float constant,linear,quadratic,exponent,cutoff;
   } lights[4];
   float zFar,zNear;
   unsigned long enable_lights;
   unsigned long z_func,logic_func;
   unsigned long clut_adr;
   unsigned char clut_mask,clut_format,clut_shift,clut_start;
   unsigned long clut_blocks;
   unsigned int clut_id;
   unsigned clut_colors[256];
   float ambient_color[4],diffuse_color[4],specular_color[4];
   unsigned long src_adr,dst_adr;
   int src_width,dst_width,revert;
   int bb_width,bb_height,bb_dst_x,bb_dst_y,bb_src_x,bb_src_y;
   PspGeCallbackData CallbackData;
   LVector<GLuint>collect_textures;
   HACCEL accel;
};
#endif
