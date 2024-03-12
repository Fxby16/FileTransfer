#include <ncurses.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <menu.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <errno.h>

#include <utils.h>
#include <globals.h>
#include <application.h>

#define PATH_MAX_SIZE 256

/**
 * Reallocate the items in the array and fill it with the entries that are not hidden
 * \param items an array of pointers to items. the old pointers must already be freed
 * \param entries an array containing the entries of the current directory
 * \param num_entries the number of entries in the current directory
 * \param num_shown_entries will be filled with the number of entries that are not hidden
*/
void RecreateMenu(ITEM **items,struct dirent *entries,int *num_entries,int *num_shown_entries){
    FILE *fp=fopen("debug.txt","w");
    
    int j=0;
    for(int i=0;i<*num_entries;i++){
        if(entries[i].d_name[0]!='.'){
            items[j]=new_item(entries[i].d_name,"");
            if(items[j]!=NULL){
                j++;
            }
        }
    }
    items[j]=NULL;
    *num_shown_entries=j;

    fclose(fp);
}

/**
 * Reallocate the items in the array and fill it with all the entries in the current directory
 * \param items an array of pointers to items. the old pointers must already be freed
 * \param entries an array containing the entries of the current directory
 * \param num_entries the number of entries in the current directory
 * \param num_shown_entries will be filled with the number of entries in the current directory
*/
void RecreateHiddenMenu(ITEM **items,struct dirent *entries,int *num_entries,int *num_shown_entries){
    int j=0;
    for(int i=0;i<*num_entries;i++){
        items[i]=new_item(entries[i].d_name,"");
        if(items[i]!=NULL){
            j++;
        }
    }
    items[*num_entries]=NULL;
    *num_shown_entries=*num_entries;
}

/**
 * Create a menu with the given items and window
*/
MENU *CreateMenu(ITEM **items,WINDOW *win){
    MENU *menu=new_menu(items);
    set_menu_win(menu,win);
    set_menu_sub(menu,derwin(win,LINES-4,COLS-4,2,2));
    set_menu_format(menu,LINES-5,1);
    set_menu_mark(menu,"");
    post_menu(menu);

    return menu;
}

void SendMenu(char *filepath,WINDOW *win){
    wclear(win);

    curs_set(1);
    nodelay(win,false);

    send_menu=false;

    char ip_address[16];
    char port[6];

    mvwprintw(win,0,0,"Enter the IP address of the receiver:");
    wrefresh(win);
    do{
        for(int i=0;i<COLS-2;i++){
            mvwprintw(win,1,i," ");
        }
        mvwgetnstr(win,1,0,ip_address,15);
    }while(!CheckIp(ip_address));

    uint32_t ip=GetIp(ip_address);

    mvwprintw(win,2,0,"Enter the port of the receiver:");
    wrefresh(win);
    do{
        for(int i=0;i<COLS-2;i++){
            mvwprintw(win,3,i," ");
        }
        mvwgetnstr(win,3,0,port,5);
    }while(atoi(port)==0);

    int port_num=atoi(port);

    int sockfd=socket(PF_INET,SOCK_STREAM,0);

    struct sockaddr_in dest;
    dest.sin_family=AF_INET;
    dest.sin_port=htons(port_num);
    dest.sin_addr.s_addr=htonl(ip);

    struct sockaddr_in serv;
    serv.sin_family=AF_INET;
    serv.sin_port=INADDR_ANY;
    serv.sin_addr.s_addr=INADDR_ANY;

    if(bind(sockfd,(struct sockaddr *)&serv,sizeof(struct sockaddr))==-1){
        perror("Unable to bind to the socket");
        return;
    }
    
    if(connect(sockfd,(struct sockaddr *)&dest,sizeof(struct sockaddr))==-1){
        perror("Unable to connect to the receiver");
        close(sockfd);
        return;
    }

    char buffer[512];
    char filename[PATH_MAX_SIZE];

    int start_index;
    for(start_index=strlen(filepath)-1;filepath[start_index]!='/';start_index--);

    start_index++;

    memset(filename,'\0',PATH_MAX_SIZE);
    for(int i=start_index;i<strlen(filepath);i++){
        filename[i-start_index]=filepath[i];
    }

    if(send(sockfd,filename,strlen(filename),0)==-1){
        perror("Unable to send file name");
        close(sockfd);
        return;
    }

    if(recv(sockfd,buffer,512,0)==-1){
        perror("Unable to receive file name confirmation");
        close(sockfd);
        return;
    }else if(strcmp(buffer,"REJ")==0){
        wprintw(win,"Receiver rejected the file. Press any key to continue.\n");
        wrefresh(win);
        wgetch(win);
        close(sockfd);
        return;
    }

    int file=open(filepath,O_RDONLY);

    if(file==-1){
        perror("Unable to open file");
        close(sockfd);
        return;
    }

    int bytes_read;
    while((bytes_read=read(file,buffer,512))>0){
        if(send(sockfd,buffer,bytes_read,0)==-1){
            perror("Unable to send file");
            close(file);
            close(sockfd);
            return;
        }
    }

    close(file);
    close(sockfd);
}

