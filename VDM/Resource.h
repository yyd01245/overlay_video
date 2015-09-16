
#ifndef _H_RESOURCE_
#define _H_RESOURCE_


#include<pthread.h>

#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

#include"vdmHelper.h"
//#include"avshm.h"
//#include"avstub.h"

#include<list>
#include<string>
#include<iostream>

using namespace std;


//#define PROC_AVDECODER_PATH "../../yyd/vdp_decoder_sync_yuv/vdpau_decoder_v1.0/decoder"
#define PROC_AVDECODER_PATH "../../yyd/vdp_decoder_sync_yuv/decoder"
#define PROC_AUDIO_PATH "./audiodispatcher2"
//#define PROC_AVDECODER_PATH "./fakedecoder"

#define AUDBUFF_SIZE 4608

//////////////////////////



class DecodeResource;




class VideoStub{

    public:
        VideoStub(int index);
        ~VideoStub();
        static void cleanup();

        int set_resource(DecodeResource*res,int x,int y);
        int unset_resource(DecodeResource*res,int x=-1,int y=-1);

        void print();

    private:

        static int _nvnc;
        static int _nref;

        static bool _inited;
        static int _vidshmid;
        static int _vidshmsize;
        static key_t _vidshmkey;
        static void* _pvidshm;

        avstub *_areas;
        int _area_idx;


};





class VideoResource{

    public:
        VideoResource(int index,int x=0,int y=0);
        ~VideoResource();

        bool operator==(VideoResource&other);


        void start_audio();

        void stop_audio();

        void get_position(int &x,int &y){
            x=_x;
            y=_y;
        }

        int get_index()const{
            return _index;
        }

        int get_resid()const{
            return _vresid;
        }


        VideoStub*get_stub(void){
            return _vncstub;
        }

        void print(){
            cerr<<"\t\tVideoResource::[id:"<<_vresid<<"|index:"<<_index<<"|x:"<<_x<<",y:"<<_y<<"]"<<endl;
        }

        void present(char*buff,int siz){
            snprintf(buff,siz,"VideoResource:<resid:%d|idx:%d|x:%d.y:%d>",_vresid,_index,_x,_y);
        }


        DecodeResource*get_decode(){
            return _decres;
        }

        void set_decode(DecodeResource*res){
            _decres=res;
        }

    private:

        VideoStub*_vncstub;
        DecodeResource*_decres;

        int _x,_y;//video area position
        int _index;

        int _vresid;

        int _apid;//audio dispatcher pid

        static int _s_resid;

};



class DecodeResource{

    public:
        DecodeResource(string url,int width,int height);
        ~DecodeResource();

        bool operator==(DecodeResource&other);
        VideoResource* attach_video(VideoResource* vnc);
        VideoResource* detach_video(VideoResource* vnc);
        VideoResource* seek_video(int index,int x,int y);
        VideoResource* seek_video_by_resid(int resid);
        list<VideoResource*>seek_video_by_index(int index);


        void print_videos(char*buff,int siz);

        int get_video_count()const{
            return _vidresources.size();
        }

        void dispatch();


        list<VideoResource*>*get_videos(){
            return &_vidresources;
        }

        const string& get_url()const {
            return _res_url;
        }

        inline void get_geometry(int&w,int&h){
            w=_res_width;
            h=_res_height;
        }


        inline int get_videokey(){
            return _vidsurf_key;
        } 

        inline int get_videoshmid(){
            return _vidsurf_shmid;
        } 

        inline const char* get_audiopath(){
            return _audfrag_path;
        } 

        inline int get_audioshmid(){
            return _audfrag_shmid;
        } 
/*
        inline int get_audiofd(){
            return _audfrag_fd;
        } 
*/
        inline audiofrag*get_audiofrag(){
            return _paudfrag;
        }

        inline int get_decpid(){
            return _dec_pid;
        } 

    private:


        int create_ipc();
        void destroy_ipc();
        int generate_videokey(string url,int width,int height);
        int generate_audiopath(string url);


        string _res_url;
        int _res_width,_res_height;

        key_t _vidsurf_key;
        int _vidsurf_shmid;

        key_t _audfrag_key;
        int _audfrag_shmid;

        audiofrag*_paudfrag;//
        char _audfrag_path[32];

//        int _audfrag_fd;

        list<VideoResource*> _vidresources;


        int _dec_pid; //decoder process
        bool _audio_thr;
        pthread_t _audio_tid;

};







#endif
