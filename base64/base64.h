#ifndef __BASE64_H__
#define __BASE64_H__

//#include "lib/lib_generic.h"

int Base64Decode(char *b64_inbuf, long b64_len, char *b64_outbuf);


// 3xn > 4xn β������0 ���س���
int Base64Encode(char *str_inbuf, long str_len, char *b64_outbuf);


#endif /* __BASE64_H__ */

