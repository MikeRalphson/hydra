ORIGIN = PWB
ORIGIN_VER = 2.1.49
PROJ = MUNPACK
PROJFILE = MUNPACK.MAK
DEBUG = 0

CC  = cl
CFLAGS_G  = /W2 /D__MSDOS__ /BATCH
CFLAGS_D  = /f /Od /Zi
CFLAGS_R  = /f- /Ot /Ol /Og /Oe /Oi /Gs
CXX  = cl
CXXFLAGS_G  = /W2 /BATCH
CXXFLAGS_D  = /f /Zi /Od
CXXFLAGS_R  = /f- /Ot /Oi /Ol /Oe /Og /Gs
MAPFILE_D  = NUL
MAPFILE_R  = NUL
LFLAGS_G  = /NOI /STACK:8192 /BATCH /ONERROR:NOEXE
LFLAGS_D  = /CO /FAR /PACKC
LFLAGS_R  = /EXE /FAR /PACKC
LINKER	= link
ILINK  = ilink
LRF  = echo > NUL
ILFLAGS  = /a /e
NMFLAGS  = /K

FILES  = CODES.C DOSOS.C GETOPT.C STRING.C XMALLOC.C MD5C.C DECODE.C DOSUNPK.C\
	UUDECODE.C PART.C
OBJS  = CODES.obj DOSOS.obj GETOPT.obj STRING.obj XMALLOC.obj MD5C.obj\
	DECODE.obj DOSUNPK.obj UUDECODE.obj PART.obj

all: $(PROJ).exe

.SUFFIXES:
.SUFFIXES: .obj .c
.SUFFIXES: .obj .c

CODES.obj : CODES.C xmalloc.h md5.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoCODES.obj CODES.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoCODES.obj CODES.C
<<
!ENDIF

DOSOS.obj : DOSOS.C xmalloc.h common.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoDOSOS.obj DOSOS.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoDOSOS.obj DOSOS.C
<<
!ENDIF

GETOPT.obj : GETOPT.C
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoGETOPT.obj GETOPT.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoGETOPT.obj GETOPT.C
<<
!ENDIF

STRING.obj : STRING.C
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoSTRING.obj STRING.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoSTRING.obj STRING.C
<<
!ENDIF

XMALLOC.obj : XMALLOC.C
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoXMALLOC.obj XMALLOC.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoXMALLOC.obj XMALLOC.C
<<
!ENDIF

MD5C.obj : MD5C.C md5.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoMD5C.obj MD5C.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoMD5C.obj MD5C.C
<<
!ENDIF

DECODE.obj : DECODE.C xmalloc.h common.h part.h md5.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoDECODE.obj DECODE.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoDECODE.obj DECODE.C
<<
!ENDIF

DOSUNPK.obj : DOSUNPK.C version.h part.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoDOSUNPK.obj DOSUNPK.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoDOSUNPK.obj DOSUNPK.C
<<
!ENDIF

UUDECODE.obj : UUDECODE.C xmalloc.h common.h part.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoUUDECODE.obj UUDECODE.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoUUDECODE.obj UUDECODE.C
<<
!ENDIF

PART.obj : PART.C part.h xmalloc.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoPART.obj PART.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoPART.obj PART.C
<<
!ENDIF


$(PROJ).exe : $(OBJS)
!IF $(DEBUG)
	$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_D)
$(LIBS: = +^
) +
$(LLIBS_G: = +^
) +
$(LLIBS_D: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_D);
<<
!ELSE
	$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_R)
$(LIBS: = +^
) +
$(LLIBS_G: = +^
) +
$(LLIBS_R: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_R);
<<
!ENDIF
	$(LINKER) @$(PROJ).lrf


.c.obj :
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /Fo$@ $<
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /Fo$@ $<
<<
!ENDIF


run: $(PROJ).exe
	$(PROJ).exe $(RUNFLAGS)

debug: $(PROJ).exe
	CV $(CVFLAGS) $(PROJ).exe $(RUNFLAGS)
