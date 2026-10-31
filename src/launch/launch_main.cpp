// hotd_launch (user 2026-10-31: "build their ReWrithe exe in that same folder like an actual game"): a tiny program
// with no DLLs of its own. The setup copies it next to the package's bin\ as "The House of the Dead ReWrithe.exe" and
// gives it the icon of the player's THOTD.EXE (util/game_exe.h); it starts bin\hotd.exe with the same arguments.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    std::wstring path(32768, L'\0');
    const DWORD n = GetModuleFileNameW(nullptr, path.data(), DWORD(path.size()));
    path.resize(n);
    const std::wstring dir = path.substr(0, path.find_last_of(L"\\/"));
    const std::wstring exe = dir + L"\\bin\\hotd.exe";
    // the arguments after the program's own name
    const wchar_t* cl = GetCommandLineW();
    if (*cl == L'"') {
        ++cl;
        while (*cl && *cl != L'"') ++cl;
        if (*cl) ++cl;
    } else {
        while (*cl && *cl != L' ' && *cl != L'\t') ++cl;
    }
    std::wstring cmd = L"\"" + exe + L"\"" + cl;
    STARTUPINFOW si{};
    si.cb = sizeof si;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exe.c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, dir.c_str(), &si, &pi)) {
        MessageBoxW(nullptr, L"bin\\hotd.exe was not found next to this program. Unzip the whole package again.",
                    L"The House of the Dead: ReWrithe", MB_ICONERROR);
        return 1;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
