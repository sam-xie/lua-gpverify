T=gpverify

PREFIX		?=/usr/local
LIB_OPTION	?= -shared

# Lua auto detect
LUAV		?= $(shell lua -e "_,_,v=string.find(_VERSION,'Lua (.+)');print(v)")
LUA_VERSION ?= $(shell pkg-config luajit --print-provides)
ifeq ($(LUA_VERSION),)
	# Not use luajit
	LUA_CFLAGS	?= -I$(PREFIX)/include/lua$(LUAV)
	LUA_LIBS	?= -L$(PREFIX)/lib 
	LUA_LIBDIR	?= $(PREFIX)/lib/lua/$(LUAV)
else
	# Use luajit
	LUA_CFLAGS	?= $(shell pkg-config luajit --cflags)
	LUA_LIBS	?= $(shell pkg-config luajit --libs)
	LUA_LIBDIR	?= $(PREFIX)/lib/lua/$(LUAV)
endif

#OS auto detect
SYS := $(shell gcc -dumpmachine)
ifneq (, $(findstring linux, $(SYS)))
# Do linux things
LDFLAGS			= -fPIC -lrt -ldl
OPENSSL_LIBS	?= $(shell pkg-config openssl --libs)
OPENSSL_CFLAGS	?= $(shell pkg-config openssl --cflags)
CFLAGS			= -fPIC $(OPENSSL_CFLAGS) $(LUA_CFLAGS)
endif
ifneq (, $(findstring apple, $(SYS)))
# Do darwin things
LDFLAGS			= -fPIC -lrt -ldl
OPENSSL_LIBS	?= $(shell pkg-config openssl --libs) 
OPENSSL_CFLAGS	?= $(shell pkg-config openssl --cflags)
CFLAGS			= -fPIC $(OPENSSL_CFLAGS) $(LUA_CFLAGS)
endif
ifneq (, $(findstring mingw, $(SYS)))
# Do mingw things
V			= $(shell lua -e "v=string.gsub('$(LUAV)','%.','');print(v)")
LDFLAGS		= -mwindows -lcrypt32 -lssl -lcrypto -lws2_32 $(PREFIX)/bin/lua$(V).dll 
LUA_CFLAGS	= -DLUA_LIB -DLUA_BUILD_AS_DLL -I$(PREFIX)/include/
CFLAGS		= $(OPENSSL_CFLAGS) $(LUA_CFLAGS)
endif
ifneq (, $(findstring cygwin, $(SYS)))
# Do cygwin things
OPENSSL_LIBS	?= $(shell pkg-config openssl --libs) 
OPENSSL_CFLAGS	?= $(shell pkg-config openssl --cflags)
CFLAGS			= -fPIC $(OPENSSL_CFLAGS) $(LUA_CFLAGS)
endif
# Custome config
ifeq (.config, $(wildcard .config))
include .config
endif



# Compilation directives
WARN_MOST	= -Wall -W -Waggregate-return -Wcast-align -Wmissing-prototypes -Wnested-externs -Wshadow -Wwrite-strings -pedantic
WARN		= -Wall -Wno-unused-value
WARN_MIN	= 
CFLAGS		+= $(WARN_MIN) -DPTHREADS 
CC= gcc -g -Wall $(CFLAGS) -Ideps

OBJS=gpverify.o

.c.o:
	$(CC) -c -o $@ $?

all: $T.so
	@echo $(SYS)

$T.so: $(OBJS)
	$(CC) $(CFLAGS) $(LIB_OPTION) -o $T.so $(OBJS) $(OPENSSL_LIBS) $(LUA_LIBS) $(LDFLAGS)

clean:
	rm -f $T.so $(OBJS)
