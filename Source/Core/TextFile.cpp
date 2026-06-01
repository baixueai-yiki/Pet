#include "TextFile.h"
#include <fstream>
#include <iterator>
#include <sstream>
#include <windows.h>

namespace
{
    constexpr unsigned char kUtf8Bom[] = { 0xEF, 0xBB, 0xBF };

    bool IsLikelyUtf8(const std::string& data)
    {
        size_t i = 0;
        const size_t n = data.size();
        while (i < n)
        {
            unsigned char c = static_cast<unsigned char>(data[i]);
            if (c <= 0x7F)
            {
                ++i;
                continue;
            }
            size_t need = 0;
            if ((c & 0xE0) == 0xC0)
                need = 1;
            else if ((c & 0xF0) == 0xE0)
                need = 2;
            else if ((c & 0xF8) == 0xF0)
                need = 3;
            else
                return false;
            if (i + need >= n)
                return false;
            for (size_t j = 1; j <= need; ++j)
            {
                unsigned char cc = static_cast<unsigned char>(data[i + j]);
                if ((cc & 0xC0) != 0x80)
                    return false;
            }
            i += need + 1;
        }
        return true;
    }

    bool ConvertToWide(const std::string& data, bool hasBom, std::wstring& out)
    {
        bool preferUtf8 = hasBom || IsLikelyUtf8(data);
        UINT codePage = CP_UTF8;
        int wlen = MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), nullptr, 0);
        if (wlen <= 0 && !preferUtf8)
        {
            codePage = CP_ACP;
            wlen = MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), nullptr, 0);
        }
        if (wlen <= 0)
            return false;
        out.assign(static_cast<size_t>(wlen), L'\0');
        MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), &out[0], wlen);
        return true;
    }

    std::string WideToUtf8(const std::wstring& text)
    {
        if (text.empty())
            return std::string();
        int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (len <= 0)
            return std::string();
        std::string out(static_cast<size_t>(len), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &out[0], len, nullptr, nullptr);
        return out;
    }

    bool ReadBinary(const std::wstring& path, std::string& data)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return false;
        data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return true;
    }
}

bool TextFile::ReadText(const std::wstring& path, std::wstring& out)
{
    std::string data;
    if (!ReadBinary(path, data))
        return false;
    if (data.empty())
    {
        out.clear();
        return true;
    }
    bool hasBom = data.size() >= 3 &&
        static_cast<unsigned char>(data[0]) == kUtf8Bom[0] &&
        static_cast<unsigned char>(data[1]) == kUtf8Bom[1] &&
        static_cast<unsigned char>(data[2]) == kUtf8Bom[2];
    if (hasBom)
        data.erase(0, 3);
    return ConvertToWide(data, hasBom, out);
}

bool TextFile::ReadLines(const std::wstring& path, std::vector<std::wstring>& lines)
{
    std::wstring text;
    if (!ReadText(path, text))
        return false;
    lines.clear();
    if (text.empty())
        return true;
    std::wistringstream iss(text);
    std::wstring line;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == L'\r')
            line.pop_back();
        lines.push_back(line);
    }
    return true;
}

bool TextFile::WriteText(const std::wstring& path, const std::wstring& content, bool append, bool ensureBom)
{
    bool writeBom = ensureBom && (!append || FileIsEmpty(path));
    std::ofstream out(path, std::ios::binary | (append ? std::ios::app : std::ios::trunc));
    if (!out.is_open())
        return false;
    if (writeBom)
        out.write(reinterpret_cast<const char*>(kUtf8Bom), sizeof(kUtf8Bom));
    std::string utf8 = WideToUtf8(content);
    out.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    return true;
}

bool TextFile::FileIsEmpty(const std::wstring& path)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open())
        return true;
    std::streampos pos = in.tellg();
    if (pos < 0)
        return true;
    return pos == 0;
}
