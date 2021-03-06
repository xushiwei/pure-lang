
name = pure-reduce
version = 0.2

# Platform-specific setup.

DLL         = $(shell pkg-config pure --variable DLL)
PIC         = $(shell pkg-config pure --variable PIC)
shared      = $(shell pkg-config pure --variable shared)

ifeq ($(DLL),.dll)
EXE = .exe
DIFF = diff --strip-trailing-cr
else
EXE =
DIFF = diff
endif

# Default installation directory. DESTDIR is supported for staging purposes to
# make package maintainers happy.

libdir      = $(shell pkg-config pure --variable libdir)
installdir  = $(addprefix $(DESTDIR), $(libdir)/pure)
bindir      = $(shell pkg-config pure --variable prefix)/bin
binstalldir = $(addprefix $(DESTDIR), $(bindir))

# Special flags and objects needed to build the reduce dll pilfered from
# reduce-algebra/csl/new-embedded/procedural/.

CFLAGS=-O2 -I. -DPAGE_BITS=19 -DHAVE_CONFIG_H=1 -DEMBEDDED=1

objects = arith01.o arith02.o arith03.o arith04.o arith05.o arith06.o arith07.o arith08.o arith09.o arith10.o arith11.o arith12.o bytes.o char.o driver.o embedcsl.o cslmpi.o cslread.o eval1.o eval2.o eval3.o eval4.o fasl.o fns1.o fns2.o fns3.o fwin.o gc.o preserve.o print.o restart.o sysfwin.o termed.o stubs.o

all: reduce.img reduce$(DLL)

reduce.img: reduce-algebra/csl/new-embedded/reduce.img
	cp $< .

# Use the reduce target to create a minimal Reduce executable if you want that
# to be installed as well.

ifneq ($(EXE),)
reduce: reduce$(EXE)
endif

reduce$(EXE): reduce-algebra/csl/new-embedded/reduce-image/reduce$(EXE) reduce.img
	cp $< .

reduce-algebra/csl/new-embedded/reduce.img reduce-algebra/csl/new-embedded/reduce-image/reduce$(EXE):
	$(MAKE) -C reduce-algebra/csl/new-embedded CFLAGS="$(CFLAGS)"

reduce$(DLL): $(addprefix reduce-algebra/csl/new-embedded/procedural/, $(objects)) proc-add.o
	$(CC) $(shared) $(PIC) -o reduce$(DLL) $^

$(addprefix reduce-algebra/csl/new-embedded/procedural/, $(objects)):
	$(MAKE) -C reduce-algebra/csl/new-embedded/procedural CFLAGS="$(CFLAGS) $(PIC)"

proc-add.o: proc-add.c
	$(CC) -Ireduce-algebra/csl/new-embedded/procedural $(CFLAGS) $(PIC) -c $< -o $@

logs: tests.log

tests.log: tests.pure
	pure -v < tests.pure | tail -n +2 > tests.log

check:
	pure -v < tests.pure | tail -n +2 | $(DIFF) tests.log -

clean:
	-$(MAKE) -C reduce-algebra/csl/new-embedded/procedural clean
	-$(MAKE) -C reduce-algebra/csl/new-embedded clean
	rm -f reduce.img reduce$(DLL) reduce$(EXE) proc-add.o

install:
	test -d $(installdir) || mkdir -p $(installdir)
	sed -e 's?/usr/lib?$(libdir)?' < reduce.pure > $(installdir)/reduce.pure
# If we got the Reduce image and library, install those as well.
	test -f reduce.img -a -f reduce$(DLL) && cp reduce.img reduce$(DLL) $(installdir) || true
# If we got the Reduce executable, install that, too.
	test -d $(binstalldir) || mkdir -p $(binstalldir)
	test -f reduce.img -a -f reduce$(EXE) && cp reduce.img reduce$(EXE) $(binstalldir) || true

uninstall:
	rm -f $(addprefix $(installdir)/, reduce.pure reduce$(DLL) reduce.img)
	rm -f $(addprefix $(binstalldir)/, reduce$(EXE) reduce.img)

# Roll a distribution tarball.

DISTFILES = COPYING README Makefile proc-add.c reduce.pure tests.pure tests.log examples.txt debian/* examples/*.pure reduce-files/*
SEDFILES = README

dist = $(name)-$(version)

date = $(shell date "+%B %-d, %Y")
datesubst = sed -e "s?@version@?$(version)?g" -e "s?|today|?$(date)?g" < $(1) > $(2)

dist: $(DISTFILES)
	rm -rf $(dist)
	mkdir $(dist) && mkdir $(dist)/debian && mkdir $(dist)/examples && mkdir $(dist)/reduce-files
	for x in $(DISTFILES); do ln -sf $$PWD/$$x $(dist)/$$x; done
	for x in $(SEDFILES); do rm -f $(dist)/$$x; $(call datesubst,$$PWD/$$x,$(dist)/$$x); done
	rm -f $(dist).tar.gz
	tar cfzh $(dist).tar.gz $(dist)
	rm -rf $(dist)

# Generate the documentation from the library sources.

sources = reduce.pure
target = pure-reduce

$(target).txt: README $(sources)
	cp $< $@
	pure-doc -is $(sources) >> $@
	cat examples.txt >> $@

# Debian package.

debsrc = $(shell echo $(dist) | sed -e 's/-$(version)/_$(version)/').orig.tar.gz

deb: $(debsrc) dist
	tar xfz $(dist).tar.gz
	cd $(dist) && debuild $(DEBUILD_FLAGS)
	rm -rf $(dist)

$(debsrc):
	wget -nv http://pure-lang.googlecode.com/files/$(dist).tar.gz -O $@
