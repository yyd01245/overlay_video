#include "videosource.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "bvbcs.h"

#include "../vdmHelper.h"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "ippcc.h"


void patch_yuv(unsigned char*dst,int width,int height,unsigned char*src,int x,int y,int sw,int sh)
{
    
    unsigned char*psrc=src;
    unsigned char*pdst=dst;

    int rw,rh;//clipped width and height

    int stride;
    rw=sw;
    rh=sh;

    if(rw<=0||rh<=0)
        return;

    stride=width;
    pdst=dst+stride*y+x;
    psrc=src;
    for(int i=0;i<rh;i++){
        memcpy(pdst,psrc,rw);
        pdst+=stride;
        psrc+=rw;
    }

                                                                                                                                                                    
    int width_2=rw>>1;
    int height_2=rh>>1;
    unsigned char*psrcu,*psrcv;
    psrcu=src+sw*sh;//+height_2*rw+width_2;
    psrcv=src+sw*sh*5/4;//+height_2*rw+width_2;
 
    uint8_t*pline;
    int i; int j;
    pline=dst+(height+(y>>1))*width;
    for(i=0;i<height_2;i++){
 
        int off=i*width_2;
//        pline=pDst[1]+((y>>1)+i)*dstStep[1];
 
        for(j=0;j<width_2;j++){
            pline[0+x+(j<<1)]=psrcu[off+j];
            pline[1+x+(j<<1)]=psrcv[off+j];
        }  
        pline+=width;
    }
 
}





void copy_yuv(unsigned char*dst,unsigned char*src,int width,int height,int x,int y,int w,int h)
{
    
    unsigned char*psrc=src;
    unsigned char*pdst=dst;

    int stride=width;

    for(int i=y;i<y+h;i++){
        memcpy(pdst+i*stride+x,psrc+i*stride+x,w);
    }

    psrc=src+width*height;
    pdst=dst+width*height;
    stride=width/2;
    for(int i=y;i<y+h;i+=2){
        memcpy(pdst+i*stride+x,psrc+i*stride+x,w/2);
    }

    psrc=src+width*height*5/4;
    pdst=dst+width*height*5/4;
    stride=width/2;
    for(int i=y;i<y+h;i+=2){
        memcpy(pdst+i*stride+x,psrc+i*stride+x,w/2);
    }

}



static void copy_rgb(unsigned char*dst,int width,int height,unsigned char*src,int x,int y,int w,int h)
{

    int rw,rh;//clipped width and height

    rw=w;
    rh=h;
    if(x+w>width)
        rw=width-x;

    if(y+h>height)
        rh=height-y;

    
    int stride=width*4;
    unsigned char*psrc=src+y*stride+x*4;
    unsigned char*pdst=dst+y*stride+x*4;

    int i;
    for(i=0;i<rh;i++){
        memcpy(pdst,psrc,rw*4);
        psrc+=stride;
        pdst+=stride;
    }


}




static void conv_bgra_yuv420(unsigned char*prgb,int width,int height,unsigned char*yuv,int x,int y,int w,int h)
{

//    fprintf(stderr,"rgb:%p, %dx%d yuv:%p, %d.%d|%dx%d\n",prgb,width,height,yuv,x,y,w,h);

    uint8_t*pSrc=prgb;
    int srcStep=width*4;

    uint8_t*pDst[3];
    pDst[0]=yuv;
    pDst[1]=yuv+width*height;    
    int dstStep[3];
    dstStep[0]=width;
    dstStep[1]=width;


    static uint8_t p1[1280*720];//temp
    static uint8_t p2[1280*720];//temp

//    uint8_t *pDst=yuv;
    uint8_t*pDst0[3];
    pDst0[0]=pDst[0]+y*width+x;//Y
    pDst0[1]=(uint8_t*)p1;
    pDst0[2]=(uint8_t*)p2;//+((roi.width*roi.height)>>2);
 
    int DstStep0[3];
    DstStep0[0]=width;//reg.width;
    DstStep0[1]=w;
    DstStep0[2]=w;
 
    IppiSize roiSize;
    roiSize.width=w;
    roiSize.height=h;
 
    uint8_t*rgb=(uint8_t*)pSrc+(width*y+x)*4;//(roi.top*reg.width+roi.left)*4;
 
    IppStatus ippret;
//    fprintf(stderr,"roi[%d,%d],Step(%d,[%d,%d])\n",roiSize.width,roiSize.height,srcStep,DstStep0[0],DstStep0[1]);
 
    ippret=ippiBGRToYCbCr420_709CSC_8u_AC4P3R(rgb,srcStep,pDst0,DstStep0,roiSize);
    if(ippret!=ippStsNoErr){
        fprintf(stderr,"Convert RGB to YUV Failed..\n");
    }
                                                                                                                                                                    
    int width_2=w>>1;
    int height_2=h>>1;
 
    uint8_t*pline;
    int i; int j;
    pline=pDst[1]+(y>>1)*width;
    for(i=0;i<height_2;i++){
 
        int off=i*w;
//        pline=pDst[1]+((y>>1)+i)*dstStep[1];
 
        for(j=0;j<width_2;j++){
            pline[0+x+(j<<1)]=p1[off+j];
            pline[1+x+(j<<1)]=p2[off+j];
        }  
        pline+=width;
    }
 

}


