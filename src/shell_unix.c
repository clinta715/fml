#ifndef _WIN32

#include "shell.h"
#include <ncurses.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int shell_spawn(const char *path) {
    endwin();
    
    pid_t pid = fork();
    if (pid < 0) {
        refresh();
        return -1;
    }
    
    if (pid == 0) {
        chdir(path);
        char *shell = getenv("SHELL");
        if (!shell) shell = "/bin/sh";
        execl(shell, shell, "-i", NULL);
        _exit(127);
    }
    
    int status;
    waitpid(pid, &status, 0);
    
    refresh();
    clear();
    return 0;
}

int shell_run_command(const char *cmd, const char *cwd) {
    (void)cwd;
    endwin();
    
    pid_t pid = fork();
    if (pid < 0) {
        refresh();
        return -1;
    }
    
    if (pid == 0) {
        if (cwd) chdir(cwd);
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127);
    }
    
    int status;
    waitpid(pid, &status, 0);
    
    refresh();
    clear();
    return WEXITSTATUS(status);
}

#endif
