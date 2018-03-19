#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/magic.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/vfs.h>
#include <sys/prctl.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <json-c/json.h>
#include <miio/net.h>
#include <ev.h>
#include <miio/frameinfo.h>
#include <shbf/receiver-ev.h>
#include <miio/av_cmd.h>
#include <miio/dev.h>
#include <assert.h>
#include "util.h"
#include "curl.h"

#include "json.h"
#include "h265_g711_mp4.h"
#include "h265_g711_queue.h"

shbfev_rcv_t main_stream_recv;
shbfev_rcv_t sub_stream_recv;
shbfev_rcv_t audio_stream_recv;
shbfev_rcv_t md_event_recv;
shbfev_rcv_t snap_shot_recv;

int main_stream_recv_callback(char *private_data, void *buf)
{
	if (NULL == buf) {
		log_err("main stream data buf is null!\n");
		return -1;
	}

        // mod by yyq@20180128
        // just judge whether frame dropped, and handle it
        size_t size=0;
        int data_size = 0;

        static int dropflag = 0;
        static unsigned int lastmainframeseq = -1;

        size = shbf_get_size(buf);
        data_size = size - sizeof (MIIO_IMPEncoderStream) ;
        if (data_size > 0 ) {
                    MIIO_IMPEncoderStream* streamInfo =  (MIIO_IMPEncoderStream* )buf;

                    if (1 == dropflag) {
                            if (1 != streamInfo->key_frame) {
                                    lastmainframeseq = streamInfo->seq;
                                    log_debug ("main record dropping frame, seq: %5d iskey: %d\n", streamInfo->seq, streamInfo->key_frame);
                                    shbf_free(buf);
                                    return 0;
                            } else {
                                    dropflag = 0;
                            }
                    } else {
                                if ((streamInfo->seq - lastmainframeseq != 1) && (lastmainframeseq != -1) && (1 != streamInfo->key_frame)) {
                                        log_err ("main record error, the seq is disconinous, seq: %d, last seq: %d, iskey: %d\n",
                                                        streamInfo->seq, lastmainframeseq, streamInfo->key_frame);
                                        dropflag = 1;
                                        lastmainframeseq = streamInfo->seq;
                                        shbf_free(buf);
                                        return 0;
                                }
                    }
                    lastmainframeseq = streamInfo->seq;
            }


	//push 1080p data to record queue
	/*log_info("1080p recv call back call\n"); */
	if (get_poweroff_value()) {
		if (get_push_flag()) {
			//log_debug("........push h265 record status = 1\n");
			push_h265_queue_pre(buf, 1, &video_prerecord_buf);
		} else {
			//log_debug("........push h265 record status = 0\n");
			push_h265_queue_pre(buf, 0, &video_prerecord_buf);
		}
	}
	shbf_free(buf);
	return 0;
}

int sub_stream_recv_callback(char *private_data, void *buf)
{
	if (NULL == buf) {
		log_err("sub stream data buf is null!\n");
		return -1;
	}

	// mod by yyq@20180128
	// just judge whether frame dropped, and handle it
	size_t size=0;
	int data_size = 0;

	static int dropflag = 0;
	static unsigned int lastmainframeseq = -1;

	size = shbf_get_size(buf);
	data_size = size - sizeof (MIIO_IMPEncoderStream) ;
	if (data_size > 0 ) {
		MIIO_IMPEncoderStream* streamInfo =  (MIIO_IMPEncoderStream* )buf;

		if (1 == dropflag) {
			if (1 != streamInfo->key_frame) {
				lastmainframeseq = streamInfo->seq;
				log_debug ("sub recordd ropping frame, seq: %5d iskey: %d\n", streamInfo->seq, streamInfo->key_frame);
				shbf_free(buf);
				return 0;
			} else {
				dropflag = 0;
			}
		} else {
			if ((streamInfo->seq - lastmainframeseq != 1) && (lastmainframeseq != -1) && (1 != streamInfo->key_frame)) {
				log_err ("sub record error, the seq is disconinous, seq: %d, last seq: %d, iskey: %d\n",
						streamInfo->seq, lastmainframeseq, streamInfo->key_frame);
				dropflag = 1;
				lastmainframeseq = streamInfo->seq;
				shbf_free(buf);
				return 0;
			}
		}

		lastmainframeseq = streamInfo->seq;
	}

	//push 360p data to record queue
	/*log_info("360p recv call back call\n"); */
	if (get_poweroff_value()) {
		if (get_write_flag() && get_motionalarm_flag()) {
			push_h265_queue_alarm(buf, 1, &video_alarm_buf);
		} else {
			push_h265_queue_alarm(buf, 0, &video_alarm_buf);
		}
	}
	shbf_free(buf);
	return 0;
}

int audio_stream_recv_callback(char *private_data, void *data)
{
	if (NULL == data) {
		log_err("audio data buf is null!\n");
		return -1;
	}
	//push audio data to record queue
	/*log_info("audio recv call back call\n"); */
	if (get_poweroff_value()) {
		if (get_push_flag()) {
			push_g711_queue_pre(data, 1, &audio_prerecord_buf);
		} else {
			push_g711_queue_pre(data, 0, &audio_prerecord_buf);
		}

		if (get_write_flag() && get_motionalarm_flag()) {
			push_g711_queue_alarm(data, 1, &audio_alarm_buf);
		} else {
			push_g711_queue_alarm(data, 0, &audio_alarm_buf);
		}
	}

	shbf_free(data);
	return 0;
}

int md_event_callback(char *private_data, void *data)
{
	//log_debug("md_event_callback start.\n");
	if (get_poweroff_value()) {
		set_motion_flag(1);
		set_record_flag(1);
		if (get_motionalarm_flag() && !is_alarm_start() && time_to_alarm()) {
			set_alarm_start_flag(1);
			set_upload_trigger(1, 1);
		}
	}
	return 0;
}

char snapshot_record_path[MAX_FILEPATH_LEN] = { 0 };

int print_len_picture = 0;
int snapshot_callback(char *private_data, void *data)
{
	MIIO_IMPEncoderStream *FrameInfo;
	int offset = sizeof(MIIO_IMPEncoderStream);
	FrameInfo = (MIIO_IMPEncoderStream *) data;

	char pic_name[MAX_FILEPATH_LEN] = { '\0' };
	if (1 == FrameInfo->dispose_flag) {
		sprintf(pic_name, "%s", "/tmp/alarm.jpeg");
		log_info("alarm picture is %s\n", pic_name);
	} else {
		snprintf(pic_name, strlen(snapshot_record_path) - 3, "%s", snapshot_record_path);
		snprintf(&pic_name[strlen(pic_name)], 6, "%s", ".jpeg");
		log_info("new picture is %s\n", pic_name + print_len_picture + 1);
	}

	FILE *fp = fopen(pic_name, "wb");
	if (fp == NULL) {
		log_err("fopen error: %m\n");
		shbf_free(data);
		return -1;
	}
	fwrite(data + offset, 1, FrameInfo->length, fp);
	fclose(fp);
	shbf_free(data);
	return 0;
}

void shbf_close(void *private_data)
{
	log_info("%s receiver closed!\n", private_data);
}

extern pthread_cond_t pre_product;
extern pthread_mutex_t pre_lock;
#if 0
extern shbfev_rcv_t main_stream_recv;
extern shbfev_rcv_t sub_stream_recv;
extern shbfev_rcv_t audio_stream_recv;
extern shbfev_rcv_t snap_shot_recv;
#endif
extern mortox_client *xc;

pthread_mutex_t timefile_lock = PTHREAD_MUTEX_INITIALIZER;
typedef enum NALUnitType {
	NAL_TRAIL_N = 0,
	NAL_TRAIL_R = 1,
	NAL_TSA_N = 2,
	NAL_TSA_R = 3,
	NAL_STSA_N = 4,
	NAL_STSA_R = 5,
	NAL_RADL_N = 6,
	NAL_RADL_R = 7,
	NAL_RASL_N = 8,
	NAL_RASL_R = 9,
	NAL_BLA_W_LP = 16,
	NAL_BLA_W_RADL = 17,
	NAL_BLA_N_LP = 18,
	NAL_IDR_W_RADL = 19,
	NAL_IDR_N_LP = 20,
	NAL_CRA_NUT = 21,
	NAL_VPS = 32,
	NAL_SPS = 33,
	NAL_PPS = 34,
	NAL_AUD = 35,
	NAL_EOS_NUT = 36,
	NAL_EOB_NUT = 37,
	NAL_FD_NUT = 38,
	NAL_SEI_PREFIX = 39,
	NAL_SEI_SUFFIX = 40
} h265_nalu_type;

typedef struct mp4_record_params {
	uint32_t width;
	uint32_t height;
	uint32_t fps;
	uint32_t time_scale;
	MP4FileHandle mp4_handle;
	MP4TrackId audio_track_id;
	MP4TrackId video_track_id;
	uint32_t key_frame;
	uint32_t dispose_flag;
	uint32_t is_need_set;
	int sei_len;
} mp4_record_params_t;

mp4_record_params_t mp4_params = {
	.width = 1920,
	.height = 1080,
	.fps = 20,
	.time_scale = 90000,
	.mp4_handle = MP4_INVALID_FILE_HANDLE,
	.audio_track_id = MP4_INVALID_TRACK_ID,
	.video_track_id = MP4_INVALID_TRACK_ID,
	.key_frame = 0,
	.dispose_flag = 0,
	.sei_len = 0,
	.is_need_set = 0
};

static volatile char sdcard_path[64] = { '\0' };
UploadAlarmData_t upload_params = { 0 };

static volatile int is_recording = 0;
static volatile int is_sdcard_ok = 0;
static volatile int sdcard_on = 0;
static volatile int is_format = 0;
static volatile int sdcard_full = 0;
static volatile int motion_record_flag = 0;
static volatile unsigned int recorded_video_frame = 0;
static volatile int upload_video = 0;
static volatile int motion_alarm_thread_done = 0;
static volatile unsigned char motion_flag = 0;
volatile int power_off = 1;
static volatile int start_record_flag = 1;	//need reset to 0 if set md record
static volatile int enable_record = 0;
static volatile int app_enable_record = 1;

char mstar_sei_data[1024] = { 0 };

void set_enable_record(int value)
{
	enable_record = value;
}

static int gProcessRun = 1;

int set_upload_trigger(int value, int flag)
{
	if (1 == flag) {
		upload_params.MD_trig = value;
	} else if (0 == flag) {
		upload_params.upload_cmd_trig = value;
	}
	return 0;
}

void set_record_flag(int value)
{
	start_record_flag = value;
}

void get_motionrecord_value(void)
{
	nvram_param_t params = { 0 };
	params.key = "motion_record";
	params.type = X_TYPE_STRING;
	nvram_get(&params);
	if (params.result) {
		if (0 == strncmp(params.value, "on", 2)) {
			motion_record_flag = 0;
			set_record_flag(0);
			log_info("motion_record is %s\n", params.value);
		} else {
			motion_record_flag = 1;
			set_record_flag(1);
		}
		if (0 == strncmp(params.value, "stop", 4)) {
			app_enable_record = 0;
			usleep(3 * 1000);
			pthread_cond_signal(&pre_product);
		} else {
			app_enable_record = 1;
		}
	} else {
		log_err("nvram_get motion_record error\n");
	}
}

