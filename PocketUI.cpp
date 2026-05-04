#include "framework.h"
#include "PocketUI.h"
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

UIManager::UIManager() : 
    m_hWnd(NULL), m_pFactory(NULL), m_pRenderTarget(NULL), m_pDWriteFactory(NULL),
    m_pBackBrush(NULL), m_pSurfaceBrush(NULL), m_pTextBrush(NULL), m_pAccentBrush(NULL), m_pZebraBrush(NULL),
    m_pMainFont(NULL), m_pFixedFont(NULL), m_pBtnFont(NULL),
    m_width(0), m_height(0), m_scrollPos(0), m_detailsScrollPos(0), m_selectedIdx(-1),
    m_startRequested(false), m_clearRequested(false), m_hoverIdx(-1),
    m_showAdapters(false), m_adapterIdx(0), m_autoScroll(true), m_privacyMode(false)
{
    std::string err;
    m_availableAdapters = PocketUtils::GetAdapters(err);
    for (const auto& a : m_availableAdapters)
        m_adapterNameMap[a.name] = a.description;
}

UIManager::~UIManager() {
    Shutdown();
}

std::wstring UIManager::MaskSensitive(const std::string& input, bool isIP) {
    if (!m_privacyMode) return PocketUtils::ConvertToWide(input);
    if (input.empty()) return L"";
    std::wstring result;
    if (isIP) {
        size_t dots = 0;
        for (char c : input) {
            if (c == '.') dots++;
            if (dots >= 2 && c != '.') result += L'*';
            else result += (wchar_t)c;
        }
    } else {
        size_t colons = 0;
        for (char c : input) {
            if (c == ':') colons++;
            if (colons >= 3 && c != ':') result += L'*';
            else result += (wchar_t)c;
        }
    }
    return result;
}

std::wstring UIManager::ResolveAdapterName(const std::string& rawName) {
    auto it = m_adapterNameMap.find(rawName);
    std::wstring desc = (it != m_adapterNameMap.end()) ? PocketUtils::ConvertToWide(it->second) : PocketUtils::ConvertToWide(rawName);
    if (m_privacyMode) {
        size_t hash = std::hash<std::string>{}(rawName);
        return L"Interface " + std::to_wstring(hash % 100);
    }
    return desc;
}

HRESULT UIManager::Initialize(HWND hWnd) {
    m_hWnd = hWnd;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory);
    if (SUCCEEDED(hr))
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
    if (SUCCEEDED(hr)) {
        hr = m_pDWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-us", &m_pMainFont);
        if (SUCCEEDED(hr)) {
            m_pMainFont->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            m_pMainFont->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_pMainFont->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
    }
    if (SUCCEEDED(hr))
        hr = m_pDWriteFactory->CreateTextFormat(L"Consolas", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &m_pFixedFont);
    if (SUCCEEDED(hr)) {
        hr = m_pDWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &m_pBtnFont);
        if (SUCCEEDED(hr)) {
            m_pBtnFont->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pBtnFont->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            m_pBtnFont->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
    }
    return hr;
}

void UIManager::Shutdown() {
    DiscardResources();
    if (m_pMainFont) m_pMainFont->Release();
    if (m_pFixedFont) m_pFixedFont->Release();
    if (m_pBtnFont) m_pBtnFont->Release();
    if (m_pDWriteFactory) m_pDWriteFactory->Release();
    if (m_pFactory) m_pFactory->Release();
}

HRESULT UIManager::CreateResources() {
    if (m_pRenderTarget) return S_OK;
    RECT rc; GetClientRect(m_hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    m_width = size.width; m_height = size.height;
    HRESULT hr = m_pFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hWnd, size, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &m_pRenderTarget);
    if (SUCCEEDED(hr)) hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.07f, 0.07f, 0.08f), &m_pBackBrush);
    if (SUCCEEDED(hr)) hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.12f, 0.12f, 0.14f), &m_pSurfaceBrush);
    if (SUCCEEDED(hr)) hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.88f, 0.88f, 0.90f), &m_pTextBrush);
    if (SUCCEEDED(hr)) hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.40f, 0.80f), &m_pAccentBrush);
    if (SUCCEEDED(hr)) hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.10f, 0.10f, 0.11f), &m_pZebraBrush);
    return hr;
}

