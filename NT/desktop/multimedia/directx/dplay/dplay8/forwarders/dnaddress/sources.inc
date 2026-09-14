TARGETNAME=dpnaddr
TARGETPATH=obj
TARGETTYPE=DYNLINK
BUFFER_OVERFLOW_CHECKS=0
UMTYPE=windows
EXPECTED_WINVER=10.00
DLLDEF=..\dnadd.def

# This causes any other directory that is building in a concurrent thread to finish
# before this directory is built.  This is necessary to ensure that dpnet.lib has
# been produced since we use it prior to the link phase via LINKLIBS.  Apparently
# when a DLL forwards to another DLL, the DEF file is parsed at compile time and the
# LIB being forwarded to is needed at that time.

SOURCES= \
	dnadd.rc \
	adddllmain.cpp