void set_motion_flag(unsigned char value)
{
	motion_flag = value;
}

unsigned char get_motion_flag(void)
{
	return motion_flag;
}

static void force_key_frame(void)
{
	shbfev_rcv_send_message(main_stream_recv, REQ_IDR_STR, strlen(REQ_IDR_STR));
}

void get_record_snapshot(void)
{
	shbfev_rcv_send_message(snap_shot_recv, REQ_SNAPSHOT_RECORD, strlen(REQ_SNAPSHOT_RECORD));
}

void get_alarm_snapshot(void)
{
	shbfev_rcv_send_message(snap_shot_recv, REQ_SNAPSHOT_ALARM, strlen(REQ_SNAPSHOT_ALARM));
}

int my_aes_cbc_encrypt(unsigned char *data_in, unsigned char *data_out, int len, unsigned char *pwd)
{
	if (!data_in || !pwd || !data_out)
		return -1;

	AES_KEY aes;
	if (AES_set_encrypt_key(pwd, 256, &aes) < 0) {
		log_err("AES_set_encrypt_key error\n");
		return -1;
	}
	unsigned char iv[AES_BLOCK_SIZE];	// init vector
	int i;
	for (i = 0; i < AES_BLOCK_SIZE; ++i) {
		iv[i] = 0;
	}

	AES_cbc_encrypt(data_in, data_out, len, &aes, iv, AES_ENCRYPT);
	return 0;
}

int my_aes_decrypt(unsigned char *data_in, unsigned char *data_out, int len, unsigned char *pwd)
{
	if (!data_in || !pwd || !data_out)
		return -1;

	AES_KEY aes;
	if (AES_set_decrypt_key(pwd, 256, &aes) < 0) {
		log_err("AES_set_decrypt_key error\n");
		return -1;
	}

	unsigned char iv[AES_BLOCK_SIZE];	// init vector
	int i;
	for (i = 0; i < AES_BLOCK_SIZE; ++i) {
		iv[i] = 0;
	}

	AES_cbc_encrypt(data_in, data_out, len, &aes, iv, AES_DECRYPT);
	return 0;
}

void thread_record_mp4_exit(void)
{
	gProcessRun = 0;
	pthread_cond_signal(&pre_product);
}

int is_sdcard_on(void)
{
	if ((access("/dev/mmcblk0", F_OK)) != -1) {
		return 1;
	} else {
		return 0;
	}
}

void run_cmd(const char *cmd, char *output, int outsize)
{
	FILE *pipe = popen(cmd, "r");
	if (!pipe) {
		log_err("run_cmd \'%s\' failed\n", cmd);
		perror("run_cmd error");
		return;
	}

	if (NULL != output) {
		fgets(output, outsize, pipe);
	}
	pclose(pipe);
}

static int is_sdcard_full(void)
{
	struct statfs tf;
	int ret = statfs((const char *)sdcard_path, &tf);
	if (ret) {
		perror("statfs error");
	} else {
		unsigned int avail_size = tf.f_bavail / 1024 * tf.f_bsize / 1024;
		log_info("Available size is %uMB\n", avail_size);
		if (avail_size < 100) {
			log_info("sdcard is full! Available size is %uMB\n", avail_size);
			sdcard_full = 1;
			return 1;
		} else {
			sdcard_full = 0;
			return 0;
		}
	}
	return 1;
}

int get_record_msg_path(char *path)
{
	if (!path) {
		return -1;
	}
	if (strlen((const char *)sdcard_path)) {
		sprintf(path, "%s/MIJIA_RECORD_VIDEO/.record_msg", sdcard_path);
	} else {
		return -1;
	}

	return 0;
}

int get_sdcard_path(char *path)
{
	if (!path) {
		return -1;
	}

	if (strlen((const char *)sdcard_path)) {
		sprintf(path, "%s/MIJIA_RECORD_VIDEO", sdcard_path);
	} else {
		return -1;
	}
	return 0;
}

int isDirectoryEmpty(char *dirname)
{
	int n = 0;
	struct dirent *d = NULL;
	DIR *dir = opendir(dirname);
	if (dir == NULL)	//Not a directory or doesn't exist
		return 1;
	while ((d = readdir(dir)) != NULL) {
		if (++n > 2)
			break;
	}
	closedir(dir);
	if (n <= 2)		//Directory Empty
		return 1;
	else
		return 0;
}

void CheckAndDelEmptyDir(time_t * timestamp)
{
	char dir[128] = { '\0' };
	if (get_sdcard_path(dir)) {
		log_err("CheckAndDelEmptyDir: get sdcard path fail!\n");
	} else {
		struct tm time_set;
		localtime_r(timestamp, &time_set);
		strftime(&dir[strlen(dir)], sizeof(dir), "/%Y%m%d%H", &time_set);
		if (isDirectoryEmpty(dir)) {
			int ret = remove(dir);
			if (!ret) {
				log_info("rm %s ok.\n", dir);
			} else {
				perror("rm dir fail\n");
			}
		}
	}
}

int is_file_exist(char *path, time_t * timestamp, char *suffix)
{
	if (NULL == timestamp || NULL == suffix)
		return -1;
	char dir[MAX_FILEPATH_LEN] = { 0 };
	if (get_sdcard_path(dir)) {
		log_err("is_file_exist: get sdcard path fail!\n");
		return 1;
	}
	struct tm time_set;
	localtime_r(timestamp, &time_set);
	strftime(&dir[strlen(dir)], sizeof(dir), "/%Y%m%d%H/%MM%SS_", &time_set);
	sprintf(&dir[strlen(dir)], "%u.%s", (unsigned int)*timestamp, suffix);
	if ((access(dir, F_OK)) != -1) {
		if (path)
			memcpy(path, dir, strlen(dir));
	} else {
		return -2;
	}
	return 0;
}

static int rm_timestamp(time_t timestamp)
{

	char file_path[MAX_FILEPATH_LEN] = { 0 };
	struct stat statbuf;
	get_record_msg_path(file_path);
	stat(file_path, &statbuf);
	unsigned int size = statbuf.st_size;

	if (0 == size) {
		log_err("rm_timestamp : record msg size is zero\n");
		return -1;
	}
	unsigned char *file_data = malloc(size);
	if (NULL == file_data) {
		log_err("rm timestamp:malloc %u Bytes fail!\n", size);
		return -1;
	}

	FILE *timestamp_fp = NULL;
	pthread_mutex_lock(&timefile_lock);

	timestamp_fp = fopen(file_path, "r+");
	if (NULL == timestamp_fp) {
		pthread_mutex_unlock(&timefile_lock);
		free(file_data);
		perror("can't open the timestamp file!");
		return -1;
	}
	if (size != fread(file_data, 1, size, timestamp_fp)) {
		fclose(timestamp_fp);
		pthread_mutex_unlock(&timefile_lock);
		free(file_data);
		return -1;
	}
	int i = 0;

	RecordFileMsg_t *tmp_node = NULL;
	for (i = 0; i < size; i += sizeof(RecordFileMsg_t)) {
		tmp_node = (RecordFileMsg_t *) (file_data + i);
		if (timestamp == tmp_node->timestamp) {
			memmove(&file_data[i], &file_data[i + sizeof(RecordFileMsg_t)],
				size - i - sizeof(RecordFileMsg_t));
			fseek(timestamp_fp, 0, SEEK_SET);
			fwrite(file_data, 1, size - sizeof(RecordFileMsg_t), timestamp_fp);
			fclose(timestamp_fp);
			truncate(file_path, size - sizeof(RecordFileMsg_t));
			pthread_mutex_unlock(&timefile_lock);
			free(file_data);
			log_info("delete file : %u\n", timestamp);
			return 0;
		}
	}
	fclose(timestamp_fp);
	pthread_mutex_unlock(&timefile_lock);
	free(file_data);
	return -1;
}

int rm_file_timestamp(time_t timestamp)
{
	time_t time_stamp = timestamp;
	char file_path[MAX_FILEPATH_LEN] = { '\0' };

	int ret = is_file_exist(file_path, &time_stamp, "mp4");
	if (0 == ret) {
		sprintf(&file_path[strlen(file_path) - 3], "%s", "mp4");
		if (remove(file_path)) {
			log_err("delete:can't rm %s!\n", file_path);
			perror("rm file fail");
			return -1;
		}

		sprintf(&file_path[strlen(file_path) - 3], "%s", "jpeg");
		remove(file_path);
		ret = rm_timestamp(timestamp);
	} else if (-2 == ret) {
		ret = rm_timestamp(timestamp);
	} else {
		log_err("delete:can't find %s!\n", file_path);
		ret = -1;
	}
	return ret;
}

int save_file_timestamp(time_t timestamp)
{
	time_t time_stamp = timestamp;
	char file_path[MAX_FILEPATH_LEN] = { 0 };
	if (0 == is_file_exist(file_path, &time_stamp, "mp4")) {
		memset(file_path, 0, sizeof(file_path));
		get_record_msg_path(file_path);
		struct stat statbuf;
		stat(file_path, &statbuf);
		if (0 == statbuf.st_size) {
			log_err("save:record msg file empty!\n");
			return -1;
		}

		pthread_mutex_lock(&timefile_lock);
		int fd = open(file_path, O_RDWR);
		if (fd < 0) {
			pthread_mutex_unlock(&timefile_lock);
			log_err("%s:%d, open error: %m\n", __func__, __LINE__);
			return -1;
		}
		unsigned char *record_msg_data = MAP_FAILED;
		record_msg_data = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (MAP_FAILED == record_msg_data) {
			pthread_mutex_unlock(&timefile_lock);
			perror("save: mmap error");
			return -1;
		}

		RecordFileMsg_t *tmp_node = NULL;
		int i = 0;
		for (i = 0; i < statbuf.st_size; i = i + sizeof(RecordFileMsg_t)) {
			tmp_node = (RecordFileMsg_t *) (record_msg_data + i);
			if (tmp_node->timestamp == timestamp) {
				tmp_node->save_flag = 1;
				break;
			}
			tmp_node = NULL;
		}
		munmap(record_msg_data, statbuf.st_size);
		close(fd);
		pthread_mutex_unlock(&timefile_lock);
		return 0;
	} else {
		log_err("save:can't find the mp4 file! file not exist\n");
		return -1;
	}
}

