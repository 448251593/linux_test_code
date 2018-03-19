
#include "h264_aac_queue.h"

pthread_cond_t pre_product = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pre_lock = PTHREAD_MUTEX_INITIALIZER;
#define RCORDTIME 10
FRAME_T64 aac_timestamp = { {{0}
			     }
};
FRAME_T64 h264_timestamp = { {{0}
			      }
};

void mp4_queue_init_pre(pre_record_buf_t * aac_buf, pre_record_buf_t * h264_buf)
{
	memset(aac_buf, 0, sizeof(pre_record_buf_t));
	memset(h264_buf, 0, sizeof(pre_record_buf_t));
	if (NULL == aac_buf || NULL == h264_buf) {
		printf( "mp4 init data ptr is NULL.\n");
		return;
	}

	aac_buf->buf.data = (char *)malloc(AUDIO_MAX_LEN);
	if (NULL == aac_buf->buf.data) {
		printf( "malloc for aac buf error\n");
		perror("malloc error");
		exit(0);
	}
	h264_buf->buf.data = (char *)malloc(VIDEO_MAX_LEN);
	if (NULL == h264_buf->buf.data) {
		printf( "malloc for aac buf error\n");
		perror("malloc error");
		free(aac_buf->buf.data);
		exit(0);
	}
	aac_buf->buf.tail_len = AUDIO_MAX_LEN;
	aac_buf->frame_num = MAX_FRA;
	h264_buf->frame_num = MAX_FRA;
	h264_buf->buf.tail_len = VIDEO_MAX_LEN;
}

char push_aac_queue_pre(gm_enc_bitstream_t * aac_bs, int record_status, pre_record_buf_t * aac_buf)
{
	if (NULL == aac_bs || aac_bs->bs_len == 0) {
		printf( "aac frame's length is 0 ,drop it or data ptr is NULL.\n");
		return -1;
	}

	pthread_mutex_lock(&pre_lock);

	if (aac_buf->frame_wIndex >= MAX_FRA
	    || aac_bs->bs_len > AUDIO_MAX_LEN - aac_buf->buf.wIndex || aac_buf->push_full) {
		if (record_status && 0 == aac_buf->frame_rIndex) {	// we will write the 0th frame
			aac_buf->push_full = 1;
			printf("$$$$$$ aac cant save: tail len is %u , frame_wIndex is %u, frame_rIndex is %u, aac_buf->frame_num is %u$$$$$$\n",
			     aac_buf->buf.tail_len, aac_buf->frame_wIndex, aac_buf->frame_rIndex, aac_buf->frame_num);
			pthread_cond_signal(&pre_product);
			pthread_mutex_unlock(&pre_lock);
			return 0;
		}
		/*printf( "aac:  cut here : aac_buf->frame_wIndex is %u, aac_buf->frame_rIndex is %u\n", */
		/*aac_buf->frame_wIndex, aac_buf->frame_rIndex); */

		aac_buf->push_full = 0;
		aac_buf->frame_num = aac_buf->frame_wIndex;
		aac_buf->buf.wIndex = 0;
		aac_buf->frame_wIndex = 0;
		if (record_status) {
			aac_buf->buf.tail_len = aac_buf->buf.rIndex;
		} else {
			aac_buf->frame_rIndex = 0;
			aac_buf->buf.tail_len = AUDIO_MAX_LEN;
		}
	}

	if (aac_buf->buf.tail_len >= aac_bs->bs_len) {
		aac_buf->buf.tail_len -= aac_bs->bs_len;
	} else {
		int diff = 0;
		int tmp_index = 0;
		diff = aac_bs->bs_len - aac_buf->frame[aac_buf->frame_wIndex].len;
		while (diff > 0) {
			tmp_index++;
			if (record_status
			    &&
			    (((aac_buf->frame_wIndex + tmp_index - aac_buf->frame_rIndex +
			       aac_buf->frame_num) % (aac_buf->frame_num)) == 0)) {
				printf("$$$$$$$$$$$$$$$: aac buf full,frame_wIndex is %u, frame rindex is %u, tmp_index is %u\n",
				     aac_buf->frame_wIndex, aac_buf->frame_rIndex, tmp_index);
				pthread_cond_signal(&pre_product);
				pthread_mutex_unlock(&pre_lock);
				return 0;
			}
			diff -= aac_buf->frame[aac_buf->frame_wIndex + tmp_index].len;
		}
		aac_buf->buf.tail_len = -diff;
	}
	memcpy(aac_buf->buf.data + aac_buf->buf.wIndex, aac_bs->bs_buf, aac_bs->bs_len);
	aac_buf->frame[aac_buf->frame_wIndex].data_index = aac_buf->buf.data + aac_buf->buf.wIndex;
	aac_buf->frame[aac_buf->frame_wIndex].len = aac_bs->bs_len;

	gettimeofday(&aac_buf->frame[aac_buf->frame_wIndex].sys_time, NULL);

	if (aac_bs->timestamp < aac_timestamp.stamp32) {
		aac_timestamp.overcnt++;
	}
	aac_timestamp.stamp32 = aac_bs->timestamp;
	aac_buf->frame[aac_buf->frame_wIndex].timestamp = aac_timestamp.stamp64;
	aac_buf->frame_wIndex++;
	aac_buf->buf.wIndex += aac_bs->bs_len;

	if (aac_buf->frame_wIndex > aac_buf->frame_num) {
		aac_buf->frame_num = aac_buf->frame_wIndex;
	}

	if (record_status) {
		pthread_cond_signal(&pre_product);
	}

	pthread_mutex_unlock(&pre_lock);
	return 0;
}

