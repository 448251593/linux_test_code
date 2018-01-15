#include<stdio.h>

#define   Conn(x,y) x##y
#define   CONFIG_WIFI_MODE    /mnt/data/miio_av/config_wifi_mode
#define   CONFIG_WIFI_MODE_CAT_CMD   Conn("cat" ,"/mnt/data/miio_av/config_wifi_mode")

 
#define f(a,b) a##b  
#define g(a) #a  
#define h(a) g(a)  
int main()  
{  
	printf("%s\n", CONFIG_WIFI_MODE_CAT_CMD);  
// printf("%s\n",g(f(1,2)));  
 return 0;  
}  