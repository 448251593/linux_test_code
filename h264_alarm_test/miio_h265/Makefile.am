AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS = miio_h265
miio_h265_LDFLAGS = -ljson-c -lmp4v2 -lcurl -lmortox -lmortox_share -lcrypto -lpthread -lrt -lshbfev -lev -lshbf -lcrypto -lmiio_util
miio_h265_CFLAGS = $(AM_FLAGS) -O0 -g3 -Wall -fmessage-length=0 -s

miio_h265_SOURCES = src/h265_main.c \
					src/h265_g711_mp4.c \
					src/h265_g711_queue.c \
					src/h265_motion_event.c \
					src/json.c 	\
					src/json.h 	\
					src/rpc_handle.c 	\
					src/rpc_handle.h 	\
					src/curl.c 	\
					src/curl.h 	\
					src/util.c

distclean-local:
	-rm -rf Makefile.in compile autom4te.cache aclocal.m4 config.h.in configure depcomp install-sh missing autoscan.log config.guess config.sub libfetch.la ltmain.sh
