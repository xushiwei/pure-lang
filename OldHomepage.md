# The Pure Programming Language #

<a href='http://llvm.org'><img src='http://wiki.pure-lang.googlecode.com/hg/DragonSmallText.png' align='right' border='0' /></a>

Pure is a modern-style functional programming language based on term rewriting. It offers equational definitions with pattern matching, full symbolic rewriting capabilities, dynamic typing, eager and lazy evaluation, lexical closures, built-in list and matrix support and an easy-to-use C interface. The interpreter uses [LLVM](http://llvm.org/) as a backend to JIT-compile Pure programs to fast native code. Pure is the successor of the author's [Q](http://q-lang.sf.net/) language. It offers many new and powerful features and programs run much faster than their Q equivalents.

Pure is free software. As of Pure 0.37, the runtime and the standard library are distributed under the [GNU Lesser General Public License V3](http://www.gnu.org/licenses/lgpl.html) to allow for commercial applications; please see the [README](http://pure-lang.googlecode.com/hg/pure/README) file for details. The interpreter is known to compile and run without hitches on Linux, OSX and Windows, porting to other POSIX platforms should be a piece of cake.

**Quick Links:** [Why Pure?](FAQ#Why_Pure?.md) [Documentation.](http://docs.pure-lang.googlecode.com/hg/puredoc.html) [Examples.](http://code.google.com/p/pure-lang/wiki/Examples) [Getting started.](GettingStarted.md) [Downloads](http://code.google.com/p/pure-lang/downloads) and [Mercurial repository.](http://code.google.com/p/pure-lang/source/checkout)

| Check out the latest sources: `hg clone https://pure-lang.googlecode.com/hg pure-lang` |
|:---------------------------------------------------------------------------------------|

## Pure on the Web ##

  * The [downloads](http://code.google.com/p/pure-lang/downloads). You can find the source releases there, as well as binary msi packages for MS Windows.
  * Old releases can still be found at Pure's [SourceForge](http://sourceforge.net/projects/pure-lang/) page.
  * The [Mercurial source repository](http://code.google.com/p/pure-lang/source/checkout). (The current development sources of the Pure interpreter are available [here](http://code.google.com/p/pure-lang/source/browse/pure/).)
  * Current [snapshots](http://codex.sigpipe.cz/pure-lang/snapshots/) of the source repository are maintained off-site by Roman Neuhauser. Thanks Roman!
  * The [pure-lang](http://groups.google.com/group/pure-lang) mailing list. Join us there to discuss Pure and ask whatever questions you have.
  * The [wiki](http://code.google.com/p/pure-lang/w). Not much there yet, but you can help with that. ;-)
  * The [wikipedia entry](http://en.wikipedia.org/wiki/Pure_(programming_language)). This is little more than a stub right now, so you can help by expanding that one, too.

### Packages and Ports ###

  * The [MacPorts](http://www.macports.org/) collection includes Ryan Schmidt's [OSX port](http://trac.macports.org/browser/trunk/dports/lang/pure/Portfile) of Pure. Ports of many addon modules are also available.
  * The [Homebrew](http://github.com/mxcl/homebrew) package manager for OSX also has a recipe a.k.a. [formula](http://github.com/mxcl/homebrew/blob/master/Library/Formula/pure.rb) by Michael Kohl for building Pure.
  * The [Packman](http://packman.links2linux.de/) project offers Toni Graffy's [rpm packages](http://packman.links2linux.de/package/pure) for SUSE Linux.
  * [Fedora](http://fedoraproject.org/) packages by Michel Salim are available in the usual repositories, see [here](http://koji.fedoraproject.org/koji/packageinfo?packageID=9272).
  * The [Gentoo](http://www.gentoo.org/) ebuild by Álvaro Castro Castilla is available [here](http://bugs.gentoo.org/show_bug.cgi?id=231966).
  * Arch Linux packages for Pure and various addon modules by Mikko Sysikaski and asitdepends are available [here](http://aur.archlinux.org/packages.php?ID=22893) and [here](http://aur.archlinux.org/packages.php?SeB=m&K=asitdepends).

### 3rd Party Projects ###

  * Michael Maul provides additional libraries for Pure as well as a project template for Pure module developers at his [pure-lang-extras](http://pure-lang-extras.googlecode.com/) project.
  * Kay-Uwe Kirstein's [OpenCV](http://sourceforge.net/projects/opencvlibrary/) module for Pure is available at his [pure-vision](http://pure-vision.googlecode.com/) project.
  * Chris Double has some [interesting bits](http://www.bluishcoder.co.nz/tags/pure/) of Pure code including an Ogg Vorbis player at his [Bluish Coder](http://www.bluishcoder.co.nz/) blog.

## Documentation and Examples ##

  * [Getting started](GettingStarted.md). A quick guide to installing and using the Pure interpreter.
  * The [FAQ](FAQ.md). Frequently asked questions with answers.
  * The **Pure Language and Library Documentation** in [html](http://docs.pure-lang.googlecode.com/hg/puredoc.html) and [pdf](http://docs.pure-lang.googlecode.com/hg/puredoc.pdf) formats.
  * The [README](http://pure-lang.googlecode.com/hg/pure/README), [INSTALL](http://pure-lang.googlecode.com/hg/pure/INSTALL) and [NEWS](http://pure-lang.googlecode.com/hg/pure/NEWS) files.
  * Have a look at some [programming examples](http://code.google.com/p/pure-lang/wiki/Examples) to get an idea how Pure programs look like. [More examples](http://code.google.com/p/pure-lang/source/browse/pure/examples/).
  * [Addons](Addons.md). An overview of addon modules and libraries for Pure.

## The Grab Bag ##

<a href='http://wiki.pure-lang.googlecode.com/hg/pure-pics.tar.gz'><img src='http://wiki.pure-lang.googlecode.com/hg/waterdrop.png' align='right' /></a>

  * Some auxiliary documentation is available, which is work in progress and often lags behind the current release quite a bit: [The Pure Programming Language](http://wiki.pure-lang.googlecode.com/hg/docs/pure-intro/pure-intro.pdf) (TPPL), the [Pure Quick Reference](http://wiki.pure-lang.googlecode.com/hg/docs/pure-quickref/pure-quickref.pdf) (PQR; this is an abridged version of TPPL for the impatient, an [ebook edition](http://wiki.pure-lang.googlecode.com/hg/docs/pure-quickref/pure-quickref-a5.pdf) of the PQR is also available), and [Pure Syntax](http://wiki.pure-lang.googlecode.com/hg/docs/pure-syntax/pure-syntax.pdf) (this is provided as a supplement for TPPL and PQR for those who prefer syntax diagrams over EBNF).
  * Eddie Rucker's "[pure drop of water](http://wiki.pure-lang.googlecode.com/hg/pure-pics.tar.gz)" images, freely available for use e.g. as desktop icons or website images (thanks, Eddie!).
  * Adam Sanderson has created a [TextMate bundle](http://endofline.wordpress.com/2009/09/03/textmate-bundle-for-pure/) for Pure, which provides syntax highlighting, some useful completions, and documentation look up.
  * asitdepends has implemented a [Pure mode](http://sourceforge.net/tracker/index.php?func=detail&aid=2793008&group_id=588&atid=300588) for [jEdit](http://www.jedit.org/), which provides syntax highlighting and completion for keywords.
  * If you're still using LLVM 2.3, you'll need Cyrille Berger's [patch](http://wiki.pure-lang.googlecode.com/hg/X86JITInfo.cpp.pic.2.3.patch) to add PIC support on x86-64 systems. (This isn't required for LLVM 2.4 and later any more.)
  * If you want to build Pure on Windows, you might wish to have a look at Jiri Spitz' [instructions](http://wiki.pure-lang.googlecode.com/hg/INSTALL-MinGW.txt) for compiling LLVM and Pure with [mingw](http://mingw.org/). For your convenience, we've also put together a little [package](http://pure-lang.googlecode.com/files/pure-mingw.zip) with required mingw libraries, dlls and headers. (Please note that there's also an msi package available for Windows users, so you only need this stuff if you want/need to compile Pure on Windows yourself.)

## Author ##

Albert Gräf <Dr.Graef at t-online.de><br />
Dept. of Computer Music<br />
Johannes Gutenberg University of Mainz (Germany)