//gcc AudioRecord.c -lpulse-simple -lpulse  -o AudioRecord
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/pulseaudio.h>
//#include <pulse/thread-mainloop.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <errno.h>
#include <unistd.h>

#include "../vdmHelper.h"
#include "pcmmix.h"

#define version_str "1.0.00.3"

#define DUMP 0



static vdm_struct vdm_main;


static int m_index = -1;

static FILE *fp_log = NULL;

void av_log(const char *fmt, ...)
{
	if(!fmt)
		return;
	if(fp_log == NULL)
	{
		char filename[256];
		snprintf(filename,sizeof(filename),"/tmp/AudioRecoder.%d.log",m_index);
		fp_log = fopen(filename,"a");
		if(fp_log == NULL)
		{		 
			printf("AudioRecoder: fopen filename %s fail.errno=%d.\n",filename,errno);		 
			return;	  
		}
	}
	char buffer[1024];
	int buffer_len = -1;
	memset(buffer,0,sizeof(buffer));

	struct timeval tv;
	gettimeofday(&tv, NULL); 
	struct tm *ptm = localtime(&tv.tv_sec);
	snprintf(buffer,sizeof(buffer),"[%4d-%02d-%02d %02d:%02d:%02d:%07.3f] ",(1900+ptm->tm_year),(1+ptm->tm_mon),
		ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec,tv.tv_usec/1000.0);
	
	buffer_len = strlen(buffer);
	
	va_list vl;
    va_start(vl, fmt);
	vsnprintf(buffer+buffer_len,sizeof(buffer) - buffer_len, fmt, vl);
	va_end(vl);

	fputs(buffer,stdout);
	fputs(buffer,fp_log);
	fflush(fp_log);
	return;
}

int main(int argc ,char **argv)
{
	if(argc != 2)
	{
		av_log(":Usage: %s <index> \n",argv[0]);
		return -1;
	}

	m_index = atoi(argv[1]);

	av_log("AudioRecoder start ... \n");
	av_log("AudioRecoder version %s\n",version_str);
	av_log("AudioRecoder index %d\n",m_index);


	int m_samplesSize = 4608;
	int16_t *m_AudioBuff = (int16_t*)calloc(m_samplesSize,sizeof(int16_t));
	if(m_AudioBuff == NULL)
	{
		av_log("AudioRecord : calloc m_AudioBuff failed\n");
	   	return -2;
	}

	int fd_fifo;
	char pipename[32] = {0};
	
	sprintf(pipename, "/tmp/pulse-00%d", m_index); 
	if((mkfifo(pipename, O_CREAT|O_RDWR|0777)) && (errno != EEXIST))  
	{
		av_log("AudioRecord : mkfifo pipe failed: errno=%d\n",errno);
		return -3;
	}  
#if 1
	if((fd_fifo = open(pipename, O_RDWR)) < 0)  
	{  
		av_log("AudioRecord : open pipe failed: errno=%d\n",errno);
		return -4; 
	}
#endif

#if DUMP
	int fd_dump;
	char dump_filename[64];

	sprintf(dump_filename, "/tmp/dump00%d.pcm", m_index);
	if((fd_dump = open(dump_filename, O_RDWR|O_CREAT|O_TRUNC)) < 0)  
	{  
		av_log("AudioRecord : open dump_filename failed: errno=%d\n",errno);
		return -5; 
	}
#endif
	

    vdm_init(&vdm_main,m_index);


    pa_simple* m_pReadStream;
    pa_sample_spec m_paSample;
    
	/*m_paSample = {PA_SAMPLE_S16LE, 48000, 2};*/
	m_paSample.format = PA_SAMPLE_S16LE;
	m_paSample.rate = 48000;
	m_paSample.channels = 2;
	
	int pa_error;
	char monitorname[32];
	int pa_index = m_index % 8;
	
	if (pa_index == 0)
		pa_index = 8;
	sprintf(monitorname,"%d.monitor",pa_index);
	
	/* Create the recording stream */
	m_pReadStream = pa_simple_new(NULL,NULL, PA_STREAM_RECORD, monitorname, "record", &m_paSample, NULL, NULL, &pa_error);
	if (!m_pReadStream) 
	{
	   av_log("AudioRecord : pa_simple_new() failed: %s\n", pa_strerror(pa_error));
	   return -6;
	}

	av_log("AudioRecoder start pa_simple_read... \n");


    char audmixedbuf[4608];

    char audbuf[4608];
    int audlen=sizeof audbuf;
    uint64_t ts;

	while(1)
	{
	    av_log("AudioRecord : pa_simpel_read(4608) \n");
//		memset(m_AudioBuff,0,m_samplesSize*sizeof(int16_t));
	    if (pa_simple_read(m_pReadStream, (uint8_t*)m_AudioBuff,m_samplesSize, &pa_error) < 0) 
		{
	    	av_log("AudioRecord : pa_simple_read() failed: %s\n", pa_strerror(pa_error));
	       	return -7;
	    }
	        av_log("AudioRecord : pa_simpel_read(4608)-0-0 \n");
#if 1
	    if((write(fd_fifo,(uint8_t*)m_AudioBuff,m_samplesSize)) < 0)  
	    {  
	        av_log("AudioRecord : write() failed\n");
	       	return -8;  
	    }
#else

        vdm_check_audiofragment(&vdm_main);
	        av_log("AudioRecord : pa_simpel_read(4608)-1-1 \n");

        videosurf*vs=NULL;
        audiofrag*af=NULL;

        VDM_FOREACH_BEGIN(&vdm_main,vs,af)

            audlen=4608;
            if(_i==0){
//                shmaudio_get(af,audmixedbuf,&audlen,&ts);
                continue;
            }else{
//                shmaudio_get(af,audbuf,&audlen,&ts);
                //mix here
//                pcm_mix_16_ch2((int16_t*)audmixedbuf,(int16_t*)audbuf,audlen);
            }

        VDM_FOREACH_END

        memcpy(m_AudioBuff,audmixedbuf,4608);

	    av_log("AudioRecord : pa_simpel_read(4608)--== \n");
	    write(fd_fifo,(uint8_t*)m_AudioBuff,m_samplesSize);
#endif
		
#if DUMP
		if((write(fd_dump,(uint8_t*)m_AudioBuff,m_samplesSize)) < 0)  
	    {  
	        av_log("AudioRecord : write() fd_dump failed\n");
	       	return -9;  
	    }
#endif

	}

	
	free(m_AudioBuff);
	close(fd_fifo);
#if DUMP
	close(fd_dump);
#endif
	pa_simple_free(m_pReadStream);

	av_log("AudioRecoder end ... \n");
	fclose(fp_log);
	
    vdm_fini(&vdm_main);


	return 0;
	
}

