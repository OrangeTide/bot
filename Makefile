all :
clean ::
.PHONY : all clean
#
ifneq ($V,1)
# delete these lines if you don't find colors amusing.
# also, if input is not a tty, then you won't get colors.
_START_COLOR := $(shell tty >/dev/null && tput setaf 1 && tput bold)
_END_COLOR := $(shell tty >/dev/null && tput sgr0)
Q = @echo "$1$(_START_COLOR)";$2;echo -n "$(_END_COLOR)"
else
Q = $2
endif
# select OS parameters
M := $(shell $(CC) -dumpmachine)
ifneq ($(findstring mingw,$M),)
CFLAGS = -Wall -W -g -O0 -mno-cygwin -mconsole
LDLIBS_net = -lws2_32
E = .exe
else
CFLAGS = -Wall -W -g -O0
LDLIBS_net =
E =
endif
#
%.o : %.c
	$(call Q,CC $@,$(CROSS_COMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(filter %.c,$^))
%.c : %.leg
	$(call Q,LEG $@ <- $<,leg -o$@ $<)
#
define gen
SRCS_$1 := $2
OBJS_$1 := $$(SRCS_$1:.c=.o)
all : $1$E
clean ::
	$$(RM) $1$E $$(OBJS_$1)
$1$E : $$(OBJS_$1)
	$(call Q,LD $$@ ($$^),$$(CROSS_COMPILE)$$(CC) $$(CFLAGS) $$(CPPFLAGS) $$(LDFLAGS) $3 -o $$@ $$(filter %.o %.a,$$^))
endef
build-exe = $(eval $(call gen,$1,$2,$3))
#
$(call build-exe,bot,net.c bot.c ini.c xcalc.c, $(LDLIBS_net))
$(call build-exe,calc,calcmain.c xcalc.c)
xcalc.o : CFLAGS += -Wno-unused
