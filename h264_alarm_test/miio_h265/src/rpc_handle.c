#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <json-c/json.h>
#include <miio/net.h>

#include "json.h"
#include "rpc_handle.h"
#include "h265_g711_mp4.h"

#define OT_ACK_SUC_TEMPLATE "{\"id\":%d,\"result\":[\"OK\"]}"
#define OT_ACK_ERR_TEMPLATE "{\"id\":%d,\"error\":{\"code\":-33020,\"message\":\"%s\"}}"
#define OT_UP_PWD_TEMPLATE "{\"id\":%d,\"method\":\"props\",\"params\":{\"p2p_id\":\"%s\",\"p2p_password\":\"%s\", \"p2p_checktoken\":\"%s\"}}"
#define OT_UP_SYS_STATUS_TEMPLATE "{\"id\":%d,\"method\":\"props\",\"params\":{\"power\":\"%s\"}}"

static int pipefd[2];
struct kit_struct kit = {
	.otd_sock = 0
};

static char *Interesting_keys[] = {
	"deleteVideo",
	"deleteSaveVideo",
	"saveVideo",
	"set_motion_record",
	"setAlarmConfig",
	"getAlarmConfig",
	"sd_storge",
	"sd_format",
	"sd_umount",
	"miIO.event",
	"upload_video"
};

uint64_t get_micro_second(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t) ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int rand_range(int low, int high)
{
	int limit = high - low;
	int divisor = RAND_MAX / (limit + 1);
	int retval;

	do {
		retval = rand() / divisor;
	} while (retval > limit);

	return retval + low;
}

int generate_random_id(void)
{
	srand(get_micro_second());

	return rand_range(MSG_ID_MIN, MSG_ID_MAX);
}

static void register_keys(int sockfd)
{
	char buf[128];
	char *reg_template = "{\"method\":\"register\",\"key\":\"%s\"}";
	int i, keys_num;

	keys_num = sizeof(Interesting_keys) / sizeof(Interesting_keys[0]);
	log_info("keys_num is %d\n", keys_num);

	for (i = 0; i < keys_num; i++) {
		snprintf(buf, sizeof(buf), reg_template, Interesting_keys[i]);
		send(sockfd, buf, strlen(buf), 0);
	}
}

int otd_sock_init(void)
{
	struct sockaddr_in serveraddr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		log_err("Create socket error: %m\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(OTD_IP);
	serveraddr.sin_port = htons(OTD_PORT);

	if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
		log_err("Connect to otd error: %s:%d\n", OTD_IP, OTD_PORT);
		return -1;
	}
	register_keys(sockfd);
	kit.otd_sock = sockfd;
	return sockfd;
}

static int get_url_id = 0;
int send_to_get_url(void)
{
	char *token = NULL;
	token = get_p2p_token(token);
	if (!token) {
		log_err("%s: get token fail %s\n", __func__, token);
		return -1;
	}
	get_url_id = generate_random_id();
	char ackbuf[OT_MAX_PAYLOAD];
	memset(ackbuf, 0, sizeof(ackbuf));

	struct json_object *send_object = json_object_new_object();
	json_object_object_add(send_object, "id", json_object_new_int(get_url_id));
	json_object_object_add(send_object, "method", json_object_new_string("_sync.gen_presigned_url"));
	struct json_object *data_object = json_object_new_object();
	json_object_object_add(data_object, "suffix", json_object_new_string("mp4"));
	json_object_object_add(data_object, "p2p_checktoken", json_object_new_string(token));
	json_object_object_add(send_object, "params", data_object);
	sprintf(ackbuf, "%s", json_object_to_json_string_ext(send_object, JSON_C_TO_STRING_NOZERO));

	json_object_put(send_object);
	send(kit.otd_sock, ackbuf, strlen(ackbuf), 0);
	log_debug("get url json str is %s\n", ackbuf);
	if (token)
		free(token);
	return 0;
}

