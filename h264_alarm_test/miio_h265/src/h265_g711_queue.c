#include <miio/frameinfo.h>
#include "h265_g711_queue.h"

#define MUTEX_TIMEOUT 1

pthread_cond_t pre_product = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pre_lock = PTHREAD_MUTEX_INITIALIZER;

void mp4_queue_init_pre(pre_record_buf_t * g711_buf, pre_record_buf_t * h265_buf)
{
	memset(g711_buf, 0, sizeof(pre_record_buf_t));
	memset(h265_buf, 0, sizeof(pre_record_buf_t));
	if (NULL == g711_buf || NULL == h265_buf) {
		log_err("mp4 init data ptr is NULL.\n");
		return;
	}

	g711_buf->buf.data = (char *)malloc(AUDIO_MAX_LEN);
	if (NULL == g711_buf->buf.data) {
		log_err("malloc for g711 buf error\n");
		perror("malloc error");
		exit(0);
	}
	h265_buf->buf.data = (char *)malloc(VIDEO_MAX_LEN);
	if (NULL == h265_buf->buf.data) {
		log_err("malloc for h265 buf error\n");
		perror("malloc error");
		free(g711_buf->buf.data);
		exit(0);
	}
	g711_buf->frame_num = MAX_FRA;
	h265_buf->frame_num = MAX_FRA;
}

void free_mp4_queue(pre_record_buf_t * g711_buf, pre_record_buf_t * h265_buf)
{
	if (g711_buf->buf.data) {
		free(g711_buf->buf.data);
	}

	if (h265_buf->buf.data) {
		free(h265_buf->buf.data);
	}
}

void mp4_queue_init_alarm(alarm_record_buf_t * g711_buf, alarm_record_buf_t * h265_buf)
{
	memset(g711_buf, 0, sizeof(alarm_record_buf_t));
	memset(h265_buf, 0, sizeof(alarm_record_buf_t));
	if (NULL == g711_buf || NULL == h265_buf) {
		log_err("alarm: mp4 init data ptr is NULL.\n");
		return;
	}

	g711_buf->buf.data = (char *)malloc(AUDIO_ALARM_LEN);
	if (NULL == g711_buf->buf.data) {
		log_err("alarm: malloc for g711 buf error\n");
		perror("malloc error");
		exit(0);
	}
	h265_buf->buf.data = (char *)malloc(VIDEO_ALARM_LEN);
	if (NULL == h265_buf->buf.data) {
		log_err("alarm: malloc for h265 buf error\n");
		perror("malloc error");
		free(g711_buf->buf.data);
		exit(0);
	}
	g711_buf->frame_num = 0;
	h265_buf->frame_num = 0;
}

void free_alarm_queue(alarm_record_buf_t * g711_buf, alarm_record_buf_t * h265_buf)
{
	if (g711_buf->buf.data) {
		free(g711_buf->buf.data);
	}

	if (h265_buf->buf.data) {
		free(h265_buf->buf.data);
	}
}

static unsigned int get_avail_space(pre_record_buf_t * buf, unsigned int buf_len)
{
	if (buf->buf.rIndex == buf->buf.wIndex) {
		return buf_len;
	} else {
		return (buf->buf.rIndex - buf->buf.wIndex + buf_len) % buf_len;
	}
}

static uint get_avail_frame_count(pre_record_buf_t *buf, uint max_frame_count)
{
	uint slot;
	if (buf->frame_wIndex >= buf->frame_rIndex) {
		slot = max_frame_count - buf->frame_wIndex + // rear
		       buf->frame_rIndex; // front
	} else {
		slot = buf->frame_rIndex - buf->frame_wIndex;
	}
	// avoid wIndex increasing too fast
	// when wIndex + 1 = rIndex, the buffer should not be writen
	return slot - 1;
}

