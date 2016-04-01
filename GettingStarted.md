# Introduction #

This page provides some information to help you get up and running quickly.

The easiest way to install Pure is from a binary package. A complete collection of packages for various recent Ubuntu releases can be found on [Launchpad](https://launchpad.net/~dr-graef). Ryan Schmidt maintains an OSX port at the [MacPorts](http://www.macports.org/) project. Michel Salim's Fedora packages are available in the usual Fedora repositories, see [Koji](http://koji.fedoraproject.org/koji/packageinfo?packageID=9272) for details. Arch Linux packages for Pure and most addon modules by Björn Lindig, Deokjae Lee and Alastair Pharo are available [here](http://aur.archlinux.org/packages.php?ID=22893). For Windows we recommend our msi package, available at this site, which includes all needed third-party libraries, online documentation and a nice GUI frontend for editing and running Pure scripts. Pointers to other ports and packages can be found on the project page.

Compiling Pure from source isn't all that difficult either, once you have all the required tools and libraries installed. Most of these are readily available on Linux and other Unix-like systems.

## Contents ##
  * [Getting Help](#Getting_Help.md)
  * [Getting Pure](#Getting_Pure.md)
  * [Other Prerequisites](#Other_Prerequisites.md)
  * [Compiling LLVM](#Compiling_LLVM.md)
  * [Compiling Pure](#Compiling_Pure.md)
  * [Install the Online Documentation](#Install_the_Online_Documentation.md)
  * [Install Other Optional Bits and Pieces](#Install_Other_Optional_Bits_and_Pieces.md)
  * [Using Pure](#Using_Pure.md)

# Getting Help #

Head over to the [pure-lang](http://groups.google.com/group/pure-lang) Google group and sign up either with your your normal email address or your Google/GMail account. You can read messages and post to the pure-lang group either through the web-based interface or by sending mail to [pure-lang@googlegroups.com](mailto:pure-lang@googlegroups.com).

# Getting Pure #

To install Pure from source, you can either go with the released source tarballs (the latest release is always listed under "Featured Downloads" on the project page), or use the latest development sources in the source code [repository](http://code.google.com/p/pure-lang/source/checkout). Please note that the Pure project now uses the [Mercurial](http://mercurial.selenic.com/) DVCS to manage its source code, so you'll have to install the Mercurial software to get access to the repository.

Depending on the method you choose, **either** unpack the `pure-x.y.tar.gz` archive you downloaded (where `x.y` is the version number):

```
$ tar xfvz pure-x.y.tar.gz
```

**Or** fetch the latest development sources (project members should use `https:` instead of `http:` for write access to the central repository):

```
$ hg clone http://pure-lang.googlecode.com/hg pure-lang
```

The former command will put the source code in the `pure-x.y` subdirectory of the current directory. The latter gives you a clone of the full Pure source code repository in the `pure-lang` directory. (Please note that there are no partial checkouts of Mercurial repositories, so you'll have to get the whole shebang.) If you go the latter route, please note the following:

  * Besides the Pure interpreter itself, which can be found in the `pure-lang/pure` subdirectory, the repository also includes all "official" [addon modules](Addons.md) which are maintained as part of the Pure project.

  * Once you've cloned the repository, you can update it to the latest revision by just running `hg pull -u` in the `pure-lang` directory, please check the Mercurial documentation for details.

  * If you go with the development sources then you should be prepared to live on the bleeding edge, as the latest version in the repository might contain some experimental and/or untested features or not yet compile on all supported systems.

# Other Prerequisites #

To compile Pure, you'll need the following tools and libraries:

  * [GNU C/C++](http://gcc.gnu.org) and the corresponding libraries.

  * [GNU make](http://www.gnu.org/software/make).

  * [Flex](http://flex.sourceforge.net) and [Bison](http://www.gnu.org/software/bison). Normally these are only required when compiling from development sources. You'll need Flex 2.5.31 and Bison 2.3 or later.

  * The [GNU multiprecision](http://www.gnu.org/software/gmp) library, or some compatible replacement such as [MPIR](http://www.mpir.org).

  * The [GNU multiprecision floating point](http://www.mpfr.org/) library.

  * The [GNU readline](http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html) library, or some compatible replacement such as [BSD editline](http://www.thrysoee.dk/editline). This isn't a strict requirement, but without it you don't get command line editing in the interactive interpreter. We recommend GNU readline because it's easier to use and has full UTF-8 support, but in some situations BSD editline may be preferable for license reasons or because it's what the operating system provides.

  * [LLVM](http://llvm.org). This is the compiler backend Pure uses for its JIT compiler. You only need the llvm-2.x or 3.x tarball which contains the LLVM library as well as most of the LLVM toolchain. LLVM versions 2.5 thru 3.2 have all been tested extensively and are known to work with Pure. You can find all LLVM releases [here](http://llvm.org/releases/download.html).

All dependencies listed above should be readily available for most Linux and BSD systems, except maybe LLVM for which we give some instructions below.

# Compiling LLVM #

The LLVM installation instructions on llvm.org are somewhat convoluted, but fortunately you only need the LLVM base libraries to install Pure. We recommend the following basic configuration (use `./configure --help` to get an explanation of the configuration options):

```
$ ./configure --enable-optimized --enable-targets=host-only
```

Notes:

  * With LLVM 2.7 or later, you might wish to add `--enable-shared` to build LLVM as a shared library.

  * With LLVM 2.7 or earlier, you might have to add `--disable-assertions --disable-expensive-checks` to disable stuff that makes LLVM very slow and/or breaks it on some systems.

  * With LLVM 2.5 and earlier, `--enable-pic` is required on 64 bit systems, please check the [installation instructions](http://docs.pure-lang.googlecode.com/hg/install.html) for details.

Then just run the usual:

```
$ make
$ sudo make install
```

The last line installs LLVM on your system so that gcc finds it when building Pure.

# Compiling Pure #

Once you have all the necessary stuff installed, compiling Pure is quite easy. Chdir to the source directory and run the following commands:

```
$ ./configure --enable-release
$ make
```

(You can also leave out the `--enable-release` option to build a version with additional debugging information and code, but this will make the interpreter run substantially slower.)

Next, make sure that the interpreter works:

```
$ make check
```

If all is well, all checks should pass. If not, please mail the author or the [mailing list](http://groups.google.com/group/pure-lang) for help. Note that on MS Windows this step is expected to fail on some math-related tests in [test020.pure](http://pure-lang.googlecode.com/hg/pure/test/test020.pure) since some of the math routines are not fully POSIX-compatible; that's nothing to worry about.

If Pure fails _all_ tests, first check that LLVM has been installed properly and that it can be found by the system linker; some systems also require you to run a special utility like `ldconfig` for that purpose, see below. If your LLVM install looks ok but the tests still fail and the interpreter executable also segfaults immediately at startup, the most likely reason is a miscompiled LLVM. We know of some popular Linux distributions which have shipped such broken packages in the past. Try to compile LLVM yourself, as explained above. If that doesn't help either, your best bet is to get help from the [mailing list](http://groups.google.com/group/pure-lang).

Finally, to install Pure, do:

```
$ sudo make install
```

On some systems you'll also have to tell the dynamic linker to update its cache and/or set some environment variable so that the interpreter finds the Pure runtime library. E.g., on Linux this is done as follows:

```
$ sudo /sbin/ldconfig
```

More details, including an explanation of various configuration and build options can be found in the [installation instructions](http://docs.pure-lang.googlecode.com/hg/install.html). Make sure to also check the "System Notes" section in that file for known quirks and workarounds for different platforms.

# Install the Online Documentation #

This step is required to read the Pure documentation in the interpreter using the `help` command. If you have a working internet connection and the `wget` command, simply do the following after installing the interpreter:

```
$ sudo make install-docs
```

Instead, you can also download the [pure-docs](http://code.google.com/p/pure-lang/downloads/list?can=3&q=pure-docs) tarball and install it manually, please check the [installation instructions](http://docs.pure-lang.googlecode.com/hg/install.html) for details. Or you can read the documentation online, see [here](http://docs.pure-lang.googlecode.com/hg/index.html).

# Install Other Optional Bits and Pieces #

The Pure tarball contains a number of syntax highlighting files and programming modes for various popular text editors. Most of these aren't installed automatically, but you can grab them in the [etc](http://code.google.com/p/pure-lang/source/browse/pure/etc) directory in the sources and install them manually. Installation instructions are contained in the language files. (Note that pure-mode.el _will_ be installed automatically during `make install` if emacs was detected at configure time.)

If you got a clone of the source code repository then you might also want to build and install the addon modules. The following shell command can be used to quickly build all modules (this is to be run in the `pure-lang` directory you got by cloning the repository):

```
for x in pure-* *-pure; do
  test -f $x/Makefile && make -C $x "$@"
done
```

If you put this into a shell script (or define it as a shell function) `make-pure`, say, then you can run the command as `make-pure [target ...]`. E.g., just `make-pure` makes all modules, `make-pure clean` cleans them, etc. To make this work, you must have installed the Pure interpreter beforehand, as discussed above, and you also need to have the dependencies for each module installed, please check the module documentation in the corresponding README files or on the [Addons](Addons.md) page for details.

For the utmost Pure experience, you may want to install some additional 3rd party software:

  * An LLVM-capable C/C++ compiler such as [llvm-gcc](http://llvm.org/), [clang](http://clang.llvm.org/) or the [dragonegg](http://dragonegg.llvm.org/) gcc plugin is needed to take advantage of Pure's capability to inline C, C++ and Fortran code. Instructions for this can be found in the [installation instructions](http://docs.pure-lang.googlecode.com/hg/install.html).

  * Likewise, Pure's interface to Grame's functional signal processing language Faust needs the [Faust compiler](http://faust.grame.fr). To enjoy inlining of Faust code in Pure programs, you'll have to install the development version of Faust, Faust2, which is available in Faust's git repository; please check the [LLVM backend for Faust](http://www.grame.fr/~letz/faust_llvm.html) website for details.

  * For scientific applications you may want to get [Gnumeric](http://projects.gnome.org/gnumeric/), [Octave](http://www.octave.org/) and [Reduce](http://reduce-algebra.sourceforge.net/) which can be used with the [gnumeric-pure](Addons#gnumeric-pure.md), [pure-octave](Addons#pure-octave.md) and [pure-reduce](Addons#pure-reduce.md) modules. As of Pure 0.56, it's also possible to run Pure as a plugin in [GNU TeXmacs](http://www.texmacs.org/), please check the [TeXmacs](TeXmacs.md) wiki page for details.

  * The Tcl/Tk-GTK+ bridge [Gnocl](http://www.gnocl.org/) provides an easy way to program GTK+ applications in Pure, using Tcl code and/or the Glade GUI builder together with the gnocl module in the [pure-tk](Addons#pure-tk.md) package.

  * If you're into computer music and multimedia applications, you should get [Pd](http://puredata.info/) or one of its derivatives. Using the corresponding Pure [addon modules](Addons#pd-faust.md), this allows you to run Pure and Faust code in the graphical Pd environment.

Most of this software can be found in major Linux distributions; for Ubuntu we also provide packages for Faust and Gnocl on [Launchpad](https://launchpad.net/~dr-graef).

# Using Pure #

Run Pure interactively as follows:

```
$ pure
 __ \  |   |  __| _ \    Pure 0.56 (x86_64-unknown-linux-gnu)
 |   | |   | |    __/    Copyright (c) 2008-2012 by Albert Graef
 .__/ \__,_|_|  \___|    (Type 'help' for help, 'help copying'
_|                       for license information.)

Loaded prelude from /usr/local/lib/pure/prelude.pure.

> 6*7;
42
```

Read the online documentation:

```
> help
```

This invokes the Pure language and library documentation in an html browser, [w3m](http://w3m.sourceforge.net/) by default; you can change this with the `PURE_HELP` or the `BROWSER` environment variable. The Windows package also includes the complete documentation in html help format which can be read using the Windows help browser. You can also read the manual online [here](http://docs.pure-lang.googlecode.com/hg/index.html).

Exit the interpreter (you can also just type the end-of-file character at the
beginning of a line, i.e., `Ctrl-D` on Unix):

```
> quit
```

The [Using Pure](UsingPure.md) page has more information about using the Pure interpreter.