static int get_pic_url_id = 0;
int request_pic_url(void)
{
	char *token = NULL;
	token = get_p2p_token(token);
	if (!token) {
		log_err("%s: get token fail %s\n", __func__, token);
		return -1;
	}
	get_pic_url_id = generate_random_id();
	char ackbuf[OT_MAX_PAYLOAD];
	memset(ackbuf, 0, sizeof(ackbuf));

	struct json_object *send_object = json_object_new_object();
	json_object_object_add(send_object, "id", json_object_new_int(get_pic_url_id));
	json_object_object_add(send_object, "method", json_object_new_string("_sync.gen_presigned_url"));
	struct json_object *data_object = json_object_new_object();
	json_object_object_add(data_object, "suffix", json_object_new_string("jpeg"));
	json_object_object_add(data_object, "p2p_checktoken", json_object_new_string(token));
	json_object_object_add(send_object, "params", data_object);
	sprintf(ackbuf, "%s", json_object_to_json_string_ext(send_object, JSON_C_TO_STRING_NOZERO));

	json_object_put(send_object);
	send(kit.otd_sock, ackbuf, strlen(ackbuf), 0);
	log_debug("get jpeg url json str is %s\n", ackbuf);
	if (token)
		free(token);
	return 0;
}

static int upload_video_process(int id, int sock, char *ackbuf)
{
	if (NULL == ackbuf)
		return -1;

	if (get_poweroff_value()) {
		sprintf(ackbuf, OT_ACK_SUC_TEMPLATE, id);
		send(sock, ackbuf, strlen(ackbuf), 0);
		set_upload_trigger(1, 0);
		set_upload_video_value(1);
		get_motionalarm_value();
		int ret = is_motion_alarm_done();
		if (1 == ret) {
			set_upload_trigger(1, 0);
		} else {
			set_motion_alarm_done(1);
			set_upload_trigger(0, 1);
			set_alarm_start_flag(1);
		}
	} else {
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-33066,\"message\":\"%s\"}}", id, "camera is sleep!");
		send(sock, ackbuf, strlen(ackbuf), 0);
		log_debug("upload json is %s\n", ackbuf);
	}
	return 0;
}

int upload_motion_alarm(UploadAlarmData_t * params)
{
	if (NULL == params)
		return -1;

	char *token = NULL;
	token = get_p2p_token(token);
	if (!token) {
		log_err("%s: get token fail %s\n", __func__, token);
		return -1;
	}

	int id = generate_random_id();
	char ackbuf[OT_MAX_PAYLOAD];
	memset(ackbuf, 0, sizeof(ackbuf));
	sprintf(ackbuf,
		"{\"id\":%u, \"method\":\"event.motion\", \"params\":[\"%s\",\"%s\",\"%s\",\"%s\",\"%d%d\",\"%d\",\"%s\"]}",
		id, params->object_name, params->pwd, params->pic_object_name, params->pic_pwd, params->MD_trig,
		params->upload_cmd_trig, params->push_flag, token);

	send(kit.otd_sock, ackbuf, strlen(ackbuf), 0);
	if (token)
		free(token);
	log_debug("motion alarm json str is %s\n", ackbuf);
	return 0;
}

