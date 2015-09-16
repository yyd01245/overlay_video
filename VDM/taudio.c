#include<unistd.h>
#include<pwd.h>

#include<sys/types.h>
#include<sys/wait.h>

#include<stdio.h>
#include<stdlib.h>

//calculate sink number`{1..8}` & userid`X{00..}` from index
static void index1(int index,int*osink,int*ouid,int pa_amount)
{

    int ret;
    int s;

    *ouid=(index-1)/pa_amount;
    s=index%pa_amount;
    if(s==0)
        s=pa_amount;
        
    *osink=s;

}



static int switch_user_by_index(int index)
{
    int ret=-1;
    char unam[16];

    int sink,uid;
    index1(index,&sink,&uid,8);

    snprintf(unam,sizeof(unam),"x%.2d",uid);

    struct passwd*urec=getpwnam(unam);
    if(!urec)
        ret=-1; 
    else{
        ret=setuid(urec->pw_uid);
        if(ret<0)
            ret=-2;    
        else
            ret=setenv("HOME",urec->pw_dir,1);
 
    }

    return ret;

}


int main(int argc,char**argv)
{
    int index=atoi(argv[1]);

    int sink;
    int uid;

//    index1(index,&sink,&uid,8);

    int pid;
    char index_str[12];
    snprintf(index_str,sizeof index_str,"%d",index);

    pid=fork();
    if(pid==0){
        
        switch_user_by_index(index);

        execl("./audioplayer","audioplayer",index_str,NULL);

    }else{


        waitpid(pid,NULL,0);


    }


    return 0;

}