void pop_aac_queue_pre(pre_record_buf_t * aac_buf)
{
	if (aac_buf->frame_rIndex > aac_buf->frame_wIndex) {
		aac_buf->buf.tail_len += aac_buf->frame[aac_buf->frame_rIndex].len;
	}
	aac_buf->buf.rIndex += aac_buf->frame[aac_buf->frame_rIndex].len;
	aac_buf->frame_rIndex++;
}

unsigned char get_aac_frame_num_pre(pre_record_buf_t * aac_buf)
{
	return (aac_buf->frame_wIndex - aac_buf->frame_rIndex + aac_buf->frame_num) % aac_buf->frame_num;
}

char push_h264_queue_pre(gm_enc_bitstream_t * h264_bs, int record_status, pre_record_buf_t * h264_buf)
{
	//lock
	if (h264_bs == NULL || h264_bs->bs_len == 0) {
		printf( "h264 len is zero\n");
		return -1;
	}

//        printf( "push_h264_queue_pre befor lock \n");
	pthread_mutex_lock(&pre_lock);

//	log_printf(LOG_INFO, "h264 frame_windex is %u,bs_len:%d,buf.wIndex:%d,push_full:%d,record_status:%d,frame_rIndex:%d,h264_buf->frame_num:%d,h264_buf->buf.rIndex:%d,tail_len:%d\n",h264_buf->frame_wIndex,h264_bs->bs_len,h264_buf->buf.wIndex,h264_buf->push_full,record_status,h264_buf->frame_rIndex,h264_buf->frame_num,h264_buf->buf.rIndex,h264_buf->buf.tail_len);

	if (h264_buf->frame_wIndex >= MAX_FRA
	    || h264_bs->bs_len > VIDEO_MAX_LEN - h264_buf->buf.wIndex || h264_buf->push_full) {
		if (record_status && 0 == h264_buf->frame_rIndex) {
			h264_buf->push_full = 1;
			//log_printf
			//    (LOG_INFO,"*******h264 cant save the current frame  tail len is %u , frame_wIndex is %u, buf windex is %u, h264_bs->bs_len is %u\n",
			//     h264_buf->buf.tail_len, h264_buf->frame_wIndex, h264_buf->buf.wIndex, h264_bs->bs_len);
			pthread_cond_signal(&pre_product);
			pthread_mutex_unlock(&pre_lock);
			return 0;
		}
		//printf( "h264:  cut here : h264_buf->frame_wIndex is %u, h264_buf->frame_rIndex is %u\n",
		//h264_buf->frame_wIndex, h264_buf->frame_rIndex); 

		h264_buf->push_full = 0;
		h264_buf->frame_num = h264_buf->frame_wIndex;
		h264_buf->buf.wIndex = 0;
		h264_buf->frame_wIndex = 0;
		if (record_status) {
			h264_buf->buf.tail_len = h264_buf->buf.rIndex;
		} else {
			h264_buf->buf.tail_len = VIDEO_MAX_LEN;
		}
	}

	//log_printf(LOG_INFO, "h264 tail_len:%d,bs_len:%d\n",h264_buf->buf.tail_len,h264_bs->bs_len);

	if (h264_buf->buf.tail_len >= h264_bs->bs_len) {
		h264_buf->buf.tail_len -= h264_bs->bs_len;
	} else {
		int diff = 0;
		int tmp_index = 0;
		//log_printf(LOG_INFO, "h264 diff:%d\n",diff);
		diff = h264_bs->bs_len - h264_buf->frame[h264_buf->frame_wIndex].len;
		while (diff > 0) {
			tmp_index++;	//the current frame not yet write to memery
			if (record_status
			    &&
			    (((h264_buf->frame_wIndex + tmp_index - h264_buf->frame_rIndex +
			       h264_buf->frame_num) % (h264_buf->frame_num)) == 0)) {
				printf(
					   "*******h264 full drop frame len is %u frame_windex is %u frame_rindex is %u tmp_index is %u*******\n",
					   h264_bs->bs_len, h264_buf->frame_wIndex, h264_buf->frame_rIndex, tmp_index);
				pthread_cond_signal(&pre_product);
				pthread_mutex_unlock(&pre_lock);
				return 0;
			}
			diff -= h264_buf->frame[h264_buf->frame_wIndex + tmp_index].len;
		}
		h264_buf->buf.tail_len = -diff;
	}
	//log_printf(LOG_INFO, "h264 frame_windex is %u,h264_bs->keyframe:%d \n", h264_buf->frame_wIndex,h264_bs->keyframe);

	memcpy(h264_buf->buf.data + h264_buf->buf.wIndex, h264_bs->bs_buf, h264_bs->bs_len);
	h264_buf->frame[h264_buf->frame_wIndex].data_index = h264_buf->buf.data + h264_buf->buf.wIndex;
	h264_buf->frame[h264_buf->frame_wIndex].len = h264_bs->bs_len;
	h264_buf->frame[h264_buf->frame_wIndex].keyframe = h264_bs->keyframe;

	if (h264_bs->timestamp < h264_timestamp.stamp32) {
		h264_timestamp.overcnt++;
	}
	h264_timestamp.stamp32 = h264_bs->timestamp;

	h264_buf->frame[h264_buf->frame_wIndex].timestamp = h264_timestamp.stamp64;
	gettimeofday(&h264_buf->frame[h264_buf->frame_wIndex].sys_time, NULL);

	h264_buf->frame_wIndex++;
	h264_buf->buf.wIndex += h264_bs->bs_len;

	if (h264_bs->keyframe) {
		
		//log_printf(LOG_INFO, "h264 h264_buf->frame[%d - 1].time_I_frame:%d \n",h264_buf->frame_wIndex,h264_buf->frame[h264_buf->frame_wIndex - 1].time_I_frame);
		h264_buf->frame[h264_buf->frame_wIndex - 1].time_I_frame = time(NULL);
		h264_buf->pre_wr_index = h264_buf->frame_wIndex - 1;
		if (!record_status) {
			h264_buf->frame_rIndex = h264_buf->pre_wr_index;
			h264_buf->buf.tail_len = VIDEO_MAX_LEN - h264_buf->buf.wIndex;
		}
	}

	if (h264_buf->frame_wIndex > h264_buf->frame_num) {
		h264_buf->frame_num = h264_buf->frame_wIndex;
	}

	//printf( "push_h264_queue_pre keyframe=%d,pre_wr_index=%d,frame_wIndex=%d,frame_num=%d alarm_status=%d,systime=%d\n", 
	//   	h264_bs->keyframe,h264_buf->pre_wr_index,h264_buf->frame_wIndex,h264_buf->frame_num,record_status
	//  	,h264_buf->frame[h264_buf->frame_wIndex-1].sys_time.tv_sec * 1000+h264_buf->frame[h264_buf->frame_wIndex-1].sys_time.tv_usec / 1000);

	if (record_status) {
		pthread_cond_signal(&pre_product);
	}

	pthread_mutex_unlock(&pre_lock);

	return 0;
}

