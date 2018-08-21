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


int logging_is_debug( void );
int logging_name_to_value(name_map_t *map, const char *name);
char *logging_value_to_name(name_map_t *map, int value);
void logging_printf(int level, const char *format, ...);
void logging_init(void);
void logging_stop(void);
void logging_prefix_enable(void);
void logging_prefix_disable(void);

#endif
