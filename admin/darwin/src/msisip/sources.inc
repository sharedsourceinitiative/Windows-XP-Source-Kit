
TARGETNAME=msisip
!INCLUDE ..\..\MsiMake.inc

TARGETTYPE=DYNLINK
TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\libcmt.lib

C_DEFINES=$(C_DEFINES)
USE_NOLIBS=1
USE_NATIVE_EH=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\msisip.def

INCLUDES=$(INCLUDES);$(INC_DIR);$(TOOLS_INC_DIR);$(RES_OBJDIR)

SOURCES=..\msisip.cpp \
	..\msisip.rc

