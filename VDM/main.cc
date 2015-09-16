#include"VDManager.h"
#include"sysutils.h"
#include<stdio.h>

int main(int argc,char**argv)
{


//    become_daemon();

    VDManager man("0.0.0.0",9898);

    man.start();

    fprintf(stderr,"VDM Exited....\n");
    return 0;

}
