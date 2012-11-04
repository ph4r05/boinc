# The default target of this Makefile is...
all:

# Define V=1 to have a more verbose compile.
#
# Define NO_OPENSSL environment variable if you do not have OpenSSL.
# This also implies MOZILLA_SHA1.
#
# Define NO_CURL if you do not have curl installed.  git-http-pull and
# git-http-push are not built, and you cannot use http:// and https://
# transports.
#
# Define CURLDIR=/foo/bar if your curl header and library files are in
# /foo/bar/include and /foo/bar/lib directories.
#
# Define NO_EXPAT if you do not have expat installed.  git-http-push is
# not built, and you cannot push using http:// and https:// transports.
#
# Define NO_D_INO_IN_DIRENT if you don't have d_ino in your struct dirent.
#
# Define NO_D_TYPE_IN_DIRENT if your platform defines DT_UNKNOWN but lacks
# d_type in struct dirent (latest Cygwin -- will be fixed soonish).
#
# Define NO_C99_FORMAT if your formatted IO functions (printf/scanf et.al.)
# do not support the 'size specifiers' introduced by C99, namely ll, hh,
# j, z, t. (representing long long int, char, intmax_t, size_t, ptrdiff_t).
# some C compilers supported these specifiers prior to C99 as an extension.
#
# Define NO_STRCASESTR if you don't have strcasestr.
#
# Define NO_STRTOUMAX if you don't have strtoumax in the C library.
# If your compiler also does not support long long or does not have
# strtoull, define NO_STRTOULL.
#
# Define NO_SETENV if you don't have setenv in the C library.
#
# Define NO_SYMLINK_HEAD if you never want .git/HEAD to be a symbolic link.
# Enable it on Windows.  By default, symrefs are still used.
#
# Define NO_SVN_TESTS if you want to skip time-consuming SVN interoperability
# tests.  These tests take up a significant amount of the total test time
# but are not needed unless you plan to talk to SVN repos.
#
# Define NO_FINK if you are building on Darwin/Mac OS X, have Fink
# installed in /sw, but don't want GIT to link against any libraries
# installed there.  If defined you may specify your own (or Fink's)
# include directories and library directories by defining CFLAGS
# and LDFLAGS appropriately.
#
# Define NO_DARWIN_PORTS if you are building on Darwin/Mac OS X,
# have DarwinPorts installed in /opt/local, but don't want GIT to
# link against any libraries installed there.  If defined you may
# specify your own (or DarwinPort's) include directories and
# library directories by defining CFLAGS and LDFLAGS appropriately.
#
# Define PPC_SHA1 environment variable when running make to make use of
# a bundled SHA1 routine optimized for PowerPC.
#
# Define ARM_SHA1 environment variable when running make to make use of
# a bundled SHA1 routine optimized for ARM.
#
# Define MOZILLA_SHA1 environment variable when running make to make use of
# a bundled SHA1 routine coming from Mozilla. It is GPL'd and should be fast
# on non-x86 architectures (e.g. PowerPC), while the OpenSSL version (default
# choice) has very fast version optimized for i586.
#
# Define NEEDS_SSL_WITH_CRYPTO if you need -lcrypto with -lssl (Darwin).
#
# Define NEEDS_LIBICONV if linking with libc is not enough (Darwin).
#
# Define NEEDS_SOCKET if linking with libc is not enough (SunOS,
# Patrick Mauritz).
#
# Define NO_MMAP if you want to avoid mmap.
#
# Define NO_PREAD if you have a problem with pread() system call (e.g.
# cygwin.dll before v1.5.22).
#
# Define NO_FAST_WORKING_DIRECTORY if accessing objects in pack files is
# generally faster on your platform than accessing the working directory.
#
# Define NO_TRUSTABLE_FILEMODE if your filesystem may claim to support
# the executable mode bit, but doesn't really do so.
#
# Define NO_IPV6 if you lack IPv6 support and getaddrinfo().
#
# Define NO_SOCKADDR_STORAGE if your platform does not have struct
# sockaddr_storage.
#
# Define NO_ICONV if your libc does not properly support iconv.
#
# Define OLD_ICONV if your library has an old iconv(), where the second
# (input buffer pointer) parameter is declared with type (const char **).
#
# Define NO_R_TO_GCC_LINKER if your gcc does not like "-R/path/lib"
# that tells runtime paths to dynamic libraries;
# "-Wl,-rpath=/path/lib" is used instead.
#
# Define USE_NSEC below if you want git to care about sub-second file mtimes
# and ctimes. Note that you need recent glibc (at least 2.2.4) for this, and
# it will BREAK YOUR LOCAL DIFFS! show-diff and anything using it will likely
# randomly break unless your underlying filesystem supports those sub-second
# times (my ext3 doesn't).
#
# Define USE_STDEV below if you want git to care about the underlying device
# change being considered an inode change from the update-cache perspective.
#