void pop_h264_queue_pre(pre_record_buf_t * h264_buf)
{
	if (h264_buf->frame_rIndex > h264_buf->frame_wIndex) {
		h264_buf->buf.tail_len += h264_buf->frame[h264_buf->frame_rIndex].len;
	}
	h264_buf->buf.rIndex += h264_buf->frame[h264_buf->frame_rIndex].len;
	h264_buf->frame_rIndex++;
	 //printf( "pop_h264_queue_pre frame_rIndex=%d\n", 
	 //   	h264_buf->frame_rIndex);
}

unsigned char get_h264_frame_num_pre(pre_record_buf_t * h264_buf)
{
	return (h264_buf->frame_wIndex - h264_buf->frame_rIndex + h264_buf->frame_num) % h264_buf->frame_num;
}

void sync_aac_with_h264(pre_record_buf_t * aac_buf, pre_record_buf_t * h264_buf)
{
	unsigned long long h264_time_ms =
	    h264_buf->frame[h264_buf->frame_rIndex].sys_time.tv_sec * 1000 +
	    h264_buf->frame[h264_buf->frame_rIndex].sys_time.tv_usec / 1000;
	unsigned long long aac_time_ms = 0;
	unsigned int index = 0;
	int diff = 0;
	int i = 0;
	int num = (int)((aac_buf->frame_wIndex - aac_buf->frame_rIndex + aac_buf->frame_num) % aac_buf->frame_num);
	while (i >= 0) {
		index = (aac_buf->frame_wIndex + aac_buf->frame_num - 1 - i) % aac_buf->frame_num;
		aac_time_ms =
		    aac_buf->frame[index].sys_time.tv_sec * 1000 + aac_buf->frame[index].sys_time.tv_usec / 1000;
		diff = (int)(aac_time_ms - h264_time_ms);
		if (diff < 0 || num - i < 0) {
			aac_buf->frame_rIndex = index + 1;
			break;
		}
		i++;
	}
}

