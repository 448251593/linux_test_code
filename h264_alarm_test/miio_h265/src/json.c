/**
 * @file json.c json related functions
 *
 * Copyright (C) 2016 Xiaomi
 * Author: Yin Kangkai <yinkangkai@xiaomi.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>		/* Definition of uint64_t */
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/time.h>
#include <miio/log.h>

#include "json-c/json.h"

#include "json.h"

/**
 * Verify if the input "string" is a valid json string.
 *
 * @return:
 *	0:  json verification pass;
 *	-1: json verification fail;
 */
int json_verify(const char *string)
{
	struct json_object *new_obj;

	new_obj = json_tokener_parse(string);
	if (new_obj == NULL) {
		log_err("%s: Not in json format: %s\n", __func__, string);
		json_object_put(new_obj);
		return -1;
	}

	json_object_put(new_obj);
	return 0;
}

/**
 * @brief Verify if it's a certain json method
 *
 * @param string: the json string
 * @param method: method string to check
 *
 * @return 0 on success, -1 on failure.
 */
int json_verify_method(const char *string, char *method)
{
	struct json_object *new_obj, *tmp_obj;

	new_obj = json_tokener_parse(string);
	if (new_obj == NULL) {
		log_err("%s: Not in json format: %s\n", __func__, string);
		json_object_put(new_obj);
		return -1;
	}

	if (!json_object_object_get_ex(new_obj, method, &tmp_obj)) {
		json_object_put(new_obj);
		return -1;
	}

	json_object_put(new_obj);
	return 0;
}

/**
 * @brief Verify if there is a "method":"value" pair in string
 *
 * @param string: the json string
 * @param method: method string to be checked
 * @param value: value to be checked
 * @param type: json_type of value
 *
 * @return 0 on success, -1 on failure.
 */
int json_verify_method_value(const char *string, char *method, void *value, enum json_type type)
{
	struct json_object *new_obj, *tmp_obj;
	const char *s;
	int32_t i;

	new_obj = json_tokener_parse(string);
	if (new_obj == NULL) {
		log_err("%s: Not in json format: %s\n", __func__, string);
		json_object_put(new_obj);
		return -1;
	}

	if (!json_object_object_get_ex(new_obj, method, &tmp_obj)) {
		json_object_put(new_obj);
		return -1;
	}

	if (!json_object_is_type(tmp_obj, type)) {
		json_object_put(new_obj);
		return -1;
	}

	switch (type) {
	case json_type_string:
		s = json_object_get_string(tmp_obj);
		if (strlen(s) == strlen((char *)value) && strncmp(s, (char *)value, strlen(s)) == 0) {
			json_object_put(new_obj);
			return 0;
		} else {
			json_object_put(new_obj);
			return -1;
		}

		break;
	case json_type_int:
		i = json_object_get_int(tmp_obj);
		if (i == *(int *)value) {
			json_object_put(new_obj);
			return 0;
		} else {
			json_object_put(new_obj);
			return -1;
		}

		break;
	case json_type_boolean:
	case json_type_double:
	default:
		break;
	}

	json_object_put(new_obj);
	return -1;
}

/**
 * @brief Verify and get the "key" value, value it type of "int"
 *
 * @param string: the json string
 * @param key: key to get
 * @param value: key's value
 *
 * @return 0 on success, -1 on failure.
 */
int json_verify_get_int(const char *string, char *key, int *value)
{
	struct json_object *new_obj, *tmp_obj;

	new_obj = json_tokener_parse(string);
	if (new_obj == NULL) {
		log_err("%s: Not in json format: %s\n", __func__, string);
		return -1;
	}

	if (!json_object_object_get_ex(new_obj, key, &tmp_obj)) {
		json_object_put(new_obj);
		return -1;
	}

	if (!json_object_is_type(tmp_obj, json_type_int)) {
		json_object_put(new_obj);
		return -1;
	}

	*value = json_object_get_int(tmp_obj);

	json_object_put(new_obj);
	return 0;
}

/**
 * @brief Verify and get the "key" value, value it type of "char *"
 *
 * @param string: the json string
 * @param key: key to get
 * @param value: key's value
 *
 * @return 0 on success, -1 on failure.
 */
int json_verify_get_string(const char *string, char *key, char *value, int val_len)
{
	struct json_object *new_obj, *tmp_obj;

	new_obj = json_tokener_parse(string);
	if (new_obj == NULL) {
		log_err("%s: Not in json format: %s\n", __func__, string);
		return -1;
	}

	if (!json_object_object_get_ex(new_obj, key, &tmp_obj)) {
		json_object_put(new_obj);
		return -1;
	}

	if (!json_object_is_type(tmp_obj, json_type_string)) {
		json_object_put(new_obj);
		return -1;
	}

	strncpy(value, json_object_get_string(tmp_obj), val_len - 1);

	json_object_put(new_obj);
	return 0;
}

/**
 * @brief Verify and get the "key" value, value it type of "char *"
 *
 * @param string: the json string
 * @param key: key to get
 * @param value: key's value
 *
 * @return 0 on success, -1 on failure.
 */
int json_verify_get_array(const char *string, char *key, char *value, int val_len)
{
	struct json_object *new_obj, *tmp_obj;

	new_obj = json_tokener_parse(string);
	if (new_obj == NULL) {
		log_err("%s: Not in json format: %s\n", __func__, string);
		return -1;
	}

	if (!json_object_object_get_ex(new_obj, key, &tmp_obj)) {
		json_object_put(new_obj);
		return -1;
	}

	if (!json_object_is_type(tmp_obj, json_type_array)) {
		json_object_put(new_obj);
		return -1;
	}

	int cnt = 0;
	const char *strTmp = NULL;
	int len = json_object_array_length(tmp_obj);
	struct json_object *loop_obj;
	int i = 0;
	for (i = 0; i < len; i++) {
		loop_obj = json_object_array_get_idx(tmp_obj, i);
		strTmp = json_object_get_string(loop_obj);
		if (NULL != strTmp) {
			strncpy(value + cnt, strTmp, val_len - cnt - 1);
			cnt += strlen(strTmp);
		}
	}

	json_object_put(new_obj);
	return 0;
}
