#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <sys/timerfd.h>
#define OT_MAX_PAYLOAD  1024
#define BUFFER_MAX  4096
#define OTD_IP      "127.0.0.1"
#define OTD_PORT    8989
#define MAX_POLL_NUMS    10
#define POLL_TIMEOUT    100	/* 100ms */

#define TIMER_INTERVAL  1000*60*60*2	/* 2h */
#define TIMER_INTERVAL_REPORT 5000

struct kit_struct {
	struct 	pollfd pollfds[MAX_POLL_NUMS];
	int 	count_pollfds;
	int 	timerfd2;
	int 	otd_sock;
	bool 	force_exit;

};


struct kit_struct kit = {
	.count_pollfds = 0,
	.timerfd2 = 0,
	.otd_sock = 0
};



static int msg_dispatcher(const char *msg, int len, int sock)
{
	char ackbuf[OT_MAX_PAYLOAD];
	int ret = -1, id = 0;
	bool sendack = false;
	memset(ackbuf, 0, sizeof(ackbuf));
	printf(  "%s, msg: %s, strlen: %d, len: %d\n", __func__, msg, (int)strlen(msg), len);
#if 0
	//{"method": "local.status", "params": "cloud_retry"},
	if (json_verify_method_value(msg, "method", "local.status", json_type_string) == 0
	    && json_verify_method_value(msg, "params", "cloud_retry", json_type_string) == 0)
	{
		gIsLogin = 0;
		return 0;
	}
	//get id
	ret = json_verify_get_int(msg, "id", &id);
	if (ret < 0) 
	{
		return ret;
	}
	
	if (json_verify_method_value(msg, "method", "test", json_type_string) == 0) 
	{

		printf(  "Got deleteVideo...\n");

	}
	else if (json_verify_method_value(msg, "method", "test2", json_type_string) == 0)
	{

		printf(  "Got deleteSaveVideo...\n");
		

	} 

#endif
	if (sendack)
		//ret = general_send_one(sock, ackbuf, strlen(ackbuf));

	return ret;
}

static int otd_sock_init(void)
{
	struct sockaddr_in serveraddr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf(  "Create socket error: %m\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(OTD_IP);
	serveraddr.sin_port = htons(OTD_PORT);

	if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
		printf(  "Connect to otd error: %s:%d\n", OTD_IP, OTD_PORT);
		return -1;
	}

	return sockfd;
}

void otd_reconnect(void)
{
	int n;

	printf(  "Retry connecting to miio_client...\n");
	kit.otd_sock = otd_sock_init();
	if (kit.otd_sock > 0) {
		printf(  "Connected to miio_client.\n");

		kit.pollfds[kit.count_pollfds].fd = kit.otd_sock;
		kit.pollfds[kit.count_pollfds].events = POLLIN;
		printf(  "OTD sockfd: %d\n", kit.pollfds[kit.count_pollfds].fd);
		kit.count_pollfds++;

		/* sanity check for pollfds */
		if (kit.count_pollfds >= MAX_POLL_NUMS) {
			printf(  "%s, %d: too many sockets to track\n", __FILE__, __LINE__);
			for (n = 0; n < kit.count_pollfds; n++)
				printf(  "pollfds[%d]: %d\n", n, kit.pollfds[n].fd);
			exit(-1);
		}
	}
}

void otd_close_retry(void)
{
	int n, found;

	if (kit.otd_sock > 0) {
		/* close sock */
		found = 0;
		for (n = 0; n < kit.count_pollfds; n++) {
			if (kit.pollfds[n].fd == kit.otd_sock) {
				found = 1;
				while (n < kit.count_pollfds) {
					kit.pollfds[n] = kit.pollfds[n + 1];
					n++;
				}
			}
		}

		if (found)
			kit.count_pollfds--;
		else
			printf(  "kit.otd_sock (%d) not in pollfds.\n", kit.otd_sock);

		close(kit.otd_sock);
		kit.otd_sock = 0;
	}

	/* We'll do retry in timer_handler */
	return;
}
#if 0
/* In some cases, we might receive several accumulated json RPC, we need to split these json.
 * E.g.:
 *   {"count":1,"stack":"sometext"}{"count":2,"stack":"sometext"}{"count":3,"stack":"sometext"}
 *
 * return the length we've consumed, -1 on error
 */
