#include"pcmmix.h"






void pcm_mix_16_ch2(int16_t*pmixed,int16_t*paud,int length)
{
    int cnt=length/(sizeof(int16_t));

    int i;
    for(i=0;i<cnt;i++){
        int16_t*psamp0=pmixed+i;
        int16_t*psamp1=paud+i;
        int16_t samp0=*psamp0;
        int16_t samp1=*psamp1;

        *psamp0=samp0+samp1;

    }

}