/*alarm function*/

void mp4_queue_init_alarm(alarm_record_buf_t * aac_alarm_buf, alarm_record_buf_t * h264_alarm_buf)
{
	memset(aac_alarm_buf, 0, sizeof(alarm_record_buf_t));
	memset(h264_alarm_buf, 0, sizeof(alarm_record_buf_t));
	if (NULL == aac_alarm_buf || NULL == h264_alarm_buf) {
		printf( "alarm: mp4 init data ptr is NULL.\n");
		return;
	}

	aac_alarm_buf->buf.data = (char *)malloc(AUDIO_ALARM_LEN);
	if (NULL == aac_alarm_buf->buf.data) {
		printf( "alarm: malloc for aac buf error\n");
		perror("malloc error");
		exit(0);
	}
	h264_alarm_buf->buf.data = (char *)malloc(VIDEO_ALARM_LEN);
	if (NULL == h264_alarm_buf->buf.data) {
		printf( "alarm: malloc for aac buf error\n");
		perror("malloc error");
		free(aac_alarm_buf->buf.data);
		exit(0);
	}
	aac_alarm_buf->frame_num = 0;
	h264_alarm_buf->frame_num = 0;
	aac_alarm_buf->buf.tail_len = AUDIO_ALARM_LEN;
	h264_alarm_buf->buf.tail_len = VIDEO_ALARM_LEN;
}

