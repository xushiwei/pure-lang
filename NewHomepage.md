# The Pure Programming Language #

<a href='http://llvm.org'><img src='http://wiki.pure-lang.googlecode.com/hg/DragonSmallText.png' align='right' border='0' /></a>

Pure is a modern-style functional programming language based on term rewriting. It offers equational definitions with pattern matching, full symbolic rewriting capabilities, dynamic typing, eager and lazy evaluation, lexical closures, built-in list and matrix support and an easy-to-use C interface. The interpreter uses [LLVM](http://llvm.org/) as a backend to JIT-compile Pure programs to fast native code.

Pure is the successor of the author's [Q](http://q-lang.sf.net/) language. It offers many new and powerful features and programs run much faster than
their Q equivalents.

A little code sample that prints the first 1000 Fibonacci numbers:

```
extern int puts(char*);
do (puts.str) (take 1000 (fibs 0L 1L)) with fibs a b = a : fibs b (a+b) & end;
```

You can execute this snippet by typing or pasting it at the Pure interpreter prompt. It uses the C '`puts`' function to do the printing. Note that if you remove the '`take 1000`', _all_ Fibonacci numbers will be printed (press '`Ctrl-C`' when you get bored). An explanation of this example can be found in the [Pure manual](http://docs.pure-lang.googlecode.com/hg/pure.html#lazy-evaluation-and-streams).

## Supported Systems ##

  * FreeBSD
  * GNU/Linux
  * Mac OS X
  * Windows

Porting to other POSIX platforms should be a piece of cake. In principle Pure should be able to run on any system which is supported by [LLVM](http://llvm.org/).

## License ##

[GNU Lesser General Public License V3](http://www.gnu.org/licenses/lgpl.html) or later. Please see the [README](http://pure-lang.googlecode.com/hg/pure/README) file for details.

## Documentation and Examples ##

  * Pure Language and Library Documentation ([html](http://docs.pure-lang.googlecode.com/hg/index.html), [pdf](http://docs.pure-lang.googlecode.com/hg/puredoc.pdf), [tarball](http://code.google.com/p/pure-lang/downloads/list?can=3&q=pure-docs))
  * Pure Quick Reference ([pdf](http://wiki.pure-lang.googlecode.com/hg/docs/pure-quickref/pure-quickref.pdf), [ebook edition](http://wiki.pure-lang.googlecode.com/hg/docs/pure-quickref/pure-quickref-a5.pdf))
  * [README](http://pure-lang.googlecode.com/hg/pure/README), [INSTALL](http://pure-lang.googlecode.com/hg/pure/INSTALL) and [NEWS](http://pure-lang.googlecode.com/hg/pure/NEWS) files

Selected wiki pages:

  * [Getting started](GettingStarted.md): a quick guide to installing and using the Pure interpreter
  * The [FAQ](FAQ.md): frequently asked questions with answers
  * [Programming examples](http://code.google.com/p/pure-lang/wiki/Examples): get an idea what Pure programs look like ([more examples](http://code.google.com/p/pure-lang/source/browse/pure/examples/))
  * [Addons](Addons.md): an overview of addon modules and libraries for Pure

## Mailing List ##

Pure development and use is discussed on the
[mailing list](http://groups.google.com/group/pure-lang).
Join us there to discuss Pure and ask whatever questions you have.

## Source Code ##

  * [Release tarballs](http://code.google.com/p/pure-lang/downloads)
  * [Mercurial repository](http://code.google.com/p/pure-lang/source/checkout)
  * [Snapshot tarballs](http://codex.sigpipe.cz/pure-lang/snapshots/)
  * [Old releases](http://sf.net/projects/pure-lang/files/pure/) at [SourceForge](http://sf.net/projects/pure-lang/)

## Packages and Ports ##

  * [Windows installers](http://code.google.com/p/pure-lang/downloads/list?can=3&q=msi) for Pure and several addons.
  * The [MacPorts](http://www.macports.org/) collection includes Ryan Schmidt's [Mac OS X port](http://trac.macports.org/browser/trunk/dports/lang/pure/Portfile) of Pure. Ports of many addon modules are also available.
  * The [Homebrew](http://github.com/mxcl/homebrew) package manager for Mac OS X also has a recipe a.k.a. [formula](http://github.com/mxcl/homebrew/blob/master/Library/Formula/pure.rb) by Michael Kohl for building Pure.
  * The [Packman](http://packman.links2linux.de/) project offers Toni Graffy's [rpm packages](http://packman.links2linux.de/package/pure) for SUSE Linux.
  * [Fedora](http://fedoraproject.org/) packages by Michel Salim are available in the usual repositories, see [here](http://koji.fedoraproject.org/koji/packageinfo?packageID=9272).
  * The [Gentoo](http://www.gentoo.org/) ebuild by Álvaro Castro Castilla is available [here](http://bugs.gentoo.org/show_bug.cgi?id=231966).
  * Arch Linux packages for Pure and various addon modules by Mikko Sysikaski and asitdepends are available [here](http://aur.archlinux.org/packages.php?ID=22893) and [here](http://aur.archlinux.org/packages.php?SeB=m&K=asitdepends).

## Pure on the Web ##

  * [pure-lang-extras](http://pure-lang-extras.googlecode.com/) provides additional libraries for Pure as well as a project template for Pure module developers, by Michael Maul.
  * [pure-vision](http://pure-vision.googlecode.com/) is an [OpenCV](http://sourceforge.net/projects/opencvlibrary/) module for Pure, by Kay-Uwe Kirstein.
  * [Interesting bits](http://www.bluishcoder.co.nz/tags/pure/) of Pure code including an Ogg Vorbis player at [Bluish Coder](http://www.bluishcoder.co.nz/) blog, by Chris Double.

Other interesting items such as programming modes and syntax highlighting files can be found on the [Grab Bag](GrabBag.md) page.

## Author ##

Albert Gräf <Dr.Graef at t-online.de><br />
Dept. of Computer Music<br />
Johannes Gutenberg University of Mainz (Germany)