RSS2PNG_VERSION = $(shell git describe --dirty --always)

CC 	= gcc
CFLAGS	= -Wall -Wextra -std=c99 -O2 -g -Wp,-D_FORTIFY_SOURCE=2 -D_FILE_OFFSET_BITS=64 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -fPIC $(shell pkg-config --cflags cairo) -DRSS2PNG_VERSION='"${RSS2PNG_VERSION}"'
LDFLAGS = -Wl,-z,relro,-z,now,--as-needed -pie $(shell pkg-config --libs cairo) -lcurl

rss2png: rss2png.c
	 $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f rss2png
