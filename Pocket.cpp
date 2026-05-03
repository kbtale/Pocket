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
std::vector<PocketUtils::ParsedPacket> g_Packets;

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
    static HWND hList;
    switch (message)
    {
    case WM_CREATE:
        {
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            hCombo = CreateWindowW(WC_COMBOBOX, L"", 
                CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
                10, 10, 400, 200, hWnd, (HMENU)IDC_ADAPTER_LIST, hInst, NULL);

            hBtn = CreateWindowW(WC_BUTTON, L"Start Capture",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                420, 10, 120, 25, hWnd, (HMENU)IDC_START_STOP, hInst, NULL);

            CreateWindowW(WC_BUTTON, L"Clear",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                550, 10, 80, 25, hWnd, (HMENU)IDC_CLEAR, hInst, NULL);

            CreateWindowW(WC_BUTTON, L"Auto-scroll",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                640, 10, 100, 25, hWnd, (HMENU)IDC_AUTOSCROLL, hInst, NULL);

            SendMessageW(GetDlgItem(hWnd, IDC_AUTOSCROLL), BM_SETCHECK, BST_CHECKED, 0);

            hList = CreateWindowW(WC_LISTVIEW, L"",
                WS_CHILD | LVS_REPORT | LVS_OWNERDATA | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
                10, 45, 760, 500, hWnd, (HMENU)IDC_PACKET_LIST, hInst, NULL);

            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

            LVCOLUMNW lvc;
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            
            const wchar_t* columns[] = { L"No.", L"Time", L"Source", L"Destination", L"Protocol", L"Length", L"Info" };
            int widths[] = { 50, 100, 130, 130, 80, 60, 200 };

            for (int i = 0; i < 7; i++) {
                lvc.iSubItem = i;
                lvc.pszText = (LPWSTR)columns[i];
                lvc.cx = widths[i];
                lvc.fmt = LVCFMT_LEFT;
                ListView_InsertColumn(hList, i, &lvc);
            }

            std::string err;
            g_Adapters = PocketUtils::GetAdapters(err);
            for (const auto& adapter : g_Adapters) {
                std::wstring desc = PocketUtils::ConvertToWide(adapter.description);
                SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)desc.c_str());
            }
            SendMessageW(hCombo, CB_SETCURSEL, 0, 0);

            SetTimer(hWnd, IDT_TIMER, 100, NULL);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == IDT_TIMER) {
                PocketUtils::PacketData pkt;
                bool changed = false;
                while (g_CaptureManager.GetQueue().Pop(pkt)) {
                    g_Packets.push_back(PocketUtils::ProtocolParser::Parse(pkt));
                    changed = true;
                }
                if (changed) {
                    ListView_SetItemCountEx(hList, g_Packets.size(), LVSICF_NOSCROLL);
                    if (SendMessageW(GetDlgItem(hWnd, IDC_AUTOSCROLL), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        ListView_EnsureVisible(hList, g_Packets.size() - 1, FALSE);
                    }
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR)lParam;
            if (lpnmhdr->code == LVN_GETDISPINFO) {
                NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;
                if (plvdi->item.mask & LVIF_TEXT) {
                    int row = plvdi->item.iItem;
                    int col = plvdi->item.iSubItem;
                    if (row < (int)g_Packets.size()) {
                        const auto& p = g_Packets[row];
                        std::wstring text;
                        switch (col) {
                        case 0: text = std::to_wstring(row + 1); break;
                        case 1: text = PocketUtils::ConvertToWide(p.timestamp); break;
                        case 2: text = PocketUtils::ConvertToWide(p.src_ip.empty() ? p.src_mac : p.src_ip); break;
                        case 3: text = PocketUtils::ConvertToWide(p.dest_ip.empty() ? p.dest_mac : p.dest_ip); break;
                        case 4: text = PocketUtils::ConvertToWide(p.protocol); break;
                        case 5: text = std::to_wstring(p.length); break;
                        case 6: text = PocketUtils::ConvertToWide(p.info); break;
                        }
                        wcsncpy_s(plvdi->item.pszText, plvdi->item.cchTextMax, text.c_str(), _TRUNCATE);
                    }
                }
            }
            else if (lpnmhdr->code == NM_CUSTOMDRAW && lpnmhdr->idFrom == IDC_PACKET_LIST) {
                LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
                switch (lplvcd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT:
                    {
                        int row = (int)lplvcd->nmcd.dwItemSpec;
                        if (row < (int)g_Packets.size()) {
                            const auto& p = g_Packets[row];
                            if (p.info.find("TCP") != std::string::npos || p.info.find("HTTP") != std::string::npos || p.info.find("HTTPS") != std::string::npos) {
                                lplvcd->clrTextBk = RGB(230, 255, 230);
                            }
                            else if (p.info.find("UDP") != std::string::npos || p.info.find("DNS") != std::string::npos) {
                                lplvcd->clrTextBk = RGB(230, 240, 255);
                            }
                            else if (p.info.find("ICMP") != std::string::npos || p.info.find("ARP") != std::string::npos) {
                                lplvcd->clrTextBk = RGB(255, 230, 230);
                            }
                        }
                        return CDRF_DODEFAULT;
                    }
                }
            }
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
            case IDC_CLEAR:
                if (wmEvent == BN_CLICKED) {
                    g_Packets.clear();
                    ListView_SetItemCountEx(hList, 0, LVSICF_NOSCROLL);
                    ListView_Update(hList, -1);
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
