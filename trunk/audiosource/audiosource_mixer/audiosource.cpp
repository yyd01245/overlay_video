#include "audiosource.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/time.h>

#include <queue>
#include <pthread.h>
#include <stdarg.h>
#include"vdmHelper.h"
#include"avstub.h"
#include"avshm.h"

#include<deque>
#include <math.h>

using namespace std;

typedef short int int16_t; 

typedef struct
{		
	int16_t *m_AudioBuff;		
	int m_samplesSize;	
}m_Audio;


typedef struct _AudioData {
	unsigned char *data;
	long size;
	unsigned long pts;
} AudioData;


typedef struct
{
	int index;
	int m_samplesSize;
	int fd;
	bool flag_stop;
	pthread_mutex_t mutex;
	queue<m_Audio> *m_AudioQueue;
	int queue_max_size;
	uint8_t* m_AudioBuff;
	uint8_t* m_dstAudioBuff;
	

}audiosource_instanse;


#define AUDFRAG_SIZ 4608
static char buffer[AUDFRAG_SIZ];
static vdm_struct vdm_main;

const int Audio_Buff_size = 4608;

std::deque<AudioData*> m_audioUsedQueue;
std::deque<AudioData*> m_audioFreeQueue;
pthread_mutex_t m_audioLocker;
const int AudioQueSize = 30;


bool initQueue(int isize)
{
	//“Ù∆µ∂‡µ„ª∫¥Ê
	for(int i=0;i<isize;++i)
	{
		AudioData* audiobuff = new AudioData;
		audiobuff->data = new unsigned char[Audio_Buff_size];
		m_audioFreeQueue.push_back(audiobuff);

	}
	pthread_mutex_init(&m_audioLocker,NULL);

	return true;
}

void Push_Audio_Data(unsigned char *sample,int isize,unsigned long ulPTS)
{

	pthread_mutex_lock(&m_audioLocker);
	if(m_audioFreeQueue.size() > 0)
	{
		AudioData* audiobuff = m_audioFreeQueue.front();
		audiobuff->pts = ulPTS;
		audiobuff->size = isize;

		unsigned char *tmpbuf = audiobuff->data;
		memcpy(tmpbuf,sample,isize);

		m_audioFreeQueue.pop_front();
		m_audioUsedQueue.push_back(audiobuff);
		//printf("-----audio m_audioFreeQueue size =%d ,get m_audioUsedQueue.size =%ld  \n",m_audioFreeQueue.size(),m_audioUsedQueue.size());
	}
	else if(m_audioUsedQueue.size() > 0)
	{
		AudioData* audiobuff = m_audioUsedQueue.front();
		audiobuff->pts = ulPTS;
		audiobuff->size = isize;

		unsigned char* tmpbuf = audiobuff->data;
		memcpy(tmpbuf,sample,isize);

		m_audioUsedQueue.pop_front();
		m_audioUsedQueue.push_back(audiobuff);
		printf("-----audio m_audioFreeQueue size =%d ,get m_audioUsedQueue.size =%ld  \n",m_audioFreeQueue.size(),m_audioUsedQueue.size());
	}
	
	pthread_mutex_unlock(&m_audioLocker);
}

int get_Audio_data(unsigned char *output_audio_data,int* input_audio_size,unsigned long* audio_pts)
{
	int iret = 0;
	pthread_mutex_lock(&m_audioLocker);
	if(m_audioUsedQueue.size() >0)
	{

		AudioData* tempbuff = m_audioUsedQueue.front();
		
		unsigned char *buff = tempbuff->data;
	
		if(audio_pts)
			*audio_pts= tempbuff->pts;
		
		if(*input_audio_size < tempbuff->size)
		{
			printf("input_audio_size is too small \n");
			pthread_mutex_unlock(&m_audioLocker);
			return 0;
		}
		*input_audio_size= tempbuff->size;
		iret = tempbuff->size;
		memcpy(output_audio_data,buff,tempbuff->size);
		tempbuff->size = 0;
		
		
		m_audioUsedQueue.pop_front();
		m_audioFreeQueue.push_back(tempbuff);
	
		pthread_mutex_unlock(&m_audioLocker);
				struct timeval tm;

		gettimeofday(&tm,NULL);
		//printf("-----get audio que size =%d  \n",m_audioUsedQueue.size());
	}
	else
	{
		printf("-----cannot get audio que size =%d  \n",m_audioUsedQueue.size());
		pthread_mutex_unlock(&m_audioLocker);
		return 0;
	}
	return iret;
}




