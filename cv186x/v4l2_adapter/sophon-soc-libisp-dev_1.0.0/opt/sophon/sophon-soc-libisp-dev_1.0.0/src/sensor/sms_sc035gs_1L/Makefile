SHELL = /bin/bash

SDIR = $(CURDIR)
SRCS = $(wildcard $(SDIR)/*.c)

INCS = -I./ -I../../../include -I../../../include/inc

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

CFLAGS = -fPIC -ffunction-sections -fdata-sections -Wl,--gc-sections
CFLAGS += $(PROJ_CFLAGS)

EXTRA_CFLAGS = $(INCS)
EXTRA_LDFLAGS =

.PHONY : clean all

all : $(OBJS)

$(SDIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

clean:
	-@rm -f $(OBJS) $(DEPS)

-include $(DEPS)