extern int m_vFrateR;
static time_t aac_time = 0;
char push_aac_buf_alarm(gm_enc_bitstream_t * aac_bs, int alarm_status, alarm_record_buf_t * aac_alarm_buf)
{
	if (aac_bs == NULL || aac_bs->bs_len == 0) {
		printf( "aac len is zero\n");
		return -1;
	}

	if (!aac_alarm_buf->buf_ready) {

          /* when frame add to it's max, init frame index */
          if ( aac_alarm_buf->frame_wIndex >= MAX_FRA )
          {
			aac_alarm_buf->frame_wIndex = 0;
			if (!alarm_status) {
				aac_alarm_buf->buf.tail_len = AUDIO_ALARM_LEN;
			}
          }

          /* when frame data add to it't max, init frame data index */
		if ( aac_bs->bs_len > AUDIO_ALARM_LEN - aac_alarm_buf->buf.wIndex) {
			aac_alarm_buf->buf.wIndex = 0;
			if (!alarm_status) {
				aac_alarm_buf->buf.tail_len = AUDIO_ALARM_LEN;
			}
		}

		if (aac_alarm_buf->buf.tail_len >= aac_bs->bs_len) {
			aac_alarm_buf->buf.tail_len -= aac_bs->bs_len;
		} else {
			int diff = 0;
			int tmp_index = 0;
			diff = aac_bs->bs_len - aac_alarm_buf->frame[aac_alarm_buf->frame_wIndex].len;
			while (diff > 0) {
				tmp_index++;
				if (alarm_status &&
				    (((aac_alarm_buf->frame_wIndex + tmp_index - aac_alarm_buf->frame_rIndex +
				       aac_alarm_buf->frame_num) % (aac_alarm_buf->frame_num)) == 0)) {
					printf(
					     "alarm: aac buf full,frame_wIndex is %u, frame rindex is %u, tmp_index is %u\n",
					     aac_alarm_buf->frame_wIndex, aac_alarm_buf->frame_rIndex, tmp_index);
					return 0;
				}
				diff -= aac_alarm_buf->frame[aac_alarm_buf->frame_wIndex + tmp_index].len;
			}
			aac_alarm_buf->buf.tail_len = -diff;
		}
		memcpy(aac_alarm_buf->buf.data + aac_alarm_buf->buf.wIndex, aac_bs->bs_buf, aac_bs->bs_len);
		aac_alarm_buf->frame[aac_alarm_buf->frame_wIndex].data_index =
		    aac_alarm_buf->buf.data + aac_alarm_buf->buf.wIndex;
		aac_alarm_buf->frame[aac_alarm_buf->frame_wIndex].len = aac_bs->bs_len;

		gettimeofday(&aac_alarm_buf->frame[aac_alarm_buf->frame_wIndex].sys_time, NULL);

		if (aac_bs->timestamp < aac_timestamp.stamp32) {
			aac_timestamp.overcnt++;
		}
		aac_timestamp.stamp32 = aac_bs->timestamp;
		aac_alarm_buf->frame[aac_alarm_buf->frame_wIndex].timestamp = aac_timestamp.stamp64;
		aac_alarm_buf->frame_wIndex++;
		aac_alarm_buf->buf.wIndex += aac_bs->bs_len;

		if (aac_alarm_buf->frame_wIndex > aac_alarm_buf->frame_num) {
			aac_alarm_buf->frame_num = aac_alarm_buf->frame_wIndex;
		}
        
/*		printf( "push_aac_buf_alarm alarm_wr_index=%d,frame_wIndex=%d,frame_num=%d alarm_status=%d,systime=%d\n", 
					aac_alarm_buf->alarm_wr_index,aac_alarm_buf->frame_wIndex,aac_alarm_buf->frame_num,alarm_status
					,aac_alarm_buf->frame[aac_alarm_buf->frame_wIndex-1].sys_time.tv_sec * 1000+aac_alarm_buf->frame[aac_alarm_buf->frame_wIndex-1].sys_time.tv_usec / 1000);
*/
		if (alarm_status) {
			if (aac_time == 0) {
				aac_time = time(NULL);
				aac_alarm_buf->alarm_wr_index = aac_alarm_buf->frame_wIndex;
				aac_alarm_buf->frame_rIndex = aac_alarm_buf->frame_wIndex;                                  
//                                    printf( "push_aac_buf_alarm aac_time=%d, frame_rIndex=%d\n", 
//                                        aac_time,aac_alarm_buf->frame_rIndex);
			}
			if (time(NULL) - aac_time > RCORDTIME) {
				aac_alarm_buf->buf_ready = 1;
//            			printf( "push_aac_buf_alarm aac alarm buf_ready=%d\n", 
//				aac_alarm_buf->buf_ready);
			}
		}
	}
	return 0;
}

