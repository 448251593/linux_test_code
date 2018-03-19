#ifndef H265_G711_QUEUE_H_
#define H265_G711_QUEUE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <miio/frameinfo.h>
#include <shbf/receiver-ev.h>
#include <miio/av_cmd.h>
#include "libmortox.h"
#include "util.h"

#define AUDIO_MAX_LEN 128*1024
#define VIDEO_MAX_LEN 1*1024*1024

#define AUDIO_ALARM_LEN 128*1024
#define VIDEO_ALARM_LEN 512*1024
#define MAX_VIDEO_FRAME_LEN (512 * 1024)
#define MAX_AUDIO_FRAME_LEN (2 * 1024)

#define MAX_FRA 4096
typedef struct _FRAME_T64_ {
	union {
		struct {
			unsigned long long stamp64;
		};
		struct {
			unsigned int stamp32;
			unsigned int overcnt;
		};
	};
} FRAME_T64;

typedef struct fra_info {
	char *data_index;
	unsigned int index;
	unsigned int len;
	unsigned int keyframe;	///< 1: keyframe 0: non-keyframe.
	unsigned int dispose_flag;
	struct timeval sys_time;
	unsigned long long timestamp;
	time_t time_I_frame;
} frame_t;

typedef struct data_info {
	char *data;
	unsigned int wIndex;
	unsigned int rIndex;
} data_t;

typedef struct alarm_record {
	data_t buf;
	frame_t frame[MAX_FRA];
	unsigned int frame_num;
	unsigned int frame_wIndex;
	unsigned int frame_rIndex;
	unsigned int alarm_wr_index;
	unsigned int buf_ready;
} alarm_record_buf_t;

typedef struct pre_record {
	data_t buf;
	frame_t frame[MAX_FRA];
	unsigned int frame_num;
	unsigned int frame_wIndex;
	unsigned int frame_rIndex;
	unsigned int pre_wr_index;
} pre_record_buf_t;

pre_record_buf_t audio_prerecord_buf;
pre_record_buf_t video_prerecord_buf;

alarm_record_buf_t audio_alarm_buf;
alarm_record_buf_t video_alarm_buf;

void dumpinfo_record_buf(const char *reason, pre_record_buf_t *h265_buf, alarm_record_buf_t *alarm_buf);
void mp4_queue_init_pre(pre_record_buf_t * g711_buf, pre_record_buf_t * h265_buf);
void free_mp4_queue(pre_record_buf_t * g711_buf, pre_record_buf_t * h265_buf);
int push_g711_queue_pre(char *data, int record_status, pre_record_buf_t * g711_buf);
void pop_g711_queue_pre(pre_record_buf_t * g711_buf);
unsigned int get_frame_num_pre(pre_record_buf_t *buf);
int push_h265_queue_pre(char *data, int record_status, pre_record_buf_t * h265_buf);
void pop_h265_queue_pre(pre_record_buf_t * h265_buf);
void sync_g711_with_h265(pre_record_buf_t * g711_buf, pre_record_buf_t * h265_buf);

void mp4_queue_init_alarm(alarm_record_buf_t * g711_buf, alarm_record_buf_t * h265_buf);
void free_alarm_queue(alarm_record_buf_t * g711_buf, alarm_record_buf_t * h265_buf);
int push_g711_queue_alarm(char *data, int alarm_status, alarm_record_buf_t * g711_buf);
int push_h265_queue_alarm(char *data, int alarm_status, alarm_record_buf_t * h265_buf);
void sync_g711_with_h265_alarm(alarm_record_buf_t * aac_buf, alarm_record_buf_t * h265_buf);
unsigned int get_g711_frame_num_alarm(alarm_record_buf_t * g711_buf);
unsigned int get_h265_frame_num_alarm(alarm_record_buf_t * h265_buf);
void sync_g711_with_h265_alarm(alarm_record_buf_t * g711_buf, alarm_record_buf_t * h265_buf);
void alarm_buf_reset(alarm_record_buf_t * aac_alarm_buf, alarm_record_buf_t * h264_alarm_buf);
void alarm_record_over(void);
void get_record_snapshot(void);
void get_alarm_snapshot(void);
void create_MotionAlarm_thread(void);
void create_MotionEvent_thread(void);
#endif
