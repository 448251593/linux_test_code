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

int main(int argn ,char *args[])
{
	
	if(argn != 4)
	{
		printf("./exe file_in file_out\n");
		return 0;
	}
	//printf("filein name=%s,fileout name=%s\n",args[2],args[3]);
	
	if(*args[1] == '1')
	{
		deal_get_timestamp(args[2],args[3]);	
	}
	else if(*args[1] == '2')
	{		
		convert_to_timestamp(args[2],args[3]);		
	}
	
	return 0;
}