static time_t h264_time = 0;
char push_h264_buf_alarm(gm_enc_bitstream_t * h264_bs, int alarm_status, alarm_record_buf_t * h264_alarm_buf, alarm_record_buf_t * aac_alarm_buf)
{
	if (h264_bs == NULL || h264_bs->bs_len == 0) {
		printf( "h264 len is zero\n");
		return -1;
	}

        
	/* when video 10s end but aac 10s not end ,check this time aac 10s is end or not */
	if ( (EN_ALARM_VIDEO_COMP_AAC_NOT_COMP == h264_alarm_buf->comp_sts) && ( 1==  aac_alarm_buf->buf_ready  ))
	{
		h264_alarm_buf->comp_sts = EN_ALARM_VIDEO_COMP_AAC_COMP;
		//alarm_record_over();
		//create_MotionRecord_thread();
//		printf( " 10s timeout video record ok aac record not ok buf_ready=%d\n", h264_alarm_buf->buf_ready);
		return 0;
	}
    
	if (!h264_alarm_buf->buf_ready) {
		if (h264_alarm_buf->frame_wIndex >= MAX_FRA) {
			h264_alarm_buf->frame_wIndex = 0;
			
			if (!alarm_status) {
				h264_alarm_buf->buf.tail_len = VIDEO_ALARM_LEN;
			}
		}

		if (h264_bs->bs_len > VIDEO_ALARM_LEN - h264_alarm_buf->buf.wIndex)
		{
			h264_alarm_buf->buf.wIndex = 0;
			if (!alarm_status) {
				h264_alarm_buf->buf.tail_len = VIDEO_ALARM_LEN;
			}
		}

		if (h264_alarm_buf->buf.tail_len >= h264_bs->bs_len) {
			h264_alarm_buf->buf.tail_len -= h264_bs->bs_len;
		} else {
			int diff = 0;
			int tmp_index = 0;
			diff = h264_bs->bs_len - h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex].len;
			while (diff > 0) {
				tmp_index++;
				if (alarm_status &&
				    (((h264_alarm_buf->frame_wIndex + tmp_index - h264_alarm_buf->frame_rIndex +
				       h264_alarm_buf->frame_num) % (h264_alarm_buf->frame_num)) == 0)) {
					printf(
					     "alarm: h264 buf full,frame_wIndex is %u, frame rindex is %u, tmp_index is %u\n",
					     h264_alarm_buf->frame_wIndex, h264_alarm_buf->frame_rIndex, tmp_index);
					
					return 0;
				}
				diff -= h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex + tmp_index].len;
			}
			h264_alarm_buf->buf.tail_len = -diff;
		}
		memcpy(h264_alarm_buf->buf.data + h264_alarm_buf->buf.wIndex, h264_bs->bs_buf, h264_bs->bs_len);
		h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex].data_index =
		    h264_alarm_buf->buf.data + h264_alarm_buf->buf.wIndex;
		h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex].len = h264_bs->bs_len;
		h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex].keyframe = h264_bs->keyframe;	//use the h264 key value

		if (h264_bs->timestamp < h264_timestamp.stamp32) {
			h264_timestamp.overcnt++;
		}
		h264_timestamp.stamp32 = h264_bs->timestamp;

		h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex].timestamp = h264_timestamp.stamp64;
		gettimeofday(&h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex].sys_time, NULL);

		h264_alarm_buf->frame_wIndex++;
			
		h264_alarm_buf->buf.wIndex += h264_bs->bs_len;

		if (h264_bs->keyframe) {
			h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex - 1].time_I_frame = time(NULL);
			h264_alarm_buf->alarm_wr_index = h264_alarm_buf->frame_wIndex - 1;
			if (!alarm_status) {
				h264_alarm_buf->frame_rIndex = h264_alarm_buf->alarm_wr_index;
				h264_alarm_buf->buf.tail_len = VIDEO_ALARM_LEN - h264_alarm_buf->buf.wIndex;
			}
		}

		if (h264_alarm_buf->frame_wIndex > h264_alarm_buf->frame_num) {
			h264_alarm_buf->frame_num = h264_alarm_buf->frame_wIndex;
		}
