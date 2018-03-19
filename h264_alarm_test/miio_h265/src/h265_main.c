#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <ev.h>
#include <shbf/receiver-ev.h>
#include "h265_main.h"
#include "rpc_handle.h"
#include "curl.h"


pthread_mutex_t nvram_lock = PTHREAD_MUTEX_INITIALIZER;

struct ev_loop *loop;
struct ev_async async;
struct ev_io ot_client;
struct ev_periodic second_tick;
mortox_client *xc = NULL;


void sighandler(int signal)
{
	ev_async_send(loop, &async);
}

void sig_dump(int signal)
{
	dumpinfo_record_buf("signal", &video_prerecord_buf, &video_alarm_buf);
}

void stop_ev_loop(void)
{
	ev_break(loop, EVBREAK_ALL);
}

static void second_tick_handle(struct ev_loop *loop, ev_io * w, int revents)
{
	if (get_otd_sockfd() < 0) {
		int otd_sock = otd_sock_init();
		if (otd_sock < 0)
			return;
		ev_io_init(&ot_client, otd_recv_handle, otd_sock, EV_READ);
		ev_io_start(loop, &ot_client);
	}
}

void set_cb(xresponse * r, void *data)
{
	if (xresponse_message_type(r) != X_CONTROL_SET) {
		log_info("Recv type error. not the set value.\n");
		return;
	}

	nvram_param_t *nvram_params = (nvram_param_t *) data;
	if (xresponse_status(r) == 0) {
		nvram_params->result = 1;
		log_info("Set %s sucess.\n", nvram_params->key);
	} else {
		nvram_params->result = 0;
		log_info("Set %s faild.\n", nvram_params->key);
	}
}

void get_cb(xresponse * r, void *data)
{
	nvram_param_t *nvram_params = (nvram_param_t *) data;
	xdata *gdata = NULL;
	if (!xcontrol_get_response(r, &gdata)) {
		log_err("Get %s faild, use default value in config.txt file in v4\n", nvram_params->key);
		nvram_params->result = 1;
		if (memcmp(nvram_params->key, "sdcard_status", 13) == 0) {
			memcpy(nvram_params->value, "0", 1);
		} else if (memcmp(nvram_params->key, "motion_record", 13) == 0) {
			memcpy(nvram_params->value, "off", 3);
		} else if (memcmp(nvram_params->key, "motion_alarm", 12) == 0) {
			memcpy(nvram_params->value, "[0,1,0,0,0,0,1]", 15);
		} else if (memcmp(nvram_params->key, "power", 5) == 0) {
			memcpy(nvram_params->value, "on", 2);
		} else if (strcmp(nvram_params->key, "last_alarm_time") == 0) {
			memcpy(nvram_params->value, "0", 1);
		}
	} else {
		nvram_params->result = 1;
		if (gdata->type == X_TYPE_STRING) {
			memcpy(nvram_params->value, gdata->store.d_string.value, gdata->store.d_string.length);
			log_info("nvram get %s is %s\n", nvram_params->key, nvram_params->value);
		} else {
			nvram_params->result = 0;
			log_err("Get ok but the value type is not right\n");
		}
		xdata_free(gdata);
	}
}

void timout_cb(xresponse * r, void *data)
{
	nvram_param_t *nvram_params = (nvram_param_t *) data;
	log_info("%s create group timeout.\n", nvram_params->key);
}

	/*X_TYPE_INT32, [>*<Represents type of an int32_t value <] */
	/*X_TYPE_UINT32, [>*<Represents type of an uint32_t value <] */
	/*X_TYPE_INT64, [>*<Represents type of a int64_t value <] */
	/*X_TYPE_UINT64, [>*<Represents type of a uint64_t value <] */
	/*X_TYPE_FLOAT, [>*<Represents type of a float value <] */
	/*X_TYPE_DOUBLE, [>*<Represents type of a double value <] */
	/*X_TYPE_BOOLEAN, [>*<Represents type of a boolean value <] */
	/*X_TYPE_UNSET, [>*<Represents unknown type for values <] */
	/*X_TYPE_MAX */
/*} xdata_type;*/

void nvram_set(nvram_param_t * nvram_params, char *value)	//layer is nvram or mem
{
	pthread_mutex_lock(&nvram_lock);
	if (libmortox_set_value
	    (xc, "nvram", NULL, nvram_params->key, X_TYPE_STRING, value, set_cb, timout_cb, 5, nvram_params) == false) {
		log_err("send set value cmd fail.\n");
	}
	pthread_mutex_unlock(&nvram_lock);
}