void dumpinfo_record_buf(const char *reason, pre_record_buf_t *h265_buf, alarm_record_buf_t *alarm_buf)
{
	log_info("%s: reason = %s\n", __func__, reason);
	log_info("pre-record buf dump:\n"
		 " frame num: %d/%d,\n"
		 " frame rIndex: %u, wIndex: %u,\n"
		 " buf rIndex %u, wIndex %u,\n"
		 " space available: %u, total: %u\n\n",
		 h265_buf->frame_num, MAX_FRA,
		 h265_buf->frame_rIndex, h265_buf->frame_wIndex,
		 h265_buf->buf.rIndex, h265_buf->buf.wIndex,
		 get_avail_space(h265_buf, VIDEO_MAX_LEN), VIDEO_MAX_LEN);

	log_info("alarm-record buf dump:\n"
		 " frame num: %d/%d,\n"
		 " frame rIndex: %u, wIndex: %u,\n"
		 " buf rIndex %u, wIndex %u,\n"
		 " space available: %u, total: %u\n\n",
		 alarm_buf->frame_num, MAX_FRA,
		 alarm_buf->frame_rIndex, alarm_buf->frame_wIndex,
		 alarm_buf->buf.rIndex, alarm_buf->buf.wIndex,
		 get_avail_space((pre_record_buf_t *)alarm_buf, VIDEO_ALARM_LEN), VIDEO_ALARM_LEN);


	return;
}

int push_g711_queue_pre(char *data, int record_status, pre_record_buf_t * g711_buf)
{
	int rc = -1;
	MIIO_IMPAudioFrame frame_info = { 0 };
	memcpy(&frame_info, data, sizeof(MIIO_IMPAudioFrame));

	if (NULL == data || NULL == g711_buf || frame_info.length == 0 || frame_info.length > MAX_AUDIO_FRAME_LEN) {
		log_err("pre:g711 frame's length is err or data ptr is NULL. len is %d\n", frame_info.length);
		if (NULL == data)
			log_err("pre: data is NULL!\n");
		if (NULL == g711_buf)
			log_err("pre: g711_buf is NULL!\n");
		return -1;
	}
	struct timespec tout;
	clock_gettime(CLOCK_REALTIME, &tout);
	tout.tv_sec += MUTEX_TIMEOUT;

	int ret = pthread_mutex_timedlock(&pre_lock, &tout);
	if (ret) {
		if (ETIMEDOUT == ret) {
			log_err("g711: get lock timeout\n");
		} else {
			perror("g711: get lock error!!!!!!!!\n");
		}
		return -1;
	}

recheck:
	if (get_avail_space(g711_buf, AUDIO_MAX_LEN) <= frame_info.length || get_avail_frame_count(g711_buf, MAX_FRA) == 0) {
		if (!record_status) {
			pop_g711_queue_pre(g711_buf);
			goto recheck;
		}
		log_warning
		    ("g711: The buffer is full, frame lost!\n"
			     "wIndex is %u,rIndex is %u, buf.wIndex is %u, buf.rIndex is %u, frame_num is %u, space is %u, frame_count: %u\n",
		     g711_buf->frame_wIndex, g711_buf->frame_rIndex, g711_buf->buf.wIndex, g711_buf->buf.rIndex,
		     g711_buf->frame_num, get_avail_space(g711_buf, AUDIO_MAX_LEN), get_frame_num_pre(g711_buf));
		goto out_signal;
	}

	if (g711_buf->frame_wIndex >= MAX_FRA || frame_info.length > AUDIO_MAX_LEN - g711_buf->buf.wIndex) {
		g711_buf->frame_num = g711_buf->frame_wIndex;
		g711_buf->buf.wIndex = 0;
		g711_buf->frame_wIndex = 0;

		// the buf.wIndex & frame_wIndex are modified. Perform buffer check again
		goto recheck;
	}

	memcpy(g711_buf->buf.data + g711_buf->buf.wIndex, data + sizeof(MIIO_IMPAudioFrame), frame_info.length);
	g711_buf->frame[g711_buf->frame_wIndex].data_index = g711_buf->buf.data + g711_buf->buf.wIndex;
	g711_buf->frame[g711_buf->frame_wIndex].len = frame_info.length;

	gettimeofday(&g711_buf->frame[g711_buf->frame_wIndex].sys_time, NULL);

	g711_buf->frame[g711_buf->frame_wIndex].timestamp = frame_info.timeStamp;
	g711_buf->frame[g711_buf->frame_wIndex].index = g711_buf->buf.wIndex;
	g711_buf->frame_wIndex++;
	g711_buf->buf.wIndex += frame_info.length;

	if (g711_buf->frame_wIndex > g711_buf->frame_num) {
		g711_buf->frame_num = g711_buf->frame_wIndex;
	}

	rc = 0;
	if (!record_status) {
		goto out_unlock;
	}

out_signal:
	pthread_cond_signal(&pre_product);
out_unlock:
	pthread_mutex_unlock(&pre_lock);
	return rc;
}

