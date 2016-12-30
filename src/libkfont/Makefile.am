AM_CPPFLAGS = \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-U_GNU_SOURCE -D_FORTIFY_SOURCE=2 \
	-funit-at-a-time

libkfont_la_SOURCES = \
	psf.c \
	../xmalloc.c ../xmalloc.h

kfont_SOURCES = \
	kfont-test.c \
	../version.h

kfont_render_SOURCES = \
	kfont-render.c \
	../version.h

kfont_LDADD = libkfont.la
kfont_render_LDADD = libkfont.la
noinst_PROGRAMS = kfont kfont-render

if BUILD_LIBKFONT
lib_LTLIBRARIES = libkfont.la
libkfont_la_LDFLAGS = -export-symbols-regex ^kfont_.*
else
noinst_LTLIBRARIES = libkfont.la
endif
