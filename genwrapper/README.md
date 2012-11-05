Readme
=====

For original readme, please see README file.

Linux Compilation
=====
Compilation on linux for linux. You will need boinc-devel libraries. It worked for me with following RPMs:
```
boinc-client-6.12.35-1.r24014svn.fc15.x86_64
boinc-client-devel-6.12.35-1.r24014svn.fc15.x86_64
boinc-client-static-6.12.35-1.r24014svn.fc15.x86_64
boinc-client-doc-6.12.35-1.r24014svn.fc15.noarch
boinc-manager-6.12.35-1.r24014svn.fc15.x86_64
boinc-client-debuginfo-6.12.35-1.r24014svn.fc15.x86_64
```

Alternatively you can clone original BOINC git repo and compile libraries on your own
```
git clone git://boinc.berkeley.edu/boinc.git
cd boinc/lib
make
make install
```

For compilation I used following gcc version: gcc version 4.6.3 20120306 (Red Hat 4.6.3-2) (GCC). 

Before compilation, you need to modify Makefile and configure correct boinc libraries, but current Makefile should work.

Windows Compilation
=====
This is much trickier to do. I decided to use cross-compilation from linux to windows with use of MinGW. Another option is to compile it on windows inside cygwin environment with use of MinGW compiler.

MinGW on linux
=====
I have following MinGW RPMs installed:
```
mingw32-binutils-2.21-1.fc15.x86_64
mingw32-cpp-4.5.3-1.fc15.x86_64
mingw32-curl-7.20.1-2.fc15.noarch
mingw32-curl-debuginfo-7.20.1-2.fc15.noarch
mingw32-curl-static-7.20.1-2.fc15.noarch
mingw32-filesystem-69-5.fc15.noarch
mingw32-gcc-4.5.3-1.fc15.x86_64
mingw32-gcc-c++-4.5.3-1.fc15.x86_64
mingw32-gcc-debuginfo-4.5.3-1.fc15.x86_64
mingw32-gettext-0.17-15.fc15.noarch
mingw32-iconv-1.12-13.fc15.noarch
mingw32-libidn-1.14-8.fc15.noarch
mingw32-libssh2-1.1-6.fc15.noarch
mingw32-openssl-1.0.0d-1.fc15.noarch
mingw32-openssl-static-1.0.0d-1.fc15.noarch
mingw32-pthreads-2.8.0-13.fc15.noarch
mingw32-readline-5.2-8.fc15.noarch
mingw32-runtime-3.15.2-5.fc13.noarch
mingw32-sqlite-3.7.5-2.fc15.noarch
mingw32-sqlite-static-3.7.5-2.fc15.noarch
mingw32-termcap-1.3.1-9.fc15.noarch
mingw32-w32api-3.15-2.fc15.noarch
mingw32-zlib-1.2.5-3.fc15.noarch
mingw32-zlib-static-1.2.5-3.fc15.noarch
```

At first, you need to compile BOINC libraries. Please read linux compile section - clone BOINC library.
Compilation with MinGW:
```
git clone git://boinc.berkeley.edu/boinc.git
cd boinc/lib
mingw32-make TARGET=i686-pc-mingw32 -f Makefile.mingw 
i686-pc-mingw32-ranlib libboinc.a
i686-pc-mingw32-ranlib libboinc_api.a
```

Then you can proceed to genwrapper compilation. Please set BOINC libraries correctly in Makefile before start
```
mingw32-make TARGET=i686-pc-mingw32
```

Cygwin
=====
Install all MinGW libraries to your cygwin with setup.exe.

BOINC lib build:
```
 make CXX=i686-pc-mingw32-g++ CC=i686-pc-mingw32-gcc TARGET=i686-pc-mingw32 -f Makefile.mingw
```

GenWrapper:
```
 make CXX=i686-pc-mingw32-g++ CC=i686-pc-mingw32-gcc TARGET=i686-pc-mingw32
```


Troubleshooting
=====
All problems mentioned below should be solved in this code, but it can be helpfull if you want to compile original genwrapper version.


