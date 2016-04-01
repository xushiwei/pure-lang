<h1>The FAQ</h1>




---


# General #

## What is Pure? ##

It is an algebraic/functional, general-purpose programming language based on term rewriting. Essentially, your programs are just collections of term rewriting rules which are used to reduce expressions to "normal form". More information about this approach can be found on the [Rewriting](Rewriting.md) page.

Modern programming languages are getting exceedingly complicated. Pure is an attempt to "go back to the roots" and boil down programming to a model of computation which is as simple as possible, yet expressive and practical enough so that people would want to use it for their everyday programming. The author thinks that term rewriting is a good candidate for achieving that goal, as all computing is essentially rewriting (of symbols on the tape of a Turing machine, for instance), but term rewriting also offers the dimension of hierarchical data structures which simple string rewriting lacks, and most important concepts of functional programming can be added on top of that quite easily.

This approach isn't really new, there are quite a few [other systems](Rewriting#Further_Information.md) which employ term rewriting as a computational model. But previous attempts to provide a real programming language based on term rewriting have suffered from various limitations or have been too slow or inconvenient for real-life applications. We think that Pure can change this situation.

## Why Pure? ##

Pure is unique in that it combines general term rewriting with modern-style functional programming in a practical and general-purpose programming language (as opposed to other systems which are mostly used for algebraic modelling). While Pure may look a lot like other modern FPLs such as Haskell and ML on the surface, under the hood it's much different:

  * Pure is based on **term rewriting** rather than the lambda calculus. This gives you much more flexibility in dealing with general algebraic rules and constructor equations. Please visit the [Rewriting](Rewriting.md) page for details. Modern FPLs don't offer this since they segregate defined functions and constructors. Computer algebra systems do, but they're not general-purpose programming languages.

  * Pure is **dynamically typed**. All terms belong to the same big [Herbrand universe](http://en.wikipedia.org/wiki/Term_algebra) and any operation (even builtins) can be extended with new term rewriting rules at any time. This means a lot of flexibility with respect to ad-hoc [polymorphism](Rewriting#Polymorphism.md). Dynamic typing has become a rare feature in contemporary functional languages which usually employ a Hindley/Milner type system to offer more safety at the expense of restricting polymorphism. Erlang is one notable exception, but it lacks a real term data structure for representing algebraic data types in a convenient fashion.

  * Pure is conceptually **much simpler** than Haskell et al. You don't have to learn about monadic I/O and type classes to get something done. If you understand equations and how they are used to evaluate expressions in a symbolic fashion, you should be able to start using Pure right away.

  * Thanks to [LLVM](http://llvm.org), Pure offers state-of-the-art **JIT compiler technology** in an interactive environment, and makes it easy to interface to C in a direct fashion. Recent releases also provide the ability to directly import LLVM bitcode modules into Pure scripts and to inline code written in various languages (currently C/C++, Fortran and [Faust](http://faust.grame.fr/)). Scripts can also be compiled to standalone native executables (they are also executed during compilation, which makes it possible to employ [partial evaluation](http://en.wikipedia.org/wiki/Partial_evaluation) techniques).

  * Pure has an efficient MATLAB-like, GSL-compatible **matrix type** which makes it easy to interface to languages and libraries for doing numeric computations and digital signal processing. In particular, Pure interfaces to (GNU) Fortran, Octave and Grame's Faust, and you can directly invoke GSL routines on Pure matrices. In a way, this is like having Haskell, Octave, DSP programming and the term rewriting capabilities of a computer algebra system under one hood.

  * Pure is mainly used for **mathematical applications** right now, but there's a growing collection of extension modules for GUI, graphics, multimedia, database and web programming which makes Pure useful as a kind of **algebraic/functional scripting language** for a variety of purposes.

## Why _Not_ Pure? ##

Obviously I tried to make Pure as useful and pretty as I could, but it's no silver bullet either and there are reasons why you would **not** want to use it (as well as some counter-arguments why you might want to try it anyway, despite these shortcomings):

  * Pure probably appeals most to mathematically inclined programmers. This certainly doesn't mean that you have to hold a Master or PhD degree in mathematics, but if you hate symbolic algebra then Pure is probably not for you.

  * Because of dynamic typing and limited module facilities, Pure is not an ideal language for big projects and teams. It's more on the "[secret weapon](http://www.paulgraham.com/avg.html)" side of Paul Graham's programming language taxonomy. Like in Lisp, data types will be mostly in your head, so some people will miss the data modeling capabilities of H/M-typed languages.

  * Pure doesn't attain the execution speed of compiled, statically typed languages, and because of its dynamic typing it probably never will. (OTOH, it's certainly much faster than most scripting languages and thus efficient enough for many purposes.)

  * Pure doesn't support multithreading yet, so if you need this you will have to look elsewhere right now. (You can fork new processes, though, on platforms which provide that functionality.) Light-weight concurrent programming facilities are the next big thing on the TODO list, however.

  * Pure doesn't have all the libraries of Perl, Python et al. But considering other FPLs like Haskell and ML, Pure's [library support](Addons.md) isn't bad, and in any case the direct C interface, along with the [pure-gen](Addons#pure-gen.md) utility, makes it easy to roll your own library interfaces if you want.

# Compiling and Installing Pure #

## When running Pure, I get the error "pure: error while loading shared libraries". ##

On Linux, you have to run `ldconfig` after installing Pure, so that the dynamic linker finds the runtime library. Usually you have to do this as `root`, e.g.: `sudo ldconfig`. Please see `man ldconfig` for details. If after doing that you still get the same error, then most likely `/usr/local/lib` (or whereever the library was installed) is not in `/etc/ld.so.conf`, so you have to add that path to the file and then rerun `ldconfig`.

If you installed Pure in a nonstandard location which you can't (or don't want to) add to `/etc/ld.so.conf`, you can also set the `LD_LIBRARY_PATH` environment variable accordingly. Please see `man ld.so` for details.

