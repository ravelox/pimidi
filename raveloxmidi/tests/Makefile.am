AM_CPPFLAGS = -I $(top_srcdir)/include

check_PROGRAMS = sdk_lifecycle_test
TESTS = sdk_lifecycle_test

sdk_lifecycle_test_SOURCES = sdk_lifecycle_test.c
sdk_lifecycle_test_LDADD = $(top_builddir)/src/libraveloxmidi.la

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