static vdm_struct vdm_main;


/*

int decode_videoarea(vdm_struct*vdm,void*pdest)
{

    videosurf*vs=NULL;
    audiofrag*af=NULL;
    VDM_FOREACH_BEGIN(vdm,vs,af)

        patch_yuv((unsigned char*)pdest,1280,720,vs->data,va->x,va->y,vs->width,vs->height);

    VDM_FOREACH_END


    return 1;
}
*/

typedef struct
{
	int width;
	int height;

	
	key_t key;
	int shm_id;
	void *shm_addr;
	unsigned int shm_size;
    unsigned char*vnc_rgb;
    unsigned char*tmp_rgb;
}videosource_instanse;

video_source_t* init_video_source(InputParams_videosource *pInputParams)
{
	if(pInputParams == NULL)
	{        
		fprintf(stderr ,"libvideosource: Error paraments..\n");        
		return NULL;    
	}

	
	videosource_instanse *p_instanse = (videosource_instanse *)malloc(sizeof(videosource_instanse));
	if(p_instanse == NULL)
	{
		fprintf(stderr, "libvideosource malloc videosource_instanse failed...\n");
		return NULL;
	}

	p_instanse->width = pInputParams->width;
	p_instanse->height = pInputParams->height;
	
	p_instanse->key = (key_t)pInputParams->index;
	p_instanse->shm_size = sizeof(bvbcs_shmhdr) + p_instanse->width*p_instanse->height*4;

	p_instanse->shm_id = shmget(p_instanse->key,p_instanse->shm_size, 0666|IPC_CREAT);
	if(p_instanse->shm_id == -1)
	{
		fprintf(stderr, "libvideosource shmget failed{%s}<id:%d,key:%d,size:%d>...\n",strerror(errno),p_instanse->shm_id,p_instanse->key,p_instanse->shm_size);
		return NULL;
	}

	p_instanse->shm_addr = shmat(p_instanse->shm_id,NULL, 0);
	if(p_instanse->shm_addr == (void*)-1)
	{
		fprintf(stderr, "libvideosource shmat failed...\n");
		return NULL;
	}

	bvbcs_shmhdr*p_shm_header = (bvbcs_shmhdr*)p_instanse->shm_addr;

	printf("libvideosource : index = %d width = %d height = %d key = 0x%x  shm_id = %d shm_addr = %p shm_size = %d\n",
		pInputParams->index,p_instanse->width,p_instanse->height,p_instanse->key,p_instanse->shm_id,p_instanse->shm_addr,p_instanse->shm_size);

    p_instanse->vnc_rgb = (unsigned char *)p_instanse->shm_addr + sizeof(bvbcs_shmhdr);

    p_instanse->tmp_rgb=(unsigned char*)calloc(p_instanse->width*4,p_instanse->height);

    vdm_init(&vdm_main,pInputParams->index);


	return p_instanse;	
}

int get_video_sample_all(video_source_t *source,char *video_sample,int *video_sample_size,int *x,int *y,int *w,int *h)
{
	if(source == NULL || video_sample == NULL || video_sample_size == NULL || x == NULL || y == NULL || w == NULL || h == NULL)
	{        
		fprintf(stderr ,"libvideosource: Error paraments..\n");        
		return -1;    
	}
	videosource_instanse *p_instanse = (videosource_instanse *)source;

	if(*video_sample_size < p_instanse->shm_size)
	{		 
		fprintf(stderr ,"libvideosource: Error video_sample_size is too small..\n");		
		return -2;	  
	}
	bvbcs_shmhdr*p_shm_header = (bvbcs_shmhdr*)p_instanse->shm_addr;
	char *video_data = (char *)p_instanse->shm_addr + sizeof(bvbcs_shmhdr);
	
	printf("libvideosource get_video_sample_all: p_shm_header->flag=%d.x=%d y=%d w=%d h=%d.\n",p_shm_header->flag,p_shm_header->x,p_shm_header->y,p_shm_header->w,p_shm_header->h);

	*x = 0;
	*y = 0;
	*w = p_instanse->width;
	*h = p_instanse->height;
	*video_sample_size = p_instanse->width*p_instanse->height*3/2;

    int stride=p_instanse->width*4;

   
   
//copy data to tmp surface
    int i;
    for(i=0;i<p_instanse->height;i++){
	    memcpy(p_instanse->tmp_rgb+i*stride,video_data+i*stride,stride);
    }

//convert bgra to yuv420
		
    unsigned char*pSrc=p_instanse->tmp_rgb;
	int pDstStep[3];
	unsigned char *pDst[3];
	pDstStep[0]=p_instanse->width;
	pDstStep[1]=pDstStep[0]/2;
	pDstStep[2]=pDstStep[1];

	IppiSize roiSize;
	roiSize.width=p_instanse->width;
	roiSize.height=p_instanse->height;

	pDst[0]=(unsigned char*)video_sample;
	pDst[1]=pDst[0]+p_instanse->width*p_instanse->height;
	pDst[2]=pDst[1]+p_instanse->width*p_instanse->height/4;
	ippiBGRToYCbCr420_709CSC_8u_AC4P3R(pSrc,p_instanse->width*4,(unsigned char**)pDst,pDstStep,roiSize);

    conv_bgra_yuv420((unsigned char*)p_instanse->tmp_rgb,p_instanse->width,p_instanse->height,(unsigned char*)video_sample,0,0,p_instanse->width,p_instanse->height);

	return 0;
}


