#include "Setting.h"
#include "../../Core/Path.h"
#include "../../Core/TextFile.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <cwctype>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../Systems/Pet/Pet.h"

namespace
{
    const std::wstring kConfigPath = GetConfigPath(L"settings.json");
    struct CategoryEntry
    {
        std::wstring name;
        std::vector<std::wstring> keys;
    };

    std::vector<CategoryEntry> s_categories;
    std::map<std::wstring, std::wstring> s_settings;
    std::map<std::wstring, std::vector<std::wstring>> s_settingOptions;
    std::unordered_map<std::wstring, size_t> s_keyCategory;
    bool s_settingsLoaded = false;
    bool s_overlayVisible = false;
    RECT s_overlayRect = {};
    bool s_dragging = false;
    POINT s_dragOffset = {};
    FILETIME s_lastWriteTime = {};

    constexpr int kOverlayWidth = 150;
    constexpr int kOverlayHeight = 250;
    constexpr int kOptionButtonSize = 20;
    constexpr int kOptionButtonPadding = 18;
    constexpr int kHeaderHeight = 20;
    constexpr int kEntryHeight = 12;
    constexpr int kEntrySpacing = 6;
    constexpr int kCategorySpacing = 12;
    constexpr int kContentStartOffset = 4;
    constexpr int kContentBottomPadding = 20;
    constexpr int kEntryTextOffset = 4;
    constexpr int kWheelScrollStep = kEntryHeight + kEntrySpacing;

    constexpr wchar_t kTierLabel[] = L"\u9636";

    struct SettingButtonArea
    {
        RECT rect{};
        std::wstring key;
    };

    std::vector<SettingButtonArea> s_entryButtons;
    int s_scrollOffset = 0;
    int s_contentHeight = 0;
    int s_viewportHeight = 0;

    std::vector<std::wstring> BuildHourLevels()
    {
        std::vector<std::wstring> hours;
        hours.reserve(24);
        for (int i = 0; i < 24; ++i)
            hours.push_back(std::to_wstring(i));
        return hours;
    }

    std::vector<std::wstring> BuildTaskLevels()
    {
        std::vector<std::wstring> values;
        values.reserve(10);
        for (int i = 1; i <= 10; ++i)
            values.push_back(std::to_wstring(i));
        return values;
    }

    const std::vector<std::wstring>* GetKeyOptions(const std::wstring& key);
    bool FileTimeEqual(const FILETIME& a, const FILETIME& b);
    bool QueryConfigWriteTime(FILETIME& out);
    int CalculateContentHeight();
    void SetScrollOffset(int offset);
    void ScrollBy(int delta);

    constexpr wchar_t kCustomCategoryName[] = L"\u81ea\u5b9a\u4e49";

    // 判断是否是空白字符（用于自定义解析器）
    bool IsWhitespace(wchar_t ch)
    {
        return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n';
    }

    std::wstring Trim(const std::wstring& value)
    {
        size_t start = 0;
        while (start < value.size() && iswspace(value[start]))
            ++start;
        if (start == value.size())
            return {};
        size_t end = value.size();
        while (end > start && iswspace(value[end - 1]))
            --end;
        return value.substr(start, end - start);
    }

    void SkipWhitespace(const std::wstring& text, size_t& pos)
    {
        while (pos < text.size() && IsWhitespace(text[pos]))
            ++pos;
    }

    bool ExpectChar(const std::wstring& text, size_t& pos, wchar_t expected)
    {
        SkipWhitespace(text, pos);
        if (pos >= text.size() || text[pos] != expected)
            return false;
        ++pos;
        return true;
    }

    bool ParseJsonString(const std::wstring& text, size_t& pos, std::wstring& out)
    {
        if (pos >= text.size() || text[pos] != L'"')
            return false;
        ++pos;
        out.clear();
        while (pos < text.size())
        {
            wchar_t ch = text[pos++];
            if (ch == L'"')
                return true;
            if (ch == L'\\' && pos < text.size())
            {
                wchar_t esc = text[pos++];
                switch (esc)
                {
                case L'"': out.push_back(L'"'); break;
                case L'\\': out.push_back(L'\\'); break;
                case L'/': out.push_back(L'/'); break;
                case L'b': out.push_back(L'\b'); break;
                case L'f': out.push_back(L'\f'); break;
                case L'n': out.push_back(L'\n'); break;
                case L'r': out.push_back(L'\r'); break;
                case L't': out.push_back(L'\t'); break;
                default: out.push_back(esc); break;
                }
            }
            else
            {
                out.push_back(ch);
            }
        }
        return false;
    }

