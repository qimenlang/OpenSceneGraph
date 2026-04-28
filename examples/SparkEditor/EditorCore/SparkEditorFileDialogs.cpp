#include "SparkEditorFileDialogs.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

namespace spark_editor
{
namespace
{

bool IsInvalidFileChar(char c)
{
    static const char kInvalid[] = "\\/:*?\"<>|";
    return std::strchr(kInvalid, c) != nullptr;
}

} // namespace

std::string SanitizeFileStem(const std::string& name)
{
    std::string s;
    s.reserve(name.size());
    for (char c : name)
    {
        if (c == '\0')
            break;
        s.push_back(IsInvalidFileChar(c) ? '_' : c);
    }
    while (!s.empty() && (s.front() == ' ' || s.front() == '.'))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '.'))
        s.pop_back();
    if (s.empty())
        s = "particle_system";
    return s;
}

#ifdef _WIN32

namespace
{

std::string WideToUtf8(const wchar_t* w)
{
    if (!w || !*w)
        return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return {};
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, &out[0], n, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0')
        out.pop_back();
    return out;
}

std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty())
        return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (n <= 0)
        return L"";
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], n);
    if (!out.empty() && out.back() == L'\0')
        out.pop_back();
    return out;
}

} // namespace

bool ShowOpenParticleFileDialog(std::string& outPath)
{
    wchar_t buf[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    static const wchar_t kFilter[] =
        L"SPK/XML (*.spk;*.xml)\0*.spk;*.xml\0"
        L"SPK (*.spk)\0*.spk\0"
        L"XML (*.xml)\0*.xml\0"
        L"All (*.*)\0*.*\0\0";
    ofn.lpstrFilter = kFilter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn))
    {
        outPath.clear();
        return false;
    }
    outPath = WideToUtf8(buf);
    return !outPath.empty();
}

bool ShowOpenImageFileDialog(std::string& outPath)
{
    wchar_t buf[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    static const wchar_t kFilter[] =
        L"Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds)\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds\0"
        L"PNG (*.png)\0*.png\0"
        L"JPEG (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0"
        L"BMP (*.bmp)\0*.bmp\0"
        L"TGA (*.tga)\0*.tga\0"
        L"DDS (*.dds)\0*.dds\0"
        L"All (*.*)\0*.*\0\0";
    ofn.lpstrFilter = kFilter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn))
    {
        outPath.clear();
        return false;
    }
    outPath = WideToUtf8(buf);
    return !outPath.empty();
}

bool ShowSaveParticleFileDialog(const std::string& suggestedStem, std::string& outPath)
{
    const std::string stem = SanitizeFileStem(suggestedStem);
    const std::string defName = stem + ".spk";
    std::wstring wdef = Utf8ToWide(defName);
    std::vector<wchar_t> fileBuf(std::max<size_t>(wdef.size() + 1, static_cast<size_t>(MAX_PATH)), 0);
    if (!wdef.empty())
        wmemcpy(fileBuf.data(), wdef.c_str(), wdef.size());

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileBuf.data();
    ofn.nMaxFile = static_cast<DWORD>(fileBuf.size());
    static const wchar_t kFilter[] =
        L"SPK/XML base (*.spk;*.xml)\0*.spk;*.xml\0"
        L"All (*.*)\0*.*\0\0";
    ofn.lpstrFilter = kFilter;
    ofn.nFilterIndex = 1u;
    ofn.lpstrDefExt = L"spk";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&ofn))
    {
        outPath.clear();
        return false;
    }
    outPath = WideToUtf8(fileBuf.data());
    return !outPath.empty();
}

#else

bool ShowOpenParticleFileDialog(std::string& outPath)
{
    (void)outPath;
    return false;
}

bool ShowOpenImageFileDialog(std::string& outPath)
{
    (void)outPath;
    return false;
}

bool ShowSaveParticleFileDialog(const std::string&, std::string& outPath)
{
    (void)outPath;
    return false;
}

#endif

} // namespace spark_editor
