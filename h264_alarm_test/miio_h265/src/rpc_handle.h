#ifndef __RPC_HANDLE_H_
#define __RPC_HANDLE_H_

#include <stdbool.h>
#include <ev.h>

#define OT_MAX_PAYLOAD			1024
#define BUFFER_MAX  4096
#define OTD_IP      "127.0.0.1"
#define OTD_PORT    54320

#define MSG_ID_MIN			1
#define MSG_ID_MAX			10000000

struct kit_struct {
	int otd_sock;
	bool force_exit;
};

int otd_sock_init(void);
int get_otd_sockfd(void);
void otd_recv_handle(struct ev_loop *loop, struct ev_io *watcher, int revents);
void create_delete_process(void);
#endif /* __RPC_HANDLE_H_ */