int loop_record_delete(void)
{
	char file_path[MAX_FILEPATH_LEN] = { 0 };
	get_record_msg_path(file_path);
	if ((access(file_path, F_OK)) != -1) {
		struct stat statbuf;
		stat(file_path, &statbuf);
		unsigned int size = statbuf.st_size;
		if (0 == size) {
			log_err("stat size is zero\n");
			return -1;
		}
		unsigned char *file_data = malloc(size);
		if (NULL == file_data) {
			log_err("malloc %u Bytes fail!\n", size);
			return -1;
		}
		FILE *timestamp_fp = NULL;
		pthread_mutex_lock(&timefile_lock);
		timestamp_fp = fopen(file_path, "r+");
		if (NULL == timestamp_fp) {
			pthread_mutex_unlock(&timefile_lock);
			perror("loop delete :can't open the timestamp file!");
			free(file_data);
			return -1;
		}
		if (size != fread(file_data, 1, size, timestamp_fp)) {
			fclose(timestamp_fp);
			pthread_mutex_unlock(&timefile_lock);
			free(file_data);
			log_err("fread size not right\n");
			return -1;
		}

		fclose(timestamp_fp);
		pthread_mutex_unlock(&timefile_lock);

		time_t start_timestamp = 0;
		char cur_del_dir[MAX_FILEPATH_LEN] = { 0 };
		char tmp_dir[MAX_FILEPATH_LEN] = { 0 };
		struct tm filetime = { 0 };
		int i = 0;
		time_t time_now = 0;
		RecordFileMsg_t *tmp_node = NULL;
		time(&time_now);
		for (i = 0; i < size; i += sizeof(RecordFileMsg_t)) {
			tmp_node = (RecordFileMsg_t *) (file_data + i);
			if (time_now - tmp_node->timestamp < 61) {
				log_info("%u maybe recording file, don't rm!\n", tmp_node->timestamp);
				localtime_r(&tmp_node->timestamp, &filetime);
				strftime(tmp_dir, sizeof(tmp_dir), "%Y%m%d%H", &filetime);
				if (strcmp(cur_del_dir, tmp_dir)) {
					CheckAndDelEmptyDir(&start_timestamp);
				}
				return 1;
			} else {
				if (!tmp_node->save_flag) {
					if (!start_timestamp) {
						start_timestamp = tmp_node->timestamp;
						localtime_r(&start_timestamp, &filetime);
						strftime(cur_del_dir, sizeof(cur_del_dir), "%Y%m%d%H", &filetime);
						rm_file_timestamp(start_timestamp);
						continue;
					}
					localtime_r(&tmp_node->timestamp, &filetime);
					strftime(tmp_dir, sizeof(tmp_dir), "%Y%m%d%H", &filetime);
					if (0 == strcmp(cur_del_dir, tmp_dir)) {
						rm_file_timestamp(tmp_node->timestamp);
					} else {
						CheckAndDelEmptyDir(&start_timestamp);
						break;
					}
				}
			}
			tmp_node = NULL;
		}
		free(file_data);
		return 0;
	} else {
		log_err("***********time stamp don't exist!!!!************\n");
		return -1;
	}
}

static unsigned long long disk_size = 0;
static unsigned long long avail_size = 0;
static unsigned long long record_size = 0;

void reset_sdcard_static(void)
{
	disk_size = 0;
	avail_size = 0;
	record_size = 0;
}

void set_sdcard_status(int commit, int value)
{
	char cmd[8] = { '\0' };
	sprintf(cmd, "%d", value);
	nvram_param_t params = { 0 };
	params.key = "sdcard_status";
	nvram_set(&params, cmd);
	if (params.result) {
		log_info("set sdcard_status = %s ok!\n", cmd);
		if (commit)
			nvram_commit();
	} else {
		log_err("set sdcard_status = %s FAIL!\n", cmd);
	}
}

int is_sdcard_avail(void)
{
	return is_sdcard_ok;
}

int get_sdcard_status(void)
{
	int result = 0;
	nvram_param_t params = { 0 };
	params.key = "sdcard_status";
	params.type = X_TYPE_STRING;
	nvram_get(&params);
	if (!params.result) {
		log_err("nvram_get sdcard_status error\n");
		return -1;
	} else {
		if (0 == strncmp(params.value, "0", 1)) {
			result = 0;
		} else if (0 == strncmp(params.value, "1", 1)) {
			result = 1;
		} else if (0 == strncmp(params.value, "2", 1)) {
			result = 2;
		} else if (0 == strncmp(params.value, "3", 1)) {
			result = 3;
		} else if (0 == strncmp(params.value, "4", 1)) {
			result = 4;
		} else if (0 == strncmp(params.value, "5", 1)) {
			result = 5;
		}
	}
	return result;
}

int sdcard_check_get_path(void)
{
	struct statfs stat;
	int rc;

restart:
	rc = statfs(MIIO_SDCARD_PATH, &stat);
	if (rc < 0) {
		if (errno == EINTR) //TEMP_FAILURE_RETRY
			goto restart;
		return -1;
	}

	if (stat.f_type != MSDOS_SUPER_MAGIC) {
		sdcard_path[0] = 0;
		return -1;
	}
	memcpy(sdcard_path, MIIO_SDCARD_PATH, sizeof(MIIO_SDCARD_PATH));
	return 0;
}

void set_sdcard_on(int value)
{
	sdcard_on = value;
}

int get_sdcard_on(void)
{
	return sdcard_on;
}

void get_sdcard_static(void)
{
	int ret = 0;
	ret = get_sdcard_status();
	if (0 == ret || 2 == ret) {
		struct statfs tf;
		int ret = statfs((const char *)sdcard_path, &tf);
		if (ret) {
			perror("statfs error");
			reset_sdcard_static();
		} else {
			disk_size = tf.f_blocks * tf.f_bsize;
			avail_size = tf.f_bavail * tf.f_bsize;

			char buf[64] = { 0 };
			char cmd[128] = { '\0' };
			char record_path[64] = { '\0' };
			get_sdcard_path(record_path);
			sprintf(cmd, "du -k %s | awk 'END{print $1}'", record_path);
			run_cmd(cmd, buf, sizeof(buf));
			if (buf[0] < '0' || buf[0] > '9') {
				record_size = 0;
			} else {
				record_size = (unsigned long long)atoi(buf) * 1024;
			}
		}
	} else {
		reset_sdcard_static();
	}
}

void sdcard_check(int commit)
{
	sdcard_on = is_sdcard_on();
	int ret = 0;
	if (!sdcard_check_get_path()) {
		if (is_sdcard_full()) {
			while (is_sdcard_full()) {
				ret = loop_record_delete();
				if (0 == ret) {
					is_sdcard_ok = 1;
					set_sdcard_status(commit, 0);
					log_info("process sdcard full ok.\n");
				} else if (1 == ret) {
					if (is_sdcard_full()) {
						is_sdcard_ok = 0;
						set_sdcard_status(commit, 2);
						log_info("There is nothing to be removed, just wait.....\n");
					} else {
						is_sdcard_ok = 1;
						set_sdcard_status(commit, 0);
						log_info("process sdcard full ok.\n");
					}
					break;
				} else {
					is_sdcard_ok = 0;
					set_sdcard_status(commit, 2);
					log_info("process sdcard full FAILD!!!\n");
					break;
				}
			}
		} else {
			is_sdcard_ok = 1;
			set_sdcard_status(commit, 0);
		}
		get_sdcard_static();
		/*check_sdcard_filesystem(); */
	} else {
		is_sdcard_ok = 0;
		if (sdcard_on) {
			if (5 != get_sdcard_status()) {
				set_sdcard_status(commit, 3);
			}
		} else {
			set_sdcard_status(commit, 1);
		}
		log_info("***sdcard_check path fail!****\n");
	}
}

int get_format_flag(void)
{
	return is_format;
}

void set_format_flag(int value)
{
	is_format = value;
}

void sdcard_umount(int id, char *ackbuf)
{
	if (NULL == ackbuf)
		return;
	stop_record();
	char cmd[128] = { '\0' };
	char buf[128] = { '\0' };
	int i = 0, retry_times = 5;

	//fuser check
	sprintf(cmd, "fuser -m %s 2>&1", sdcard_path);
	log_debug("fuser: cmd is %s\n", cmd);

	for (i = 0; i < retry_times; i++) {
		run_cmd(cmd, buf, sizeof(buf));
		if (0 == strlen(buf)) {
			log_debug("No one is using sdcard.\n");
			break;
		} else {
			log_info("fuser result is %s\n", buf);
		}
		memset(buf, 0, sizeof(buf));
	}
	if (retry_times == i) {
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2004,\"message\":\"fuser fail\"}}", id);
		sdcard_check(1);
		set_enable_record(1);
		return;
	}
	//umount
	sprintf(cmd, "umount %s 2>&1 && rm -fr /mnt/sdcard/*", sdcard_path);
	memset(buf, 0, sizeof(buf));
	for (i = 0; i < retry_times; i++) {
		run_cmd(cmd, buf, sizeof(buf));
		if (0 == strlen(buf)) {
			log_debug("umount SD done.\n");
			break;
		} else {
			log_info("umount sdcard result is %s\n", buf);
		}
		memset(buf, 0, sizeof(buf));
	}
	if (retry_times == i) {
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2004,\"message\":\"umount fail\"}}", id);
		sdcard_check(1);
		set_enable_record(1);
		return;
	}
	sprintf(ackbuf, "{\"id\":%d,\"result\":[\"OK\"]}", id);
	set_sdcard_status(0, 5);
}

void *sdcard_format_thread(void *arg)
{
	set_sdcard_status(0, 4);
	int ret = 0;
	ret = prctl(PR_SET_NAME, "av_SDformat\0", NULL, NULL, NULL);
	if (ret != 0) {
		log_err("prctl setname failed\n");
	}
	log_debug("sdcard format thread start ok.\n");
	char cmd[128] = { '\0' };
	char buf[128] = { '\0' };
	int i = 0, retry_times = 5;
	const char *key = "scard.exception";

	set_format_flag(1);
	set_enable_record(0);
	if (get_sdcard_on() && (!is_sdcard_avail())) {
		log_debug("No need to umount sdcard.\n");
	} else {
		stop_record();
		sleep(1);

		//fuser check
		sprintf(cmd, "fuser -mk %s 2>&1", sdcard_path);
		log_debug("fuser: cmd is %s\n", cmd);

		for (i = 0; i < retry_times; i++) {
			run_cmd(cmd, buf, sizeof(buf));
			if (0 == strlen(buf)) {
				log_debug("No one is using sdcard.\n");
				break;
			} else {
				log_debug("fuser result is %s, kill!\n", buf);
			}
			memset(buf, 0, sizeof(buf));
		}
		if (retry_times == i) {
			set_format_flag(-1);
			log_info(" fuser fail!!!!!!!!!!!!!!!!!!!!\n");
			sdcard_check(1);
			set_enable_record(1);
			return NULL;
		}
		//umount
		sprintf(cmd, "umount %s 2>&1 && rm -fr /mnt/sdcard/*", sdcard_path);
		cmd[strlen(cmd)] = '\0';
		log_debug("umount: cmd is %s\n", cmd);

		for (i = 0; i < retry_times; i++) {
			run_cmd(cmd, buf, sizeof(buf));
			if (0 == strlen(buf)) {
				log_debug("umount SD done.\n");
				break;
			} else {
				log_info("umount sdcard result is %s\n", buf);
			}
			memset(buf, 0, sizeof(buf));

		}
		if (retry_times == i) {
			set_format_flag(-1);
			log_err(" umount fail!!!!!!!!!!!!!!!!!!!!\n");
			sdcard_check(1);
			set_enable_record(1);
			return NULL;
		}

	}

	/* temporary disable hotplug */
	run_cmd("echo '' > /proc/sys/kernel/hotplug", NULL, 0);

	run_cmd("echo -e \"d\nn\np\n1\n\n\nt\nc\nw\n\" | fdisk /dev/mmcblk0", NULL, 0);
	memset(buf, 0, sizeof(buf));
	run_cmd("mkfs.vfat /dev/mmcblk0p1 2>&1", buf, sizeof(buf));
	if (0 == strlen(buf)) {
		log_debug("mkfs.vfat: format SD done.\n");
	} else {
		log_info("mkfs.vfat: format sdcard result is %s\n", buf);
	}

	/* re-enable hotplug */
	run_cmd("echo /sbin/mdev > /proc/sys/kernel/hotplug", NULL, 0);
	run_cmd("mdev -s", NULL, 0);

	create_record_dir();
	sdcard_check(1);
	if (is_sdcard_avail()) {
		set_enable_record(1);
	}
	set_format_flag(0);
	pthread_exit(0);
}

