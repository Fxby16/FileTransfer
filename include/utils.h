#ifndef UTILS_H
#define UTILS_H

#include <ncurses.h>
#include <menu.h>
#include <stdbool.h>
#include <stdint.h>

extern void SortEntries(struct dirent *entries,int num_entries);
extern void HandleInputs(WINDOW *win,MENU *menu);
extern bool CheckIp(char *ip_address);
extern uint32_t GetIp(char *ip_address);
extern long GetFileSize(const char *filename);
extern char *GetSizeAsStr(long size);

#endif