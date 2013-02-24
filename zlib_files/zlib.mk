SOURCES += $(wildcard $(ZLIBDIR)/*.c)
CFLAGS	+= -I$(ZLIBDIR)
VPATH   += $(ZLIBDIR)