int upload_url_parse(const char *msg, int flag)
{
	if (NULL == msg)
		return -1;
	struct json_object *new_obj = NULL;
	json_bool ret = 0;
	new_obj = json_tokener_parse(msg);
	if (NULL == new_obj) {
		log_err("%s: Not in json format: %s\n", __func__, msg);
		return -1;
	}

	struct json_object *props = NULL;
	ret = json_object_object_get_ex(new_obj, "result", &props);
	if (!ret) {
		log_err("%s: set_props error\n", __func__);
		json_object_put(new_obj);
		return -1;
	}

	struct json_object *mp4;
	if (flag) {
		ret = json_object_object_get_ex(props, "mp4", &mp4);
	} else {
		ret = json_object_object_get_ex(props, "jpeg", &mp4);
	}
	if (!ret) {
		log_err("%s:  error\n", __func__);
		json_object_put(new_obj);
		return -1;
	}

	struct json_object *url_object = NULL;
	ret = json_object_object_get_ex(mp4, "url", &url_object);
	if (!ret) {
		log_err("%s: url error\n", __func__);
		json_object_put(new_obj);
		return -1;
	}

	struct json_object *obj_name = NULL;
	ret = json_object_object_get_ex(mp4, "obj_name", &obj_name);
	if (!ret) {
		log_err("%s: obj_name error\n", __func__);
		json_object_put(new_obj);
		return -1;

	}

	struct json_object *pwd = NULL;
	ret = json_object_object_get_ex(mp4, "pwd", &pwd);
	if (!ret) {
		log_err("%s: pwd error\n", __func__);
		json_object_put(new_obj);
		return -1;
	}
	set_upload_url(json_object_get_string(url_object), flag);
	set_upload_object_name(json_object_get_string(obj_name), flag);
	set_upload_pwd(json_object_get_string(pwd), flag);

	json_object_put(new_obj);
	return 0;
}