/*		printf( "push_h264_buf_alarm keyframe=%d, alarm_wr_index=%d,frame_wIndex=%d,frame_num=%d,systime=%d\n", 
					h264_bs->keyframe,h264_alarm_buf->alarm_wr_index,h264_alarm_buf->frame_wIndex,h264_alarm_buf->frame_num,
					h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex-1].sys_time.tv_sec * 1000+h264_alarm_buf->frame[h264_alarm_buf->frame_wIndex-1].sys_time.tv_usec / 1000);
*/		if (alarm_status) {
			if (h264_time == 0) {
				//get_alarm_picture();
				h264_time = time(NULL);
				h264_alarm_buf->frame_rIndex = h264_alarm_buf->alarm_wr_index;
//				printf( "get_alarm_picturew h264_time=%d, frame_rIndex=%d\n", 
//					h264_time,h264_alarm_buf->frame_rIndex);
			}
			if (time(NULL) - h264_time > RCORDTIME) {
				h264_alarm_buf->buf_ready = 1;
				/* when alarm video 10s time is end,check alarm aac 10s end or not */
				if ( 1==  aac_alarm_buf->buf_ready )
				{
					/* when alarm video 10s time is end, alarm aac 10s is end ,comp status is ok */
					h264_alarm_buf->comp_sts = EN_ALARM_VIDEO_COMP_AAC_COMP;
					//alarm_record_over();
					//create_MotionRecord_thread();
//					printf( " 10s end video record ok aac record ok buf_ready=%d\n", h264_alarm_buf->buf_ready);
							
				}
				else
				{
					/* when alarm video 10s time is end, alarm aac 10s is not end ,wait for aac 10s end */
					h264_alarm_buf->comp_sts = EN_ALARM_VIDEO_COMP_AAC_NOT_COMP;
					printf( " 10s end video record ok aac record not ok buf_ready=%d\n", h264_alarm_buf->buf_ready);
				}
			}
		}
	}
	return 0;
}

void sync_aac_with_h264_alarm(alarm_record_buf_t * aac_buf, alarm_record_buf_t * h264_buf)
{
	unsigned long long h264_time_ms =
	    h264_buf->frame[h264_buf->frame_rIndex].sys_time.tv_sec * 1000 +
	    h264_buf->frame[h264_buf->frame_rIndex].sys_time.tv_usec / 1000;
	unsigned long long aac_time_ms = 0;
	unsigned int index = 0;
	int num = (int)((aac_buf->frame_wIndex - aac_buf->frame_rIndex + MAX_FRA) % MAX_FRA);
	int diff = 0;
	int i = 0;
    
	printf( "sync_aac_with_h264_alarm h264_time_ms=%lld alarm_rindex=%d,frame_wIndex=%d,frame_num=%d\n", 
					h264_time_ms, aac_buf->frame_rIndex,aac_buf->frame_wIndex,aac_buf->frame_num);
	while (i >= 0) {
		index = (aac_buf->alarm_wr_index + MAX_FRA - 1 - i) % MAX_FRA;
		aac_time_ms =
		    aac_buf->frame[index].sys_time.tv_sec * 1000 + aac_buf->frame[index].sys_time.tv_usec / 1000;
		diff = (int)(aac_time_ms - h264_time_ms);
		if (diff <= 0 || num - i < 0) {
			aac_buf->frame_rIndex = index - 1;        
//			printf( "sync_aac_with_h264_alarm i=%d aac_time_ms=%d alarm_wr_index=%d,frame_wIndex=%d,frame_num=%d\n", 
//					i,aac_time_ms, aac_buf->alarm_wr_index,aac_buf->frame_wIndex,aac_buf->frame_num);
			break;
		}
		i++;
	}
}

