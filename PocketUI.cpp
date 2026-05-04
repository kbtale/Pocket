#include "framework.h"
#include "PocketUI.h"
#include <algorithm>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

UIManager::UIManager() : 
    m_hWnd(NULL), m_pFactory(NULL), m_pRenderTarget(NULL), m_pDWriteFactory(NULL),
    m_pBackBrush(NULL), m_pSurfaceBrush(NULL), m_pTextBrush(NULL), m_pAccentBrush(NULL), m_pZebraBrush(NULL),
    m_pMainFont(NULL), m_pFixedFont(NULL), m_pBtnFont(NULL),
    m_width(0), m_height(0), m_scrollPos(0), m_selectedIdx(-1),
    m_startRequested(false), m_clearRequested(false), m_hoverIdx(-1),
    m_showAdapters(false), m_adapterIdx(0), m_autoScroll(true)
{
    std::string err;
    m_availableAdapters = PocketUtils::GetAdapters(err);
    for (const auto& a : m_availableAdapters)
        m_adapterNameMap[a.name] = a.description;
}

UIManager::~UIManager() {
    Shutdown();
}

std::wstring UIManager::ResolveAdapterName(const std::string& rawName) {
    auto it = m_adapterNameMap.find(rawName);
    if (it != m_adapterNameMap.end())
        return PocketUtils::ConvertToWide(it->second);
    return PocketUtils::ConvertToWide(rawName);
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

void UIManager::Render(const std::vector<PocketUtils::PacketData>& packets, uint32_t dropped, bool is_running) {
    if (FAILED(CreateResources())) return;
    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->Clear(D2D1::ColorF(0.07f, 0.07f, 0.08f));

    float w = (float)m_width;
    float h = (float)m_height;
    float headerH = 45.0f;
    float footerH = 25.0f;
    float contentH = h - headerH - footerH;
    float listH = contentH * 0.6f;
    float rowH = 20.0f;

    m_pRenderTarget->FillRectangle(D2D1::RectF(0, 0, w, headerH), m_pSurfaceBrush);

    auto drawBtn = [&](const wchar_t* text, float bx, float bw, bool hover, bool active = false) {
        D2D1_RECT_F r = D2D1::RectF(bx, 8, bx + bw, 37);
        if (active)
            m_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 4, 4), m_pAccentBrush);
        else
            m_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 4, 4), hover ? m_pAccentBrush : m_pBackBrush);
        m_pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(r, 4, 4), m_pTextBrush, 0.5f);
        D2D1_RECT_F clip = D2D1::RectF(bx + 4, r.top, bx + bw - 4, r.bottom);
        m_pRenderTarget->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_ALIASED);
        m_pRenderTarget->DrawTextW(text, (UINT32)wcslen(text), m_pBtnFont, r, m_pTextBrush);
        m_pRenderTarget->PopAxisAlignedClip();
    };

    float rightEdge = w - 10;
    drawBtn(is_running ? L"\x25A0 STOP" : L"\x25B6 START", rightEdge - 90, 90, m_hoverIdx == 100);
    drawBtn(L"CLEAR", rightEdge - 165, 70, m_hoverIdx == 101);
    drawBtn(m_autoScroll ? L"\x2193 AUTO" : L"\x2193 AUTO", rightEdge - 240, 70, m_hoverIdx == 103, m_autoScroll);

    std::wstring adapterLabel = L"ALL INTERFACES";
    if (m_adapterIdx > 0 && m_adapterIdx <= (int)m_availableAdapters.size())
        adapterLabel = PocketUtils::ConvertToWide(m_availableAdapters[m_adapterIdx - 1].description);
    float adapterBtnW = (std::min)(w - 270.0f, 350.0f);
    drawBtn(adapterLabel.c_str(), 10, adapterBtnW, m_hoverIdx == 102);

    D2D1_RECT_F listRect = D2D1::RectF(0, headerH, w, headerH + listH);
    m_pRenderTarget->FillRectangle(listRect, m_pBackBrush);

    float colPcts[] = { 0.05f, 0.09f, 0.18f, 0.20f, 0.20f, 0.10f, 0.08f };
    float totalPct = 0; for (int i = 0; i < 7; i++) totalPct += colPcts[i];
    for (int i = 0; i < 7; i++) colPcts[i] /= totalPct;
    float availW = w;

    const wchar_t* cols[] = { L"No.", L"Time", L"Adapter", L"Source", L"Destination", L"Protocol", L"Len" };
    float hdrY = listRect.top;
    float hdrH = 22.0f;
    m_pRenderTarget->FillRectangle(D2D1::RectF(0, hdrY, w, hdrY + hdrH), m_pSurfaceBrush);
    float x = 5;
    for (int i = 0; i < 7; i++) {
        float cw = availW * colPcts[i];
        D2D1_RECT_F cell = D2D1::RectF(x, hdrY, x + cw - 2, hdrY + hdrH);
        m_pRenderTarget->PushAxisAlignedClip(cell, D2D1_ANTIALIAS_MODE_ALIASED);
        m_pRenderTarget->DrawTextW(cols[i], (UINT32)wcslen(cols[i]), m_pMainFont, cell, m_pTextBrush);
        m_pRenderTarget->PopAxisAlignedClip();
        x += cw;
    }

    float dataTop = hdrY + hdrH;
    float dataH = listH - hdrH;
    int visibleRows = (int)(dataH / rowH);
    int maxScroll = (std::max)(0, (int)packets.size() - visibleRows);
    if (m_scrollPos > maxScroll) m_scrollPos = maxScroll;
    int startIdx = m_scrollPos;
    int endIdx = (std::min)((int)packets.size(), startIdx + visibleRows);

    D2D1_RECT_F dataClip = D2D1::RectF(0, dataTop, w, dataTop + dataH);
    m_pRenderTarget->PushAxisAlignedClip(dataClip, D2D1_ANTIALIAS_MODE_ALIASED);

    for (int i = startIdx; i < endIdx; i++) {
        float y = dataTop + (i - startIdx) * rowH;
        D2D1_RECT_F rowRect = D2D1::RectF(0, y, w, y + rowH);
        if (i == m_selectedIdx)
            m_pRenderTarget->FillRectangle(rowRect, m_pAccentBrush);
        else if (i % 2)
            m_pRenderTarget->FillRectangle(rowRect, m_pZebraBrush);

        auto parsed = PocketUtils::ProtocolParser::Parse(packets[i]);
        x = 5;
        auto drawCell = [&](const std::wstring& text, float width) {
            D2D1_RECT_F r = D2D1::RectF(x, y, x + width - 4, y + rowH);
            m_pRenderTarget->PushAxisAlignedClip(r, D2D1_ANTIALIAS_MODE_ALIASED);
            m_pRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pMainFont, r, m_pTextBrush);
            m_pRenderTarget->PopAxisAlignedClip();
            x += width;
        };
        drawCell(std::to_wstring(i + 1), availW * colPcts[0]);
        drawCell(PocketUtils::ConvertToWide(parsed.timestamp), availW * colPcts[1]);
        drawCell(ResolveAdapterName(parsed.adapter), availW * colPcts[2]);
        drawCell(PocketUtils::ConvertToWide(parsed.src_ip.empty() ? parsed.src_mac : parsed.src_ip), availW * colPcts[3]);
        drawCell(PocketUtils::ConvertToWide(parsed.dest_ip.empty() ? parsed.dest_mac : parsed.dest_ip), availW * colPcts[4]);
        drawCell(PocketUtils::ConvertToWide(parsed.protocol), availW * colPcts[5]);
        drawCell(std::to_wstring(parsed.length), availW * colPcts[6]);
    }
    m_pRenderTarget->PopAxisAlignedClip();

    if ((int)packets.size() > visibleRows) {
        float scrollBarW = 8;
        float scrollAreaH = dataH;
        float thumbH = (std::max)(20.0f, scrollAreaH * (float)visibleRows / (float)packets.size());
        float thumbY = dataTop + (scrollAreaH - thumbH) * ((float)m_scrollPos / (float)maxScroll);
        m_pRenderTarget->FillRectangle(D2D1::RectF(w - scrollBarW, thumbY, w, thumbY + thumbH), m_pSurfaceBrush);
    }

    D2D1_RECT_F detailsRect = D2D1::RectF(0, listRect.bottom + 1, w, h - footerH);
    m_pRenderTarget->FillRectangle(detailsRect, m_pBackBrush);
    m_pRenderTarget->DrawLine(D2D1::Point2F(0, detailsRect.top), D2D1::Point2F(w, detailsRect.top), m_pSurfaceBrush);
    if (m_selectedIdx >= 0 && m_selectedIdx < (int)packets.size()) {
        std::wstring details = PocketUtils::GetPacketDetails(packets[m_selectedIdx]);
        D2D1_RECT_F textRect = D2D1::RectF(detailsRect.left + 8, detailsRect.top + 4, detailsRect.right - 8, detailsRect.bottom - 4);
        m_pRenderTarget->PushAxisAlignedClip(textRect, D2D1_ANTIALIAS_MODE_ALIASED);
        m_pRenderTarget->DrawTextW(details.c_str(), (UINT32)details.length(), m_pFixedFont, textRect, m_pTextBrush);
        m_pRenderTarget->PopAxisAlignedClip();
    }

    if (m_showAdapters) {
        float overlayW = (std::min)(w * 0.7f, 500.0f);
        float overlayH = (std::min)(h * 0.7f, 40.0f + 32.0f * ((float)m_availableAdapters.size() + 1));
        float ox = (w - overlayW) * 0.5f;
        float oy = (h - overlayH) * 0.5f;
        D2D1_RECT_F overlay = D2D1::RectF(ox, oy, ox + overlayW, oy + overlayH);
        m_pRenderTarget->FillRectangle(D2D1::RectF(0, 0, w, h), m_pBackBrush);
        m_pRenderTarget->FillRectangle(overlay, m_pSurfaceBrush);
        m_pRenderTarget->DrawRectangle(overlay, m_pAccentBrush, 2.0f);
        
        float ay = overlay.top + 10;
        for (int i = 0; i <= (int)m_availableAdapters.size(); i++) {
            D2D1_RECT_F row = D2D1::RectF(overlay.left + 8, ay, overlay.right - 8, ay + 28);
            if (i == m_adapterIdx)
                m_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(row, 3, 3), m_pAccentBrush);
            std::wstring desc = (i == 0) ? L"\x2630 Capture on All Interfaces" : PocketUtils::ConvertToWide(m_availableAdapters[i-1].description);
            D2D1_RECT_F textRow = D2D1::RectF(row.left + 6, ay, row.right - 6, ay + 28);
            m_pRenderTarget->PushAxisAlignedClip(textRow, D2D1_ANTIALIAS_MODE_ALIASED);
            m_pRenderTarget->DrawTextW(desc.c_str(), (UINT32)desc.length(), m_pMainFont, textRow, m_pTextBrush);
            m_pRenderTarget->PopAxisAlignedClip();
            ay += 32;
        }
    }

    float footerY = h - footerH;
    m_pRenderTarget->FillRectangle(D2D1::RectF(0, footerY, w, h), m_pSurfaceBrush);
    std::wstringstream ss;
    ss << L"Packets: " << packets.size() << L"  |  Dropped: " << dropped << L"  |  " << (is_running ? L"CAPTURING" : L"READY");
    m_pRenderTarget->DrawTextW(ss.str().c_str(), (UINT32)ss.str().length(), m_pMainFont, D2D1::RectF(10, footerY, w - 10, h), m_pTextBrush);

    HRESULT hr = m_pRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) DiscardResources();
}

