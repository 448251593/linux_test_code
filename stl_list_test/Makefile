INCLUDEFLAG= -I.
USER_SRC_DIRS =$(shell find . -type d)
USER_SRCS += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.c)) 
#USER_SRCS = rpc_demo.c
USER_OBJS=$(addsuffix .o, $(basename  $(USER_SRCS) ) )
CC=g++



test:$(USER_OBJS)
	$(CC) $(INCLUDEFLAG) $^ -o $@
#$(TARGET):
#	$(CC) $(INCLUDEFLAG) $^ -o $@

%.o:%.c
	$(CC) $(INCLUDEFLAG) -c $^ -o $@
clean:
	rm *.o $(TARGET) -rf
