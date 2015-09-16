#!/usr/bin/python3

import sys
import socket
import json
import time
import random



class VDM:

    BUFSIZ=2048
    def __init__(self,ip,port):
        self.address=(ip,port)
        self.resids=[]
        self.cnt=0

    def _connect(self):
        self.sock=socket.socket(socket.AF_INET,socket.SOCK_STREAM);
        self.sock.connect(self.address);
    
    def _close(self):
        self.sock.close();

    def send_msg(self,smsg):
        self._connect();
        ns=self.sock.send(smsg.encode());
        rmsg=self.sock.recv(self.BUFSIZ).decode();
        self._close();
        
        print("Recv: "+rmsg)
        xxee_off=rmsg.find("XXEE");
        if xxee_off<0:
            return -1,{},""
        
        retdict=json.loads(rmsg[:xxee_off]);
        
        return int(retdict['retcode']),retdict,rmsg


class DecodeRes:
    def __init__(self,url,w,h):
        self.url=url;
        self.width=w;
        self.height=h;

    def get_url(self):
        return self.url
    def get_geometry(self):
        return self.width,self.height


class VideoRes:
    def __init__(self,index,x,y):
        self.index=index;
        self.x=x;
        self.y=y;

    def get_index(self):
        return self.index

    def get_position(self):
        return self.x,self.y




class VDMessage:

    def __init__(self):
        pass

    def add_decoder(self,dr,vr):
        x,y=vr.get_position()
        index=vr.get_index();
        w,h=dr.get_geometry();
        url=dr.get_url();
        d={"cmd":"adddecoder",
                "vncindex":str(index),
                "addr":str(url),
                "x":str(x),
                "y":str(y),
                "width":str(w),
                "height":str(h)};

        jsonstr=json.dumps(d)+"XXEE";
        print("Add Decoder with: "+jsonstr)
        return jsonstr

    def del_decoder(self,vr):
        index=vr.get_index();
        d={"cmd":"deldecoder",
                "vncindex":str(index)};

        jsonstr=json.dumps(d)+"XXEE";
        print("Delete Decoder with: "+jsonstr)
        return jsonstr


class TestVDM:
    
    def __init__(self,vdm,logname="./test-vdm.log"):
        self.vdm=vdm
        self.logf=open(logname,'a+')
        self.logf.write('\n\n')
        self.logf.write('=============================\n')
        pass



    def log(self,sstr,level='msg'):
        now=time.strftime("%y/%m/%d %H:%M:%S")
        nows=(str(time.clock_gettime(time.CLOCK_REALTIME)).split('.')[1]+'000')[0:3]
        head=tail=''
        if level=='error':
            head='\033[31m'
            tail='\033[0m'
        output=now+nows+"\033[32m ["+str(level)+"]\033[0m "+":"+head+str(sstr)+tail
        print(output,file=sys.stderr)
        self.logf.write(output+"\n")



    def runTest0(self):
        index=1
        dr=DecodeRes("udp://a.ts",400,300);
        vr0=VideoRes(index,10,10)
        vr1=VideoRes(index,410,10)
        vr2=VideoRes(index,10,320)
        vr3=VideoRes(index,410,320)

        l0=[vr0,vr1,vr2,vr3]
        l1=[] 

        while True:
            d=random.randint(0,9)

            vdmm=VDMessage()
            msg=""


            if d%2==0 and len(l0)>0:
                v=l0[0];
                l0.pop(0);
                msg=vdmm.add_decoder(dr,v)
                l1.append(v);
            elif d%2==1 and len(l1)>0:
                v=l1[0];
                l1.pop(0);
                msg=vdmm.del_decoder(v)
                l0.append(v);
            else:
                continue


            self.vdm.send_msg(msg)


            time.sleep(1)





if __name__ == "__main__":
    r=VDM("192.168.188.132",9898);


    tv=TestVDM(r)


    tv.runTest0();




