To turn Pure into a practical programming tool, we strive to provide a collection of useful addon modules and interfaces to third-party libraries covering various important programming areas, such as GUI, graphics, multimedia, databases, scientific programming, computer algebra etc. Suggestions and contributions are welcome. This page lists the addon modules which are already available or under development. Most of the addons are distributed under a commercial-friendly LGPL or BSD-style license. Please refer to the documentation in each package for copyright/license notes and installation instructions. Some additional libraries for Pure can be found at Michael Maul's [pure-lang-extras](http://pure-lang-extras.googlecode.com/) and Kay-Uwe Kirstein's [pure-vision](http://pure-vision.googlecode.com/) project.

To use any of these, you also need the Pure interpreter, see the _Featured Downloads_ on the frontpage. A Windows package of the interpreter in msi format is also available, this already includes some of the addon modules listed here. Some addons are also packaged separately for Windows, see the download links below. Ubuntu packages of the interpreter and all addons are available on [Launchpad](https://launchpad.net/~dr-graef). Most if not all of the addons are also available as OSX ports from [MacPorts](http://www.macports.org/) (by Ryan Schmidt). Please also check the frontpage for other distributions with binary Pure packages.



A classification of the available modules according to topic can be found in the [documentation](http://docs.pure-lang.googlecode.com/hg/index.html).

# Utilities #

Some utility programs written with or for Pure.

## pure-doc ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

pure-doc is a simple source documentation and literate programming utility for Pure. It is designed to be used with the excellent [docutils](http://docutils.sourceforge.net) tools and the gentle [RST](http://docutils.sourceforge.net/rst.html) markup format supported by these.

The basic idea is that you just comment your code as usual, but using RST markup instead of plain text. In addition, you can also designate literate programming fragments in your code, which will be translated to RST literal blocks automatically. You then run pure-doc on your source files to extract all marked up comments and the literate code blocks. The resulting RST source can then be processed with the docutils utilities like rst2html.py and rst2latex.py to create the documentation in a variety of formats.

Documentation is available in [html](http://docs.pure-lang.googlecode.com/hg/pure-doc.html) format. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pure-doc).

**Download:** [pure-doc-0.6.tar.gz](http://pure-lang.googlecode.com/files/pure-doc-0.6.tar.gz)

## pure-gen ##

**Authors:** Albert Graef <Dr.Graef at t-online.de>, Scott E. Dillard

pure-gen is a C interface generator for the Pure language. It takes a C header file as input and generates a corresponding Pure module with the constant definitions and extern declarations needed to use the C module from Pure. pure-gen can also generate FFI interfaces rather than externs (using the pure-ffi module, see below), and it can optionally create a C wrapper module which allows you to create interfaces to pretty much any code which can be called via C. Interfaces to C++ can be made using SWIG's new [C language module](http://swig.svn.sourceforge.net/viewvc/swig/branches/gsoc2008-maciekd/Doc/Manual/C.html) which can wrap arbitrary C++ libraries in C.

pure-gen itself is written in Pure, but it uses a [C parser](http://www.sivity.net/projects/language.c) written in [Haskell](http://www.haskell.org). A manual page is included, you can also read this online in [html](http://docs.pure-lang.googlecode.com/hg/pure-gen.html) format. pure-gen is distributed under a BSD-style license, please see the [COPYING](http://pure-lang.googlecode.com/hg/pure-gen/COPYING) file for details. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pure-gen).

**Download:** [pure-gen-0.15.tar.gz](http://pure-lang.googlecode.com/files/pure-gen-0.15.tar.gz)

## faust2pd ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

faust2pd is a little utility to generate "graph-on-parent" GUI abstractions from Faust programs which make it easier to use Faust signal processing units with Miller Puckette's [PureData](http://puredata.info/) (Pd). [Faust](http://faust.grame.fr/) is Yann Orlarey's functional language for real-time audio signal processing. (Pure also provides a direct Faust interface, see the [manual](http://docs.pure-lang.googlecode.com/hg/pure.html#interfacing-to-faust) for details.) The Pure faust2pd script is an enhanced version of an [earlier Q script](http://q-lang.sourceforge.net/examples.html#Pd).

The documentation is available online in [html](http://docs.pure-lang.googlecode.com/hg/faust2pd.html) format. faust2pd is distributed under GPLv3 (or later), please see the [COPYING](http://pure-lang.googlecode.com/hg/faust2pd/COPYING) file for details. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/faust2pd).

**Download:** [faust2pd-2.5.tar.gz](http://pure-lang.googlecode.com/files/faust2pd-2.5.tar.gz)<br />

# Plugins #

These plugins let you use Pure as an embedded scripting language in other environments.

## gnumeric-pure ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

<a href='http://wiki.pure-lang.googlecode.com/hg/pure-gnumeric.png'><img src='http://wiki.pure-lang.googlecode.com/hg/pure-gnumeric-mini.png' align='right' /></a>

This package provides a Pure plugin for [Gnumeric](http://projects.gnome.org/gnumeric/), the Gnome spreadsheet. The plugin allows you to extend Gnumeric with functions written in Pure, and also lets you use Gnumeric as a spreadsheet frontend to Pure. All Gnumeric data types are supported, as well as calling Gnumeric functions from Pure code and setting up asynchronous Pure data sources which are executed as background tasks. A function to render OpenGL scenes in Gnumeric is also provided.

Documentation is available in [html](http://docs.pure-lang.googlecode.com/hg/gnumeric-pure.html) format. Please also see the [pure\_func.pure](http://pure-lang.googlecode.com/hg/gnumeric-pure/pure-func/pure_func.pure) and [pure\_glfunc.pure](http://pure-lang.googlecode.com/hg/gnumeric-pure/pure-glfunc/pure_glfunc.pure) scripts for some examples. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/gnumeric-pure). Sample spreadsheets can be found in the [examples](http://code.google.com/p/pure-lang/source/browse/gnumeric-pure/examples) folder.

**Download:** [gnumeric-pure-0.10.tar.gz](http://pure-lang.googlecode.com/files/gnumeric-pure-0.10.tar.gz)


## pd-faust ##

<a href='http://wiki.pure-lang.googlecode.com/hg/pd-faust.png'><img src='http://wiki.pure-lang.googlecode.com/hg/pd-faust-mini.png' align='right' /></a>

**Author:** Albert Graef <Dr.Graef at t-online.de>

pd-faust is a collection of [Pd](http://puredata.info/) objects written using [pd-pure](#pd-pure.md) which lets you run [Faust](http://faust.grame.fr/) programs inside Pd. It combines the functionality of [pure-faust](#pure-faust.md) and [faust2pd](#faust2pd.md) in a single package, and also features dynamic reloading of Faust plugins, automatic generation of controller GUIs and MIDI/OSC controller mappings, as well as OSC-based controller automation. It thus provides a complete solution for interactively developing Faust dsps in Pd.

pd-faust requires the [pd-pure](#pd-pure.md) and [pure-faust](#pure-faust.md) packages. The documentation is available online in [html](http://docs.pure-lang.googlecode.com/hg/pd-faust.html) format. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pd-faust).

**Download:** [pd-faust-0.4.tar.gz](http://pure-lang.googlecode.com/files/pd-faust-0.4.tar.gz)

## pd-pure ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

Pd a.k.a. [PureData](http://puredata.info/) is Miller Puckette's computer music and multimedia software which enables you to create interactive realtime applications in a convenient graphical programming environment. pd-pure is a Pure "loader" plugin for Pd, which allows you to write Pd control and audio objects in Pure. If you're into graphical/visual programming then this module provides you with an interesting way to run Pure in Pd by connecting Pure functions and other Pd objects to do complex signal processing tasks in realtime.

The documentation is available online in [html](http://docs.pure-lang.googlecode.com/hg/pd-pure.html) format. Like Pd itself, pd-pure is distributed under a BSD-style license, please see the [COPYING](http://pure-lang.googlecode.com/hg/pd-pure/COPYING) file for details. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pd-pure).

**Download:** [pd-pure-0.16.tar.gz](http://pure-lang.googlecode.com/files/pd-pure-0.16.tar.gz)<br />
**Windows package:** [pd-pure-0.15.msi](http://pure-lang.googlecode.com/files/pd-pure-0.15.msi) (for use with Miller's "vanilla" Pd 0.43 Windows package available [here](http://crca.ucsd.edu/~msp/software.html))

# Library Modules #

The following library modules can be used in Pure programs. They mostly provide interfaces to various useful third party libraries and/or data formats.

## pure-audio ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

pure-audio is a digital audio interface for the Pure programming language. It currently includes wrappers for [PortAudio](http://www.portaudio.com/), [FFTW3](http://www.fftw.org/), [libsndfile](http://www.mega-nerd.com/libsndfile/) and [libsamplerate](http://www.mega-nerd.com/SRC/), as well as a `realtime` module which gives Pure scripts access to realtime scheduling on systems which have a pthreads library with the POSIX realtime threads extension.

See the [README](http://pure-lang.googlecode.com/hg/pure-audio/README) file for more information. pure-audio is distributed under a BSD-style license, please see the [COPYING](http://pure-lang.googlecode.com/hg/pure-audio/COPYING) file for details. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pure-audio).

**Download:** [pure-audio-0.4.tar.gz](http://pure-lang.googlecode.com/files/pure-audio-0.4.tar.gz)

**Windows package:** [pure-audio-0.4.1.msi](http://pure-lang.googlecode.com/files/pure-audio-0.4.1.msi)

## pure-csv ##

**Author:** Eddie Rucker <erucker at bmc.edu>

[pure-csv](http://code.google.com/p/pure-lang/source/browse/pure-csv/) is a module for reading and writing Comma Separated Value (CSV) files from within Pure. Please make sure to read the [README](http://pure-lang.googlecode.com/hg/pure-csv/README) file in the package for installation instructions and take a peek in the examples sub-directory for module usage.

The module is based on the q-csv module which is loosely based on
[Python's CSV module](http://docs.python.org/lib/module-csv.html). The module accepts quotes and field delimiters of more than one character as a means of handling Unicode. Currently, pure-csv has three built in dialects, [RFC4180](http://www.ietf.org/rfc/rfc4180.txt) standard, "default" (RFC4180 without '\r's), and "EXCEL" but provides a simple user interface for creating new dialects. Examples are provided with the accompanying documentation. Since the reading, writing, and conversion routines are written in C, operations are efficient.

Documentation is available in [html](http://wiki.pure-lang.googlecode.com/hg/docs/pure-csv.html) and [pdf](http://wiki.pure-lang.googlecode.com/hg/docs/pure-csv.pdf) format. pure-csv is distributed under a BSD-style license, please see the [COPYING](http://pure-lang.googlecode.com/hg/pure-csv/COPYING) file for details.

Note that the routines may be abused for many purposes other than just importing and exporting data files. For example, they are useful for configuration and temporary data files.

Please send bug reports or questions to pure-lang@googlegroups.com.

**Download:** [pure-csv-1.5.tar.gz](http://pure-lang.googlecode.com/files/pure-csv-1.5.tar.gz)

## pure-fastcgi ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

A [FastCGI](http://www.fastcgi.com/) interface, which is useful for developing server-side web applications with Pure. Compared to normal CGI scripts, this has the advantage that the script keeps running and may process as many requests from the web server as you like, in order to reduce startup times and enable caching techniques. Most ordinary CGI scripts can be converted to use FastCGI with minimal changes. Your server needs a FastCGI module to make this work; such modules are readily available for most popular web servers.

Documentation is available [here](http://docs.pure-lang.googlecode.com/hg/pure-fastcgi.html). The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-fastcgi).

**Download:** [pure-fastcgi-0.5.tar.gz](http://pure-lang.googlecode.com/files/pure-fastcgi-0.5.tar.gz)

## pure-faust ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

pure-faust provides an interface to Yann Orlarey's [Faust](http://faust.grame.fr/) programming language, so that you can load and run Faust-generated signal processing modules in Pure. Faust is a functional programming language for real-time sound processing and synthesis developed at [Grame](http://www.grame.fr/) and distributed as GPL'ed software. With Faust you can develop signal processing algorithms in a high-level functional language, but still achieve a performance comparable to carefully handcrafted C routines.

Please note that recent Pure versions also provide a [direct Faust interface](http://docs.pure-lang.googlecode.com/hg/pure.html#interfacing-to-faust) which includes the ability to inline Faust code in Pure programs. The built-in interface requires faust2, the latest Faust incarnation from Faust's git repository, which is still under development. pure-faust can be run on top of the built-in Faust interface, but also continues to work with mainline Faust. It provides an alternative interface which is tailored for applications which load Faust dsps dynamically at runtime.

This is a port of an earlier Q module. The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-faust). Preliminary documentation is available online in [html](http://docs.pure-lang.googlecode.com/hg/pure-faust.html) format.

**Download:** [pure-faust-0.8.tar.gz](http://pure-lang.googlecode.com/files/pure-faust-0.8.tar.gz)

## pure-ffi ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

The [libffi](http://sourceware.org/libffi/) library provides a portable, high level programming interface to various calling conventions. This allows a programmer to call any function specified by a call interface description at run time.

This module provides an interface to libffi which enables you to call C functions from Pure and vice versa. It extends and complements Pure's built-in C interface in that it also handles C structs and makes Pure functions callable from C without writing a single line of C code. Moreover, depending on the libffi implementation, it may also be possible to call foreign languages other than C.

Abridged documentation is available online in [html](http://docs.pure-lang.googlecode.com/hg/pure-ffi.html) format. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pure-ffi).

**Download:** [pure-ffi-0.12.tar.gz](http://pure-lang.googlecode.com/files/pure-ffi-0.12.tar.gz)

## pure-g2 ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

This is a straight wrapper of the [g2 graphics library](http://g2.sf.net/).

g2 is a simple, no-frills 2D graphics library, distributed under the LGPL. Quoting from the g2 website, "it's easy to use, portable and supports PostScript, X11, PNG and Win32." Just the kind of thing that you need if you want to quickly knock out some basic graphics, and whipping out the almighty OpenGL or GTK/Cairo seems overkill.

The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-g2). Installation instructions can be found in the [README](http://pure-lang.googlecode.com/hg/pure-g2/README) file, and the package also contains a few examples.

**Download:** [pure-g2-0.2.tar.gz](http://pure-lang.googlecode.com/files/pure-g2-0.2.tar.gz)

## pure-gl ##

**Authors:** Albert Graef <Dr.Graef at t-online.de>, Scott E. Dillard

pure-gl is Pure's interface to [OpenGL](http://www.opengl.org/), the well-known graphics library. It covers pretty much all of OpenGL, including the popular extensions. Extensions are loaded on demand, functions will throw an exception if they are not available in your OpenGL implementation.

See the [README](http://pure-lang.googlecode.com/hg/pure-gl/README) file for more information. pure-gl is distributed under a BSD-style license, please see the [COPYING](http://pure-lang.googlecode.com/hg/pure-gl/COPYING) file for details. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pure-gl).

**Download:** [pure-gl-0.8.tar.gz](http://pure-lang.googlecode.com/files/pure-gl-0.8.tar.gz)

**Windows package:** [pure-gl-0.8.1.msi](http://pure-lang.googlecode.com/files/pure-gl-0.8.1.msi)

## pure-glpk ##

**Author:** Jiri Spitz

This module provides a feature complete GLPK interface for the Pure programming language, which lets you use all capabilities of the [GNU Linear Programming Kit](http://www.gnu.org/software/glpk) (GLPK) directly from Pure. The module is under development (the interface is more or less complete and working but the documentation is still under construction). pure-glpk is distributed under the GPLv3, please see the [COPYING](http://pure-lang.googlecode.com/hg/pure-glpk/COPYING) file for details. You can find the current current sources [here](http://code.google.com/p/pure-lang/source/browse/pure-glpk).

**Download:** [pure-glpk-0.2.tar.gz](http://pure-lang.googlecode.com/files/pure-glpk-0.2.tar.gz)

## pure-gplot ##

**Author:** Kay-Uwe Kirstein

This module provides an interface to the [Gnuplot](http://www.gnuplot.info/) plotting utility. Plot commands and the respective data is send to Gnuplot via stdin-pipe. The module is still under development, but provides basic 2d plotting functionality. 3d-plots (`splot`) are not directly supported, yet. pure-gplot needs a standard Gnuplot installation and is distributed under the GPLv3. Documentation is available in [html](http://docs.pure-lang.googlecode.com/hg/pure-gplot.html) format. The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-gplot).

**Download:** [pure-gplot-0.1.tar.gz](http://pure-lang.googlecode.com/files/pure-gplot-0.1.tar.gz)

## pure-gsl ##

**Authors:** Albert Graef <Dr.Graef at t-online.de>, Eddie Rucker <erucker at bmc.edu>

Building on Pure's GSL-compatible matrix support, this module aims to provide a complete wrapper for the [GNU Scientific Library](http://www.gnu.org/software/gsl) which offers a wide range of mathematical routines useful for scientific programming, number crunching and signal processing applications. This is work in progress, only a small part of the interface is finished right now. Here is a brief summary of the operations which are implemented:

  * Matrix-scalar and matrix-matrix arithmetic. This is fairly complete and includes matrix multiplication, as well as element-wise exponentiation (`^`) and integer operations (`div`, `mod`, bit shifts and bitwise logical operations) which aren't actually in the GSL API.

  * SVD (singular value decomposition), as well as the corresponding solvers, pseudo inverses and left and right matrix division. This is only available for real matrices right now, as GSL doesn't implement complex SVD.

  * Random distributions and statistic functions.

  * Polynomial evaluation and root finding.

  * Linear least-squares fitting. Multifitting is not available yet.

You can find the current sources [here](http://code.google.com/p/pure-lang/source/browse/pure-gsl/). pure-gsl is distributed under the GPLv3 (or later), see the [COPYING](http://pure-lang.googlecode.com/hg/pure-gsl/COPYING) file for details. Please also see the [README](http://pure-lang.googlecode.com/hg/pure-gsl/README) file for installation instructions, and [gslexample.pure](http://pure-lang.googlecode.com/hg/pure-gsl/examples/gslexample.pure) for some examples. Documentation is available in [html](http://docs.pure-lang.googlecode.com/hg/pure-gsl.html) format.

**Download:** [pure-gsl-0.11.tar.gz](http://pure-lang.googlecode.com/files/pure-gsl-0.11.tar.gz)

## pure-gtk ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

pure-gtk is a collection of bindings to use the [GTK+](http://www.gtk.org) GUI toolkit version 2.x with Pure. The bindings include the gtk (+gdk), glib, atk, cairo and pango libraries, each in their own Pure module. At present these are just straight 1-1 wrappers of the C libraries, created with pure-gen. So they still lack some convenience, but they are perfectly usable already, and a higher-level API for accessing all the functionality will hopefully become available in time.

Please see the [README](http://pure-lang.googlecode.com/hg/pure-gtk/README) file for installation instructions and licensing conditions, and [hello.pure](http://pure-lang.googlecode.com/hg/pure-gtk/examples/hello.pure) in the source for a basic example. The files [uiexample.pure](http://pure-lang.googlecode.com/hg/pure-gtk/examples/uiexample.pure) and [uiexample.glade](http://pure-lang.googlecode.com/hg/pure-gtk/examples/uiexample.glade) show how to run a GUI created with the [Glade](http://glade.gnome.org/) interface builder.

Note that if your version of GTK+ and/or the support libraries differs widely from what I have then you may have to regenerate the headers with pure-gen (`make generate` does this). The present version of the bindings were created with GTK+ 2.24.4 on Ubuntu 11.04.

**Download:** [pure-gtk-0.10.tar.gz](http://pure-lang.googlecode.com/files/pure-gtk-0.10.tar.gz)<br />
**Windows package:** [pure-gtk-0.10.msi](http://pure-lang.googlecode.com/files/pure-gtk-0.10.msi) (_Note:_ You'll also need [GTK+ 2.22](http://www.gtk.org/download-windows.html) from the GTK+ website.)

## pure-liblo ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

The [liblo](http://liblo.sourceforge.net/) library is a lightweight C implementation of Berkeley's [OSC](http://opensoundcontrol.org/) ("Open Sound Control") protocol. It allows you to read and write OSC-formatted data across the network, and can also be used to communicate with various OSC-enabled devices and multimedia software in realtime.

The Pure module provides complete Pure bindings of liblo (`lo.pure`) along with a more high-level interface which makes it easy to implement both OSC clients and servers (`osc.pure`).

Abridged documentation is available online in [html](http://docs.pure-lang.googlecode.com/hg/pure-liblo.html) format. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pure-liblo).

**Download:** [pure-liblo-0.8.tar.gz](http://pure-lang.googlecode.com/files/pure-liblo-0.8.tar.gz)

**Windows package:** [pure-liblo-0.8.msi](http://pure-lang.googlecode.com/files/pure-liblo-0.8.msi)

## pure-midi ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

pure-midi is Pure's interface to [MIDI](http://home.roadrunner.com/~jgglatt/), the "Musical Instrument Digital Interface". This module enables you to process MIDI data from hardware devices and other MIDI applications in realtime and also provides support for reading and writing standard MIDI files. To these ends, it provides wrappers around Roger B. Dannenberg's PortMidi library (part of the [PortMedia](http://portmedia.sourceforge.net/) project) and David G. Slomin's midifile library (from his [MIDI Utilities](http://public.sreal.com:8000/~div/midi-utilities/) package).

See the [README](http://pure-lang.googlecode.com/hg/pure-midi/README) file for more information. pure-midi is distributed under a BSD-style license, please see the [COPYING](http://pure-lang.googlecode.com/hg/pure-midi/COPYING) file for details. The current sources are available [here](http://code.google.com/p/pure-lang/source/browse/pure-midi).

**Download:** [pure-midi-0.5.tar.gz](http://pure-lang.googlecode.com/files/pure-midi-0.5.tar.gz)

**Windows package:** [pure-midi-0.5.msi](http://pure-lang.googlecode.com/files/pure-midi-0.5.msi)

## pure-mpfr ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

pure-mpfr is an interface to the [GNU mpfr](http://www.mpfr.org/) library for performing multiprecision floating point arithmetic with proper rounding. The module plugs into Pure's numeric tower, overloading some of the operations in the math.pure module in order to provide seamless integration of mpfr numbers with the Pure language. The desired precision can be changed on the fly, and the module hooks into the interpreter to pretty-print mpfr numbers in decimal format.

The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-mpfr), and documentation in html format is available [here](http://docs.pure-lang.googlecode.com/hg/pure-mpfr.html).

**Download:** [pure-mpfr-0.4.tar.gz](http://pure-lang.googlecode.com/files/pure-mpfr-0.4.tar.gz)

## pure-octave ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

This module provides an interface to [Octave](http://www.octave.org/), the premier open source numeric math software. It embeds the full Octave interpreter into your Pure programs. You can execute arbitrary Octave code, exchange data between Pure and Octave, execute Octave functions directly from Pure (including support for Octave "inline" functions), and also call Pure functions from Octave. The mapping between Pure and Octave
data such as matrices and strings is straightforward and handled automatically.

The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-octave). pure-octave is distributed under the GPLv3 (or later), see the [COPYING](http://pure-lang.googlecode.com/hg/pure-octave/COPYING) file for details. Documentation in html format is available [here](http://docs.pure-lang.googlecode.com/hg/pure-octave.html).

**Download:** [pure-octave-0.2.tar.gz](http://pure-lang.googlecode.com/files/pure-octave-0.2.tar.gz)

## pure-odbc ##

**Authors:** Albert Graef <Dr.Graef at t-online.de>, Jiri Spitz

A port of Q's [ODBC](http://en.wikipedia.org/wiki/Open_Database_Connectivity) module, which provides access to all ODBC-capable databases, including [MySQL](http://www.mysql.com/), [PostgreSQL](http://www.postgresql.org/) and [SQLite](http://www.sqlite.org/) (via the [SQLite ODBC driver](http://www.ch-werner.de/sqliteodbc)). Documentation is available online in [html](http://docs.pure-lang.googlecode.com/hg/pure-odbc.html) format. The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-odbc/).

**Download:** [pure-odbc-0.8.tar.gz](http://pure-lang.googlecode.com/files/pure-odbc-0.8.tar.gz)

## pure-rational ##

**Author:** Rob Hubbard, Jiri Spitz

A rational number library, originally written by Rob Hubbard for Q, ported to Pure by Jiri Spitz. Provides a wealth of additional functions useful for dealing with rational numbers, including continued fractions, various approximation routines, string formatting and evaluation and support for complex rationals. Please see the [documentation](http://docs.pure-lang.googlecode.com/hg/pure-rational.html) for details. The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-rational/).

**Download:** [pure-rational-0.1.tar.gz](http://pure-lang.googlecode.com/files/pure-rational-0.1.tar.gz)

## pure-readline ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

A simple interface to the GNU readline and BSD editline libraries. The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-readline/). (Please note that while pure-readline itself is just a few lines of trivial glue code released under a simple all-permissive license, it needs to be linked against either GNU readline or BSD editline, so if you use this module then you are also bound by the corresponding license terms, which means GPLv3+ in the case of readline, and the BSD license in the case of editline.)

**Download:** [pure-readline-0.1.tar.gz](http://pure-lang.googlecode.com/files/pure-readline-0.1.tar.gz)

## pure-reduce ##

**Authors:** Albert Graef <Dr.Graef at t-online.de>, Kurt Pagani <kp@scios.ch>

An interface to [REDUCE](http://reduce-algebra.com/), the venerable computer algebra system which is still going strong. This module embeds the REDUCE system in Pure, so that you can employ Pure as a full-featured computer algebra system. With Pure 0.56 and later this module can also be used in conjunction with the Pure [TeXmacs](TeXmacs.md) plugin. Please note that this module is still somewhat experimental and is only available in the hg repository right now. But the basic functionality already works, and the package includes some examples illustrating its use.

More information can be found in the [online documentation](http://docs.pure-lang.googlecode.com/hg/pure-reduce.html). The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-reduce).

## pure-sockets ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

A Pure interface to the Berkeley socket functions. Provides most of the core functionality, so you can create sockets for both stream and datagram based protocols and use these to transmit messages. Unix-style file sockets are also available if the host system supports them.

Documentation in html format is available [here](http://docs.pure-lang.googlecode.com/hg/pure-sockets.html). The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-sockets).

**Download:** [pure-sockets-0.6.tar.gz](http://pure-lang.googlecode.com/files/pure-sockets-0.6.tar.gz)

## pure-sql3 ##

**Author:** Peter Summerland, Albert Graef <Dr.Graef at t-online.de>

Sql3 is an interface to the popular [Sqlite3](http://www.sqlite.org/) database. The module provides a minimal wrapper around Sqlite3's C interface which is designed to give the developer access to all of Sqlite3's features in a way that is convenient for Pure programmers. It provides a handful of convenience functions that make the "core" functions in the interface easier to use, while at the same time "keeping out of the way" so the user can tap into the rest of the C interface directly.

The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-sql3). Documentation in html format is available [here](http://docs.pure-lang.googlecode.com/hg/pure-sql3.html).

**Download:** [pure-sql3-0.4.tar.gz](http://pure-lang.googlecode.com/files/pure-sql3-0.4.tar.gz)

## pure-stldict ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

This package provides a light-weight, no frills interface to the C++ dictionary containers `map` and `unordered_map`, see the [C++ standard library documentation](http://en.cppreference.com/w/cpp) for details. The `stldict` module makes these data structures available in Pure land and equips them with a (more or less) idiomatic Pure container interface.

Documentation in html format is available [here](http://docs.pure-lang.googlecode.com/hg/pure-stldict.html). The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-stldict).

**Download:** [pure-stldict-0.3.tar.gz](http://pure-lang.googlecode.com/files/pure-stldict-0.3.tar.gz)

## pure-stllib ##

**Author:** Peter Summerland

The C++ [Standard Template Library](http://www.cplusplus.com/reference/stl/) ("STL") is a library of generic containers (data structures designed for storing other objects) and a rich set of generic algorithms that operate on them. pure-stllib provides Pure interfaces to a selection of containers from the library, specialized to hold pointers to arbitrary Pure expressions. pure-stlvec is a Pure interface to C++'s vector and the STL algorithms that act on them. pure-stlmap is an interface to six (of the eight) of C++'s associative containers: map, set, multimap, multiset, unordered\_map and unordered\_set.

The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-stllib). Documentation in html format is available [here](http://docs.pure-lang.googlecode.com/hg/pure-stllib.html).

**Download:** [pure-stllib-0.4.tar.gz](http://pure-lang.googlecode.com/files/pure-stllib-0.4.tar.gz)

## pure-tk ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

<a href='http://wiki.pure-lang.googlecode.com/hg/graphedit.png'><img src='http://wiki.pure-lang.googlecode.com/hg/graphedit-mini.png' align='right' /></a>

This is a much improved port of the Q Tk module which lets you develop GUI applications with [Tcl/Tk](http://www.tcl.tk/). The module embeds a Tcl/Tk interpreter in your Pure programs.

As a GUI toolkit, Tk is somewhat old-fashioned, but it has a really nice canvas widget and nothing beats Tcl's simplicity if you want to quickly knock out a GUI for your application. Also, the Pure module includes some special support for Peter Baum's [Gnocl](http://www.gnocl.org/) extension which turns Tcl into a frontend for GTK+/Gnome development and provides a convenient alternative to using Pure's GTK+ interface in a direct fashion. Last but not least, Tcl offers its own extensions for almost every area of programming, so this module can also serve as a bridge to various non-GUI libraries.

The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-tk). Examples are included in the package, and you can also find some complete demo applications [here](http://code.google.com/p/pure-lang/source/browse/pure-tk-examples). Documentation in html format is available [here](http://docs.pure-lang.googlecode.com/hg/pure-tk.html).

**Download:** [pure-tk-0.3.tar.gz](http://pure-lang.googlecode.com/files/pure-tk-0.3.tar.gz)

**Windows package:** [pure-tk-0.3.msi](http://pure-lang.googlecode.com/files/pure-tk-0.3.msi) (_Notes:_ You'll also need [Tcl/Tk 8.5](http://www.tcl.tk/software/tcltk/8.5.html). Also, a Windows build of Gnocl 0.9.95 is included in the package. You can also get this as a separate download [here](http://pure-lang.googlecode.com/files/gnocl0.9.95-win32.zip); this also includes the required libglade dll. Note that in order to use Gnocl on Windows, you'll also need [GTK+ for Windows](http://www.gtk.org/download-windows.html).)

## pure-xml ##

**Author:** Albert Graef <Dr.Graef at t-online.de>

This is a port of Q's [XML](http://www.w3.org/TR/xml) module, which also includes support for [XSLT](http://www.w3.org/TR/xslt). The module provides a simplified interface to the [Gnome libxml2 and libxslt](http://xmlsoft.org/) libraries, and supplies all the necessary data structures and operations to inspect, create, modify and transform XML documents with ease.

Documentation is available online in [html](http://docs.pure-lang.googlecode.com/hg/pure-xml.html) format. The current sources can be found [here](http://code.google.com/p/pure-lang/source/browse/pure-xml/).

**Download:** [pure-xml-0.6.tar.gz](http://pure-lang.googlecode.com/files/pure-xml-0.6.tar.gz)