#include <sys/types.h>          /* See NOTES */
 #include <sys/socket.h>

#include <stdio.h>

#include <netdb.h>
#include <string.h>
#include <errno.h>

#define LOCAL       static

#define OPENSSL_DEMO_THREAD_NAME "ssl_demo"
#define OPENSSL_DEMO_THREAD_STACK_WORDS 2048
#define OPENSSL_DEMO_THREAD_PRORIOTY 6

#define OPENSSL_DEMO_FRAGMENT_SIZE 8192

#define OPENSSL_DEMO_LOCAL_TCP_PORT 1000

#define OPENSSL_DEMO_TARGET_NAME "f.hiphotos.baidu.com"
#define OPENSSL_DEMO_TARGET_PATH "/image/pic/item/a5c27d1ed21b0ef4069800b4d7c451da80cb3ee3.jpg"

#define OPENSSL_DEMO_TARGET_TCP_PORT 80
#define OPENSSL_DEMO_REQUEST "GET "OPENSSL_DEMO_TARGET_PATH" HTTP/1.1\r\n" \
"User-Agent: Fiddler\r\n" \
"Host: "OPENSSL_DEMO_TARGET_NAME"\r\n" \
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n" \
"\r\n"


//GET /50/v2-9c8751a696290647aadc08af80dbb091_hd.jpg HTTP/1.1
//Host: pic2.zhimg.com
//User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
#define OPENSSL_DEMO_RECV_BUF_LEN 1024

#define os_printf(fmt,...)  printf(fmt, ##__VA_ARGS__)

unsigned int recv_heard_data_len;
char recv_heard_data[1024];
unsigned int recv_body_data_len;

LOCAL char send_data[] = OPENSSL_DEMO_REQUEST;
LOCAL int send_bytes = 0;

LOCAL char recv_buf[OPENSSL_DEMO_RECV_BUF_LEN];



/******************************************************************************
 * Description : Connect_dns.连接IPCamera
 * Arguments   :
 * Returns     :
 * Caller        : APP layer.
 * Notes        : none.
 ******************************************************************************/
int Connect_dns(char* Remote_IP, int Remote_port)
{

    struct sockaddr_in server_addr;
    struct hostent *host;
    int portnumber;
    int 					 IPCsockfd;
    if((host=gethostbyname(Remote_IP))==NULL)
    {


        printf("Gethostname error\n");

        return  -1;
    }
    portnumber = Remote_port;
    /* 客户程序开始建立 sockfd描述符  */
    if((IPCsockfd=socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("Socket Error:%s\a\n",strerror(errno));

        return  -1;
    }

    /* 客户程序填充服务端的资料       */
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(portnumber);
    server_addr.sin_addr=*((struct in_addr *)host->h_addr);

    /* 客户程序发起连接请求*/

    printf("connect server..ip=%s:%d\n", Remote_IP, Remote_port);


    if(connect(IPCsockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
    {
        printf("Connect Error:%s\r\n",strerror(errno));
        return  -1;
    }
    return IPCsockfd;
}

LOCAL void demo_http(void *p)
{
    int ret;

    int socket;
    struct sockaddr_in sock_addr;

  

    int recv_bytes = 0;

    os_printf("OpenSSL demo thread start...\n");



  	socket = Connect_dns(OPENSSL_DEMO_TARGET_NAME, OPENSSL_DEMO_TARGET_TCP_PORT);

    os_printf("send request to \n%s \n", send_data);
    ret = send(socket, send_data, send_bytes, 0);
    if (ret <= 0) {
        os_printf("failed, return [-0x%x]\n", -ret);
        goto failed4;
    }
    os_printf("OK\n\n");
	
	FILE			  *HD1;
		
	
	HD1= fopen("test.jpg", "w");//只写
	if(HD1 == NULL)
	{
		printf("UploadPicdata:open file error\r\n");
		goto failed4;
	}
	
	//接收
	int i =0;
	recv_heard_data_len = 0;
    do {
        ret = recv(socket, recv_buf, 4, 0);
        if (ret <= 0) {
			os_printf("recv over\n");
            break;
        }
		//------4个\r\n处理
		if(i < 4)
        {
            if(recv_buf[0] == '\r' || recv_buf[1] == '\n')
            {
                i++;
            }
            else
            {
                i = 0;
            }
			/*把http头信息打印在屏幕上*/
//            printf("%c", recv_buf[0]);
			recv_heard_data[recv_heard_data_len++] = recv_buf[0];
			char *p1 = strstr(recv_heard_data,"Content-Length");
			if(p1)
			{
				char *p2 = strstr(p1,":");
				char *p3 = strstr(p1,"\r\n");
				if(p2 && p3)
				{
					recv_body_data_len = atoi(p2+1);
					os_printf("Content-Length=%d\n",atoi(p2+1));
				}
			}
        }
		else
		{
			i++;
		    recv_bytes += ret;
//       		os_printf("%2x ", (char)recv_buf[0]);
			os_printf("downloading... %d%%\n", recv_bytes*100/recv_body_data_len);
			
			fwrite(recv_buf, ret, 1, HD1);
			if(recv_bytes==recv_body_data_len)
			{
				break;
			}
		}
        
    } while (1);
    os_printf("read %d bytes data from %s ......\n", recv_bytes, OPENSSL_DEMO_TARGET_NAME);
	
	fclose(HD1);//关闭文件
failed6:
failed5:
failed4:
    close(socket);
failed3:
failed2:
//    SSL_CTX_free(ctx);
failed1:
  

    os_printf("task exit\n");

    return ;
}

int main(void)
{
	send_bytes =strlen(send_data);

    int ret;

	demo_http(0);


	return 0;
}


