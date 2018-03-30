#include<stdio.h>
#include<string.h>
#include<sys/wait.h>

#define STRING_TIMESTAMP_FILENAME  "str_timestamp"

int  scan_file_and_createfile(char *ppath)
{
		
	 char p_cmd_shell[100];
	memset(p_cmd_shell,0, sizeof(p_cmd_shell));
	snprintf(p_cmd_shell,sizeof(p_cmd_shell),"find %s -name \"*.mp4\"|awk '{print substr($1,21,10)}' > %s",ppath,STRING_TIMESTAMP_FILENAME);
		printf("p_cmd_shell->%s\n",p_cmd_shell);
		
	int status = system(p_cmd_shell);
	if (WIFEXITED(status) && -1 != status && !WEXITSTATUS(status)){
		
		printf("cmd  success\n");
		return 0;
	}
	else
	{
		printf("cmd err..\n");
		return -1;
	}
}
void  convert_to_timestamp(char *pfile_in,char *pfile_out)
{
	char str_times[50];
	FILE *fp = NULL;
	FILE *fp_timestamp_32bit = NULL;
	int   ct = 0;
	fp_timestamp_32bit = fopen(pfile_out,"w+b");
	fp = fopen(pfile_in,"r");
	if(fp != NULL && fp_timestamp_32bit != NULL)
	{
		while(1)
		{
			memset(str_times,0,sizeof(str_times));
			size_t len = 0;
			char *rslt = fgets(str_times,sizeof(str_times),fp);
			if(rslt != NULL)
			{
				int ts = atoi(str_times);
				fwrite((char*)&ts,1,4,fp_timestamp_32bit);
				str_times[strlen(str_times) - 1] = 0;
			//	printf("%s\n",str_times);
				ct++;
			}	
			else 
			{				
				break;			
			}
		}
	}
	printf("scan all vedio file and update .timestamp file count=%d\n",ct);
	if(fp != NULL)
	{
		fclose(fp);
	}
	if(fp_timestamp_32bit != NULL)
	{
		fclose(fp_timestamp_32bit);
	}
}

void  deal_get_timestamp(char *pfile_in,char *pfile_out)
{
	char str_times[100];
	FILE *fp = NULL;
	FILE *fp_timestamp_32bit = NULL;
	
	fp_timestamp_32bit = fopen(pfile_out,"w+");
	fp = fopen(pfile_in,"r");
	
	if(fp != NULL && fp_timestamp_32bit != NULL)
	{
		while(1)
		{
			memset(str_times,0,sizeof(str_times));
			
			char *rslt = fgets(str_times,sizeof(str_times),fp);
			if(rslt != NULL)
			{
				//int ts = atoi(str_times);
				//fwrite((char*)&ts,1,4,fp_timestamp_32bit);
				str_times[strlen(str_times) - 1] = 0;
				//printf("%s\n",str_times);
				char *pstart = 0,*pend = 0;
				pstart = strrchr(str_times,'_');
				pend = strrchr(str_times,'.');
				if(pstart != NULL && pend != NULL)
				{
					*pend = '\r';
					*(pend+1) = '\n';
					*(pend+2) = '\0';
					fwrite(pstart + 1, 1, strlen(pstart + 1),fp_timestamp_32bit);
					//printf("%d,%s\n",(int)strlen(pstart+1), pstart+1);
					//snprintf(tmp, sizeof(tmp), "%s\n",pstart+1);
					//printf("%s",pstart + 1);
				}
			}	
			else 
			{				
				break;			
			}
		}
	}	
	if(fp != NULL)
	{
		fclose(fp);
	}
	if(fp_timestamp_32bit != NULL)
	{
		fclose(fp_timestamp_32bit);
	}
}

void run_cmd(const char *cmd, char *output, int outsize)
{
	FILE *pipe = popen(cmd, "r");
	if (!pipe)
		return;
	if (NULL != output) {
		fgets(output, outsize, pipe);
	}
	pclose(pipe);
}
const char  *find_all_cmd = "find /mnt/media/mmcblk0p1/MIJIA_RECORD_VIDEO -name \"*.mp4\" > /tmp/allfile.txt";
const char  *sort_timestamp = "sort /tmp/allfile_ts.txt > /tmp/allfile_ts.sort";
int main(int argn ,char *args[])
{
	char  rslt[128];
	/*
	find ./ -name "*.mp4" > /tmp/allfile.txt
	#通过程序截取文件名中的时间戳.. 1 表示截取时间戳 
	./exescanfile 1 /tmp/allfile.txt /tmp/allfile_ts.txt
	#排序
	sort /tmp/allfile_ts.txt > /tmp/allfile_ts.sort
	#通过程序转换成hex
	./exescanfile 2 /tmp/allfile_ts.sort /tmp/allfile_ts.32bit
	#拷贝更新时间戳文件
	#cp /tmp/allfile_ts.32bit /mnt/media/mmcblk0p1/MIJIA_RECORD_VIDEO/.timestamp -rf
	cp /tmp/allfile_ts.32bit ./timestamp -rf

	rm /tmp/allfile.txt
	rm /tmp/allfile_ts.txt
	rm /tmp/allfile_ts.sort
	rm /tmp/allfile_ts.32bit
	*/
	run_cmd(find_all_cmd, rslt, sizeof(rslt));
	
	deal_get_timestamp("/tmp/allfile.txt","/tmp/allfile_ts.txt");
	
	run_cmd(sort_timestamp, rslt, sizeof(rslt));
	
	convert_to_timestamp("/tmp/allfile_ts.sort","/tmp/allfile_ts.32bit");
	
	run_cmd("cp /tmp/allfile_ts.32bit /mnt/media/mmcblk0p1/MIJIA_RECORD_VIDEO/.timestamp -rf", rslt, sizeof(rslt));
	
	run_cmd("rm /tmp/allfile.txt", rslt, sizeof(rslt));
	run_cmd("rm /tmp/allfile_ts.txt", rslt, sizeof(rslt));
	run_cmd("rm /tmp/allfile_ts.sort", rslt, sizeof(rslt));
	run_cmd("rm /tmp/allfile_ts.32bit", rslt, sizeof(rslt));
	
	return 0;
}
