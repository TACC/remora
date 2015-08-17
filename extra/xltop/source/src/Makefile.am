AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-DXLTOP_CONF_DIR=\"${XLTOP_CONF_DIR}\" \
	-DXLTOP_MASTER=\"${XLTOP_MASTER}\" \
	-DXLTOP_PORT=\"${XLTOP_PORT}\" \
	-DXLTOP_CLUS=\"${XLTOP_CLUS}\" \
	-DXLTOP_DOMAIN=\"${XLTOP_DOMAIN}\" \
	-I/usr/include/libev \
	-I${top_srcdir}/src

AM_CFLAGS = -g -Wall -Werror -Wno-strict-aliasing

bin_PROGRAMS = xltop

xltop_SOURCES = xltop.c hash.c n_buf.c screen.c curl_x.c

xltop_LDADD = -lcurl -lev -lncurses
