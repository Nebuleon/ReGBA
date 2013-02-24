SOURCES	+= $(wildcard $(I2CDIR)/*.c)
CFLAGS	+= -DI2C=$(I2C)
CFLAGS	+= -I$(I2CDIR)
VPATH   += $(I2CDIR)
