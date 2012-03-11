#
#  pulse123.lib
#

#DLL      = no
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\pulseaudio;..\json-c
TARGET   = all2
CFLAGS_USR = -DHAVE_CONFIG_H

!include $(TOPDIR)\config\makerules

all2: libpulse$(LBO) $(MDUMMY)

# browser$(CO) glib-mainloop$(CO)

OBJECTS = channelmap$(CO) client-conf$(CO)
OBJECTS = $(OBJECTS) context$(CO) error$(CO) ext-device-manager$(CO) ext-stream-restore$(CO)
OBJECTS = $(OBJECTS) fork-detect$(CO) i18n$(CO) introspect$(CO)
OBJECTS = $(OBJECTS) mainloop-api$(CO) mainloop-signal$(CO) mainloop$(CO) operation$(CO)
OBJECTS = $(OBJECTS) proplist$(CO) rtclock$(CO) sample$(CO) scache$(CO) format$(CO)
OBJECTS = $(OBJECTS) simple$(CO) stream$(CO) subscribe$(CO) thread-mainloop$(CO)
OBJECTS = $(OBJECTS) timeval$(CO) utf8$(CO) util$(CO) volume$(CO) xmalloc$(CO)               


libpulse$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libpulse$(LBO) 2> nul
	-@del $(OBJECTS)    2> nul
