LIBMXDIR = ../../../libMx

all: vtimer_multi vtimer_oneshot vtimer_periodic

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

vtimer_multi: vtimer_multi.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vtimer_multi$(DOTEXE) vtimer_multi.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) \
		$(LIB_DIRS) $(LIBRARIES)

vtimer_oneshot: vtimer_oneshot.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vtimer_oneshot$(DOTEXE) vtimer_oneshot.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) \
		$(LIB_DIRS) $(LIBRARIES)

vtimer_periodic: vtimer_periodic.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vtimer_periodic$(DOTEXE) vtimer_periodic.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) \
		$(LIB_DIRS) $(LIBRARIES)

clean:
	-$(RM) vtimer_multi vtimer_oneshot vtimer_periodic
	-$(RM) *.o *.obj *.exe *.ilk *.pdb *.manifest

