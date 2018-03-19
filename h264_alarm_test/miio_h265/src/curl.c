/*
 * Copyright (c) 2018 Beijing Xiaomi Mobile Software Co., Ltd. All rights reserved.
 */

#include <assert.h>
#include <pthread.h>
#include <curl/curl.h>
#include <stdlib.h>
#include "curl.h"
#include <miio/log.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static CURL *curl_ctx = NULL;

static int debug_fn(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
	if (CURLINFO_TEXT == type) {
		log_debug("== Info: %s\n", data);
	}
	return 0;
}

static size_t curl_write_func(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	// will size * nmemb larger than INT_MAX??
	log_debug("curl: %*s", (int) (size * nmemb), ptr);
	return size * nmemb;
}

static void curl_setup(CURL *ctx, const char *url, FILE *fp, size_t file_size)
{
	curl_easy_setopt(ctx, CURLOPT_DEBUGFUNCTION, debug_fn);
	curl_easy_setopt(ctx, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(ctx, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(ctx, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(ctx, CURLOPT_WRITEFUNCTION, curl_write_func);

	curl_easy_setopt(ctx, CURLOPT_READDATA, fp);
	curl_easy_setopt(ctx, CURLOPT_INFILESIZE_LARGE, (curl_off_t) file_size);
	curl_easy_setopt(ctx, CURLOPT_URL, url);
}

static void log_result(CURL *ctx, size_t file_size)
{
	double speed_upload = 0, total_time = 0, upload_size = 0;

	curl_easy_getinfo(ctx, CURLINFO_SPEED_UPLOAD, &speed_upload);
	curl_easy_getinfo(ctx, CURLINFO_TOTAL_TIME, &total_time);
	curl_easy_getinfo(ctx, CURLINFO_SIZE_UPLOAD, &upload_size);
	log_info("File size: %zu bytes, Upload size: %.3f bytes, Speed: %.3f bytes/sec during %.3f s\n",
	         file_size, upload_size, speed_upload, total_time);
}

int mcurl_upload(const char *filename, const char *url)
{
	assert(filename != NULL);
	assert(url != NULL);
	assert(curl_ctx != NULL);
	int rc = 0;
	int i;
	FILE *fp = NULL;
	struct stat st;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		rc = -1;
		goto out;
	}
	rc = fstat(fileno(fp), &st);
	if (rc < 0) {
		goto out;
	}
	pthread_mutex_lock(&lock);
	curl_setup(curl_ctx, url, fp, (size_t) st.st_size);
	for (i = 0; i < 3; ++i) {
		rc = curl_easy_perform(curl_ctx);
		if (rc == CURLE_OK) {
			log_result(curl_ctx, (size_t) st.st_size);
			goto out_reset;
		}
	}
	log_err("Can not upload %s to %s: %s (%d)\n", filename, url, curl_easy_strerror((CURLcode) rc), rc);
	rc = -1;
out_reset:
	curl_easy_reset(curl_ctx);
	pthread_mutex_unlock(&lock);
out:
	if (fp != NULL) {
		fclose(fp);
	}
	return rc;
}


void mcurl_init()
{
	pthread_mutex_init(&lock, NULL);
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_ctx = curl_easy_init();
	if (!curl_ctx) {
		log_crit("curl_create: %s\n", strerror(errno));
		abort();
	}
}

void mcurl_exit()
{
	if (curl_ctx) {
		curl_easy_cleanup(curl_ctx);
		curl_ctx = NULL;
	}
	curl_global_cleanup();
}