void UIManager::DiscardResources() {
    if (m_pBackBrush) m_pBackBrush->Release();
    if (m_pSurfaceBrush) m_pSurfaceBrush->Release();
    if (m_pTextBrush) m_pTextBrush->Release();
    if (m_pAccentBrush) m_pAccentBrush->Release();
    if (m_pZebraBrush) m_pZebraBrush->Release();
    if (m_pRenderTarget) m_pRenderTarget->Release();
    m_pBackBrush = m_pSurfaceBrush = m_pTextBrush = m_pAccentBrush = m_pZebraBrush = NULL;
    m_pRenderTarget = NULL;
}

void UIManager::Resize(UINT width, UINT height) {
    m_width = width; m_height = height;
    if (m_pRenderTarget) m_pRenderTarget->Resize(D2D1::SizeU(width, height));
}

void UIManager::OnNewPackets(int totalCount) {
    if (!m_autoScroll) return;
    float headerH = 45.0f; float footerH = 25.0f;
    float contentH = (float)m_height - headerH - footerH;
    float listH = contentH * 0.6f;
    int visibleRows = (int)((listH - 30) / 20.0f);
    int target = totalCount - visibleRows;
    if (target < 0) target = 0;
    m_scrollPos = target;
}

void UIManager::RenderCharts(D2D1_RECT_F rect, const std::vector<PocketUtils::PacketData>& packets) {
    std::map<std::string, int> protoCount;
    for (const auto& p : packets) {
        auto parsed = PocketUtils::ProtocolParser::Parse(p);
        protoCount[parsed.protocol]++;
    }
    if (packets.empty()) return;
    float centerX = rect.left + (rect.right - rect.left) * 0.5f;
    float centerY = rect.top + (rect.bottom - rect.top) * 0.5f;
    float radius = (std::min)(rect.right - rect.left, rect.bottom - rect.top) * 0.35f;
    float startAngle = 0;
    float colors[][3] = { {0.15f, 0.40f, 0.80f}, {0.80f, 0.20f, 0.20f}, {0.20f, 0.80f, 0.20f}, {0.80f, 0.80f, 0.20f}, {0.20f, 0.80f, 0.80f} };
    int colorIdx = 0;
    for (auto const& [proto, count] : protoCount) {
        float sweep = (float)count / (float)packets.size() * 360.0f;
        ID2D1SolidColorBrush* pBrush;
        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(colors[colorIdx % 5][0], colors[colorIdx % 5][1], colors[colorIdx % 5][2]), &pBrush);
        m_pRenderTarget->DrawTextW(PocketUtils::ConvertToWide(proto).c_str(), (UINT32)proto.length(), m_pMainFont, D2D1::RectF(rect.right - 100, rect.top + colorIdx * 20, rect.right, rect.top + (colorIdx + 1) * 20), pBrush);
        D2D1_POINT_2F p1 = D2D1::Point2F(centerX + radius * cosf(startAngle * 3.14159f / 180.0f), centerY + radius * sinf(startAngle * 3.14159f / 180.0f));
        D2D1_POINT_2F p2 = D2D1::Point2F(centerX + radius * cosf((startAngle + sweep) * 3.14159f / 180.0f), centerY + radius * sinf((startAngle + sweep) * 3.14159f / 180.0f));
        ID2D1PathGeometry* pPath; m_pFactory->CreatePathGeometry(&pPath);
        ID2D1GeometrySink* pSink; pPath->Open(&pSink);
        pSink->BeginFigure(D2D1::Point2F(centerX, centerY), D2D1_FIGURE_BEGIN_FILLED);
        pSink->AddLine(p1);
        pSink->AddArc(D2D1::ArcSegment(p2, D2D1::SizeF(radius, radius), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, sweep > 180 ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
        pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
        pSink->Close(); pSink->Release();
        m_pRenderTarget->FillGeometry(pPath, pBrush);
        m_pRenderTarget->DrawGeometry(pPath, m_pBackBrush, 1.0f);
        pPath->Release(); pBrush->Release();
        startAngle += sweep; colorIdx++;
    }
}

void UIManager::Render(const std::vector<PocketUtils::PacketData>& packets, uint32_t dropped, bool is_running) {
    if (FAILED(CreateResources())) return;
    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->Clear(D2D1::ColorF(0.07f, 0.07f, 0.08f));
    float w = (float)m_width; float h = (float)m_height;
    float headerH = 45.0f; float footerH = 25.0f;
    float contentH = h - headerH - footerH;
    float listH = contentH * 0.6f;
    m_pRenderTarget->FillRectangle(D2D1::RectF(0, 0, w, headerH), m_pSurfaceBrush);
    auto drawBtn = [&](const wchar_t* text, float bx, float bw, bool hover, bool active = false) {
        D2D1_RECT_F r = D2D1::RectF(bx, 8, bx + bw, 37);
        if (active) m_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 4, 4), m_pAccentBrush);
        else m_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 4, 4), hover ? m_pAccentBrush : m_pBackBrush);
        m_pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(r, 4, 4), m_pTextBrush, 0.5f);
        D2D1_RECT_F clip = D2D1::RectF(bx + 4, r.top, bx + bw - 4, r.bottom);
        m_pRenderTarget->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_ALIASED);
        m_pRenderTarget->DrawTextW(text, (UINT32)wcslen(text), m_pBtnFont, r, m_pTextBrush);
        m_pRenderTarget->PopAxisAlignedClip();
    };
    float rightEdge = w - 10;
    drawBtn(is_running ? L"\x25A0 STOP" : L"\x25B6 START", rightEdge - 90, 90, m_hoverIdx == 100);
    drawBtn(L"CLEAR", rightEdge - 165, 70, m_hoverIdx == 101);
    drawBtn(L"\x2193 AUTO", rightEdge - 240, 70, m_hoverIdx == 103, m_autoScroll);
    drawBtn(L"\xD83D\xDD12 PRIVACY", rightEdge - 340, 90, m_hoverIdx == 104, m_privacyMode);

    std::wstring adapterLabel = L"ALL INTERFACES";
    if (m_adapterIdx > 0 && m_adapterIdx <= (int)m_availableAdapters.size())
        adapterLabel = ResolveAdapterName(m_availableAdapters[m_adapterIdx - 1].name);
    float adapterBtnW = (std::min)(w - 370.0f, 300.0f);
    drawBtn(adapterLabel.c_str(), 10, adapterBtnW, m_hoverIdx == 102);

    D2D1_RECT_F listRect = D2D1::RectF(0, headerH, w, headerH + listH);
    m_pRenderTarget->FillRectangle(listRect, m_pBackBrush);
    float colPcts[] = { 0.05f, 0.09f, 0.18f, 0.20f, 0.20f, 0.10f, 0.08f };
    float totalPct = 0; for (int i = 0; i < 7; i++) totalPct += colPcts[i];
    for (int i = 0; i < 7; i++) colPcts[i] /= totalPct;
    const wchar_t* cols[] = { L"No.", L"Time", L"Adapter", L"Source", L"Destination", L"Protocol", L"Len" };
    float hdrY = listRect.top; float hdrH = 22.0f;
    m_pRenderTarget->FillRectangle(D2D1::RectF(0, hdrY, w, hdrY + hdrH), m_pSurfaceBrush);
    float x = 5;
    for (int i = 0; i < 7; i++) {
        float cw = w * colPcts[i];
        D2D1_RECT_F cell = D2D1::RectF(x, hdrY, x + cw - 2, hdrY + hdrH);
        m_pRenderTarget->PushAxisAlignedClip(cell, D2D1_ANTIALIAS_MODE_ALIASED);
        m_pRenderTarget->DrawTextW(cols[i], (UINT32)wcslen(cols[i]), m_pMainFont, cell, m_pTextBrush);
        m_pRenderTarget->PopAxisAlignedClip();
        x += cw;
    }
    float dataTop = hdrY + hdrH; float dataH = listH - hdrH;
    int visibleRows = (int)(dataH / 20.0f);
    int maxScroll = (std::max)(0, (int)packets.size() - visibleRows);
    if (m_scrollPos > maxScroll) m_scrollPos = maxScroll;
    int startIdx = m_scrollPos;
    int endIdx = (std::min)((int)packets.size(), startIdx + visibleRows);
    m_pRenderTarget->PushAxisAlignedClip(D2D1::RectF(0, dataTop, w, dataTop + dataH), D2D1_ANTIALIAS_MODE_ALIASED);
    for (int i = startIdx; i < endIdx; i++) {
        float y = dataTop + (i - startIdx) * 20.0f;
        if (i == m_selectedIdx) m_pRenderTarget->FillRectangle(D2D1::RectF(0, y, w, y + 20.0f), m_pAccentBrush);
        else if (i % 2) m_pRenderTarget->FillRectangle(D2D1::RectF(0, y, w, y + 20.0f), m_pZebraBrush);
        auto p = PocketUtils::ProtocolParser::Parse(packets[i]);
        x = 5;
        auto drawCell = [&](const std::wstring& text, float width) {
            D2D1_RECT_F r = D2D1::RectF(x, y, x + width - 4, y + 20.0f);
            m_pRenderTarget->PushAxisAlignedClip(r, D2D1_ANTIALIAS_MODE_ALIASED);
            m_pRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pMainFont, r, m_pTextBrush);
            m_pRenderTarget->PopAxisAlignedClip();
            x += width;
        };
        drawCell(std::to_wstring(i + 1), w * colPcts[0]);
        drawCell(PocketUtils::ConvertToWide(p.timestamp), w * colPcts[1]);
        drawCell(ResolveAdapterName(p.adapter), w * colPcts[2]);
        drawCell(MaskSensitive(p.src_ip.empty() ? p.src_mac : p.src_ip, !p.src_ip.empty()), w * colPcts[3]);
        drawCell(MaskSensitive(p.dest_ip.empty() ? p.dest_mac : p.dest_ip, !p.dest_ip.empty()), w * colPcts[4]);
        drawCell(PocketUtils::ConvertToWide(p.protocol), w * colPcts[5]);
        drawCell(std::to_wstring(p.length), w * colPcts[6]);
    }
    m_pRenderTarget->PopAxisAlignedClip();

    D2D1_RECT_F detailsRect = D2D1::RectF(0, listRect.bottom + 1, w * 0.65f, h - footerH);
    m_pRenderTarget->FillRectangle(detailsRect, m_pBackBrush);
    m_pRenderTarget->DrawLine(D2D1::Point2F(0, detailsRect.top), D2D1::Point2F(w, detailsRect.top), m_pSurfaceBrush);
    if (m_selectedIdx >= 0 && m_selectedIdx < (int)packets.size()) {
        std::wstring details = PocketUtils::GetPacketDetails(packets[m_selectedIdx]);
        if (m_privacyMode) {
            auto p = PocketUtils::ProtocolParser::Parse(packets[m_selectedIdx]);
            std::wstringstream ss;
            ss << L"--- Frame Details --- (Privacy Mode Active)\n" << L"Arrival Time: " << PocketUtils::ConvertToWide(p.timestamp) << L"\nFrame Length: " << p.length << L" bytes\n\n"
               << L"--- Ethernet II ---\n" << L"Source MAC: " << MaskSensitive(p.src_mac, false) << L"\nDest MAC: " << MaskSensitive(p.dest_mac, false) << L"\n\n"
               << L"--- Internet Protocol ---\n" << L"Source IP: " << MaskSensitive(p.src_ip, true) << L"\nDest IP: " << MaskSensitive(p.dest_ip, true) << L"\n\n"
               << L"--- Raw Data (Hex) --- (Masked)\n";
            details = ss.str();
        }
        D2D1_RECT_F textRect = D2D1::RectF(detailsRect.left + 8, detailsRect.top + 4 - (float)m_detailsScrollPos, detailsRect.right - 8, detailsRect.bottom + 1000);
        m_pRenderTarget->PushAxisAlignedClip(D2D1::RectF(detailsRect.left, detailsRect.top + 1, detailsRect.right, detailsRect.bottom), D2D1_ANTIALIAS_MODE_ALIASED);
        m_pRenderTarget->DrawTextW(details.c_str(), (UINT32)details.length(), m_pFixedFont, textRect, m_pTextBrush);
        m_pRenderTarget->PopAxisAlignedClip();
    }
    D2D1_RECT_F chartsRect = D2D1::RectF(w * 0.65f, listRect.bottom + 1, w, h - footerH);
    m_pRenderTarget->FillRectangle(chartsRect, m_pBackBrush);
    m_pRenderTarget->DrawLine(D2D1::Point2F(chartsRect.left, chartsRect.top), D2D1::Point2F(chartsRect.left, chartsRect.bottom), m_pSurfaceBrush);
    RenderCharts(chartsRect, packets);
    if (m_showAdapters) {
        float overlayW = (std::min)(w * 0.7f, 500.0f); float overlayH = (std::min)(h * 0.7f, 40.0f + 32.0f * ((float)m_availableAdapters.size() + 1));
        float ox = (w - overlayW) * 0.5f; float oy = (h - overlayH) * 0.5f;
        ID2D1SolidColorBrush* pDim; m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0.5f), &pDim);
        m_pRenderTarget->FillRectangle(D2D1::RectF(0, 0, w, h), pDim); pDim->Release();
        D2D1_RECT_F ov = D2D1::RectF(ox, oy, ox + overlayW, oy + overlayH);
        m_pRenderTarget->FillRectangle(ov, m_pSurfaceBrush); m_pRenderTarget->DrawRectangle(ov, m_pAccentBrush, 2.0f);
        float ay = ov.top + 10;
        for (int i = 0; i <= (int)m_availableAdapters.size(); i++) {
            D2D1_RECT_F row = D2D1::RectF(ov.left + 8, ay, ov.right - 8, ay + 28);
            if (i == m_adapterIdx) m_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(row, 3, 3), m_pAccentBrush);
            std::wstring desc = (i == 0) ? L"\x2630 Capture on All Interfaces" : ResolveAdapterName(m_availableAdapters[i-1].name);
            m_pRenderTarget->PushAxisAlignedClip(D2D1::RectF(row.left + 6, ay, row.right - 6, ay + 28), D2D1_ANTIALIAS_MODE_ALIASED);
            m_pRenderTarget->DrawTextW(desc.c_str(), (UINT32)desc.length(), m_pMainFont, D2D1::RectF(row.left + 6, ay, row.right - 6, ay + 28), m_pTextBrush);
            m_pRenderTarget->PopAxisAlignedClip(); ay += 32;
        }
    }
    m_pRenderTarget->FillRectangle(D2D1::RectF(0, h - footerH, w, h), m_pSurfaceBrush);
    std::wstringstream fs; fs << L"Packets: " << packets.size() << L"  |  Dropped: " << dropped << L"  |  " << (is_running ? L"CAPTURING" : L"READY");
    m_pRenderTarget->DrawTextW(fs.str().c_str(), (UINT32)fs.str().length(), m_pMainFont, D2D1::RectF(10, h - footerH, w - 10, h), m_pTextBrush);
    if (m_pRenderTarget->EndDraw() == D2DERR_RECREATE_TARGET) DiscardResources();
}