## Compiling Pure on NetBSD. ##

For convenience, you should set the following environment variables in your shell startup script:

```
export C_INCLUDE_PATH=/usr/local/include:/usr/pkg/include
export LIBRARY_PATH=/usr/local/lib:/usr/pkg/lib
export LD_LIBRARY_PATH=/usr/pkg/lib:/usr/local/lib
```

To compile LLVM and Pure, you'll need updated versions of the following packages: gmake, perl5, flex, bison, readline, gmp, and gsl (Mercurial will be needed, too, if you plan to use the latest sources from the repository).

The rest of the compilation process is as described in the INSTALL file, except that you have to use `gmake` instead of `make`. Thus:

```
$ ./configure
$ gmake
# gmake install
```

Remember you must be a root user to install. Also, NetBSD folks recommend not putting stuff in ld.so.conf, so instead put `export LD_LIBRARY_PATH=/usr/pkg/lib:/usr/local/lib`
in your shell startup script, as shown above.

Most addons are compiled in the same way, minus the `configure` step:

```
$ gmake
# gmake install
```

# Using Pure #

## The interpreter segfaults when I compute something like fact 100000000. ##

As of Pure 0.56 stack checks are now enabled by default, but since the interpreter has no way of knowing the actual stack space of your system, you might still have to set the `PURE_STACK` environment variable to an appropriate stack size limit in kilobytes. A reasonable default is assumed if `PURE_STACK` is not set (at the time of this writing this is 8192K - 128K for interpreter and runtime). This should work on most modern PCs, but may be too large on some embedded systems such as tablets and mobile phones, for instance. You can change the value as follows (Bourne shell syntax):

```
export PURE_STACK=4096
```

