CC = gcc
CFLAGS = $(shell pkg-config --cflags glib-2.0)
LIBS = $(shell pkg-config --libs glib-2.0)

SRCS = mount-tracker.c
OBJS = $(SRCS:.c=.o)

all: mount-tracker

mount-tracker: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) mount-tracker

