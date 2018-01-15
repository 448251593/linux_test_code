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
void echo_show_send_audio(char *p_audio, int p_audio_len)
{
		
	if (echo_show_audio_buf_len + p_audio_len > sizeof(echo_show_audio_buf))
	{
	
		int remain_len = sizeof(echo_show_audio_buf) - echo_show_audio_buf_len;

		memcpy(&echo_show_audio_buf[echo_show_audio_buf_len],p_audio,remain_len);
		echo_show_audio_buf_len += remain_len;
		
	    //echo show  音频发送 必须800字节
		//tuya_ipc_audio_fifo_put(echo_show_audio_buf, echo_show_audio_buf_len);
		printf_m(echo_show_audio_buf, echo_show_audio_buf_len);
		echo_show_audio_buf_len = 0;

		//copy remain data
		memcpy(&echo_show_audio_buf[echo_show_audio_buf_len],(p_audio+remain_len),p_audio_len - remain_len);
		echo_show_audio_buf_len += p_audio_len - remain_len;
		
	}
	else
	{
	    memcpy(&echo_show_audio_buf[echo_show_audio_buf_len],p_audio,p_audio_len);
		echo_show_audio_buf_len += p_audio_len;
	}
	
}

int main()
{
	echo_show_send_audio("12345678", 8);
	echo_show_send_audio("12345678", 8);
	echo_show_send_audio("12345678", 8);
	echo_show_send_audio("12345678", 8);
	return 0;
}