void exec_format_thread(void)
{
	int ret;

	pthread_t FormatSDThread_ID;

	if ((ret = pthread_create(&FormatSDThread_ID, NULL, &sdcard_format_thread, NULL))) {
		log_err("pthread_create ret=%d\n", ret);
		exit(-1);
	}
	pthread_detach(FormatSDThread_ID);
}

void create_record_dir(void)
{
	if (is_sdcard_ok) {
		char record_path[MAX_FILEPATH_LEN] = { 0 };
		get_sdcard_path(record_path);
		if (mkdir(record_path, 0666) && EEXIST != errno) {
			is_sdcard_ok = 0;
			perror("create record dir fail");
		}
	} else {
		log_err("sdcard is invalid!\n");
	}
}

void *thread_sdcard_check(void *arg)
{
	int ret = 0;
	ret = prctl(PR_SET_NAME, "av_TFcheck\0", NULL, NULL, NULL);
	if (ret != 0) {
		log_err("prctl setname failed\n");
	}

	struct sockaddr_nl nls;
	struct pollfd pfd;
	char buf[128];
	char remove[53] = "remove@/devices/soc0/soc/soc:sdmmc/mmc_host/mmc0/mmc0";
	char add[50] = "add@/devices/soc0/soc/soc:sdmmc/mmc_host/mmc0/mmc0";
	int cnt = 0;
	// Open hotplug event netlink socket
	memset(&nls, 0, sizeof(struct sockaddr_nl));
	nls.nl_family = AF_NETLINK;
	nls.nl_pid = getpid();
	nls.nl_groups = 1;

	pfd.events = POLLIN;
	pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (pfd.fd == -1) {
		log_err("not root\n");
		return NULL;
	}
	// Listen to netlink socket
	if (bind(pfd.fd, (void *)&nls, sizeof(struct sockaddr_nl))) {
		log_err("bind error\n");
		return NULL;
	}
	log_info("sdcard check thread ok~\n");
	while (gProcessRun) {
		int poll_ret = poll(&pfd, 1, 1 * 60 * 1000);
		if (0 == poll_ret) {	//timeout
			if (get_format_flag()) {
				sleep(1);
				continue;
			}
			if (cnt >= 5) {
				ret = get_sdcard_status();
				if (3 != ret && 5 != ret) {
					log_info("start common sdcard check~\n");
					sdcard_check(0);
					nvram_get_power_value();
				}
				cnt = 0;
			} else {
				cnt++;
			}
			get_motionalarm_value();
		} else if (poll_ret > 0) {
			int len = recv(pfd.fd, buf, sizeof(buf), MSG_DONTWAIT);
			if (-1 == len || get_format_flag()) {
				sleep(1);
				continue;
			}
			if (0 == strncmp(buf, add, sizeof(add))) {
				log_info("sdcard has been add! \n");
				if (!is_sdcard_avail()) {
					usleep(200 * 1000);	//wait for mount action
					sdcard_check(0);
					if (is_sdcard_avail()) {
						create_record_dir();
						set_enable_record(1);
					}
					set_sdcard_on(1);
				}
			} else if (0 == strncmp(buf, remove, sizeof(remove))) {
				is_sdcard_ok = 0;
				is_recording = 0;
				set_sdcard_on(0);
				set_enable_record(0);
				memset((void *)sdcard_path, '\0', sizeof(sdcard_path));
				set_sdcard_status(0, 1);
				reset_sdcard_static();
				log_info("sdcard has been removed!\n");
			} else {
				log_info("poll data is %s\n", buf);
			}
		}
	}
	pthread_exit(0);
}

void create_sdcheck_thread(void)
{
	int ret;

	pthread_t ThreadSDcardCheck_ID;

	if ((ret = pthread_create(&ThreadSDcardCheck_ID, NULL, &thread_sdcard_check, NULL))) {
		log_err("pthread_create ret=%d\n", ret);
		exit(-1);
	}
	pthread_detach(ThreadSDcardCheck_ID);

}

static volatile int motion_alarm_flag = 0;
static volatile int motion_alarm_start_record = 0;
static int is_upload_video = 0;
static int motion_alarm_interval = 5;	//units is mins,default value is 5 mins

static const char hex_table_uc[16] = { '0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'A', 'B',
	'C', 'D', 'E', 'F'
};

static const char hex_table_lc[16] = { '0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'a', 'b',
	'c', 'd', 'e', 'f'
};

char *encodeToHex(char *buff, const uint8_t * src, int len, int type)
{
	int i;

	const char *hex_table = type ? hex_table_lc : hex_table_uc;

	for (i = 0; i < len; i++) {
		buff[i * 2] = hex_table[src[i] >> 4];
		buff[i * 2 + 1] = hex_table[src[i] & 0xF];
	}

	buff[2 * len] = '\0';

	return buff;
}

int get_str_md5(unsigned char *str, int str_len, unsigned char *str_md5)
{
	if (NULL == str || NULL == str_md5)
		return -1;
	unsigned char md[16];
	unsigned char tmp[3] = { '\0' }, buf[33] = {
	'\0'};

	MD5(str, str_len, md);

	int i;
	for (i = 0; i < 16; i++) {
		sprintf((char *)tmp, "%2.2x", md[i]);
		strcat((char *)buf, (char *)tmp);
	}
	memcpy(str_md5, buf, strlen((char *)buf));
	return 0;
}

time_t get_last_alarm_time()
{
	nvram_param_t params = {0x00};
	params.key = "last_alarm_time";
	params.type = X_TYPE_STRING;
	nvram_get(&params);
	if (!params.result) {
		log_err("do_nvram_get last_alarm_time error\n");
		return 0;
	}
	time_t t = atol(params.value);
	log_info("nvram get last_alarm_time %d \n", t);
	return t;
}

void set_last_alarm_time(time_t t)
{
	char cmd[64] = {0x00};
	sprintf(cmd, "%d", t);
	nvram_param_t params = {0x00};
	params.key = "last_alarm_time";
	nvram_set(&params, cmd);
	if (params.result) {
		log_info("set last_alarm_time = %s ok!\n", cmd);
		//nvram_commit();
	} else {
		log_err("set last_alarm_time = %s FAIL!\n", cmd);
	}
}

char *get_p2p_token(char *token)
{
	token = NULL;
	nvram_param_t params = { 0 };
	params.key = "miio_token";
	params.type = X_TYPE_STRING;
	nvram_get(&params);
	if (!params.result) {
		log_err("do_nvram_get miio_token error\n");
		return token;
	}
	log_debug("token is %s \n", params.value);
	token = malloc(2 * strlen(params.value) + 1);
	if (!token) {
		log_err("malloc for token error\n");
		return token;
	}
	encodeToHex(token, (uint8_t *) params.value, strlen(params.value), 1);
	return token;
}

int set_upload_pwd(const char *pwd, int flag)
{
	if (NULL == pwd)
		return -1;
	if (flag) {
		sprintf((char *)&(upload_params.pwd), "%s", pwd);
	} else {
		sprintf((char *)&(upload_params.pic_pwd), "%s", pwd);
	}
	return 0;
}

int set_upload_object_name(const char *obj_name, int flag)
{
	if (NULL == obj_name)
		return -1;
	if (flag) {
		sprintf((char *)&(upload_params.object_name), "%s", obj_name);
	} else {
		sprintf((char *)&(upload_params.pic_object_name), "%s", obj_name);
	}
	return 0;
}

int set_upload_url(const char *url, int flag)
{
	if (NULL == url)
		return -1;
	if (flag) {
		sprintf((char *)&(upload_params.url), "%s", url);
		upload_params.get_url_ok = 1;
	} else {
		sprintf((char *)&(upload_params.pic_url), "%s", url);
		upload_params.get_pic_url_ok = 1;
	}
	return 0;
}

