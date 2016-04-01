# Introduction #

This page describes, how to install and run Pure on a WindowsXP machine (currently there are hardly experiences with Vista and Win7). Already known issues are named and if possible solution are given.


# Installing Pure #

The easiest way to get started with Pure on Windows is by using the "one-click" installer (pure-xx.msi), provided in the download area.

As installation directory a path **without** spaces should be chosen, especially, when Pure is used together with other command line tools (see sections [Using Batch-Compiler](PureOnMSWindows#Using_Batch-Compiler.md) and [Using MSYS](PureOnMSWindows#Using_MSYS.md)).


# Running Pure #

The same features and command-line options as for the other platforms are valid.

The following issues are already known:

  * GNU readline bug for non-US keyboards:
> There is a bug in the readline library (see also [this discussion thread](http://groups.google.com/group/pure-lang/browse_thread/thread/b3950568f10819a7#)), used by the Pure interpreter, which sometimes makes alternate-keys on non-US keyboards unusable. If you are observing problems in generating special keys, like `[]{}\~`, you might need to disable readline by calling pure with the `--noediting` option:
```
 > pure --noediting
```

> Alternatively, readline can also be disabled during compilation of Pure.

  * interactive help command:
> The default installation of Pure is looking for the `w3m` webbrowser to show the html documentation, which usually is not part of a typical Windows setup. You can easily switch to another webbrowser just by setting the `PURE_HELP` environment variable. You might need to suround the path to the webbrowser by "" (no escaping like \" is necessary), if the path includes special characters like spaces. It can easily be checked by calling the set command in cmd.exe (the Windows command shell):
```
 > set PURE_HELP
 PURE_HELP="C:\Program Files\Mozilla Firefox\firefox.exe"
```

## Using the Batch Compiler ##

If the batch-compiler of Pure `pure -c` additional tools are needed:

  1. MinGW's C-compiler (gcc & g++)
  1. LLVM toolchain

For windows both tools are available as a binary distribution, so there is no need for compiling them in the first place:

### Installing MinGW ###

The one-click windows installer works fine, but spaces in the installation path should be avoided. The current binary release (MinGW-5.1.6) still has some old versions of the included tools, e.g. gcc-3.5.4, and also some include files are outdated, but it works fine with Pure, so no further manuals updates are needed. Manual update of the MinGW-environment is usually done by downloading and extracting the respective packages from the MinGW-download site (where also the binary distribution can be found): http://sourceforge.net/projects/mingw/files/

**Note:** The one-click installer also adds an update function, which usually overrides any manually added tools, header-files, etc.


### Installing LLVM ###

A binary distribution is available as a zip file. Simply extract the binaries to a path (without spaces, again) and add it to the search path. Binaries for Windows can be found [here](http://llvm.org/releases/) (choose Mingw32/x86 as architecture).

# Compiling Pure #

Compilation of Pure is only needed if different compilation options from the msi-version are desired, e.g. no readline support for the interpreter, or if Pure has to be installed into the MSYS environment (see [below](PureOnMSWindows#Using_MSYS.md)).
In this case, you might wish to have a look at Jiri Spitz' [instructions](http://wiki.pure-lang.googlecode.com/hg/INSTALL-MinGW.txt). For your convenience, there's also a little [package](http://pure-lang.googlecode.com/files/pure-mingw2.zip) with required mingw libraries, dlls and headers.

# Using MSYS #

MSYS is a kind of "add-on" to MinGW, which provides a Unix- (and Linux-) style environment (sh-tool with a wrapper to map Unix-style path names to the Windows world). Installing instructions and the correct setup of paths, tools etc. is also described [here](http://wiki.pure-lang.googlecode.com/hg/INSTALL-MinGW.txt).

Known issues:

  * Matching pathnames for pure-gen:

> MSYS mounts the Windows drive letters, e.g. `C:`, `D:`, as root dirs `/c` or `/d`, so one can access `C:\MinGW` also by `/c/MinGW` or `/C/MinGW` (**MSYS is case-sensitive incontrast to Windows, where `C:\mingw` would also work**). Please be aware that internally MSYS converts `/c/MinGW` back to `c:/MinGW`. This can lead to undesired effects for instance when using pure-gen with a pattern-matching option for the target symbols:
```
pure-gen -s '/c/myproject/myinclude.h:' /c/myproject/myinclude.h
```
> Here the path `/c/myproject/myinclude.h` is internally converted to `c:/myproject/myinclude.h` and does not match the given pattern anymore. The result would be an empty pure- and c-wrapper file. Better stick with the _original_ path style of windows:
```
pure-gen -s 'c:/myproject/myinclude.h:' c:/myproject/myinclude.h
```
> The drawback is that the colon in `c:/` sometimes raise further trouble when dealing with other Unix tools...

  * Fixing _install_ target for "Linux" Makefiles:

> Makefiles for the Pure addons often contain a section to guess the installation path:
```
# Try to guess the installation prefix (this needs GNU make):
prefix = $(patsubst %/bin/pure,%,$(shell which pure 2>/dev/null))

ifeq ($(strip $(prefix)),)
# Fall back to /usr/local.
prefix = /usr/local
endif
```
> This fails when using MSYS together with the MSI-package of Pure, which does not have the usual (Unix/Linux)-style path structure, e.g. _prefix_/bin/pure. This can be fixed by calling `make prefix=_whereverPureIsLocated_`. A more convient method is to add a clause covering this case to the Makefile:
```
# Try to guess the host system type.
host = $(shell ./config.guess)

# Try to guess the installation prefix (this needs GNU make):
prefix = $(patsubst %/bin/pure,%,$(shell which pure 2>/dev/null))

ifeq ($(strip $(prefix)),)
ifneq "$(findstring -mingw,$(host))" ""
# Windows: there might be an MSI installation:
prefix = $(patsubst %/pure,%,$(shell which pure 2>/dev/null))
else
# Fall back to /usr/local.
prefix = /usr/local
endif
endif
```
> Of course, here `host` must be defined beforehand.