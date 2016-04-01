# Introduction #

To whet your appetite, here are some programming examples for your browsing
pleasure. You might also wish to refer to our very, **very** brief [introduction](PureIntro.md) to Pure, to help you reading the examples, and the
[standard library](http://code.google.com/p/pure-lang/source/browse/pure/lib) scripts, in particular the
[prelude](http://pure-lang.googlecode.com/hg/pure/lib/prelude.pure), for
definitions of the predefined operations.

# Examples #

Some basic examples first.

  * [hello.pure](http://pure-lang.googlecode.com/hg/pure/examples/hello.pure): Start here to quickly get an idea how Pure programs look like.
  * [avltree.pure](http://pure-lang.googlecode.com/hg/pure/examples/avltree.pure): An example illustrating how AVL trees can be implemented in Pure.
  * [recursive.pure](http://pure-lang.googlecode.com/hg/pure/examples/recursive.pure): One of the benchmarks from the Language Shootout. Have some fun comparing this to other language versions available [here](http://shootout.alioth.debian.org/gp4/benchmark.php?test=recursive&lang=all).
  * [sortalgos.pure](http://pure-lang.googlecode.com/hg/pure/examples/sortalgos.pure): Some classical sorting algorithms in Pure.
  * [sudoku.pure](http://pure-lang.googlecode.com/hg/pure/examples/sudoku.pure): Peter Bernschneider's Sudoku solver.

Matrix examples. These show off the new GSL matrix capabilities built into Pure
0.7 and later.

  * [linalg.pure](http://pure-lang.googlecode.com/hg/pure/examples/linalg.pure): Various basic linear algebra operations.
  * [gauss.pure](http://pure-lang.googlecode.com/hg/pure/examples/gauss.pure): A Pure implementation of the Gaussian elimination algorithm.
  * [gslexample.pure](http://pure-lang.googlecode.com/hg/pure-gsl/examples/gslexample.pure): Some examples for Pure's new [GSL module](http://code.google.com/p/pure-lang/wiki/Addons#pure-gsl), which aims to provide a complete wrapper for the [GNU Scientific Library](http://www.gnu.org/software/gsl).

Calling foreign code. These examples illustrate how easy it is to interface to your own code written in C, Fortran and other languages in order to "outsource" expensive operations such as heavy number crunching.

  * [fortran.pure](http://pure-lang.googlecode.com/hg/pure/examples/fortran.pure): Illustrates how to call GNU Fortran subroutines and functions from Pure.
  * [fortran.f90](http://pure-lang.googlecode.com/hg/pure/examples/fortran.f90): Accompanying Fortran module.
  * [inline\_examp.pure](http://pure-lang.googlecode.com/hg/pure/examples/bitcode/inline_examp.pure): How to inline C, C++ and Fortran code in Pure scripts.
  * [faust\_examp.pure](http://pure-lang.googlecode.com/hg/pure/examples/bitcode/faust_examp.pure): How to call Faust code from Pure. [Faust](http://faust.grame.fr/) is a functional signal processing language.

The following examples illustrate some of Pure's metaprogramming and symbolic evaluation capabilities:

  * [quasiquote.pure](http://pure-lang.googlecode.com/hg/pure/lib/quasiquote.pure): An implementation of a Lisp-like quasiquote operation which makes it easy to manipulate expressions as literals. (An alternative implementation can be found [here](http://pure-lang.googlecode.com/hg/pure/examples/quasiquote1.pure), and some usage examples [here](http://pure-lang.googlecode.com/hg/pure/test/test034.pure).)
  * [reflection.pure](http://pure-lang.googlecode.com/hg/pure/examples/reflection.pure): A Pure program can inspect (and modify) itself, very much like Lisp programs. (This is just a very basic example. Please check the [manual](http://docs.pure-lang.googlecode.com/hg/pure.html#reflection) for more.)
  * [rules.pure](http://pure-lang.googlecode.com/hg/pure/examples/rules.pure): An example showing how to implement local rewriting rules with the `reduce` builtin.
  * [symbolic.pure](http://pure-lang.googlecode.com/hg/pure/examples/symbolic.pure): Simplistic symbolic algebra examples (symbolic differentiation, disjunctive normal form).

Also make sure you have a look at some of [Libor Spacek](http://cmp.felk.cvut.cz/~spacelib/)'s examples:

  * [date.pure](http://pure-lang.googlecode.com/hg/pure/examples/libor/date.pure): A Mayan calendar counting off the days until the current cycle ends. A "must have" for hobby doomsday prophets!
  * [queens.pure](http://pure-lang.googlecode.com/hg/pure/examples/libor/queens.pure): Some additional n-queens algorithms, including Libor's blazingly fast non-backtracking solution.

That are all the featured examples for now. More examples can be found in the
[sources](http://code.google.com/p/pure-lang/source/browse/pure/examples).