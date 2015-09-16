#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<sys/mman.h>
#include<fcntl.h>
#include<sys/types.h>
#include<signal.h>

#include<cuda.h>
#include"nvEncodeAPI.h"
#include"helper_nvenc.h"
#include"drvapi_error_string.h"
//#include"formats.h"
#include"encodenv.h"


typedef int bool;

#define TRUE 1
#define FALSE 0


typedef enum _NvEncodeCodecType{

    NV_ENC_CODEC_UNKNOW=0,
    NV_ENC_CODEC_H264,
    NV_ENC_CODEC_HEVC

}NvEncCodecType;

typedef enum _NvEncodeStart{

    NV_ENC_STARTUP_NORMAL=0,
    NV_ENC_STARTUP_SLOW

}NvEncodeStartup;

typedef enum _NvEncodeInterfaceType{

    NV_ENC_IFACE_NORMAL=0,//
    NV_ENC_IFACE_CUDA

}NvEncIfaceType; 


typedef enum _NvEncodeInputFmt{

    NV_ENC_INPUT_NULL=0,
    NV_ENC_INPUT_YV12=1<<0,
    NV_ENC_INPUT_NV12=1<<1,//2,
    NV_ENC_INPUT_BGRX=1<<2,//4,

}NvEncInputFmt; 


/*
                     conv              copy
    INPUTDATA(SHM) --------> HOSTDATA -------> DATADEVICE [HOST Conversion(using ippc)]

*/




typedef struct _ENC_CONFIG{

    NvEncCodecType              codec;//Only H264 allowed

    unsigned int                profile;
    int                         preset;
    unsigned int                level;

    unsigned int                width;
    unsigned int                height;
    unsigned int                maxWidth;
    unsigned int                maxHeight;

    unsigned int                frameRateNum;
    unsigned int                frameRateDen;

    unsigned int                avgBitRate;
    unsigned int                maxBitRate;
    unsigned int                gopLength0;//==5
    unsigned int                gopLength;//==100
//    unsigned int                enableInitialRCQP;
//    NV_ENC_QP                   initialRCQP;
//    unsigned int                numBFrames;//set to 0
//    unsigned int                fieldEncoding;
//    unsigned int                bottomFieldFrist;
    unsigned int                rateControl; // 0= QP, 1= CBR. 2= VBR
    int                         qp_I,qp_P;
//    unsigned int                vbvBufferSize;
//    unsigned int                vbvInitialDelay;
//    NV_ENC_MV_PRECISION         mvPrecision;
//    unsigned int                enablePTD;
//    int                         syncMode;
//    NvEncIfaceType              iface_type;


//    unsigned int                chromaFormatIDC;//1 for YUV420 ;3 for YUV444
//    unsigned int                separateColourPlaneFlag;
//    unsigned int                enableAQ;
    unsigned int                intraRefreshCount;//set to 1
    unsigned int                intraRefreshPeriod; //key frame

//    unsigned int                idr_period_startup;//adopted when frame_count <=idr_period

//    unsigned int                vle_cabac_enable;
//    int                         numSlices;
//    unsigned int                sliceMode;
//    unsigned int                sliceModeData;

    unsigned int                device_id;

//    NvEncInputFmt               input_fmt;

    unsigned int                num_frames;



}ENC_CONFIG;



static void _init_enc_config(ENC_CONFIG*econf)
{

    memset(econf,0,sizeof(ENC_CONFIG));

    econf->codec =NV_ENC_CODEC_H264;

    econf->rateControl =NV_ENC_PARAMS_RC_CBR;
//    econf->rateControl =NV_ENC_PARAMS_RC_CBR;

    //set below values to 0;Fix Startup's Low Quality of Iframe..
//    econf->vbvBufferSize = 2000000;
//    econf->vbvInitialDelay = 1000000;

    econf->avgBitRate = 2000000;
    econf->maxBitRate = econf->avgBitRate+econf->avgBitRate;

//    econf->gopLength = NVENC_INFINITE_GOPLENGTH;//notice this 
    econf->gopLength = 25;
    econf->gopLength0 = 5;

    econf->frameRateNum =25;
    econf->frameRateDen =1;

    econf->width =1280;
    econf->height=720;
    econf->maxWidth =econf->width;
    econf->maxHeight=econf->height;

//    econf->preset = NV_ENC_PRESET_DEFAULT;//0
//    econf->profile = NV_ENC_CODEC_PROFILE_AUTOSELECT; //0
    econf->level =NV_ENC_LEVEL_AUTOSELECT;//0
 
    econf->num_frames =0;
    
//    econf->enableAQ =0;

    econf->intraRefreshCount =1;//1
    econf->intraRefreshPeriod =25;

//    econf->vle_cabac_enable =0;
//    econf->fieldEncoding =0;
//    econf->sliceMode =H264_SLICE_MODE_NUMSLICES;
//    econf->sliceModeData =1;

    econf->device_id=-1;//1;


}