static int otd_set_props(const char *msg, int id, char *ackbuf, char *set_prop)
{
	if (NULL == msg || NULL == ackbuf)
		return -1;
	struct json_object *new_obj;
	json_bool ret = 0;
	new_obj = json_tokener_parse(msg);
	if (NULL == new_obj) {
		log_err("%s: Not in json format: %s\n", __func__, msg);
		return -1;
	}

	struct json_object *props;
	ret = json_object_object_get_ex(new_obj, "params", &props);
	if (!ret) {
		log_err("%s: set_props error\n", __func__);
		json_object_put(new_obj);
		return -1;
	}

	json_type type = json_object_get_type(props);
	if (type != json_type_array) {
		log_err("%s: params json type error\n", __func__);
		json_object_put(new_obj);
		return -1;
	}

	const char *str;
	struct json_object *send_object = json_object_new_object();
	json_object_object_add(send_object, "id", json_object_new_int(id));

	nvram_param_t params = { 0 };
	char pair[256] = { 0 };
	char output[256] = { 0 };
	if (0 == strcmp(set_prop, "motion_record")) {
		struct json_object *prop;
		prop = json_object_array_get_idx(props, 0);
		str = json_object_get_string(prop);
		params.key = "motion_record";
		params.type = X_TYPE_STRING;
		nvram_set(&params, str);
		if (params.result) {
			json_object_object_add(send_object, "result", json_object_new_int(1));
			log_info("set %s %s ok!\n", set_prop, str);
		} else {
			json_object_object_add(send_object, "result", json_object_new_int(0));
		}
		sprintf(ackbuf, "%s", json_object_to_json_string(send_object));
	} else if (0 == strcmp(set_prop, "motion_alarm")) {
		str = json_object_to_json_string_ext(props, JSON_C_TO_STRING_NOZERO);
		params.key = "motion_alarm";
		params.type = X_TYPE_STRING;
		nvram_set(&params, str);
		if (params.result) {
			json_object_object_add(send_object, "result", json_object_new_int(1));
			log_info("set %s %s ok!\n", set_prop, str);
		} else {
			json_object_object_add(send_object, "result", json_object_new_int(0));
		}
	} else if (0 == strcmp(set_prop, "motion_alarm_get")) {
		params.key = "motion_alarm";
		nvram_get(&params);
		params.type = X_TYPE_STRING;
		log_info("motion_alarm is %s\n", params.value);
		struct json_object *result_array = json_tokener_parse(params.value);
		json_type type = json_object_get_type(result_array);
		if (type != json_type_array) {
			log_err("%s: params json type error\n", __func__);
		}
		json_object_object_add(send_object, "result", result_array);
		sprintf(ackbuf, "%s", json_object_to_json_string(send_object));
	} else if (0 == strcmp(set_prop, "get_motion_region")) {
		/*do_nvram_get("motion_region", output, sizeof(output), 0); */
		struct json_object *result_array = json_tokener_parse(output);
		json_type type = json_object_get_type(result_array);
		if (type != json_type_array) {
			log_err("%s: params json type error\n", __func__);
		}
		json_object_object_add(send_object, "result", result_array);
		sprintf(ackbuf, "%s", json_object_to_json_string_ext(send_object, JSON_C_TO_STRING_NOZERO));
		log_debug("get_motion_region: %s\n", ackbuf);
	} else if (0 == strcmp(set_prop, "motion_region")) {
		str = json_object_to_json_string_ext(props, JSON_C_TO_STRING_NOZERO);
		snprintf(pair, sizeof(pair), "%s=%s", set_prop, str);
		log_debug("set_motion_region: %s\n", pair);
		json_object_object_add(send_object, "result", json_object_new_int(1));

		/*do_nvram_set(pair, 0); */
		/*do_nvram_get(set_prop, output, sizeof(output), 0); */
		if (0 == strcmp(str, output)) {
			json_object_object_add(send_object, "result", json_object_new_int(1));
			log_info("set %s %s ok!\n", set_prop, str);
		} else {
			json_object_object_add(send_object, "result", json_object_new_int(0));
			log_err("set %s error: str: %s, output: %s\n", set_prop, str, output);
		}

		int size = json_object_array_length(props);
		int *region_array;
		int i;

		region_array = malloc(sizeof(int) * size);
		if (region_array == NULL) {
			log_err("malloc error: %s(): %m\n", __func__);
			goto err_out;
		}

		for (i = 0; i < size; i++) {
			region_array[i] = json_object_get_int(json_object_array_get_idx(props, i));
		}

		/*update_interesting_region(region_array, size); */
		free(region_array);
	} else if (0 == strcmp(set_prop, "motion_detection_enable")) {
		struct json_object *prop;
		prop = json_object_array_get_idx(props, 0);
		str = json_object_get_string(prop);
		snprintf(pair, sizeof(pair), "%s=%s", set_prop, str);
		/*do_nvram_set(pair, 0); */
		/*do_nvram_get(set_prop, output, sizeof(output), 0); */
		if (0 == strcmp(str, output)) {
			json_object_object_add(send_object, "result", json_object_new_int(1));
			log_info("set %s %s ok!\n", set_prop, str);
		} else {
			json_object_object_add(send_object, "result", json_object_new_int(0));
		}
		sprintf(ackbuf, "%s", json_object_to_json_string(send_object));

		if (strncmp(str, "off", strlen(str)) == 0) {
			/*update_interesting_region_disable(); */
		} else if (strncmp(str, "on", strlen(str)) == 0) {
			/*int md_array[MAX_SUBREGION_NUM] = { 0 }; */
			/*int get_md_ret = get_md_array(md_array); */
			/*if (get_md_ret < 0) { */
			/*log_err("get motion detection array fail, write default.\n"); */
			/*get_md_ret = update_interesting_region_default(); */
			/*} else { */
			/*get_md_ret = update_interesting_region(md_array, MAX_SUBREGION_NUM); */
			/*} */
			/*if (get_md_ret != 0) { */
			/*log_err("Error to do set_interesting_region!\n"); */
			/*goto err_out; */
			/*} */
		}
	}

err_out:
	json_object_put(new_obj);
	json_object_put(send_object);
	return 0;

}

static int otd_deleteVideo_process(const char *msg, int flag, int pipefd)
{
	if (NULL == msg)
		return -1;
	int msg_len = strlen(msg);
	if (msg_len > BUFFER_MAX / 2 - 1) {
		log_err("del msg len is too big! msg_len is %d\n", msg_len);
		return -1;
	}
	char pipe_msg[BUFFER_MAX / 2] = { 0 };
	pipe_msg[0] = (char)flag;

	memcpy(&pipe_msg[1], msg, msg_len);

	if (write(pipefd, pipe_msg, sizeof(pipe_msg)) != sizeof(pipe_msg)) {
		perror("pipe msg write");
		return -1;
	} else {
		return 0;
	}
}