    std::wstring EscapeJson(const std::wstring& value)
    {
        std::wstring escaped;
        for (wchar_t ch : value)
        {
            switch (ch)
            {
            case L'\"': escaped += L"\\\""; break;
            case L'\\': escaped += L"\\\\"; break;
            case L'\b': escaped += L"\\b"; break;
            case L'\f': escaped += L"\\f"; break;
            case L'\n': escaped += L"\\n"; break;
            case L'\r': escaped += L"\\r"; break;
            case L'\t': escaped += L"\\t"; break;
            default: escaped.push_back(ch); break;
            }
        }
        return escaped;
    }

    bool ParseStringArray(const std::wstring& text, size_t& pos, std::vector<std::wstring>& out)
    {
        if (!ExpectChar(text, pos, L'['))
            return false;
        SkipWhitespace(text, pos);
        out.clear();
        if (pos < text.size() && text[pos] == L']')
        {
            ++pos;
            return true;
        }
        while (pos < text.size())
        {
            SkipWhitespace(text, pos);
            std::wstring item;
            if (!ParseJsonString(text, pos, item))
                return false;
            out.push_back(item);
            SkipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == L',')
            {
                ++pos;
                continue;
            }
            if (pos < text.size() && text[pos] == L']')
            {
                ++pos;
                return true;
            }
            return false;
        }
        return false;
    }

    bool ParseSettingValue(const std::wstring& text, size_t& pos, std::wstring& outValue, std::vector<std::wstring>& outOptions)
    {
        SkipWhitespace(text, pos);
        if (pos >= text.size())
            return false;
        if (text[pos] == L'"')
        {
            outOptions.clear();
            return ParseJsonString(text, pos, outValue);
        }
        if (text[pos] == L'{')
        {
            ++pos;
            outValue.clear();
            outOptions.clear();
            while (pos < text.size())
            {
                SkipWhitespace(text, pos);
                if (pos < text.size() && text[pos] == L'}')
                {
                    ++pos;
                    return true;
                }
                std::wstring field;
                if (!ParseJsonString(text, pos, field))
                    return false;
                SkipWhitespace(text, pos);
                if (!ExpectChar(text, pos, L':'))
                    return false;
                SkipWhitespace(text, pos);
                if (field == L"value")
                {
                    if (!ParseJsonString(text, pos, outValue))
                        return false;
                }
                else if (field == L"options")
                {
                    if (!ParseStringArray(text, pos, outOptions))
                        return false;
                }
                else
                {
                    std::wstring ignored;
                    if (!ParseJsonString(text, pos, ignored))
                        return false;
                }
                SkipWhitespace(text, pos);
                if (pos < text.size() && text[pos] == L',')
                {
                    ++pos;
                    continue;
                }
                if (pos < text.size() && text[pos] == L'}')
                {
                    ++pos;
                    return true;
                }
            }
            return false;
        }
        return false;
    }

    size_t GetOrCreateCategory(const std::wstring& name)
    {
        for (size_t i = 0; i < s_categories.size(); ++i)
        {
            if (s_categories[i].name == name)
                return i;
        }
        s_categories.emplace_back();
        s_categories.back().name = name;
        return s_categories.size() - 1;
    }

    void RegisterKeyToCategory(size_t categoryIndex, const std::wstring& key)
    {
        CategoryEntry& category = s_categories[categoryIndex];
        if (std::find(category.keys.begin(), category.keys.end(), key) == category.keys.end())
            category.keys.push_back(key);
        s_keyCategory[key] = categoryIndex;
    }

    size_t EnsureKeyExists(const std::wstring& key)
    {
        auto it = s_keyCategory.find(key);
        if (it != s_keyCategory.end())
            return it->second;
        size_t index = GetOrCreateCategory(kCustomCategoryName);
        RegisterKeyToCategory(index, key);
        return index;
    }

    bool ParseCategoryObject(const std::wstring& text, size_t& pos, CategoryEntry& category, size_t categoryIndex)
    {
        if (!ExpectChar(text, pos, L'{'))
            return false;
        while (true)
        {
            SkipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == L'}')
            {
                ++pos;
                return true;
            }
            std::wstring key;
            if (!ParseJsonString(text, pos, key))
                return false;
            SkipWhitespace(text, pos);
            if (!ExpectChar(text, pos, L':'))
                return false;
            SkipWhitespace(text, pos);
            std::wstring value;
            std::vector<std::wstring> options;
            if (!ParseSettingValue(text, pos, value, options))
                return false;
            s_settings[key] = value;
            if (!options.empty())
                s_settingOptions[key] = options;
            RegisterKeyToCategory(categoryIndex, key);
            SkipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == L',')
            {
                ++pos;
                continue;
            }
            if (pos < text.size() && text[pos] == L'}')
            {
                ++pos;
                return true;
            }
            return false;
        }
    }

    bool ParseSettingsJson(const std::wstring& text)
    {
        size_t pos = 0;
        SkipWhitespace(text, pos);
        if (!ExpectChar(text, pos, L'{'))
            return false;
        while (true)
        {
            SkipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == L'}')
            {
                ++pos;
                return true;
            }
            std::wstring categoryName;
            if (!ParseJsonString(text, pos, categoryName))
                return false;
            SkipWhitespace(text, pos);
            if (!ExpectChar(text, pos, L':'))
                return false;
            size_t index = GetOrCreateCategory(categoryName);
            CategoryEntry& category = s_categories[index];
            SkipWhitespace(text, pos);
            if (!ParseCategoryObject(text, pos, category, index))
                return false;
            SkipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == L',')
            {
                ++pos;
                continue;
            }
            if (pos < text.size() && text[pos] == L'}')
            {
                ++pos;
                return true;
            }
            return false;
        }
    }

    bool LoadSettings()
    {
        s_settings.clear();
        s_categories.clear();
        s_settingOptions.clear();
        s_keyCategory.clear();

        std::wstring raw;
        if (!TextFile::ReadText(kConfigPath, raw) || raw.empty())
        {
            s_lastWriteTime = {};
            s_settingsLoaded = true;
            return true;
        }

        if (!ParseSettingsJson(raw))
        {
            s_lastWriteTime = {};
            s_settingsLoaded = true;
            return true;
        }

        FILETIME ft;
        if (QueryConfigWriteTime(ft))
            s_lastWriteTime = ft;
        else
            s_lastWriteTime = {};
        s_settingsLoaded = true;
        return true;
    }

    bool PersistSettings()
    {
        std::wstringstream ss;
        ss << L"{\r\n";
        for (size_t i = 0; i < s_categories.size(); ++i)
        {
            const auto& category = s_categories[i];
            if (category.keys.empty())
                continue;
            ss << L"  \"" << EscapeJson(category.name) << L"\": {\r\n";
            for (size_t j = 0; j < category.keys.size(); ++j)
            {
                const auto& key = category.keys[j];
                auto it = s_settings.find(key);
                if (it == s_settings.end())
                    continue;
                auto optIt = s_settingOptions.find(key);
                if (optIt == s_settingOptions.end() || optIt->second.empty())
                {
                    ss << L"    \"" << EscapeJson(key) << L"\": \"" << EscapeJson(it->second) << L"\"";
                }
                else
                {
                    ss << L"    \"" << EscapeJson(key) << L"\": {\r\n";
                    ss << L"      \"value\": \"" << EscapeJson(it->second) << L"\",\r\n";
                    ss << L"      \"options\": [";
                    for (size_t k = 0; k < optIt->second.size(); ++k)
                    {
                        ss << L"\"" << EscapeJson(optIt->second[k]) << L"\"";
                        if (k + 1 < optIt->second.size())
                            ss << L", ";
                    }
                    ss << L"]\r\n";
                    ss << L"    }";
                }
                if (j + 1 < category.keys.size())
                    ss << L",";
                ss << L"\r\n";
            }
            ss << L"  }";
            if (i + 1 < s_categories.size())
                ss << L",";
            ss << L"\r\n";
        }
        ss << L"}\r\n";
        bool ok = TextFile::WriteText(kConfigPath, ss.str(), false, true);
        if (ok)
        {
            FILETIME ft;
            if (QueryConfigWriteTime(ft))
                s_lastWriteTime = ft;
        }
        return ok;
    }

    bool ReloadIfChanged()
    {
        FILETIME ft;
        if (!QueryConfigWriteTime(ft))
            return false;
        if (FileTimeEqual(ft, s_lastWriteTime))
            return false;
        return LoadSettings();
    }

    bool EnsureLoaded()
    {
        if (!s_settingsLoaded)
            return LoadSettings();
        ReloadIfChanged();
        return true;
    }

    void RecalculateOverlayRect()
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int width = std::min(kOverlayWidth, screenW - 80);
        int height = std::min(kOverlayHeight, screenH - 120);
        int top = g_pet.y;
        if (top + height > screenH)
            top = std::max(20, screenH - height - 20);
        if (top < 20)
            top = 20;

        const int margin = 12;
        int left = g_pet.x - width - margin;
        if (left < 20)
        {
            // No room on left, try right side
            int rightCandidate = g_pet.x + g_pet.w + margin;
            if (rightCandidate + width <= screenW - 20)
                left = rightCandidate;
            else
                left = std::max(20, std::min(screenW - width - 20, g_pet.x));
        }

        s_overlayRect.left = left;
        s_overlayRect.top = top;
        s_overlayRect.right = left + width;
        s_overlayRect.bottom = top + height;
        s_dragging = false;
    }

    bool OverlayContainsPoint(int x, int y)
    {
        return x >= s_overlayRect.left &&
               x <= s_overlayRect.right &&
               y >= s_overlayRect.top &&
               y <= s_overlayRect.bottom;
    }

    const std::vector<std::wstring>* GetKeyOptions(const std::wstring& key)
    {
        auto it = s_settingOptions.find(key);
        if (it == s_settingOptions.end() || it->second.empty())
            return nullptr;
        return &it->second;
    }

    bool FileTimeEqual(const FILETIME& a, const FILETIME& b)
    {
        return a.dwLowDateTime == b.dwLowDateTime && a.dwHighDateTime == b.dwHighDateTime;
    }

    bool QueryConfigWriteTime(FILETIME& out)
    {
        WIN32_FILE_ATTRIBUTE_DATA info = {};
        if (!GetFileAttributesExW(kConfigPath.c_str(), GetFileExInfoStandard, &info))
            return false;
        out = info.ftLastWriteTime;
        return true;
    }

    int CalculateContentHeight()
    {
        int total = 0;
        for (const auto& category : s_categories)
        {
            total += kHeaderHeight;
            total += static_cast<int>(category.keys.size()) * (kEntryHeight + kEntrySpacing);
            total += kCategorySpacing;
        }
        if (total > 0)
            total -= kCategorySpacing;
        return std::max(0, total);
    }

    void SetScrollOffset(int offset)
    {
        if (s_viewportHeight <= 0)
        {
            s_scrollOffset = 0;
            return;
        }
        int maxOffset = std::max(0, s_contentHeight - s_viewportHeight);
        s_scrollOffset = std::clamp(offset, 0, maxOffset);
    }

    void ScrollBy(int delta)
    {
        SetScrollOffset(s_scrollOffset + delta);
    }

    std::wstring BuildOptionLabel(const std::wstring& key)
    {
        const std::vector<std::wstring>* levels = GetKeyOptions(key);
        if (!levels)
            return kTierLabel;
        auto it = s_settings.find(key);
        std::wstring current = (it != s_settings.end()) ? it->second : L"";
        for (const auto& value : *levels)
        {
            if (value == current)
                return std::wstring(kTierLabel) + L"(" + value + L")";
        }
        return kTierLabel;
    }

    bool CycleOptionValue(const std::wstring& key)
    {
        const std::vector<std::wstring>* levels = GetKeyOptions(key);
        if (!levels || levels->empty())
            return false;
        auto it = s_settings.find(key);
        std::wstring current = (it != s_settings.end()) ? it->second : L"";
        size_t idx = 0;
        while (idx < levels->size() && (*levels)[idx] != current)
            ++idx;
        size_t next = (idx >= levels->size()) ? 0 : (idx + 1) % levels->size();
        EnsureLoaded();
        s_settings[key] = (*levels)[next];
        EnsureKeyExists(key);
        if (!PersistSettings())
        {
            if (current.empty())
                s_settings.erase(key);
            else
                s_settings[key] = current;
            return false;
        }
        return true;
    }
}