int encrypt_alarm_file(char *filename, int is_video)
{
#define BLK_SIZE 4096
	unsigned char pwd_md5[33] = { '\0' };
	char *tmpfile = "/tmp/tmpfile.xyz";
	unsigned char ibuf[BLK_SIZE];
	unsigned char *obuf;
	EVP_CIPHER_CTX ctx;
	unsigned char iv[AES_BLOCK_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int ilen, olen;
	int blocksize;
	int ret;

	struct stat statbuf;
	FILE *fp_in = NULL;
	FILE *fp_out = NULL;

	stat(filename, &statbuf);
	if (0 == statbuf.st_size) {
		log_err("%s size is zero!\n", filename);
		return -1;
	} else
		log_debug("%s size: %u\n", filename, statbuf.st_size);

	if (is_video) {
		get_str_md5((unsigned char *)&(upload_params.pwd), strlen((char *)&(upload_params.pwd)), pwd_md5);
		log_debug("%s'md5 is %s, md5 len is %d\n", &(upload_params.pwd), pwd_md5, strlen((char *)pwd_md5));
	} else {
		get_str_md5((unsigned char *)&(upload_params.pic_pwd), strlen((char *)&(upload_params.pic_pwd)), pwd_md5);
		log_debug("%s'md5 is %s, md5 len is %d\n", &(upload_params.pic_pwd), pwd_md5, strlen((char *)pwd_md5));
	}

	EVP_CipherInit(&ctx, EVP_aes_256_cbc(), pwd_md5, iv, 1);
	blocksize = EVP_CIPHER_CTX_block_size(&ctx);
	obuf = malloc(sizeof(ibuf) + blocksize);
	if (obuf == NULL) {
		log_err("malloc fail: %m\n");
		return -1;
	}

	fp_in = fopen(filename, "r");
	if (NULL == fp_in) {
		log_err("can't open the file: %s, %m\n", filename);
		free(obuf);
		return -1;
	}
	fp_out = fopen(tmpfile, "w");
	if (NULL == fp_out) {
		log_err("can't open file: %s, %m\n", tmpfile);
		fclose(fp_in);
		free(obuf);
		return -1;
	}

	while ((ilen = fread(ibuf, 1, BLK_SIZE, fp_in)) > 0) {
		ret = EVP_CipherUpdate(&ctx, obuf, &olen, ibuf, ilen);
		if (ret != 1) {
			EVP_CIPHER_CTX_cleanup(&ctx);
			log_err("EVP_CipherUpdate failed\n");
			ret = -1;
			goto close_out;
		}

		if (olen == 0)
			continue;

		/* TODO: check partial write */
		ret = fwrite(obuf, 1, olen, fp_out);
		if (ret == 0) {
			log_err("fwrite return 0: %m\n");
			ret = -1;
			goto close_out;
		}
	}

	ret = EVP_CipherFinal_ex(&ctx, obuf, &olen);
	EVP_CIPHER_CTX_cleanup(&ctx);
	if (ret != 1)
		log_err("EVP_CipherFinal_ex failed\n");

	fwrite(obuf, 1, olen, fp_out);

	remove(filename);
	if (rename(tmpfile, filename) != 0) {
		log_err("rename error: %s -> %s, %m\n", tmpfile, filename);
		return -1;
	}

	fclose(fp_in);
	fclose(fp_out);
	free(obuf);
	return 0;

close_out:
	fclose(fp_in);
	fclose(fp_out);
	free(obuf);
	return ret;
}

time_t last_alarm_time = 0;
int time_to_alarm(void)
{
	if (last_alarm_time == 0) {
		last_alarm_time = get_last_alarm_time();
	}
	if (time(NULL) - last_alarm_time > motion_alarm_interval * 60) {
		return 1;
	} else {
		return 0;
	}
}

void *thread_MotionEvent(void *arg)
{
	motion_alarm_thread_done = 1;
	int ret = prctl(PR_SET_NAME, "av_motionevent\0", NULL, NULL, NULL);
	if (ret != 0) {
		log_err("prctl setname failed\n");
	}
	log_info("MotionEvent thread start~\n");
	int i = 3;
	while (i--) {
		request_pic_url();
		send_to_get_url();
		sleep(1);
		if (upload_params.get_url_ok && upload_params.get_pic_url_ok) {
			log_debug("##########get url is ok###########\n");
			upload_motion_alarm(&upload_params);
			break;
		} else {
			sleep(1);
		}
	}
	last_alarm_time = time(NULL);
	set_last_alarm_time(last_alarm_time);
	pthread_exit(0);
}

void *thread_MotionAlarm(void *arg)
{
	int ret = prctl(PR_SET_NAME, "av_motionalarm\0", NULL, NULL, NULL);
	if (ret != 0) {
		log_err("prctl setname failed\n");
	}
	log_info("MotionAlarm thread start ok~\n");

	int upload_result = 1;
	if (get_motionalarm_flag()) {
		if (!make_alarm_mp4_pre()) {
			if (1 == upload_params.get_url_ok) {
				if (0 == encrypt_alarm_file("/tmp/alarm.mp4", 1)) {
					upload_result = mcurl_upload("/tmp/alarm.mp4", upload_params.url);
				} else {
					log_err("encrypt alarm video file fail !\n");
				}
			} else {
				log_info("!!!!!!!!!!!!!get upload video url fail!!!!!!!!!!!!\n");
			}
			if (!upload_result) {
				if (1 == upload_params.get_pic_url_ok) {
					if (0 == encrypt_alarm_file("/tmp/alarm.jpeg", 0)) {
						mcurl_upload("/tmp/alarm.jpeg", upload_params.pic_url);
					} else {
						log_info("encrypt alarm picture file fail\n");
					}
				} else {
					log_info("!!!!!!!!!!!!!get upload picture url fail!!!!!!!!!!!!\n");
				}
			} else {
				log_err("alarm video file upload fail !\n");
			}
		}
	}

	alarm_buf_reset(&audio_alarm_buf, &video_alarm_buf);
	memset(&upload_params, 0, sizeof(upload_params));
	set_upload_video_value(0);

	sleep(1);
	if (remove("/tmp/alarm.mp4"))
		log_warning("remove alarm.mp4 fail: %m\n");
	if (remove("/tmp/alarm.jpeg"))
		log_warning("remove alarm.jpeg fail: %m\n");

	/*if (motion_alarm_flag) { */
	/*motion_alarm_thread_done = 2; */
	/*sleep(motion_alarm_interval * 60); */
	/*} */
	motion_alarm_thread_done = 0;
	set_alarm_start_flag(0);
	log_debug("alarm thread done, quit\n");
	pthread_exit(0);
}

pthread_t MotionEvent_ID;
void create_MotionEvent_thread(void)
{
	int ret;

	if ((ret = pthread_create(&MotionEvent_ID, NULL, &thread_MotionEvent, NULL))) {
		log_err("pthread_create ret=%d\n", ret);
		exit(-1);
	}
	pthread_detach(MotionEvent_ID);
}

pthread_t MotionAlarm_ID;
void create_MotionAlarm_thread(void)
{
	int ret;

	if ((ret = pthread_create(&MotionAlarm_ID, NULL, &thread_MotionAlarm, NULL))) {
		log_err("pthread_create ret=%d\n", ret);
		exit(-1);
	}
	pthread_detach(MotionAlarm_ID);
}

pthread_t get_motionalarm_thread_id(void)
{
	return MotionAlarm_ID;
}

void set_motion_alarm_done(int value)
{
	motion_alarm_thread_done = value;
}

int is_motion_alarm_done(void)
{
	return motion_alarm_thread_done;
}

void set_upload_video_value(int value)
{
	upload_video = value;
}

int get_upload_video_value(void)
{
	return upload_video;
}

int alarm_params_parse(char *data, int len, int nth)
{
	if (NULL == data)
		return 0;
	int num = 0;
	int i = 0;
	int comma_num = 0;
	for (i = 0; i < len; i++) {
		if (',' == data[i]) {
			comma_num++;
			if (comma_num == nth) {
				return num;
			} else {
				num = 0;
				continue;
			}
		}
		if (data[i] > 57 || data[i] < 48)
			return num;

		num *= 10;
		num += data[i] - 48;
	}
	return num;
}

void get_motionalarm_value(void)
{
	nvram_param_t params = { 0 };
	params.key = "motion_alarm";
	params.type = X_TYPE_STRING;
	nvram_get(&params);

	int start_hour;
	int start_min;
	int end_hour;
	int end_min;

	if (params.result) {
		motion_alarm_interval = alarm_params_parse(&params.value[3], strlen(params.value) - 3, 6);
		upload_params.push_flag = alarm_params_parse(&params.value[3], strlen(params.value) - 3, 5);
		if (0 == motion_alarm_interval) {
			motion_alarm_interval = 5;
		}

		is_upload_video = 1;
		if ('0' == params.value[1]) {
			motion_alarm_flag = 0;
		} else if ('1' == params.value[1]) {
			motion_alarm_flag = 1;
		} else if ('2' == params.value[1]) {
			time_t timenow = time(NULL);
			struct tm time_now = { 0 };
			localtime_r(&timenow, &time_now);
			start_hour = alarm_params_parse(&params.value[3], strlen(params.value) - 3, 1);
			start_min = alarm_params_parse(&params.value[3], strlen(params.value) - 3, 2);
			end_hour = alarm_params_parse(&params.value[3], strlen(params.value) - 3, 3);
			end_min = alarm_params_parse(&params.value[3], strlen(params.value) - 3, 4);
			int diff = 0;
			diff = end_hour - start_hour;
			if (diff > 0) {
				if (start_hour < time_now.tm_hour && end_hour > time_now.tm_hour) {
					motion_alarm_flag = 1;
				} else if (time_now.tm_hour == start_hour && time_now.tm_min >= start_min) {
					motion_alarm_flag = 1;
				} else if (time_now.tm_hour == end_hour && time_now.tm_min <= end_min) {
					motion_alarm_flag = 1;
				} else {
					motion_alarm_flag = 0;
				}
			} else if (diff < 0) {
				if (start_hour < time_now.tm_hour || end_hour > time_now.tm_hour) {
					motion_alarm_flag = 1;
				} else if (time_now.tm_hour == start_hour && time_now.tm_min >= start_min) {
					motion_alarm_flag = 1;
				} else if (time_now.tm_hour == end_hour && time_now.tm_min <= end_min) {
					motion_alarm_flag = 1;
				} else {
					motion_alarm_flag = 0;
				}
			} else {
				if (start_min < end_min && time_now.tm_min >= start_min && time_now.tm_min <= end_min) {
					motion_alarm_flag = 1;
				} else if (start_min > end_min && time_now.tm_min >= start_min) {
					motion_alarm_flag = 1;
				} else if (start_min == end_min) {
					motion_alarm_flag = 1;
				} else {
					motion_alarm_flag = 0;
				}
			}
		}
	} else {
		log_err("nvram_get motion_alarm error\n");
	}
	log_info("interval is %d, motion_alarm_flga is %d, push flag is %d\n", motion_alarm_interval, motion_alarm_flag,
		 upload_params.push_flag);
	if (0 == motion_alarm_flag && !get_upload_video_value()) {
		usleep(5 * 1000);
		alarm_buf_reset(&audio_alarm_buf, &video_alarm_buf);
	}
}

int fsck_repair(void)
{
	stop_record();
	char cmd[128] = { '\0' };
	char buf[64] = { '\0' };
	int i = 0, retry_times = 5;
	int ret = 0;
	//umount
	sprintf(cmd, "umount %s 2>&1", sdcard_path);
	memset(buf, 0, sizeof(buf));
	for (i = 0; i < retry_times; i++) {
		run_cmd(cmd, buf, sizeof(buf));
		if (0 == strlen(buf)) {
			log_info("umount SD done.\n");
			break;
		} else {
			log_err("umount sdcard result is %s\n", buf);
		}
		memset(buf, 0, sizeof(buf));
	}
	if (retry_times == i) {
		ret = -1;
	} else {
		//fsck repair
		sprintf(cmd, "fsck.vfat -aw /dev/mmcblk0p1 &>/dev/null && echo $?");
		memset(buf, 0, sizeof(buf));
		run_cmd(cmd, buf, sizeof(buf));
		if (0 == strlen(buf)) {
			ret = -1;
		} else {
			if (0 == strncmp(buf, "0", 1) || 0 == strncmp(buf, "1", 1)) {
				ret = 0;
			} else {
				ret = -1;
			}
		}
		run_cmd("mdev -s", NULL, 0);
	}
	return ret;
}

void set_alarm_start_flag(int value)
{
	motion_alarm_start_record = value;
}

int is_alarm_start(void)
{
	return motion_alarm_start_record;
}

int get_motionalarm_flag(void)
{
	return (motion_alarm_flag || get_upload_video_value());
}

int get_write_flag(void)
{
	return motion_alarm_start_record;
}

void alarm_record_over(void)
{
	set_alarm_start_flag(0);
}

void get_motion_params(void)
{
	get_motionrecord_value();
	get_motionalarm_value();
}

int get_push_flag(void)
{
	/*
	log_debug("motion_record_flag = %d\n", motion_record_flag);
	log_debug("start_record_flag = %d\n", start_record_flag);
	log_debug("is_recording = %d\n", is_recording);
	log_debug("is_sdcard_ok = %d\n", is_sdcard_ok);
	log_debug("get_format_flag() = %d\n", get_format_flag());
	log_debug("app_enable_record = %d\n", app_enable_record);
	log_debug("enable_record = %d\n", enable_record);
	*/
	return ((motion_record_flag || start_record_flag || is_recording) && is_sdcard_ok && (!get_format_flag()) &&
		app_enable_record && enable_record);
}

void set_poweroff_value(int value)
{
	power_off = value;
}

int get_poweroff_value(void)
{
	return power_off;
}

void nvram_get_power_value(void)
{
	nvram_param_t params = { 0 };
	params.key = "power";
	params.type = X_TYPE_STRING;
	nvram_get(&params);
	if (params.result) {
		if (0 == strncmp(params.value, "on", 2)) {
			set_poweroff_value(1);
		} else if (0 == strncmp(params.value, "off", 3)) {
			set_poweroff_value(0);
		}
		log_info("power is %s\n", params.value);
	} else {
		log_err("nvram_get power error\n");
	}
}

static int write_record_msg(time_t * timestamp, unsigned char motion_flag, unsigned char duration, int flag)
{
	if (NULL == timestamp)
		return -1;
	RecordFileMsg_t record_msg_tmp = { 0 };
	record_msg_tmp.duration = duration;
	record_msg_tmp.timestamp = *timestamp;
	record_msg_tmp.motion_flag = motion_flag;
	time_t tmp = 0;
	char record_timestamp[MAX_FILEPATH_LEN] = { 0 };

	int ret = get_record_msg_path(record_timestamp);
	if (!ret) {
		pthread_mutex_lock(&timefile_lock);
		FILE *timestamp_fp = NULL;
		if (flag) {
			timestamp_fp = fopen(record_timestamp, "r+");
			if (timestamp_fp == NULL) {
				log_err("%s:%d, fopen error: %m\n", __func__, __LINE__);
				pthread_mutex_unlock(&timefile_lock);
				return -1;
			}
			fseek(timestamp_fp, -sizeof(RecordFileMsg_t), SEEK_END);
			fread(&tmp, sizeof(time_t), 1, timestamp_fp);
			if (tmp == *timestamp) {
				fseek(timestamp_fp, -sizeof(RecordFileMsg_t), SEEK_END);
				fwrite(&record_msg_tmp, sizeof(RecordFileMsg_t), 1, timestamp_fp);
			} else {
				fclose(timestamp_fp);
				pthread_mutex_unlock(&timefile_lock);
				log_err("write_record_msg: the last timestamp check error!\n");
				return -1;
			}
		} else {
			timestamp_fp = fopen(record_timestamp, "a+");
			if (timestamp_fp == NULL) {
				log_err("%s:%d, fopen error: %m\n", __func__, __LINE__);
				pthread_mutex_unlock(&timefile_lock);
				return -1;
			}
			fseek(timestamp_fp, 0, SEEK_END);
			fwrite(&record_msg_tmp, sizeof(RecordFileMsg_t), 1, timestamp_fp);
		}
		fclose(timestamp_fp);
		pthread_mutex_unlock(&timefile_lock);
	} else {
		log_err("write_record_msg: get record timestamp error!\n");
		return -1;
	}
	return 0;
}

static int write_g711_mp4(unsigned char *buf, int size, mp4_record_params_t * mp4_params)
{
	if (!buf || !size) {
		log_err("write g711 error:buf or size is null\n");
		return -1;
	}
	if (!MP4WriteSample(mp4_params->mp4_handle, mp4_params->audio_track_id, buf, size, size, 0, 0)) {
		log_err("write g711 sample error!\n");
		return -1;
	}
	return 0;
}

static int write_h265_mp4(unsigned char *nalu_data, int nal_type, unsigned int nalu_len, unsigned int start_code_len,
			  mp4_record_params_t * mp4_params)
{
	int sampleLenFieldSizeMinusOne = 3;	// 4 bytes length before each NAL unit
	if (start_code_len == 3)
		sampleLenFieldSizeMinusOne = 2;

	unsigned char *data = nalu_data + start_code_len;
	unsigned int len = nalu_len - start_code_len;

	char *new_buf = NULL;
	switch (nal_type) {
	case NAL_VPS:
		if (mp4_params->is_need_set) {	//if the vps sps pps and so on they are not in a key frame then this logic need change.
			mp4_params->video_track_id = MP4AddH265VideoTrack(mp4_params->mp4_handle, mp4_params->time_scale, mp4_params->time_scale / mp4_params->fps, mp4_params->width, mp4_params->height,	// height
									  data[1],	// sps[1] AVCProfileIndication
									  data[2],	// sps[2] profile_compat
									  data[3],	// sps[3] AVCLevelIndication
									  sampleLenFieldSizeMinusOne);
			if (mp4_params->video_track_id == MP4_INVALID_TRACK_ID) {
				log_err("vps:add h265 video track failed.\n");
				return -1;
			}
			MP4SetVideoProfileLevel(mp4_params->mp4_handle, 0x7F);
			MP4AddH265VideoParameterSet(mp4_params->mp4_handle, mp4_params->video_track_id, data, len, 0);
			MP4SetTrackDurationPerChunk(mp4_params->mp4_handle, mp4_params->video_track_id,
						    mp4_params->time_scale / mp4_params->fps);
		}
		break;
	case NAL_SPS:
		if (mp4_params->is_need_set) {
			if (mp4_params->video_track_id == MP4_INVALID_TRACK_ID) {
				log_err("sps: track id is invalid!\n");
				return -1;
			} else {
				MP4AddH265SequenceParameterSet(mp4_params->mp4_handle, mp4_params->video_track_id, data,
							       len, 1);
			}
		}
		break;
	case NAL_PPS:
		if (mp4_params->is_need_set) {
			if (mp4_params->video_track_id == MP4_INVALID_TRACK_ID) {
				log_err("pps: track id is invalid!\n");
				return -1;
			} else {
				MP4AddH265PictureParameterSet(mp4_params->mp4_handle, mp4_params->video_track_id, data,
							      len, 1);
			}
		}

		break;
	case NAL_SEI_PREFIX:
		/*if (mp4_params->key_frame) { */
		/*mstar_sei_data[0] = len >> 24; */
		/*mstar_sei_data[1] = len >> 16; */
		/*mstar_sei_data[2] = len >> 8; */
		/*mstar_sei_data[3] = len & 0xff; */
		/*memcpy(mstar_sei_data + 4, data, len); */
		/*mstar_sei_data[len + 4] = '\0'; */
		/*mp4_params->sei_len = len + 4; */
		/*} */
		break;
	case NAL_TRAIL_N:
	case NAL_TRAIL_R:
	case NAL_TSA_N:
	case NAL_TSA_R:
	case NAL_STSA_N:
	case NAL_STSA_R:
	case NAL_RADL_N:
	case NAL_RADL_R:
	case NAL_RASL_N:
	case NAL_RASL_R:
	case NAL_BLA_W_LP:
	case NAL_BLA_W_RADL:
	case NAL_BLA_N_LP:
	case NAL_IDR_W_RADL:
	case NAL_IDR_N_LP:
	case NAL_CRA_NUT:
		if (mp4_params->video_track_id == MP4_INVALID_TRACK_ID) {
			log_err("nalu: invalid track id\n");
			return -1;
		} else {
			unsigned char *tmp_data = NULL;
			unsigned int tmp_len = 0;

			nalu_data[0] = len >> 24;
			nalu_data[1] = len >> 16;
			nalu_data[2] = len >> 8;
			nalu_data[3] = len & 0xff;
			if (mp4_params->sei_len > 0 && !mp4_params->key_frame) {	//only p frame write sei
				//need to write sei
				if (mp4_params->sei_len + nalu_len > sizeof(mstar_sei_data)) {
					//need malloc more memory
					new_buf = malloc(mp4_params->sei_len + nalu_len);
					if (NULL == new_buf) {
						log_err("cant malloc for sei data\n");
						return -1;
					}
					memcpy(new_buf, mstar_sei_data, mp4_params->sei_len);
					memcpy(new_buf + mp4_params->sei_len, nalu_data, nalu_len);
					tmp_data = new_buf;
				} else {
					memcpy(mstar_sei_data + mp4_params->sei_len, nalu_data, nalu_len);
					tmp_data = nalu_data;
				}
				tmp_len = nalu_len + mp4_params->sei_len;
				mp4_params->sei_len = 0;
			} else {
				tmp_data = nalu_data;
				tmp_len = nalu_len;
			}

			//nalu_data[4], nalu_data[5]  this two byte is nalu header,
			// mod the 5th byte first bit for mark whether it is a special frame which can be dropped
			if (1 == mp4_params->dispose_flag) {	// this code is platform related, just for Mstar  TODO
				nalu_data[4] |= 0x80;
			}

			if (!MP4WriteSample(mp4_params->mp4_handle, mp4_params->video_track_id,
					    tmp_data, tmp_len, MP4_INVALID_DURATION, 0, mp4_params->key_frame)) {
				log_err("mp4 sample write error!\n");
				if (new_buf)
					free(new_buf);
				return 1;
			}
			if (new_buf)
				free(new_buf);
		}
		break;
	default:
		break;
	}

	return 0;
}

static int find_start_code3(unsigned char *buf, int *start_code_len)
{
	if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01) {
		return 0;
	} else {
		*start_code_len = 3;
		return 1;
	}
}

