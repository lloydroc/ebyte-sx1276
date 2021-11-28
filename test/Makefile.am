test_options_CFLAGS = -I$(top_srcdir)/src
test_options_LDADD = ../src/options.o ../src/error.o ../src/socket.o

test_list_CFLAGS = -I$(top_srcdir)/src
test_list_LDADD = ../src/list.o

test_packet_CFLAGS = -I$(top_srcdir)/src
test_packet_LDADD = ../src/packet.o ../src/message.o ../src/convert.o ../src/error.o ../src/misc.o -lm

test_message_CFLAGS = -I$(top_srcdir)/src
test_message_LDADD = ../src/packet.o ../src/message.o ../src/convert.o ../src/error.o ../src/misc.o -lm

test_options_lorax_CFLAGS = -I$(top_srcdir)/src
test_options_lorax_LDADD = ../src/options_lorax.o ../src/error.o ../src/socket.o ../src/misc.o -lm

test_misc_CFLAGS = -I$(top_srcdir)/src
test_misc_LDADD = ../src/misc.o ../src/error.o -lm

test_control_CFLAGS = -I$(top_srcdir)/src
test_control_LDADD = ../src/error.o ../src/control.o ../src/list.o ../src/neighbor.o ../src/connection.o ../src/misc.o -lm

check_PROGRAMS = test_options test_options_lorax test_list test_packet test_message test_misc test_control
test_options_SOURCES = test_options.c $(top_builddir)/src/options.h $(top_builddir)/src/error.h $(top_builddir)/src/socket.h
test_options_lorax_SOURCES = test_options_lorax.c $(top_builddir)/src/options_lorax.h $(top_builddir)/src/error.h $(top_builddir)/src/socket.h $(top_builddir)/src/misc.h
test_list_SOURCES = test_list.c $(top_builddir)/src/list.h
test_packet_SOURCES = test_packet.c $(top_builddir)/src/packet.h $(top_builddir)/src/message.h $(top_builddir)/src/convert.h $(top_builddir)/src/error.h $(top_builddir)/src/misc.h
test_message_SOURCES = test_message.c $(top_builddir)/src/message.h $(top_builddir)/src/packet.h $(top_builddir)/src/convert.h $(top_builddir)/src/error.h $(top_builddir)/src/misc.h
test_misc = test_misc.c $(top_builddir)/src/misc.h $(top_builddir)/src/error.h
test_control = test_control.c $(top_builddir)/src/error.h $(top_builddir)/src/control.h $(top_builddir)/src/list.h $(top_builddir)/src/neighbor.h $(top_builddir)/src/connection.h $(top_builddir)/src/misc.h
TESTS = $(check_PROGRAMS)