HOST := $(shell ./config.guess)
TARGET := $(shell ./config.guess)

SVN_REV=$(shell if [ `which svnversion`x != x ]; then svnversion; else echo local-build; fi)
# CFLAGS and LDFLAGS are for the users to override from the command line.
CFLAGS += -g -Wall 
LDFLAGS += 
ALL_CFLAGS = $(CFLAGS) -DSVNREV=\"Revision:$(SVN_REV)\"
ALL_LDFLAGS = $(LDFLAGS)
EXTRA_PROGRAMS =

HOST_CC = gcc
ifeq ($(HOST),$(TARGET))
	CC = $(HOST_CC)
	AR = ar
	STRIP = strip
else
	CC = $(TARGET)-gcc
	AR = $(TARGET)-ar
	STRIP = $(TARGET)-strip
endif
RM = rm -f

#setting DCAPI to "yes" will set BOINC to "yes" also
BOINC=yes
DCAPI=yes

ifndef BOINCDIR
ifeq ($(findstring mingw,$(TARGET)),mingw)
       BOINCDIR=../../boinc-mingw/
else
       BOINCDIR=/usr/include/BOINC
endif
endif

ifeq ($(findstring mingw,$(TARGET)),mingw)
BOINC_CFLAGS=-I$(BOINCDIR)/api -I$(BOINCDIR)/lib -I./tools/boinc-mingw32/include/
BOINC_LIBS=-L./tools/boinc-mingw32/ -lboinc -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic -lm
else
BOINC_CFLAGS=-I$(BOINCDIR)
BOINC_LIBS=-lboinc_api -lboinc -lcrypto -lstdc++ -lpthread -lm
endif

ifeq ($(DCAPI),yes)
BOINC=yes
LAUNCHER_CFLAGS+= -DWANT_DCAPI
endif

ifeq ($(BOINC),yes)
LAUNCHER_CFLAGS+=-DUSE_GLIBC_ERRNO
LAUNCHER_LDFLAGS+=-Lbox/ -lbox
EXTRA_PROGRAMS+=gw_launcher$X
ifeq ($(findstring mingw,$(TARGET)),mingw)
OPENSSLDIR=$(OPENSSL_DIR)
ALL_LDFLAGS+=$(BOINC_LIBS) -lwinmm -lth32
ALL_CFLAGS+=$(BOINC_CFLAGS) -DBOINC 
LAUNCHER_CFLAGS+=-DWIN32 -D_WIN32 -D_MT -DNDEBUG -D_WINDOWS -DCLIENT -DNODB -D_CONSOLE -fexceptions
else
ALL_CFLAGS+=$(BOINC_CFLAGS) -DBOINC -DUSE_GLIBC_ERRNO
ALL_LDFLAGS+=$(BOINC_LIBS)
endif
endif

### --- END CONFIGURATION SECTION ---