#define PA_LIKELY(x) (__builtin_expect(!!(x),1))
#define PA_UNLIKELY(x) (__builtin_expect(!!(x),0))


#define PA_CLAMP_UNLIKELY(x, low, high)                                 \
    __extension__ ({                                                    \
            typeof(x) _x = (x);                                         \
            typeof(low) _low = (low);                                   \
            typeof(high) _high = (high);                                \
            (PA_UNLIKELY(_x > _high) ? _high : (PA_UNLIKELY(_x < _low) ? _low : _x)); \
        })


short  remix(short buffer1,short buffer2)  
{  
#if 0
    int value = (buffer1 + buffer2);  
	PA_CLAMP_UNLIKELY(value , -0x8000, 0x7FFF);
   // return (short)(value/2);  
   
   return value;
#endif
	short value =0;
	if( buffer1 < 0 && buffer2 < 0)  
    value = buffer1+buffer2 - (buffer1 * buffer2 / -(pow(2,16-1)-1));  
else  
    value = buffer1+buffer2 - (buffer1 * buffer2 / (pow(2,16-1)-1));  
	return value;
}


int operation_audio_pcm(unsigned char *pSrcAudio1,int iSrcLen1,unsigned char *pSrcAudio2,int iSrclen2,
			unsigned char *pDstAudio,int *pDstLen)
{
	//srclen1 == srclen2
	//16s 2channel
	
	int iIndex = 0;
	short* pSrc1 = (short*)pSrcAudio1;
	short* pSrc2 = (short*)pSrcAudio2;
	short* pDst = (short*)pDstAudio;
	while(iIndex < iSrcLen1/2)
	{
		short sSrc1 = *(short*)(pSrc1+iIndex);
		short sSrc2 = *(short*)(pSrc2+iIndex);
		*pDst = remix(sSrc1,sSrc2);
		pDst++;
		iIndex++;
	}
	*pDstLen = iSrcLen1;
	return 0;
}


static void*audio_fetch(void *param)
{

#if 0
    int wfd=open("/home/x00/yyd/trunk/audiosource/audiosource_mixer/cap.pcm",O_CREAT|O_TRUNC|O_WRONLY,0666);
    if(wfd<0){
        fprintf(stderr,"open cap.pcm{%s}\n",strerror(errno));
    }

#endif
	//audiosource_instanse *p_instanse = (audiosource_instanse *)param;

    int nr,nw;
    int ret;
    uint64_t ts=0,lastts=0;

    while(1){


        ret=vdm_check_audiofragment(&vdm_main);
        if(ret==0){
            fprintf(stderr,"No Audio data Ready..\n");
            usleep(20000);
            continue;
        }
        audiofrag*af=NULL;
        videosurf*vs=NULL;

        if(ret>0){
    		
            //FIXME only one way audio available..
            VDM_FOREACH_BEGIN(&vdm_main,vs,af)
                 
    retry:
                nr=AUDFRAG_SIZ;
                ret=shmaudio_get(af,(unsigned char*)buffer,&nr,&ts);
				//fprintf(stderr,"af=%p\n",af);
			   // fprintf(stderr,"cache ret =%d  ts:%ld  ..nr=%d \n",ret,ts,nr);
               // fprintf(stderr,"ts:%"PRIu64" lastts:%"PRIu64" ..nr=%d \n",ts,lastts,nr);
               //
                if(ts-lastts>1){
					fprintf(stderr,"audio(%d): ts check error[ts:%ld,lastts:%ld]..\n",_i,ts,lastts);
                    //fprintf(stderr,"audio(%d): ts check error[ts:%"PRIu64",lastts:%"PRIu64"]..\n",_i,ts,lastts);
//                    usleep(1000);
//                    goto retry;
                }else if(ts-lastts==0){
                    usleep(4000);
                    goto retry;
                }
				//fprintf(stderr,"cache ts:%ld  ..nr=%d \n",ts,nr);

                lastts=ts;
                //fprintf(stderr,"write audio[%d:%p,%d]..\n",wfd,buffer,nr);
           //     write(wfd,buffer,nr);
           //     fprintf(stderr,"cache ts:%"PRIu64"  ..nr=%d audspace:%d \n",ts,nr,audbuff_space(&aud_buffer));
				
				//fprintf(stderr,"cache ts:%"PRIu64"  ..nr=%d \n",ts,nr);
              //  audbuff_put(&aud_buffer,buffer,nr);

                Push_Audio_Data((unsigned char*)buffer,nr,0);
                
            VDM_FOREACH_END
    
    
        }else{

            fprintf(stderr,"fill null audio data...\n");
            memset(buffer,0,sizeof buffer);
            nr=sizeof buffer;
        }


    }

    return NULL;
}