static int otd_recv_handler_block(int sockfd, char *msg, int msg_len)
{
	struct json_tokener *tok;
	struct json_object *json;
	int ret = 0;

//	printf(  "%s(), sockfd: %d, msg: %.*s, length: %d bytes\n",
//		   __func__, sockfd, msg_len, msg, msg_len);
	if (json_verify(msg) < 0)
		return -1;

	/* split json if multiple */
	tok = json_tokener_new();
	while (msg_len > 0) {
		char *tmpstr;
		int tmplen;

		json = json_tokener_parse_ex(tok, msg, msg_len);
		if (json == NULL) {
			printf(  "%s(), token parse error msg: %.*s, length: %d bytes\n",
				   __func__, msg_len, msg, msg_len);
			json_tokener_free(tok);
			return ret;
		}

		tmplen = tok->char_offset;
		tmpstr = malloc(tmplen + 1);
		if (tmpstr == NULL) {
			printf(  "%s(), malloc error\n", __func__);
			json_tokener_free(tok);
			json_object_put(json);
			return -1;
		}
		memcpy(tmpstr, msg, tmplen);
		tmpstr[tmplen] = '\0';

		msg_dispatcher((const char *)tmpstr, tmplen, sockfd);

		free(tmpstr);
		json_object_put(json);
		ret += tok->char_offset;
		msg += tok->char_offset;
		msg_len -= tok->char_offset;
	}

	json_tokener_free(tok);

	return ret;
}
#endif
static int otd_recv_handler(int sockfd)
{
	char 	buf[BUFFER_MAX];
	ssize_t count;
	int 	left_len = 0;
	bool 	first_read = true;
	int 	ret = 0;

	while (true) 
	{
		count = recv(sockfd, buf + left_len, sizeof(buf) - left_len, MSG_DONTWAIT);
		if (count < 0) 
		{
			return -1;
		}

		if (count == 0) 
		{
			if (first_read) 
			{
				otd_close_retry();
			}

			if (left_len)
			{
				buf[left_len] = '\0';
				printf(  "%s() remain str: %s\n", __func__, buf);
			}

			return 0;
		}
		first_read = false;

		//ret = otd_recv_handler_block(sockfd, buf, count + left_len);
		//if (ret < 0) 
		//{
		//	printf(  "%s_one() return -1\n", __func__);
		//	return -1;
		//}

		left_len = count + left_len - ret;
		memmove(buf, buf + ret, left_len);
	}

	return 0;
}


/**
 * 
 */
static int timer_handler2(int fd)
{
	int n;
	static int i = 0;
	uint64_t exp = 0;

	/* just read out the "events" in fd, otherwise poll will keep
	 * reporting POLLIN */
	n = read(fd, &exp, sizeof(uint64_t));

	if (kit.otd_sock <= 0) {
		otd_reconnect();
	}
	
	return n;
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


int main()
{
	int n = 0;


	kit.otd_sock = otd_sock_init();
	if (kit.otd_sock > 0) 
	{
		kit.pollfds[kit.count_pollfds].fd = kit.otd_sock;
		kit.pollfds[kit.count_pollfds].events = POLLIN;
		printf(  "OTD sockfd: %d\n", kit.pollfds[kit.count_pollfds].fd);
		kit.count_pollfds++;
	}


	/* send to cloud timer */
	kit.timerfd2 = timer_setup();
	if (kit.timerfd2 <= 0) 
	{
		printf(  "timer_setup() fail, quit.\n");
		goto done;
	}
	
	kit.pollfds[kit.count_pollfds].fd = kit.timerfd2;
	kit.pollfds[kit.count_pollfds].events = POLLIN;
	printf(  "Timer fd: %d, interval: %d ms\n",
	kit.pollfds[kit.count_pollfds].fd, TIMER_INTERVAL_REPORT);
	
	
	kit.count_pollfds++;
	for (n = 0; n < kit.count_pollfds; n++)
		printf(  "pollfds[%d]: %d\n", n, kit.pollfds[n].fd);

	timer_start(kit.timerfd2, TIMER_INTERVAL_REPORT, TIMER_INTERVAL_REPORT);


	/* --------------- launch ----------------- */
	n = 0;
	int i;
	while (n >= 0 ) 
	{
		n = poll(kit.pollfds, kit.count_pollfds, POLL_TIMEOUT);
		if (n < 0)
		{
			perror("poll");
			continue;
		}
		if (n == 0) 
		{
			/* printf("poll timeout\n"); */
			continue;
		}
		/* printf("poll returns %d, count_pollfds: %d\n", n, kit.count_pollfds); */
		for (i = 0; i < kit.count_pollfds && n > 0; i++) 
		{
			if (kit.pollfds[i].revents & POLLIN) 
			{
				if (kit.pollfds[i].fd == kit.timerfd2)
					timer_handler2(kit.timerfd2);
				else if (kit.pollfds[i].fd == kit.otd_sock)
					otd_recv_handler(kit.otd_sock);
				n--;
				/* printf("POLLIN fd: %d\n", kit.pollfds[i].fd); */
			} else if (kit.pollfds[i].revents & POLLOUT) 
			{
				if (kit.pollfds[i].fd == kit.otd_sock)
					printf(  "POLLOUT fd: %d\n", kit.otd_sock);
				n--;
				/* printf("POLLOUT fd: %d\n", kit.pollfds[i].fd); */
			}
			else if (kit.pollfds[i].revents & (POLLNVAL | POLLHUP | POLLERR)) 
			{
				int j = i;

				printf( 
					   "POLLNVAL | POLLHUP | POLLERR fd: pollfds[%d]: %d, revents: 0x%08x\n",
					   i, kit.pollfds[i].fd, kit.pollfds[i].revents);

				if (kit.pollfds[i].fd == kit.otd_sock) {
					otd_close_retry();
					n--;
					continue;
				}

				close(kit.pollfds[i].fd);
				while (j < kit.count_pollfds) {
					kit.pollfds[j] = kit.pollfds[j + 1];
					j++;
				}
				kit.count_pollfds--;
				n--;
			}
		}
	}

done:
	if (kit.otd_sock > 0)
	{
		close(kit.otd_sock);
	}
	if (kit.timerfd2 > 0)
	{
		close(kit.timerfd2);
	}
}