static int find_start_code4(unsigned char *buf, int *start_code_len)
{
	if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x00 || buf[3] != 0x01) {
		return 0;
	} else {
		*start_code_len = 4;
		return 1;
	}
}

static int find_nalu_write(unsigned char *buf, unsigned int len, mp4_record_params_t * mp4_params)
{
	if (NULL == buf || 0 == len) {
		log_err("buf is NULL, or len is zero\n");
		return -2;
	}

	unsigned nalu_num = 0;
	unsigned int process_len = 0;
	int pos = 0;
	int start_code_len = 0;
	int find_next_code = 0;
	int nal_unit_type = 0;
	unsigned int nalu_len = 0;
	int ret = 0;
	while (1) {
		if (!find_start_code4(buf + process_len, &start_code_len)) {
			if (!find_start_code3(buf + process_len, &start_code_len)) {
				log_err("There is no start code in frame data!\n");
				return -1;
			} else {
				pos = 3;
			}
		} else {
			pos = 4;
		}
		find_next_code = 0;

		while (!find_next_code) {
			if ((process_len + (pos + 3)) >= len) {
				nalu_len = (len - process_len);
				nalu_num++;
				nal_unit_type = ((buf[process_len + start_code_len]) & 0x7e) >> 1;	// 5 bit
				/*printf("the %dth nalu type is %d, data index is %d, nalu len is %d\n",nalu_num, nal_unit_type, process_len, nalu_len); */
				ret =
				    write_h265_mp4(&buf[process_len], nal_unit_type, nalu_len, start_code_len,
						   mp4_params);
				return ret;
			}

			find_next_code = (find_start_code4(&buf[pos + process_len], &start_code_len)
					  || find_start_code3(&buf[pos + process_len], &start_code_len));
			pos++;
		}
		nalu_len = pos - 1;
		nalu_num++;
		process_len += pos - 1;
		nal_unit_type = ((buf[process_len + 1 - pos + start_code_len]) & 0x7e) >> 1;	// 5 bit
		/*printf("the %dth nalu type is %d , data index is %d, nalu len is %d\n",nalu_num, nal_unit_type, process_len + 1 - pos, nalu_len); */
		ret = write_h265_mp4(&buf[process_len + 1 - pos], nal_unit_type, nalu_len, start_code_len, mp4_params);
		if (1 == ret) {
			return ret;
		}
	}
}

