test_options_CFLAGS = -I$(top_srcdir)/src
test_options_LDADD = ../src/options.o ../src/error.o

check_PROGRAMS = test_options
test_options_SOURCES = test_options.c $(top_builddir)/src/options.h $(top_builddir)/src/error.h
TESTS = $(check_PROGRAMS)