static int otd_saveVideo_process(const char *msg)
{
	if (NULL == msg)
		return -1;
	struct json_object *new_obj;

	new_obj = json_tokener_parse(msg);
	if (NULL == new_obj) {
		log_err("%s: Not in json format: %s\n", __func__, msg);
		return -1;
	}

	struct json_object *timestamp;
	if (!json_object_object_get_ex(new_obj, "params", &timestamp)) {
		log_err("%s: get timestamp error\n", __func__);
		json_object_put(new_obj);
		return -1;
	}

	json_type type = json_object_get_type(timestamp);
	if (type != json_type_array) {
		log_err("%s: params json type error\n", __func__);
		json_object_put(new_obj);
		return -1;
	}

	int len = json_object_array_length(timestamp);
	time_t time_stamp;
	json_object *timestamp_val = NULL;
	int i = 0;
	for (i = 0; i < len; i++) {
		timestamp_val = json_object_array_get_idx(timestamp, i);
		time_stamp = json_object_get_int(timestamp_val);
		log_debug("save timestamp_val=%u\n", time_stamp);
		save_file_timestamp(time_stamp);
	}
	json_object_put(new_obj);
	return 0;
}

static int sdcard_format_process(int id, int sock, char *ackbuf)
{
	if (NULL == ackbuf)
		return -1;

	int ret = 0;
	ret = get_format_flag();
	if (1 != ret) {
		set_format_flag(1);
		if (get_sdcard_on()) {
			sprintf(ackbuf, OT_ACK_SUC_TEMPLATE, id);
			exec_format_thread();
		} else {
			sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2003,\"message\":\"%s\"}}", id,
				"The sdcard don't exist!");
		}
	} else {
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2000,\"message\":\"%s\"}}", id,
			"The sdcard is formating!");
	}

	send(sock, ackbuf, strlen(ackbuf), 0);
	return 0;
}

static int sdcard_umount_process(int id, int sock, char *ackbuf)
{
	if (NULL == ackbuf)
		return -1;

	int ret = 0;
	ret = get_format_flag();
	if (1 != ret) {
		if (get_sdcard_on()) {
			if (is_sdcard_avail() || 2 == get_sdcard_status()) {
				sdcard_umount(id, ackbuf);
				sdcard_check(1);
			} else {
				sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2002,\"message\":\"%s\"}}", id,
					"The sdcard status error");
			}
		} else {
			sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2003,\"message\":\"%s\"}}", id,
				"The sdcard don't exist!");
		}
	} else {
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2000,\"message\":\"%s\"}}", id,
			"The sdcard is formating!");
	}

	send(sock, ackbuf, strlen(ackbuf), 0);
	return 0;
}

