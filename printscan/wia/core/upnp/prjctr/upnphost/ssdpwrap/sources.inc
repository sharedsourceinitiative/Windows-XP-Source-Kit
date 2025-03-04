# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   orenr
#
#Date:
#   2-Feb-2000
#
#Module Name:
#    sources.inc
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds
#     ssdpwrap.lib    -
#
# ############################################################


!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=ssdpwrap
TARGETTYPE=LIBRARY
TARGETPATH=..\..\lib\$(_OBJ_DIR)

USE_ATL=1

INCLUDES=$(INCLUDES);..\..\inc;

SOURCES=\
	..\private.cpp