void merge_config(ENC_CONFIG*conf,nv_encoder_config*pubconf)
{

    conf->num_frames=pubconf->num_frames; // only encode `num_frame` frames,or less

    conf->width=pubconf->width;
    conf->height=pubconf->height;
    conf->maxWidth=conf->width;
    conf->maxHeight=conf->height;

    //30000/1001=29.97; 24000/1001=23.97 60000/1001=59.94; 25/1=25; 30/1=30 and etc
    conf->frameRateNum = pubconf->frame_rate_num;
    conf->frameRateDen = pubconf->frame_rate_den;

//    conf->rateControl = pubconf->rate_control;

//TODO
//    if(conf->rateControl==)

    conf->avgBitRate=pubconf->avg_bit_rate;//average bit rate
    if(pubconf->peak_bit_rate>pubconf->avg_bit_rate)
        conf->maxBitRate=pubconf->peak_bit_rate;//max bit rate
    else
        conf->maxBitRate=pubconf->avg_bit_rate*2;//max bit rate



    conf->gopLength =pubconf->idr_period;//NVENC_INFINITE_GOPLENGTH;
    conf->intraRefreshPeriod = conf->gopLength;
    if(pubconf->idr_period_startup<=0){
//        conf->startup_type=NV_ENC_STARTUP_NORMAL;
        conf->gopLength0=0;
    }else{
//        conf->startup_type=NV_ENC_STARTUP_SLOW;
        conf->gopLength0=pubconf->idr_period_startup;
        fprintf(stderr,"\033[32mSlow Startup{%d:%d} \033[0m\n",conf->gopLength0,conf->gopLength);
    }

//    conf->input_fmt=NV_ENC_BUFFER_FORMAT_NV12_PL;
#if 0
    switch(pubconf->input_fmt)//1:yv12 2:nv12 3:bgrx
    {
        case 1:
            conf->input_fmt|=NV_ENC_INPUT_YV12;
            break;

        case 2:
            conf->input_fmt|=NV_ENC_INPUT_NV12;
            break;

        case 3:
            conf->input_fmt|=NV_ENC_INPUT_BGRX;
            break;

        default:
            fprintf(stderr,"WARN::Input Type not set,Using NV12 default..\n");
            conf->input_fmt|=NV_ENC_INPUT_NV12;
            
    }
#endif
    conf->device_id=pubconf->device_id;

}









typedef struct _EncInputStruct{
    //NV12 Format here
    unsigned int      dwWidth;//max
    unsigned int      dwHeight;//max
    unsigned int      dwStride;//for cudaMallocPitch

    unsigned int      dwLumaSize;//==chromaOffset
    unsigned int      dwChromaSize;//==lumaSize/2
    unsigned int      dwChromaOffset;//=dwWidth*dwHeight

    NV_ENC_BUFFER_FORMAT bufferFmt;
    NvEncInputFmt    inputType;

    void              *pExtAlloc;//For Encode Input<NV12>, copy from pExtAllocHost
    unsigned char     *pExtAllocHost;//converted data

    NV_ENC_INPUT_RESOURCE_TYPE type;

    void              *hMappedResource;//mapped
    void              *hRegisteredHandle;//registered

}ENC_INPUT_INFO;



typedef void* HANDLE;

typedef  struct _EncOutputInfo{

    unsigned int     dwSize;
    unsigned int     dwBitstreamDataSize;
    void             *hBitstreamBuffer;
    void             *pBitstreamBufferPtr;
    bool             bEOSFlag;
//    bool             bReconfiguredflag;

}ENC_OUTPUT_INFO;





typedef struct _EncFrameConf{

    unsigned int width;//current width
    unsigned int height;//current height
    unsigned int stride[3];//current stride may equal to width
    unsigned char* yuv[3];
//    patch_t       enc_roi;//patch size
    NV_ENC_PIC_STRUCT picStruct;//NV_ENC_PIC_STRUCT_FRAME
    NvEncodeStartup startup_type;//for gopsize
    int frame_count;
}ENC_FRAME_CONFIG;





///////////////////////////////////////////////////////////////////////////////


/*First Step.*/

NV_ENCODE_API_FUNCTION_LIST* init_encode_api(void)
{

    NV_ENCODE_API_FUNCTION_LIST*encapi;
    NVENCSTATUS nvret;
    encapi=calloc(1,sizeof(NV_ENCODE_API_FUNCTION_LIST));
    if(!encapi)
        return NULL;
    encapi->version=NV_ENCODE_API_FUNCTION_LIST_VER; 

    nvret=NvEncodeAPICreateInstance(encapi);
    //<CHK>
    CHK_NVENC_ERR(nvret);

    return encapi;
}

void fini_encode_api(NV_ENCODE_API_FUNCTION_LIST*encapi)
{
    if(!encapi)
        return;

    free(encapi);

}