void UIManager::OnLButtonDown(int x, int y) {
    if (m_showAdapters) {
        float w = (float)m_width; float h = (float)m_height;
        float overlayW = (std::min)(w * 0.7f, 500.0f); float overlayH = (std::min)(h * 0.7f, 40.0f + 32.0f * ((float)m_availableAdapters.size() + 1));
        float ox = (w - overlayW) * 0.5f; float oy = (h - overlayH) * 0.5f;
        D2D1_RECT_F overlay = D2D1::RectF(ox, oy, ox + overlayW, oy + overlayH);
        if ((float)x < overlay.left || (float)x > overlay.right || (float)y < overlay.top || (float)y > overlay.bottom) { m_showAdapters = false; return; }
        float ay = overlay.top + 10;
        for (int i = 0; i <= (int)m_availableAdapters.size(); i++) {
            if ((float)y >= ay && (float)y < ay + 28) { m_adapterIdx = i; m_showAdapters = false; return; }
            ay += 32;
        }
        return;
    }
    float w = (float)m_width; float rightEdge = w - 10;
    if (y >= 8 && y <= 37) {
        if ((float)x >= rightEdge - 90 && (float)x <= rightEdge) m_startRequested = true;
        else if ((float)x >= rightEdge - 165 && (float)x <= rightEdge - 95) m_clearRequested = true;
        else if ((float)x >= rightEdge - 240 && (float)x <= rightEdge - 170) { m_autoScroll = !m_autoScroll; return; }
        else if ((float)x >= rightEdge - 340 && (float)x <= rightEdge - 250) { m_privacyMode = !m_privacyMode; return; }
        else if ((float)x >= 10 && (float)x <= 10 + (std::min)(w - 370.0f, 300.0f)) { m_showAdapters = true; return; }
    }
    float headerH = 45.0f; float listH = ((float)m_height - headerH - 25.0f) * 0.6f;
    float dataTop = headerH + 22.0f; float dataBottom = headerH + listH;
    if ((float)y > dataTop && (float)y < dataBottom) {
        m_selectedIdx = m_scrollPos + (int)(((float)y - dataTop) / 20.0f); m_autoScroll = false; m_detailsScrollPos = 0;
    }
}

