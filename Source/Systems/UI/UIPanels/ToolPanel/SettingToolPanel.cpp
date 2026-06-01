#include "SettingToolPanel.h"
#include "TaskToolPanel.h"
#include "../../../../Runtime/StateManager.h"
#include "../../../../Core/Path.h"
#include "../../../../Core/TextFile.h"
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
#include "../../../Pet/PetActor.h"

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
    RECT s_overlayRect = {};
    bool s_dragging = false;
    POINT s_dragOffset = {};
    FILETIME s_lastWriteTime = {};

    constexpr int kTabWidth = 24;
    constexpr int kTabHeight = 60;
    constexpr int kOverlayWidth = 150 + kTabWidth;
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

    // UI 状态统一走 Runtime/StateManager 的 ui.panel 维度
    // 值: "idle" | "input" | "overlay_settings" | "overlay_tasks" | "overlay_task_menu"
    static bool IsOverlayOpen() {
        auto s = StateGet(L"ui.panel");
        return s == L"overlay_settings" || s == L"overlay_tasks" || s == L"overlay_task_menu";
    }
    static int ActiveTabIndex() {
        auto s = StateGet(L"ui.panel");
        if (s == L"overlay_settings") return 0;
        if (s == L"overlay_tasks" || s == L"overlay_task_menu") return 1;
        return -1;
    }

    // 任务页 hit-test + 清理弹窗
    struct TaskItemArea { RECT rect; int index; };
    static std::vector<TaskItemArea> s_taskItemAreas;
    static HWND s_killPopupWnd = nullptr;
    static int s_killTaskIndex = -1;

    constexpr wchar_t kTierLabel[] = L"\u9636";

    struct SettingButtonArea
    {
        RECT prevRect{};   // < 按钮
        RECT nextRect{};   // > 按钮
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

    bool PrevOptionValue(const std::wstring& key)
    {
        const std::vector<std::wstring>* levels = GetKeyOptions(key);
        if (!levels || levels->empty())
            return false;
        auto it = s_settings.find(key);
        std::wstring current = (it != s_settings.end()) ? it->second : L"";
        size_t idx = levels->size() - 1;
        while (idx < levels->size() && (*levels)[idx] != current)
            --idx;
        size_t prev = (idx == 0) ? levels->size() - 1 : idx - 1;
        EnsureLoaded();
        s_settings[key] = (*levels)[prev];
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
        if (IsOverlayOpen())
            return true;
        StateSet(L"ui.panel", L"overlay_settings");
        return true;
    }

    void HideOverlay()
    {
        StateSet(L"ui.panel", L"idle");
        s_dragging = false;
        if (s_killPopupWnd)
        {
            DestroyWindow(s_killPopupWnd);
            s_killPopupWnd = nullptr;
            s_killTaskIndex = -1;
        }
    }

    bool ToggleOverlay()
    {
        if (IsOverlayOpen())
        {
            HideOverlay();
            return false;
        }
        ShowOverlay();
        return true;
    }

    bool IsOverlayVisible()
    {
        return IsOverlayOpen();
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
    if (!IsOverlayOpen() || !hdc)
            return;

        s_entryButtons.clear();

        // 整体背景
        HBRUSH fill = CreateSolidBrush(RGB(255, 245, 250));
        FillRect(hdc, &s_overlayRect, fill);
        DeleteObject(fill);

        // 左侧标签栏背景
        RECT tabBar = s_overlayRect;
        tabBar.right = tabBar.left + kTabWidth;
        HBRUSH tabBg = CreateSolidBrush(RGB(255, 238, 245));
        FillRect(hdc, &tabBar, tabBg);
        DeleteObject(tabBg);

        // 标签分隔线
        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(240, 215, 225));
        HPEN oldPen = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, tabBar.right, s_overlayRect.top, nullptr);
        LineTo(hdc, tabBar.right, s_overlayRect.bottom);
        SelectObject(hdc, oldPen);
        DeleteObject(sepPen);

        SetBkMode(hdc, TRANSPARENT);
        HFONT oldFont = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

        // 竖排标签文字
        const wchar_t* tabLabels[] = { L"设置", L"任务" };
        for (int i = 0; i < 2; ++i)
        {
            RECT tabRc = { s_overlayRect.left,
                           s_overlayRect.top + i * kTabHeight,
                           s_overlayRect.left + kTabWidth,
                           s_overlayRect.top + (i + 1) * kTabHeight };

            // 选中标签高亮
            if (i == ActiveTabIndex())
            {
                HBRUSH selBg = CreateSolidBrush(RGB(255, 245, 250));
                FillRect(hdc, &tabRc, selBg);
                DeleteObject(selBg);
                SetTextColor(hdc, RGB(210, 130, 155));
            }
            else
            {
                SetTextColor(hdc, RGB(175, 150, 160));
            }

            // 竖排绘制（每个字一行）
            const wchar_t* label = tabLabels[i];
            int len = (int)wcslen(label);
            int charH = 18;
            int totalH = len * charH;
            int startY = tabRc.top + (kTabHeight - totalH) / 2;
            for (int c = 0; c < len; ++c)
            {
                wchar_t ch[2] = { label[c], L'\0' };
                int chW = (ch[0] >= 0x4e00) ? 16 : 10; // CJK char width estimate
                TextOutW(hdc, tabRc.left + (kTabWidth - chW) / 2, startY + c * charH, ch, 1);
            }
        }

        // 内容区域（标签右侧）
        int contentLeft = s_overlayRect.left + kTabWidth + 4;
        int contentTop = s_overlayRect.top + kContentStartOffset;
        s_viewportHeight = s_overlayRect.bottom - contentTop - kContentBottomPadding;
        if (s_viewportHeight < 0)
            s_viewportHeight = 0;

        if (ActiveTabIndex() == 0)
        {
            // === 设置页 ===
            SetTextColor(hdc, RGB(30, 30, 30));
            s_contentHeight = CalculateContentHeight();
            SetScrollOffset(s_scrollOffset);

            if (s_categories.empty())
            {
                const wchar_t* help = L"no settings yet";
                TextOutW(hdc, contentLeft + 4, contentTop, help, (int)wcslen(help));
                s_contentHeight = kEntryHeight;
            }
            else
            {
                int contentY = 0;
                int entryStride = kEntryHeight + kEntrySpacing;
                for (const auto& category : s_categories)
                {
                    int headerTop = contentTop + contentY - s_scrollOffset;
                    RECT headerRect = { contentLeft,
                                        headerTop - 2,
                                        s_overlayRect.right - 4,
                                        headerTop + kHeaderHeight - 2 };
                    if (headerTop + kHeaderHeight >= contentTop && headerTop <= s_overlayRect.bottom - kContentBottomPadding)
                    {
                        HBRUSH headerFill = CreateSolidBrush(RGB(255, 245, 250));
                        FillRect(hdc, &headerRect, headerFill);
                        DeleteObject(headerFill);
                        SetTextColor(hdc, RGB(185, 130, 150));
                        TextOutW(hdc, contentLeft + 4, headerTop,
                                 (L"[" + category.name + L"]").c_str(),
                                 (int)(category.name.size() + 2));
                        SetTextColor(hdc, RGB(160, 130, 145));
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
                            TextOutW(hdc, contentLeft + 4, entryTop + kEntryTextOffset, line.c_str(), (int)line.size());

                            const std::vector<std::wstring>* levels = GetKeyOptions(key);
                            if (levels)
                            {
                                const int btnSize = 14;
                                int btnY = entryTop + (kEntryHeight - btnSize) / 2;
                                int lineRight = s_overlayRect.right - 4;

                                SettingButtonArea area{};
                                area.key = key;
                                area.nextRect = { lineRight - btnSize, btnY, lineRight, btnY + btnSize };
                                area.prevRect = { lineRight - btnSize * 2 - 2, btnY, lineRight - btnSize - 2, btnY + btnSize };
                                s_entryButtons.push_back(area);

                                // < button
                                {
                                    RECT r = area.prevRect;
                                    HBRUSH b = CreateSolidBrush(RGB(255, 248, 250));
                                    FillRect(hdc, &r, b); DeleteObject(b);
                                    HPEN p = CreatePen(PS_SOLID, 1, RGB(245, 225, 235));
                                    SelectObject(hdc, p);
                                    SelectObject(hdc, GetStockObject(NULL_BRUSH));
                                    Rectangle(hdc, r.left, r.top, r.right, r.bottom);
                                    DeleteObject(p);
                                    SetTextColor(hdc, RGB(190, 150, 165));
                                    TextOutW(hdc, r.left + 4, r.top, L"<", 1);
                                }
                                // > button
                                {
                                    RECT r = area.nextRect;
                                    HBRUSH b = CreateSolidBrush(RGB(255, 248, 250));
                                    FillRect(hdc, &r, b); DeleteObject(b);
                                    HPEN p = CreatePen(PS_SOLID, 1, RGB(245, 225, 235));
                                    SelectObject(hdc, p);
                                    SelectObject(hdc, GetStockObject(NULL_BRUSH));
                                    Rectangle(hdc, r.left, r.top, r.right, r.bottom);
                                    DeleteObject(p);
                                    SetTextColor(hdc, RGB(190, 150, 165));
                                    TextOutW(hdc, r.left + 4, r.top, L">", 1);
                                }
                            }
                        }
                        contentY += entryStride;
                    }
                    contentY += kCategorySpacing;
                }
            }
        }
        else
        {
            // === 任务页 ===
            s_taskItemAreas.clear();
            SetTextColor(hdc, RGB(160, 130, 145));
            int count = TaskToolPanel::GetTaskCount();
            if (count == 0)
            {
                TextOutW(hdc, contentLeft + 4, contentTop, L"No visible windows", 18);
            }
            else
            {
                int y = contentTop;
                int itemH = 20;
                for (int i = 0; i < count; ++i)
                {
                    if (y + itemH > s_overlayRect.bottom - kContentBottomPadding)
                        break;

                    HICON icon = TaskToolPanel::GetTaskIcon(i);
                    if (icon)
                        DrawIconEx(hdc, contentLeft, y, icon, 16, 16, 0, nullptr, DI_NORMAL);

                    std::wstring title = TaskToolPanel::GetTaskTitle(i);
                    if (title.empty())
                        title = L"(untitled)";
                    int maxChars = (s_overlayRect.right - contentLeft - 24) / 8;
                    if ((int)title.size() > maxChars)
                        title = title.substr(0, maxChars - 1) + L"…";
                    TextOutW(hdc, contentLeft + 20, y + 1, title.c_str(), (int)title.size());

                    // 记录 hit-test 区域
                    TaskItemArea area;
                    area.rect = { contentLeft, y, s_overlayRect.right - 4, y + itemH };
                    area.index = i;
                    s_taskItemAreas.push_back(area);

                    y += itemH;
                }
            }
        }

        SelectObject(hdc, oldFont);
    }

    // 清理弹窗 WndProc
    LRESULT CALLBACK KillPopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            HBRUSH bg = CreateSolidBrush(RGB(255, 245, 250));
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);
            HPEN border = CreatePen(PS_SOLID, 1, RGB(240, 200, 215));
            HGDIOBJ oldPen = SelectObject(hdc, border);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, 0, 0, rc.right, rc.bottom);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(border);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(200, 120, 145));
            HFONT f = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
            HFONT oldF = (HFONT)SelectObject(hdc, f);
            TextOutW(hdc, 10, 6, L"清理", 2);
            SelectObject(hdc, oldF);
            DeleteObject(f);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONUP:
            if (s_killTaskIndex >= 0)
            {
                TaskToolPanel::KillTask(s_killTaskIndex);
                s_killTaskIndex = -1;
            }
            DestroyWindow(hwnd);
            s_killPopupWnd = nullptr;
            StateSet(L"ui.panel", L"overlay_tasks");
            break;
        case WM_DESTROY:
            s_killPopupWnd = nullptr;
            StateSet(L"ui.panel", L"overlay_tasks");
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void ShowKillPopup(HWND parent, int screenX, int screenY, int taskIndex)
    {
        if (s_killPopupWnd)
            DestroyWindow(s_killPopupWnd);

        s_killTaskIndex = taskIndex;
        StateSet(L"ui.panel", L"overlay_task_menu");
        s_killPopupWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            L"STATIC", nullptr,
            WS_POPUP, screenX, screenY, 56, 32,
            parent, nullptr, GetModuleHandle(nullptr), nullptr);
        if (!s_killPopupWnd)
            return;
        SetLayeredWindowAttributes(s_killPopupWnd, 0, 248, LWA_ALPHA);
        SetWindowLongPtrW(s_killPopupWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(KillPopupProc));
        ShowWindow(s_killPopupWnd, SW_SHOW);
        InvalidateRect(s_killPopupWnd, nullptr, TRUE);
    }

    bool HandleOverlayMouse(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (!IsOverlayOpen())
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

        if (msg == WM_RBUTTONDOWN && ActiveTabIndex() == 1)
        {
            for (const auto& area : s_taskItemAreas)
            {
                if (x >= area.rect.left && x <= area.rect.right &&
                    y >= area.rect.top && y <= area.rect.bottom)
                {
                    POINT pt = { x, y };
                    ClientToScreen(hwnd, &pt);
                    ShowKillPopup(hwnd, pt.x + 8, pt.y, area.index);
                    return true;
                }
            }
            if (!inside)
                return false;
            return true; // 在任务页内但不在任何任务项上，吞掉事件
        }

        if (msg == WM_LBUTTONDOWN)
        {
            // 检查标签栏点击
            int tabIndex = -1;
            if (x >= s_overlayRect.left && x <= s_overlayRect.left + kTabWidth)
            {
                tabIndex = (y - s_overlayRect.top) / kTabHeight;
                if (tabIndex >= 0 && tabIndex <= 1)
                {
                    StateSet(L"ui.panel", tabIndex == 0 ? L"overlay_settings" : L"overlay_tasks");
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return true;
                }
            }

            for (const auto& area : s_entryButtons)
            {
                // 检测 < 按钮
                if (x >= area.prevRect.left && x <= area.prevRect.right &&
                    y >= area.prevRect.top && y <= area.prevRect.bottom)
                {
                    if (PrevOptionValue(area.key))
                    {
                        InvalidateRect(hwnd, nullptr, TRUE);
                        return true;
                    }
                }
                // 检测 > 按钮
                if (x >= area.nextRect.left && x <= area.nextRect.right &&
                    y >= area.nextRect.top && y <= area.nextRect.bottom)
                {
                    if (CycleOptionValue(area.key))
                    {
                        InvalidateRect(hwnd, nullptr, TRUE);
                        return true;
                    }
                }
            }

            if (!inside)
                return false; // 不在浮层内，不处理，交给下层（桌宠交互）

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
        if (!IsOverlayOpen())
            return false;
        return OverlayContainsPoint(x, y);
    }

    void TickTaskRefresh(HWND hwnd)
    {
        static DWORD s_lastRefresh = 0;
        if (!IsOverlayOpen() || ActiveTabIndex() != 1)
            return;
        // 右键弹窗打开时不刷新（避免列表变动导致误杀）
        if (StateGet(L"ui.panel") == L"overlay_task_menu")
            return;
        DWORD now = GetTickCount();
        if (now - s_lastRefresh >= 1000)
        {
            s_lastRefresh = now;
            TaskToolPanel::RefreshTaskList();
            InvalidateRect(hwnd, nullptr, TRUE);
        }
    }
}