typedef struct _Encoder_Main{

    NV_ENCODE_API_FUNCTION_LIST*encapi;

    void*handler;

    nv_encoder_struct*caller;

    ENC_CONFIG *config;



    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS st_enc_session_par;
//    NV_ENC_CAPS_PARAM caps_pars;
//    GUID client_guid;
    GUID st_encode_guid;
    GUID st_preset_guid;
    GUID st_profile_guid;
    

    NV_ENC_CONFIG st_encode_config;
    NV_ENC_PRESET_CONFIG st_preset_config;

    NV_ENC_BUFFER_FORMAT st_input_fmt;

    NV_ENC_INITIALIZE_PARAMS st_init_enc_pars;
//    NV_ENC_RECONFIGURE_PARAMS st_reconf_enc_pars;

    //per frame's configuration
    ENC_FRAME_CONFIG st_frame_conf;

    //input&output resources;
    ENC_INPUT_INFO input;
    ENC_OUTPUT_INFO output;


    CUcontext st_cu_ctx;
//////////////////////////

    unsigned int st_frame_width;
    unsigned int st_frame_height;
    unsigned int st_max_width;
    unsigned int st_max_height;

    //profile performance
    struct timespec ts0;
    struct timespec ts1;


}ENCODER;




void _frame_conf_set_head(nv_encoder_struct*pencoder,int x,int y,int w,int h)
{
    if(!pencoder)
        return;

    ENCODER*enc=(ENCODER*)pencoder->dummy[0];
/*
    enc->st_frame_conf.enc_roi.left=x;
    enc->st_frame_conf.enc_roi.top=y;
    enc->st_frame_conf.enc_roi.width=w;
    enc->st_frame_conf.enc_roi.height=h;
*/
}





int open_encoder_session(enc)
    ENCODER* enc;
{

    int ret=0;
    NVENCSTATUS nvret;
    
    //Init Cuda
    //

    CUresult cuResult;
    CUdevice cuDevice;
    CUcontext cuContext;




    cuResult=cuInit(0);
    CHK_CUDA_ERR(cuResult);

    int devcount;
    int devid;

    cuResult=cuDeviceGetCount(&devcount);
    CHK_CUDA_ERR(cuResult);

    printf("Dev Count:= %d\n",devcount);
    devid=enc->config->device_id;
    
    if(devid==-1 || devid>=devcount){

        devid=get_next_core();
        fprintf(stderr,"\033[33mAuto Select DevID:= %d\n\033[0m",devid);
        enc->config->device_id=devid;

    }else{
        fprintf(stderr,"\033[33mGiven DevID:= %d\n\033[0m",devid);
    }

    cuResult=cuDeviceGet(&cuDevice,devid/*enc->config->device_id*/);
    CHK_CUDA_ERR(cuResult);

    cuResult=cuCtxCreate(&cuContext,0,cuDevice);
    CHK_CUDA_ERR(cuResult);


    enc->st_enc_session_par.device = cuContext;//reinterpret_cast<void *>(m_cuContext);
    enc->st_enc_session_par.deviceType = NV_ENC_DEVICE_TYPE_CUDA;

    enc->st_enc_session_par.version=NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    enc->st_enc_session_par.apiVersion= NVENCAPI_VERSION;


    nvret=enc->encapi->nvEncOpenEncodeSessionEx(&enc->st_enc_session_par,&enc->handler);
    CHK_NVENC_ERR(nvret);


    return ret;

}