void pop_g711_queue_pre(pre_record_buf_t *g711_buf)
{
	/*g711_buf->buf.rIndex += g711_buf->frame[g711_buf->frame_rIndex].len; */
	g711_buf->frame_rIndex++;
	if (g711_buf->frame_rIndex >= g711_buf->frame_num) {
		g711_buf->frame_rIndex = 0;
	}

	if (get_frame_num_pre(g711_buf) > 0) {
		g711_buf->buf.rIndex = g711_buf->frame[g711_buf->frame_rIndex].index;
	} else {
		g711_buf->buf.rIndex = g711_buf->buf.wIndex;
	}
}

unsigned int get_frame_num_pre(pre_record_buf_t *buf)
{
	if (buf->frame_wIndex >= buf->frame_rIndex) {
		return buf->frame_wIndex - buf->frame_rIndex;
	} else {
		return buf->frame_num - buf->frame_rIndex + buf->frame_wIndex;
	}
}

//h265 function
int push_h265_queue_pre(char *data, int record_status, pre_record_buf_t * h265_buf)
{
	int rc = -1;
	MIIO_IMPEncoderStream frame_info = { 0 };
	memcpy(&frame_info, data, sizeof(MIIO_IMPEncoderStream));

	if (data == NULL || frame_info.length == 0 || h265_buf == NULL || frame_info.length > MAX_VIDEO_FRAME_LEN) {
		log_err("pre:h265 frame's length is 0 or data ptr is NULL. len is %d\n", frame_info.length);
		if (NULL == data)
			log_err("pre: data is NULL!\n");
		if (NULL == h265_buf)
			log_err("pre: h265 is NULL!\n");
		return -1;
	}

	struct timespec tout;
	clock_gettime(CLOCK_REALTIME, &tout);
	tout.tv_sec += MUTEX_TIMEOUT;

	//lock
	int ret = pthread_mutex_timedlock(&pre_lock, &tout);
	if (ret) {
		if (ETIMEDOUT == ret) {
			log_err("h265: get lock timeout\n");
		} else {
			perror("h265: get lock error!!!!!!!!\n");
		}
		return -1;
	}

recheck:
	if (get_avail_space(h265_buf, VIDEO_MAX_LEN) < frame_info.length || get_avail_frame_count(h265_buf, MAX_FRA) == 0) {
		if (!record_status) {
			pop_h265_queue_pre(h265_buf);
			goto recheck;
		}
		static time_t dump_time;
		time_t dump_now = time(NULL);

		if (dump_now - dump_time > 30) {
			dump_time = dump_now;
			dumpinfo_record_buf("buffer full", &video_prerecord_buf, &video_alarm_buf);
		}
		goto out_signal;
	}

	if (h265_buf->frame_wIndex >= MAX_FRA || frame_info.length > VIDEO_MAX_LEN - h265_buf->buf.wIndex) {
		h265_buf->frame_num = h265_buf->frame_wIndex;
		h265_buf->buf.wIndex = 0;
		h265_buf->frame_wIndex = 0;

		// the buf.wIndex & frame_wIndex are modified. Perform buffer check again
		goto recheck;
	}

	memcpy(h265_buf->buf.data + h265_buf->buf.wIndex, data + sizeof(MIIO_IMPEncoderStream), frame_info.length);
	h265_buf->frame[h265_buf->frame_wIndex].data_index = h265_buf->buf.data + h265_buf->buf.wIndex;
	h265_buf->frame[h265_buf->frame_wIndex].len = frame_info.length;
	h265_buf->frame[h265_buf->frame_wIndex].keyframe = frame_info.key_frame;
	h265_buf->frame[h265_buf->frame_wIndex].dispose_flag = frame_info.dispose_flag;

	h265_buf->frame[h265_buf->frame_wIndex].timestamp = frame_info.timestamp;
	gettimeofday(&h265_buf->frame[h265_buf->frame_wIndex].sys_time, NULL);
	h265_buf->frame[h265_buf->frame_wIndex].index = h265_buf->buf.wIndex;

	h265_buf->frame_wIndex++;
	h265_buf->buf.wIndex += frame_info.length;

	if (frame_info.key_frame) {
		h265_buf->frame[h265_buf->frame_wIndex - 1].time_I_frame = time(NULL);
		h265_buf->pre_wr_index = h265_buf->frame_wIndex - 1;
		if (!record_status) {
			h265_buf->buf.rIndex = h265_buf->buf.wIndex - frame_info.length;
			h265_buf->frame_rIndex = h265_buf->pre_wr_index;
		}
	}

	if (h265_buf->frame_wIndex > h265_buf->frame_num) {
		h265_buf->frame_num = h265_buf->frame_wIndex;
	}

	rc = 0;
	if (!record_status) {
		goto out_unlock;
	}
out_signal:
	pthread_cond_signal(&pre_product);
out_unlock:
	pthread_mutex_unlock(&pre_lock);

	return rc;
}

