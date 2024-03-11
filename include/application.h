#ifndef APPLICATION_H
#define APPLICATION_H

#include <ncurses.h>
#include <menu.h>
#include <dirent.h>

extern void RunClient();
extern void RunServer();
extern void RecreateHiddenMenu(ITEM **items,struct dirent *entries,int *num_entries,int *num_shown_entries);
extern void RecreateMenu(ITEM **items,struct dirent *entries,int *num_entries,int *num_shown_entries);
extern MENU *CreateMenu(ITEM **items,WINDOW *win);
extern void SendMenu(char *filepath,WINDOW *win);

#endif