If you now run the interpreter, it should generate an orderly exception instead (if not then you'll have to further reduce the `PURE_STACK` value):

```
> fact n = if n>0 then n*fact(n-1) else 1;
> fact 100000000;
<stdin>, line 2: unhandled exception 'stack_fault' while evaluating 'fact 100000000'
```

Please note that `PURE_STACK` is only an advisory limit. Your operating system should provide you with a command (such as `ulimit` on Linux) to set the actual stack size available to processes. (On Windows, the stack size is hardcoded into the interpreter executable, but there are [utilities](http://cs.nyu.edu/exact/core/doc/stackOverflow.txt) to change that setting if you need to.)

## Dict or Stldict? ##

Properties of `dict`:

  * included in the standard library

  * purely functional/referentially transparent (no side effects)

  * implemented in Pure

  * mature

Properties of `stldict`:

  * requires a separate package

  * imperative (insert/delete modifies data structure)

  * implemented using STL maps

  * faster than `dict`

  * supports larger tables than `dict`

  * may be converted to lazy lists

  * may be used both for dictionaries and as a set data structure.

  * somewhat experimental

# Using the Pure Sphinx Framework #

You can use the Pure sphinx framework to format the Pure documentation in your local copy of the pure-lang repository. This is mainly useful for project members who want to test their documentation before committing to the repositories, and for users who want to get their hands on the latest bits of documentation which might not be online yet.

The following notes describe the steps needed to get the necessary software installed, so that you can run Sphinx on the Pure documentation files. The steps should be followed in the indicated order.

## Comments ##

This is a somewhat verbose version of instructions that Albert sent me to help me set up the Pure sphinx framework on my system.

1. In order to be able to refer to a specific directory, I'll assume that you have a directory, ~/Downloads, that you can use for the installation. Obviously you could use another directory.

2. For the same reason, I'll assume that you have set up a local copy of the pure-lang repository in your ~/Downloads directory. I.e., your copy is at ~/Downloads/pure-lang.

If you haven't set up a local copy, the repository is available at
http://code.google.com/p/pure-lang/source/checkout.

3. The step-by-step instructions that follow were tested on Mint 11 (katya), using python-docutils version 0.7-2 and python-setuptools version 0.6.15-1ubuntu1. Katya is based on Ubuntu 11.04, so the same instructions work there. The instructions should also work with other recent Linux distributions, but might need minor adjustments concerning package names etc.


## Step-by-Step Installation Instructions ##

1. If you haven't already done so, install the python-setuptools and python-docutils packages using your package manager (synaptic et al).

2. While in the package manager, make sure that you do **not** have the python-sphinx or python-pygments packages installed. If they are installed already for some reason, please remove those packages. You'll install your own versions of these packages below.

3. cd to ~/Downloads, download the patched Pygments tarball at http://pure-lang.googlecode.com/files/pygments-1.3.1+pure.tar.bz2 and unpack the tarball. This is a Pygments version which has been patched up to provide the necessary support for highlighting Pure code.

4. cd to ~/Downloads/pygments and install the patched Pygments package by executing the following commands:

```
./setup.py build
sudo ./setup.py install
```

5. cd back to ~/Downloads and download the latest Sphinx from http://sphinx.pocoo.org/latest/ by executing:

```
hg clone https://bitbucket.org/birkenfeld/sphinx
```

6. cd to ~/Downloads/sphinx and install sphinx by executing the following commands:

```
python setup.py build
sudo python setup.py install
```

7. cd back ~/Downloads and copy the puredomain directory in the pure-lang repository into ~/Downloads by executing the following command:

```
cp -p -r pure-lang/puredomain .
```

8. cd into ~/Downloads/puredomain and install the Sphinx Pure domain by executing the following commands:

```
python setup.py build
sudo python setup.py install
```

9. cd into ~/Downloads/pure-lang/sphinx and kick the tires. To see if things are working, you can run "make allhtml" to format the docs in your local copy of the pure-lang repository. If you do, the generated html files will be at ~/Downloads/pure-lang/sphinx/build/html.

You can run "make" if you want to build the pdf manual in addition to the html files, but this requires a fairly complete TeX installation. You can use texlive from the Ubuntu/Mint package repos for that, but you also need some extra fonts and styles not usually installed by default, specifically: texlive-fonts-extra and texlive-latex-extra.

Sphinx also supports a couple of other formats; e.g., it's possible to format the documentation as a Windows html help file or as an ebook. Please check pure-lang/sphinx/Makefile for details. You can also run "make help" to see the available options.

10. To clean up, cd back to ~/Downloads and delete the puredomain, sphinx and pygments directories and the downloaded Pygments tarball.