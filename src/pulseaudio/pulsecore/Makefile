#
#  pulse123.lib
#

#DLL      = no
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\pulseaudio;$(TOPDIR)\snd123\src
TARGET   = all2
CFLAGS_USR = -DHAVE_CONFIG_H

!include $(TOPDIR)\config\makerules

all2: libpulsecore$(LBO) $(MDUMMY)

# Not supported
# avahi-wrap$(CO) database-gdbm$(CO) database-tdb$(CO) dbus-shared$(CO) dbus-util$(CO)
# lock-autospawn$(CO) mutex-posix$(CO) mutex-win32$(CO) protocol-dbus$(CO) rtkit$(CO)
# semaphore-osx$(CO) semaphore-posix$(CO) semaphore-win32$(CO) shmasyncq$(CO)
# g711$(CO) 

OBJECTS = asyncmsgq$(CO) asyncq$(CO) aupdate$(CO) auth-cookie$(CO) authkey$(CO)
OBJECTS = $(OBJECTS) bitset$(CO) card$(CO) cli-command$(CO) cli-text$(CO) cli$(CO) client$(CO)
OBJECTS = $(OBJECTS) conf-parser$(CO) core-error$(CO) core-rtclock$(CO) core-scache$(CO)
OBJECTS = $(OBJECTS) core-subscribe$(CO) core-util$(CO) core$(CO) cpu-arm$(CO) cpu-x86$(CO)
OBJECTS = $(OBJECTS) database-simple$(CO)
OBJECTS = $(OBJECTS) dllmain$(CO) dynarray$(CO) envelope$(CO) fdsem$(CO)
OBJECTS = $(OBJECTS) flist$(CO) hashmap$(CO) hook-list$(CO) idxset$(CO)
OBJECTS = $(OBJECTS) inet_ntop$(CO) inet_pton$(CO) iochannel$(CO) ioline$(CO) ipacl$(CO)
OBJECTS = $(OBJECTS) log$(CO) ltdl-helper$(CO) mcalign$(CO)
OBJECTS = $(OBJECTS) memblock$(CO) memblockq$(CO) memchunk$(CO) memtrap$(CO) mime-type$(CO)
OBJECTS = $(OBJECTS) modargs$(CO) modinfo$(CO) module$(CO) msgobject$(CO)
OBJECTS = $(OBJECTS) mutex-os2$(CO) namereg$(CO) object$(CO) once$(CO) packet$(CO)
OBJECTS = $(OBJECTS) parseaddr$(CO) pdispatch$(CO) pid$(CO) pipe$(CO) play-memblockq$(CO)
OBJECTS = $(OBJECTS) play-memchunk$(CO) poll$(CO) prioq$(CO) proplist-util$(CO)
OBJECTS2 = protocol-cli$(CO) protocol-esound$(CO)
OBJECTS2 = $(OBJECTS2) protocol-http$(CO) protocol-native$(CO) protocol-simple$(CO)
OBJECTS2 = $(OBJECTS2) pstream-util$(CO) pstream$(CO) queue$(CO) random$(CO)
OBJECTS2 = $(OBJECTS2) ratelimit$(CO) remap$(CO) remap_mmx$(CO) remap_sse$(CO) resampler$(CO)
OBJECTS2 = $(OBJECTS2) rtpoll$(CO) sample-util$(CO) sconv-s16be$(CO) sconv-s16le$(CO)
OBJECTS2 = $(OBJECTS2) sconv$(CO) sconv_sse$(CO) semaphore-os2$(CO)
OBJECTS2 = $(OBJECTS2) shared$(CO) shm$(CO)
OBJECTS2 = $(OBJECTS2) sink-input$(CO) sink$(CO) sioman$(CO) sndfile-util$(CO)
OBJECTS2 = $(OBJECTS2) socket-client$(CO) socket-server$(CO) socket-util$(CO)
OBJECTS2 = $(OBJECTS2) sound-file-stream$(CO) sound-file$(CO) source-output$(CO) source$(CO)
OBJECTS2 = $(OBJECTS2) start-child$(CO) strbuf$(CO) strlist$(CO) svolume_arm$(CO)
OBJECTS2 = $(OBJECTS2) svolume_c$(CO) svolume_mmx$(CO) svolume_sse$(CO) tagstruct$(CO)
OBJECTS2 = $(OBJECTS2) thread-mq$(CO) thread-os2$(CO) time-smoother$(CO)
OBJECTS2 = $(OBJECTS2) tokenizer$(CO) usergroup$(CO)
OBJECTS2 = $(OBJECTS2) ffmpeg/resample2$(CO)