void nvram_get(nvram_param_t * nvram_params)	//layer is nvram or mem
{
	pthread_mutex_lock(&nvram_lock);
	if (libmortox_get_value(xc, "nvram", NULL, nvram_params->key, get_cb, timout_cb, 5, nvram_params) == false) {
		log_err("send get value cmd fail.\n");
	}
	pthread_mutex_unlock(&nvram_lock);
}

void nvram_commit()
{
	libmortox_sync(xc, "nvram", NULL, NULL, 0, NULL);
}

int main(int argc, char *argv[])
{
	int otd_sock;

	loop = EV_DEFAULT;
	struct sigaction act = {
		.sa_handler = sighandler
	};
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	signal(SIGUSR1, sig_dump);

	otd_sock = otd_sock_init();
	if (otd_sock < 0) {
		log_err("otd_sock init error\n");
		return -1;
	}
	mcurl_init();
	xc = libmortox_config_open(NULL);
	if (!xc) {
		log_err("libmortox_open faild.\n");
		exit(1);
	}

	get_motion_params();
	nvram_get_power_value();
	create_sdcheck_thread();
	create_delete_process();
	mp4_queue_init_pre(&audio_prerecord_buf, &video_prerecord_buf);
	mp4_queue_init_alarm(&audio_alarm_buf, &video_alarm_buf);
	create_record_thread();
	create_stream_thread();
#if 0
	shbf_rcv_global_init();
	main_stream_recv = shbfev_rcv_create(loop, VIDEO_MAINSTREAM_CONTROLLER);
	shbfev_rcv_event(main_stream_recv, SHBF_RCVEV_SHARED_BUFFER, SHBF_EVT_HANDLE(main_stream_recv_callback), NULL);
	shbfev_rcv_event(main_stream_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "main_stream");
	shbfev_rcv_start(main_stream_recv);

	sub_stream_recv = shbfev_rcv_create(loop, VIDEO_SUBSTREAM_CONTROLLER);
	shbfev_rcv_event(sub_stream_recv, SHBF_RCVEV_SHARED_BUFFER, SHBF_EVT_HANDLE(sub_stream_recv_callback), NULL);
	shbfev_rcv_event(sub_stream_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "sub_stream");
	shbfev_rcv_start(sub_stream_recv);

	audio_stream_recv = shbfev_rcv_create(loop, AI_CONTROLLER);
	shbfev_rcv_event(audio_stream_recv, SHBF_RCVEV_SHARED_BUFFER, SHBF_EVT_HANDLE(audio_stream_recv_callback),
			 NULL);
	shbfev_rcv_event(audio_stream_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "audio_stream");
	shbfev_rcv_start(audio_stream_recv);

	md_event_recv = shbfev_rcv_create(loop, MD_CONTROLLER);
	shbfev_rcv_event(md_event_recv, SHBF_RCVEV_MESSAGE, SHBF_EVT_HANDLE(md_event_callback), NULL);
	shbfev_rcv_event(md_event_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "md_event_stream");
	shbfev_rcv_start(md_event_recv);

	snap_shot_recv = shbfev_rcv_create(loop, JPEG_CONTROLLER);
	shbfev_rcv_event(snap_shot_recv, SHBF_RCVEV_SHARED_BUFFER, SHBF_EVT_HANDLE(snapshot_callback), NULL);
	shbfev_rcv_event(snap_shot_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "snapshot_stream");
	shbfev_rcv_start(snap_shot_recv);
#endif

	ev_io_init(&ot_client, otd_recv_handle, otd_sock, EV_READ);
	ev_io_start(loop, &ot_client);

	ev_periodic_init(&second_tick, second_tick_handle, 0., 5., 0);
	ev_periodic_start(loop, &second_tick);

	ev_async_init(&async, stop_ev_loop);
	ev_async_start(loop, &async);

	ev_run(loop, 0);
#if 0
	free_mp4_queue(&audio_prerecord_buf, &video_prerecord_buf);
	free_alarm_queue(&audio_alarm_buf, &video_alarm_buf);
#endif
	thread_record_mp4_exit();
#if 0
	shbfev_rcv_destroy(main_stream_recv);
	shbfev_rcv_destroy(sub_stream_recv);
	shbfev_rcv_destroy(audio_stream_recv);
	shbfev_rcv_destroy(md_event_recv);
	shbfev_rcv_destroy(snap_shot_recv);

	shbf_rcv_global_exit();
#endif
	ev_default_destroy();
	libmortox_close(xc);
	mcurl_exit();
	log_info("h265 record process exit!!!\n");
	exit(0);
}
