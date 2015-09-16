#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<nvml.h>


static void check_nvml_error(nvmlReturn_t nvret,const char*msg,int line)
{

    if(nvret!=NVML_SUCCESS){

        fprintf(stderr,"NVML:@%d::%s:{%s}\n",line,msg,nvmlErrorString(nvret));
        exit(1);
    }

}

#define CHK_NVML(ret,message) \
    check_nvml_error(ret,message,__LINE__)



typedef struct _stats{
   
    nvmlDevice_t handler;
    nvmlMemory_t meminfo;
    nvmlUtilization_t utils;

    unsigned int encutil;
    unsigned int decutil;


}devstat;


int bytes_to(unsigned long long val,int u,char*ounit)
{
    static const char*unit[]={"Byte","KiB","MiB","GiB"};
    
    if(val<9999){
        strcpy(ounit,unit[u]);
        return val;
    }else{
        bytes_to(val/1024,u+1,ounit);
    }

}

int conv_bytes(unsigned long long val,char*unit)
{
    int v;
    int u=0;
    v=bytes_to(val,0,unit);
    return v;
}




void print_devstats(devstat*s)
{


    printf("==========\n");
    
    printf("Meminfo::\ttotal:%llu\tused:%llu\tfree:%llu\n",s->meminfo.total,s->meminfo.used,s->meminfo.free);



    printf("Utils  ::\tgpu:%u\tmemory:%u\n",s->utils.gpu,s->utils.memory);

    printf("Encodec::\tencoder:%u\tdecoder:%u\n",s->encutil,s->decutil);
    printf("----------\n");
}


int probe_gpustats(devstat**stats)
{

    int n_dev;
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
#if 0
    int maxfreeind=0;
    int maxfree=0;
    for(i=0;i<n_dev;i++){

        print_devstats(&pstats[i]);

        int free=pstats[i].meminfo.free; 
//        fprintf(stderr,"<%d\n",free);
        if(free>maxfree){
            maxfree=free;
            maxfreeind=i;
        }

    }
#endif
    nvret=nvmlShutdown();
    CHK_NVML(nvret,"Shutdown NVML");


    return n_dev;
}



int get_core_by_mem(devstat*ps,int n_dev)
{
    int maxfreeind=0;
    int maxfree=0;
    int i;
    for(i=0;i<n_dev;i++){

        print_devstats(&ps[i]);

        int free=ps[i].meminfo.free; 
//        fprintf(stderr,"<%d\n",free);
        if(free>maxfree){
            maxfree=free;
            maxfreeind=i;
        }

    }
  
    return maxfreeind;

}


int get_next_core(void)
{
    
    devstat*ds;
    int n;
    n=probe_gpustats(&ds);

    n=get_core_by_mem(ds,n);  

    free(ds);

    return n;
}


int main(int argc,char**argv)
{
    
    int n=get_next_core();
    
    fprintf(stdout,"Get %d\n",n);


    return 0;

}

