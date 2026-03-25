#include "InputChat.h"
#include <fstream>
#include <map>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    // 去除行首/行尾的空白字符，避免读取配置时因空格导致 Key mismatch
    std::wstring Trim(const std::wstring& text)
    {
        const wchar_t* ws = L" \t\r\n";
        size_t start = text.find_first_not_of(ws);
        size_t end = text.find_last_not_of(ws);
        if (start == std::wstring::npos)
            return L"";
        return text.substr(start, end - start + 1);
    }

    std::map<std::wstring, std::wstring> s_textResponses;
    std::map<std::wstring, std::wstring> s_buttonResponses;
    std::wstring s_defaultResponse;

#ifdef _WIN32
    std::wstring s_activeConfigPath;
    FILETIME s_configWriteTime = {};
    bool s_hasConfigTime = false;

    // 记录当前配置文件的最后修改时间，用来实现热重新加载
    bool UpdateConfigMetadata(const std::wstring& configPath)
    {
        WIN32_FILE_ATTRIBUTE_DATA data;
        if (!GetFileAttributesExW(configPath.c_str(), GetFileExInfoStandard, &data))
            return false;

        s_configWriteTime = data.ftLastWriteTime;
        s_activeConfigPath = configPath;
        s_hasConfigTime = true;
        return true;
    }

    // 检查配置文件是否在磁盘上更新了
    bool ConfigFileChanged()
    {
        if (!s_hasConfigTime || s_activeConfigPath.empty())
            return false;

        WIN32_FILE_ATTRIBUTE_DATA data;
        if (!GetFileAttributesExW(s_activeConfigPath.c_str(), GetFileExInfoStandard, &data))
            return false;

        return memcmp(&data.ftLastWriteTime, &s_configWriteTime, sizeof(FILETIME)) != 0;
    }
#else
    bool UpdateConfigMetadata(const std::wstring& configPath)
    {
        (void)configPath;
        return true;
    }

    bool ConfigFileChanged()
    {
        return false;
    }
#endif
}

// 处理字符输入，包括回车提交、退格删除以及普通字符追加
ChatInputAction ProcessChatInputChar(std::wstring& text, WPARAM key)
{
    if (key == VK_RETURN)
        return ChatInputAction::Submit;

    if (key == VK_BACK)
    {
        if (!text.empty())
        {
            text.pop_back();
            return ChatInputAction::TextChanged;
        }

        return ChatInputAction::None;
    }

    if (key < 0x20 || key == VK_ESCAPE)
        return ChatInputAction::None;

    text.push_back(static_cast<wchar_t>(key));
    return ChatInputAction::TextChanged;
}

// 从指定配置路径加载对话/按钮映射，支持 text/button/default
bool LoadChatConfig(const std::wstring& configPath)
{
    s_textResponses.clear();
    s_buttonResponses.clear();
    s_defaultResponse.clear();

    std::wifstream file(configPath);
    if (!file.is_open())
        return false;

    std::wstring line;
    while (std::getline(file, line))
    {
        line = Trim(line);
        if (line.empty() || line.front() == L'#')
            continue;

        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos)
            continue;

        std::wstring left = Trim(line.substr(0, eq));
        std::wstring value = Trim(line.substr(eq + 1));
        if (left.empty())
            continue;

        std::wstring type = L"text";
        std::wstring key = left;
        size_t colon = left.find(L':');
        if (colon != std::wstring::npos)
        {
            type = Trim(left.substr(0, colon));
            key = Trim(left.substr(colon + 1));
        }

        if (type == L"default" || key == L"default")
        {
            s_defaultResponse = value;
            continue;
        }

        if (type == L"button")
            s_buttonResponses[key] = value;
        else
            s_textResponses[key] = value;
    }

    if (!UpdateConfigMetadata(configPath))
        return false;

    return true;
}

const std::wstring* GetChatTextResponse(const std::wstring& key)
{
    auto it = s_textResponses.find(key);
    return (it != s_textResponses.end()) ? &it->second : nullptr;
}

const std::wstring* GetChatButtonResponse(const std::wstring& buttonId)
{
    auto it = s_buttonResponses.find(buttonId);
    return (it != s_buttonResponses.end()) ? &it->second : nullptr;
}

const std::wstring* GetDefaultChatResponse()
{
    return s_defaultResponse.empty() ? nullptr : &s_defaultResponse;
}

// 更新按钮状态，返回按下/释放事件供 UI 处理
ButtonInputEvent ProcessButtonInput(ButtonInputState& state, WPARAM key, bool isDown)
{
    (void)key;

    if (isDown && !state.isDown)
    {
        state.isDown = true;
        state.wasDown = true;
        return ButtonInputEvent::Press;
    }

    if (!isDown && state.isDown)
    {
        state.isDown = false;
        return ButtonInputEvent::Release;
    }

    return ButtonInputEvent::None;
}

// 每次使用配置前调用，可在配置文件修改后刷新缓存
void MaybeReloadChatConfig()
{
#ifdef _WIN32
    if (ConfigFileChanged() && !s_activeConfigPath.empty())
        LoadChatConfig(s_activeConfigPath);
#else
    (void)ConfigFileChanged;
#endif
}
