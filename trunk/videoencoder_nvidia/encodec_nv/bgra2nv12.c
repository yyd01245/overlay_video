#include<ippcc.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>


void load_rgb_bgrx_ippcc_patch(unsigned char*yuv,unsigned char*rgb,
        int left,int top,int width,int height,//patch rectangle
        int rgbheight,
        int ostride)
{
#if 0 
    static int icnt;
    if(icnt++%100==0)
        fprintf(stderr,"RGB<%p> , YUV<%p>.{%d,%d}\n",rgb,yuv,ostride,rgbheight);
#endif 
    static unsigned char p2[1920*1080];//temp

    int _width=ostride;
    int _height=rgbheight;

    unsigned char *pDst=yuv;
    unsigned char*pDst2[3];
    pDst2[0]=yuv+top*_width+left;
    pDst2[1]=(unsigned char*)p2;//yuv+rgbheight*ostride;
    pDst2[2]=(unsigned char*)p2+((_width*_height)>>2);

    int pDstStep[3];
    pDstStep[0]=_width;
    pDstStep[1]=_width>>1;
    pDstStep[2]=_width>>1;

    IppiSize roiSize;
    roiSize.width=width;
    roiSize.height=height;

    int chromaoff=_width*_height;

    IppStatus ippret;

    ippret=ippiBGRToYCbCr420_709CSC_8u_AC4P3R(rgb,width*4,pDst2,pDstStep,roiSize);
    if(ippret!=ippStsNoErr){
        fprintf(stderr,"Convert RGB to YUV Failed..\n");
    }

    int width_2=width>>1;
    int height_2=height>>1;
    int _width_2=_width>>1;
#if 1
    char*pline;
    int i; int j;
    for(i=0;i<height_2;i++){
        pline=yuv+chromaoff+((top>>1)+i)*_width;
        int ioff=i*_width_2;
        for(j=0;j<width_2;j++){
            int j2=j<<1;
            pline[left+j2  ]=pDst2[1][ioff+j];
            pline[left+j2+1]=pDst2[2][ioff+j];
        }  
    }
#endif
}


