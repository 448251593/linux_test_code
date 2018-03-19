#ifndef __JSON_H
#define __JSON_H
#include "json-c/json.h"

enum json_type;

int json_verify(const char *string);
int json_verify_method(const char *string, char *method);
int json_verify_method_value(const char *string, char *method, void *value, enum json_type type);
int json_verify_get_int(const char *string, char *key, int *value);
int json_verify_get_string(const char *string, char *key, char *value, int val_len);
int json_verify_get_array(const char *string, char *key, char *value, int val_len);

#endif
