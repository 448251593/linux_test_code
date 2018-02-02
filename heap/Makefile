INCLUDEFLAG=-I.
USER_SRC_DIRS =$(shell find . -type d)
USER_SRCS += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.c)) 

USER_OBJS=$(addsuffix .o, $(basename  $(USER_SRCS) ) )
CC=gcc
test:
	@echo $(USER_SRCS)
	@echo $(USER_OBJS)
	

all:$(USER_OBJS)
	$(CC) $(INCLUDEFLAG) $(USER_OBJS) -o exerun

%.o:%.c
	$(CC) $(INCLUDEFLAG) -c $^ -o $@
clean:
	rm $(USER_OBJS) exerun -rf
