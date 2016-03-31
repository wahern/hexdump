all: # default rule

#
# G N U  M A K E  F U N C T I O N S
#

# template for invoking luapath script
LUAPATH = mk/luapath
LUAPATH_FN = $(shell env CC='$(subst ',\\',$(CC))' CPPFLAGS='$(subst ',\\',$(CPPFLAGS))' LDFLAGS='$(subst ',\\',$(LDFLAGS))' $(LUAPATH) -krxm3 -I '$(subst ',\\',$(DESTDIR)$(includedir))' -I/usr/include -I/usr/local/include -P '$(subst ',\\',$(DESTDIR)$(bindir))' -P '$(subst ',\\',$(bindir))' -L '$(subst ',\\',$(DESTDIR)$(libdir))' -L '$(subst ',\\',$(libdir))' -v$(1) $(2))

# check whether luapath can locate Lua $(1) headers
HAVE_API_FN = $(and $(filter $(1),$(call LUAPATH_FN,$(1),version)),$(1)$(info enabling Lua $(1)))

# check whether $(1) in LUA_APIS or $(LUA$(1:.=)_CPPFLAGS) is non-empty
WITH_API_FN = $$(and $$(or $$(filter $(1),$$(LUA_APIS)),$$(LUA$(subst .,,$(1))_CPPFLAGS)),$(1))

#
# B U I L D  F L A G S
#
CP = cp
RM = rm -f
RMDIR = rmdir
MKDIR = mkdir

VENDOR_OS = $(shell mk/vendor.os)
VENDOR_CC = $(shell mk/vendor.cc)

ALL_CPPFLAGS = $(CPPFLAGS)

ifeq ($(VENDOR_CC), sunpro)
ALL_CFLAGS = -g -xcode=pic13 $(CFLAGS)
else
ALL_CFLAGS = -fPIC -g -O2 -std=c99 -Wall -Wextra -Werror $(CFLAGS)
endif

ifeq ($(VENDOR_OS), Darwin)
ALL_SOFLAGS = -bundle -undefined dynamic_lookup $(SOFLAGS)
else
ALL_SOFLAGS = -shared $(SOFLAGS)
endif

ALL_DYFLAGS = -dynamiclib -undefined error

LUA_APIS := $(call HAVE_API_FN,5.1) $(call HAVE_API_FN,5.2) $(call HAVE_API_FN,5.3)
LUA_APIS := $(strip $(LUA_APIS))

# define LUA51_CPPFLAGS, LUA52_CPPFLAGS, etc
define LUA_CPPFLAGS_template
LUA$(subst .,,$(1))_CPPFLAGS = $$(and $$(call WITH_API_FN,$(1)),$$(call LUAPATH_FN,$(1),cppflags))
ALL_LUA$(subst .,,$(1))_CPPFLAGS = $$(LUA$(subst .,,$(1))_CPPFLAGS)
endef
$(foreach V,$(strip $(LUA_APIS)),$(eval $(call LUA_CPPFLAGS_template,$(V))))


#
# I N S T A L L  P A T H S
#
prefix = /usr/local
includedir = $(prefix)/include
libdir = $(prefix)/lib
bindir = $(prefix)/bin

luacpath = $(libdir)/lua/$$(V)

# define lua51cpath, lua52cpath, etc
define LUACPATH_template
ifneq ($(strip $(luacpath)),)
lua$(subst .,,$(1))cpath := $(luacpath)
else
lua$(subst .,,$(1))cpath := $(or $$(call LUAPATH_FN,$(1),cdir),$$(libdir)/lua/$(1))
endif
endef
$(foreach V,$(strip $(LUA_APIS)),$(eval $(call LUACPATH_template,$(V))))


#
# B U I L D  R U L E S
#
hexdump: hexdump.c hexdump.h
	$(CC) -o $@ $< $(ALL_CFLAGS) -DHEXDUMP_MAIN $(ALL_CPPFLAGS)

libhexdump.so: hexdump.c hexdump.h
	$(CC) -o $@ $< $(ALL_CFLAGS) $(ALL_CPPFLAGS) $(ALL_SOFLAGS)

libhexdump.dylib: hexdump.c hexdump.h
	$(CC) -o $@ $< $(ALL_CFLAGS) $(ALL_CPPFLAGS) $(ALL_DYFLAGS)

$(DESTDIR)$(bindir)/hexdump: hexdump
	$(MKDIR) -p $(@D)
	$(CP) -fp $< $@

$(DESTDIR)$(libdir)/libhexdump.so: libhexdump.so
	$(MKDIR) -p $(@D)
	$(CP) -fp $< $@

$(DESTDIR)$(libdir)/libhexdump.dylib: libhexdump.dylib
	$(MKDIR) -p $(@D)
	$(CP) -fp $< $@

.PHONY: uninstall $(DESTDIR)$(bindir)/hexdump-uninstall \
        $(DESTDIR)$(libdir)/libhexdump.so-uninstall \
        $(DESTDIR)$(libdir)/libhexdump.dylib-uninstall

$(DESTDIR)$(bindir)/hexdump-uninstall \
$(DESTDIR)$(libdir)/libhexdump.so-uninstall \
$(DESTDIR)$(libdir)/libhexdump.dylib-uninstall:
	$(RM) $(@:%-uninstall=%)

.SECONDARY: all install

all: hexdump
install: $(DESTDIR)$(bindir)/hexdump
uninstall: $(DESTDIR)$(bindir)/hexdump-uninstall

ifeq ($(VENDOR_OS), Darwin)
all: libhexdump.dylib
install: $(DESTDIR)$(libdir)/libhexdump.dylib
uninstall: $(DESTDIR)$(libdir)/libhexdump.dylib-uninstall
else
all: libhexdump.so
install: $(DESTDIR)$(libdir)/libhexdump.so
uninstall: $(DESTDIR)$(libdir)/libhexdump.so-uninstall
endif


define LUALIB_BUILD

$(1)/hexdump.so: hexdump.c hexdump.h
	$$(MKDIR) -p $$(@D)
	$$(CC) -o $$@ $$< $$(ALL_CFLAGS) -DHEXDUMP_LUALIB $$(ALL_LUA$(subst .,,$(1))_CPPFLAGS) $$(ALL_SOFLAGS)

$$(DESTDIR)$$(lua$(subst .,,$(1))cpath)/hexdump.so: $(1)/hexdump.so
	$$(MKDIR) -p $$(@D)
	$$(CP) -fp $$< $$@

.SECONDARY: all$(1) install$(1)

all all$(1): $(1)/hexdump.so
install install$(1): $$(DESTDIR)$$(lua$(subst .,,$(1))cpath)/hexdump.so

.PHONY: uninstall$(1)

uninstall$(1):
	$$(RM) $$(lua$(subst .,,$(1))cpath)

uninstall: uninstall$(1)

endef # LUALIB_BUILD

$(foreach V,$(strip $(LUA_APIS)),$(eval $(call LUALIB_BUILD,$(V))))


#
# C L E A N  T A R G E T S
#
.PHONY: clean distclean clean~

distclean: clean
	$(RM) -f .config

clean:
	$(RM) -f hexdump
	$(RM) -fr 5.?/
	$(RM) -fr *.dSYM/

clean~: clean
	$(RM) -f *~
