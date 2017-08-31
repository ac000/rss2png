/*
 * rss2png.c
 *
 * Copyright (c) 2017		Securolytics, Inc.
 *				Andrew Clayton <andrew.clayton@securolytics.io>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <cairo.h>

#define FEED_URL		"https://blog.securolytics.io/feed/"
#define BLOG_URL		"https://blog.securolytics.io/"
#define IMG_NAME		"/var/tmp/rss.png"

#define err_exit(...)  \
	do { \
		fprintf(stderr, __VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while (0)

struct curl_buf {
	char *buf;
	size_t size;
};

static struct curl_buf buf;
static char title[64];
static char summary[64];
static char *env_debug;

static void create_image(void)
{
	cairo_surface_t *surface =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 450, 68);
	cairo_t *cr = cairo_create(surface);

	if (env_debug)
		printf("Gnerating image...\n");

	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);

	cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 14.0);
	cairo_move_to(cr, 5.0, 20.0);
	cairo_show_text(cr, title);

	cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12.0);
	cairo_move_to(cr, 7.0, 40.0);
	cairo_show_text(cr, summary);

	cairo_set_font_size(cr, 10.0);
	cairo_move_to(cr, 5.0, 62.0);
	cairo_show_text(cr, BLOG_URL);

	cairo_destroy(cr);
	cairo_surface_write_to_png(surface, IMG_NAME);
	cairo_surface_destroy(surface);
}

static void find_item(void)
{
	char *ptr;
	int i = 0;

	ptr = strstr(buf.buf, "<item>");
	if (!ptr)
		err_exit("Couldn't find <item> tag\n");
	ptr = strstr(ptr, "<title>");
	if (!ptr)
		err_exit("Couldn't find <title> tag\n");
	ptr += 7;

	while (*ptr != '<' && i < 50) {
		title[i++] = *ptr;
		ptr++;
	}
	title[i] = '\0';
	if (i == 50)
		strcat(title, " ...");

	ptr = strstr(ptr, "<description>");
	if (!ptr)
		err_exit("Couldn't find <description> tag\n");
	ptr = strstr(ptr, "<p>");
	if (!ptr)
		err_exit("Couldn't find <p> tag\n");
	ptr += 3;

	i = 0;
	while (*ptr != '<' && i < 50) {
		summary[i++] = *ptr;
		ptr++;
	}
	summary[i] = '\0';
	if (i == 50)
		strcat(summary, " ...");

	if (env_debug) {
		printf("Extracted title and summary :-\n");
		printf("  title   : %s\n", title);
		printf("  summary : %s\n", summary);
	}
}

static size_t curl_cb(void *contents, size_t size, size_t nmemb,
		      void *userp __attribute__((unused)))
{
	size_t realsize = size * nmemb;

	buf.buf = realloc(buf.buf, buf.size + realsize + 1);

	memcpy(buf.buf + buf.size, contents, realsize);
	buf.size += realsize;
	buf.buf[buf.size] = '\0';

	return realsize;
}

static void get_feed(void)
{
	CURL *curl;
	CURLcode res;

	if (env_debug)
		printf("Fetching feed: %s\n", FEED_URL);

	buf.buf = malloc(1);
	buf.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, FEED_URL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_cb);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
		err_exit("curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

int main(void)
{
	env_debug = getenv("RSS2PNG_DEBUG");

	get_feed();
	find_item();
	create_image();

	free(buf.buf);

	exit(EXIT_SUCCESS);
}
