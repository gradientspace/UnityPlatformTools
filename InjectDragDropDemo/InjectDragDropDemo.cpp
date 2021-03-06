// InjectDragDropDemo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "InjectDragDropDemo.h"

// RMS
#include "Shellapi.h"
#include <string>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_INJECTDRAGDROPDEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_INJECTDRAGDROPDEMO));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INJECTDRAGDROPDEMO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_INJECTDRAGDROPDEMO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   //HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 600, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


static bool INJECTED = false;

typedef int(*get_filename_length_func)(int);
get_filename_length_func GetDroppedFilenameLength;

typedef int(*get_filename)(int, void *, int);
get_filename GetDroppedFilename;



void __stdcall DragDropCallback(int nFiles)
{
	wchar_t buffer[1024];
	swprintf(buffer, 1024, L"DragDropCallback: %d files\n", nFiles);
	OutputDebugString(buffer);

	for (int i = 0; i < nFiles; ++i) {
		int length = GetDroppedFilenameLength(i);
		swprintf(buffer, 1024, L"File %d: %d characters\n", i, length);
		OutputDebugString(buffer);

		std::wstring filename; filename.resize(length+1);
		int bufsize = (int)filename.size() * sizeof(wchar_t);
		unsigned char * pBytes = (unsigned char *)filename.c_str();
		memset(pBytes, 0xFF, bufsize);
		int retval = GetDroppedFilename(i, pBytes, bufsize);
		swprintf(buffer, 1024, L"Filename: %s\n", filename.c_str());
		OutputDebugString(buffer);

	}

}


void InjectDragDropR2()
{
	if (INJECTED)
		return;

	// find the dll
	HWND hWnd = GetForegroundWindow();
	HMODULE hLibModule = LoadLibrary(TEXT("Win32DragDropHook.dll"));
	if (hLibModule == 0) {
		OutputDebugString(L"Could not find DLL!");
		int last_error = GetLastError();
		return;
	}

	// call function that sets up dragdrop
	typedef int(*install_hook_func)(HWND hWnd);
	install_hook_func install_hook = (install_hook_func)GetProcAddress(hLibModule, "InstallDragDropHook");
	if (install_hook == 0) {
		OutputDebugString(L"Could not find InstallDragDropHook");
		int last_error = GetLastError();
		return;
	}
	int installed = install_hook(hWnd);

	// call function that sets up callback
	typedef void(*install_callback_func)(void *);
	install_callback_func install_callback = (install_callback_func)GetProcAddress(hLibModule, "SetActiveCallback");
	if (install_callback != 0)
		install_callback(DragDropCallback);

	GetDroppedFilenameLength = (get_filename_length_func)GetProcAddress(hLibModule, "GetLastDropFilenameLength");
	GetDroppedFilename = (get_filename)GetProcAddress(hLibModule, "GetLastDropFilename"); 

	INJECTED = true;
}




//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;


	case WM_KEYDOWN:
		//DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		//InjectDragDrop();
		InjectDragDropR2();
		break;


		// [RMS]
	// accept drag-drop
	//case WM_DROPFILES:
	//	DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
	//	break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}




