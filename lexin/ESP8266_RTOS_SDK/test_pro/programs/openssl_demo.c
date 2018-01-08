#include <stddef.h>
#include "openssl_demo.h"
#include "openssl/ssl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "espressif/c_types.h"
#include "lwip/sockets.h"

#define OPENSSL_DEMO_THREAD_NAME "ssl_demo"
#define OPENSSL_DEMO_THREAD_STACK_WORDS 2048
#define OPENSSL_DEMO_THREAD_PRORIOTY 6

#define OPENSSL_DEMO_FRAGMENT_SIZE 8192

#define OPENSSL_DEMO_LOCAL_TCP_PORT 1000
//http://p1.pstatp.com/origin/3ecc0001f2adaecc0dd4

#define OPENSSL_DEMO_TARGET_NAME "p1.pstatp.com"
#define OPENSSL_DEMO_TARGET_PATH "/origin/3ecc0001f2adaecc0dd4"

//#define OPENSSL_DEMO_TARGET_NAME "f.hiphotos.baidu.com"
//#define OPENSSL_DEMO_TARGET_PATH "/image/pic/item/a5c27d1ed21b0ef4069800b4d7c451da80cb3ee3.jpg"

#define OPENSSL_DEMO_TARGET_TCP_PORT 80
#define OPENSSL_DEMO_REQUEST "GET "OPENSSL_DEMO_TARGET_PATH" HTTP/1.1\r\n" \
"User-Agent: esp8266\r\n" \
"Host: "OPENSSL_DEMO_TARGET_NAME"\r\n" \
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n" \
"\r\n"

#define OPENSSL_DEMO_RECV_BUF_LEN 1024

LOCAL xTaskHandle openssl_handle;

unsigned int recv_heard_data_len;
char 		 recv_heard_data[1024];

unsigned int recv_body_data_len;

LOCAL char send_data[] = OPENSSL_DEMO_REQUEST;
LOCAL int send_bytes = sizeof(send_data);

LOCAL char recv_buf[OPENSSL_DEMO_RECV_BUF_LEN];


typedef struct  
{
	 int offset;
	 int total_len;
	unsigned char *pdata;
	 int pdata_len;	
}write_info_struct;

write_info_struct write_info;

void  init_write_info(write_info_struct * p_write_info)
{

	p_write_info->offset = 0;
	p_write_info->total_len = 0;
	p_write_info->pdata = 0;	
	p_write_info->pdata_len = 0;

}



int  write_img_flash(write_info_struct * p_write_info)
{

	printf("write offset=%d, len= %d\n",p_write_info->offset,p_write_info->pdata_len);
	
	p_write_info->offset = p_write_info->offset + p_write_info->pdata_len;

	if(p_write_info->offset == p_write_info->total_len)
	{
		printf("all file recv\n");
	}
	
}







LOCAL void httpdownload_demo_thread(void *p)
{
    int ret;

    int socket;
    struct sockaddr_in sock_addr;

    ip_addr_t target_ip;

    int recv_bytes = 0;

    os_printf("OpenSSL demo thread start...\n");

    do {
        ret = netconn_gethostbyname(OPENSSL_DEMO_TARGET_NAME, &target_ip);
    } while(ret);
    os_printf("get target IP is %d.%d.%d.%d\n", (unsigned char)((target_ip.addr & 0x000000ff) >> 0),
                                                (unsigned char)((target_ip.addr & 0x0000ff00) >> 8),
                                                (unsigned char)((target_ip.addr & 0x00ff0000) >> 16),
                                                (unsigned char)((target_ip.addr & 0xff000000) >> 24));



    os_printf("create socket ......");
    socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        os_printf("failed\n");
        goto failed3;
    }
    os_printf("OK\n");

    os_printf("bind socket ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = 0;
    sock_addr.sin_port = htons(OPENSSL_DEMO_LOCAL_TCP_PORT);
    ret = bind(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (ret) {
        os_printf("failed\n");
        goto failed4;
    }
    os_printf("OK\n");

    os_printf("socket connect to remote ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = target_ip.addr;
    sock_addr.sin_port = htons(OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = connect(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (ret) {
        os_printf("failed\n", OPENSSL_DEMO_TARGET_NAME);
        goto failed5;
    }
    os_printf("OK\n");



    os_printf("send request to %s port %d ......", OPENSSL_DEMO_TARGET_NAME, OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = send(socket, send_data, send_bytes,0);
    if (ret <= 0) {
        os_printf("failed, return [-0x%x]\n", -ret);
        goto failed8;
    }
    os_printf("OK\n\n");


	
	
	//接收
	int header_end_flag =0;
	recv_heard_data_len = 0;
    do {		       	
		//------4个\r\n处理
		if(header_end_flag < 2)
        {  
	        ret = recv(socket, recv_buf, 1, 0); 
	        if (ret <= 0) {
				os_printf("recv over\n");
	            break;
	        }
//          printf("%c", recv_buf[0]);
			recv_heard_data[recv_heard_data_len++] = recv_buf[0];
			if (header_end_flag < 1)
			{
			    char *p1 = strstr(recv_heard_data,"Content-Length");
				if(p1)
				{
					char *p2 = strstr(p1,":");
					char *p3 = strstr(p1,"\r\n");
					if(p2 && p3)
					{
						recv_body_data_len = atoi(p2+1);
						os_printf("Content-Length=%d\n",atoi(p2+1));
						header_end_flag = 1;
						write_info.total_len = recv_body_data_len;
					}
				}
			}
			
			if(strstr(recv_heard_data,"\r\n\r\n"))
			{
				header_end_flag = 2;
			}
        }
		else
		{
			ret = recv(socket, recv_buf, OPENSSL_DEMO_RECV_BUF_LEN-1, 0); 
	        if (ret <= 0) {
				os_printf("recv over\n");
	            break;
	        }
		    recv_bytes += ret;
			os_printf("downloading... %d%%\n", recv_bytes*100/recv_body_data_len);		
			write_info.pdata = recv_buf;
			write_info.pdata_len= ret;			
			write_img_flash(&write_info);
			
			if(recv_bytes==recv_body_data_len)
			{
				break;
			}
		}
        
    } while (1);
    os_printf("read %d bytes data from %s ......\n", recv_bytes, OPENSSL_DEMO_TARGET_NAME);

failed8:
//    SSL_shutdown(ssl);
failed7:
//    SSL_free(ssl);
failed6:
failed5:
failed4:
    close(socket);
failed3:
failed2:
//    SSL_CTX_free(ctx);
failed1:
    vTaskDelete(NULL);

    os_printf("task exit\n");

    return ;
}

void user_conn_init(void)
{
    int ret;
	init_write_info(&write_info);

    ret = xTaskCreate(httpdownload_demo_thread,
                      OPENSSL_DEMO_THREAD_NAME,
                      OPENSSL_DEMO_THREAD_STACK_WORDS,
                      NULL,
                      OPENSSL_DEMO_THREAD_PRORIOTY,
                      &openssl_handle);
    if (ret != pdPASS)  {
        os_printf("create thread %s failed\n", OPENSSL_DEMO_THREAD_NAME);
        return ;
    }
}

