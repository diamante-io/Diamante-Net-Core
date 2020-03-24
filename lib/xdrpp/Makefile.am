## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = $(cereal_CPPFLAGS) $(autocheck_CPPFLAGS)

bin_PROGRAMS = xdrc/xdrc

xdrc_xdrc_SOURCES = xdrc/xdrc.cc xdrc/gen_hh.cc xdrc/gen_server.cc	\
	xdrc/scan.ll xdrc/parse.yy xdrc/union.h xdrc/xdrc_internal.h
if NEED_GETOPT_LONG
xdrc_xdrc_SOURCES += compat/getopt_long.c compat/getopt.h
endif # ! NEED_GETOPT_LONG
xdrc_xdrc_LDADD =

AM_YFLAGS = -d
# Next line is needed on very parallel builds
xdrc/scan.$(OBJEXT) xdrc/parse.$(OBJEXT): xdrc/parse.hh
# Next line for old automakes that generate parse.h, not parse.hh
.h.hh:
	cp $< $@

lib_LIBRARIES = xdrpp/libxdrpp.a
xdrpp_libxdrpp_a_SOURCES = xdrpp/iniparse.cc xdrpp/marshal.cc	\
	xdrpp/msgsock.cc xdrpp/printer.cc xdrpp/pollset.cc	\
	xdrpp/rpcbind.cc xdrpp/rpc_msg.cc xdrpp/server.cc	\
	xdrpp/socket.cc xdrpp/socket_unix.cc xdrpp/srpc.cc xdrpp/arpc.cc

nodist_pkginclude_HEADERS = xdrpp/build_endian.h

BUILT_SOURCES = xdrc/parse.cc xdrc/parse.hh xdrc/scan.cc	\
	xdrpp/rpc_msg.hh xdrpp/rpcb_prot.hh xdrpp/config.h

# If we use AC_CONFIG_HEADERS([xdrpp/config.h]) in configure.ac, then
# autoconf adds -Ixdrpp, which causes errors for files like endian.h
# (system headers find the xdrpp version instead of the system
# version).  Creating ./config.h and then copying it into ./xdrpp/
# solves the problem.
xdrpp/config.h: $(builddir)/config.h
	cp $(builddir)/config.h xdrpp/config.h
xdrc/xdrc.$(OBJEXT): xdrpp/config.h

pkginclude_HEADERS = xdrpp/types.h xdrpp/cereal.h xdrpp/clear.h		\
	xdrpp/exception.h xdrpp/iniparse.h xdrpp/marshal.h		\
	xdrpp/printer.h xdrpp/rpc_msg.hh xdrpp/message.h		\
	xdrpp/msgsock.h xdrpp/arpc.h xdrpp/pollset.h xdrpp/server.h	\
	xdrpp/socket.h xdrpp/srpc.h xdrpp/rpcbind.h xdrpp/autocheck.h	\
	xdrpp/endian.h xdrpp/build_endian.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xdrpp.pc

LDADD = $(top_builddir)/xdrpp/libxdrpp.a

check_PROGRAMS = tests/test-stacklim tests/test-msgsock		\
	tests/test-marshal tests/test-srpc tests/test-printer	\
	tests/test-listener tests/test-arpc tests/test-compare	\
	tests/test-types tests/test-validate
TESTS = tests/test-stacklim tests/test-msgsock tests/test-printer	\
	tests/test-compare tests/test-types tests/test-validate
if USE_CEREAL
check_PROGRAMS += tests/test-cereal
TESTS += tests/test-cereal
endif
if USE_AUTOCHECK
check_PROGRAMS += tests/test-autocheck
TESTS += tests/test-autocheck
endif
tests_test_arpc_SOURCES = tests/arpc.cc
tests_test_autocheck_SOURCES = tests/autocheck.cc
tests_test_cereal_SOURCES = tests/cereal.cc
tests_test_compare_SOURCES = tests/compare.cc
tests_test_listener_SOURCES = tests/listener.cc
tests_test_marshal_SOURCES = tests/marshal.cc
tests_test_msgsock_SOURCES = tests/msgsock.cc
tests_test_printer_SOURCES = tests/printer.cc
tests_test_srpc_SOURCES = tests/srpc.cc
tests_test_stacklim_SOURCES = tests/stacklim.cc
tests_test_types_SOURCES = tests/types.cc
tests_test_validate_SOURCES = tests/validate.cc
tests/arpc.$(OBJEXT): tests/xdrtest.hh
tests/arpc.$(OBJEXT): tests/xdrtest.hh
tests/autocheck.$(OBJEXT): tests/xdrtest.hh
tests/cereal.$(OBJEXT): tests/xdrtest.hh
tests/compare.$(OBJEXT): tests/xdrtest.hh
tests/listener.$(OBJEXT): tests/xdrtest.hh
tests/marshal.$(OBJEXT): tests/xdrtest.hh
tests/printer.$(OBJEXT): tests/xdrtest.hh
tests/srpc.$(OBJEXT): tests/xdrtest.hh
tests/stacklim.$(OBJEXT): tests/xdrtest.hh
tests/types.$(OBJEXT): tests/xdrtest.hh
tests/validate.$(OBJEXT): tests/xdrtest.hh

SUFFIXES = .x .hh
.x.hh:
	$(XDRC) -hh -o $@ $<
$(top_builddir)/tests/xdrtest.hh: $(XDRC)
$(top_builddir)/xdrpp/rpc_msg.hh: $(XDRC)
$(top_builddir)/xdrpp/rpcb_prot.hh: $(XDRC)

CLEANFILES = *~ */*~ */*/*~ .gitignore~ tests/xdrtest.hh	\
	xdrpp/rpc_msg.hh xdrpp/rpcb_prot.hh
DISTCLEANFILES = xdrpp/config.h getopt.h

$(srcdir)/doc/xdrc.1: $(srcdir)/doc/xdrc.1.md
	-pandoc -s -w man $(srcdir)/doc/xdrc.1.md -o $(srcdir)/doc/xdrc.1

.PHONY: gh-pages
gh-pages:
	cd $(srcdir) && doxygen && ./mk-gh-pages
	@echo '************************************************************'
	@echo 'Now you may wan to run:'
	@echo '  git push -f origin gh-pages'
	@echo '************************************************************'

distclean-local:
	rm -rf autom4te.cache

maintainer-clean-local:
	cd $(srcdir) && rm -rf `sed -ne 's!^/!!p' .gitignore`

man_MANS = doc/xdrc.1
EXTRA_DIST = .gitignore autogen.sh doc/xdrc.1 doc/xdrc.1.md		\
	xdrpp/build_endian.h.in xdrpp/rpc_msg.x xdrpp/rpcb_prot.x	\
	tests/xdrtest.x doc/rfc1833.txt doc/rfc4506.txt			\
	doc/rfc5531.txt doc/rfc5665.txt

ACLOCAL_AMFLAGS = -I m4