int get_video_sample_inc(video_source_t *source,char *video_sample,int *video_sample_size,int *x,int *y,int *w,int *h)
{
	if(source == NULL || video_sample == NULL || video_sample_size == NULL || x == NULL || y == NULL || w == NULL || h == NULL)
	{        
		fprintf(stderr ,"libvideosource: Error paraments..\n");        
		return -1;    
	}
	videosource_instanse *p_instanse = (videosource_instanse *)source;

	if(*video_sample_size < p_instanse->shm_size)
	{		 
		fprintf(stderr ,"libvideosource: Error video_sample_size is too small..\n");		
		return -2;	  
	}
	bvbcs_shmhdr*p_shm_header = (bvbcs_shmhdr*)p_instanse->shm_addr;
//	char *video_data = (char *)p_instanse->shm_addr + sizeof(bvbcs_shmhdr);
    int stride=p_instanse->width*4;
	
    int noupdate=0;

    int vdmret,vncret;
    int _x0,_x1,_y0,_y1,_w,_h;

    vncret=shmhdr_get_extents(p_shm_header,&_x0,&_y0,&_w,&_h,&_x1,&_y1);
    if(vncret){
        copy_rgb(p_instanse->tmp_rgb,1280,720,p_instanse->vnc_rgb,_x0,_y0,_w,_h);
        conv_bgra_yuv420(p_instanse->tmp_rgb,1280,720,(unsigned char*)video_sample,_x0,_y0,_w,_h);
        noupdate=1;
    }


    videosurf*vs=NULL;
    audiofrag*af=NULL;
    vdmret=vdm_check_videosurface(&vdm_main);
    if(vdmret>0){
        
//        fprintf(stderr,".");
        VDM_FOREACH_BEGIN(&vdm_main,vs,af)

            int px=vdm_main.rects[_i].x1;
            int py=vdm_main.rects[_i].y1;
            int pw=vdm_main.rects[_i].x2-px;
            int ph=vdm_main.rects[_i].y2-py;

//            copy_rgb(p_instanse->tmp_rgb,1280,720,p_instanse->vnc_rgb,px,py,pw,ph);
//            fprintf(stderr,"(%d):x:%d.y:%d,%dx%d\n",i,px,py,pw,ph);
//            conv_bgra_yuv420(p_instanse->tmp_rgb,1280,720,(unsigned char*)video_sample,px,py,pw,ph);
            patch_yuv((unsigned char*)video_sample,1280,720,vs->data,px,py,pw,ph);


        VDM_FOREACH_END

        noupdate=1;
    }else
    if(vdmret<0){
//full flush

        VDM_FOREACH_BEGIN(&vdm_main,vs,af)

            int px=vdm_main.rects[_i].x1;
            int py=vdm_main.rects[_i].y1;
            int pw=vdm_main.rects[_i].x2-px;
            int ph=vdm_main.rects[_i].y2-py;

            copy_rgb(p_instanse->tmp_rgb,1280,720,p_instanse->vnc_rgb,px,py,pw,ph);
            conv_bgra_yuv420(p_instanse->tmp_rgb,1280,720,(unsigned char*)video_sample,px,py,pw,ph);

        VDM_FOREACH_END

        noupdate=1;
    }
// no update


    if(noupdate){
    	*x = 0;
    	*y = 0;
    	*w = 0;
    	*h = 0;
    	*video_sample_size = 0;

        return 0;
    }

	*x = 0;
	*y = 0;
	*w = p_instanse->width;
	*h = p_instanse->height;
	*video_sample_size = p_instanse->width*p_instanse->height*3/2;


	return 0;
}




int uninit_video_source(video_source_t *source)
{
	if(source == NULL)
	{		 
		fprintf(stderr ,"libvideosource: Error paraments..\n");		
		return -1;	  
	}
	videosource_instanse *p_instanse = (videosource_instanse *)source;

	if(shmdt(p_instanse->shm_addr) == -1)
	{
		fprintf(stderr, "libvideosource shmdt failed...\n");	
		return -1;	  
	}

    vdm_fini(&vdm_main);

    free(p_instanse->tmp_rgb);

	free(p_instanse);

	return 0;
}