# Those must not be GNU-specific; they are shared with perl/ which may
# be built by a different compiler. (Note that this is an artifact now
# but it still might be nice to keep that distinction.)
BASIC_CFLAGS =
BASIC_LDFLAGS =

PROGRAMS = gitbox$X $(EXTRA_PROGRAMS)

# what 'all' will build and 'install' will install
ALL_PROGRAMS = $(PROGRAMS)

GIT_OBJS = standalone-box.o ctype.o quote.o usage.o \
	 run-command.o exec_cmd.o spawn-pipe.o

BOX_FILE = box/libbox.a

BOX_H = \
	applets.h autoconf.h busybox.h dump.h \
	libbb.h platform.h shadow_.h \
	usage.h xatonum.h xregex.h

BOX_OBJS = \
	archival/bbunzip.o \
	archival/gzip.o \
	archival/libunarchive/check_header_gzip.o \
	archival/libunarchive/data_align.o \
	archival/libunarchive/data_extract_all.o \
	archival/libunarchive/data_extract_to_stdout.o \
	archival/libunarchive/data_skip.o \
	archival/libunarchive/decompress_bunzip2.o \
	archival/libunarchive/decompress_unlzma.o \
	archival/libunarchive/decompress_unzip.o \
	archival/libunarchive/filter_accept_all.o \
	archival/libunarchive/filter_accept_reject_list.o \
	archival/libunarchive/find_list_entry.o \
	archival/libunarchive/get_header_tar.o \
	archival/libunarchive/header_list.o \
	archival/libunarchive/header_verbose_list.o \
	archival/libunarchive/header_skip.o \
	archival/libunarchive/init_handle.o \
	archival/libunarchive/seek_by_jump.o \
	archival/libunarchive/seek_by_read.o \
	archival/tar.o \
	archival/unzip.o \
	coreutils/basename.o \
	coreutils/cat.o \
	coreutils/chmod.o \
	coreutils/cp.o \
	coreutils/cut.o \
	coreutils/date.o \
	coreutils/dirname.o \
	coreutils/dos2unix.o \
	coreutils/echo.o \
	coreutils/env.o \
	coreutils/expr.o \
	coreutils/false.o \
	coreutils/head.o \
	coreutils/libcoreutils/cp_mv_stat.o \
	coreutils/ls.o \
	coreutils/md5_sha1_sum.o \
	coreutils/mkdir.o \
	coreutils/mv.o \
	coreutils/od.o \
	coreutils/printenv.o \
	coreutils/printf.o \
	coreutils/pwd.o \
	coreutils/realpath.o \
	coreutils/rmdir.o \
	coreutils/rm.o \
	coreutils/seq.o \
	coreutils/sleep.o \
	coreutils/sort.o \
	coreutils/sum.o \
	coreutils/tac.o \
	coreutils/tail.o \
	coreutils/test.o \
	coreutils/touch.o \
	coreutils/tr.o \
	coreutils/true.o \
	coreutils/uniq.o \
	coreutils/wc.o \
	coreutils/yes.o \
	debianutils/which.o  \
	editors/awk.o \
	editors/cmp.o \
	editors/diff.o \
	editors/patch.o \
	editors/sed.o \
	findutils/find.o \
	findutils/grep.o \
	libbb/appletlib.o \
	libbb/ask_confirmation.o \
	libbb/bb_basename.o \
	libbb/bb_do_delay.o \
	libbb/bb_qsort.o \
	libbb/bb_strtonum.o \
	libbb/chomp.o \
	libbb/compare_string_array.o \
	libbb/concat_path_file.o \
	libbb/concat_subpath_file.o \
	libbb/copyfd.o \
	libbb/copy_file.o \
	libbb/crc32.o \
	libbb/default_error_retval.o \
	libbb/dump.o \
	libbb/error_msg_and_die.o \
	libbb/error_msg.o \
	libbb/execable.o \
	libbb/fclose_nonstdin.o \
	libbb/fflush_stdout_and_exit.o \
	libbb/fgets_str.o \
	libbb/full_write.o \
	libbb/get_last_path_component.o \
	libbb/get_line_from_file.o \
	libbb/getopt32.o \
	libbb/herror_msg_and_die.o \
	libbb/herror_msg.o \
	libbb/human_readable.o \
	libbb/info_msg.o \
	libbb/inode_hash.o \
	libbb/isdirectory.o \
	libbb/last_char_is.o \
	libbb/llist.o \
	libbb/make_directory.o \
	libbb/mode_string.o \
	libbb/md5.o \
	libbb/messages.o \
	libbb/parse_mode.o \
	libbb/perror_msg_and_die.o \
	libbb/perror_msg.o \
	libbb/perror_nomsg_and_die.o \
	libbb/perror_nomsg.o \
	libbb/process_escape_sequence.o \
	libbb/ptr_to_globals.o \
	libbb/read.o \
	libbb/recursive_action.o \
	libbb/remove_file.o \
	libbb/safe_strncpy.o \
	libbb/safe_write.o \
	libbb/simplify_path.o \
	libbb/skip_whitespace.o \
	libbb/trim.o \
	libbb/u_signal_names.o \
	libbb/verror_msg.o \
	libbb/warn_ignoring_args.o \
	libbb/wfopen_input.o \
	libbb/wfopen.o \
	libbb/xatonum.o \
	libbb/xfuncs.o \
	libbb/xgetcwd.o \
	libbb/xreadlink.o \
	libbb/xregcomp.o \
	shell/ash.o \
	shell/ash_ptr_hack.o