static int msg_dispatcher(const char *msg, int len, int sock, int pipefd)
{
	char ackbuf[OT_MAX_PAYLOAD];
	int ret = -1, id = 0;
	bool sendack = false;
	memset(ackbuf, 0, sizeof(ackbuf));
	log_debug("%s, msg: %s, strlen: %d, len: %d\n", __func__, msg, (int)strlen(msg), len);

	//get id
	ret = json_verify_get_int(msg, "id", &id);
	if (ret < 0) {
		return ret;
	}
	//receving result
	if (json_verify_method(msg, "result") == 0) {
		sendack = false;
		if (id == get_url_id) {
			upload_url_parse(msg, 1);
		} else if (id == get_pic_url_id) {
			upload_url_parse(msg, 0);
		}
		return 0;
	}
	if (json_verify_method_value(msg, "method", "deleteVideo", json_type_string) == 0) {
		log_debug("Got deleteVideo...\n");
		sprintf(ackbuf, OT_ACK_SUC_TEMPLATE, id);
		ret = send(sock, ackbuf, strlen(ackbuf), 0);
		otd_deleteVideo_process(msg, 1, pipefd);
	} else if (json_verify_method_value(msg, "method", "deleteSaveVideo", json_type_string) == 0) {
		log_debug("Got deleteSaveVideo...\n");
		sprintf(ackbuf, OT_ACK_SUC_TEMPLATE, id);
		ret = send(sock, ackbuf, strlen(ackbuf), 0);
		otd_deleteVideo_process(msg, 0, pipefd);
	} else if (json_verify_method_value(msg, "method", "saveVideo", json_type_string) == 0) {
		log_debug("Got saveVideo...\n");
		if (is_sdcard_avail()) {
			sprintf(ackbuf, OT_ACK_SUC_TEMPLATE, id);
			ret = send(sock, ackbuf, strlen(ackbuf), 0);
			otd_saveVideo_process(msg);
		} else {
			sprintf(ackbuf, OT_ACK_ERR_TEMPLATE, id, "The sdcard don't exist!");
			ret = send(sock, ackbuf, strlen(ackbuf), 0);
		}
	} else if (json_verify_method_value(msg, "method", "set_motion_record", json_type_string) == 0) {
		log_debug("Got set_motion_record...\n");
		otd_set_props(msg, id, ackbuf, "motion_record");
		ret = send(sock, ackbuf, strlen(ackbuf), 0);
		nvram_commit();
		get_motionrecord_value();
	} else if (json_verify_method_value(msg, "method", "setAlarmConfig", json_type_string) == 0) {
		log_debug("Got set_motion_alarm...\n");
		otd_set_props(msg, id, ackbuf, "motion_alarm");
		sprintf(ackbuf, OT_ACK_SUC_TEMPLATE, id);
		ret = send(sock, ackbuf, strlen(ackbuf), 0);
		nvram_commit();
		get_motionalarm_value();
	} else if (json_verify_method_value(msg, "method", "getAlarmConfig", json_type_string) == 0) {
		log_debug("Got motion_alarm...\n");
		otd_set_props(msg, id, ackbuf, "motion_alarm_get");
		ret = send(sock, ackbuf, strlen(ackbuf), 0);
	} else if (json_verify_method_value(msg, "method", "sd_storge", json_type_string) == 0) {
		log_debug("Got sd_storge cmd...\n");
		sdcard_storge_process(id, sock, ackbuf);
	} else if (json_verify_method_value(msg, "method", "sd_format", json_type_string) == 0) {
		log_debug("Got sd_format cmd...\n");
		sdcard_format_process(id, sock, ackbuf);
	} else if (json_verify_method_value(msg, "method", "sd_umount", json_type_string) == 0) {
		log_debug("Got sd_umount cmd...\n");
		sdcard_umount_process(id, sock, ackbuf);
	} else if (json_verify_method_value(msg, "method", "upload_video", json_type_string) == 0) {
		upload_video_process(id, sock, ackbuf);
	}
	if (ret < 0) {
		perror("net send fail");
	}

	if (sendack)
		ret = send(sock, ackbuf, strlen(ackbuf), 0);

	return ret;
}

/* In some cases, we might receive several accumulated json RPC, we need to split these json.
 * E.g.:
 *   {"count":1,"stack":"sometext"}{"count":2,"stack":"sometext"}{"count":3,"stack":"sometext"}
 *
 * return the length we've consumed, -1 on error
 */
static int otd_recv_handle_one(int sockfd, char *msg, int msg_len, int pipefd)
{
	struct json_tokener *tok;
	struct json_object *json;
	int ret = 0;

	log_debug("%s(), sockfd: %d, msg: %.*s, length: %d bytes\n", __func__, sockfd, msg_len, msg, msg_len);
	if (json_verify(msg) < 0)
		return -1;

	/* split json if multiple */
	tok = json_tokener_new();
	while (msg_len > 0) {
		char *tmpstr;
		int tmplen;

		json = json_tokener_parse_ex(tok, msg, msg_len);
		if (json == NULL) {
			log_warning("%s(), token parse error msg: %.*s, length: %d bytes\n",
				    __func__, msg_len, msg, msg_len);
			json_tokener_free(tok);
			return ret;
		}

		tmplen = tok->char_offset;
		tmpstr = malloc(tmplen + 1);
		if (tmpstr == NULL) {
			log_warning("%s(), malloc error\n", __func__);
			json_tokener_free(tok);
			json_object_put(json);
			return -1;
		}
		memcpy(tmpstr, msg, tmplen);
		tmpstr[tmplen] = '\0';

		msg_dispatcher((const char *)tmpstr, tmplen, sockfd, pipefd);

		free(tmpstr);
		json_object_put(json);
		ret += tok->char_offset;
		msg += tok->char_offset;
		msg_len -= tok->char_offset;
	}

	json_tokener_free(tok);

	return ret;
}