void alarm_buf_reset(alarm_record_buf_t * aac_alarm_buf, alarm_record_buf_t * h264_alarm_buf)
{
	aac_time = 0;
	h264_time = 0;
	aac_alarm_buf->frame_num = 0;
	h264_alarm_buf->frame_num = 0;
	//aac_alarm_buf->frame_wIndex = 0;
	//h264_alarm_buf->frame_wIndex = 0;
	aac_alarm_buf->frame_rIndex = 0;
	h264_alarm_buf->frame_rIndex = 0;
	aac_alarm_buf->buf.tail_len = AUDIO_ALARM_LEN;
	h264_alarm_buf->buf.tail_len = VIDEO_ALARM_LEN;
	aac_alarm_buf->buf.wIndex = 0;
	h264_alarm_buf->buf.wIndex = 0;
	aac_alarm_buf->buf_ready = 0;
	h264_alarm_buf->buf_ready = 0;
    h264_alarm_buf->comp_sts = EN_ALARM_VIDEO_AAC_NOT_RECORD;
	aac_alarm_buf->comp_sts = EN_ALARM_VIDEO_AAC_NOT_RECORD;
	h264_alarm_buf->keyframeindex[0] = 0;
	h264_alarm_buf->keyframeindex[1] = 1;
}


gm_enc_bitstream_t  tmp_gm;
#define   TEST_LEN     30
int main()
{
		
	printf("hello world\n");

	mp4_queue_init_alarm(&audio_alarm_buf,&video_alarm_buf);
//	tmp_gm.bs_buf = "123456789";
//	tmp_gm.bs_buf_len = 9;
//	tmp_gm.bs_len = 1;	
	int i;
	for ( i = 0; i < 100; i++ )
	{
		char * tmp = malloc(TEST_LEN);
		memset(tmp, i , TEST_LEN);
		
	    tmp_gm.bs_buf = tmp;
		tmp_gm.bs_buf_len = TEST_LEN;
		tmp_gm.bs_len = TEST_LEN;	 
		push_aac_buf_alarm(&(tmp_gm), 0, &audio_alarm_buf);
	}
	
	



	int aac_num =	(audio_alarm_buf.frame_wIndex - audio_alarm_buf.frame_rIndex +
			 MAX_FRA) % MAX_FRA;
	
		if (aac_num == 0 && audio_alarm_buf.frame_wIndex > audio_alarm_buf.frame_rIndex) {
			aac_num += audio_alarm_buf.frame_num;
		}
		
		while (aac_num != 0) {
			if (audio_alarm_buf.frame_rIndex >= MAX_FRA) {
				audio_alarm_buf.frame_rIndex = 0;
			}
			
			printf("aac_num =%d\n", aac_num);
			printf("audio_alarm_buf.frame[audio_alarm_buf.frame_rIndex].data_index = %d\n",audio_alarm_buf.frame[audio_alarm_buf.frame_rIndex].data_index);
			printf("audio_alarm_buf.frame[audio_alarm_buf.frame_rIndex].len = %d\n",audio_alarm_buf.frame[audio_alarm_buf.frame_rIndex].len);
			
			audio_alarm_buf.frame_rIndex++;
			aac_num--;
		}

	
	
	return 0;
}