LAUNCHER_OBJS = \
	ctype.o quote.o usage.o \
	run-command.o exec_cmd.o spawn-pipe.o \
	launcher/gw_common.o \
	launcher/task.o \
	launcher/gw_launcher.o

LAUNCHER_SOURCE = \
	ctype.c quote.c usage.c \
	run-command.c exec_cmd.c spawn-pipe.c \
	launcher/gw_common.C \
	launcher/task.C \
	launcher/gw_launcher.C

ifeq ($(BOINC),yes)
BOX_OBJS += boinc/boinc.o
endif

GIT_VERSION = 1.5.3.GIT

BOX_CFLAGS = -Ibox/include -Ibox/libbb \
	-I. -DBB_VER=BUSYBOX_VERSION \
	-DBB_BT="\"GitBox v$(GIT_VERSION), \" AUTOCONF_TIMESTAMP "

BOX_H := $(patsubst %.h,box/include/%.h,$(BOX_H))
BOX_OBJS := $(patsubst %.o,box/%.o,$(BOX_OBJS))

EXTLIBS = -lm

PACKAGE_FILES = \
	gitbox$X \
	gw_launcher$X \
	README \
	AUTHORS \
	ChangeLog \
	LICENSE

#
# Platform specific tweaks
#

# We choose to avoid "if .. else if .. else .. endif endif"
# because maintaining the nesting to match is a pain.  If
# we had "elif" things would have been much nicer...

ifeq ($(findstring darwin,$(TARGET)),darwin)
	NEEDS_SSL_WITH_CRYPTO = YesPlease
	NEEDS_LIBICONV = YesPlease
	OLD_ICONV = UnfortunatelyYes
	BASIC_CFLAGS += -fnested-functions

	ifndef NO_FINK
		ifeq ($(shell test -d /sw/lib && echo y),y)
			BASIC_CFLAGS += -I/sw/include
			BASIC_LDFLAGS += -L/sw/lib
		endif
	endif
	ifndef NO_DARWIN_PORTS
		ifeq ($(shell test -d /opt/local/lib && echo y),y)
			BASIC_CFLAGS += -I/opt/local/include
			BASIC_LDFLAGS += -L/opt/local/lib
		endif
	endif