int get_otd_sockfd(void)
{
	return kit.otd_sock;
}

void *delete_process_thread(void *args)
{
	struct pollfd fds[1];
	fds[0].fd = *(int *)args;
	fds[0].events = POLLIN;

	int ret = 0;
	ret = prctl(PR_SET_NAME, "mstar_delete_process\0", NULL, NULL, NULL);
	if (ret != 0) {
		log_err("prctl setname failed\n");
	}

	char data[BUFFER_MAX / 2] = { 0 };
	for (;;) {
		// wait for an event from one of our producer(s)
		if (poll(fds, 1, -1) != 1) {
			perror("poll");
			exit(1);
		}
		if (fds[0].revents != POLLIN) {
			fprintf(stderr, "unexpected poll revents: %hd\n", fds[0].revents);
			continue;
		}
		if (read(fds[0].fd, data, sizeof(data)) != sizeof(data)) {
			perror("read");
		} else {
			struct json_object *new_obj;
			struct json_object *timestamp;
			time_t time_stamp;
			int i = 0;
			int len = 0;

			new_obj = json_tokener_parse(&data[1]);
			if (NULL == new_obj) {
				log_err("%s: Not in json format: %s\n", __func__, data);
				continue;
			}

			if (!json_object_object_get_ex(new_obj, "params", &timestamp)) {
				log_err("%s: get timestamp error\n", __func__);
				json_object_put(new_obj);
				continue;
			}

			json_type type = json_object_get_type(timestamp);
			if (type != json_type_array) {
				log_err("%s: params json type error\n", __func__);
				json_object_put(new_obj);
				continue;
			}

			len = json_object_array_length(timestamp);
			json_object *timestamp_val = NULL;

			for (i = 0; i < len; i++) {
				timestamp_val = json_object_array_get_idx(timestamp, i);
				time_stamp = json_object_get_int(timestamp_val);
				rm_file_timestamp(time_stamp);
				CheckAndDelEmptyDir(&time_stamp);
			}
			json_object_put(new_obj);
		}
	}
	pthread_exit(0);
}

void create_delete_process(void)
{
	int ret;
	if (pipe(pipefd) != 0) {
		perror("pipe");
		exit(1);
	}
	pthread_t del_thread_id;
	ret = pthread_create(&del_thread_id, NULL, &delete_process_thread, (void *)(&pipefd[0]));
	if (ret < 0) {
		printf("delete video create failed ret[%d]\n", ret);
		exit(1);
	} else {
		pthread_detach(del_thread_id);
	}
}

void otd_recv_handle(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	char buf[BUFFER_MAX] = { 0 };
	ssize_t count;
	int left_len = 0, ret = 0;

	if (EV_ERROR & revents) {
		log_debug("error event in read");
		return;
	}

	memset(buf, 0, BUFFER_MAX);
	while (true) {
		count = recv(watcher->fd, buf + left_len, BUFFER_MAX - left_len, MSG_DONTWAIT);
		if (count > 0) {
			ret = otd_recv_handle_one(watcher->fd, buf, count + left_len, pipefd[1]);
			if (ret < 0) {
				log_warning("otd_recv_handle_one return %d\n", ret);
				return;
			}
			left_len = count + left_len - ret;
			memmove(buf, buf + ret, left_len);
		} else if (count == 0) {
			log_warning("peer %d might closing,now, start retry\n", watcher->fd);
			ev_io_stop(loop, watcher);
			close(watcher->fd);
			kit.otd_sock = -1;
		} else {
			break;
		}
	}
}
