# Hand-written file with variables common to all makefiles

AM_CPPFLAGS = -isystem "$(top_srcdir)" -I"$(top_srcdir)/src" -I"$(top_builddir)/src"
AM_CPPFLAGS += $(libsodium_CFLAGS) $(xdrpp_CFLAGS) $(libmedida_CFLAGS)	\
	$(soci_CFLAGS) $(sqlite3_CFLAGS) $(libasio_CFLAGS)
AM_CPPFLAGS += -isystem "$(top_srcdir)/lib"             \
	-isystem "$(top_srcdir)/lib/autocheck/include"      \
	-isystem "$(top_srcdir)/lib/cereal/include"         \
	-isystem "$(top_srcdir)/lib/util"                   \
	-isystem "$(top_srcdir)/lib/fmt/include"            \
	-isystem "$(top_srcdir)/lib/soci/src/core"          \
	-isystem "$(top_srcdir)/lib/tracy"

if USE_POSTGRES
AM_CPPFLAGS += -DUSE_POSTGRES=1 $(libpq_CFLAGS)
endif # USE_POSTGRES

# USE_TRACY and tracy_CFLAGS here represent the case of enabling
# tracy at configure-time; but even when it is disabled we want
# its includes in the CPPFLAGS above, so its (disabled) headers
# and zone-definition macros are included in our code (and
# compiled to no-ops).
if USE_TRACY
AM_CPPFLAGS += -DUSE_TRACY $(tracy_CFLAGS)
endif # USE_TRACY

if BUILD_TESTS
AM_CPPFLAGS += -DBUILD_TESTS=1
endif # BUILD_TESTS

if USE_EASYLOGGING
AM_CPPFLAGS += -DUSE_EASYLOGGING
endif # USE_EASYLOGGING
