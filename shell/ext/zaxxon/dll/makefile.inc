#clean:
#    -del /F /Q $(NTTARGETFILE0)
#    -del /F /Q $(NTTARGETFILES)


# These are the dependencies that the RC file has
#
..\resource.rc: $(SELFREGNAME)


!include $(CCSHELL_DIR)\makefile.inc

