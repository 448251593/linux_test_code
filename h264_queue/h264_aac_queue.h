#ifndef H264_AAC_QUEUE_H_
#define H264_AAC_QUEUE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "gmlib.h"
//#include "util.h"
//#include "miio_avstreamer.h"
#define MIJIA_PTZ_CAMERA

#if defined (MIJIA_PTZ_CAMERA)
#define AUDIO_MAX_LEN 24*1024		/* audio bitstream is 4kbytes/s, 5s max 20kbytes. */
#define VIDEO_MAX_LEN 1800*1024		/* 720p bitrate max 100kbytes/s, so prepare record 5s. */
#ifdef USE_AUDIO_CODEC_G711
#define AUDIO_ALARM_LEN 1*1024		/* 10s max = 6kbytes x 10 */
#else
#define AUDIO_ALARM_LEN 1*1024		/* 10s max = 4kbytes x 10 */
#endif
#define VIDEO_ALARM_LEN 1800*1024	/* 360p bitrate max 50kbytes/s, so alarm record 5s. */	
#define MAX_FRA 300
#else
#define AUDIO_MAX_LEN 128*1024
#define VIDEO_MAX_LEN 3*1024*1024
#define AUDIO_ALARM_LEN 64*1024
#define VIDEO_ALARM_LEN 1*1024*1024
#define MAX_FRA 1024
#endif

enum ALARM_VIDEO_AAC_COMP_STATUS
{
	EN_ALARM_VIDEO_AAC_NOT_RECORD = 0		/* alarm video and aac not record */
,	EN_ALARM_VIDEO_NOT_COMP_AAC_NOT_COMP	/* alarm video and aac record , but they not complete*/
,	EN_ALARM_VIDEO_COMP_AAC_NOT_COMP		/* alarm video record complete,but aac record not complete */
,	EN_ALARM_VIDEO_NOT_COMP_AAC_COMP		/* alarm video record not complete,but aac record is complete */
,	EN_ALARM_VIDEO_COMP_AAC_COMP			/* alarm video record and aac complete */
};


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
	unsigned int len;
	unsigned int keyframe;	///< 1: keyframe 0: non-keyframe.
	struct timeval sys_time;
	unsigned long long timestamp;
	time_t time_I_frame;
} frame_t;

typedef struct data_info {
	char *data;
	unsigned int wIndex;
	unsigned int rIndex;
	unsigned int tail_len;	//means the space to the next available frame
} data_t;

typedef struct alarm_record {
	data_t buf;
	frame_t frame[MAX_FRA];
	unsigned int frame_num;	/* frame num */
	unsigned int frame_wIndex;	/* each record is from 0 add */
	unsigned int frame_rIndex;	/* get read frame index */
	unsigned int alarm_wr_index;	/*  */
	unsigned int buf_ready;		/* frame buffer can copy to alarm buffer flag */
	enum ALARM_VIDEO_AAC_COMP_STATUS  comp_sts;
	unsigned int keyframeindex[2];//buf[0] last ket frame  buf[1] crnt key frame
} alarm_record_buf_t;

typedef struct pre_record {
	data_t buf;
	frame_t frame[MAX_FRA];
	unsigned int frame_num;
	unsigned int frame_wIndex; /*first index*/
	unsigned int frame_rIndex; /* end index */
	unsigned int pre_wr_index;
	unsigned int push_full;
} pre_record_buf_t;

pre_record_buf_t audio_prerecord_buf;
pre_record_buf_t video_prerecord_buf;

alarm_record_buf_t audio_alarm_buf;
alarm_record_buf_t video_alarm_buf;

void mp4_queue_init_pre(pre_record_buf_t * aac_buf, pre_record_buf_t * h264_buf);
char push_aac_queue_pre(gm_enc_bitstream_t * aac_bs, int record_status, pre_record_buf_t * aac_buf);
void pop_aac_queue_pre(pre_record_buf_t * aac_buf);
unsigned char get_aac_frame_num_pre(pre_record_buf_t * aac_buf);
char push_h264_queue_pre(gm_enc_bitstream_t * h264_bs, int record_status, pre_record_buf_t * h264_buf);
void pop_h264_queue_pre(pre_record_buf_t * h264_buf);
unsigned char get_h264_frame_num_pre(pre_record_buf_t * h264_buf);
void sync_aac_with_h264(pre_record_buf_t * aac_buf, pre_record_buf_t * h264_buf);

void mp4_queue_init_alarm(alarm_record_buf_t * aac_alarm_buf, alarm_record_buf_t * h264_alarm_buf);
char push_aac_buf_alarm(gm_enc_bitstream_t * aac_bs, int alarm_status, alarm_record_buf_t * aac_buf);
char push_h264_buf_alarm(gm_enc_bitstream_t * h264_bs, int alarm_status, alarm_record_buf_t * h264_buf, alarm_record_buf_t * aac_alarm_buf);
void sync_aac_with_h264_alarm(alarm_record_buf_t * aac_buf, alarm_record_buf_t * h264_buf);
void alarm_buf_reset(alarm_record_buf_t * aac_alarm_buf, alarm_record_buf_t * h264_alarm_buf);
void alarm_record_over(void);
void create_MotionRecord_thread();
#endif