void UIManager::OnLButtonDown(int x, int y) {
    if (m_showAdapters) {
        float w = (float)m_width; float h = (float)m_height;
        float overlayW = (std::min)(w * 0.7f, 500.0f);
        float overlayH = (std::min)(h * 0.7f, 40.0f + 32.0f * ((float)m_availableAdapters.size() + 1));
        float ox = (w - overlayW) * 0.5f;
        float oy = (h - overlayH) * 0.5f;
        D2D1_RECT_F overlay = D2D1::RectF(ox, oy, ox + overlayW, oy + overlayH);
        if ((float)x < overlay.left || (float)x > overlay.right || (float)y < overlay.top || (float)y > overlay.bottom) {
            m_showAdapters = false; return;
        }
        float ay = overlay.top + 10;
        for (int i = 0; i <= (int)m_availableAdapters.size(); i++) {
            if ((float)y >= ay && (float)y < ay + 28) { m_adapterIdx = i; m_showAdapters = false; return; }
            ay += 32;
        }
        return;
    }

    float w = (float)m_width;
    float rightEdge = w - 10;
    if (y >= 8 && y <= 37) {
        if ((float)x >= rightEdge - 90 && (float)x <= rightEdge) m_startRequested = true;
        else if ((float)x >= rightEdge - 165 && (float)x <= rightEdge - 95) m_clearRequested = true;
        else if ((float)x >= rightEdge - 240 && (float)x <= rightEdge - 170) { m_autoScroll = !m_autoScroll; return; }
        else if ((float)x >= 10 && (float)x <= 10 + (std::min)(w - 270.0f, 350.0f)) { m_showAdapters = true; return; }
    }

    float headerH = 45.0f;
    float footerH = 25.0f;
    float contentH = (float)m_height - headerH - footerH;
    float listH = contentH * 0.6f;
    float hdrH = 22.0f;
    float dataTop = headerH + hdrH;
    float dataBottom = headerH + listH;
    float rowH = 20.0f;
    if ((float)y > dataTop && (float)y < dataBottom) {
        int clicked = m_scrollPos + (int)(((float)y - dataTop) / rowH);
        m_selectedIdx = clicked;
        m_autoScroll = false;
    }
}

void UIManager::OnMouseMove(int x, int y) {
    m_hoverIdx = -1;
    float w = (float)m_width;
    float rightEdge = w - 10;
    if (y >= 8 && y <= 37) {
        if ((float)x >= rightEdge - 90 && (float)x <= rightEdge) m_hoverIdx = 100;
        else if ((float)x >= rightEdge - 165 && (float)x <= rightEdge - 95) m_hoverIdx = 101;
        else if ((float)x >= rightEdge - 240 && (float)x <= rightEdge - 170) m_hoverIdx = 103;
        else if ((float)x >= 10 && (float)x <= 10 + (std::min)(w - 270.0f, 350.0f)) m_hoverIdx = 102;
    }
}

void UIManager::OnMouseWheel(int delta) {
    m_autoScroll = false;
    m_scrollPos -= (delta / 120) * 3;
    if (m_scrollPos < 0) m_scrollPos = 0;
}
