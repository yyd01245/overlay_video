#include "tssmooth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <semaphore.h>
#include <pthread.h>


typedef struct
{
	char listen_udp_ip[32];
	int  listen_udp_port;
	char dst_udp_ip[32];
	int  dst_udp_port;

	int bit_rate;//b
	int buffer_max_size;

	char *buffer;

	int read_index;
	int write_index;

	pthread_t read_thread_id;
	pthread_t write_thread_id;
	pthread_mutex_t m_mutex;

	sem_t sem;
	int sem_flag;// 1 wait 0 no wait

	//for statistics
	unsigned long real_send_bytes;
}tssmooth_instanse;


int applyforUDPPort(char *ip,int *port)
{
	struct sockaddr_in bindaddr;
	socklen_t len;
	
	struct sockaddr_in servaddr;
	memset(&servaddr,0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip);
	servaddr.sin_port = htons(*port);
	
	int test_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(test_socket == -1)
		return -1;
	
	int optval = 1;
	if ((setsockopt(test_socket,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int))) == -1)
	{
		close(test_socket);
		return -2;
	}

	if(bind(test_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		close(test_socket);
		return -3;
	}
	
	if( 0 == getsockname( test_socket, ( struct sockaddr* )&bindaddr, &len ))
	{
		//printf("%s:%d\n",inet_ntoa(bindaddr.sin_addr),ntohs(bindaddr.sin_port));
		
		*port = ntohs(bindaddr.sin_port);
	}
	else
	{
		close(test_socket);
		return -4;
	}
	
	close(test_socket);
	return 0;
}

void *ts_recv_thread(void *arg)
{
	tssmooth_instanse *param = NULL;
	param = (tssmooth_instanse *)arg;


	struct sockaddr_in servaddr;
	memset(&servaddr,0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(param->listen_udp_port);
	
	int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_id == -1)
		return NULL;
	
	int optval = 1;
	if ((setsockopt(socket_id,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int))) == -1)
	{
		printf("ERROR :setsockopt SO_REUSEADDR error\n");
		close(socket_id);
		return NULL;
	}
	
	int recv_buffer_size = 1024*1024;	
	if((setsockopt(socket_id, SOL_SOCKET, SO_RCVBUF, (const char *)&recv_buffer_size, sizeof(int))) == -1)
	{
		printf("ERROR :setsockopt SO_RCVBUF error\n");
		close(socket_id);
		return NULL;
	}

	if(bind(socket_id, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		printf("ERROR :socket bind error\n");
		close(socket_id);
		return NULL;
	}

	struct sockaddr_in cliaddr;
	int addr_len = sizeof(struct sockaddr_in);
	char recv_buffer[2048];
	int recv_len = -1;

	while (1)
	{
		memset(recv_buffer, 0, sizeof(recv_buffer));
		recv_len = recvfrom(socket_id, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&cliaddr, (socklen_t*)&addr_len);
		if(recv_len <= 0)
			continue;

		pthread_mutex_lock(&param->m_mutex);
		
		if(param->write_index - param->read_index + recv_len > param->buffer_max_size)
		{
			//reset
			param->write_index = 0;
			param->read_index = 0;
			//
			pthread_mutex_unlock(&param->m_mutex);
			printf("Waring :buffer is too small,or send is slow,it will reset!\n");
			continue;
		}

		if(param->buffer_max_size - param->write_index < recv_len)
		{
			memmove(param->buffer,param->buffer+param->read_index,param->write_index - param->read_index);

			param->write_index = param->write_index - param->read_index;
			param->read_index = 0;
		}

		memcpy(param->buffer+param->write_index,recv_buffer,recv_len);
		param->write_index += recv_len;	

		if(param->write_index - param->read_index > 0)
		{
			if(param->sem_flag == 1)
				sem_post(&param->sem);
		}
		
		pthread_mutex_unlock(&param->m_mutex);
	}
}

