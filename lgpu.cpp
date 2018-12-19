#include "lpsp.h"
#include "resource.h"
#include <stdio.h>

extern HINSTANCE hInstance;
extern BOOL bQuit;
static FILE *fp1 = NULL;
//---------------------------------------------------------------------------
LGPU::LGPU()
{
   m_hWnd = NULL;
   hBitmap = NULL;
   hDC = NULL;
   hRC = NULL;
   accel = NULL;
   out_buffer = NULL;
}
//---------------------------------------------------------------------------
LGPU::~LGPU()
{
    if(hRC != NULL){
        wglMakeCurrent(NULL,NULL);
        wglDeleteContext(hRC);
        hRC = NULL;
    }
    if(hDC != NULL)
        ReleaseDC(m_hWnd,hDC);
    if(hBitmap != NULL)
        DeleteObject(hBitmap);
    if(m_hWnd != NULL)
        ::DestroyWindow(m_hWnd);
    display_lists.Clear();
    if(out_buffer != NULL){
        LocalFree(out_buffer);
        out_buffer = NULL;
    }
}
//---------------------------------------------------------------------------
BOOL LGPU::Init()
{
   HMENU menu;
   RECT rc,rc1;
   PIXELFORMATDESCRIPTOR pfd={0};
   int pixelformat;

   menu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_FILE_MENU));
   m_hWnd = CreateWindow("SAM","Sam",WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,0,0,480,272,
		NULL,menu,hInstance,NULL);
   if(m_hWnd == NULL)
       return FALSE;
   hDC = GetDC(m_hWnd);
   if(hDC == NULL)
   	return FALSE;

    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.dwLayerMask = PFD_MAIN_PLANE;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 16;
    pfd.cStencilBits = 8;

   if((pixelformat = ChoosePixelFormat(hDC,&pfd)) == 0)
       return FALSE;
   if(!::SetPixelFormat(hDC,pixelformat,&pfd))
       return FALSE;
   hRC = wglCreateContext(hDC);
   if(hRC == NULL)
       return FALSE;
   wglMakeCurrent(hDC,hRC);

	ZeroMemory(&bminfo,sizeof(BITMAPINFO));
   bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bminfo.bmiHeader.biWidth = 480;
   bminfo.bmiHeader.biHeight = -272;
   bminfo.bmiHeader.biPlanes = 1;
   bminfo.bmiHeader.biBitCount = 32;
   bminfo.bmiHeader.biCompression = BI_RGB;
   hBitmap = CreateDIBSection(hDC,&bminfo,DIB_RGB_COLORS,(LPVOID *)&screen,NULL,0);
	if(hBitmap == NULL)
   	return FALSE;
   ::SelectObject(hDC,hBitmap);

   GetWindowRect(m_hWnd,&rc);
   GetClientRect(m_hWnd,&rc1);
   rc1.right = ((rc.right - rc.left) - rc1.right) + 480;
   rc1.bottom = ((rc.bottom - rc.top) - rc1.bottom) + 272;

   SetWindowPos(m_hWnd,NULL,0,0,rc1.right,rc1.bottom,SWP_NOMOVE|SWP_NOREPOSITION);
   SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
  	oldWndProc = (WNDPROC)SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG)WindowProc);
	accel = LoadAccelerators(hInstance,MAKEINTRESOURCE(111));

   Reset();
   return TRUE;
}
//---------------------------------------------------------------------------
void LGPU::Reset()
{
   base_adr = 0;
   enable_lights = 0;
   zNear = 0;
   zFar = 1;
   ZeroMemory(&textures,sizeof(textures));
   textures.env_color[0] = textures.env_color[1] = textures.env_color[2] = textures.env_color[3] = 1;
   ZeroMemory(&lights,sizeof(lights));
   ZeroMemory(&CallbackData,sizeof(CallbackData));
   display_lists.Clear();
   if(screen != NULL){
       ZeroMemory(screen,4*480*272);
       BitBlt();
   }
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   return FALSE;
}
//---------------------------------------------------------------------------
LRESULT LGPU::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   switch(uMsg){
       case WM_CLOSE:
           bQuit = TRUE;
           cycles = 99999999;
           lines = 99999999;
       break;
       case WM_COMMAND:
           if(HIWORD(wParam) < 2)
               psp.OnCommand(LOWORD(wParam));
       break;
       case WM_KEYDOWN:
           wParam = wParam;
       break;
       case WM_INITMENUPOPUP:
           psp.OnInitMenuPopup((HMENU)wParam,(UINT)LOWORD(lParam));
       break;
   }
   return ::CallWindowProc((WNDPROC)oldWndProc,m_hWnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT LGPU::WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LGPU *cl;

   cl = (LGPU *)GetWindowLong(hWnd,GWL_USERDATA);
   if(cl != NULL)
       return cl->OnWindowProc(uMsg,wParam,lParam);
  	return ::DefWindowProc(hWnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
int LGPU::add_DisplayList(LPGPU_LIST ls)
{
   if(!display_lists.Add((LPVOID)ls))
       return -1;
   return display_lists.Count();
}
//---------------------------------------------------------------------------
int LGPU::draw_DisplayList(int syncType)
{
   DWORD dwPos,*dst;
   LPGPU_LIST p;
   unsigned long adr,value,vertex_adr,index_adr,adr0,adr1,col;
   BOOL end,swap;
   int clearFlags,vertex_count,i,vertex_size,n,cmd;
   int size_t[]={0,1,2,4};
   float world_mtx[16],view_mtx[16],prj_mtx[16],vtx[6],ts[4],nor[6],color[4],zNear,zFar,*pmtx;
   int tst_t[]={GL_NEVER, GL_ALWAYS, GL_EQUAL, GL_NOTEQUAL, GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL};
   int prim_t[]={GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN};
   int TFUNC_T[]  = {GL_MODULATE, GL_DECAL, GL_BLEND, GL_REPLACE, GL_ADD};
   int ztst_t[]={GL_NEVER, GL_ALWAYS, GL_EQUAL, GL_NOTEQUAL, GL_GREATER, GL_GEQUAL,GL_LESS, GL_LEQUAL};
   int tlft_t[] = {GL_NEAREST, GL_LINEAR, 0, 0, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR};
   int BLENDE_T[]   = {GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX, GL_FUNC_ADD};
   int BLENDF_T_S[] = {GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA};
   int stenop_t[] = {GL_KEEP, GL_ZERO, GL_REPLACE, GL_INVERT, GL_INCR, GL_DECR};
   int logicop_t[]  = {GL_CLEAR, GL_AND, GL_AND_REVERSE, GL_COPY, GL_AND_INVERTED, GL_NOOP, GL_XOR, GL_OR, GL_NOR, GL_EQUIV, GL_INVERT, GL_OR_REVERSE, GL_COPY_INVERTED, GL_OR_INVERTED, GL_NAND, GL_SET};
   
   glViewport(0,0,480,272);
   p = (LPGPU_LIST )display_lists.GetFirstItem(&dwPos);
   while(p != NULL){
       end = FALSE;
       swap = TRUE;
       if(p->end){
           p = (LPGPU_LIST)display_lists.GetNextItem(&dwPos);
           continue;
       }
       adr = p->adr;
       if(adr == p->stall_adr){
           p->stalled = 1;
           return 0;
       }
       clearFlags = 0;
       ZeroMemory(world_mtx,sizeof(world_mtx));
       ZeroMemory(view_mtx,sizeof(view_mtx));
       ZeroMemory(prj_mtx,sizeof(prj_mtx));
       world_mtx[0] = world_mtx[5] = world_mtx[10] = world_mtx[15] = 1;
       view_mtx[0] = view_mtx[5] = view_mtx[10] = view_mtx[15] = 1;
       prj_mtx[0] = prj_mtx[5] = prj_mtx[10] = prj_mtx[15] = 1;
       while(!end && !bQuit){
           value = read_dword(adr);
           adr += 4;
           cmd = (value >> 24);
           switch(cmd){
               case 0x0: //nop
               break;
               case 0x1: //VADDR
                   p->vertex_adr = p->base | (value & 0x00FFFFFF);
               break;
               case 0x2: //IADDR
                   p->index_adr =  p->base | (value & 0x00FFFFFF);
               break;
               case 0x4: //PRIM
                   if(enable_lights & 0x80000000){
                       glEnable(GL_LIGHTING);
                       for(i=0;i<4;i++){
                           if(enable_lights & (1 << i)){
                               glEnable(GL_LIGHT0+i);
                               lights[i].pos[3] = lights[i].dir[3] = 0;
                               glLightfv(GL_LIGHT0+i, GL_POSITION, lights[i].pos);
                               glLightfv(GL_LIGHT0+i, GL_SPOT_DIRECTION, lights[i].dir);
                               glLightfv(GL_LIGHT0+i, GL_SPECULAR, lights[i].specular_color);
                               glLightfv(GL_LIGHT0+i, GL_DIFFUSE, lights[i].diffuse_color);
                               glLightf(GL_LIGHT0+i, GL_CONSTANT_ATTENUATION, lights[i].constant);
                               glLightf(GL_LIGHT0+i, GL_LINEAR_ATTENUATION, lights[i].linear);
                               glLightf(GL_LIGHT0+i, GL_QUADRATIC_ATTENUATION, lights[i].quadratic);
                               glLightf(GL_LIGHT0+i, GL_SPOT_EXPONENT, lights[i].exponent);
                               //				                glLightf(GL_LIGHT0+i, GL_SPOT_CUTOFF, lights[i].cutoff);
                           }
                           else
                               glDisable(GL_LIGHT0+i);
                       }
                   }
                   else{
                       glDisable(GL_LIGHTING);
                       glDisable(GL_LIGHT0);
                       glDisable(GL_LIGHT1);
                       glDisable(GL_LIGHT2);
                       glDisable(GL_LIGHT3);
                   }
                   //                   glDepthRange(0,1);
                   if(p->transform2D){
                       glMatrixMode(GL_PROJECTION);
                       glLoadIdentity();
                       glMatrixMode(GL_MODELVIEW);
                       glLoadIdentity();
                       glOrtho(0.0f, 480.0f, 272.0f, 0.0f, -1.0f, 1.0f);
                   }
                   else{
                       glMatrixMode(GL_PROJECTION);
                       glLoadMatrixf(prj_mtx);
                       glMatrixMode(GL_MODELVIEW);
                       glLoadMatrixf(view_mtx);
                       glMultMatrixf(world_mtx);
                   }
                   glMatrixMode(GL_TEXTURE);
                   glLoadIdentity();
                   for(i=0;i<8;i++){
                       if(textures.textures[i].adr != 0){
                           gen_texture(i);
                       }
                   }
                   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,textures.filter_min);
                   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,textures.filter_mag);
                   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,textures.wrapS);
                   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,textures.wrapT);
                   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, textures.env_mode);
                   glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,textures.env_color);
                   if(p->transform2D && (textures.scale_u == 1 && textures.scale_v == 1) &&
                       (textures.textures[0].width != 0 && textures.textures[0].height != 0))
                       glScalef(1.0f/textures.textures[0].width,1.0f/textures.textures[0].height,1);
                   else
                       glScalef(textures.scale_u,textures.scale_v,1);
                   glTranslatef(textures.off_u,textures.off_v,0);
                   glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,ambient_color);
                   glColor4fv(diffuse_color);
                   vertex_count = (unsigned short)value;
                   p->vertex_size = size_t[p->weight] * p->skinningWeightCount;
                   p->vertex_size += p->color ? ((p->color == 7) ? 4 : 2) : 0;
                   p->vertex_size += size_t[p->texture] * 2;
                   p->vertex_size += size_t[p->position] * 3;
                   p->vertex_size += size_t[p->normal] * 3;
                   p->mem_adr = p->vertex_adr;
                   switch((value >> 16) & 7){
                       case 6:
                           glPushAttrib(GL_CULL_FACE);
                           glDisable(GL_CULL_FACE);
                           glBegin(GL_QUADS);
                           ZeroMemory(vtx,sizeof(vtx));
                           for (;vertex_count;vertex_count-=2) {
                               p->move_index();
                               if(p->weight){
                               }
                               if(p->texture){
                                   p->get_value(p->texture,2,ts);
                               }
                               if(p->color){
                                   p->read_color();
                               }
                               if(p->normal){
                                   p->get_value(p->normal,3,nor);
                               }
                               if(p->position){
                                   p->get_value(p->position,3,vtx);
                               }
                               p->move_index();
                               if(p->weight){
                               }
                               if(p->texture){
                                   p->get_value(p->texture,2,&ts[2]);
                               }
                               if(p->color){
                                   p->read_color();
                                   p->col[3] = 1;
                                   glColor4fv(p->col);
                               }
                               if(p->normal){
                                   p->get_value(p->normal,3,&nor[3]);
                               }
                               if(p->position){
                                   p->get_value(p->position,3,&vtx[3]);
                               }
                               if(p->position){
                                   if(p->texture)
                                       glTexCoord2f(ts[0],ts[1]);
                                   glVertex3fv(vtx);
                                   if(p->texture)
                                       glTexCoord2f(ts[2],ts[1]);
                                   glVertex3f(vtx[3],vtx[1],vtx[2]);
                                   if(p->texture)
                                       glTexCoord2f(ts[2],ts[3]);
                                   glVertex3f(vtx[3],vtx[4],vtx[2]);
                                   if(p->texture)
                                       glTexCoord2f(ts[0],ts[3]);
                                   glVertex3f(vtx[0],vtx[4],vtx[5]);
                               }
                           }
                           glEnd();
                           glPopAttrib();
                       break;
                       default:
                           glBegin(prim_t[(value >> 16) & 7]);
                           for(;vertex_count > 0;vertex_count--){
                               p->move_index();
                               if(p->weight){
                               }
                               if(p->texture){
                                   p->get_value(p->texture,2,ts);
                                   glTexCoord2fv(ts);
                               }
                               if(p->color){
                                   p->read_color();
                                   glColor4fv(p->col);
                               }
                               if(p->normal){
                                   p->get_value(p->normal,3,nor);
                                   glNormal3fv(nor);
                               }
                               if(p->position){
                                   p->get_value(p->position,3,vtx);
                                   //                                   vtx[2] = -vtx[2];
                                   glVertex3fv(vtx);
                               }
                           }
                           glEnd();
                       break;
                   }
               break;
               case 0x7:
               break;
               case 0x8:
                   adr = p->base | (value & 0x00FFFFFF);
               break;
               case 0xA: //CALL
                   p->stack.push(adr);
                   adr = read_dword((p->base | (value & 0x00FFFFFF)));
               break;
               case 0xB: // RET
                   if(p->stack.count())
                       adr = p->stack.pop();
               break;
               case 0xC: //END
                   p->end = 1;
                   end = TRUE;
               break;
               case 0xF://FINISH
               break;
               case 0x10: // BASE
                   p->base = (value & 0x00FFFFFF) << 8;
               break;
               case 0x12: // VTYPE
                   p->texture = (value & 3);
                   p->color = (value >>  2) & 7;
                   p->normal = (value >>  5) & 3;
                   p->position = (value >>  7) & 3;
                   p->weight = (value >>  9) & 3;
                   p->index = (value >> 11) & 3;
                   p->skinningWeightCount = ((value >> 14) & 7) + 1;
                   p->morphingVertexCount = (value >> 18) & 3;
                   p->transform2D         = (value >> 23) & 1;
               break;
               case 0x17:
                   if(value & 0xFFFFFF)
                       enable_lights |= 0x80000000;
                   else
                       enable_lights &= ~0x80000000;
               break;
               case 0x18:
                   if(value & 0xFFFFFF)
                       enable_lights |= 0x1;
                   else
                       enable_lights &= ~0x1;
               break;
               case 0x19:
                   if(value & 0xFFFFFF)
                       enable_lights |= 0x2;
                   else
                       enable_lights &= ~0x2;
               break;
               case 0x1A:
                   if(value & 0xFFFFFF)
                       enable_lights |= 0x4;
                   else
                       enable_lights &= ~0x4;
               break;
               case 0x1B:
                   if(value & 0xFFFFFF)
                       enable_lights |= 0x8;
                   else
                       enable_lights &= ~0x8;
               break;
               case 0x1D: //BCE
                   if(value & 0xFFFFFF)
                       glEnable(GL_CULL_FACE);
                   else
                       glDisable(GL_CULL_FACE);
               break;
               case 0x1E:
                   if((value & 0xFFFFFF))
                       glEnable(GL_TEXTURE_2D);
                   else
                       glDisable(GL_TEXTURE_2D);
               break;
               case 0x20:
                   if((value & 0xFFFFFF))
                       glEnable(GL_DITHER);
                   else
                       glDisable(GL_DITHER);
               break;
               case 0x21:
                   if((value & 0xFFFFFF))
                       glEnable(GL_BLEND);
                   else
                       glDisable(GL_BLEND);
               break;
               case 0x23:
                   if((value & 0xFFFFFF)){
                       glEnable(GL_DEPTH_TEST);
                       glDepthFunc(z_func);
                   }
                   else
                       glDisable(GL_DEPTH_TEST);
               break;
               case 0x24:
                   if((value & 0xFFFFFF))
                       glEnable(GL_STENCIL_TEST);
                   else
                       glDisable(GL_STENCIL_TEST);
               break;
               case 0x31:
                   if((value & 0xFFFFFF))
                       glEnable(GL_BLEND);
                   else
                       glDisable(GL_BLEND);
               break;
               case 0x28:
                   if((value & 0xFFFFFF)){
                       glEnable(GL_COLOR_LOGIC_OP);
                       glLogicOp(logic_func);
                   }
                   else
                       glDisable(GL_COLOR_LOGIC_OP);
               break;
               case 0x3A://World matrix
                   pmtx = world_mtx;
                   for(i=0;i<4;i++){
                       for(n=0;n<3;n++){
                           value = read_dword(adr) << 8;
                           adr += 4;
                           *pmtx++ = (float)*((float *)&value);
                       }
                       *pmtx++ = 0;
                   }
                   world_mtx[15] = 1;
               break;
               case 0x3C://View Matrix
                   pmtx = view_mtx;
                   for(i=0;i<4;i++){
                       for(n=0;n<3;n++){
                           value = read_dword(adr) << 8;
                           adr += 4;
                           *pmtx++ = (float)*((float *)&value);
                       }
                       *pmtx++ = 0;
                   }
                   view_mtx[15] = 1;
               break;
               case 0x3E:
                   pmtx = prj_mtx;
                   for(i=0;i<4;i++){
                       for(n=0;n<4;n++){
                           value = read_dword(adr) << 8;
                           adr += 4;
                           *pmtx++ = (float)*((float *)&value);
                       }
                   }
               break;
               case 0x44:
                   value = value;
               break;
               case 0x47:
                   value = value;
               break;
               case 0x48:
                   value <<= 8;
                   textures.scale_u = *((float *)&value);
               break;
               case 0x49:
                   value <<= 8;
                   textures.scale_v = *((float *)&value);
               break;
               case 0x4A:
                   value <<= 8;
                   textures.off_u = *((float *)&value);
               break;
               case 0x4B:
                   value <<= 8;
                   textures.off_v =  *((float *)&value);
               break;
               case 0x50:
                   glShadeModel((value & 0xFFFFFF) ? GL_SMOOTH : GL_FLAT);
               break;
               case 0x55:
                   ambient_color[0] = (value & 0xFF) / 255.0f;
                   ambient_color[1] = ((value >> 8) & 0xFF) / 255.0f;
                   ambient_color[2] = ((value >> 16) & 0xFF) / 255.0f;
               break;
               case 0x56:
                   diffuse_color[0] = (value & 0xFF) / 255.0f;
                   diffuse_color[1] = ((value >> 8) & 0xFF) / 255.0f;
                   diffuse_color[2] = ((value >> 16) & 0xFF) / 255.0f;
                   diffuse_color[3] = 1;
                   glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse_color);
               break;
               case 0x57:
                   specular_color[0] = (value & 0xFF) / 255.0f;
                   specular_color[1] = ((value >> 8) & 0xFF) / 255.0f;
                   specular_color[2] = ((value >> 16) & 0xFF) / 255.0f;
                   specular_color[3] = 1;
                   //                   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular_color);
               break;
               case 0x58:
                   ambient_color[4] = (value & 0xFF) / 255.0f;
               break;
               case 0x5F:
               case 0x60:
               case 0x61:
               case 0x62:
                   lights[cmd-0x5f].type = (value >> 8) & 3;
                   lights[cmd-0x5f].kind = (value & 3);
               break;
               case 0x63:
               case 0x64:
               case 0x65:
               case 0x66:
               case 0x67:
               case 0x68:
               case 0x69:
               case 0x6A:
               case 0x6B:
               case 0x6C:
               case 0x6D:
               case 0x6E:
                   value <<= 8;
                   lights[(cmd - 0x63) / 3].pos[(cmd - 0x63) % 3] = *((float *)&value);
               break;
               case 0x6F:
               case 0x70:
               case 0x71:
               case 0x72:
               case 0x73:
               case 0x74:
               case 0x75:
               case 0x76:
               case 0x77:
               case 0x78:
               case 0x79:
               case 0x7A:
                   value <<= 8;
                   lights[(cmd - 0x6F) / 3].dir[(cmd - 0x6F) % 3] = *((float *)&value);
               break;
               case 0x7C:
               case 0x7F:
               case 0x82:
               case 0x85:
                   value <<= 8;
                   lights[(cmd - 0x7C) / 3].linear = *((float *)&value);
               break;
               case 0x7D:
               case 0x80:
               case 0x83:
               case 0x86:
                   value <<= 8;
                   lights[(cmd - 0x7C) / 3].quadratic = *((float *)&value);
               break;
               case 0x87:
               case 0x88:
               case 0x89:
               case 0x8A:
                   value <<= 8;
                   lights[cmd - 0x87].exponent = *((float *)&value);
               break;
               case 0x8F:
               case 0x92:
               case 0x95:
               case 0x98:
                   i = (cmd - 0x8F) / 3;
                   lights[i].ambient_color[0] = (value & 0xFF) / 255.0f;
                   lights[i].ambient_color[1] = ((value >> 8) & 0xFF) / 255.0f;
                   lights[i].ambient_color[2] = ((value >> 16) & 0xFF) / 255.0f;
                   lights[i].ambient_color[3] = 1;
               break;
               case 0x90:
               case 0x93:
               case 0x96:
               case 0x99:
                   i = (cmd - 0x90) / 3;
                   lights[i].diffuse_color[0] = (value & 0xFF) / 255.0f;
                   lights[i].diffuse_color[1] = ((value >> 8) & 0xFF) / 255.0f;
                   lights[i].diffuse_color[2] = ((value >> 16) & 0xFF) / 255.0f;
                   lights[i].diffuse_color[3] = 1;
               break;
               case 0x91:
               case 0x94:
               case 0x97:
               case 0x9A:
                   i = (cmd - 0x91) / 3;
                   lights[i].specular_color[0] = (value & 0xFF) / 255.0f;
                   lights[i].specular_color[1] = ((value >> 8) & 0xFF) / 255.0f;
                   lights[i].specular_color[2] = ((value >> 16) & 0xFF) / 255.0f;
                   lights[i].specular_color[3] = 1;
               break;
               case 0x9B:
                   glFrontFace((value & 1) ? GL_CCW : GL_CW);
               break;
               case 0x9D:
                   value = 0;
               break;
               case 0xA0: //TBP
               case 0xA1:
               case 0xA2:
               case 0xA3:
               case 0xA4:
               case 0xA5:
               case 0xA6:
               case 0xA7:
                   textures.textures[cmd - 0xA0].in_memory = 0;
                   textures.textures[cmd - 0xA0].adr &= 0xFF000000;
                   textures.textures[cmd - 0xA0].adr |= (value & 0xFFFFFF);
               break;
               case 0xA8: //TBW
               case 0xA9:
               case 0xAA:
               case 0xAB:
               case 0xAC:
               case 0xAD:
               case 0xAE:
               case 0xAF:
                   textures.textures[cmd - 0xA8].in_memory = 0;
                   textures.textures[cmd - 0xA8].adr &= 0xFFFFFF;
                   textures.textures[cmd - 0xA8].adr |= ((value & 0xF0000) << 8);
                   textures.textures[cmd - 0xA8].width_bytes = (value & 0xFFFF);
               break;
               case 0xB0:
                   clut_adr = (clut_adr & 0xFF00000) | (value & 0xFFFFFF);
               break;
               case 0xB1:
                   clut_adr = (clut_adr & 0xFFFFFF) | ((value & 0xFFFFFF) << 8);
               break;
               case 0xB2:
                   src_adr = (src_adr & 0xFF00000) | (value & 0xFFFFFF);
               break;
               case 0xB3:
                   src_adr = (src_adr & 0xFFFFFF) | ((value & 0xFF0000) << 8);
                   src_width = (value & 0xFFFF);
               break;
               case 0xB4:
                   dst_adr = (dst_adr & 0xFF00000) | (value & 0xFFFFF);
               break;
               case 0xB5:
                   dst_adr = (dst_adr & 0xFFFFFF) | ((value & 0xFF0000) << 8);
                   dst_width = (value & 0xFFFF);
               break;
               case 0xB8: //TSIZE
               case 0xB9:
               case 0xBA:
               case 0xBB:
               case 0xBC:
               case 0xBD:
               case 0xBE:
               case 0xBF:
                   textures.textures[cmd - 0xB8].width = (1 << (value & 0xFF));
                   textures.textures[cmd - 0xB8].height = (1 << ((value & 0xFF00) >> 8));
                   textures.textures[cmd - 0xB8].format = textures.format;
               break;
               case 0xC5:
                   clut_format = (value & 3);
                   clut_mask = (value >> 8) & 0xFF;
                   clut_shift = (value >> 2) & 0x1F;
                   clut_start = ((value >> 16) & 0x1F) << 4;
               break;
               case 0xC4:
                   clut_blocks = (value & 0xFFFFFF);
                   gen_clut();
               break;
               case 0xC6:
                   textures.filter_min = tlft_t[(value &1)];
                   textures.filter_mag = tlft_t[(value >> 8) & 1];
               break;
               case 0xC7:
                   textures.wrapS = (value & 0xFF) ? GL_CLAMP : GL_REPEAT;
                   textures.wrapT = (value & 0xFF00) ? GL_CLAMP : GL_REPEAT;
               break;
               case 0xC2: //TMODE
                   textures.swizz = (char)(value & 1);
                   textures.mipmap_level = (char)((value >> 16) & 4);
               break;
               case 0xC3: //TPSM
                   textures.format = (value & 0xFFFFFF);
               break;
               case 0xC9:
                   textures.env_mode = TFUNC_T[value & 0x07];
               break;
               case 0xCA:
                   textures.env_color[0] = (value & 0xFF) / 255.0f;
                   textures.env_color[1] = ((value >> 8) & 0xFF) / 255.0f;
                   textures.env_color[2] = ((value >> 16) & 0xFF) / 255.0f;
               break;
               case 0xD3:
                   if((value & 1) == 0){
                       glClearColor(p->col[0],p->col[1],p->col[2],p->col[3]);
                       glClear(clearFlags);
                   }
                   else{
                       clearFlags = 0;
                       if (value & 0x100) clearFlags |= GL_COLOR_BUFFER_BIT;
                       if (value & 0x200) clearFlags |= GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
                       if (value & 0x400) clearFlags |= GL_DEPTH_BUFFER_BIT;
                   }
               break;
               case 0xD6:
                   zNear = (unsigned short)value / 65535.0f;
               break;
               case 0xD7:
                   zFar = (unsigned short)value / 65535.0f;
               break;
               case 0xDC: //Stencil Test
                   glStencilFunc(tst_t[value & 0xFF],((value >>  8) & 0xFF),((value >> 16) & 0xFF));
               break;
               case 0xDD:
                   glStencilOp(stenop_t[value & 0xFF],stenop_t[(value >> 8) & 0xFF],stenop_t[(value >> 16) & 0xFF]);
               break;
               case 0xDE:
                   z_func = ztst_t[value & 7];
               break;
               case 0xDF:
                   //                    glBlendEquation(BLENDE_T[(param >> 8) & 0x03]);
                   glBlendFunc(BLENDF_T_S[value & 0xF],BLENDF_T_S[(value >> 4) & 0xF]);
               break;
               case 0xE6:
                   logic_func = logicop_t[value & 0xF];
               break;
               case 0xE7:
                   glDepthMask((value & 0xFFFFFF) ? GL_FALSE : GL_TRUE);
               break;
               case 0xEA:
                   swap = FALSE;
                   switch(value & 1){
                       case 0:
                           for(i=0;i<=bb_height;i++){
                               adr0 = src_adr + ((bb_src_y + i) * src_width * 2) + (bb_src_x * 2);
                               adr1 = dst_adr + ((bb_dst_y + i) * dst_width * 2) + (bb_dst_x * 2);
                               for(n=0;n<=bb_width;n++){
                                   ::write_word(adr1,read_word(adr0));
                                   adr1 += 2;
                                   adr0 += 2;
                               }
                           }
                       break;
                       case 1:
                           for(i=0;i<=bb_height;i++){
                               adr0 = src_adr + ((bb_src_y + i) * src_width * 4) + (bb_src_x * 4);
                               adr1 = dst_adr + ((bb_dst_y + i) * dst_width * 4) + (bb_dst_x * 4);
                               for(n=0;n<=bb_width;n++){
                                   ::write_dword(adr1,read_word(adr0));
                                   adr1 += 4;
                                   adr0 += 4;
                               }
                           }
                       break;
                   }
                   src_adr = dst_adr = 0;
               break;
               case 0xEB:
                   bb_src_x = (value & 0x3FF);
                   bb_src_y = (value >> 10) & 0x1FF;
               break;
               case 0xEC:
                   bb_dst_x = (value & 0x3FF);
                   bb_dst_y = (value >> 10) & 0x1FF;
               break;
               case 0xEE:
                   bb_width = (value & 0x3FF);
                   bb_height = (value >> 10) & 0x1FF;
               break;
               default:
                   end = false;
               break;
           }
           if(p->adr == p->stall_adr){
               p->adr = adr;
               p->stalled = 1;
               return 0;
           }
       }
       p = (LPGPU_LIST)display_lists.GetNextItem(&dwPos);
   }
   display_lists.Clear();
   if(swap){
       if(out_buffer == NULL)
           out_buffer = (LPBYTE)LocalAlloc(LMEM_FIXED,512*272*4);
       dst = (LPDWORD)&video_mem[base_adr & 0x1FFFFF];
       glReadPixels(0,0,512,272,GL_BGRA,GL_UNSIGNED_BYTE,out_buffer);
       for(i=0x87800;i>=0;i-= 2048){
           for(n=0;n<2048;n+=4)
               *dst++ = *((LPDWORD)&out_buffer[i+n]);
       }
   }
   for(i=0;i<collect_textures.count();i++){
       n = collect_textures[i+1];
       glDeleteTextures(1,(GLuint *)&n);
       if(glGetError() != GL_NO_ERROR)
           break;
       collect_textures.pop();
   }
   return 0;
}
//---------------------------------------------------------------------------
void LGPU::gen_texture(int index)
{
   unsigned char *p;
   GLenum err;
   long *data,color,*p1;
   int i,i1,x,y;

   if(textures.textures[index].in_memory && textures.textures[index].id != 0){
       glBindTexture(GL_TEXTURE_2D,textures.textures[index].id);
       return;
   }
   if(textures.textures[index].width < 3 || textures.textures[index].height < 3)
       return;
   if(textures.textures[index].id != 0)
       collect_textures.push(textures.textures[index].id);
    glGenTextures(1,&textures.textures[index].id);
    glBindTexture(GL_TEXTURE_2D,textures.textures[index].id);
    p = &mem[(textures.textures[index].adr & 0x0FFFFFFF) - 0x08000000];
    switch(textures.textures[index].format){
       case 0:
           data = (long *)LocalAlloc(LPTR,textures.textures[index].height*textures.textures[index].width*4);
           p1 = data;
            for(y=0;y<textures.textures[index].height;y++){
               for(x=0;x<textures.textures[index].width;x++){
                   i = *((LPWORD)p);
                   p += 2;
                   color = (i & 31) << 3;
                   color |= ((i & 0x7E0) << 5);
                   color |= ((i & 0xF800) << 8);
                   *p1++ = 0xFF000000|color;
               }
               p += (textures.textures[index].width_bytes - textures.textures[index].width) * 2;
            }
            glTexImage2D(GL_TEXTURE_2D,0,4,textures.textures[index].width,
            textures.textures[index].height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
            textures.textures[index].in_memory = 1;
            LocalFree(data);
       break;
       case 1:
            glTexImage2D(GL_TEXTURE_2D,0,4,textures.textures[index].width,
            textures.textures[index].height,0,GL_RGBA,GL_UNSIGNED_SHORT_1_5_5_5_REV,p);
            textures.textures[index].in_memory = 1;
       break;
        case 2:
            glTexImage2D(GL_TEXTURE_2D,0,4,textures.textures[index].width,
            textures.textures[index].height,0,GL_RGBA,GL_UNSIGNED_SHORT_4_4_4_4_REV,p);
            textures.textures[index].in_memory = 1;
        break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D,0,4,textures.textures[index].width,
            textures.textures[index].height,0,GL_RGBA,GL_UNSIGNED_INT_8_8_8_8_REV,p);
            textures.textures[index].in_memory = 1;
        break;
        case 5:
            i1 = textures.textures[index].width*textures.textures[index].height;
            data = (long *)LocalAlloc(LPTR,i1 * 4);
            for(i=0;i<i1;i++){
                data[i] = clut_colors[*p++];
            }
            glTexImage2D(GL_TEXTURE_2D,0,4,textures.textures[index].width,
            textures.textures[index].height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
            LocalFree(data);
            textures.textures[index].in_memory = 1;            
        break;
        default:
           data = NULL;
        break;
    }
}
//---------------------------------------------------------------------------
void LGPU::gen_clut()
{
    unsigned long mem,value;
    int blockSize,count,i;

    mem = clut_adr;
    if(mem == 0)
        return;
    blockSize = ((clut_format == 3) ? 8 : 16);
    count = clut_blocks * blockSize;
    for(i=0;i<count;i++){
        switch(clut_format){
            case 3:
                value = (int)(signed int)read_dword(mem);
                mem += 4;
            break;
            default:
               value = 0;
           break;
        }
        switch(clut_format){
            case 3:
                clut_colors[i] = value;
            break;
        }
    }
}
//---------------------------------------------------------------------------
int LGPU::ge_SetMode(int m,int w,int h)
{
   width = w;
}
//---------------------------------------------------------------------------
int LGPU::ge_SwapBuffer(unsigned long adr,int width,int _mode,int ukn)
{
   mode = _mode;
   base_adr = adr;
   return 1;
}
//---------------------------------------------------------------------------
void LGPU::BitBlt()
{
   int x,y,r,g,b;
   unsigned char *src,*p;
   unsigned long col;

   if(video_mem != NULL){
       src = &video_mem[base_adr & 0x1FFFFF];
       switch(mode){
           case 1:
               for(y=0;y<272;y++){
                   p = src + (y*512*2);
                   for(x=0;x<480;x++){
                       col = *((unsigned short *)p);
                       p+= 2;
                       r = (col >> 10) & 31;
                       g = (col >> 5) & 31;
                       b = col & 31;
                       *((int *)&screen[y*1920+x*4]) = RGB(r<<3,g<<3,b<<3);
                   }
               }
           break;
           default:                                  
               for(y=0;y<272;y++){
                   p = src + (y*512*4);
                   for(x=0;x<1920;x+=4){
                       *((int *)&screen[y*1920+x]) = *((unsigned long *)p);
                       p += 4;
                   }
               }
           break;
       }
   }
   revert = 0;
   StretchDIBits(hDC,0,0,480,272,0,0,480,272,screen,&bminfo,DIB_RGB_COLORS,SRCCOPY);
}
//---------------------------------------------------------------------------
void LGPU::set_CallbackData(unsigned long adr)
{
   CallbackData.signal_func = read_dword(adr);
   CallbackData.signal_arg = read_dword(adr+4);
   CallbackData.finish_func = read_dword(adr+8);
   CallbackData.finish_arg = read_dword(adr+12);
}