extern char snapshot_record_path[MAX_FILEPATH_LEN];
extern int print_len_picture;
static int record_init(time_t time_stamp, mp4_record_params_t * mp4_params, int flag)
{
	time_t utc_timestamp = time_stamp;
	struct tm filetime = { 0 };
	localtime_r(&utc_timestamp, &filetime);
	int print_len = 0;
	char record_path[MAX_FILEPATH_LEN] = { 0 };
	if (flag) {
		strftime(record_path, sizeof(record_path), "/tmp/MIJIA_RECORD_VIDEO/%Y%m%d%H", &filetime);
		mkdir(record_path, 0666);
		strftime(record_path, sizeof(record_path), "/tmp/MIJIA_RECORD_VIDEO/%Y%m%d%H/%MM%SS_", &filetime);
		sprintf(&record_path[strlen(record_path)], "%u.mp4", (unsigned int)utc_timestamp);
	} else {
		if (get_sdcard_path(record_path)) {
			log_err("get sdcard path error\n");
			return -1;
		}
		strftime(&(record_path[strlen(record_path)]), sizeof(record_path), "/%Y%m%d%H", &filetime);
		print_len = strlen(record_path);
		mkdir(record_path, 0666);
		strftime(&(record_path[strlen(record_path)]), sizeof(record_path), "/%MM%SS_", &filetime);
		sprintf(&record_path[strlen(record_path)], "%u.mp4", (unsigned int)utc_timestamp);
	}
	log_info("new file is %s\n", &record_path[print_len + 1]);
	print_len_picture = print_len;

	//file handle
	mp4_params->mp4_handle = MP4Create(record_path, 0);
	if (mp4_params->mp4_handle == MP4_INVALID_FILE_HANDLE) {
		log_err("creat mp4 file error\n");
		return -1;
	}
	MP4SetTimeScale(mp4_params->mp4_handle, mp4_params->time_scale);

	//audio track
	mp4_params->audio_track_id = MP4AddALawAudioTrack(mp4_params->mp4_handle, 8000);
	if (mp4_params->audio_track_id == MP4_INVALID_TRACK_ID) {
		log_err("wrong audio track id\n");
		return -1;
	}
	MP4SetTrackIntegerProperty(mp4_params->mp4_handle, mp4_params->audio_track_id,
				   "mdia.minf.stbl.stsd.alaw.channels", 1);
	MP4SetAudioProfileLevel(mp4_params->mp4_handle, 0x02);

	memcpy(snapshot_record_path, record_path, strlen(record_path));
	get_record_snapshot();
	return 0;
}

static void record_close(MP4FileHandle mp4FHandle, unsigned int flag)
{
	if (mp4FHandle) {
		MP4Close(mp4FHandle, flag);
	}
}

void stop_record(void)
{
	set_enable_record(0);
	is_recording = 0;
	sleep(1);
	if (mp4_params.mp4_handle) {
		record_close(mp4_params.mp4_handle, 0);
		mp4_params.mp4_handle = MP4_INVALID_FILE_HANDLE;
	}
}

void *thread_record_mp4(void *arg)
{
	log_info("mp4 record thread start ok~\n");

	time_t timenow;
	time_t time_latest = 0;
	time_t video_start_time = 0;
	int ret = 0;
	int next_frame_type = 0;

	static char h265_frame[512 * 1024];

	char *h265_data = NULL;
	unsigned int h265_len = 0;
	unsigned int h265_free = 0;

	char g711_frame[1024] = { '\0' };
	char *g711_data = NULL;
	unsigned int g711_len = 0;
	unsigned int g711_free = 0;

	ret = prctl(PR_SET_NAME, "mstar_TFrecord\0", NULL, NULL, NULL);
	if (ret != 0) {
		log_err("prctl setname failed\n");
	}
	sdcard_check(0);

	int err_cnt = 0;
	int force_I_frame = 1;

	if (is_sdcard_on()) {
		create_record_dir();
		// The user-controlled flag is app_enable_record, this operation do not break user's config.
		set_enable_record(1);
	}
	ret = 0;
	while (gProcessRun) {
		pthread_mutex_lock(&pre_lock);
		pthread_cond_wait(&pre_product, &pre_lock);
		//log_debug("Recode thread start to record...\n");
		time(&timenow);
		if (0 == is_recording) {
			is_recording = 1;
			recorded_video_frame = 1;
			mp4_params.audio_track_id = MP4_INVALID_TRACK_ID;
			mp4_params.video_track_id = MP4_INVALID_TRACK_ID;
			mp4_params.mp4_handle = MP4_INVALID_FILE_HANDLE;
			mp4_params.is_need_set = 1;
			g711_data = NULL;
			h265_data = NULL;

			if (video_prerecord_buf.frame_rIndex >= video_prerecord_buf.frame_num) {
				video_prerecord_buf.frame_rIndex = 0;
			}
			if (audio_prerecord_buf.frame_rIndex >= audio_prerecord_buf.frame_num) {
				audio_prerecord_buf.frame_rIndex = 0;
			}

			if (get_frame_num_pre(&video_prerecord_buf) == 0) {
				log_warning("No h265 frame available, video_prerecord_buf.frame_rIndex is invalid.\n");
			}

			if (!video_prerecord_buf.frame[video_prerecord_buf.frame_rIndex].keyframe) {
				is_recording = 0;
				if (get_frame_num_pre(&video_prerecord_buf) > 1) {
					pop_h265_queue_pre(&video_prerecord_buf);
				}
				if (get_frame_num_pre(&audio_prerecord_buf) > 1) {
					pop_g711_queue_pre(&audio_prerecord_buf);
				}
				pthread_mutex_unlock(&pre_lock);
				continue;
			}

			sync_g711_with_h265(&audio_prerecord_buf, &video_prerecord_buf);
			
			video_start_time = video_prerecord_buf.frame[video_prerecord_buf.frame_rIndex].time_I_frame;
			if (video_start_time < 1438992488) {
				/* drop this frame */
				if (get_frame_num_pre(&video_prerecord_buf) > 1) {
					pop_h265_queue_pre(&video_prerecord_buf);
				}
				if (get_frame_num_pre(&audio_prerecord_buf) > 1) {
					pop_g711_queue_pre(&audio_prerecord_buf);
				}

				is_recording = 0;
				pthread_mutex_unlock(&pre_lock);
				continue;
			}
			pthread_mutex_unlock(&pre_lock);
			if (record_init(video_start_time, &mp4_params, 0)) {
				is_recording = 0;
				set_motion_flag(0);
				log_err("%u start time file record FAILD!\n", timenow);
				err_cnt++;
				if (err_cnt >= 3) {
					if (fsck_repair()) {
						err_cnt = 0;
						set_sdcard_status(0, 3);
					} else {
						log_err("fsck cant repair the sdcard.\n");
					}
				}
				continue;
			}
			time_latest = timenow;
			if (write_record_msg(&video_start_time, 0, 60, 0)) {
				record_close(mp4_params.mp4_handle, 0);
				log_err("write time stamp error\n");
				rm_file_timestamp(video_start_time);
				is_recording = 0;
				recorded_video_frame = 0;
				mp4_params.audio_track_id = MP4_INVALID_TRACK_ID;
				mp4_params.video_track_id = MP4_INVALID_TRACK_ID;
				mp4_params.is_need_set = 1;
				continue;
			}
		} else if ((timenow - time_latest >= 60) || 1 == ret || 0 == app_enable_record) {
			/*log_debug("timenow = %d, time_latest = %d, timenow-time_latest = %d\n",
					timenow, time_latest, timenow-time_latest);
			log_debug("next_frame_type = %d\n", next_frame_type);
			*/
			if (!next_frame_type && force_I_frame) {
				force_key_frame();
				force_I_frame = 0;
			}
			if (next_frame_type || 1 == ret || 0 == app_enable_record) {
				is_recording = 0;
				pthread_mutex_unlock(&pre_lock);
				record_close(mp4_params.mp4_handle, 0);
				recorded_video_frame = 0;
				mp4_params.audio_track_id = MP4_INVALID_TRACK_ID;
				mp4_params.video_track_id = MP4_INVALID_TRACK_ID;
				mp4_params.mp4_handle = MP4_INVALID_FILE_HANDLE;
				ret = 0;
				err_cnt = 0;
				force_I_frame = 1;
				//if (!ret) {
					if (write_record_msg
					    (&video_start_time, get_motion_flag(),
					     (unsigned char)(timenow - time_latest), 1)) {
						log_err("write_record_msg error\n");
						rm_file_timestamp(video_start_time);
					}
				//} else {
				//	rm_file_timestamp(video_start_time);
				//}
				set_motion_flag(0);
				time_latest = timenow;
				continue;
			}
		}
		/*
		log_debug("timenow = %d, time_latest = %d, timenow-time_latest = %d\n",
					timenow, time_latest, timenow-time_latest);
					*/

		if (timenow - time_latest <= 50) {
			set_record_flag(0);
		}

		if (get_frame_num_pre(&video_prerecord_buf) > 1) {
			if (video_prerecord_buf.frame_rIndex >= video_prerecord_buf.frame_num) {
				video_prerecord_buf.frame_rIndex = 0;
			}

			h265_len = video_prerecord_buf.frame[video_prerecord_buf.frame_rIndex].len;
			mp4_params.key_frame = video_prerecord_buf.frame[video_prerecord_buf.frame_rIndex].keyframe;
			mp4_params.dispose_flag =
			    video_prerecord_buf.frame[video_prerecord_buf.frame_rIndex].dispose_flag;
			if (h265_len > sizeof(h265_frame)) {
				h265_data = malloc(h265_len);
				if (NULL == h265_data) {
					log_err("malloc for h265 frame date fail, size is %u\n", h265_len);
				} else {
					memcpy(h265_data,
					       video_prerecord_buf.frame[video_prerecord_buf.frame_rIndex].data_index,
					       h265_len);
					h265_free = 1;
				}
			} else {
				h265_data = h265_frame;
				memcpy(h265_data,
				       video_prerecord_buf.frame[video_prerecord_buf.frame_rIndex].data_index,
				       h265_len);
			}
			next_frame_type =
			    video_prerecord_buf.frame[video_prerecord_buf.frame_rIndex + 1].keyframe ? 1 : 0;
			pop_h265_queue_pre(&video_prerecord_buf);
		}
		//lock
		if (get_frame_num_pre(&audio_prerecord_buf) > 1) {
			if (audio_prerecord_buf.frame_rIndex >= audio_prerecord_buf.frame_num) {
				audio_prerecord_buf.frame_rIndex = 0;
			}
			g711_len = audio_prerecord_buf.frame[audio_prerecord_buf.frame_rIndex].len;
			if (g711_len > sizeof(g711_frame)) {
				g711_data = malloc(g711_len);
				if (NULL == g711_data) {
					log_err("malloc for g711 frame date fail, size is %u\n", g711_len);
				} else {
					memcpy(g711_data,
					       audio_prerecord_buf.frame[audio_prerecord_buf.frame_rIndex].data_index,
					       g711_len);
					g711_free = 1;
				}
			} else {
				g711_data = g711_frame;
				memcpy(g711_data,
				       audio_prerecord_buf.frame[audio_prerecord_buf.frame_rIndex].data_index,
				       g711_len);
			}

			pop_g711_queue_pre(&audio_prerecord_buf);
		}
		pthread_mutex_unlock(&pre_lock);
		if (h265_data) {
			ret = find_nalu_write((unsigned char *)h265_data, h265_len, &mp4_params);
			if (h265_free) {
				free(h265_data);
				h265_free = 0;
			}
			recorded_video_frame++;
			h265_data = NULL;
		}
		mp4_params.is_need_set = 0;
		if (g711_data) {
			write_g711_mp4((unsigned char *)g711_data, g711_len, &mp4_params);
			if (g711_free) {
				free(g711_data);
				g711_free = 0;
			}
			g711_data = NULL;
		}
	}

	log_info("record over normal!\n");
	record_close(mp4_params.mp4_handle, 0);
	pthread_exit(0);
}

