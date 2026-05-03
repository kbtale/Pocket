// Pocket.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Pocket.h"
#include "PocketUtils.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

PocketUtils::CaptureManager g_CaptureManager;
std::vector<PocketUtils::AdapterInfo> g_Adapters;

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

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_POCKET, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!PocketUtils::IsNpcapAvailable()) {
        MessageBoxW(NULL, PocketUtils::GetNpcapErrorMessage().c_str(), L"Pocket - Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_POCKET));

    MSG msg;

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



ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_POCKET));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_POCKET);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hCombo;
    static HWND hBtn;
    switch (message)
    {
    case WM_CREATE:
        {
            hCombo = CreateWindowW(WC_COMBOBOX, L"", 
                CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
                10, 10, 400, 200, hWnd, (HMENU)IDC_ADAPTER_LIST, hInst, NULL);

            hBtn = CreateWindowW(WC_BUTTON, L"Start Capture",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                420, 10, 120, 25, hWnd, (HMENU)IDC_START_STOP, hInst, NULL);

            std::string err;
            g_Adapters = PocketUtils::GetAdapters(err);
            for (const auto& adapter : g_Adapters) {
                std::wstring desc = PocketUtils::ConvertToWide(adapter.description);
                SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)desc.c_str());
            }
            SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            switch (wmId)
            {
            case IDC_START_STOP:
                if (wmEvent == BN_CLICKED) {
                    if (g_CaptureManager.IsRunning()) {
                        g_CaptureManager.Stop();
                        SetWindowTextW(hBtn, L"Start Capture");
                        EnableWindow(hCombo, TRUE);
                    }
                    else {
                        int index = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                        if (index != CB_ERR && index < (int)g_Adapters.size()) {
                            if (g_CaptureManager.Start(g_Adapters[index].name)) {
                                SetWindowTextW(hBtn, L"Stop Capture");
                                EnableWindow(hCombo, FALSE);
                            }
                        }
                    }
                }
                break;
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
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

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