unzip
------
* Symptom: box/archival/unzip.c:109:7: error: size of array 'BUG_zip_header_must_be_26_bytes' is negative
* Reason: tructure packing does not work... It helps to add pragma pack preprocessor directive.
* Solution:
```
#pragma pack(1)
typedef union {
  uint8_t raw[ZIP_HEADER_LEN];
	struct {
		uint16_t version ;                       /* 0-1 */
		uint16_t flags ;                         /* 2-3 */
		uint16_t method ;                        /* 4-5 */
		uint16_t modtime ;                       /* 6-7 */
		uint16_t moddate ;                       /* 8-9 */
		uint32_t crc32 ATTRIBUTE_PACKED;        /* 10-13 */
		uint32_t cmpsize ATTRIBUTE_PACKED;      /* 14-17 */
		uint32_t ucmpsize ATTRIBUTE_PACKED;     /* 18-21 */
		uint16_t filename_len ;                  /* 22-23 */
		uint16_t extra_len ;                     /* 24-25 */
	} formatted ATTRIBUTE_PACKED;
} zip_header_t; /* ATTRIBUTE_PACKED - gcc 4.2.1 doesn't like it (spews warning) */
#pragma pack()
```

locale.h
--------
* Symptom: During genwrapper compilation, gitbox particularly.
```
In file included from ../boinc_src/boinc/lib/boinc_win.h:209:0,
                 from ../boinc_src/boinc/api/boinc_api.h:24,
                 from box/boinc/boinc.c:23:
/usr/i686-pc-mingw32/sys-root/mingw/include/locale.h:71:40: error: expected identifier or '(' before 'void'
/usr/i686-pc-mingw32/sys-root/mingw/include/locale.h:71:40: error: expected ')' before numeric constant
```
* Solution:
During gitbox compilation comment following lines in mentioned locale.h file:
```
//_CRTIMP  char* __cdecl __MINGW_NOTHROW setlocale (int, const char*);
//_CRTIMP struct lconv* __cdecl __MINGW_NOTHROW localeconv (void);
```
Please note that after compilation of gitbox, you HAVE TO revert these changes, because you won't be able to compile genwrapper, it will fail.

lth32
-----
* Problem:
```
LINK gitbox.exe
/usr/lib64/gcc/i686-pc-mingw32/4.5.3/../../../../i686-pc-mingw32/bin/ld: cannot find -lth32
```
* Fix:
Just remove -lth32 occurrence from Makefile, this library is not needed

gw_launcher linker
------------------
```
LINK gw_launcher.exe
launcher/gw_common.o: In function `_Z23gw_report_fraction_doned':
/home/ph4r05/Documents/Vmshare/genwrapper/launcher/gw_common.C:133: undefined reference to `_app_client_shm'
/home/ph4r05/Documents/Vmshare/genwrapper/launcher/gw_common.C:140: undefined reference to `_app_client_shm'
launcher/gw_common.o: In function `_Z16gw_report_statusddb':
/home/ph4r05/Documents/Vmshare/genwrapper/launcher/gw_common.C:148: undefined reference to `_app_client_shm'
/home/ph4r05/Documents/Vmshare/genwrapper/launcher/gw_common.C:168: undefined reference to `_app_client_shm'
/home/ph4r05/Documents/Vmshare/genwrapper/launcher/gw_common.C:172: undefined reference to `_app_client_shm'
launcher/gw_common.o: In function `_Z9gw_finishid':
/home/ph4r05/Documents/Vmshare/genwrapper/launcher/gw_common.C:126: undefined reference to `_boinc_finish'
launcher/gw_launcher.o: In function `_Z19poll_boinc_messagesR4TASK':
/home/ph4r05/Documents/Vmshare/genwrapper/launcher/gw_launcher.C:86: undefined reference to `_boinc_get_status'
launcher/gw_launcher.o: In function `main':
/home/ph4r05/Documents/Vmshare/genwrapper/launcher/gw_launcher.C:126: undefined reference to `_boinc_init_options'
```

* Solution: both libboinc.a and libboinc_api.a needs to be COMPILED AND LINKED with gw_launcher
At first, you must index libboinc libraries (it is neccessary for linker - it would demand it otherwise):
```
$ i686-pc-mingw32-ranlib ../boinc_src/lib/libboinc.a 
$ i686-pc-mingw32-ranlib ../boinc_src/lib/libboinc_api.a
```

Please see Makefile to see modifications for fixing this bug.


