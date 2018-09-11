#ifndef _LOGGING_H
#define _LOGGING_H

#include <unistd.h>
#include <stdarg.h>

typedef struct {
	char *name;
	int value;
} name_map_t;

#define LOGGING_DEBUG	0
#define LOGGING_INFO	1
#define LOGGING_NORMAL	2
#define LOGGING_WARN	3
#define LOGGING_ERROR	4

#ifdef INSIDE_LOGGING
int logging_threshold = 3;
#else
extern int logging_threshold;
#define DEBUG_ONLY	if(logging_threshold!=LOGGING_DEBUG) return;
#endif

int logging_name_to_value(name_map_t *map, const char *name);
char *logging_value_to_name(name_map_t *map, int value);
void logging_printf(int level, const char *format, ...);
void logging_init(void);
void logging_teardown(void);
void logging_prefix_enable(void);
void logging_prefix_disable(void);

#endif