int sendtolen(int socket_id,struct sockaddr_in *servaddr,char *buffer,int length)
{
	const int want_send_len = 7*188;
	
	int send_index = 0;
	int real_send_len = -1;
	int have_send_len = 0;
	int real_send_count = 0;

	while(1)
	{
		if(length - send_index >= want_send_len)
		{
			have_send_len = want_send_len;
		}
		else if(length - send_index > 0)
		{
			have_send_len = length - send_index;
		}
		else //length - send_index <= 0
		{
			//error
			break;
		}

		real_send_len = sendto(socket_id, buffer+send_index, have_send_len, 0, (struct sockaddr *)servaddr, sizeof(*servaddr));			
		if(real_send_len > 0)
		{
			send_index+=real_send_len;
			real_send_count += real_send_len;
		}
		else
		{
			send_index+=have_send_len;
			printf("Waring :sentto error have_send_len=%d real_send_len=%d errno=%d !\n",have_send_len,real_send_len,errno);
		}
	}
	return real_send_count;
}

void *ts_send_thread(void *arg)
{
	tssmooth_instanse *param = NULL;
	param = (tssmooth_instanse *)arg;
	
	struct sockaddr_in servaddr;
	memset(&servaddr, 0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;	
	servaddr.sin_addr.s_addr = inet_addr(param->dst_udp_ip);	
	servaddr.sin_port = htons(param->dst_udp_port);

	int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_id == -1)
		return NULL;
	
	int sndbuf_size = 0;	
	if((setsockopt(socket_id, SOL_SOCKET, SO_SNDBUF, (const char *)&sndbuf_size, sizeof(int))) == -1)
	{
		printf("ERROR :setsockopt SO_SNDBUF error\n");
		close(socket_id);
		return NULL;
	}
	
	const int delay_ms = 10;//ms ʱ�侫�ȶ�Ϊ10ms
	
	int send_buffer_size = 7*188*10;
	send_buffer_size = param->bit_rate/8000*delay_ms;
	//printf("--1---send_buffer_size=%d------\n",send_buffer_size);
	send_buffer_size = (send_buffer_size+1316)/1316*1316;
	//printf("---2--send_buffer_size=%d------\n",send_buffer_size);
	int send_buffer_len = 0;
	char *send_buffer = (char *)malloc(send_buffer_size);
	if(send_buffer == NULL)
	{
		printf("ERROR :malloc send_buffer error\n");
		close(socket_id);
		return NULL;
	}
	

	int enable_send_len = -1;
	int real_send_len = -1;	

	struct timeval tv;
	long long us_time1,us_time2;
	long have_delay = -1;
	
	while(1)
	{

		pthread_mutex_lock(&param->m_mutex);
		enable_send_len = param->write_index - param->read_index;
		pthread_mutex_unlock(&param->m_mutex);

		if(enable_send_len <= 0)
		{
			param->sem_flag = 1;
			sem_wait(&param->sem);
			param->sem_flag = 0;
		}

		gettimeofday(&tv, NULL);
		us_time1 = tv.tv_sec*1000*1000 + tv.tv_usec;

		pthread_mutex_lock(&param->m_mutex);
		if(param->write_index - param->read_index >= send_buffer_size)
		{
			memcpy(send_buffer,param->buffer+param->read_index,send_buffer_size);
			send_buffer_len = send_buffer_size;
			param->read_index += send_buffer_size;
		}
		else
		{
			memcpy(send_buffer,param->buffer+param->read_index,param->write_index - param->read_index);
			send_buffer_len = param->write_index - param->read_index;
			param->read_index = param->write_index;
		}
		pthread_mutex_unlock(&param->m_mutex);


		have_delay = ((double)send_buffer_len/param->bit_rate)*8*1000*1000;
		//printf("---%d---have_delay=%ld send_buffer_len=%d send_buffer_size=%d\n",__LINE__,have_delay,send_buffer_len,send_buffer_size);

		real_send_len = sendtolen(socket_id,&servaddr,send_buffer,send_buffer_len);
		param->real_send_bytes += real_send_len;
		
		gettimeofday(&tv, NULL);
		us_time2 = tv.tv_sec*1000*1000 + tv.tv_usec;

		if(have_delay-(us_time2-us_time1) > 0)
			usleep(have_delay-(us_time2-us_time1));
		
	}
}