int set_guid(enc)
    ENCODER*enc;
{

    int ret;
    NVENCSTATUS nvret;


    int i;
    unsigned int nb_encode_guid=0;
    unsigned int n_encode_guid=0;

    nvret=enc->encapi->nvEncGetEncodeGUIDCount(enc->handler,&nb_encode_guid);
    CHK_NVENC_ERR(nvret);


    GUID *encode_guids=calloc(nb_encode_guid,sizeof(GUID));

    nvret=enc->encapi->nvEncGetEncodeGUIDs(enc->handler,encode_guids,nb_encode_guid,&n_encode_guid);
    CHK_NVENC_ERR(nvret);

    fprintf(stderr,"\033[32mSupported Codec [%d]\033[0m\n",n_encode_guid);
/*
    for(i=0;i<n_encode_guid;i++){
        if(compareGUIDs(encode_guids[i],NV_ENC_CODEC_H264_GUID)){
            break;
        }
    }

    fprintf(stderr,"-> Encode Guid[H264(%d)]\n",i);

    enc->st_encode_guid=encode_guids[i];
*/
    enc->st_encode_guid=NV_ENC_CODEC_H264_GUID;
    free(encode_guids);


    unsigned int nb_preset_guid=0;
    unsigned int n_preset_guid=0;
//Preset GUIDs;
    nvret=enc->encapi->nvEncGetEncodePresetCount(enc->handler,enc->st_encode_guid,&nb_preset_guid);
    CHK_NVENC_ERR(nvret);

    fprintf(stderr,"INFO::Preset GUID Count:%d\n",nb_preset_guid);

    GUID *preset_guids=calloc(nb_preset_guid,sizeof(GUID));

    nvret=enc->encapi->nvEncGetEncodePresetGUIDs(enc->handler,enc->st_encode_guid,preset_guids,nb_preset_guid,&n_preset_guid);
    CHK_NVENC_ERR(nvret);
#if 0
    GUID guid;

    for(i=0;i<n_preset_guid;i++){
        guid=get_preset_by_index(i);
        if(compareGUIDs(preset_guids[i],guid)){
            fprintf(stderr,"Find preset:%d \n",i);
            break;
        }
    }

    if(i==n_preset_guid){
        fprintf(stderr,"WARN:: Preset GUID not found,using 0 default \n");
        i=0;
    }else{
        fprintf(stderr,"-> Preset GUID[%d]\n",i);

    }

    enc->st_preset_guid=preset_guids[i];
#endif

    enc->st_preset_guid=NV_ENC_PRESET_HP_GUID;


    enc->st_encode_config.version=NV_ENC_CONFIG_VER;
    enc->st_preset_config.version=NV_ENC_PRESET_CONFIG_VER;
    enc->st_preset_config.presetCfg.version=NV_ENC_CONFIG_VER;


    nvret=enc->encapi->nvEncGetEncodePresetConfig(enc->handler,enc->st_encode_guid,enc->st_preset_guid,&enc->st_preset_config);
    CHK_NVENC_ERR(nvret);

    free(preset_guids);

    memcpy(&enc->st_encode_config,&enc->st_preset_config.presetCfg,sizeof(NV_ENC_CONFIG));

    unsigned int nb_profile_guid=0;
    unsigned int n_profile_guid=0;
//Profile GUIDs;
//
    nvret=enc->encapi->nvEncGetEncodeProfileGUIDCount(enc->handler,enc->st_encode_guid,&nb_profile_guid);
    CHK_NVENC_ERR(nvret);
    //
    fprintf(stderr,"-> %d profile GUIDs found\n",nb_profile_guid);

    GUID *profile_guids=calloc(nb_profile_guid,sizeof(GUID));

    nvret=enc->encapi->nvEncGetEncodeProfileGUIDs(enc->handler,enc->st_encode_guid,profile_guids,nb_profile_guid,&n_profile_guid);
    CHK_NVENC_ERR(nvret);


#if 0
    for(i=0;i<n_profile_guid;i++){
        guid=get_profile_by_index(i);
        if(compareGUIDs(profile_guids[i],guid)){
            fprintf(stderr,"Find Profile:%d \n",i);
            break;
        }
    }
    if(i==n_profile_guid){
        fprintf(stderr,"WARN:: Profile GUID not found,using 0 default \n");
        i=0;
    }else{
        fprintf(stderr,"-> Profile Guid[%d]\n",i);

    }

    enc->st_profile_guid=profile_guids[i];
#endif

    enc->st_profile_guid=NV_ENC_H264_PROFILE_BASELINE_GUID;

    free(profile_guids);

    return ret;

}





int set_format(enc)
    ENCODER*enc;
{
#if 0    
    int ret=0;
    NVENCSTATUS nvret;

    nvret=enc->encapi->nvEncGetInputFormatCount(enc->handler,enc->st_encode_guid,&enc->nb_input_fmt);
    //<CHK>
    CHK_NVENC_ERR(nvret);

    NV_ENC_BUFFER_FORMAT*input_fmts=calloc(enc->nb_input_fmt,sizeof(NV_ENC_BUFFER_FORMAT));
    unsigned int n_input_fmt;

    nvret=enc->encapi->nvEncGetInputFormats(enc->handler,enc->st_encode_guid,input_fmts,enc->nb_input_fmt,&n_input_fmt);
    //<CHK>
    CHK_NVENC_ERR(nvret);

    fprintf(stderr,"Number of Supported Formats(%d)\n",n_input_fmt);
    free(input_fmts);
    return ret;
#else
    //direct assignments
    enc->st_input_fmt=NV_ENC_BUFFER_FORMAT_NV12_PL;
#endif

    return 0;
}





