
#include <string.h>
#include <stdio.h>

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

static void get_cpu_idle(float *idle_value)
{

	char  cpu_idle[50];
	const char *cmd_cpu="mpstat |awk 'NR==4{print $11}'";
	memset(cpu_idle,0, sizeof(cpu_idle));
	
	run_cmd(cmd_cpu,cpu_idle,sizeof(cpu_idle));
	if (strlen(cpu_idle) > 0)
	{
	    printf( "cpu_idle = %s\n",cpu_idle);
		
			sscanf(cpu_idle, "%f", idle_value);
			printf( "cpu_idle_float = %f\n",*idle_value);
		
	}
	
	
	
} 
int main()
{
	float idl = 0;
	get_cpu_idle(&idl);
}