void RunClient(){
    initscr();
    curs_set(0);

    //window for the border
    WINDOW *win=newwin(LINES,COLS,0,0);
    box(win,0,0);
    wrefresh(win);
    delwin(win);

    //window for the menu
    win=newwin(LINES-2,COLS-2,1,1);
    keypad(win,true);
    nodelay(win,true);

    char current_path[PATH_MAX_SIZE]; 
    memset(current_path,'\0',PATH_MAX_SIZE);

    DIR *dir=opendir(".");
    if(dir==NULL) {
        endwin();
        perror("Unable to open directory");
        return;
    }

    struct dirent *entries=NULL;
    int entries_size=0;
    int num_entries=0;

    struct dirent *entry;
    while((entry=readdir(dir))!=NULL){
        num_entries++;
        entries_size++;
        entries=realloc(entries,num_entries*sizeof(struct dirent));

        if(entries==NULL){
            endwin();
            perror("Unable to allocate memory");
            return;
        }

        memcpy(&entries[num_entries-1],entry,sizeof(struct dirent));
    }

    SortEntries(entries,num_entries);

    char *cwd=getcwd(NULL,0);
    strcpy(current_path,cwd);
    free(cwd);

    int num_shown_entries=0;

    ITEM **items=(ITEM **)calloc(num_entries+1,sizeof(ITEM *));

    if(show_hidden){
        RecreateHiddenMenu(items,entries,&num_entries,&num_shown_entries);
    }else{
        RecreateMenu(items,entries,&num_entries,&num_shown_entries);
    }

    MENU *menu=CreateMenu(items,win);
    wrefresh(win);

    while(!quit){
        HandleInputs(win,menu);

        if(send_menu){
            send_menu=false;
            struct dirent *selected_entry;
            for(int i=0;i<num_entries;i++){
                if(strcmp(item_name(current_item(menu)),entries[i].d_name)==0){
                    selected_entry=&entries[i];
                    break;
                }
            }

            if(selected_entry->d_type!=DT_DIR){
                char *filepath=malloc(strlen(current_path)+strlen(selected_entry->d_name)+2);
                strcpy(filepath,current_path);
                strcat(filepath,"/");
                strcat(filepath,selected_entry->d_name);
                SendMenu(filepath,win);
                free(filepath);

                nodelay(win,true);
                curs_set(0);
            }

            unpost_menu(menu);
            wclear(win);
            wrefresh(win);
            post_menu(menu);
            wrefresh(win);
        }else{
            if(scan_dir){ //directory was changed and the menu needs to be updated
                struct dirent *selected_entry=NULL;
                if(!back){
                    for(int i=0;i<num_entries;i++){
                        if(item_name(current_item(menu))!=NULL && strcmp(item_name(current_item(menu)),entries[i].d_name)==0){
                            selected_entry=&entries[i];
                            break;
                        }
                    }
                }

                if(selected_entry!=NULL && selected_entry->d_type!=DT_DIR){
                    scan_dir=false;
                }else if(back){ //go back to the previous directory
                    back=false;

                    char *last_slash=strrchr(current_path,'/');
                    if(last_slash!=current_path){
                        *last_slash='\0';
                    }

                    num_entries=0;
                    closedir(dir);
                    dir=opendir(current_path);

                    while((entry=readdir(dir))!=NULL){
                        num_entries++;
                        if(num_entries>entries_size){
                            entries=realloc(entries,num_entries*sizeof(struct dirent));
                            entries_size++;
                        }

                        if(entries==NULL){
                            endwin();
                            perror("Unable to allocate memory");
                            return;
                        }

                        memcpy(&entries[num_entries-1],entry,sizeof(struct dirent));
                    }

                    SortEntries(entries,num_entries);
                }else{ //go to the selected directory
                    if(strlen(current_path)+strlen(item_name(current_item(menu))+1)>PATH_MAX_SIZE){
                        endwin();
                        perror("Path too long");
                        return;   
                    }

                    if(strcmp(item_name(current_item(menu)),".")!=0){
                        if(strcmp(item_name(current_item(menu)),"..")==0){
                            char *last_slash=strrchr(current_path,'/');
                            if(last_slash!=current_path){
                                *last_slash='\0';
                            }
                        }else{
                            strcat(current_path,"/");
                            strcat(current_path,item_name(current_item(menu)));
                        }

                        num_entries=0;
                        closedir(dir);
                        if((dir=opendir(current_path))==NULL){
                            endwin();
                            perror("Unable to open directory");
                            printf("%s\n",current_path);
                            printf("Selected entry: %s\n",selected_entry->d_name);
                            return;
                        }

                        while((entry=readdir(dir))!=NULL){
                            num_entries++;
                            if(num_entries>entries_size){
                                entries=realloc(entries,num_entries*sizeof(struct dirent));
                                entries_size++;
                            }

                            if(entries==NULL){
                                endwin();
                                perror("Unable to allocate memory");
                                return;
                            }

                            memcpy(&entries[num_entries-1],entry,sizeof(struct dirent));
                        }

                        SortEntries(entries,num_entries);
                    }
                }
            }

            if(prev_hidden!=show_hidden || scan_dir){
                unpost_menu(menu);
                free_menu(menu);
                for(int i=0;i<num_shown_entries;i++){
                    free_item(items[i]);
                }

                if((items=(ITEM **)realloc(items,(num_entries+1)*sizeof(ITEM *)))==NULL){
                    endwin();
                    perror("Unable to allocate memory");
                    return;
                }

                if(show_hidden){
                    RecreateHiddenMenu(items,entries,&num_entries,&num_shown_entries);
                }else{
                    RecreateMenu(items,entries,&num_entries,&num_shown_entries);
                }

                if(scan_dir)
                    scan_dir=false;

                prev_hidden=show_hidden;
                menu=CreateMenu(items,win);
            }

            for(int i=0;i<COLS-2;i++){
                mvwprintw(win,0,i," ");
            }
            mvwprintw(win,0,0,"Entries in %s:",current_path);

            wrefresh(win);
        }
        usleep(64000);
    }

    unpost_menu(menu);
    for(int i=0;i<num_shown_entries;i++){
        free_item(items[i]);
    }
    free_menu(menu);
    free(items);

    closedir(dir);

    delwin(win);

    endwin();
}