void init_encoder(enc)
    ENCODER*enc;
{
     
    int ret;
    NVENCSTATUS nvret;

    //0.Query Caps..
    
    int val;

    NV_ENC_CAPS_PARAM cap_pars;
    cap_pars.version=NV_ENC_CAPS_PARAM_VER;
    cap_pars.capsToQuery=NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT;

    nvret=enc->encapi->nvEncGetEncodeCaps(enc->handler,enc->st_encode_guid,
            &cap_pars,&val);
    CHK_NVENC_ERR(nvret);
    printf(" Async Mode Caps[%d]\n",val);


    cap_pars.capsToQuery=NV_ENC_CAPS_SUPPORT_FIELD_ENCODING;
    nvret=enc->encapi->nvEncGetEncodeCaps(enc->handler,enc->st_encode_guid,
            &cap_pars,&val);
    CHK_NVENC_ERR(nvret);
    printf(" Support FieldEncoding Caps[%d]\n",val);




/**
 * Fill enc->st_init_enc_pars;
 *
 */

    enc->st_init_enc_pars.version=NV_ENC_INITIALIZE_PARAMS_VER;

    enc->st_encode_config.version=NV_ENC_CONFIG_VER;


    enc->st_frame_width=enc->config->width;
    enc->st_frame_height=enc->config->height;

    enc->st_max_width=enc->config->maxWidth;
    enc->st_max_height=enc->config->maxHeight;


    enc->st_init_enc_pars.darHeight=enc->config->height;//Aspect ratio
    enc->st_init_enc_pars.darWidth=enc->config->width;
    enc->st_init_enc_pars.encodeHeight=enc->config->height;
    enc->st_init_enc_pars.encodeWidth=enc->config->width;

    enc->st_init_enc_pars.maxEncodeHeight=enc->config->maxHeight;
    enc->st_init_enc_pars.maxEncodeWidth=enc->config->maxWidth;

    enc->st_init_enc_pars.frameRateNum=enc->config->frameRateNum;
    enc->st_init_enc_pars.frameRateDen=enc->config->frameRateDen;

    enc->st_init_enc_pars.enableEncodeAsync=0;//enc->b_async_mode;//not support

    enc->st_init_enc_pars.enablePTD=1;//enc->config->enablePTD;

    enc->st_init_enc_pars.encodeGUID=enc->st_encode_guid;
    enc->st_init_enc_pars.presetGUID=enc->st_preset_guid;


    enc->st_init_enc_pars.encodeConfig=&enc->st_encode_config;
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

    enc->st_init_enc_pars.encodeConfig->monoChromeEncoding=0;
    enc->st_init_enc_pars.encodeConfig->frameFieldMode=NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;

    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.chromaFormatIDC=1;//enc->config->chromaFormatIDC;

    enc->st_init_enc_pars.encodeConfig->profileGUID=enc->st_profile_guid;

    enc->st_init_enc_pars.encodeConfig->frameIntervalP=1;//enc->config->numBFrames+1;//==1

    enc->st_init_enc_pars.encodeConfig->gopLength=enc->config->gopLength;
//    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.sliceMode=enc->config->sliceMode;
//    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.sliceModeData=enc->config->sliceModeData;
//    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.separateColourPlaneFlag=enc->config->separateColourPlaneFlag;
//    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.bdirectMode=enc->config->numBFrames > 0 ? NV_ENC_H264_BDIRECT_MODE_TEMPORAL : NV_ENC_H264_BDIRECT_MODE_DISABLE;
//    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.disableSPSPPS=(NV_ENC_H264_ENTROPY_CODING_MODE)!!enc->config->vle_cabac_enable;

    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.repeatSPSPPS=1;
    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.entropyCodingMode=NV_ENC_H264_ENTROPY_CODING_MODE_AUTOSELECT;
    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.idrPeriod=enc->config->gopLength;
    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.level=enc->config->level;

    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.enableIntraRefresh=enc->config->intraRefreshCount>0;
    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.intraRefreshCnt=enc->config->intraRefreshCount;
    enc->st_init_enc_pars.encodeConfig->encodeCodecConfig.h264Config.intraRefreshPeriod=enc->config->intraRefreshPeriod;


//    enc->st_init_enc_pars.encodeConfig->mvPrecision=NV_ENC_MV_PRECISION_QUARTER_PEL;//enc->config->mvPrecision;



    enc->st_init_enc_pars.encodeConfig->rcParams.rateControlMode=enc->config->rateControl;
    enc->st_init_enc_pars.encodeConfig->rcParams.averageBitRate=enc->config->avgBitRate;
    enc->st_init_enc_pars.encodeConfig->rcParams.maxBitRate=enc->config->maxBitRate;
//    enc->st_init_enc_pars.encodeConfig->rcParams.vbvBufferSize=enc->config->vbvBufferSize;
//    enc->st_init_enc_pars.encodeConfig->rcParams.vbvInitialDelay=enc->config->vbvInitialDelay;
//    enc->st_init_enc_pars.encodeConfig->rcParams.enableAQ=enc->config->enableAQ;


//    enc->st_init_enc_pars.encodeConfig->rcParams.enableInitialRCQP=enc->config->enableInitialRCQP;
//    enc->st_init_enc_pars.encodeConfig->rcParams.initialRCQP.qpInterB=enc->config->initialRCQP.qpInterB;
//    enc->st_init_enc_pars.encodeConfig->rcParams.initialRCQP.qpInterP=enc->config->initialRCQP.qpInterP;
//    enc->st_init_enc_pars.encodeConfig->rcParams.initialRCQP.qpIntra=enc->config->initialRCQP.qpIntra;


    nvret=enc->encapi->nvEncInitializeEncoder(enc->handler,&enc->st_init_enc_pars);
    CHK_NVENC_ERR(nvret);
    //


    if(enc->config->gopLength0==0)
        enc->st_frame_conf.startup_type=NV_ENC_STARTUP_NORMAL;
    else
        enc->st_frame_conf.startup_type=NV_ENC_STARTUP_SLOW;


}




void fini_buffers(enc)
    ENCODER*enc;
{
    
    int ret=0;
    NVENCSTATUS nvret;

    nvret=enc->encapi->nvEncUnmapInputResource(enc->handler,enc->input.hMappedResource);

    nvret=enc->encapi->nvEncUnregisterResource(enc->handler,enc->input.hRegisteredHandle);

    CHK_NVENC_ERR(nvret);


    cuMemFree((CUdeviceptr)enc->input.pExtAlloc);
    cuMemFreeHost(enc->input.pExtAllocHost);

    enc->input.hMappedResource=NULL;


    nvret=enc->encapi->nvEncDestroyBitstreamBuffer(enc->handler,enc->output.hBitstreamBuffer);
    CHK_NVENC_ERR(nvret);

    enc->output.hBitstreamBuffer=NULL;


}





