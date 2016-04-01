# Just a Minute: A Very Brief Introduction to Pure #

This is just the executive summary. A much more comprehensive overview of the language
can be found in the
[Pure Manual](http://docs.pure-lang.googlecode.com/hg/pure.html). Also, you may wish to refer to [Using Pure](UsingPure.md) for a quick introduction to the Pure interpreter.

Pure is a rather simple language. Basically programs are just collections of
term rewriting rules, which are used to reduce expressions to "normal form" in a symbolic fashion. This is explained in more detail on the [Rewriting](Rewriting.md) page. For convenience, Pure does offer some extensions to the basic term rewriting calculus, such as lexical bindings of local variables (`when` keyword) and functions (`with` keyword). In difference to most traditional languages, these local scopes can be tacked on to any expression. So, for instance, `foo x when x = bar y end with foo x = x+1; bar x = x-1 end` evaluates `foo x` in the context of the given definitions of the local variable `x` and the local functions `foo` and `bar`. Note that the local blocks are always terminated with the keyword `end`. (Unlike [Haskell](http://www.haskell.org), Pure is a free-format language.) Also note that, as in most modern FPLs, function applications are usually written in "curried" notation (`foo x y` rather than `foo(x,y)`).

Pure is rather unusual in that it is essentially a typeless language. Keeping with
the spirit of term rewriting, all data belongs to the same universe of terms. Terms are formed by applying functions (and operators) to other terms (with some special notations thrown in for denoting customary data elements such as numbers and strings). Some of these terms may evaluate to something else, others are normal forms which just stand for themselves. Consequently, Pure does **not** distinguish between "defined" and "constructor" functions; partial function definitions are the norm rather than the exception in Pure, and any function symbol (or operator) can also act as a constructor if it happens to occur in a normal form. Pure also makes heavy use of ad-hoc polymorphism; in fact it's possible to extend any operation at any time with new defining equations. This makes for a general and flexible programming model which is much different in style from other modern FPLs like Haskell, even though the syntax is superficially similar.

Another fairly unique feature is that Pure has built-in support for **both** lists and
matrices (numeric arrays), which is like having Haskell and [Octave](http://www.octave.org)
under one hood, and makes it very easy to express common mathematical
operations which go beyond simple arithmetic and operations on scalars. Lists are denoted with the usual bracket notation, `[1,2,3]`. Matrices work pretty much like in Octave, except that they are written using curly braces, as in `{1,2,3}` (denoting a row vector) or `{1,2,3;4,5,6}` (a 2x3 matrix).

Lazy data structures such as streams (a.k.a. lazy lists) are supported through "futures", as in [Alice ML](http://www.ps.uni-sb.de/alice/). This makes it possible to work with infinite lists such as `1..inf` (the list of all positive integers) and enables some powerful programming techniques not available in mainstream languages. Pure also has a built-in macro facility which lets you define your own "special forms" featuring call-by-name argument passing. Of course, macros are defined using rewriting rules just like ordinary functions, but they are processed at compile time rather than runtime.

The basic numeric types are (machine) integers, bigints and double precision floating point numbers; the [math.pure](http://pure-lang.googlecode.com/hg/pure/lib/math.pure) module from the [standard library](http://code.google.com/p/pure-lang/source/browse/pure/lib) also provides support for complex and rational numbers. Strings like `"abc"` or `"Hello, world!\n"` are supported, too. These are always encoded in UTF-8 internally, so no separate type of Unicode strings is needed.

Here is a brief rundown of frequently-used operations defined in the [prelude](http://pure-lang.googlecode.com/hg/pure/lib/prelude.pure):

  * `x+y`, `x*y`, etc.: The common arithmetic, relational and logical operators usually have the same notation as in C (except logical negation which is denoted `~`; consequently, inequality is denoted `~=`). Note that the `+` operator is also used to denote string and list concatenation in Pure.

  * `x:y`: This is the list-consing operation. `x` becomes the head of the list, `y` its tail. This is a constructor symbol, and hence can be used on the left-hand side of a definition for pattern-matching.

  * `x..y`: Constructs arithmetic sequences. `x:y..z` can be used to denote sequences with arbitrary stepsize `y-x`. Infinite sequences can be constructed using an infinite bound (i.e., `inf` or `-inf`). E.g., `1:3..inf` denotes the (lazy) list of all positive odd integers.

  * `#x`: The size (number of elements) of the list, tuple or matrix `x`. In addition, `dim x` yields the dimensions (number of rows and columns) of a matrix.

  * `x!y`: This is the string, list, tuple and matrix indexing operation. Note that all indices in Pure are zero-based, thus `x!0` and `x!(#x-1)` are the first and last element of a list, tuple or matrix, respectively. In the case of matrices, the subscript may also be a pair of row and column indices, such as `x!(1,2)`.

  * `x!!ys`: This is Pure’s string, list, tuple and matrix "slicing" operation, which returns the list, tuple or matrix of all `x!y` while `y` runs through the elements of the list (or matrix) `ys`. In the case of matrices the index range may also contain two-dimensional subscripts, or the index range itself may be specified as a pair of row/column index lists such as `x!!(i..j,k..l)`.

Besides these, Pure also offers list and matrix comprehensions, and the prelude
provides a fairly complete set of customary list operations like `head`,
`tail`, `drop`, `take`, `filter`, `map`, `foldl`, `foldr`, `scanl`, `scanr`,
`zip`, `unzip`, etc., which make list programming so much fun in modern FPLs. In Pure, these also work on strings and matrices accordingly, although these data structures are internally represented as different kinds of C arrays for efficiency and easy interfacing to C.

A complete description of the standard library can be found in the [Pure Library Manual](http://docs.pure-lang.googlecode.com/hg/purelib.html).

Ok, this is hopefully enough to get you started reading Pure programs. Let's
take a look at some [examples](Examples.md).