endif
ifeq ($(findstring solaris,$(TARGET)),solaris)
	NEEDS_SOCKET = YesPlease
	NEEDS_NSL = YesPlease
	NO_STRCASESTR = YesPlease
	NO_HSTRERROR = YesPlease
	NO_UNSETENV = YesPlease
	NO_SETENV = YesPlease
	NO_C99_FORMAT = YesPlease
	NO_STRTOUMAX = YesPlease
	ifeq ($(shell -c 'uname -s 2>/dev/null || echo not'),5.8)
		NEEDS_LIBICONV = YesPlease
	endif
	BASIC_CFLAGS += -D__EXTENSIONS__
endif
ifeq ($(findstring cygwin,$(TARGET)),cygwin)
	NO_D_TYPE_IN_DIRENT = YesPlease
	NO_D_INO_IN_DIRENT = YesPlease
	NO_STRCASESTR = YesPlease
	NO_SYMLINK_HEAD = YesPlease
	NEEDS_LIBICONV = YesPlease
	NO_FAST_WORKING_DIRECTORY = UnfortunatelyYes
	NO_TRUSTABLE_FILEMODE = UnfortunatelyYes
	# There are conflicting reports about this.
	# On some boxes NO_MMAP is needed, and not so elsewhere.
	# Try commenting this out if you suspect MMAP is more efficient
	NO_MMAP = YesPlease
	NO_IPV6 = YesPlease
	X = .exe
endif
ifeq ($(findstring freebsd,$(TARGET)),freebsd)
	NEEDS_LIBICONV = YesPlease
	BASIC_CFLAGS += -I/usr/local/include
	BASIC_LDFLAGS += -L/usr/local/lib
endif
ifeq ($(findstring openbsd,$(TARGET)),openbsd)
	NO_STRCASESTR = YesPlease
	NEEDS_LIBICONV = YesPlease
	BASIC_CFLAGS += -I/usr/local/include
	BASIC_LDFLAGS += -L/usr/local/lib
endif
ifeq ($(findstring netbsd,$(TARGET)),netbsd)
	ifeq ($(shell expr "$(uname_R)" : '[01]\.'),2)
		NEEDS_LIBICONV = YesPlease
	endif
	BASIC_CFLAGS += -I/usr/pkg/include
	BASIC_LDFLAGS += -L/usr/pkg/lib
	ALL_LDFLAGS += -Wl,-rpath,/usr/pkg/lib
endif
ifeq ($(findstring ibm-aix,$(TARGET)),ibm-aix)
	NO_STRCASESTR=YesPlease
	NEEDS_LIBICONV=YesPlease
endif
ifeq ($(findstring sgi-irix,$(TARGET)),sgi-irix)
	NO_IPV6=YesPlease
	NO_SETENV=YesPlease
	NO_STRCASESTR=YesPlease
	NO_SOCKADDR_STORAGE=YesPlease
	BASIC_CFLAGS += -DPATH_MAX=1024
	# for now, build 32-bit version
	BASIC_LDFLAGS += -L/usr/lib32
endif
ifeq ($(findstring mingw,$(TARGET)),mingw)
	NO_MMAP=YesPlease
	NO_PREAD=YesPlease
	NO_OPENSSL=YesPlease
	NO_CURL=YesPlease
	NO_SYMLINK_HEAD=YesPlease
	NO_IPV6=YesPlease
	NO_ETC_PASSWD=YesPlease
	NO_SETENV=YesPlease
	NO_UNSETENV=YesPlease
	NO_STRCASESTR=YesPlease
	NO_ICONV=YesPlease
	NO_C99_FORMAT = YesPlease
	NO_STRTOUMAX = YesPlease
	NO_SYMLINKS=YesPlease
	NO_SVN_TESTS=YesPlease
	COMPAT_CFLAGS += -DNO_ETC_PASSWD -DNO_ST_BLOCKS -DSTRIP_EXTENSION=\".exe\" -I compat
	COMPAT_OBJS += compat/mingw.o compat/fnmatch.o compat/regex.o compat/vasprintf.o
	EXTLIBS += -lws2_32
	X = .exe
	NOEXECTEMPL = .noexec
