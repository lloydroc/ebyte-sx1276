bin_PROGRAMS = e32 lorax loratx lorax_echo
e32_SOURCES = main.c options.h options.c e32.h e32.c gpio.c gpio.h uart.h uart.c error.h error.c become_daemon.h become_daemon.c list.h list.c socket.h socket.c
lorax_SOURCES = main_lorax.c misc.h misc.c lorax.h lorax.c convert.h convert.c message.h message.c neighbor.h neighbor.c connection.h connection.c packet.h packet.c error.h error.c options_lorax.h options_lorax.c socket.h socket.c list.h list.c  become_daemon.h become_daemon.c
lorax_LDADD = -lm
loratx_SOURCES = loratx.c error.h error.c message.h message.c socket.h socket.c misc.h misc.c
loratx_LDADD = -lm
lorax_echo_SOURCES = lorax_echo.c error.h error.c message.h message.c socket.h socket.c misc.h misc.c  become_daemon.h become_daemon.c
lorax_echo_LDADD = -lm