namespace Setting
{
    std::wstring GetString(const std::wstring& key, const std::wstring& defaultValue)
    {
        if (!EnsureLoaded())
            return defaultValue;
        auto it = s_settings.find(key);
        return (it != s_settings.end() && !it->second.empty()) ? it->second : defaultValue;
    }

    int GetInt(const std::wstring& key, int defaultValue)
    {
        std::wstring text = GetString(key, {});
        if (text.empty())
            return defaultValue;
        try
        {
            return std::stoi(text);
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    bool ShowOverlay()
    {
        EnsureLoaded();
        RecalculateOverlayRect();
        if (s_overlayVisible)
            return true;
        s_overlayVisible = true;
        return true;
    }

    void HideOverlay()
    {
        s_overlayVisible = false;
        s_dragging = false;
    }

    bool ToggleOverlay()
    {
        if (s_overlayVisible)
        {
            HideOverlay();
            return false;
        }
        ShowOverlay();
        return true;
    }

    bool IsOverlayVisible()
    {
        return s_overlayVisible;
    }

    bool TryApplyInlineValue(const std::wstring& command)
    {
        if (command.empty())
            return false;
        size_t eq = command.find(L'=');
        if (eq == std::wstring::npos)
            return false;
        std::wstring key = Trim(command.substr(0, eq));
        std::wstring value = Trim(command.substr(eq + 1));
        if (key.empty())
            return false;
        EnsureLoaded();
        s_settings[key] = value;
        EnsureKeyExists(key);
        return PersistSettings();
    }

    void RenderOverlay(HDC hdc)
    {
    if (!s_overlayVisible || !hdc)
            return;

        s_entryButtons.clear();

        HBRUSH fill = CreateSolidBrush(RGB(253, 229, 237));
        FillRect(hdc, &s_overlayRect, fill);
        DeleteObject(fill);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(30, 30, 30));
        HFONT oldFont = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
        int contentTop = s_overlayRect.top + kContentStartOffset;
        s_viewportHeight = s_overlayRect.bottom - contentTop - kContentBottomPadding;
        if (s_viewportHeight < 0)
            s_viewportHeight = 0;
        s_contentHeight = CalculateContentHeight();
        SetScrollOffset(s_scrollOffset);

        if (s_categories.empty())
        {
            const wchar_t* help = L"尚未定义任何设置项。";
            TextOutW(hdc, s_overlayRect.left + 18, contentTop, help, static_cast<int>(wcslen(help)));
            s_contentHeight = kEntryHeight;
        }
        else
        {
        int contentY = 0;
        int entryStride = kEntryHeight + kEntrySpacing;
        for (const auto& category : s_categories)
        {
            int headerTop = contentTop + contentY - s_scrollOffset;
            RECT headerRect = { s_overlayRect.left + 10,
                                headerTop - 2,
                                s_overlayRect.right - 10,
                                headerTop + kHeaderHeight - 2 };
            if (headerTop + kHeaderHeight >= contentTop && headerTop <= s_overlayRect.bottom - kContentBottomPadding)
            {
                HBRUSH headerFill = CreateSolidBrush(RGB(255, 240, 244));
                FillRect(hdc, &headerRect, headerFill);
                DeleteObject(headerFill);
                SetTextColor(hdc, RGB(60, 20, 30));
                TextOutW(hdc, s_overlayRect.left + 18, headerTop,
                         (L"[" + category.name + L"]").c_str(),
                         static_cast<int>(category.name.size() + 2));
                SetTextColor(hdc, RGB(30, 30, 30));
            }
            contentY += kHeaderHeight;

            for (const auto& key : category.keys)
            {
                int entryTop = contentTop + contentY - s_scrollOffset;
                int entryBottom = entryTop + kEntryHeight;
                if (entryBottom >= contentTop && entryTop <= s_overlayRect.bottom - kContentBottomPadding)
                {
                    std::wstring value;
                    auto it = s_settings.find(key);
                    if (it != s_settings.end())
                        value = it->second;
                    std::wstring line = key + L" = " + value;
                    TextOutW(hdc, s_overlayRect.left + 18, entryTop + kEntryTextOffset, line.c_str(), static_cast<int>(line.size()));

                    const std::vector<std::wstring>* levels = GetKeyOptions(key);
                    if (levels)
                    {
                        SettingButtonArea area{};
                        area.key = key;
                        area.rect.right = s_overlayRect.right - kOptionButtonPadding;
                        area.rect.left = area.rect.right - kOptionButtonSize;
                        int buttonTop = entryTop + (kEntryHeight - kOptionButtonSize) / 2;
                        area.rect.top = buttonTop;
                        area.rect.bottom = area.rect.top + kOptionButtonSize;
                        s_entryButtons.push_back(area);

                        HBRUSH buttonBrush = CreateSolidBrush(RGB(255, 245, 245));
                        HBRUSH prevBrush = (HBRUSH)SelectObject(hdc, buttonBrush);
                        HPEN buttonPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
                        HPEN prevPen = (HPEN)SelectObject(hdc, buttonPen);
                        Rectangle(hdc, area.rect.left, area.rect.top, area.rect.right, area.rect.bottom);
                        SelectObject(hdc, prevBrush);
                        SelectObject(hdc, prevPen);
                        DeleteObject(buttonBrush);
                        DeleteObject(buttonPen);

                        const int arrowWidth = 6;
                        const int arrowHeight = 10;
                        const int centerX = (area.rect.left + area.rect.right) / 2;
                        const int centerY = (area.rect.top + area.rect.bottom) / 2;
                        POINT arrowPoints[3] = {
                            { centerX - arrowWidth / 2, centerY - arrowHeight / 2 },
                            { centerX - arrowWidth / 2, centerY + arrowHeight / 2 },
                            { centerX + arrowWidth / 2, centerY }
                        };
                        HPEN arrowPen = CreatePen(PS_SOLID, 2, RGB(80, 80, 80));
                        HPEN prevArrowPen = (HPEN)SelectObject(hdc, arrowPen);
                        MoveToEx(hdc, arrowPoints[0].x, arrowPoints[0].y, nullptr);
                        LineTo(hdc, arrowPoints[1].x, arrowPoints[1].y);
                        LineTo(hdc, arrowPoints[2].x, arrowPoints[2].y);
                        SelectObject(hdc, prevArrowPen);
                        DeleteObject(arrowPen);
                    }
                }
                contentY += entryStride;
            }
            contentY += kCategorySpacing;
        }
        }

        SelectObject(hdc, oldFont);
        // no close button UI anymore
    }

    bool HandleOverlayMouse(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (!s_overlayVisible)
            return false;
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        bool inside = OverlayContainsPoint(x, y);

        if (msg == WM_MOUSEWHEEL)
        {
            if (!inside)
                return false;
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta != 0)
            {
                ScrollBy(-(delta / WHEEL_DELTA) * kWheelScrollStep);
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return true;
        }

        if (msg == WM_LBUTTONDOWN)
        {
            for (const auto& area : s_entryButtons)
            {
                if (x >= area.rect.left && x <= area.rect.right &&
                    y >= area.rect.top && y <= area.rect.bottom)
                {
                    if (CycleOptionValue(area.key))
                    {
                        InvalidateRect(hwnd, nullptr, TRUE);
                        return true;
                    }
                }
            }

            if (!inside)
            {
                HideOverlay();
                InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }

            s_dragging = true;
            s_dragOffset.x = x - s_overlayRect.left;
            s_dragOffset.y = y - s_overlayRect.top;
            return true;
        }

        if (msg == WM_MOUSEMOVE && s_dragging)
        {
            int width = s_overlayRect.right - s_overlayRect.left;
            int height = s_overlayRect.bottom - s_overlayRect.top;
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            int maxLeft = std::max(0, screenW - width);
            int maxTop = std::max(0, screenH - height);
            int left = x - s_dragOffset.x;
            int top = y - s_dragOffset.y;
            left = std::min(std::max(0, left), maxLeft);
            top = std::min(std::max(0, top), maxTop);
            s_overlayRect.left = left;
            s_overlayRect.top = top;
            s_overlayRect.right = left + width;
            s_overlayRect.bottom = top + height;
        // No close button anymore.
            InvalidateRect(hwnd, nullptr, TRUE);
            return true;
        }

        if ((msg == WM_LBUTTONUP || msg == WM_RBUTTONUP) && s_dragging)
        {
            s_dragging = false;
            return true;
        }

        return inside;
    }

    bool IsPointInsideOverlay(int x, int y)
    {
        if (!s_overlayVisible)
            return false;
        return OverlayContainsPoint(x, y);
    }
}
