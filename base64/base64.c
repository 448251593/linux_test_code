//#include "longcheer/base64/base64.h"
//#include "base64.h"






/* ----------------base64 from:http://base64.sourceforge.net/ ----------------- */

/*
** Translation Table as described in RFC1113
*/
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

/*
** Translation Table to decode (created by author)
*/
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";




/*
** decodeblock
**
** decode 4 '6-bit' characters into 3 8-bit binary bytes
*/
static void decodeblock( unsigned char in[4], unsigned char out[3] )
{   
	out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
	out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
	out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

static int base64_map_rev(char c)
{		   
	int j;
	for(j = 0; j < 65; j++)
	{
		if(c == cb64[j]) 
		{ 
			return j;
		} 
	}
	return 0;
}



int Base64Decode(char *b64_inbuf, long b64_len, char *b64_outbuf) 
{ 

	int i, out_len = 0, n = 0; 
	char raw_buf[4];

	for(;n < b64_len/4; n++) 
	{ 
		for(i = 0; i < 4; i++) 
			raw_buf[i] = base64_map_rev( b64_inbuf[(n * 4) + i]);

		decodeblock((unsigned char*)raw_buf, (unsigned char*)&b64_outbuf[n*3] );
		out_len += 3;
		if(raw_buf[3] == 64)out_len--;
		if(raw_buf[2] == 64)out_len--;
	} 
	return out_len;
} 


static void encodeblock( unsigned char *in, unsigned char *out, int len )
{
	out[0] = (unsigned char) cb64[ (int)(in[0] >> 2) ];
	out[1] = (unsigned char) cb64[ (int)(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)) ];
	out[2] = (unsigned char) (len > 1 ? cb64[ (int)(((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)) ] : '=');
	out[3] = (unsigned char) (len > 2 ? cb64[ (int)(in[2] & 0x3f) ] : '=');
}

// 3xn > 4xn 尾部不加0 返回长度
int Base64Encode(char *str_inbuf, long str_len, char *b64_outbuf)
{
	unsigned char in[3];
	unsigned char out[4];
	int in_len, out_len;
	int i;

	*in = (unsigned char) 0;
	*out = (unsigned char) 0;
	in_len = 0;
	out_len = 0;

	while( in_len < str_len ) {
		in[0] = in[1] = in[2] = 0;

		for( i = 0; i < 3 && in_len < str_len; i++,in_len++) {
			in[i] = (unsigned char) str_inbuf[in_len];
		}
		encodeblock( in, (unsigned char*)(b64_outbuf + out_len), i );
		out_len += 4;

	}

	return out_len;
}




