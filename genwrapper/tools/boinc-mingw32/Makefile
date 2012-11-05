
ifndef BOINCDIR
       BOINCDIR = ../boinc-6
endif

HOST_CC = gcc        
HOST_CXX = g++
ifeq ($(HOST),$(TARGET))                   
     CC = $(HOST_CC)
     CXX = g++
     AR = ar
     RANLIB = ranlib
else  
     CC = $(TARGET)-gcc
     CXX = $(TARGET)-g++
     AR = $(TARGET)-ar
     RANLIB = $(TARGET)-ranlib
endif

DEBUG = -D__DEBUG__ -g3
OBJ  = 	boinc_api.o \
	util.o \
	win_util.o \
	app_ipc.o \
	diagnostics.o \
	diagnostics_win.o \
	filesys.o \
	hostinfo.o \
	md5.o \
	md5_file.o \
	mem_usage.o \
	mfile.o \
	miofile.o \
	parse.o \
	prefs.o \
	proxy_info.o \
	str_util.o \
	shmem.o \
	base64.o
LINKOBJ  = $(OBJ)
LDFLAGS = -lwinmm -march=i386
INCS = -I"$(BOINCDIR)" -I"$(BOINCDIR)/db" -I"$(BOINCDIR)" -I"$(BOINCDIR)/lib" -I"$(BOINCDIR)/api" 
CXXINCS = $(INCS)
BIN  = libboinc.a
#
CXXFLAGS = $(CXXINCS) $(DEBUG) --include $(BOINCDIR)/version.h -DWIN32 -D_WIN32 -D_WIN32_WINDOWS=0x0501 -D_MT -DNDEBUG -D_WINDOWS  -DBOINC -DCLIENT -DNODB -D_CONSOLE -Wall -mtune=pentium-m   -fexceptions -march=i386 -O1
CFLAGS = $(INCS) $(DEBUG) --include $(BOINCDIR)/version.h -DWIN32 -D_WIN32 -D_WIN32_WINDOWS=0x0501 -D_MT -DNDEBUG -D_WINDOWS  -DBOINC -DCLIENT -DNODB -D_CONSOLE -Wall -mtune=pentium-m   -fexceptions -march=i386 -O1

INCLUDESTOCOPY =\
	       lib/app_ipc.h \
	       api/boinc_api.h \
	       lib/boinc_win.h \
	       lib/common_defs.h \
	       lib/diagnostics.h \
	       lib/error_numbers.h \
	       lib/filesys.h \
	       lib/hostinfo.h \
	       lib/mfile.h \
	       lib/miofile.h \
	       lib/parse.h \
	       lib/prefs.h \
	       lib/proxy_info.h \
	       lib/str_util.h \
	       lib/util.h \
	       lib/win_util.h

.PHONY: all all-before all-after clean clean-custom


all: all-before $(BIN) all-after

all-after: copy-files clean-up

clean: clean-custom
	${RM} -rf $(OBJ) $(BIN) include/

$(BIN): $(LINKOBJ)
	$(AR) rc $@ $(LINKOBJ)
	$(RANLIB) $@

%.o: $(BOINCDIR)/api/%.C
	$(CXX) -c $< -o $@ $(CXXFLAGS)

%.o: $(BOINCDIR)/lib/%.C
	$(CXX) -c $< -o $@ $(CXXFLAGS)

md5.o: $(BOINCDIR)/lib/md5.c
	$(CXX) -c $(BOINCDIR)/lib/md5.c -o md5.o $(CXXFLAGS)

stackwalker_win.o: $(BOINCDIR)/lib/stackwalker_win.cpp
	$(CXX) -c $(BOINCDIR)/lib/stackwalker_win.cpp -o stackwalker_win.o $(CXXFLAGS)

copy-files:
	mkdir -p include
	cp $(addprefix $(BOINCDIR), $(INCLUDESTOCOPY)) include/

clean-up:
	${RM} $(OBJ) 