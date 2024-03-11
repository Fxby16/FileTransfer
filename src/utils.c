#include <dirent.h>
#include <string.h>
#include <ncurses.h>
#include <menu.h>
#include <stdlib.h>

#include <globals.h>
#include <ctype.h>

void SortEntries(struct dirent *entries,int num_entries){
    for(int i=0;i<num_entries-1;i++){
        for(int j=0;j<num_entries-i-1;j++){
            if(entries[j+1].d_type==DT_DIR && entries[j].d_type!=DT_DIR){
                struct dirent temp=entries[j];
                entries[j]=entries[j+1];
                entries[j+1]=temp;
            }else if(entries[j].d_type==entries[j+1].d_type){
                if(strcmp(entries[j].d_name,entries[j+1].d_name)>0){
                    struct dirent temp=entries[j];
                    entries[j]=entries[j+1];
                    entries[j+1]=temp;
                }
            }
        }
    }
}

void HandleInputs(WINDOW *win,MENU *menu){
    int ch=wgetch(win);
    flushinp();
    switch(ch){
        case 'h':
            prev_hidden=show_hidden;
            show_hidden=!show_hidden;
            break;
        case 'q':
            quit=true;
            break;
        case '\n':
            scan_dir=true;
            break;
        case KEY_DOWN:
            menu_driver(menu,REQ_DOWN_ITEM);
            break;
        case KEY_UP:
            menu_driver(menu,REQ_UP_ITEM);
            break;
        case KEY_LEFT:
            back=true;
            scan_dir=true;
            break;
        case 's':
            send_menu=true;
            break;
        default:
            break;
    }
}

bool CheckIp(char *ip_address){
    int num_periods=0;

    for(int i=0;i<strlen(ip_address);i++){
        if(!isdigit(ip_address[i]) && ip_address[i]!='.'){
            return false;
        }

        if(ip_address[i]=='.'){
            num_periods++;
        }
    }

    if(num_periods!=3){
        return false;
    }else{
        return true;
    }
}

uint32_t GetIp(char *ip_address){
    uint32_t ip=0;
    int shift=24;
    char *token=strtok(ip_address,".");

    while(token!=NULL){
        ip+=atoi(token)<<shift;
        shift-=8;
        token=strtok(NULL,".");
    }

    return ip;
}