void* StartGetAudio(void *param)
{
	audiosource_instanse *p_instanse = (audiosource_instanse *)param;
	
	int ret;
	m_Audio sample;	
	
	sample.m_samplesSize = p_instanse->m_samplesSize;	
	//printf("sample.m_samplesSize %d \n",audiosample->m_samplesSize);	
	
	
	while(!p_instanse->flag_stop)
	{
		sample.m_AudioBuff = (int16_t*)calloc(sample.m_samplesSize,sizeof(int16_t));
		if(sample.m_AudioBuff == NULL)
		{
			printf("error:calloc fail!");
			break;
		}
		
		if((read(p_instanse->fd,(unsigned char*)sample.m_AudioBuff,sample.m_samplesSize)) < 0)      	
		{
			perror("read");
			break;
		}

		pthread_mutex_lock(&p_instanse->mutex);
		p_instanse->m_AudioQueue->push(sample);
		if(p_instanse->m_AudioQueue->size() > p_instanse->queue_max_size)		
		{
			free(p_instanse->m_AudioQueue->front().m_AudioBuff);
			p_instanse->m_AudioQueue->front().m_AudioBuff = NULL;
			p_instanse->m_AudioQueue->pop();
		}
		pthread_mutex_unlock(&p_instanse->mutex);
	}
}


audio_source_t* init_audio_source(InputParams_audiosource *pInputParams)
{
	if(pInputParams == NULL)
	{        
		fprintf(stderr ,"libaudiosource: Error paraments..\n");        
		return NULL;    
	}

	audiosource_instanse *p_instanse = (audiosource_instanse *)malloc(sizeof(audiosource_instanse));
	if(p_instanse == NULL)
	{        
		fprintf(stderr ,"libaudiosource: malloc audiosource_instanse fail..\n");        
		return NULL;    
	}
	queue<m_Audio> *m_AudioQueue = new queue<m_Audio>;
	if(m_AudioQueue == NULL)
	{
		printf("libaudiosource:new m_AudioQueue fail..!\n");
		return NULL;
	}

	p_instanse->index = pInputParams->index;
	p_instanse->m_samplesSize = 4608;
	p_instanse->flag_stop = false;
	p_instanse->queue_max_size = 8;
	p_instanse->m_AudioQueue = m_AudioQueue;

	
	int m_samplesSize = 4608;
	p_instanse->m_AudioBuff = (uint8_t*)calloc(m_samplesSize,sizeof(uint8_t));
	if(p_instanse->m_AudioBuff == NULL)
	{
		fprintf(stderr,"libaudiosource :m_AudioBuff failed\n");
	   	return NULL;
	}
	p_instanse->m_dstAudioBuff = (uint8_t*)calloc(m_samplesSize,sizeof(uint8_t));
	if(p_instanse->m_dstAudioBuff == NULL)
	{
		fprintf(stderr,"libaudiosource :calloc m_dstAudioBuff failed\n");
	   	return NULL;
	}

	char pipename[32] = {0};    	
	sprintf(pipename, "/tmp/pulse-00%d", p_instanse->index);        
	//print("pipename is  %s \n",pipename);
	if((mkfifo(pipename, O_CREAT|O_RDWR|0777)) && (errno != EEXIST))  
	{
		fprintf(stderr,"libaudiosource : mkfifo pipe failed: errno=%d\n",errno);
		return NULL;
	}
	
	chmod(pipename,0777);
	
	if((p_instanse->fd = open(pipename, O_RDWR)) < 0)          
	{          	
		perror("libaudiosource: open");          	
		return NULL;      	
	}
	
	pthread_mutex_init(&p_instanse->mutex,NULL);

	pthread_t pthread;
	int ret = pthread_create(&pthread, NULL, StartGetAudio, p_instanse);

	int index =  pInputParams->index;
    vdm_init(&vdm_main,index);

   initQueue(AudioQueSize);

	pthread_t audfetchid;
	pthread_create(&audfetchid,NULL,audio_fetch,&index);
	if(ret != 0)
	{
		printf("libaudiosource: pthread_create fail!...\n");
		return NULL;
	}

	return p_instanse;
}


