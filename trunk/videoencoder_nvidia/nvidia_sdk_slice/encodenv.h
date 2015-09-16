#ifndef _H_ENCODE_NV_
#define _H_ENCODE_NV_

#ifdef __cplusplus
extern "C"{
#endif

#include<stdio.h>





typedef struct _enc_rect{

    unsigned int x;//left
    unsigned int y;//top
    unsigned int w;//width
    unsigned int h;//height

}rect;





typedef struct _ENC_CONFIG_PUB{

    unsigned int num_frames; // only encode `num_frame` frames,or less

    unsigned int width;
    unsigned int height;

    //30000/1001=29.97; 24000/1001=23.97 60000/1001=59.94; 25/1=25; 30/1=30 ...
    unsigned int frame_rate_num;
    unsigned int frame_rate_den;

//    unsigned int rate_control; //  1= CBR. 2= VBR
//    int qp;

    unsigned int avg_bit_rate;//average bit rate
    unsigned int peak_bit_rate;//max bit rate for vbr
    unsigned int idr_period_startup;
    unsigned int idr_period;

//    unsigned int input_fmt;//1:yv12 2:nv12 3:bgra

    unsigned int device_id;

}nv_encoder_config;






typedef struct _NV_CONTROL{
    void* dummy[2];//
//    ENCODER*encoder;
//    nv_encoder_config*configure;
}nv_encoder_struct;




void _frame_conf_set_head(nv_encoder_struct*,int x,int y,int w,int h);





typedef int (*nv_encoder_read_data)(nv_encoder_struct*,void*,size_t width,size_t height,size_t filestride,size_t buffstride);
typedef int (*nv_encoder_write_data)(nv_encoder_struct*,void*,size_t framesize);
//typedef size_t (*nv_encoder_flush_data)(nv_encoder_struct*);






int nv_encoder_init(nv_encoder_struct* ,nv_encoder_config*);

int nv_encoder_fini(nv_encoder_struct*);


int nv_encoder_encodeframe(nv_encoder_struct*pencoder,const char*src,int srcsiz,char*bitstream,int*bssiz,rect*encrect);
//int nv_encoder_encodeframe(nv_encoder_struct*,char*src,size_t srcsiz,char*bitstream,size_t*bssiz);

int nv_encoder_encode(nv_encoder_struct*,int eos);

void nv_encoder_get_default_config(nv_encoder_config*config);



//int nv_encoder_update_configs(nv_encoder_struct*,nv_encoder_config*);


//set user-defined struct for read/write implementation;
void nv_encoder_set_io_data(nv_encoder_struct*,void*iodata);
void* nv_encoder_get_io_data(nv_encoder_struct*);
int nv_encoder_set_std_io(nv_encoder_struct*,FILE*src,FILE*dst);


//int nv_encoder_get_last_encoded_size(nv_encoder_struct*);


// if read nv_encoder reader callback return 0;indicate EOS & auto stop;
// if return val less than 0,error was occured;
int nv_encoder_set_read(nv_encoder_struct*,nv_encoder_read_data,void*iodata);
int nv_encoder_set_write(nv_encoder_struct*,nv_encoder_write_data,void*iodata);







#ifdef __cplusplus
}
#endif












#endif
