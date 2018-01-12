#include<stdio.h>
 #include <string.h>

char echo_show_audio_buf[10] = {0};
int  echo_show_audio_buf_len = 0;
void printf_m(char *p_audio, int p_audio_len)
{
	int i = 0;
	for(i = 0; i< p_audio_len; i++)
	{
		printf("%x ", p_audio[i]);
	}
	printf("\n");
}
char  audio_buf_pcm[1760];//最少 1760 因为 一次就是 1280 byte ;1280 - 800=480; 下一次 480+1280=1760 byte;1760/2=880 short byte
int  audio_buf_pcm_len;


void echo_show_send_audio(char *data, int size)
{
	#define  ECHO_SHOW_PCM_SEND_CONST_LEN   8
	
	
	int 	*p_audio_len  = &audio_buf_pcm_len;
	char   *p_audio = (char  *)audio_buf_pcm;
	int    send_times ,i;
	int    rslt = 0;
	 memcpy((audio_buf_pcm + *p_audio_len), data, size);
	rslt = size;
	*p_audio_len += rslt;

	send_times = *p_audio_len / ECHO_SHOW_PCM_SEND_CONST_LEN;

	for(i = 0 ; i < send_times; i++)
	{
		//echo show  音频发送 必须800字节
		//tuya_ipc_audio_fifo_put((p_audio+(i*ECHO_SHOW_PCM_SEND_CONST_LEN)), ECHO_SHOW_PCM_SEND_CONST_LEN);
		printf_m(p_audio+(i*ECHO_SHOW_PCM_SEND_CONST_LEN),ECHO_SHOW_PCM_SEND_CONST_LEN);
	}
	
	int remain_len = *p_audio_len % ECHO_SHOW_PCM_SEND_CONST_LEN;
	if(remain_len > 0)
	{
		memmove(p_audio, (p_audio+(i*ECHO_SHOW_PCM_SEND_CONST_LEN)), remain_len);	
		*p_audio_len = remain_len;
	}
	else
	{
	   *p_audio_len = 0; 
	}

}

int main()
{
	echo_show_send_audio("1234567890", 10);
echo_show_send_audio("1234567890", 10);
echo_show_send_audio("1234567890", 10);
echo_show_send_audio("1234567890", 10);
echo_show_send_audio("1234567890", 10);
	printf("remain_len=%d\n",audio_buf_pcm_len);
	
	return 0;
}
