#ifdef _WIN32

#include "shell.h"
#include <windows.h>

int shell_spawn(const char *path) {
    endwin();
    
    char cmd[MAX_PATH];
    snprintf(cmd, sizeof(cmd), "cmd.exe");
    
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, path, &si, &pi)) {
        refresh();
        return -1;
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    refresh();
    clear();
    return 0;
}

int shell_run_command(const char *cmd, const char *cwd) {
    endwin();
    
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    char cmdline[MAX_PATH * 2];
    snprintf(cmdline, sizeof(cmdline), "cmd.exe /c %s", cmd);
    
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, cwd, &si, &pi)) {
        refresh();
        return -1;
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    refresh();
    clear();
    return (int)exit_code;
}

#endif
