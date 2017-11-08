all :  ov5642_capture streamer

objects = arducam.o arducam_arch_raspberrypi.o

streamer : $(objects) streamer.o
	gcc -o streamer $(objects) streamer.o -lwiringPi -Wall -lpthread
ov5642_capture : $(objects) arducam_ov5642_capture.o
	gcc -o ov5642_capture $(objects) arducam_ov5642_capture.o -lwiringPi -Wall -lpthread

arducam.o : arducam.c
	gcc -c arducam.c -lwiringPi -Wall
arducam_arch_raspberrypi.o : arducam_arch_raspberrypi.c
	gcc -c arducam_arch_raspberrypi.c -lwiringPi -Wall

streamer.o : streamer.c
	gcc -c streamer.c -lwiringPi -Wall -lpthread
arducam_ov5642_capture.o : arducam_ov5642_capture.c
	gcc -c arducam_ov5642_capture.c -lwiringPi -Wall -lpthread

clean :
	rm -f  ov5642_capture streamer $(objects) *.o
