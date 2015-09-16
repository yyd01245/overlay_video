files=avencoder.cpp 

libs=-pthread -lavmuxer -laudioencoder -laudiosource -lvideoencoder -lvideosource -ltssmooth

DEBUG=-g  -D_DEBUG
	
avencoder:avencoder.cpp
	-rm -f avencoder
	g++ $(files) -fPIC $(libs) $(DEBUG) -D__linux__ -o avencoder
clean:
	-rm -f avencoder
	-rm -f *.so

