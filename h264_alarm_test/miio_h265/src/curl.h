/*
 * Copyright (c) 2018 Beijing Xiaomi Mobile Software Co., Ltd. All rights reserved.
 */

#ifndef MIIO_H265_CURL_H
#define MIIO_H265_CURL_H

void mcurl_init();

void mcurl_exit();

int mcurl_upload(const char *filename, const char *url);

#endif //MIIO_H265_CURL_H