void pop_h265_queue_pre(pre_record_buf_t *h265_buf)
{
	h265_buf->frame_rIndex++;
	if (h265_buf->frame_rIndex >= h265_buf->frame_num) {
		h265_buf->frame_rIndex = 0;
	}

	if (get_frame_num_pre(h265_buf) > 0) {
		h265_buf->buf.rIndex = h265_buf->frame[h265_buf->frame_rIndex].index;
	} else {
		h265_buf->buf.rIndex = h265_buf->buf.wIndex;
	}
}

void sync_g711_with_h265(pre_record_buf_t * g711_buf, pre_record_buf_t * h265_buf)
{
	unsigned long long h265_time_ms =
	    h265_buf->frame[h265_buf->frame_rIndex].sys_time.tv_sec * 1000 +
	    h265_buf->frame[h265_buf->frame_rIndex].sys_time.tv_usec / 1000;
	unsigned long long g711_time_ms = 0;
	unsigned int index = 0;
	int diff = 0;
	int i = 0;
	int num = (int)((g711_buf->frame_wIndex - g711_buf->frame_rIndex + g711_buf->frame_num) % g711_buf->frame_num);
	while (i >= 0) {
		index = (g711_buf->frame_wIndex + g711_buf->frame_num - 1 - i) % g711_buf->frame_num;
		g711_time_ms =
		    g711_buf->frame[index].sys_time.tv_sec * 1000 + g711_buf->frame[index].sys_time.tv_usec / 1000;
		diff = (int)(g711_time_ms - h265_time_ms);
		if (diff < 0 || num - i < 0) {
			g711_buf->frame_rIndex = index + 1;
			if (get_frame_num_pre(g711_buf) > 0) {
				g711_buf->buf.rIndex = g711_buf->frame[g711_buf->frame_rIndex].index;
			} else {
				g711_buf->buf.rIndex = g711_buf->buf.wIndex;
			}
			break;
		}
		i++;
	}
}

//motion alarm function
static unsigned int get_alarm_avail_space(alarm_record_buf_t * buf, unsigned int buf_len)
{
	if (buf->buf.rIndex == buf->buf.wIndex) {
		return buf_len;
	} else {
		return (buf->buf.rIndex - buf->buf.wIndex + buf_len) % buf_len;
	}
}

