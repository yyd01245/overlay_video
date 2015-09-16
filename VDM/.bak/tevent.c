#include<stdio.h>
#include<stdlib.h>
#include"event2/event.h"

#include<signal.h>
#include<unistd.h>
#include<stdarg.h>
#include<string.h>

void pr(const char*format,...){

    va_list ap;

    char buff[1024];

    va_start(ap,format);

    vsnprintf(buff,sizeof buff,format,ap);

    va_end(ap);

    write(STDERR_FILENO,buff,strlen(buff));

}


void do_quit(evutil_socket_t fd, short what, void *arg)
{

    pr("quit\n");

}





void do_int(evutil_socket_t fd, short what, void *arg)
{

    int i=10;
    while(i){
    
        pr("int\n");
        i--;
        usleep(500000);
    }

}


int main(int argc,char**argv)
{




    struct event_base*base;
    struct event*se0,*se1;

    
    base=event_base_new();

    se0=evsignal_new(base,SIGQUIT,do_quit,NULL);
    se1=evsignal_new(base,SIGINT,do_int,NULL);

    evsignal_add(se0,NULL);
    evsignal_add(se1,NULL);



    event_base_dispatch(base);








}