int make_alarm_mp4_pre(void)
{
	mp4_record_params_t alarm_mp4 = {
		.width = 640,
		.height = 360,
		.fps = 20,
		.time_scale = 90000,
		.mp4_handle = MP4_INVALID_FILE_HANDLE,
		.audio_track_id = MP4_INVALID_TRACK_ID,
		.video_track_id = MP4_INVALID_TRACK_ID,
		.key_frame = 0,
		.dispose_flag = 0,
		.is_need_set = 1,
		.sei_len = 0
	};
	int is_write_keyframe = 0;
	alarm_mp4.mp4_handle = MP4Create("/tmp/alarm.mp4", 0);
	if (alarm_mp4.mp4_handle == MP4_INVALID_FILE_HANDLE) {
		log_err("alarm: creat mp4 file error\n");
		return -1;
	}
	MP4SetTimeScale(alarm_mp4.mp4_handle, alarm_mp4.time_scale);

	//audio track
	alarm_mp4.audio_track_id = MP4AddALawAudioTrack(alarm_mp4.mp4_handle, 8000);
	if (alarm_mp4.audio_track_id == MP4_INVALID_TRACK_ID) {
		log_err("alarm: wrong audio track id\n");
		return -1;
	}
	MP4SetTrackIntegerProperty(alarm_mp4.mp4_handle, alarm_mp4.audio_track_id, "mdia.minf.stbl.stsd.alaw.channels",
				   1);
	MP4SetAudioProfileLevel(alarm_mp4.mp4_handle, 0x02);

	sync_g711_with_h265_alarm(&audio_alarm_buf, &video_alarm_buf);

	int h265_num =
	    (video_alarm_buf.frame_wIndex - video_alarm_buf.frame_rIndex +
	     video_alarm_buf.frame_num) % video_alarm_buf.frame_num;
	if (h265_num == 0 && video_alarm_buf.frame_wIndex > video_alarm_buf.frame_rIndex) {
		h265_num += video_alarm_buf.frame_num;
	}
	while (h265_num != 0) {
		if (video_alarm_buf.frame_rIndex >= video_alarm_buf.frame_num) {
			video_alarm_buf.frame_rIndex = 0;
		}
		alarm_mp4.key_frame = video_alarm_buf.frame[video_alarm_buf.frame_rIndex].keyframe;
		if (!is_write_keyframe && !alarm_mp4.key_frame) {
			h265_num--;
			continue;
		}
		is_write_keyframe = 1;
		find_nalu_write((unsigned char *)
				video_alarm_buf.frame[video_alarm_buf.frame_rIndex].data_index,
				video_alarm_buf.frame[video_alarm_buf.frame_rIndex].len, &alarm_mp4);
		video_alarm_buf.frame_rIndex++;
		h265_num--;
		alarm_mp4.is_need_set = 0;
	}

	int g711_num =
	    (audio_alarm_buf.frame_wIndex - audio_alarm_buf.frame_rIndex +
	     audio_alarm_buf.frame_num) % audio_alarm_buf.frame_num;
	if (g711_num == 0 && audio_alarm_buf.frame_wIndex > audio_alarm_buf.frame_rIndex) {
		g711_num += audio_alarm_buf.frame_num;
	}
	while (g711_num != 0) {
		if (audio_alarm_buf.frame_rIndex >= audio_alarm_buf.frame_num) {
			audio_alarm_buf.frame_rIndex = 0;
		}
		write_g711_mp4((unsigned char *)audio_alarm_buf.frame[audio_alarm_buf.frame_rIndex].data_index,
			       audio_alarm_buf.frame[audio_alarm_buf.frame_rIndex].len, &alarm_mp4);
		audio_alarm_buf.frame_rIndex++;
		g711_num--;
	}
	MP4Close(alarm_mp4.mp4_handle, 0);

	return 0;
}

void create_record_thread(void)
{
	int ret;

	pthread_t thread_record_id;

	if ((ret = pthread_create(&thread_record_id, NULL, &thread_record_mp4, NULL))) {
		log_err("pthread_create ret=%d\n", ret);
		exit(-1);
	}
	pthread_detach(thread_record_id);
}

void *thread_shbuf_stream(void *arg)
{
	log_info("shbuf stream thread start ok~\n");
	prctl(PR_SET_NAME, "shbuf_stream\0", NULL, NULL, NULL);
	struct ev_loop *stream_loop = ev_loop_new(EVBACKEND_EPOLL | EVFLAG_NOENV);
	shbf_rcv_global_init();
	main_stream_recv = shbfev_rcv_create(stream_loop, VIDEO_MAINSTREAM_CONTROLLER);
	shbfev_rcv_event(main_stream_recv, SHBF_RCVEV_SHARED_BUFFER, SHBF_EVT_HANDLE(main_stream_recv_callback), NULL);
	shbfev_rcv_event(main_stream_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "main_stream");
	shbfev_rcv_start(main_stream_recv);

	sub_stream_recv = shbfev_rcv_create(stream_loop, VIDEO_SUBSTREAM_CONTROLLER);
	shbfev_rcv_event(sub_stream_recv, SHBF_RCVEV_SHARED_BUFFER, SHBF_EVT_HANDLE(sub_stream_recv_callback), NULL);
	shbfev_rcv_event(sub_stream_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "sub_stream");
	shbfev_rcv_start(sub_stream_recv);

	audio_stream_recv = shbfev_rcv_create(stream_loop, AI_CONTROLLER);
	shbfev_rcv_event(audio_stream_recv, SHBF_RCVEV_SHARED_BUFFER, SHBF_EVT_HANDLE(audio_stream_recv_callback),
			NULL);
	shbfev_rcv_event(audio_stream_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "audio_stream");
	shbfev_rcv_start(audio_stream_recv);

	md_event_recv = shbfev_rcv_create(stream_loop, MD_CONTROLLER);
	shbfev_rcv_event(md_event_recv, SHBF_RCVEV_MESSAGE, SHBF_EVT_HANDLE(md_event_callback), NULL);
	shbfev_rcv_event(md_event_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "md_event_stream");
	shbfev_rcv_start(md_event_recv);

	snap_shot_recv = shbfev_rcv_create(stream_loop, JPEG_CONTROLLER);
	shbfev_rcv_event(snap_shot_recv, SHBF_RCVEV_SHARED_BUFFER, SHBF_EVT_HANDLE(snapshot_callback), NULL);
	shbfev_rcv_event(snap_shot_recv, SHBF_RCVEV_CLOSE, SHBF_EVT_HANDLE(shbf_close), "snapshot_stream");
	shbfev_rcv_start(snap_shot_recv);

	ev_run(stream_loop, 0);
	free_mp4_queue(&audio_prerecord_buf, &video_prerecord_buf);
	free_alarm_queue(&audio_alarm_buf, &video_alarm_buf);
	shbfev_rcv_destroy(main_stream_recv);
	shbfev_rcv_destroy(sub_stream_recv);
	shbfev_rcv_destroy(audio_stream_recv);
	shbfev_rcv_destroy(md_event_recv);
	shbfev_rcv_destroy(snap_shot_recv);
	shbf_rcv_global_exit();
	ev_loop_destroy(stream_loop);
	pthread_exit(0);
}

void create_stream_thread(void)
{
	pthread_t thread_stream_id;
	int ret = pthread_create(&thread_stream_id, NULL, &thread_shbuf_stream, NULL);
	if (ret != 0) {
		log_err("pthread_create shbuf stream ret=%d\n", ret);
		exit(-1);
	}
	pthread_detach(thread_stream_id);
	return;
}

int sdcard_storge_process(int id, int sock, char *ackbuf)
{
	if (NULL == ackbuf)
		return -1;

	int status = get_sdcard_status();
	switch (status) {
	case SD_OK:
	case SD_NOSPACE:
		sprintf(ackbuf, "{\"id\":%u, \"result\":[\"%llu\",\"%llu\",\"%llu\",\"%d\"]}", id, disk_size,
			record_size, avail_size, status);
		break;
	case SD_NOTEXIST:
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2003,\"message\":\"%s\"}}", id,
			"The sdcard don't exist!");
		break;
	case SD_EXCEPTION:
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2002,\"message\":\"%s\"}}", id,
			"The sdcard status error");
		break;
	case SD_FORMATIING:
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2000,\"message\":\"%s\"}}", id,
			"The sdcard is formating!");
		break;
	case SD_FORMAT_ERROR:
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2000,\"message\":\"%s\"}}", id,
			"The sdcard is formating!");
		break;
	case SD_UMOUNT:
		sprintf(ackbuf, "{\"id\":%d,\"error\":{\"code\":-2005,\"message\":\"%s\"}}", id,
			"The sdcard is umount");
		break;
	}

	log_info("storge ack is %s\n", ackbuf);
	send(sock, ackbuf, strlen(ackbuf), 0);
	return 0;
}
