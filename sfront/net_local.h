
#include <signal.h>

#ifndef NSYS_MIDIEVENTS 
#define NSYS_MIDIEVENTS 1
#endif
 
#ifndef NSYS_NETSTART
#define NSYS_NETSTART 64
#endif

#ifndef NSYS_NET
extern sig_atomic_t graceful_exit;
#endif

