#ifndef H265_G711_MP4_H_
#define H265_G711_MP4_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mp4v2/mp4v2.h>
#include <shbf/receiver-ev.h>
#include "h265_g711_queue.h"
#define MAX_FILEPATH_LEN 256
#define MAX_MSG_LEN 2048

typedef struct UploadAlarmData {
	int get_url_ok;
	unsigned char url[512];
	unsigned char object_name[128];
	unsigned char pwd[32];
	int get_pic_url_ok;
	unsigned char pic_url[512];
	unsigned char pic_object_name[128];
	unsigned char pic_pwd[32];
	int push_flag;
	int MD_trig;
	int upload_cmd_trig;
} UploadAlarmData_t;

typedef struct RecordFileMsgData {
	time_t timestamp;
	unsigned char duration;
	unsigned char motion_flag;
	unsigned char save_flag;
	unsigned char reserve_byte;
} RecordFileMsg_t;

/*typedef enum {
	SD_IS_UMOUNT,
	SD_STATUS_ERROR,
	SD_IS_NOTEXIST,
	SD_FORMAT_ERROR,
	SD_IS_FORMATIING
}sd_error_code_e;
*/
typedef enum {
	SD_OK,
	SD_NOTEXIST,
	SD_NOSPACE,
	SD_EXCEPTION,
	SD_FORMATIING,
	SD_UMOUNT,
	SD_FORMAT_ERROR,
} sd_status_e;

typedef struct nvram_param {
	char *key;
	char value[64];
	xdata_type type; /**<Type of data stored */
	unsigned result;
} nvram_param_t;

int get_push_flag(void);
void create_record_thread(void);
void create_stream_thread(void);
void set_poweroff_value(int value);
int get_poweroff_value(void);
void thread_record_mp4_exit(void);
int get_motionalarm_flag(void);
void set_motion_flag(unsigned char value);
void set_record_flag(int value);
int sdcard_storge_process(int id, int sock, char *ackbuf);
void set_cb(xresponse * r, void *data);
void get_motionrecord_value(void);
void nvram_set(nvram_param_t * nvram_params, char *value);	//layer is nvram or mem;
void nvram_get(nvram_param_t * nvram_params);	//layer is nvram or mem
void nvram_commit(void);
void set_alarm_start_flag(int value);
void get_motion_params(void);
int get_write_flag(void);
void get_motionalarm_value(void);
int upload_motion_alarm(UploadAlarmData_t * params);
int send_to_get_url(void);
int request_pic_url(void);
int set_upload_url(const char *url, int flag);
int set_upload_pwd(const char *pwd, int flag);
int set_upload_object_name(const char *obj_name, int flag);
int is_upload_alarm_ok(const char *msg);
char *get_p2p_token(char *token);
void set_upload_video_value(int value);
int get_upload_video_value(void);
void enable_motion_alarm_record(void);
int is_motion_alarm_done(void);
int set_upload_trigger(int value, int flag);
int is_alarm_start(void);
pthread_t get_motionalarm_thread_id(void);
void create_sdcheck_thread(void);
void set_motion_alarm_done(int value);
void stop_record(void);
int make_alarm_mp4_pre(void);
int save_file_timestamp(time_t timestamp);
int is_sdcard_avail(void);
int rm_file_timestamp(time_t timestamp);
void CheckAndDelEmptyDir(time_t * timestamp);
int get_format_flag(void);
void set_format_flag(int value);
void exec_format_thread(void);
int is_sdcard_on(void);
int get_sdcard_on(void);
int get_sdcard_status(void);
void sdcard_umount(int id, char *ackbuf);
void create_record_dir(void);
void nvram_get_power_value(void);
int time_to_alarm(void);
#endif
