CROSS_TOOL:=arm-unknown-linux-uclibcgnueabi-
FS_PATH=$(shell pwd)/../../../../sdk/busybox/_install

BD_CC     := $(CROSS_TOOL)gcc
BD_AR     := $(CROSS_TOOL)ar
BD_LD     := $(CROSS_TOOL)ld
STRIP	  :=$(CROSS_TOOL-ddd)strip

INCFLAG = $(BD_INCFLAGS)
CFLAG = $(BD_CFLAGS) -D_LINUX_ -fPIC -DMIJIA_PTZ_CAMERA -rdynamic
CFLAG += \
	-I./ \
	-I../../miio_bt \
	-I./gm_lib/inc \
	-I../../../extinc/include \
	-I/home/bcg/old_ipc003/ipc003tuya/tuya_sdk \
	-I/usr/include
	
DEMO_LDLIBS := \
#	-ljson-c \
	-lpthread \
        -lrt \
        -ldl \
	
		
#TUYASDKLIB = /home/bcg/old_ipc003/ipc003tuya/tuya_sdk/libtuya_ipc.a

		
LDFLAG = $(BD_LDFLAGS) $(DEMO_LDLIBS) \
	-L'$(FS_PATH)/lib' \
        -L'$(FS_PATH)/usr/lib'
SOURCES = $(wildcard *.c)
OBJS = $(patsubst %.c, $(SUB)%.o, $(SOURCES))
all:target_exe 

target_exe:  $(OBJS)
	$(BD_CC) $(INCFLAG) $(CFLAG) -o $@ $^ $(LDFLAG) $(TUYASDKLIB)
#	$(STRIP) $@
	
%.o: %.c
	 $(BD_CC) $(INCFLAG) $(CFLAG) -O2 -o $@ -c $<

release:
	cp target_exe $(RELEASE_DIR)
clean:
	rm -rf target_exe *~  *.o