volatile time_t g711_time = 0;
int push_g711_queue_alarm(char *data, int alarm_status, alarm_record_buf_t * g711_buf)
{
	MIIO_IMPAudioFrame frame_info = { 0 };
	memcpy(&frame_info, data, sizeof(MIIO_IMPAudioFrame));

	if (NULL == data || NULL == g711_buf || frame_info.length == 0 || frame_info.length > MAX_AUDIO_FRAME_LEN) {
		log_err("alarm:g711 frame's length is 0 ,drop it or data ptr is NULL. len is %d\n", frame_info.length);
		if (NULL == data)
			log_err("alarm: data is NULL!\n");
		if (NULL == g711_buf)
			log_err("alarm: g711_buf is NULL!\n");
		return -1;
	}

	if (!g711_buf->buf_ready) {
		if (g711_buf->frame_wIndex >= MAX_FRA || frame_info.length > AUDIO_ALARM_LEN - g711_buf->buf.wIndex) {
			g711_buf->buf.wIndex = 0;
			g711_buf->frame_wIndex = 0;
		}

		memcpy(g711_buf->buf.data + g711_buf->buf.wIndex, data + sizeof(MIIO_IMPAudioFrame), frame_info.length);
		g711_buf->frame[g711_buf->frame_wIndex].data_index = g711_buf->buf.data + g711_buf->buf.wIndex;
		g711_buf->frame[g711_buf->frame_wIndex].len = frame_info.length;

		gettimeofday(&g711_buf->frame[g711_buf->frame_wIndex].sys_time, NULL);

		g711_buf->frame_wIndex++;
		g711_buf->buf.wIndex += frame_info.length;

		if (g711_buf->frame_wIndex > g711_buf->frame_num) {
			g711_buf->frame_num = g711_buf->frame_wIndex;
		}
		if (alarm_status) {
			if (g711_time == 0) {
				g711_time = time(NULL);
				g711_buf->alarm_wr_index = g711_buf->frame_wIndex;
				g711_buf->frame_rIndex = g711_buf->frame_wIndex;
			}
			if (time(NULL) - g711_time >= 10) {
				g711_buf->buf_ready = 1;
			}
		}
	}
	return 0;
}

void pop_g711_queue_alarm(alarm_record_buf_t * g711_buf)
{
	/*g711_buf->buf.rIndex += g711_buf->frame[g711_buf->frame_rIndex].len; */
	g711_buf->buf.rIndex = g711_buf->frame[g711_buf->frame_rIndex].index;
	g711_buf->frame_rIndex++;
}

unsigned int get_g711_frame_num_alarm(alarm_record_buf_t * g711_buf)
{
	return (g711_buf->frame_wIndex - g711_buf->frame_rIndex + g711_buf->frame_num) % g711_buf->frame_num;
}

//h265 function
volatile time_t h265_time = 0;
int push_h265_queue_alarm(char *data, int alarm_status, alarm_record_buf_t * h265_buf)
{
	MIIO_IMPEncoderStream frame_info = { 0 };
	memcpy(&frame_info, data, sizeof(MIIO_IMPEncoderStream));

	if (data == NULL || h265_buf == NULL || frame_info.length == 0 || frame_info.length > MAX_VIDEO_FRAME_LEN) {
		log_err("alarm:h265 frame's length is 0 or data ptr is NULL. len is %d\n", frame_info.length);
		if (NULL == data)
			log_err("alarm: data is NULL!\n");
		if (NULL == h265_buf)
			log_err("alarm: h265 is NULL!\n");
		return -1;
	}

	if (!h265_buf->buf_ready) {
		if (h265_buf->frame_wIndex >= MAX_FRA || frame_info.length > VIDEO_ALARM_LEN - h265_buf->buf.wIndex) {
			h265_buf->buf.wIndex = 0;
			h265_buf->frame_wIndex = 0;
		}

		memcpy(h265_buf->buf.data + h265_buf->buf.wIndex, data + sizeof(MIIO_IMPEncoderStream),
		       frame_info.length);
		h265_buf->frame[h265_buf->frame_wIndex].data_index = h265_buf->buf.data + h265_buf->buf.wIndex;
		h265_buf->frame[h265_buf->frame_wIndex].len = frame_info.length;
		h265_buf->frame[h265_buf->frame_wIndex].keyframe = frame_info.key_frame;	//use the h265 key value

		h265_buf->frame[h265_buf->frame_wIndex].timestamp = frame_info.timestamp;
		gettimeofday(&h265_buf->frame[h265_buf->frame_wIndex].sys_time, NULL);

		h265_buf->frame_wIndex++;
		h265_buf->buf.wIndex += frame_info.length;

		if (frame_info.key_frame) {
			h265_buf->frame[h265_buf->frame_wIndex - 1].time_I_frame = time(NULL);
			h265_buf->alarm_wr_index = h265_buf->frame_wIndex - 1;
			if (!alarm_status) {
				h265_buf->frame_rIndex = h265_buf->alarm_wr_index;
			}
		}

		if (h265_buf->frame_wIndex > h265_buf->frame_num) {
			h265_buf->frame_num = h265_buf->frame_wIndex;
		}

		if (alarm_status) {
			if (h265_time == 0) {
				get_alarm_snapshot();
				create_MotionEvent_thread();
				h265_time = time(NULL);
				h265_buf->frame_rIndex = h265_buf->alarm_wr_index;
			}
			if (time(NULL) - h265_time >= 10 && audio_alarm_buf.buf_ready) {
				h265_buf->buf_ready = 1;
				alarm_record_over();
				create_MotionAlarm_thread();
			}
		}
	}
	return 0;
}