void fini_encoder(enc)
    ENCODER*enc;
{
    
    int ret=0;
    NVENCSTATUS nvret;

    nvret=enc->encapi->nvEncDestroyEncoder(enc->handler);
    CHK_NVENC_ERR(nvret);

    if(enc->st_cu_ctx){

        CUresult cuResult=CUDA_SUCCESS;
        cuResult=cuCtxDestroy(enc->st_cu_ctx);
        CHK_CUDA_ERR(cuResult);
    }

}






int init_buffers(enc)
    ENCODER*enc;
{

    
    int ret=0;
    NVENCSTATUS nvret;
    CUresult cuResult;


    NV_ENC_REGISTER_RESOURCE reg_res={0,};


    enc->input.dwWidth=enc->st_max_width;
    enc->input.dwHeight=enc->st_max_height;

    ////////////////////////////
    //Allocate INPUT Resource
    //

    CUcontext cuCtxCur;
    CUdeviceptr devPtr;

//    cuResult=cuMemAllocPitch(&devPtr,(size_t*)&enc->input.dwCuPitch,enc->st_max_width,enc->st_max_height*3/2,16);
    cuResult=cuMemAlloc(&devPtr,enc->input.dwWidth*enc->input.dwHeight*3/2);
    CHK_CUDA_ERR(cuResult);

    enc->input.pExtAlloc=(void*)devPtr;
    enc->input.dwStride=enc->input.dwWidth;//fake pitch==dwWidth 

    fprintf(stderr,"CUDA INPUT BUFFER::Stride =%u\n",enc->input.dwStride);
    fprintf(stderr,"CUDA INPUT BUFFER::Width  =%u\n",enc->input.dwWidth);
    fprintf(stderr,"CUDA INPUT BUFFER::Height =%u\n",enc->input.dwHeight);


    cuResult=cuMemAllocHost((void**)&enc->input.pExtAllocHost,enc->input.dwStride*enc->input.dwHeight*3/2);//YUV420(NV12)
    CHK_CUDA_ERR(cuResult);

    enc->input.type=NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
    enc->input.bufferFmt=enc->st_input_fmt;


    enc->input.dwLumaSize=enc->input.dwStride*enc->input.dwHeight;
    int lumasiz=enc->input.dwLumaSize;
    enc->input.dwChromaOffset=lumasiz;//(enc->config->chromaFormatIDC==3)?lumasiz:lumasiz>>2;//yuv444 or yuv420
    enc->input.dwChromaSize=lumasiz>>1;//yuv444 or yuv420



    ///////////////////////
    //Using Mapped Resource..
    reg_res.version=NV_ENC_REGISTER_RESOURCE_VER;
    reg_res.resourceType=enc->input.type;

    reg_res.resourceToRegister=enc->input.pExtAlloc;
    reg_res.width = enc->input.dwWidth;
    reg_res.height= enc->input.dwHeight;
    reg_res.pitch = enc->input.dwStride;

    nvret=enc->encapi->nvEncRegisterResource(enc->handler,&reg_res);
    CHK_NVENC_ERR(nvret);

    enc->input.hRegisteredHandle=reg_res.registeredResource;

    ///map resource
    NV_ENC_MAP_INPUT_RESOURCE map_res={0,};
    map_res.version=NV_ENC_MAP_INPUT_RESOURCE_VER;
    map_res.registeredResource=enc->input.hRegisteredHandle;
    nvret=enc->encapi->nvEncMapInputResource(enc->handler,&map_res);
    CHK_NVENC_ERR(nvret);

    enc->input.hMappedResource=map_res.mappedResource;

    ////////////////////////////
    //Allocate OUTPUT Resource
    //

    NV_ENC_CREATE_BITSTREAM_BUFFER output_buff;
    bzero(&output_buff,sizeof(NV_ENC_CREATE_BITSTREAM_BUFFER));
    output_buff.version=NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
    
    enc->output.dwSize=1024*1024;//1MiB per frame

    output_buff.size=enc->output.dwSize;
    output_buff.memoryHeap=NV_ENC_MEMORY_HEAP_AUTOSELECT;

    nvret=enc->encapi->nvEncCreateBitstreamBuffer(enc->handler,&output_buff);
    CHK_NVENC_ERR(nvret);
    //

    enc->output.hBitstreamBuffer=output_buff.bitstreamBuffer;
    enc->output.pBitstreamBufferPtr=output_buff.bitstreamBufferPtr;//Reserved and should not be used 

/////////////////////////////////////
////////////////////////////////////

    enc->st_frame_conf.yuv[0]=enc->input.pExtAllocHost;
    enc->st_frame_conf.yuv[1]=enc->input.pExtAllocHost+enc->input.dwLumaSize;
    enc->st_frame_conf.yuv[2]=NULL;//NV12 not used

    enc->st_frame_conf.stride[0]=enc->input.dwStride;
    enc->st_frame_conf.stride[1]=enc->input.dwStride;
    enc->st_frame_conf.stride[2]=0;//enc->input.dwStride/2;

    enc->st_frame_conf.width=enc->input.dwWidth;
    enc->st_frame_conf.height=enc->input.dwHeight;

    enc->st_frame_conf.picStruct=NV_ENC_PIC_STRUCT_FRAME;


    return ret;
}