void UIManager::OnMouseMove(int x, int y) {
    m_hoverIdx = -1; float w = (float)m_width; float rightEdge = w - 10;
    if (y >= 8 && y <= 37) {
        if ((float)x >= rightEdge - 90 && (float)x <= rightEdge) m_hoverIdx = 100;
        else if ((float)x >= rightEdge - 165 && (float)x <= rightEdge - 95) m_hoverIdx = 101;
        else if ((float)x >= rightEdge - 240 && (float)x <= rightEdge - 170) m_hoverIdx = 103;
        else if ((float)x >= rightEdge - 340 && (float)x <= rightEdge - 250) m_hoverIdx = 104;
        else if ((float)x >= 10 && (float)x <= 10 + (std::min)(w - 370.0f, 300.0f)) m_hoverIdx = 102;
    }
}

void UIManager::OnMouseWheel(int delta) {
    float headerH = 45.0f; float listH = ((float)m_height - headerH - 25.0f) * 0.6f;
    POINT pt; GetCursorPos(&pt); ScreenToClient(m_hWnd, &pt);
    if ((float)pt.y < headerH + listH) {
        m_autoScroll = false; m_scrollPos -= (delta / 120) * 3;
        if (m_scrollPos < 0) m_scrollPos = 0;
    } else if ((float)pt.x < (float)m_width * 0.65f) {
        m_detailsScrollPos -= (delta / 120) * 20;
        if (m_detailsScrollPos < 0) m_detailsScrollPos = 0;
    }
}
