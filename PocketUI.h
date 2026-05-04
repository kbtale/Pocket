#pragma once
#include "framework.h"
#include <d2d1.h>
#include <dwrite.h>
#include <map>
#include "PocketUtils.h"

class UIManager {
public:
    UIManager();
    ~UIManager();

    HRESULT Initialize(HWND hWnd);
    void Shutdown();
    void Resize(UINT width, UINT height);
    void Render(const std::vector<PocketUtils::PacketData>& packets, uint32_t dropped, bool is_running);

    void OnLButtonDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(int delta);
    void OnNewPackets(int totalCount);

    bool IsStartRequested() { bool r = m_startRequested; m_startRequested = false; return r; }
    bool IsClearRequested() { bool r = m_clearRequested; m_clearRequested = false; return r; }
    
    std::vector<std::string> GetSelectedAdapters() const {
        std::vector<std::string> result;
        if (m_adapterIdx == 0) {
            for (const auto& a : m_availableAdapters) result.push_back(a.name);
        } else if (m_adapterIdx > 0 && m_adapterIdx <= (int)m_availableAdapters.size()) {
            result.push_back(m_availableAdapters[m_adapterIdx - 1].name);
        }
        return result;
    }

private:
    HRESULT CreateResources();
    void DiscardResources();
    std::wstring ResolveAdapterName(const std::string& rawName);

    HWND m_hWnd;
    ID2D1Factory* m_pFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    IDWriteFactory* m_pDWriteFactory;
    
    ID2D1SolidColorBrush* m_pBackBrush;
    ID2D1SolidColorBrush* m_pSurfaceBrush;
    ID2D1SolidColorBrush* m_pTextBrush;
    ID2D1SolidColorBrush* m_pAccentBrush;
    ID2D1SolidColorBrush* m_pZebraBrush;

    IDWriteTextFormat* m_pMainFont;
    IDWriteTextFormat* m_pFixedFont;
    IDWriteTextFormat* m_pBtnFont;

    UINT m_width;
    UINT m_height;
    int m_scrollPos;
    int m_selectedIdx;
    bool m_startRequested;
    bool m_clearRequested;
    int m_hoverIdx;

    bool m_showAdapters;
    int m_adapterIdx;
    bool m_autoScroll;
    std::vector<PocketUtils::AdapterInfo> m_availableAdapters;
    std::map<std::string, std::string> m_adapterNameMap;
};
