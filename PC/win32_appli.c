#include <windows.h>                    /* must include this before GL/gl.h */
#include <GL/gl.h>                      /* OpenGL header file */
#include <GL/glu.h>                     /* OpenGL utilities header file */
#include <stdio.h>
#include <math.h>

#include "model.h"
#include "appli.h"

/* inevitables variables globales */

extern flat_tex_quad flat;

#include <stdarg.h>

/* gasp pour windows */

void gasp( char *fmt, ... )  /* fatal error handling */
{
  char lbuf[1024];
  va_list  argptr;
  va_start( argptr, fmt );
  vsprintf( lbuf, fmt, argptr );
  va_end( argptr );
  MessageBox(NULL, lbuf, "HORREUR", MB_OK);
  exit(1);
}


/** ============================ call backs ======================= */

/* la fonction qui interprete les messages. 
   elle traite ceux qui sont independants de l'application et appelle
   aDisplay, aKey(), aButtons() et aPosition() pour les autres
 */
LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
short int mx, my;	// on les declare short pour enclencher l'extension de signe
static int curbou = 0;

    switch(uMsg) {
    case WM_PAINT:
	aDisplay();
	swap_win32_buffers();
        return 0;

    case WM_SIZE:
	config_viewport( &flat, LOWORD(lParam), HIWORD(lParam) );
        PostMessage(hWnd, WM_PAINT, 0, 0);
        return 0;

    case WM_CHAR:
        switch (wParam) {
        case 27 : PostQuitMessage(0); break;
        default : aKey( (short int) wParam );
                  PostMessage(hWnd, WM_PAINT, 0, 0);
        }
        return 0;

    case WM_LBUTTONDOWN:
        // we set the capture to get messages when mouse moves outside the window.
        SetCapture(hWnd);
	mx = (short int) LOWORD(lParam);
	my = (short int) HIWORD(lParam);
	curbou = 1;
        aMouse( (int)mx, (int)my, curbou );
        // if ( wParam & MK_SHIFT  )   /* avec shift */
        return 0;
        
    case WM_RBUTTONDOWN:
        // we set the capture to get messages when mouse moves outside the window.
        SetCapture(hWnd);
	mx = (short int) LOWORD(lParam);
	my = (short int) HIWORD(lParam);
	curbou = 3;
        aMouse( (int)mx, (int)my, curbou );
        return 0;
        
    case WM_LBUTTONUP:
        ReleaseCapture();
	mx = (short int) LOWORD(lParam);
	my = (short int) HIWORD(lParam);
	curbou = 0;
        aMouse( (int)mx, (int)my, curbou );
        PostMessage(hWnd, WM_PAINT, 0, 0);
        return 0;

    case WM_RBUTTONUP:
        ReleaseCapture();
	mx = (short int) LOWORD(lParam);
	my = (short int) HIWORD(lParam);
	curbou = 0;
        aMouse( (int)mx, (int)my, curbou );
        PostMessage(hWnd, WM_PAINT, 0, 0);
	return 0;

    case WM_MOUSEMOVE:
        if	( curbou )
		{
		mx = (short int) LOWORD(lParam);
		my = (short int) HIWORD(lParam);
		aMouse( (int)mx, (int)my, curbou );
		PostMessage(hWnd, WM_PAINT, 0, 0);
		}
        return 0;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }
return DefWindowProc(hWnd, uMsg, wParam, lParam); 
} 

/** ============================ main ======================= */

/* creation fenetre Win32 tout a fait ordinaire */
HWND CreateMyWindow(char* title, int x, int y, int width, int height )
{
HWND  hWnd;
    WNDCLASS    wc;
    static HINSTANCE hInstance = 0;

    /* only register the window class once - use hInstance as a flag. */
    if (!hInstance) {
        hInstance = GetModuleHandle(NULL);
        wc.style         = CS_OWNDC;
        wc.lpfnWndProc   = (WNDPROC)WindowProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = "myClass";

        if (!RegisterClass(&wc)) {
            MessageBox(NULL, "RegisterClass() failed: Cannot register window class.", "Error", MB_OK);
            return NULL;
        }
    }
    hWnd = CreateWindow("myClass", title, WS_OVERLAPPEDWINDOW |
                        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                        x, y, width, height, NULL, NULL, hInstance, NULL);
    if (hWnd == NULL) {
        MessageBox(NULL, "CreateWindow() failed",
                   "Error", MB_OK);
        return NULL;
    }
return hWnd;
}    

// la fonction main a la Win32
int APIENTRY WinMain(HINSTANCE hCurrentInst, HINSTANCE hPreviousInst,
                      LPSTR lpszCmdLine, int nCmdShow)
{
HWND  hWnd;                         /* window */
MSG   msg;                          /* message */
const char * bmpname;

if	( lpszCmdLine[0] )
	bmpname = lpszCmdLine;
else	bmpname = "bak.bmp";

read_bmp_rgba( &flat, bmpname );

// ajuster la window aux dimensions de la bmp avec de la marge ;-)
// c'est pas oblige, en fait, puisqu'on peut la resizer a tout moment
hWnd = CreateMyWindow("jlnij - Double Buffer - True Color", 0, 0, flat.tW + 20, flat.tH + 40);
if	( hWnd == NULL )
	gasp("CreateWindow failed");

// allumer openGL
prepare_win32_pixel( hWnd );
    
// preparer application
aInit();

ShowWindow(hWnd, nCmdShow);
    
/* boucle principale de Windows */

    while(GetMessage(&msg, hWnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    largue_win32_contexts();
    DestroyWindow(hWnd);
    return msg.wParam;
}
