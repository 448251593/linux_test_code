#include<stdio.h>
#include<string.h>
#include <stdlib.h>

// char *str_code="3200,1600,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,1260,420,1260,420,420,420,420,420,420,420,1260,420,420,420,420,420,1260,420,420,420,420,420,1260,420,1260,420,420,420,1260,420,1260,420,420,420,1260,420,1260,420,420,420,1260,420,1260,420,420,420,420,420,1260,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,420,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,1260,420,420,420,1260,420,1260,420,420,420,1260,420,1260,420,1260,420,420,420,1260,420,420,420,420,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,1260,420,420,420,1260,420,420,420,420,420,420,420,1260,420,1260,420,420,420,1260,420,420,420,1260,420,1260,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,420,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1260,420,1000";
char *str_code="3200,1600,420,1260,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,1260,420,420,420,420";

typedef unsigned int uint32_t;

typedef struct {
	union {
		struct {
			uint32_t duration0 :15;
			uint32_t level0 :1;
			uint32_t duration1 :15;
			uint32_t level1 :1;
		};
		uint32_t val;
	};
} rmt_item32_t;


int  get_split_num(char *pdata, int len, char  split_char)
{
	int ulI = 0;
	int nums = 0;
	char *pt = pdata;
	for ( ulI = 0; ulI < len; ulI++ )
	{
		if (*(pdata+ulI) == split_char)
		{
			nums++;
		}
	}
	/* 返回数值 是分隔符数量+1*/
	return nums+1;
}


int  send_code_to_ir(char *pdata, int len)
{
	int splitnums = 0, i;
	rmt_item32_t* item, *pitem;
	int  item_offset = 0;
	char *pt1,*p_find_start, tmp_val_c[10];
	 size_t size;
	 int item_nums;

	int group_ct;

	//计算元素数量
	splitnums = get_split_num(pdata, len, ',');	
	printf("splitnums=%d\n", splitnums);

	/*2个elem 组成一个  的波形*/
	item_nums = (splitnums ) / 2;	
    if ((splitnums % 2) != 0)			
    {
    	item_nums++;
    }
	//申请内存,并填充码值
    size = (sizeof(rmt_item32_t) * item_nums);
	printf("item nums=%d\n", item_nums);
	
	pitem = (rmt_item32_t *) malloc(size);
	item = pitem;
	#if 1
	p_find_start = pdata;
	while(1)
	{
		//查找','
	     pt1 = strchr(p_find_start, ',');
	
		 if(pt1)
		 {
			//把',' 改为0,最后在改回来
			*pt1 = 0;
			//利用	group_ct 分离成 2个元素一组的码值(构成一个波形)		
			group_ct++;
			if (group_ct == 1)
			{
				item->duration0 = atoi(p_find_start);
				item->level0 = 1;
			}
			else if (group_ct == 2)
			{
				
				group_ct = 0;
				  
				item->duration1 = atoi(p_find_start);
				item->level1 = 1;

				item++;
				
			}
			//把',' 改为0,最后在改回来
			*pt1 = ',';
			
			//next	
			p_find_start = pt1 + 1;
			if((int)(p_find_start - pdata) >  len)
			{
				break;
			}
		 }
		 else
		 {
		 	printf("last = %s\n", p_find_start);
			item->duration1 = atoi(p_find_start);
			item->level1 = 1;			
	     	break;
		 }
		 
	}
	#endif

	free(pitem);
}
#define HEAT_SHRINK_BUF_SIZE 1024
typedef struct 
{
	uint32_t  				infrared_wave_data[200];
//	char      				infrared_wave_data_split[200];//接收码值中断标记 1 
	int       				wave_data_len;
//	learn_mode_enum      	learn_flag;//锁定标识; -1 -> 完整结束 ;0->新建立一个码值;//1->接收中
//	char 					key[24];
}learn_wave_struct;
learn_wave_struct learn_wave_data_buf;
typedef unsigned char uint8_t;

int  get_learn_code(char *k,char *pout, int *len)
{
    int ulI = 0;
	learn_wave_struct *pln_code = &learn_wave_data_buf;
	
	//--------------------------------------------------------
	char *heat_shrink_buf = NULL;
	int   heat_shrink_buf_len = *len;
	char *heat_shrink_buf_after = NULL;
	rmt_item32_t  *tmp_code;
	
	heat_shrink_buf = pout;//(char *) malloc(HEAT_SHRINK_BUF_SIZE);
	
	if (heat_shrink_buf == NULL)
	{		
		return -4;		
	}
	
	heat_shrink_buf_after = (char *) malloc(HEAT_SHRINK_BUF_SIZE/2);
	if (heat_shrink_buf_after == NULL)
	{
	    return -4;
	}
	
	memset(heat_shrink_buf, 0, heat_shrink_buf_len);
	memset(heat_shrink_buf_after, 0, HEAT_SHRINK_BUF_SIZE/2);
	
	int	i = 0;
	for ( ulI = 0; ulI < pln_code->wave_data_len; ulI++ )  
	{		
		tmp_code = (rmt_item32_t*)(&pln_code->infrared_wave_data[ulI]); 
		i += snprintf((char*)(heat_shrink_buf + i), heat_shrink_buf_len - i, "%d,", tmp_code->duration0);
		i += snprintf((char*)(heat_shrink_buf + i), heat_shrink_buf_len - i, "%d,", tmp_code->duration1);
	}
	//delete last ',' 
	heat_shrink_buf[strlen(heat_shrink_buf) - 1] = '\0';
	printf("heat_shrink_buf=%s\n", heat_shrink_buf);
	//压缩
//	int en_len = encode_chuangmi_app((uint8_t *)heat_shrink_buf, i,(uint8_t *) heat_shrink_buf_after, HEAT_SHRINK_BUF_SIZE/2);
	//base64
//	memset(heat_shrink_buf, 0, heat_shrink_buf_len);
//	int len_base64 = Base64Encode(heat_shrink_buf_after, en_len, heat_shrink_buf);
	free(heat_shrink_buf_after);
	

//	*pout = heat_shrink_buf;
//	*len = len_base64;
	
	return 0;
}
int main()
{
    int ulI = 0;

	
//	char *tmp = 0;
//	tmp = malloc(strlen(str_code)+1);

//	memset(tmp, 0, strlen(str_code)+1);
//	strcpy(tmp,str_code);
//	send_code_to_ir((char*)tmp, strlen(tmp));

//	free(tmp);


	char *tmp = 0;
	int  len=2048;
	rmt_item32_t  item;

	tmp = malloc(2048);


	
	for ( ulI = 0; ulI < 10; ulI++ )
	{
		item.duration0 = 100;
		item.duration1 = 200;		
	    learn_wave_data_buf.infrared_wave_data[ulI] = (item.val);
		
	}
	learn_wave_data_buf.wave_data_len = ulI;
	get_learn_code("111", tmp, &len);
	
	free(tmp);

	
	return 0;
}




