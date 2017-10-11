#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <signal.h>
#include <execinfo.h>

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#define USER_LIMIT 		10
#define BUFFER_SIZE 	1024
#define FD_LIMIT 		30
#define OTD_IP      	"127.0.0.1"
#define OTD_PORT    	54322

typedef struct client_data                  //  客户端的数据结构
{
    struct sockaddr_in  address;            //
    char*               write_buf;                //  发送数据缓冲区
    char                buf[ BUFFER_SIZE ];        //  接收数据缓冲区
}client_data;

	client_data         *users = NULL;
	struct pollfd       fds[USER_LIMIT + 1];
	int user_counter = 0;








static int timer_handler(int fd)
{
	uint64_t exp = 0;

	/* just read out the "events" in fd, otherwise poll will keep
	 * reporting POLLIN */
	read(fd, &exp, sizeof(uint64_t));

	printf("---------> 1s timer\n");
	return 0;

}
static int timer_handler1(int fd)
{
	uint64_t exp = 0;

	/* just read out the "events" in fd, otherwise poll will keep
	 * reporting POLLIN */
	read(fd, &exp, sizeof(uint64_t));

	printf("---------> 2s timer\n");
	return 0;

}
int timer_setup(void)
{
	int fd;

	fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (fd < 0) {
		perror("timerfd_create");
		return fd;
	}

	return fd;
}

int timer_start(int fd, int first_expire, int interval)
{
	struct itimerspec new_value;

	new_value.it_value.tv_sec = first_expire / 1000;
	new_value.it_value.tv_nsec = first_expire % 1000 * 1000000;

	new_value.it_interval.tv_sec = interval / 1000;
	new_value.it_interval.tv_nsec = interval % 1000 * 1000000;

	if (timerfd_settime(fd, 0, &new_value, NULL) == -1) {
		perror("timerfd_settime");
		return -1;
	}

	return 0;
}


int main(int argc, char**argv)
{
    int	listenfd;
	int	timerfd;
	int	timerfd2;
	int listen_connfd;
	int client_connfd;
    int ret = 0;
	printf( "--maketime-<%s-%s>-----\n",__DATE__, __TIME__); 


	//定时器
	timerfd = timer_setup();
	fds[0].fd = timerfd;
   	fds[0].events = POLLIN ;		   //  POLLIN表示有数据可读
   	fds[0].revents = 0;
    timer_start(timerfd, 1000, 1000);
	
	//定时器
	timerfd2 = timer_setup();
	fds[1].fd = timerfd2;
   	fds[1].events = POLLIN ;		   //  POLLIN表示有数据可读
   	fds[1].revents = 0;
    timer_start(timerfd2, 2000, 2000);
	/* 初始化poll的 从2开始初始化*/
    int ii = 0;
    for( ii = 2; ii <= USER_LIMIT; ++ii )
    {
        fds[ii].fd = -1;
        fds[ii].events = 0;
    }

	int i = 0;

    while( 1 )
    {
        ret = poll( fds,                        //  准备轮训的套接字文件描述符
                    2,           //
                    -1);                        //   poll 永远等待。poll() 只有在一个描述符就绪时返回，或者在调用进程捕捉到信号时返回
        if ( ret < 0 )
        {
            printf( "poll failure\n" );
            break;
        }

		/*poll模型的本质就是轮训, 在pull返回时，轮询所有的文件描述符, 查找到有事情请求的那个文件 */
        for(i = 0; i <2; ++i )
        {
            if( fds[i].revents & POLLIN)              /*  有数据可读取  */
           	{
		       	if((fds[i].fd == timerfd))           
	        	{
					timer_handler(fds[i].fd);
				}
				else if((fds[i].fd == timerfd2))           
	        	{
					timer_handler1(fds[i].fd);
				}				
			}
        }
    }

    free(users);
    close(listenfd);
    return 0;
}