endif

ifdef NO_R_TO_GCC_LINKER
	# Some gcc does not accept and pass -R to the linker to specify
	# the runtime dynamic library path.
	CC_LD_DYNPATH = -Wl,-rpath=
else
	CC_LD_DYNPATH = -R
endif

ifndef NO_CURL
	ifdef CURLDIR
		# Try "-Wl,-rpath=$(CURLDIR)/lib" in such a case.
		BASIC_CFLAGS += -I$(CURLDIR)/include
		CURL_LIBCURL = -L$(CURLDIR)/lib $(CC_LD_DYNPATH)$(CURLDIR)/lib -lcurl
	else
		CURL_LIBCURL = -lcurl
	endif
	ifndef NO_EXPAT
		EXPAT_LIBEXPAT = -lexpat
	endif
endif

ifndef NO_OPENSSL
	OPENSSL_LIBSSL = -lssl
	ifdef OPENSSLDIR
		BASIC_CFLAGS += -I$(OPENSSLDIR)/include
		OPENSSL_LINK = -L$(OPENSSLDIR)/lib $(CC_LD_DYNPATH)$(OPENSSLDIR)/lib
	else
		OPENSSL_LINK =
	endif
else
	BASIC_CFLAGS += -DNO_OPENSSL
	MOZILLA_SHA1 = 1
	OPENSSL_LIBSSL =
endif
ifdef NEEDS_SSL_WITH_CRYPTO
	LIB_4_CRYPTO = $(OPENSSL_LINK) -lcrypto -lssl
else
	LIB_4_CRYPTO = $(OPENSSL_LINK) -lcrypto
endif
ifdef NEEDS_LIBICONV
	ifdef ICONVDIR
		BASIC_CFLAGS += -I$(ICONVDIR)/include
		ICONV_LINK = -L$(ICONVDIR)/lib $(CC_LD_DYNPATH)$(ICONVDIR)/lib
	else
		ICONV_LINK =
	endif
	EXTLIBS += $(ICONV_LINK) -liconv
endif
ifdef NEEDS_SOCKET
	EXTLIBS += -lsocket
endif
ifdef NEEDS_NSL
	EXTLIBS += -lnsl
endif
ifdef NO_D_TYPE_IN_DIRENT
	BASIC_CFLAGS += -DNO_D_TYPE_IN_DIRENT
endif
ifdef NO_D_INO_IN_DIRENT
	BASIC_CFLAGS += -DNO_D_INO_IN_DIRENT
endif
ifdef NO_C99_FORMAT
	BASIC_CFLAGS += -DNO_C99_FORMAT
endif
ifdef NO_SYMLINK_HEAD
	BASIC_CFLAGS += -DNO_SYMLINK_HEAD
endif
ifdef NO_STRCASESTR
	COMPAT_CFLAGS += -DNO_STRCASESTR
	COMPAT_OBJS += compat/strcasestr.o
endif
ifdef NO_STRTOUMAX
	COMPAT_CFLAGS += -DNO_STRTOUMAX
	COMPAT_OBJS += compat/strtoumax.o
endif
ifdef NO_STRTOULL
	COMPAT_CFLAGS += -DNO_STRTOULL
endif
ifdef NO_SETENV
	COMPAT_CFLAGS += -DNO_SETENV
	COMPAT_OBJS += compat/setenv.o
endif
ifdef NO_UNSETENV
	COMPAT_CFLAGS += -DNO_UNSETENV
	COMPAT_OBJS += compat/unsetenv.o
endif
ifdef NO_MMAP
	COMPAT_CFLAGS += -DNO_MMAP
	COMPAT_OBJS += compat/mmap.o
endif
ifdef NO_PREAD
	COMPAT_CFLAGS += -DNO_PREAD
	COMPAT_OBJS += compat/pread.o