void flush_encoder(enc)
    ENCODER*enc;
{

    NVENCSTATUS nvret;

    NV_ENC_PIC_PARAMS pic_pars={0,};

    pic_pars.version=NV_ENC_PIC_PARAMS_VER;

    pic_pars.encodePicFlags=NV_ENC_PIC_FLAG_EOS;
    pic_pars.completionEvent=NULL;//Only sync mode supported

    enc->output.bEOSFlag=TRUE;//Indicate this output is EOS

    nvret=enc->encapi->nvEncEncodePicture(enc->handler,&pic_pars);
    CHK_NVENC_ERR(nvret);

}


int nv_encoder_encodeframe(nv_encoder_struct*pencoder,const char*src,int srcsiz,char*bitstream,int*bssiz,rect*encrect)
{

    int ret=0;
    ENCODER*enc=(ENCODER*)pencoder->dummy[0];

///////////////////////////////////////////////////////
    NVENCSTATUS nvret;

    ENC_FRAME_CONFIG*econf=&enc->st_frame_conf;

    int dwWidth=enc->input.dwWidth;//config->width;
    int dwHeight=enc->input.dwHeight;//config->height;
//    int dwSurfWidth=(dwWidth+0x1f)&~0x1f;//align 32 as driver does the same
//    int dwSurfHeight=(dwHeight+0x1f)&~0x1f;
    int dwSurfWidth=enc->input.dwStride;//(dwWidth+0x1f)&~0x1f;//align 32 as driver does the same
    int dwSurfHeight=dwHeight;//(dwHeight+0x1f)&~0x1f;
//    fprintf(stderr,"[[[[[SurfWidth:%d  SurfHeight:%d]]]]]\n",dwSurfWidth,dwSurfHeight);

    unsigned char* inputSurfLuma;//luma channel
    unsigned char* inputSurfChroma;//chroma channel
    int lockedPitch;//Buffer Stride..


    //

    //
    //Synchronous Mode
    //1. load yuv data into buffer


    lockedPitch=enc->input.dwStride;
    inputSurfLuma=enc->input.pExtAllocHost;
    inputSurfChroma=inputSurfLuma+enc->input.dwChromaOffset;//(dwSurfHeight*lockedPitch);


    enc->input.inputType=NV_ENC_INPUT_BGRX;//enc->config->input_fmt;


    if(enc->input.inputType & NV_ENC_INPUT_BGRX){

//        int af_luma_siz=dwSurfWidth*dwSurfHeight;
//        int af_chroma_siz=af_luma_siz/2;

        if(encrect->w==0||encrect->h==0){
//        if(econf->enc_roi.width==0||econf->enc_roi.height==0){
            //Need not load data anymore
            //fprintf(stderr,"Invalid width/height of Patch area, To Skip..\n");
            goto SKIP_LOAD_DATA;
        }



//        conv_bgra_to_nv12(src,dwWidth*4,pDst,dstStep,econf->enc_roi,reg);
  
        load_rgb_bgrx_ippcc_patch(econf->yuv[0],src,
        encrect->x,encrect->y,encrect->w,encrect->h,//patch rectangle
        dwHeight,
        dwWidth);


    }
    else{
        fprintf(stderr,"Only RGBX INPUT Supporting now..\n");
        return -1;
    }

//SKIP_LOAD_DATA:


    CUcontext cuCtxCur;
    CUresult cuResult;

    //copy converted yuv data to device memory
    cuResult=
    cuMemcpyHtoD((CUdeviceptr)enc->input.pExtAlloc,enc->input.pExtAllocHost,
            enc->input.dwStride*enc->input.dwHeight*3/2);
//    cuMemcpyHtoDAsync((CUdeviceptr)enc->input.pExtAlloc,enc->input.pExtAllocHost,
//            enc->input.dwCuPitch*enc->input.dwHeight*3/2,NULL);

SKIP_LOAD_DATA:
    (void)0;    
    //2. encode a frame

    NV_ENC_PIC_PARAMS pic_pars;
    bzero(&pic_pars,sizeof(NV_ENC_PIC_PARAMS));

    pic_pars.version=NV_ENC_PIC_PARAMS_VER;

    pic_pars.inputBuffer=enc->input.hMappedResource;
    pic_pars.bufferFmt=enc->input.bufferFmt;
    pic_pars.inputWidth=enc->input.dwWidth;
    pic_pars.inputPitch=enc->input.dwStride;
    pic_pars.inputHeight=enc->input.dwHeight;
    pic_pars.outputBitstream=enc->output.hBitstreamBuffer;
    pic_pars.completionEvent=NULL;//Only Synchronous mode supported
    pic_pars.pictureStruct=econf->picStruct;
    pic_pars.encodePicFlags =0;
    pic_pars.inputTimeStamp =0;
    pic_pars.inputDuration  =0;
    pic_pars.codecPicParams.h264PicParams.sliceMode =enc->st_encode_config.encodeCodecConfig.h264Config.sliceMode;
    pic_pars.codecPicParams.h264PicParams.sliceModeData =enc->st_encode_config.encodeCodecConfig.h264Config.sliceModeData;


    if(econf->startup_type==NV_ENC_STARTUP_SLOW){
        if(econf->frame_count<=enc->config->gopLength 
                && econf->frame_count%enc->config->gopLength0==0){
//            pic_pars.codecPicParams.h264PicParams.refPicFlag=1;
            pic_pars.encodePicFlags|=NV_ENC_PIC_FLAG_FORCEIDR;//only valid when enablePTD==1
//            pic_pars.encodePicFlags|=NV_ENC_PIC_FLAG_FORCEINTRA;
        }
    }

    econf->frame_count++;

//    memcpy(&pic_pars.rcParams,&enc->st_encode_config.rcParams,sizeof(NV_ENC_RC_PARAMS));

    nvret=enc->encapi->nvEncEncodePicture(enc->handler,&pic_pars);
    //<CHK>
    CHK_NVENC_ERR(nvret);



    if(enc->output.hBitstreamBuffer==NULL && enc->output.bEOSFlag == FALSE){
        //ERROR
        fprintf(stderr,"Error::Copy Bitstream Data\n");
    }


    NV_ENC_LOCK_BITSTREAM bstream={0,};
//    bzero(&bstream,sizeof(NV_ENC_LOCK_BITSTREAM));

    bstream.version=NV_ENC_LOCK_BITSTREAM_VER;
    bstream.outputBitstream=enc->output.hBitstreamBuffer;
    bstream.doNotWait=0;//FALSE;//blocking when output not ready..


    nvret=enc->encapi->nvEncLockBitstream(enc->handler,&bstream);
    CHK_NVENC_ERR(nvret);
    //if ok

    //copy es to user
    memcpy(bitstream,bstream.bitstreamBufferPtr,bstream.bitstreamSizeInBytes );
    *bssiz=bstream.bitstreamSizeInBytes;
 ////
    nvret=enc->encapi->nvEncUnlockBitstream(enc->handler,enc->output.hBitstreamBuffer);
    CHK_NVENC_ERR(nvret);

    return 0;

}






