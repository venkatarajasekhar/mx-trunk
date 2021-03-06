#
# Module: libusb-0.1
#
# This module supports the USB interface provided by the old, deprecated
# libusb-0.1 API that is described at the http://www.libusb.org/ website.
#
# Note that libusb-0.1 itself supports a variety of operating systems,
# but the MX libusb-0.1 module has only been built on Linux so far.
#
# Supported values of MX_ARCH: linux
#

ifndef MX_ARCH
    MX_ARCH = linux
endif

ifeq ($(MX_ARCH),win32)
	LIBUSB-0.1_DIR = c:\\docume~1\\lavender\\libusb-0.1
	CFLAGS += -EHsc
	CFLAGS += -I$(LIBUSB-0.1_DIR)\\Include
endif

all: libusb-0.1.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

LIBUSB-0.1_OBJS = libusb-0.1.$(OBJ) i_libusb-0.1.$(OBJ) 

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

libusb-0.1.mxo: $(LIBUSB-0.1_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,libusb-0.1.mxo \
		-o libusb-0.1.mxo $(LIBUSB-0.1_OBJS) -lusb

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

libusb-0.1.mxo: $(LIBUSB-0.1_OBJS)
	link /dll /debug /nologo /out:libusb-0.1.mxo /nodefaultlib:libc \
		$(LIBUSB-0.1_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(LIBUSB-0.1_DIR)\\Lib\\PccLib.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f libusb-0.1.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:libusb-0.1.mxo\;2 \
			-manifest libusb-0.1.mxo.manifest ; \
		rm -f libusb-0.1.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

libusb-0.1.$(OBJ): libusb-0.1.c i_libusb-0.1.h
	$(COMPILE) $(CFLAGS) libusb-0.1.c

i_libusb-0.1.$(OBJ): i_libusb-0.1.c i_libusb-0.1.h
	$(COMPILE) $(CFLAGS) i_libusb-0.1.c

install: libusb-0.1.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp libusb-0.1.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.o *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


