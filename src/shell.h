#ifndef SHELL_H
#define SHELL_H

int shell_spawn(const char *path);
int shell_run_command(const char *cmd, const char *cwd);

#endif