endif
ifdef NO_FAST_WORKING_DIRECTORY
	BASIC_CFLAGS += -DNO_FAST_WORKING_DIRECTORY
endif
ifdef NO_TRUSTABLE_FILEMODE
	BASIC_CFLAGS += -DNO_TRUSTABLE_FILEMODE
endif
ifdef NO_IPV6
	BASIC_CFLAGS += -DNO_IPV6
endif
ifdef NO_SOCKADDR_STORAGE
ifdef NO_IPV6
	BASIC_CFLAGS += -Dsockaddr_storage=sockaddr_in
else
	BASIC_CFLAGS += -Dsockaddr_storage=sockaddr_in6
endif
endif
ifdef NO_INET_NTOP
	COMPAT_OBJS += compat/inet_ntop.o
endif
ifdef NO_INET_PTON
	COMPAT_OBJS += compat/inet_pton.o
endif

ifdef NO_ICONV
	BASIC_CFLAGS += -DNO_ICONV
endif

ifdef OLD_ICONV
	BASIC_CFLAGS += -DOLD_ICONV
endif
ifdef NO_HSTRERROR
	COMPAT_CFLAGS += -DNO_HSTRERROR
	COMPAT_OBJS += compat/hstrerror.o
endif

QUIET_SUBDIR0  = +$(MAKE) -C # space to separate -C and subdir
QUIET_SUBDIR1  =

ifneq ($(findstring $(MAKEFLAGS),w),w)
PRINT_DIR = --no-print-directory
else # "make -w"
NO_SUBDIR = :
endif

ifneq ($(findstring $(MAKEFLAGS),s),s)
ifndef V
	QUIET_CC       = @echo '   ' CC $@;
	QUIET_AR       = @echo '   ' AR $@;
	QUIET_LINK     = @echo ' ' LINK $@;
	QUIET_BUILT_IN = @echo  BUILTIN $@;
	QUIET_GEN      = @echo '  ' GEN $@;
	QUIET_STRIP    = @echo ' ' STRIP $@;
	QUIET_SUBDIR0  = +@subdir=
	QUIET_SUBDIR1  = ;$(NO_SUBDIR) echo '   ' SUBDIR $$subdir; \
			 $(MAKE) $(PRINT_DIR) -C $$subdir
	export V
	export QUIET_GEN
	export QUIET_BUILT_IN
endif
endif

# Shell quote (do not use $(call) to accommodate ancient setups);

SHA1_HEADER_SQ = $(subst ','\'',$(SHA1_HEADER))

LIBS = $(EXTLIBS)

BASIC_CFLAGS += -DSHA1_HEADER='$(SHA1_HEADER_SQ)' $(COMPAT_CFLAGS)
GIT_OBJS += $(COMPAT_OBJS)

ALL_CFLAGS += $(BASIC_CFLAGS)
ALL_LDFLAGS += $(BASIC_LDFLAGS)

ALL_CFLAGS += $(BOX_CFLAGS) -std=gnu99 -D_GNU_SOURCE
ALL_LDFLAGS += $(BOX_LDFLAGS)


### Build rules

all: $(ALL_PROGRAMS)

strip: $(PROGRAMS)
	$(STRIP) $(STRIP_OPTS) $(PROGRAMS)

configure: configure.ac
	$(QUIET_GEN)$(RM) $@ $<+ && \
	sed -e 's/@@GIT_VERSION@@/$(GIT_VERSION)/g' \
	    $< > $<+ && \
	autoconf -o $@ $<+ && \
	$(RM) $<+

%.o: %.c 
	$(QUIET_CC)$(CC) -o $*.o -c $(ALL_CFLAGS) $<
%.o: %.C 
	$(QUIET_CC)$(CC) -o $*.o -c $(ALL_CFLAGS) $(LAUNCHER_CFLAGS) $<
%.s: %.c 
	$(QUIET_CC)$(CC) -S $(ALL_CFLAGS) $<
