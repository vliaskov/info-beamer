# See Copyright Notice in LICENSE.txt

RELEASE = 1.0pre4

VERSION = $(RELEASE).$(shell git rev-parse --short=6 HEAD)

ifdef DEBUG
CFLAGS ?= -ggdb -DDEBUG
else
CFLAGS ?= -O3 -DNDEBUG
endif

ifdef USE_LUAJIT
LUA_CFLAGS  ?= -I/usr/include/luajit-2.0
LUA_LDFLAGS ?= -lluajit-5.1
LUA_LUAC    ?= luac
CFLAGS      += -DUSE_LUAJIT=1
else
#################################################
# 
# If you have compile/link problems related to lua, try
# setting these variables while running make. For example:
#
# $ LUA_LDFLAGS=-llua make
#
#################################################
LUA_CFLAGS  ?= -I/usr/include/lua5.1
LUA_LDFLAGS ?= -L/usr/lib -llua5.1
LUA_LUAC    ?= luac
endif

CFLAGS  += -DVERSION='"$(VERSION)"'
CFLAGS  += $(LUA_CFLAGS) -g -I/usr/include/freetype2/ -I/usr/include/ffmpeg -std=c99 -Wall

#LDFLAGS += -Wl,--as-needed,-rpath,/opt/vliaskov/devel/info-beamer/ -L. -L/usr/lib/x86_64-linux-gnu  -lfreetype -lfreetype-gl
#LDFLAGS += -lfreetype -L. -L/usr/lib/x86_64-linux-gnu  -Wl,-unresolved-symbols=ignore-in-shared-libs,-rpath,/opt/vliaskov/devel/info-beamer/ -lfreetype-gl
LDFLAGS += -L.  -Wl,-unresolved-symbols=ignore-in-shared-libs,-rpath,/opt/vliaskov/devel/info-beamer/ -lfreetype-gl -lfreetype
LDFLAGS += $(LUA_LDFLAGS) -levent -lglfw -lGL -lGLU -lGLEW -lftgl -lIL -lILU -lavformat -lavcodec -lavutil -lswscale -lz -lm -lXi -lX11 -lXxf86vm -lXrandr -lpthread

#LDFLAGS += -lfreetype -L.  -Wl,-rpath,/opt/vliaskov/devel/info-beamer/ libfreetype-gl.a

#LDFLAGS += -L. -Wl,-rpath,/opt/vliaskov/devel/info-beamer/ -lfreetype-gl -lfreetype
#-Wl,-rpath,/opt/vliaskov/devel/hw-decoding/trunk/freetype-gl
prefix 		?= /usr/local
exec_prefix ?= $(prefix)
bindir 		?= $(exec_prefix)/bin

all: info-beamer

info-beamer: main.o image.o font.o video.o shader.o vnc.o framebuffer.o misc.o struct.o
	$(CC) -o $@ $^ $(LDFLAGS) 

main.o: main.c kernel.h userlib.h module_json.h

info-beamer.1: info-beamer.1.ronn
	ronn $< -r --pipe > $@

bin2c: bin2c.c
	$(CC) $^ -o $@

%.h: %.lua bin2c
	$(LUA_LUAC) -p $<
	./bin2c $* < $< > $@

doc:
	markdown_py -x toc -x tables -x codehilite doc/manual.md > doc/manual.html

install: info-beamer
	install -D -o root -g root -m 755 $< $(DESTDIR)$(bindir)/$<

clean:
	rm -f *.o info-beamer kernel.h userlib.h module_*.h bin2c *.compiled doc/manual.html info-beamer.1

.PHONY: clean doc install