ts_smooth_t* init_tssmooth(InputParams_tssmooth *pInputParams)
{
	if(pInputParams == NULL)
	{        
		fprintf(stderr ,"libtssmooth: Error paraments..\n");        
		return NULL;    
	}

	
	tssmooth_instanse *p_instanse = (tssmooth_instanse *)malloc(sizeof(tssmooth_instanse));
	if(p_instanse == NULL)
	{
		fprintf(stderr, "libtssmooth malloc tssmooth_instanse failed...\n");
		return NULL;
	}

	if(pInputParams->listen_udp_port <= 0)
	{
		strncpy(pInputParams->listen_udp_ip,"127.0.0.1",sizeof(pInputParams->listen_udp_ip));
		pInputParams->listen_udp_port = 0;
		int ret = applyforUDPPort(pInputParams->listen_udp_ip,&pInputParams->listen_udp_port);
		if(ret < 0)
		{
			fprintf(stderr, "+++libtssmooth applyforUDPPort is error %d ,ret = %d\n",pInputParams->listen_udp_port,ret);
			pInputParams->listen_udp_port = 40000 + pInputParams->index;
			printf("+++libtssmooth now use port is %d \n",pInputParams->listen_udp_port);
		}
		else
		{
			printf("+++libtssmooth applyforUDPPort is %d ,ret = %d\n",pInputParams->listen_udp_port,ret);
		}
	}

	memset(p_instanse,0,sizeof(tssmooth_instanse));
	strncpy(p_instanse->listen_udp_ip,pInputParams->listen_udp_ip,sizeof(p_instanse->listen_udp_ip));
	p_instanse->listen_udp_port = pInputParams->listen_udp_port;
	strncpy(p_instanse->dst_udp_ip,pInputParams->dst_udp_ip,sizeof(p_instanse->dst_udp_ip));
	p_instanse->dst_udp_port = pInputParams->dst_udp_port;
	p_instanse->bit_rate = pInputParams->bit_rate;
	p_instanse->buffer_max_size = pInputParams->buffer_max_size;
	p_instanse->read_index = 0;
	p_instanse->write_index = 0; 
	pthread_mutex_init(&p_instanse->m_mutex, 0);
	p_instanse->sem_flag = 0;
	sem_init(&p_instanse->sem, 0, 0);
	p_instanse->real_send_bytes = 0;
	
	p_instanse->buffer = (char *)malloc(p_instanse->buffer_max_size);

	if(p_instanse->buffer == NULL)
	{
		fprintf(stderr, "libtssmooth malloc buffer failed...\n");
		pthread_mutex_destroy(&p_instanse->m_mutex);
		free(p_instanse);
		return NULL;
	}

	pthread_attr_t attr;	
	pthread_attr_init(&attr);	
	pthread_attr_setstacksize(&attr,10*1024*1024);	
	
	if (pthread_create(&p_instanse->read_thread_id, &attr, &ts_recv_thread, p_instanse) != 0)		
	{
		fprintf(stderr, "libtssmooth pthread_create failed...\n");
		pthread_mutex_destroy(&p_instanse->m_mutex);
		free(p_instanse->buffer);
		free(p_instanse);
		return NULL;
	}
	if (pthread_create(&p_instanse->write_thread_id, &attr, &ts_send_thread, p_instanse) != 0)		
	{
		fprintf(stderr, "libtssmooth pthread_create failed...\n");
		pthread_cancel(p_instanse->read_thread_id);
		pthread_mutex_destroy(&p_instanse->m_mutex);
		free(p_instanse->buffer);
		free(p_instanse);
		return NULL;
	}
	
	return p_instanse;
	
}

unsigned long get_real_send_bytes(ts_smooth_t *smooth)
{
	if(smooth == NULL)
	{		 
		//fprintf(stderr ,"libtssmooth: Error paraments..\n");		
		return -1;	  
	}
	tssmooth_instanse *p_instanse = (tssmooth_instanse *)smooth;

	
	return p_instanse->real_send_bytes;
}

int uninit_tssmooth(ts_smooth_t *smooth)
{
	if(smooth == NULL)
	{		 
		fprintf(stderr ,"libtssmooth: Error paraments..\n");		
		return -1;	  
	}
	tssmooth_instanse *p_instanse = (tssmooth_instanse *)smooth;

	pthread_cancel(p_instanse->read_thread_id);
	pthread_cancel(p_instanse->write_thread_id);
	pthread_mutex_destroy(&p_instanse->m_mutex);
	if(p_instanse->buffer != NULL)
		free(p_instanse->buffer);
	free(p_instanse);

	return 0;
}



