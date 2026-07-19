AM_CPPFLAGS = -I $(top_srcdir)/include

check_PROGRAMS = sdk_lifecycle_test timing_test journal_test
TESTS = sdk_lifecycle_test timing_test journal_test

sdk_lifecycle_test_SOURCES = sdk_lifecycle_test.c
sdk_lifecycle_test_CFLAGS = @PTHREAD_CFLAGS@
sdk_lifecycle_test_LDADD = $(top_builddir)/src/.libs/libraveloxmidi.a @PTHREAD_LIBS@ @AVAHI_LIBS@ @ALSA_LIBS@

timing_test_SOURCES = timing_test.c
timing_test_CFLAGS = @PTHREAD_CFLAGS@
timing_test_LDADD = $(top_builddir)/src/.libs/libraveloxmidi.a @PTHREAD_LIBS@ @AVAHI_LIBS@ @ALSA_LIBS@

journal_test_SOURCES = journal_test.c
journal_test_CFLAGS = @PTHREAD_CFLAGS@
journal_test_LDADD = $(top_builddir)/src/.libs/libraveloxmidi.a @PTHREAD_LIBS@ @AVAHI_LIBS@ @ALSA_LIBS@

VALGRIND = valgrind
VALGRIND_FLAGS = \
	--leak-check=full \
	--show-leak-kinds=definite,possible \
	--errors-for-leak-kinds=definite,possible \
	--track-origins=yes \
	--error-exitcode=1 \
	--suppressions=$(top_srcdir)/debug/valgrind.suppressions

check-valgrind:
	@command -v $(VALGRIND) >/dev/null 2>&1 || { \
		echo "$(VALGRIND) is required for make memcheck. Install valgrind or run make docker-memcheck."; \
		exit 127; \
	}

memcheck: check-valgrind $(check_PROGRAMS) $(top_builddir)/src/raveloxmidi
	$(LIBTOOL) --mode=execute $(VALGRIND) $(VALGRIND_FLAGS) ./sdk_lifecycle_test
	$(LIBTOOL) --mode=execute $(VALGRIND) $(VALGRIND_FLAGS) $(top_builddir)/src/raveloxmidi --help
