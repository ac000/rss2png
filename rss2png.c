/*
 * rss2png.c
 *
 * Copyright (c) 2017		Securolytics, Inc.
 *				Andrew Clayton <andrew.clayton@securolytics.io>
 *
 * Licensed under the MIT license. See MIT-LICENSE.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <gumbo.h>
#include <curl/curl.h>
#include <cairo.h>

#define FEED_URL		"https://blog.securolytics.io/feed/"
#define BLOG_URL		"https://blog.securolytics.io/"
#define DEF_IMG_PATH		"/var/tmp/rss.png"

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
static char sbuf[256];
static char summary[64];
static bool htt_done;
static const char *env_debug;

static void create_image(const char *img_path)
{
	cairo_surface_t *surface =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 450, 68);
	cairo_t *cr = cairo_create(surface);

	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);

	cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 14.0);
	cairo_move_to(cr, 5.0, 20.0);
	cairo_show_text(cr, title);

	if (strlen(summary) == 51) {
		summary[50] = '\0';
		strcat(summary, " ...");
	}
	if (env_debug)
		printf("  summary : %s\n", summary);

	cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12.0);
	cairo_move_to(cr, 7.0, 40.0);
	cairo_show_text(cr, summary);

	cairo_set_font_size(cr, 10.0);
	cairo_move_to(cr, 5.0, 62.0);
	cairo_show_text(cr, BLOG_URL);

	if (env_debug)
		printf("Generating image...\n");

	cairo_destroy(cr);
	cairo_surface_write_to_png(surface, img_path);
	cairo_surface_destroy(surface);
}

static const char *html_to_text(GumboNode *node)
{
	const GumboVector *children;
	unsigned int i;

	if (htt_done)
		return "";

	if (node->type == GUMBO_NODE_TEXT) {
		return node->v.text.text;
	} else if (node->type == GUMBO_NODE_ELEMENT &&
		   node->v.element.tag != GUMBO_TAG_SCRIPT &&
		   node->v.element.tag != GUMBO_TAG_STYLE) {
		children = &node->v.element.children;
		for (i = 0; i < children->length; i++) {
			const char *text = html_to_text(children->data[i]);

			if (strlen(summary) + strlen(text) >= 51) {
				snprintf(summary + strlen(summary),
					 51 - strlen(summary) + 1, "%s", text);
				htt_done = true;
			} else if (strlen(text) > 0) {
				strcat(summary, text);
			}
		}
	}

	return "";
}

static void find_item(void)
{
	char *ptr;
	unsigned int i = 0;

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
	snprintf(sbuf, sizeof(sbuf), "%s", ptr);

	if (env_debug) {
		printf("Extracted title and summary :-\n");
		printf("  title   : %s\n", title);
		printf("  summary : %s\n", sbuf);
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

int main(int argc, char *argv[])
{
	int optind;
	GumboOutput *gparser;
	const char *img_path = NULL;

	while ((optind = getopt(argc, argv, "ho:")) != -1) {
		switch (optind) {
		case 'h':
			printf("Usage: rss2png [-o output]\n");
			exit(EXIT_SUCCESS);
		case 'o':
			img_path = optarg;
			break;
		default:
			printf("Usage: rss2png [-o output]\n");
			exit(EXIT_FAILURE);
		}
	}

	env_debug = getenv("RSS2PNG_DEBUG");
	if (env_debug)
		printf("rss2png %s\n", RSS2PNG_VERSION);

	if (!img_path)
		img_path = DEF_IMG_PATH;

	get_feed();
	find_item();

	gparser = gumbo_parse(sbuf);
	html_to_text(gparser->root);

	create_image(img_path);

	gumbo_destroy_output(&kGumboDefaultOptions, gparser);
	free(buf.buf);

	exit(EXIT_SUCCESS);
}
