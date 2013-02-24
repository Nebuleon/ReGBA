SOURCES	+= $(wildcard $(WAVDIR)/*.c)
CFLAGS	+= -I$(WAVDIR)
VPATH   +=$(WAVDIR)