FILE *fpa = NULL;
int get_audio_sample(audio_source_t *source,char *audio_sample,int *audio_sample_size)
{
	
	m_Audio sample;
	int ret = -1;

#if 0
	struct timeval tv;
	long start_time_us,end_time_us;
	gettimeofday(&tv,NULL);
	start_time_us = tv.tv_sec*1000 + tv.tv_usec/1000;
	printf("--get_audio_sample time %ld\n",start_time_us);
#endif

	if(source == NULL || audio_sample == NULL || audio_sample_size == NULL)
	{        
		fprintf(stderr ,"libaudiosource: Error paraments..\n");        
		return -1;    
	}

	audiosource_instanse *p_instanse = (audiosource_instanse *)source;

//	if(NULL==fpa)
//		fpa=fopen("/home/x00/yyd/trunk/audiosource/audiosource_mixer/mix.pcm","wb");
	

	//get decoder audio
	int iAudiolen = 0;
	unsigned long tmp=0;
	int idstAudiolen=0;
	memset(p_instanse->m_AudioBuff,0,AUDFRAG_SIZ);
	memset(p_instanse->m_dstAudioBuff,0,AUDFRAG_SIZ);
	iAudiolen = AUDFRAG_SIZ;
	idstAudiolen = AUDFRAG_SIZ;
	if(*audio_sample_size < AUDFRAG_SIZ)
	{		 
		fprintf(stderr ,"libaudiosource: audio_sample_size is toot small..\n");
		//free(sample.m_AudioBuff);
		return -2;	  
	}

	ret = get_Audio_data((unsigned char *)p_instanse->m_AudioBuff,&iAudiolen,&tmp);

	if(p_instanse->m_AudioQueue->size() <= 0)
	{
/*		if(ret <= 0)
		{
			return -1;
		}
*/		
	//	fprintf(stderr ,"libaudiosource: audio_sample_size only one ret=%d ..\n",ret);
		memcpy(audio_sample,p_instanse->m_AudioBuff,iAudiolen);
		*audio_sample_size = iAudiolen;

//		fwrite(audio_sample,1,*audio_sample_size,fpa);
//		fflush(fpa);
		return 1;
	}

	pthread_mutex_lock(&p_instanse->mutex);

	sample.m_AudioBuff = p_instanse->m_AudioQueue->front().m_AudioBuff;
	sample.m_samplesSize = p_instanse->m_AudioQueue->front().m_samplesSize;

	p_instanse->m_AudioQueue->pop();

	pthread_mutex_unlock(&p_instanse->mutex);

//	gettimeofday(&tv,NULL);
//	start_time_us = tv.tv_sec*1000 + tv.tv_usec/1000;
//	printf("--get_audio_sample time %ld\n",start_time_us);
	
	if(ret > 0)
	{
		//mixer audio
		//printf("---mixer data\n");
#if 1		
		operation_audio_pcm(p_instanse->m_AudioBuff,iAudiolen,(unsigned char*)sample.m_AudioBuff,sample.m_samplesSize,p_instanse->m_dstAudioBuff,&idstAudiolen);
		memcpy(audio_sample,p_instanse->m_dstAudioBuff,idstAudiolen);
		*audio_sample_size = idstAudiolen;
#else
		memcpy(audio_sample,p_instanse->m_AudioBuff,iAudiolen);
		*audio_sample_size = iAudiolen;
#endif		
	}
	else
	{
	//	printf("---org pulse data\n");
		memcpy(audio_sample,sample.m_AudioBuff,sample.m_samplesSize);
		*audio_sample_size = sample.m_samplesSize;
	}
//	fwrite(audio_sample,1,*audio_sample_size,fpa);
//	fflush(fpa);
	free(sample.m_AudioBuff);	
	return 1;
}

int uninit_audio_source(audio_source_t *source)
{
	if(source == NULL)
	{		 
		fprintf(stderr ,"libaudiosource: Error paraments..\n");		
		return -1;	  
	}
	audiosource_instanse *p_instanse = (audiosource_instanse *)source;
	p_instanse->flag_stop = true;

	
	while(p_instanse->m_AudioQueue->size() > 0)
	{
		free(p_instanse->m_AudioQueue->front().m_AudioBuff);
		p_instanse->m_AudioQueue->front().m_AudioBuff = NULL;
		p_instanse->m_AudioQueue->pop();
	}

	pthread_mutex_destroy(&p_instanse->mutex);

	if(p_instanse->m_AudioQueue != NULL)
		delete p_instanse->m_AudioQueue;
		
	free(p_instanse);
	
	return 0;
}


