if HAVE_GTK
GTK_SUBDIRS = playback seek snapshot
endif

if USE_OPENGL
GL_DIR=gl
else
GL_DIR=
endif

SUBDIRS = app audio compositor decodebin_next dynamic fft gio $(GL_DIR) $(GTK_SUBDIRS) overlay overlaycomposition playrec encoding
DIST_SUBDIRS = app audio compositor decodebin_next dynamic fft gio gl playback overlay overlaycomposition seek snapshot playrec encoding

include $(top_srcdir)/common/parallel-subdirs.mak