void fini_encoder_config(conf)
    ENC_CONFIG*conf;
{
    if(conf)
        free(conf);
}



ENC_CONFIG*init_encoder_config()
{
    ENC_CONFIG*conf=calloc(1,sizeof(ENC_CONFIG));
    if(!conf){
        return NULL;
    }
    _init_enc_config(conf);


    return conf;
}





void nv_encoder_get_default_config(nv_encoder_config*pconfig)
{

    bzero(pconfig,sizeof(nv_encoder_config));

    pconfig->device_id=-1;

    pconfig->width=1280;
    pconfig->height=720;

    pconfig->frame_rate_num=25;
    pconfig->frame_rate_den=1;

    pconfig->idr_period=25;

//    pconfig->input_fmt=NV_ENC_INPUT_BGRX;

//    pconfig->rate_control=2;
    pconfig->avg_bit_rate=  2000000;
    pconfig->peak_bit_rate =6000000;

    pconfig->num_frames=0;

}








static void fini_nv_encoder(ENCODER*enc)
{

    if(!enc)
        return;

    fini_buffers(enc);

    fini_encoder(enc);


    fini_encoder_config(enc->config);

    fini_encode_api(enc->encapi);

    free(enc);

}


static ENCODER* init_nv_encoder(ENC_CONFIG*conf)
{

    int ret;

    ENCODER*enc=calloc(1,sizeof(ENCODER));
    if(!enc){
        return NULL;
    }

//    enc->b_async_mode=0;


    enc->config=conf;

    enc->encapi=init_encode_api();
    if(!enc->encapi){
        return NULL;
    }


    open_encoder_session(enc);
    fprintf(stderr,"After Open Encode Session..\n");

    set_guid(enc);
    fprintf(stderr,"After Set GUIDs..\n");
    
    set_format(enc);
    fprintf(stderr,"After Init Formats..\n");

    init_encoder(enc);
    fprintf(stderr,"After Init Encoder..\n");

    init_buffers(enc);
    fprintf(stderr,"After Init Resource..\n");


    ret=clock_gettime(CLOCK_REALTIME,&enc->ts0);
    if(ret<0){
        fprintf(stderr,"WARN:: GET TIME...\n");
    }

    return enc;
}





int nv_encoder_init(nv_encoder_struct*pencoder ,nv_encoder_config*pconfig)
{

    int ret;

    ENC_CONFIG* conf=init_encoder_config();
    if(!conf){
        fprintf(stderr,"ERROR:: Can not allocate configuration struct..\n");
        return -1;
    }

    //read configuration from pconfig;
    merge_config(conf,pconfig);


    ENCODER*enc=init_nv_encoder(conf);
    enc->caller=pencoder;


    pencoder->dummy[0]=enc;
    pencoder->dummy[1]=pconfig;

    return 0;

}



int nv_encoder_fini(nv_encoder_struct*pencoder)
{
    
    ENCODER*enc=(ENCODER*)pencoder->dummy[0];

    fini_nv_encoder(enc);

    fprintf(stderr,"Encoder Finalized...\n");

}