void pop_h265_queue_alarm(alarm_record_buf_t * h265_buf)
{
	h265_buf->buf.rIndex = h265_buf->frame[h265_buf->frame_rIndex].index;
	h265_buf->frame_rIndex++;
}

unsigned int get_h265_frame_num_alarm(alarm_record_buf_t * h265_buf)
{
	return (h265_buf->frame_wIndex - h265_buf->frame_rIndex + h265_buf->frame_num) % h265_buf->frame_num;
}

void sync_g711_with_h265_alarm(alarm_record_buf_t * g711_buf, alarm_record_buf_t * h265_buf)
{
	unsigned long long h265_time_ms =
	    h265_buf->frame[h265_buf->frame_rIndex].sys_time.tv_sec * 1000 +
	    h265_buf->frame[h265_buf->frame_rIndex].sys_time.tv_usec / 1000;
	unsigned long long g711_time_ms = 0;
	unsigned int index = 0;
	int diff = 0;
	int i = 0;
	int num = (int)((g711_buf->frame_wIndex - g711_buf->frame_rIndex + g711_buf->frame_num) % g711_buf->frame_num);
	while (i >= 0) {
		index = (g711_buf->frame_wIndex + g711_buf->frame_num - 1 - i) % g711_buf->frame_num;
		g711_time_ms =
		    g711_buf->frame[index].sys_time.tv_sec * 1000 + g711_buf->frame[index].sys_time.tv_usec / 1000;
		diff = (int)(g711_time_ms - h265_time_ms);
		if (diff < 0 || num - i < 0) {
			g711_buf->frame_rIndex = index + 1;
			if (get_g711_frame_num_alarm(g711_buf) > 0) {
				g711_buf->buf.rIndex = g711_buf->frame[g711_buf->frame_rIndex].index;
			} else {
				g711_buf->buf.rIndex = g711_buf->buf.wIndex;
			}
			break;
		}
		i++;
	}
}

void alarm_buf_reset(alarm_record_buf_t * g711_alarm_buf, alarm_record_buf_t * h265_alarm_buf)
{
	g711_time = 0;
	h265_time = 0;
	g711_alarm_buf->frame_num = 0;
	h265_alarm_buf->frame_num = 0;
	g711_alarm_buf->frame_wIndex = 0;
	h265_alarm_buf->frame_wIndex = 0;
	g711_alarm_buf->frame_rIndex = 0;
	h265_alarm_buf->frame_rIndex = 0;
	g711_alarm_buf->buf.wIndex = 0;
	h265_alarm_buf->buf.wIndex = 0;
	g711_alarm_buf->buf_ready = 0;
	h265_alarm_buf->buf_ready = 0;
}
