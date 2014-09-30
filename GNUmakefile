-include .config

prefix ?= /usr/local
includedir ?= $(prefix)/include
libdir ?= $(prefix)/lib
bindir ?= $(prefix)/bin

luacpath ?=
lua51cpath ?=
lua52cpath ?=
lua53cpath ?=

RMDIR ?= rmdir
MKDIR ?= mkdir
CP ?= cp

VENDOR_OS ?= $(shell mk/vendor.os)
VENDOR_CC ?= $(shell mk/vendor.cc)

ifeq ($(SOFLAGS),)
ifeq ($(VENDOR_OS), Darwin)
SOFLAGS = -bundle -undefined dynamic_lookup
else
SOFLAGS = -shared
endif
endif


all:

.PHONY: config

config:
	printf 'prefix ?= $(value prefix)'"\n" >| .config
	printf 'includedir ?= $(value includedir)'"\n" >> .config
	printf 'libdir ?= $(value libdir)'"\n" >> .config
	printf 'bindir ?= $(value bindir)'"\n" >> .config
	printf 'luacpath ?= $(value luacpath)'"\n" >> .config
	printf 'CC ?= $(value CC)'"\n" >> .config
	printf 'CPPFLAGS ?= $(value CPPFLAGS)'"\n" >> .config
	printf 'CFLAGS ?= $(value CFLAGS)'"\n" >> .config
	printf 'LDFLAGS ?= $(value LDFLAGS)'"\n" >> .config
	printf 'SOFLAGS ?= $(value SOFLAGS)'"\n" >> .config
	printf 'RM ?= $(value RM)'"\n" >> .config
	printf 'RMDIR ?= $(value RMDIR)'"\n" >> .config
	printf 'MKDIR ?= $(value MKDIR)'"\n" >> .config
	printf 'CP ?= $(value CP)'"\n" >> .config
	printf 'VENDOR_OS ?= $(value VENDOR_OS)'"\n" >> .config
	printf 'VENDOR_CC ?= $(value VENDOR_CC)'"\n" >> .config

LUAPATH = $(shell env CC="$(CC)" CPPFLAGS="$(CPPFLAGS)" LDFLAGS="$(LDFLAGS)" mk/luapath -krxm3 $(if $(includedir),$(if $(DESTDIR), -I$(DESTDIR)$(includedir)) -I$(includedir)) -I/usr/include -I/usr/local/include $(if $(DESTDIR),-P$(DESTDIR)$(bindir)) -P$(bindir) -v$(1) $(2))

define LUALIB_BUILD
$(1)/hexdump.so: hexdump.c hexdump.h
	test "$(1)" = "$$(call LUAPATH, $(1), version)"
	$$(MKDIR) -p $$(@D)
	$$(CC) $$(CFLAGS) $$(SOFLAGS) -o $$@ $$< $$(CPPFLAGS) $$(call LUAPATH, $(1), cppflags) -DHEXDUMP_LUALIB

.SECONDARY: all$(1)

all$(1): $(1)/hexdump.so

endef # LUALIB_BUILD

$(eval $(call LUALIB_BUILD,5.1))

$(eval $(call LUALIB_BUILD,5.2))

$(eval $(call LUALIB_BUILD,5.3))


define LUALIB_INSTALL
ifneq ($(filter install install$(1) uninstall uninstall$(1), $(MAKECMDGOALS)),)
ifeq ($$($(2)),) # define lua5?cpath if empty
$(2)_dyn = $$(call LUAPATH, $(1), cdir)
$(2)_sed = $$(shell printf "$$(luacpath)" | sed -ne 's/[[:digit:]].[[:digit:]]/$(1)/p')
$(2)_lib = $$(libdir)/lua/$(1)

override $(2) = $$(or $$($(2)_dyn), $$($(2)_sed), $$($(2)_lib))
endif

$$($(2))/hexdump.so: $(1)/hexdump.so
	$$(MKDIR) -p $$(@D)
	$$(CP) -fp $$< $$@

.SECONDARY: install install$(1)

install install$(1): $$($(2))/hexdump.so

.PHONY: uninstall uninstall$(1)

uninstall$(1):
	$(RM) -f $$($(2))/hexdump.so

uninstall: uninstall$(1)

endif # if install or install$(1)
endef # LUALIB_INSTALL

$(eval $(call LUALIB_INSTALL,5.1,lua51cpath))

$(eval $(call LUALIB_INSTALL,5.2,lua52cpath))

$(eval $(call LUALIB_INSTALL,5.3,lua53cpath))


#
# 
#
.PHONY: clean distclean clean~

distclean: clean
	$(RM) -f .config

clean:
	$(RM) -f hexdump
	$(RM) -fr 5.?/
	$(RM) -fr *.dSYM/

clean~:
	find . -name *~ -exec rm {} +