libpulsecore$(LBO): $(OBJECTS) $(OBJECTS2)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)
	$(LB) $@ $(OBJECTS2) $(LBENDL)

clean: $(MDUMMY)
	-@del libpulsecore$(LBO) 2> nul
	-@del $(OBJECTS)    2> nul
	-@del $(OBJECTS2)    2> nul

asyncmsgq.c:	..\..\include\config.h ..\pulse\xmalloc.h atomic.h macro.h log.h thread.h semaphore.h core-util.h flist.h asyncmsgq.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\pulse\xmalloc.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
..\pulse\version.h:	..\pulse\cdecl.h
atomic.h:	macro.h ..\..\include\atomic_ops.h
macro.h:	..\pulse\gccmacro.h
..\..\include\atomic_ops.h:	..\..\include\config.h ..\..\utils\interlocked.h
..\..\utils\interlocked.h:	..\..\include\config.h
log.h:		macro.h ..\pulse\gccmacro.h
thread.h:	..\pulse\def.h once.h core-util.h
..\pulse\def.h:	..\pulse\cdecl.h ..\pulse\sample.h ..\pulse\version.h
..\pulse\sample.h:	..\pulse\gccmacro.h ..\pulse\cdecl.h ..\pulse\version.h
once.h:		mutex.h atomic.h
mutex.h:	macro.h atomic.h
core-util.h:	..\pulse\gccmacro.h
semaphore.h:	macro.h atomic.h
flist.h:	..\pulse\def.h ..\pulse\gccmacro.h once.h core-util.h
asyncmsgq.h:	asyncq.h memchunk.h msgobject.h
asyncq.h:	..\pulse\def.h macro.h
memchunk.h:	memblock.h
memblock.h:	..\pulse\def.h llist.h refcnt.h atomic.h
llist.h:	macro.h
refcnt.h:	atomic.h macro.h log.h
msgobject.h:	..\pulse\xmalloc.h refcnt.h macro.h object.h memchunk.h
object.h:	..\pulse\xmalloc.h refcnt.h macro.h
asyncq.c:	..\..\include\config.h atomic.h log.h thread.h macro.h core-util.h llist.h flist.h ..\pulse\xmalloc.h asyncq.h fdsem.h
fdsem.h:	..\pulse\def.h
auth-cookie.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\util.h refcnt.h macro.h core-util.h shared.h authkey.h auth-cookie.h
..\pulse\util.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
shared.h:	core.h strbuf.h
..\pulse\mainloop-api.h:	..\pulse\cdecl.h ..\pulse\sample.h ..\pulse\version.h
cpu.h:		cpu-x86.h cpu-arm.h
cpu-x86.h:	macro.h
cpu-arm.h:	macro.h
resampler.h:	..\pulse\sample.h ..\pulse\channelmap.h memblock.h memchunk.h
..\pulse\channelmap.h:	..\pulse\sample.h ..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
queue.h:	idxset.h
idxset.h:	macro.h
hook-list.h:	..\pulse\xmalloc.h ..\pulse\gccmacro.h llist.h
sample-util.h:	..\pulse\sample.h ..\pulse\volume.h ..\pulse\channelmap.h memblock.h memchunk.h
..\pulse\volume.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\sample.h ..\pulse\channelmap.h ..\pulse\version.h
native-common.h:	..\pulse\cdecl.h ..\pulse\def.h
strbuf.h:	..\pulse\gccmacro.h
auth-cookie.h:	core.h
authkey.c:	..\..\include\config.h ..\pulse\util.h ..\pulse\xmalloc.h core-error.h core-util.h log.h random.h macro.h authkey.h
core-error.h:	..\pulse\cdecl.h
avahi-wrap.c:	..\..\include\config.h ..\pulse\timeval.h ..\pulse\xmalloc.h log.h macro.h avahi-wrap.h
..\pulse\timeval.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\sample.h ..\pulse\version.h
avahi-wrap.h:	..\pulse\mainloop-api.h
bitset.c:	..\..\include\config.h ..\pulse\xmalloc.h log.h bitset.h
bitset.h:	macro.h
card.c:		..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\util.h log.h sink.h macro.h core-util.h core-subscribe.h namereg.h hashmap.h card.h
sink.h:		..\pulse\def.h ..\pulse\sample.h ..\pulse\channelmap.h ..\pulse\volume.h core.h idxset.h source.h module.h refcnt.h msgobject.h rtpoll.h card.h queue.h thread-mq.h
source.h:	..\pulse\sample.h ..\pulse\channelmap.h ..\pulse\volume.h core.h idxset.h memblock.h memchunk.h module.h asyncmsgq.h msgobject.h rtpoll.h card.h queue.h thread-mq.h
module.h:	..\pulse\proplist.h core.h
..\pulse\proplist.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
rtpoll.h:	..\pulse\sample.h asyncmsgq.h fdsem.h macro.h
card.h:		..\pulse\proplist.h core.h module.h idxset.h
thread-mq.h:	..\pulse\mainloop-api.h asyncmsgq.h rtpoll.h
namereg.h:	core.h macro.h
hashmap.h:	idxset.h
cli-command.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\error.h module.h sink.h source.h client.h sink-input.h source-output.h tokenizer.h strbuf.h namereg.h cli-text.h core-scache.h sample-util.h sound-file.h play-memchunk.h sound-file-stream.h shared.h core-util.h core-error.h modinfo.h dynarray.h cli-command.h
..\pulse\error.h:	..\pulse\cdecl.h ..\pulse\version.h
client.h:	..\pulse\proplist.h core.h module.h
memblockq.h:	memblock.h memchunk.h ..\pulse\def.h
envelope.h:	macro.h memchunk.h ..\pulse\sample.h
source-output.h:	..\pulse\sample.h source.h memblockq.h resampler.h module.h client.h sink-input.h
cli-text.h:	core.h
core-scache.h:	core.h memchunk.h sink.h
sound-file.h:	..\pulse\sample.h ..\pulse\channelmap.h memchunk.h
play-memchunk.h:	sink.h memchunk.h
sound-file-stream.h:	sink.h
modinfo.h:	macro.h
dynarray.h:	idxset.h
cli-command.h:	strbuf.h core.h
cli-text.c:	..\..\include\config.h ..\pulse\volume.h ..\pulse\xmalloc.h ..\pulse\timeval.h module.h client.h sink.h source.h sink-input.h source-output.h strbuf.h sample-util.h core-scache.h macro.h core-util.h hashmap.h cli-text.h
cli.c:		..\..\include\config.h ..\pulse\xmalloc.h ioline.h module.h sink.h source.h client.h sink-input.h source-output.h tokenizer.h strbuf.h namereg.h cli-text.h cli-command.h log.h macro.h cli.h
ioline.h:	iochannel.h core-util.h
iochannel.h:	..\pulse\mainloop-api.h creds.h macro.h
cli.h:		iochannel.h core.h module.h
client.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\util.h core-subscribe.h log.h macro.h core-util.h client.h
conf-parser.c:	..\..\include\config.h ..\pulse\xmalloc.h core-error.h log.h core-util.h macro.h conf-parser.h
core-error.c:	..\..\include\config.h ..\pulse\utf8.h ..\pulse\xmalloc.h core-util.h native-common.h thread.h macro.h log.h core-error.h
..\pulse\utf8.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
core-rtclock.c:	..\..\include\config.h ..\pulse\timeval.h macro.h log.h core-error.h core-rtclock.h
core-rtclock.h:	macro.h ..\pulse\sample.h
core-scache.c:	..\..\include\config.h ..\pulse\mainloop.h ..\pulse\channelmap.h ..\pulse\timeval.h ..\pulse\util.h ..\pulse\volume.h ..\pulse\xmalloc.h ..\pulse\rtclock.h sink-input.h sample-util.h play-memchunk.h core-subscribe.h namereg.h sound-file.h core-rtclock.h core-util.h log.h core-error.h macro.h core-scache.h
..\pulse\mainloop.h:	..\pulse\mainloop-api.h ..\pulse\cdecl.h
..\pulse\rtclock.h:	..\pulse\cdecl.h ..\pulse\def.h ..\pulse\gccmacro.h
core-subscribe.c:	..\..\include\config.h ..\pulse\xmalloc.h queue.h log.h macro.h core-subscribe.h
core.c:		..\..\include\config.h ..\pulse\rtclock.h ..\pulse\timeval.h ..\pulse\xmalloc.h module.h sink.h source.h namereg.h core-rtclock.h core-util.h core-scache.h core-subscribe.h shared.h random.h log.h macro.h hashmap.h core.h
cpu-arm.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h log.h cpu-arm.h
cpu-x86.c:	..\..\include\config.h log.h cpu-x86.h
database-gdbm.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h log.h database.h
database.h:	macro.h
database-simple.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h log.h core-error.h hashmap.h database.h
database-tdb.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h log.h database.h
dbus-shared.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\timeval.h log.h shared.h dbus-shared.h
dbus-shared.h:	core.h dbus-util.h
dbus-util.h:	llist.h ..\pulse\mainloop-api.h ..\pulse\proplist.h
dbus-util.c:	..\..\include\config.h ..\pulse\rtclock.h ..\pulse\timeval.h ..\pulse\utf8.h ..\pulse\xmalloc.h core-rtclock.h core-util.h log.h dbus-util.h
dllmain.c:	..\..\include\config.h
dynarray.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h log.h dynarray.h
envelope.c:	..\..\include\config.h ..\pulse\sample.h ..\pulse\xmalloc.h endianmacros.h memchunk.h macro.h flist.h semaphore.h g711.h envelope.h
fdsem.c:	..\..\include\config.h atomic.h log.h thread.h macro.h core-util.h core-error.h ..\pulse\xmalloc.h pipe.h fdsem.h
flist.c:	..\..\include\config.h ..\pulse\xmalloc.h atomic.h log.h thread.h macro.h core-util.h flist.h
g711.c:		g711.h
hashmap.c:	..\..\include\config.h ..\pulse\xmalloc.h idxset.h log.h flist.h macro.h hashmap.h
hook-list.c:	..\..\include\config.h macro.h log.h hook-list.h
idxset.c:	..\..\include\config.h ..\pulse\xmalloc.h log.h flist.h macro.h idxset.h
inet_ntop.c:	..\..\include\config.h core-util.h winsock.h inet_ntop.h
inet_ntop.h:	winsock.h
inet_pton.c:	..\..\include\config.h core-util.h winsock.h inet_pton.h
inet_pton.h:	winsock.h
iochannel.c:	..\..\include\config.h winsock.h ..\pulse\xmalloc.h core-error.h core-util.h socket-util.h log.h macro.h iochannel.h
socket-util.h:	macro.h
ioline.c:	..\..\include\config.h ..\pulse\xmalloc.h winsock.h core-error.h log.h macro.h refcnt.h ioline.h
..\pulse\i18n.h:	..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\version.h
lock-autospawn.h:	macro.h
ratelimit.h:	..\pulse\sample.h macro.h
ltdl-helper.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\util.h core-util.h macro.h log.h ltdl-helper.h
mcalign.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h mcalign.h
mcalign.h:	memblock.h memchunk.h
memblock.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\def.h shm.h log.h hashmap.h semaphore.h macro.h flist.h core-util.h memtrap.h memblock.h
shm.h:		macro.h
memtrap.h:	macro.h
memblockq.c:	..\..\include\config.h ..\pulse\xmalloc.h log.h mcalign.h macro.h flist.h memblockq.h
memchunk.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h core-util.h memchunk.h
mime-type.h:	..\..\include\config.h macro.h ..\pulse\sample.h ..\pulse\channelmap.h
modargs.c:	..\..\include\config.h ..\pulse\xmalloc.h hashmap.h idxset.h sample-util.h namereg.h sink.h source.h core-util.h macro.h modargs.h
modargs.h:	..\pulse\sample.h ..\pulse\channelmap.h core.h macro.h
msgobject.c:	..\..\include\config.h msgobject.h
mutex-posix.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h log.h core-error.h mutex.h
mutex-win32.c:	..\..\include\config.h ..\pulse\xmalloc.h hashmap.h mutex.h
object.c:	..\..\include\config.h core-util.h object.h
packet.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h packet.h
packet.h:	refcnt.h
parseaddr.h:	macro.h
pdispatch.c:	..\..\include\config.h ..\pulse\rtclock.h ..\pulse\timeval.h ..\pulse\xmalloc.h native-common.h llist.h log.h core-util.h macro.h refcnt.h flist.h core-rtclock.h pdispatch.h
pdispatch.h:	..\pulse\mainloop-api.h ..\pulse\def.h tagstruct.h packet.h creds.h
tagstruct.h:	..\pulse\sample.h ..\pulse\channelmap.h ..\pulse\volume.h ..\pulse\proplist.h ..\pulse\gccmacro.h
pid.c:		..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\util.h core-error.h core-util.h log.h macro.h pid.h
play-memblockq.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\gccmacro.h sink-input.h thread-mq.h sample-util.h play-memblockq.h
play-memblockq.h:	sink-input.h memblockq.h
play-memchunk.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\gccmacro.h sink-input.h thread-mq.h play-memblockq.h play-memchunk.h
prioq.c:	..\..\include\config.h ..\pulse\xmalloc.h flist.h log.h prioq.h
prioq.h:	macro.h idxset.h
proplist-util.c:	..\..\include\config.h ..\pulse\gccmacro.h ..\pulse\proplist.h ..\pulse\utf8.h ..\pulse\xmalloc.h ..\pulse\util.h core-util.h log.h proplist-util.h
proplist-util.h:	..\pulse\proplist.h
protocol-cli.c:	..\..\include\config.h ..\pulse\xmalloc.h cli.h log.h macro.h shared.h protocol-cli.h
protocol-cli.h:	core.h socket-server.h module.h modargs.h
socket-server.h:	..\pulse\mainloop-api.h iochannel.h
protocol-dbus.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h dbus-util.h hashmap.h idxset.h shared.h strbuf.h protocol-dbus.h
protocol-dbus.h:	core.h macro.h
protocol-esound.c:	..\..\include\config.h ..\pulse\rtclock.h ..\pulse\sample.h ..\pulse\timeval.h ..\pulse\utf8.h ..\pulse\xmalloc.h ..\pulse\proplist.h esound.h memblock.h client.h sink-input.h sink.h source-output.h source.h core-scache.h sample-util.h authkey.h namereg.h log.h core-util.h core-error.h ipacl.h macro.h thread-mq.h shared.h endianmacros.h protocol-esound.h
protocol-esound.h:	core.h ipacl.h auth-cookie.h iochannel.h module.h modargs.h
protocol-http.h:	core.h module.h modargs.h iochannel.h strlist.h
pstream.h:	..\pulse\mainloop-api.h ..\pulse\def.h packet.h memblock.h iochannel.h memchunk.h creds.h macro.h
pstream-util.h:	pstream.h tagstruct.h creds.h
protocol-native.h:	core.h ipacl.h auth-cookie.h iochannel.h module.h modargs.h strlist.h hook-list.h pstream.h tagstruct.h
protocol-simple.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\timeval.h sink-input.h source-output.h client.h sample-util.h namereg.h log.h core-error.h atomic.h thread-mq.h core-util.h shared.h protocol-simple.h
protocol-simple.h:	socket-server.h module.h core.h modargs.h
pstream-util.c:	..\..\include\config.h native-common.h macro.h pstream-util.h
pstream.c:	..\..\include\config.h ..\pulse\xmalloc.h winsock.h queue.h log.h core-scache.h creds.h refcnt.h flist.h macro.h pstream.h
random.c:	..\..\include\config.h core-util.h log.h macro.h random.h
ratelimit.c:	..\..\include\config.h ..\pulse\rtclock.h log.h mutex.h ratelimit.h
remap.c:	..\..\include\config.h ..\pulse\sample.h log.h macro.h remap.h
remap.h:	..\pulse\sample.h
remap_mmx.c:	..\..\include\config.h ..\pulse\sample.h log.h macro.h cpu-x86.h remap.h
remap_sse.c:	..\..\include\config.h ..\pulse\sample.h log.h macro.h cpu-x86.h remap.h
resampler.c:	..\..\include\config.h ..\speex\speex_resampler.h ..\pulse\xmalloc.h sconv.h log.h macro.h strbuf.h ffmpeg/avcodec.h remap.h
..\speex\speex_resampler.h:	..\speex\speex_types.h
sconv.h:	..\pulse\sample.h
ffmpeg/avcodec.h:	..\..\include\config.h
rtkit.c:	rtkit.h
rtpoll.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\timeval.h poll.h core-error.h core-rtclock.h macro.h llist.h flist.h core-util.h winsock.h ratelimit.h ..\pulse\rtclock.h rtpoll.h
sample-util.c:	..\..\include\config.h ..\pulse\timeval.h log.h core-error.h macro.h g711.h core-util.h sample-util.h endianmacros.h
sconv-s16be.c:	..\..\include\config.h endianmacros.h sconv-s16le.h sconv-s16le.c
sconv-s16le.c:	..\..\include\config.h sconv.h macro.h log.h endianmacros.h sconv-s16le.h
sconv.c:	..\..\include\config.h g711.h macro.h log.h endianmacros.h sconv-s16le.h sconv-s16be.h sconv.h
sconv_sse.c:	..\..\include\config.h g711.h macro.h log.h endianmacros.h cpu-x86.h sconv.h
semaphore-osx.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h semaphore.h
semaphore-posix.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h semaphore.h
semaphore-win32.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h semaphore.h
shared.c:	..\..\include\config.h ..\pulse\xmalloc.h log.h macro.h hashmap.h shared.h
shm.c:		..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\gccmacro.h core-error.h log.h random.h core-util.h atomic.h shm.h
shmasyncq.c:	..\..\include\config.h atomic.h log.h thread.h macro.h core-util.h ..\pulse\xmalloc.h shmasyncq.h
sink-input.c:	..\..\include\config.h ..\pulse\utf8.h ..\pulse\xmalloc.h ..\pulse\util.h sample-util.h core-subscribe.h log.h play-memblockq.h namereg.h core-util.h hashmap.h source-output.h ..\pulse\timeval.h sink-input.h
sink.c:		..\..\include\config.h ..\pulse\introspect.h ..\pulse\utf8.h ..\pulse\xmalloc.h ..\pulse\timeval.h ..\pulse\util.h ..\pulse\i18n.h ..\pulse\rtclock.h sink-input.h namereg.h core-util.h sample-util.h core-subscribe.h log.h macro.h play-memblockq.h flist.h hashmap.h source-output.h sink.h
..\pulse\introspect.h:	..\pulse\operation.h ..\pulse\context.h ..\pulse\cdecl.h ..\pulse\gccmacro.h ..\pulse\channelmap.h ..\pulse\volume.h ..\pulse\proplist.h ..\pulse\version.h
..\pulse\operation.h:	..\pulse\cdecl.h ..\pulse\def.h ..\pulse\version.h
..\pulse\context.h:	..\pulse\sample.h ..\pulse\def.h ..\pulse\mainloop-api.h ..\pulse\cdecl.h ..\pulse\operation.h ..\pulse\proplist.h ..\pulse\version.h
sioman.c:	..\..\include\config.h macro.h atomic.h log.h sioman.h
sndfile-util.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\utf8.h macro.h core-util.h log.h sndfile-util.h
sndfile-util.h:	..\..\snd123\src\sndfile.h ..\pulse\sample.h ..\pulse\channelmap.h ..\pulse\proplist.h
socket-client.c:	..\..\include\config.h ..\pulse\rtclock.h ..\pulse\timeval.h ..\pulse\xmalloc.h winsock.h core-error.h socket-util.h core-rtclock.h core-util.h log.h parseaddr.h macro.h refcnt.h socket-client.h
socket-client.h:	..\pulse\mainloop-api.h iochannel.h
socket-server.c:	..\..\include\config.h inet_ntop.h inet_pton.h winsock.h ..\pulse\xmalloc.h ..\pulse\util.h socket-util.h core-util.h log.h macro.h core-error.h refcnt.h socket-server.h
socket-util.c:	..\..\include\config.h inet_ntop.h winsock.h ..\pulse\xmalloc.h core-error.h core-util.h log.h macro.h socket-util.h
sound-file-stream.c:	..\..\include\config.h ..\..\snd123\src\sndfile.h ..\pulse\xmalloc.h ..\pulse\util.h core-error.h sink-input.h log.h thread-mq.h core-util.h sample-util.h sndfile-util.h sound-file-stream.h
sound-file.c:	..\..\include\config.h ..\..\snd123\src\sndfile.h ..\pulse\sample.h log.h macro.h core-error.h core-util.h core-scache.h sndfile-util.h sound-file.h
source-output.c:	..\..\include\config.h ..\pulse\utf8.h ..\pulse\xmalloc.h ..\pulse\util.h sample-util.h core-subscribe.h log.h namereg.h core-util.h source-output.h
source.c:	..\..\include\config.h ..\pulse\utf8.h ..\pulse\xmalloc.h ..\pulse\timeval.h ..\pulse\util.h core-util.h source-output.h namereg.h core-subscribe.h log.h sample-util.h hashmap.h source.h
start-child.c:	..\..\include\config.h core-util.h core-error.h log.h start-child.h
strbuf.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h log.h strbuf.h
strlist.c:	..\..\include\config.h ..\pulse\xmalloc.h strbuf.h macro.h core-util.h log.h strlist.h
svolume_arm.c:	..\..\include\config.h ..\pulse\timeval.h random.h macro.h g711.h core-util.h cpu-arm.h sample-util.h endianmacros.h
svolume_c.c:	..\..\include\config.h macro.h g711.h core-util.h sample-util.h endianmacros.h
svolume_mmx.c:	..\..\include\config.h ..\pulse\timeval.h ..\pulse\rtclock.h random.h macro.h g711.h core-util.h cpu-x86.h sample-util.h endianmacros.h
svolume_sse.c:	..\..\include\config.h ..\pulse\timeval.h ..\pulse\rtclock.h random.h macro.h g711.h core-util.h cpu-x86.h sample-util.h endianmacros.h
tagstruct.c:	..\..\include\config.h ..\pulse\xmalloc.h winsock.h macro.h log.h tagstruct.h
thread-mq.c:	..\..\include\config.h ..\pulse\xmalloc.h atomic.h once.h log.h thread.h semaphore.h macro.h core-util.h flist.h thread-mq.h
thread-posix.c:	..\..\include\config.h ..\pulse\xmalloc.h mutex.h once.h atomic.h macro.h thread.h
thread-win32.c:	..\..\include\config.h ..\pulse\xmalloc.h log.h once.h thread.h
time-smoother.c:	..\..\include\config.h ..\pulse\sample.h ..\pulse\xmalloc.h macro.h log.h time-smoother.h
time-smoother.h:	macro.h ..\pulse\sample.h
tokenizer.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\gccmacro.h dynarray.h log.h tokenizer.h
usergroup.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h usergroup.h
x11prop.c:	..\..\include\config.h x11prop.h macro.h
x11wrap.c:	..\..\include\config.h ..\pulse\xmalloc.h llist.h log.h shared.h core-util.h macro.h x11wrap.h
x11wrap.h:	core.h
aupdate.c:	..\..\include\config.h ..\pulse\xmalloc.h semaphore.h macro.h log.h mutex.h aupdate.h
core-util.c:	..\..\include\config.h rtkit.h ..\pulse\xmalloc.h ..\pulse\util.h ..\pulse\utf8.h core-error.h winsock.h log.h macro.h thread.h strbuf.h usergroup.h strlist.h cpu-x86.h core-util.h
ipacl.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h llist.h log.h macro.h winsock.h inet_pton.h ipacl.h
lock-autospawn.c:	..\..\include\config.h ..\pulse\i18n.h ..\pulse\xmalloc.h poll.h mutex.h thread.h core-util.h log.h lock-autospawn.h
log.c:		..\..\include\config.h ..\pulse\rtclock.h ..\pulse\utf8.h ..\pulse\xmalloc.h ..\pulse\util.h ..\pulse\timeval.h macro.h core-util.h core-rtclock.h once.h ratelimit.h log.h
memtrap.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h aupdate.h atomic.h once.h mutex.h log.h memtrap.h
mime-type.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h log.h mime-type.h
modinfo.c:	..\..\include\config.h ..\pulse\xmalloc.h core-util.h log.h macro.h ltdl-helper.h modinfo.h
module.c:	..\..\include\config.h ..\pulse\timeval.h ..\pulse\xmalloc.h ..\pulse\proplist.h core-subscribe.h log.h core-util.h macro.h ltdl-helper.h modinfo.h module.h
mutex-os2.c:	..\..\include\config.h ..\pulse\xmalloc.h hashmap.h mutex.h
namereg.c:	..\..\include\config.h ..\pulse\xmalloc.h source.h sink.h core-subscribe.h core-util.h macro.h hashmap.h namereg.h
once.c:		..\..\include\config.h macro.h mutex.h log.h once.h
parseaddr.c:	..\..\include\config.h ..\pulse\xmalloc.h ..\pulse\util.h core-util.h macro.h log.h inet_pton.h parseaddr.h
pipe.c:		..\..\include\config.h winsock.h core-util.h pipe.h
poll.c:		..\..\include\config.h winsock.h core-util.h ..\pulse\util.h poll.h
protocol-http.c:	..\..\include\config.h ..\pulse\util.h ..\pulse\xmalloc.h ..\pulse\timeval.h ioline.h thread-mq.h macro.h log.h namereg.h cli-text.h shared.h core-error.h mime-type.h memblockq.h client.h source-output.h protocol-http.h
protocol-native.c:	..\..\include\config.h ..\pulse\rtclock.h ..\pulse\timeval.h ..\pulse\version.h ..\pulse\utf8.h ..\pulse\util.h ..\pulse\xmalloc.h native-common.h packet.h client.h source-output.h sink-input.h pstream.h tagstruct.h pdispatch.h pstream-util.h authkey.h namereg.h core-scache.h core-subscribe.h log.h strlist.h shared.h sample-util.h llist.h creds.h core-util.h ipacl.h thread-mq.h hashmap.h protocol-native.h
queue.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h flist.h log.h queue.h
core-subscribe.h:	core.h native-common.h
semaphore-os2.c:	..\..\include\config.h ..\pulse\xmalloc.h macro.h log.h semaphore.h
shmasyncq.h:	..\pulse\def.h macro.h fdsem.h
core.h:		..\pulse\mainloop-api.h ..\pulse\sample.h cpu.h memblock.h resampler.h queue.h llist.h hook-list.h sample-util.h msgobject.h
sink-input.h:	..\pulse\sample.h hook-list.h memblockq.h resampler.h module.h client.h sink.h core.h envelope.h
..\..\snd123\src\sndfile.h:	..\..\include\config.h
thread-os2.c:	..\..\include\config.h ..\pulse\xmalloc.h log.h once.h dynarray.h thread.h