%.o: %.S
	$(QUIET_CC)$(CC) -o $*.o -c $(ALL_CFLAGS) $<

box/applets/applet_tables.o: box/applets/applet_tables.c box/include/autoconf.h box/include/busybox.h
	$(QUIET_CC)$(HOST_CC) -o $*.o -c $(ALL_CFLAGS) $<
box/applets/applet_tables: box/applets/applet_tables.o
	$(QUIET_LINK)$(HOST_CC) -o $@ $^

box/include/applet_tables.h: box/include/applets.h box/applets/applet_tables
	$(QUIET_GEN)box/applets/applet_tables box/include/applet_tables.h

box/libbb/appletlib.o: box/include/applet_tables.h

$(BOX_OBJS): $(BOX_H) git-compat-util.h exec_cmd.h quote.h run-command.h spawn-pipe.h
box/shell/ash.o: box/shell/ash_fork.c box/shell/ash_fork.h

$(BOX_FILE): $(BOX_OBJS)
	$(QUIET_AR)rm -f $@ && $(AR) rcs $@ $(BOX_OBJS)

gitbox$X: $(GIT_OBJS) $(BOX_FILE)
	$(QUIET_LINK)$(CC) $(ALL_CFLAGS) -o $@ $^ $(ALL_LDFLAGS) $(LIBS)

gw_launcher$X: $(LAUNCHER_OBJS) $(COMPAT_OBJS) $(BOX_FILE)
	$(QUIET_LINK)$(CC) $(ALL_CFLAGS) $(LAUNCHER_CFLAGS) -o $@ $^ $(COMPAT_CFLAGS)  $(LAUNCHER_LDFLAGS) $(ALL_LDFLAGS) $(LIBS) 

gw_launcher$X-clean:
	$(RM) $(LAUNCHER_OBJS) gw_launcher$X

PACKAGEFLAGS=
ifneq ($(shell uname),Darwin)
 PACKAGEFLAGS+=-static
endif

package: LDFLAGS+=$(PACKAGEFLAGS)
package: clean gitbox$X gw_launcher$X
	$(QUIET_STRIP)$(STRIP) gitbox$X
	$(QUIET_STRIP)$(STRIP) gw_launcher$X
	@if [ `svnversion` != "exported" ]; then svn2cl --group-by-day --authors=AUTHORS -o ChangeLog; fi
	@case `file ./gitbox$X | cut -d " " -f 2-3,5-6,14 | tr -d ","` in \
	     "PE32 executable MS Windows") \
	     	  EXEARCH="win32" ;; \
	     "ELF 32-bit executable Intel"*) \
	     	  EXEARCH="linux32" ;; \
	     "ELF 64-bit executable x86-64"*) \
	     	  EXEARCH="linux64" ;; \
	     "Mach-O executable"*) \
		  EXEARCH="macosx" ;; \
	esac; \
	if [ "`which latex`" != "" ]; then \
	   $(MAKE) -C doc clean manual; \
	   MANUALPDF=doc/manual.pdf; \
	fi; \
	tar czf genwrapper-$${EXEARCH}-`svnversion`.tar.gz $(PACKAGE_FILES) $${MANUALPDF}
	
### Cleaning rules

clean:
	$(RM) $(ALL_PROGRAMS)
	$(RM) $(BOX_OBJS) $(GIT_OBJS) $(LAUNCHER_OBJS) $(BOX_FILE)
	$(RM) box/applets/applet_tables.o box/applets/applet_tables box/include/applet_tables.h
	$(RM) *.spec *.pyc *.pyo */*.pyc */*.pyo common-cmds.h TAGS tags
	$(RM) -r autom4te.cache
	$(RM) configure config.log config.mak.autogen config.mak.append config.status config.cache
	find . -name "*~" -exec $(RM) {} \;
	$(MAKE) -C doc clean

.PHONY: all install clean strip