browser.c:	..\..\include\config.h xmalloc.h ..\pulsecore\log.h ..\pulsecore\core-util.h ..\pulsecore\avahi-wrap.h ..\pulsecore\refcnt.h ..\pulsecore\macro.h browser.h
xmalloc.h:	cdecl.h gccmacro.h version.h
version.h:	cdecl.h
browser.h:	mainloop-api.h sample.h channelmap.h cdecl.h version.h
mainloop-api.h:	cdecl.h version.h
sample.h:	gccmacro.h cdecl.h version.h
channelmap.h:	sample.h cdecl.h gccmacro.h version.h
channelmap.c:	..\..\include\config.h xmalloc.h i18n.h ..\pulsecore\core-util.h ..\pulsecore\macro.h ..\pulsecore\bitset.h ..\pulsecore\sample-util.h channelmap.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
i18n.h:		cdecl.h version.h
client-conf-x11.c:	..\..\include\config.h xmalloc.h i18n.h ..\pulsecore\x11prop.h ..\pulsecore\log.h ..\pulsecore\core-util.h ..\pulsecore\macro.h client-conf-x11.h
client-conf-x11.h:	client-conf.h
client-conf.h:	..\pulsecore\macro.h ..\pulsecore\native-common.h
client-conf.c:	..\..\include\config.h xmalloc.h i18n.h ..\pulsecore\macro.h ..\pulsecore\core-error.h ..\pulsecore\log.h ..\pulsecore\conf-parser.h ..\pulsecore\core-util.h ..\pulsecore\authkey.h client-conf.h
context.c:	..\..\include\config.h version.h xmalloc.h util.h i18n.h mainloop.h timeval.h fork-detect.h client-conf.h client-conf-x11.h ..\pulsecore\core-error.h ..\pulsecore\native-common.h ..\pulsecore\pdispatch.h ..\pulsecore\pstream.h ..\pulsecore\hashmap.h ..\pulsecore\socket-client.h ..\pulsecore\pstream-util.h ..\pulsecore\core-rtclock.h ..\pulsecore\core-util.h ..\pulsecore\log.h ..\pulsecore\socket.h ..\pulsecore\creds.h ..\pulsecore\macro.h ..\pulsecore\proplist-util.h internal.h context.h
utf8.h:		cdecl.h gccmacro.h version.h
util.h:		cdecl.h version.h
mainloop.h:	mainloop-api.h cdecl.h
timeval.h:	cdecl.h gccmacro.h sample.h version.h
internal.h:	mainloop-api.h context.h stream.h operation.h subscribe.h ext-device-manager.h ext-device-restore.h ext-stream-restore.h ..\pulsecore\socket-client.h ..\pulsecore\pstream.h ..\pulsecore\pdispatch.h ..\pulsecore\llist.h ..\pulsecore\native-common.h ..\pulsecore\strlist.h ..\pulsecore\mcalign.h ..\pulsecore\memblockq.h ..\pulsecore\hashmap.h ..\pulsecore\refcnt.h ..\pulsecore\time-smoother.h ..\pulsecore\dbus-util.h client-conf.h
context.h:	sample.h def.h mainloop-api.h cdecl.h operation.h proplist.h version.h
def.h:		cdecl.h sample.h version.h
operation.h:	cdecl.h def.h version.h
proplist.h:	cdecl.h gccmacro.h version.h
stream.h:	sample.h format.h channelmap.h volume.h def.h cdecl.h operation.h context.h proplist.h
volume.h:	cdecl.h gccmacro.h sample.h channelmap.h version.h
subscribe.h:	def.h context.h cdecl.h version.h
ext-device-manager.h:	cdecl.h context.h version.h
ext-stream-restore.h:	cdecl.h context.h version.h volume.h channelmap.h
error.c:	..\..\include\config.h def.h i18n.h error.h
error.h:	cdecl.h version.h
ext-device-manager.c:	..\..\include\config.h context.h xmalloc.h fork-detect.h operation.h ..\pulsecore\macro.h ..\pulsecore\pstream-util.h internal.h ext-device-manager.h
ext-stream-restore.c:	..\..\include\config.h context.h fork-detect.h operation.h ..\pulsecore\macro.h ..\pulsecore\pstream-util.h internal.h ext-stream-restore.h
fork-detect.c:	..\..\include\config.h ..\pulsecore\atomic.h ..\pulsecore\macro.h fork-detect.h
glib-mainloop.c:	..\..\include\config.h xmalloc.h timeval.h ..\pulsecore\core-util.h ..\pulsecore\log.h ..\pulsecore\llist.h glib-mainloop.h
glib-mainloop.h:	mainloop-api.h cdecl.h version.h
i18n.c:		..\..\include\config.h ..\pulsecore\once.h i18n.h
introspect.c:	..\..\include\config.h context.h xmalloc.h fork-detect.h ..\pulsecore\macro.h ..\pulsecore\core-util.h ..\pulsecore\pstream-util.h internal.h introspect.h
introspect.h:	operation.h context.h cdecl.h gccmacro.h channelmap.h volume.h proplist.h format.h version.h
mainloop-api.c:	..\..\include\config.h xmalloc.h i18n.h ..\pulsecore\macro.h ..\pulsecore\log.h mainloop-api.h
mainloop-signal.c:	..\..\include\config.h xmalloc.h i18n.h ..\pulsecore\core-error.h ..\pulsecore\core-util.h ..\pulsecore\log.h ..\pulsecore\macro.h mainloop-signal.h
mainloop-signal.h:	mainloop-api.h cdecl.h
mainloop.c:	..\..\include\config.h ..\pulsecore\pipe.h i18n.h rtclock.h timeval.h xmalloc.h ..\pulsecore\poll.h ..\pulsecore\core-rtclock.h ..\pulsecore\core-util.h ..\pulsecore\llist.h ..\pulsecore\log.h ..\pulsecore\core-error.h ..\pulsecore\socket.h ..\pulsecore\macro.h mainloop.h internal.h ..\..\utils\debuglog.h
rtclock.h:	cdecl.h sample.h
operation.c:	..\..\include\config.h xmalloc.h ..\pulsecore\macro.h ..\pulsecore\flist.h internal.h operation.h
proplist.c:	..\..\include\config.h xmalloc.h utf8.h ..\pulsecore\hashmap.h ..\pulsecore\strbuf.h ..\pulsecore\core-util.h ..\pulsecore\log.h proplist.h
rtclock.c:	..\..\include\config.h timeval.h ..\pulsecore\core-rtclock.h
sample.c:	..\..\include\config.h timeval.h i18n.h ..\pulsecore\core-util.h ..\pulsecore\macro.h ..\pulsecore\log.h sample.h
scache.c:	..\..\include\config.h utf8.h fork-detect.h ..\pulsecore\pstream-util.h ..\pulsecore\macro.h ..\pulsecore\proplist-util.h internal.h scache.h
scache.h:	context.h stream.h cdecl.h version.h
simple.c:	..\..\include\config.h pulseaudio.h thread-mainloop.h xmalloc.h ..\pulsecore\log.h ..\pulsecore\macro.h simple.h
pulseaudio.h:	mainloop-api.h sample.h format.h def.h context.h stream.h introspect.h subscribe.h scache.h version.h error.h operation.h channelmap.h volume.h xmalloc.h utf8.h thread-mainloop.h mainloop-signal.h util.h timeval.h proplist.h rtclock.h
thread-mainloop.h:	mainloop-api.h cdecl.h version.h
simple.h:	sample.h channelmap.h def.h cdecl.h version.h
stream.c:	..\..\include\config.h def.h timeval.h rtclock.h xmalloc.h fork-detect.h ..\pulsecore\pstream-util.h ..\pulsecore\log.h ..\pulsecore\hashmap.h ..\pulsecore\macro.h ..\pulsecore\core-rtclock.h ..\pulsecore\core-util.h internal.h stream.h
subscribe.c:	..\..\include\config.h ..\pulsecore\macro.h ..\pulsecore\pstream-util.h internal.h subscribe.h
thread-mainloop.c:	..\..\include\config.h xmalloc.h mainloop.h i18n.h ..\pulsecore\log.h ..\pulsecore\thread.h ..\pulsecore\mutex.h ..\pulsecore\macro.h ..\pulsecore\poll.h thread-mainloop.h ..\..\utils\debuglog.h
timeval.c:	..\..\include\config.h ..\pulsecore\macro.h ..\pulsecore\core-util.h ..\pulsecore\log.h timeval.h
utf8.c:		..\..\include\config.h xmalloc.h ..\pulsecore\macro.h ..\pulsecore\log.h utf8.h
util.c:		..\..\include\config.h xmalloc.h timeval.h ..\pulsecore\socket.h ..\pulsecore\core-util.h ..\pulsecore\macro.h ..\pulsecore\log.h ..\pulsecore\usergroup.h
volume.c:	..\..\include\config.h i18n.h ..\pulsecore\core-util.h ..\pulsecore\macro.h ..\pulsecore\log.h ..\pulsecore\sample-util.h volume.h
xmalloc.c:	..\..\include\config.h gccmacro.h ..\pulsecore\core-util.h ..\pulsecore\macro.h ..\pulsecore\log.h xmalloc.h
..\pulse\xmalloc.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
..\pulse\version.h:	..\pulse\cdecl.h
..\pulsecore\log.h:	..\pulsecore\macro.h gccmacro.h
..\pulsecore\core-util.h:	gccmacro.h ..\pulsecore\macro.h ..\pulsecore\socket.h
..\pulsecore\avahi-wrap.h:	mainloop-api.h
..\pulse\mainloop-api.h:	..\pulse\cdecl.h ..\pulse\sample.h ..\pulse\version.h
..\pulse\sample.h:	..\pulse\gccmacro.h ..\pulse\cdecl.h ..\pulse\version.h
..\pulsecore\refcnt.h:	..\pulsecore\atomic.h ..\pulsecore\macro.h ..\pulsecore\log.h
..\pulsecore\atomic.h:	..\pulsecore\macro.h ..\..\utils\atomic_ops.h
..\..\include\atomic_ops.h:	..\..\include\config.h ..\..\utils\interlocked.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\pulse\channelmap.h:	..\pulse\sample.h ..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
..\pulse\i18n.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
..\pulsecore\bitset.h:	..\pulsecore\macro.h
..\pulsecore\sample-util.h:	gccmacro.h sample.h volume.h channelmap.h ..\pulsecore\memblock.h ..\pulsecore\memchunk.h
..\pulse\volume.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\sample.h ..\pulse\channelmap.h ..\pulse\version.h
..\pulsecore\memblock.h:	def.h ..\pulsecore\atomic.h
..\pulse\def.h:	..\pulse\cdecl.h ..\pulse\sample.h ..\pulse\version.h
..\pulsecore\llist.h:	..\pulsecore\macro.h
..\pulsecore\memchunk.h:	..\pulsecore\memblock.h
..\pulsecore\native-common.h:	cdecl.h def.h
..\pulsecore\core-error.h:	cdecl.h
..\pulse\utf8.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
..\pulse\util.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
..\pulse\mainloop.h:	..\pulse\mainloop-api.h ..\pulse\cdecl.h
..\pulse\timeval.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\sample.h ..\pulse\version.h
..\pulsecore\pdispatch.h:	mainloop-api.h def.h ..\pulsecore\tagstruct.h ..\pulsecore\packet.h ..\pulsecore\creds.h
..\pulsecore\tagstruct.h:	sample.h format.h channelmap.h volume.h proplist.h ..\pulsecore\macro.h
..\pulse\proplist.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
..\pulsecore\packet.h:	..\pulsecore\refcnt.h
..\pulsecore\pstream.h:	mainloop-api.h def.h ..\pulsecore\packet.h ..\pulsecore\memblock.h ..\pulsecore\iochannel.h ..\pulsecore\memchunk.h ..\pulsecore\creds.h ..\pulsecore\macro.h
..\pulsecore\iochannel.h:	mainloop-api.h ..\pulsecore\creds.h ..\pulsecore\macro.h
..\pulsecore\hashmap.h:	..\pulsecore\idxset.h
..\pulsecore\idxset.h:	..\pulsecore\macro.h
..\pulsecore\socket-client.h:	mainloop-api.h ..\pulsecore\iochannel.h
..\pulsecore\pstream-util.h:	..\pulsecore\pstream.h ..\pulsecore\tagstruct.h ..\pulsecore\creds.h
..\pulsecore\core-rtclock.h:	..\pulsecore\macro.h sample.h
..\pulsecore\socket-util.h:	..\pulsecore\macro.h
..\pulsecore\proplist-util.h:	proplist.h
..\pulse\context.h:	..\pulse\sample.h ..\pulse\def.h ..\pulse\mainloop-api.h ..\pulse\cdecl.h ..\pulse\operation.h ..\pulse\proplist.h ..\pulse\version.h
..\pulse\operation.h:	..\pulse\cdecl.h ..\pulse\def.h ..\pulse\version.h
..\pulse\stream.h:	..\pulse\sample.h ..\pulse\channelmap.h ..\pulse\volume.h ..\pulse\def.h ..\pulse\cdecl.h ..\pulse\operation.h ..\pulse\context.h ..\pulse\proplist.h
..\pulse\subscribe.h:	..\pulse\def.h ..\pulse\context.h ..\pulse\cdecl.h ..\pulse\version.h
..\pulse\ext-device-manager.h:	..\pulse\context.h ..\pulse\version.h
..\pulse\ext-stream-restore.h:	..\pulse\context.h ..\pulse\version.h ..\pulse\volume.h ..\pulse\channelmap.h
..\pulsecore\mcalign.h:	..\pulsecore\memblock.h ..\pulsecore\memchunk.h
..\pulsecore\memblockq.h:	..\pulsecore\memblock.h ..\pulsecore\memchunk.h def.h
..\pulsecore\time-smoother.h:	..\pulsecore\macro.h sample.h
..\pulsecore\dbus-util.h:	gccmacro.h mainloop-api.h proplist.h ..\pulsecore\llist.h
..\pulsecore\once.h:	..\pulsecore\atomic.h
..\pulsecore\mutex.h:	..\pulsecore\macro.h ..\pulsecore\atomic.h
..\pulse\rtclock.h:	..\pulse\cdecl.h ..\pulse\def.h ..\pulse\gccmacro.h
..\pulsecore\flist.h:	def.h gccmacro.h ..\pulsecore\once.h ..\pulsecore\core-util.h
..\pulsecore\strbuf.h:	gccmacro.h ..\pulsecore\macro.h
..\pulse\scache.h:	..\pulse\context.h ..\pulse\stream.h ..\pulse\cdecl.h ..\pulse\version.h
..\pulse\pulseaudio.h:	..\pulse\mainloop-api.h ..\pulse\sample.h ..\pulse\def.h ..\pulse\context.h ..\pulse\stream.h ..\pulse\introspect.h ..\pulse\subscribe.h ..\pulse\scache.h ..\pulse\version.h ..\pulse\error.h ..\pulse\operation.h ..\pulse\channelmap.h ..\pulse\volume.h ..\pulse\xmalloc.h ..\pulse\utf8.h ..\pulse\thread-mainloop.h ..\pulse\mainloop.h ..\pulse\mainloop-signal.h ..\pulse\util.h ..\pulse\timeval.h ..\pulse\proplist.h ..\pulse\rtclock.h
..\pulse\introspect.h:	..\pulse\operation.h ..\pulse\context.h ..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\channelmap.h ..\pulse\volume.h ..\pulse\proplist.h ..\pulse\version.h
..\pulse\error.h:	..\pulse\cdecl.h ..\pulse\version.h
..\pulse\thread-mainloop.h:	..\pulse\mainloop-api.h ..\pulse\cdecl.h ..\pulse\version.h
..\pulse\mainloop-signal.h:	..\pulse\mainloop-api.h ..\pulse\cdecl.h
..\pulsecore\thread.h:	def.h gccmacro.h ..\pulsecore\once.h ..\pulsecore\core-util.h
..\..\utils\atomic_ops.h:	..\..\include\config.h ..\..\utils\interlocked.h
format.h:	cdecl.h gccmacro.h proplist.h sample.h channelmap.h
..\pulsecore\creds.h:	..\pulsecore\socket.h
ext-device-restore.h:	context.h version.h
ext-device-restore.c:	..\..\include\config.h context.h gccmacro.h xmalloc.h fork-detect.h operation.h format.h ..\pulsecore\macro.h ..\pulsecore\pstream-util.h internal.h ext-device-restore.h
format.c:	..\..\include\config.h internal.h xmalloc.h i18n.h ..\pulsecore\core-util.h ..\pulsecore\macro.h format.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
