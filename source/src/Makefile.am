bin_PROGRAMS = policy-milter

policy_milter_SOURCES = \
		pm_main.c \
		avl_tree.c \
		pm_match.c \
		pm_policy.c \
		pm_daemon.c \
		pm_debug.c \
		pm_syslog.c \
		pm_tempfail.c

policy_milter_LDADD	= -lmilter -lpthread

policy_milter_LDFLAGS	= 

EXTRA_DIST = *.h Makefile.unix

INCLUDES =
