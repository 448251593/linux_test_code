#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>

int main(int argc,char *argv[])
{

	int opt;
  struct option longopts[] = {
    {"initialize",0,NULL,'1'},
    {"file",1,NULL,'f'},
    {"list",1,NULL,'1'},
    {"restart",0,NULL,'r'},
    {0,0,0,0}};
	
	  while((opt = getopt_long(argc, argv, "if:l:r", longopts, NULL)) != -1){
		switch(opt){
		case 'i':
		
		case 'r':
		  printf("option: %c\n",opt);
		  break;
		case 'f':
		case 'l':
		  printf("optarg: %s\n",optarg);
		  break;
		case ':':
		  printf("option needs a value\n");
		  break;
		case '?':
		  printf("unknown option: %c\n",optopt);
		  break;
	  }
		for(; optind < argc; optind++){
		  printf("argument: %s\n",argv[optind]);
		}
	   // exit(0);
		return 0;
	
	}
}


