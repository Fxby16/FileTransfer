#include <application.h>

int main(){
    char option;

    do{
        printf("Client(c) or Server(s)? ");
        scanf("%c",&option);
    }while(option!='c' && option!='s');
    
    if(option=='c')
        RunClient();
    else
        RunServer();

    return 0;
}