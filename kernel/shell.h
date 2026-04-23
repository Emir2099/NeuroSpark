#ifndef SHELL_H
#define SHELL_H

#define SHELL_CMD_MAX 96

void scroll_shell(void);
void list_files(void);
void list_files_gfx(void);
void process_command(char *cmd);
int shell_is_recording_replay(void);

#endif