void RunServer(){
    initscr();
    curs_set(1);

    //window for the border
    WINDOW *win=newwin(LINES,COLS,0,0);
    box(win,0,0);
    wrefresh(win);
    delwin(win);

    //window for the menu
    win=newwin(LINES-2,COLS-2,1,1);
    keypad(win,true);
    nodelay(win,false);

    char filename[PATH_MAX_SIZE];

    int sockfd=socket(PF_INET,SOCK_STREAM,0);

    struct sockaddr_in serv;
    serv.sin_family=AF_INET;
    serv.sin_port=INADDR_ANY;
    serv.sin_addr.s_addr=INADDR_ANY;

    if(bind(sockfd,(struct sockaddr *)&serv,sizeof(struct sockaddr))==-1){
        endwin();
        perror("Unable to bind to the socket");
        return;
    }

    if(listen(sockfd,1)==-1){
        endwin();
        perror("Unable to listen to the socket");
        return;
    }

    struct sockaddr_in addr;
    socklen_t addr_len=sizeof(addr);
    getsockname(sockfd,(struct sockaddr *)&addr,&addr_len);
    int port=ntohs(addr.sin_port);

    struct sockaddr_in client;
    int client_size=sizeof(struct sockaddr_in);;
    int client_sock;
    int bytes_read;
    char option;
    int file;
    int curx,cury;
    char path[PATH_MAX_SIZE];
    char buffer[512];
    
    struct ifaddrs *addrs,*temp;
    getifaddrs(&addrs);

    while(!quit){
        wprintw(win,"Server interfaces: \n");

        temp=addrs;
        while(temp){
            if(temp->ifa_addr && temp->ifa_addr->sa_family==AF_INET){
                struct sockaddr_in *pAddr=(struct sockaddr_in*)temp->ifa_addr;
                wprintw(win,"%s: %s\n",temp->ifa_name,inet_ntoa(pAddr->sin_addr));
            }
            temp=temp->ifa_next;
        }
        wprintw(win,"Server is listening on port %d\n",port);
        wrefresh(win);
        
        client_sock=accept(sockfd,(struct sockaddr *)&client,(socklen_t*)&client_size);

        if(client_sock==-1){
            endwin();
            perror("Unable to accept connection");
            return;
        }

        if((bytes_read=recv(client_sock,filename,PATH_MAX_SIZE,0))==-1){
            endwin();
            perror("Unable to receive file name");
            return;
        }else{  
            filename[bytes_read]='\0';

            wprintw(win,"Client wants to send %s. Accept(a) or reject(r)?\n",filename);
            wrefresh(win);

            do{
                option=wgetch(win);
                while(wgetch(win)!='\n');
            }while(option!='a' && option!='r');

            if(option=='r'){
                if(send(client_sock,"REJ",3,0)==-1){
                    endwin();
                    perror("Unable to send file name confirmation");
                    return;
                }
            }else{
                if(send(client_sock,"ACC",3,0)==-1){
                    endwin();
                    perror("Unable to send file name confirmation");
                    return;
                }

                wprintw(win,"Path to save the file: ");
                wrefresh(win);
                
                file=0;

                do{
                    if(file<0){
                        wprintw(win,"Error: %s\n",strerror(errno));
                    }
                    getyx(win,cury,curx);
                    for(int i=curx;i<COLS-2;i++){
                        mvwprintw(win,cury,i," ");
                    }
                    mvwgetnstr(win,cury,curx,path,PATH_MAX_SIZE);
                }while((file=open(path,O_WRONLY | O_CREAT | O_TRUNC,0666))<0);

                while((bytes_read=recv(client_sock,buffer,512,0))>0){
                    if(write(file,buffer,bytes_read)==-1){
                        endwin();
                        perror("Unable to write to file");
                        return;
                    }
                }

                close(file);

                wprintw(win,"File received correctly. Press a key to wait for another connection.");
                wgetch(win);
                wclear(win);
            }
        }
    }

    freeifaddrs(addrs);

    delwin(win);
    endwin();
}