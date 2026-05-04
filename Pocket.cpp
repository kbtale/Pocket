// Pocket.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Pocket.h"
#include "PocketUtils.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

HWND hAdapterList, hPacketList, hStartStop, hClear, hAutoScroll, hDetailsPane;
HFONT hFont, hFixedFont;
PocketUtils::CaptureEngine engine;
std::vector<PocketUtils::PacketData> display_packets;
std::mutex display_mutex;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void UpdateLayout(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_POCKET, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!PocketUtils::IsNpcapAvailable()) {
        PocketUtils::TriggerNpcapInstall();
        if (!PocketUtils::IsNpcapAvailable()) {
            return FALSE;
        }
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        hFixedFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

        hAdapterList = CreateWindowW(WC_COMBOBOX, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_ADAPTER_LIST, hInst, nullptr);
        SendMessage(hAdapterList, WM_SETFONT, (WPARAM)hFont, TRUE);

        hStartStop = CreateWindowW(WC_BUTTON, L"Start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_START_STOP, hInst, nullptr);
        SendMessage(hStartStop, WM_SETFONT, (WPARAM)hFont, TRUE);

        hClear = CreateWindowW(WC_BUTTON, L"Clear", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_CLEAR, hInst, nullptr);
        SendMessage(hClear, WM_SETFONT, (WPARAM)hFont, TRUE);

        hAutoScroll = CreateWindowW(WC_BUTTON, L"Auto-scroll", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_AUTOSCROLL, hInst, nullptr);
        SendMessage(hAutoScroll, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hAutoScroll, BM_SETCHECK, BST_CHECKED, 0);

        hPacketList = CreateWindowW(WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA | WS_BORDER | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_PACKET_LIST, hInst, nullptr);
        SendMessage(hPacketList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
        SendMessage(hPacketList, WM_SETFONT, (WPARAM)hFont, TRUE);

        hDetailsPane = CreateWindowW(WC_EDIT, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_BORDER, 0, 0, 0, 0, hWnd, (HMENU)IDC_DETAILS_PANE, hInst, nullptr);
        SendMessage(hDetailsPane, WM_SETFONT, (WPARAM)hFixedFont, TRUE);

        LVCOLUMNW lvc = { 0 };
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        const wchar_t* headers[] = { L"No.", L"Time", L"Source", L"Destination", L"Protocol", L"Length", L"Info" };
        int widths[] = { 50, 150, 140, 140, 80, 70, 300 };

        for (int i = 0; i < 7; i++) {
            lvc.iSubItem = i;
            lvc.pszText = (LPWSTR)headers[i];
            lvc.cx = widths[i];
            SendMessage(hPacketList, LVM_INSERTCOLUMNW, i, (LPARAM)&lvc);
        }

        std::string err;
        auto adapters = PocketUtils::GetAdapters(err);
        for (const auto& adapter : adapters) {
            SendMessageW(hAdapterList, CB_ADDSTRING, 0, (LPARAM)std::wstring(adapter.description.begin(), adapter.description.end()).c_str());
        }
        SendMessage(hAdapterList, CB_SETCURSEL, 0, 0);

        SetTimer(hWnd, IDT_TIMER, 100, nullptr);
        break;
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
    case WM_SIZE:
        UpdateLayout(hWnd);
        break;
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId == IDC_START_STOP) {
            if (!engine.IsRunning()) {
                int sel = (int)SendMessage(hAdapterList, CB_GETCURSEL, 0, 0);
                std::string err;
                auto adapters = PocketUtils::GetAdapters(err);
                if (sel >= 0 && sel < (int)adapters.size()) {
                    if (engine.Start(adapters[sel].name)) {
                        SetWindowTextW(hStartStop, L"Stop");
                    }
                }
            } else {
                engine.Stop();
                SetWindowTextW(hStartStop, L"Start");
            }
        } else if (wmId == IDC_CLEAR) {
            std::lock_guard<std::mutex> lock(display_mutex);
            display_packets.clear();
            SendMessage(hPacketList, LVM_SETITEMCOUNT, 0, 0);
            SetWindowTextW(hDetailsPane, L"");
        } else if (LOWORD(wParam) == IDM_EXIT) {
            DestroyWindow(hWnd);
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        if (nmhdr->idFrom == IDC_PACKET_LIST) {
            if (nmhdr->code == LVN_GETDISPINFO) {
                NMLVDISPINFO* pdi = (NMLVDISPINFO*)lParam;
                if (pdi->item.mask & LVIF_TEXT) {
                    std::lock_guard<std::mutex> lock(display_mutex);
                    if (pdi->item.iItem < (int)display_packets.size()) {
                        const auto& packet = display_packets[pdi->item.iItem];
                        auto parsed = PocketUtils::ProtocolParser::Parse(packet);
                        switch (pdi->item.iSubItem) {
                            case 0: swprintf_s(pdi->item.pszText, pdi->item.cchTextMax, L"%d", pdi->item.iItem + 1); break;
                            case 1: swprintf_s(pdi->item.pszText, pdi->item.cchTextMax, L"%S", parsed.timestamp.c_str()); break;
                            case 2: swprintf_s(pdi->item.pszText, pdi->item.cchTextMax, L"%S", parsed.src_ip.empty() ? parsed.src_mac.c_str() : parsed.src_ip.c_str()); break;
                            case 3: swprintf_s(pdi->item.pszText, pdi->item.cchTextMax, L"%S", parsed.dest_ip.empty() ? parsed.dest_mac.c_str() : parsed.dest_ip.c_str()); break;
                            case 4: swprintf_s(pdi->item.pszText, pdi->item.cchTextMax, L"%S", parsed.protocol.c_str()); break;
                            case 5: swprintf_s(pdi->item.pszText, pdi->item.cchTextMax, L"%d", parsed.length); break;
                            case 6: swprintf_s(pdi->item.pszText, pdi->item.cchTextMax, L"%S", parsed.info.c_str()); break;
                        }
                    }
                }
            } else if (nmhdr->code == LVN_ITEMCHANGED) {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED)) {
                    std::lock_guard<std::mutex> lock(display_mutex);
                    if (pnmv->iItem >= 0 && pnmv->iItem < (int)display_packets.size()) {
                        SetWindowTextW(hDetailsPane, PocketUtils::GetPacketDetails(display_packets[pnmv->iItem]).c_str());
                    }
                }
            }
        }
        break;
    }
    case WM_TIMER: {
        if (wParam == IDT_TIMER) {
            std::vector<PocketUtils::PacketData> new_packets;
            engine.GetPackets(new_packets);
            if (!new_packets.empty()) {
                std::lock_guard<std::mutex> lock(display_mutex);
                display_packets.insert(display_packets.end(), new_packets.begin(), new_packets.end());
                SendMessage(hPacketList, LVM_SETITEMCOUNT, (WPARAM)display_packets.size(), 0);
                if (SendMessage(hAutoScroll, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    SendMessage(hPacketList, LVM_ENSUREVISIBLE, display_packets.size() - 1, FALSE);
                }
            }
        }
        break;
    }
    case WM_DESTROY:
        engine.Stop();
        DeleteObject(hFont);
        DeleteObject(hFixedFont);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void UpdateLayout(HWND hWnd) {
    RECT rect;
    GetClientRect(hWnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    MoveWindow(hAdapterList, 10, 10, w - 320, 30, TRUE);
    MoveWindow(hStartStop, w - 300, 10, 80, 25, TRUE);
    MoveWindow(hClear, w - 210, 10, 80, 25, TRUE);
    MoveWindow(hAutoScroll, w - 120, 10, 110, 25, TRUE);

    int listHeight = (h - 50) * 6 / 10;
    int detailsHeight = (h - 50) - listHeight - 10;

    MoveWindow(hPacketList, 10, 45, w - 20, listHeight, TRUE);
    MoveWindow(hDetailsPane, 10, 45 + listHeight + 5, w - 20, detailsHeight, TRUE);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
