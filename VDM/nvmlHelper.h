#ifndef _NVML_HELPER_H_
#define _NVML_HELPER_H_
#include<nvml.h>


static int check_nvml_error(nvmlReturn_t nvret,const char*msg,int line)
{

    if(nvret!=NVML_SUCCESS){
        fprintf(stderr,"NVML:@%d::%s:{%s}\n",line,msg,nvmlErrorString(nvret));
        return -1;
    }
    return 0;

}

#define CHK_NVML(ret,message) \
    if(check_nvml_error(ret,message,__LINE__)<0)return -1;



typedef struct _stats{
   
    nvmlDevice_t handler;
    nvmlUtilization_t utils;
    nvmlMemory_t meminfo;

    unsigned int encutil;
    unsigned int decutil;


}devstat;


/*
int get_gpu_core_num(void){

    int n_dev;
    nvmlReturn_t nvret;


    nvret=nvmlInit();
    CHK_NVML(nvret,"Init NVML");


    nvret=nvmlDeviceGetCount(&n_dev);
    CHK_NVML(nvret,"getCount");

    nvret=nvmlShutdown();
    CHK_NVML(nvret,"Shutdown NVML");


    return n_dev;

}
*/

static int probe_gpustats(devstat**stats)
{

    unsigned int n_dev;
    nvmlReturn_t nvret;


    nvret=nvmlInit();
    CHK_NVML(nvret,"Init NVML");


    nvret=nvmlDeviceGetCount(&n_dev);
    CHK_NVML(nvret,"getCount");


    *stats=(devstat*)calloc(n_dev,sizeof(devstat));
    devstat*pstats=*stats;


    int i;
    for(i=0;i<n_dev;i++)
        nvmlDeviceGetHandleByIndex(i,&pstats[i].handler);

    
    for(i=0;i<n_dev;i++)
        nvmlDeviceGetMemoryInfo(pstats[i].handler,&pstats[i].meminfo);
    
    for(i=0;i<n_dev;i++)
        nvmlDeviceGetUtilizationRates(pstats[i].handler,&pstats[i].utils);

    unsigned int sampp;
    for(i=0;i<n_dev;i++)
        nvmlDeviceGetEncoderUtilization(pstats[i].handler,&pstats[i].encutil,&sampp);

    for(i=0;i<n_dev;i++)
        nvmlDeviceGetDecoderUtilization(pstats[i].handler,&pstats[i].decutil,&sampp);


    nvret=nvmlShutdown();
    CHK_NVML(nvret,"Shutdown NVML");


    return n_dev;
}


static int get_core_by_gpu(devstat*ps,int n_dev)
{
    int mingpu=0;
    long minuse=0;
    int i;
    for(i=0;i<n_dev;i++){
        long use=ps[i].utils.gpu; 
        if(use<minuse){
            minuse=use;
            mingpu=i;
        }
    }
  
    return mingpu;

}


static int get_core_by_encoder(devstat*ps,int n_dev)
{
    int minenc=0;
    long minuse=100000;
    int i;
    for(i=0;i<n_dev;i++){
        long use=ps[i].encutil; 
        if(use<minuse){
            minuse=use;
            minenc=i;
        }
    }
    return minenc;
}




static int get_core_by_decoder(devstat*ps,int n_dev)
{
    int mindec=0;
    long minuse=100000;
    int i;
    for(i=0;i<n_dev;i++){
        long use=ps[i].decutil; 
        if(use<minuse){
            minuse=use;
            mindec=i;
        }
    }
    return mindec;
}




static int get_core_by_mem(devstat*ps,int n_dev)
{
    int maxfreeind=0;
    long long maxfree=0;
    int i;
    for(i=0;i<n_dev;i++){
        long long free=ps[i].meminfo.free; 
        if(free>maxfree){
            maxfree=free;
            maxfreeind=i;
        }
    }
  
    return maxfreeind;

}


static int get_next_core(void)
{
    
    devstat*ds;
    int n;
    int idx;
    n=probe_gpustats(&ds);
    if(n<0)
        return -1;
#if 1
    idx=get_core_by_decoder(ds,n);  
#else
    idx=get_core_by_mem(ds,n);  
#endif
    free(ds);

    return idx;
}



#endif
