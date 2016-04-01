# The Pure Programming Language #

<a href='http://pure-lang.googlecode.com/'><img src='http://wiki.pure-lang.googlecode.com/hg/waterdrop.png' align='right' border='0' /></a>

Pure is a modern-style functional programming language based on term rewriting. It offers equational definitions with pattern matching, full symbolic rewriting capabilities, dynamic typing, eager and lazy evaluation, lexical closures, built-in list and matrix support and an easy-to-use C interface. The interpreter uses [LLVM](http://llvm.org/) as a backend to JIT-compile Pure programs to fast native code.

Pure is the successor of the author's [Q](http://q-lang.sf.net/) language. It offers many new and powerful features and programs run much faster than
their Q equivalents. It also integrates nicely with a number of other computing environments, most notably [Faust](Faust.md), PureData, [Octave](Octave.md), [Reduce](Reduce.md) and TeXmacs.

A little code sample that prints the first 1000 Fibonacci numbers:

```
extern int puts(char*);
do (puts.str) (take 1000 (fibs 0L 1L)) with fibs a b = a : fibs b (a+b) & end;
```

You can execute this snippet by typing or pasting it at the Pure interpreter prompt. It uses the C '`puts`' function to do the printing. Note that if you remove the '`take 1000`', _all_ Fibonacci numbers will be printed (press '`Ctrl-C`' when you get bored). An explanation of this example can be found in the [Pure manual](http://docs.pure-lang.googlecode.com/hg/pure.html#lazy-evaluation-and-streams). More examples can be found [here](http://docs.pure-lang.googlecode.com/hg/pure.html#examples). (This points to the Examples section of the Pure manual which  quickly gives you an idea how Pure programs look like.)

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

> This is the full documentation with all the gory details (currently at 900+ pages). You can also read this as online documentation with the `help` command in the interpreter. This is intended primarily as a reference manual, although you may want to at least skim through [The Pure Manual](http://docs.pure-lang.googlecode.com/hg/pure.html) to pick up the language. In particular, the [Examples](http://docs.pure-lang.googlecode.com/hg/pure.html#examples) section of the Pure Manual can be used as a Pure tutorial.

  * Pure Quick Reference ([pdf](http://wiki.pure-lang.googlecode.com/hg/docs/pure-quickref/pure-quickref.pdf))

> This is a quick guide to Pure for the impatient (80+ pages) which explains the most important parts of the language. Includes examples, references, index and an appendix with the Pure grammar and a brief introduction to term rewriting. This is what you should read first if the amount of information in the reference documentation seems overwhelming.

  * The Pure TeXmacs Plugin ([pdf](http://wiki.pure-lang.googlecode.com/hg/docs/pure-texmacs.en.pdf))

> A guide to using Pure with [Octave](Octave.md), [Reduce](Reduce.md) and [TeXmacs](TeXmacs.md). Read this if you'd like to use Pure as a scientific computing environment, or if you just want to use TeXmacs as a convenient frontend to Pure.

  * [README](http://pure-lang.googlecode.com/hg/pure/README), [INSTALL](http://pure-lang.googlecode.com/hg/pure/INSTALL) and [NEWS](http://pure-lang.googlecode.com/hg/pure/NEWS) files

Selected wiki pages:

  * [Getting started](GettingStarted.md): a quick guide to installing and using the Pure interpreter
  * The [FAQ](FAQ.md): frequently asked questions with answers
  * [Programming examples](Examples.md): some typical code examples from the distribution ([more examples](http://code.google.com/p/pure-lang/source/browse/pure/examples/))
  * [Addons](Addons.md): an overview of addon modules and libraries for Pure

## Mailing List and Chat ##

Pure development and use is discussed on the
[mailing list](http://groups.google.com/group/pure-lang).
Join us there to discuss Pure and ask whatever questions you have.

There's also a [#pure-lang](irc://irc.freenode.net/pure-lang) IRC channel on [freenode](http://freenode.net/) where Pure developers and users hang out, which is archived [here](https://yanovich.net/pure-lang/) (thanks, yano!). To use this, point your IRC client at irc.freenode.net, or else use the page at http://webchat.freenode.net from your browser.

## Source Code ##

  * [Release tarballs](http://code.google.com/p/pure-lang/downloads)
  * [Mercurial repository](http://code.google.com/p/pure-lang/source/checkout)
  * [Snapshot tarballs](http://codex.sigpipe.cz/pure-lang/snapshots/)
  * [Old releases](http://sf.net/projects/pure-lang/files/pure/) at [SourceForge](http://sf.net/projects/pure-lang/)

## Packages and Ports ##

  * [Windows installers](http://code.google.com/p/pure-lang/downloads/list?can=3&q=msi) for Pure and several addons.
  * The [MacPorts](http://www.macports.org/) collection includes Ryan Schmidt's [Mac OS X port](http://trac.macports.org/browser/trunk/dports/lang/pure/Portfile) of Pure. Ports of many addon modules are also available.
  * The [Homebrew](http://github.com/mxcl/homebrew) package manager for Mac OS X also has a recipe a.k.a. [formula](http://github.com/mxcl/homebrew/blob/master/Library/Formula/pure.rb) by Michael Kohl for building Pure.
  * The [Packman](http://packman.links2linux.de/) project used to offer Toni Graffy's [rpm packages](http://packman.links2linux.de/package/pure) for openSUSE Linux. (Unfortunately, while the Pure project is still listed there, the packages haven't been updated for a long time.)
  * [Fedora](http://fedoraproject.org/) packages by Michel Salim are available in the usual repositories, see [here](http://koji.fedoraproject.org/koji/packageinfo?packageID=9272). Additional packages for Fedora users by Fernando Lopez-Lezcano can be found in the [Planet CCRMA](http://ccrma.stanford.edu/planetccrma/software/) repositories.
  * The [Gentoo](http://www.gentoo.org/) ebuild by Álvaro Castro Castilla is available [here](http://bugs.gentoo.org/show_bug.cgi?id=231966).
  * A fairly complete set of Pure packages by Björn Lindig, Deokjae Lee and Alastair Pharo for [Arch Linux](http://www.archlinux.org/) is available [here](http://aur.archlinux.org/packages.php?ID=22893).
  * A [FreeBSD](http://www.freebsd.org) port by Zhihao Yuan is available [here](http://www.freshports.org/lang/pure/).
  * Packages for various [Ubuntu](http://www.ubuntu.com/) releases are available on [Launchpad](https://launchpad.net/~dr-graef). There are several PPAs, one with the interpreter and all addon modules, and a few others with packages of 3rd party software which can be used with Pure, such as Faust and Gnocl.

## Pure on the Web ##

  * [pure-lang-extras](http://pure-lang-extras.googlecode.com/) provides additional libraries for Pure as well as a project template for Pure module developers, by Michael Maul.
  * [pure-vision](http://pure-vision.googlecode.com/) is an [OpenCV](http://sourceforge.net/projects/opencvlibrary/) module for Pure, by Kay-Uwe Kirstein.
  * [Interesting bits](http://www.bluishcoder.co.nz/tags/pure/) of Pure code including an Ogg Vorbis player at [Bluish Coder](http://www.bluishcoder.co.nz/) blog, by Chris Double.
  * Other [interesting bits](https://github.com/dubiousjim/unspoiled) of Pure code by dubiousjim, such as an implementation of 2-3 trees and weak references and dictionaries.
  * An [interview](http://blueparen.com/node/6) with the creator of Pure conducted by Nick Mudge can be found on his [blueparen](http://blueparen.com/) website.

Other interesting items such as programming modes and syntax highlighting files can be found on the [Grab Bag](GrabBag.md) page.

## Author ##

Albert Gräf <Dr.Graef at t-online.de>,
Dept. of Computer Music,
Johannes Gutenberg University of Mainz (Germany)