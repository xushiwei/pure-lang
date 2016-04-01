

# Introduction #

Pure aims to provide you with a seamless blend of general term rewriting and important concepts and techniques from the functional programming world. Programs are simply collections of equations which are used to rewrite expressions in a symbolic fashion. This process is repeated until a "normal form" is reached which cannot be reduced any further by applying equations.

In the simplest case, an equation looks just like an ordinary function definition:

```
> square x = x*x;
> square 99;
9801
```

The left-hand side of an equation may contain structured arguments, which allows you to define functions operating on structured data by "pattern matching":

```
> sum [] = 0;
> sum (x:xs) = x+sum xs;
> sum (1..30);
465
```

More generally, the patterns can be any kind of "algebraic" data structure built by applying constructors to arbitrary subterms. As another example, here is an operation which inserts elements into a binary search tree represented with the constructor symbols `nil` and `bin`:

```
> nonfix nil;
> insert nil y         = bin y nil nil;
> insert (bin x L R) y = bin x (insert L y) R if y<x;
>                      = bin x L (insert R y) otherwise;
> foldl insert nil [7,3,9,18];
bin 7 (bin 3 nil nil) (bin 9 nil (bin 18 nil nil))
```

# Polymorphism #

Simple rewriting rules of the kind shown above are fairly common business in most if not all modern FPLs. Pure differs from languages like Haskell and ML, however, in that it is dynamically typed. That is, all terms belong to the same big [Herbrand universe](http://en.wikipedia.org/wiki/Term_algebra). Thus Pure supports an arbitrary degree of [polymorphism](http://en.wikipedia.org/wiki/Type_polymorphism) (not just parametric polymorphism). Any operation can be extended at any time, even builtins. For instance, the following equations lift the `+` and `-` operators to pointwise operations:

```
> f + g = \x -> f x + g x if nargs f > 0 && nargs g > 0;
> f - g = \x -> f x - g x if nargs f > 0 && nargs g > 0;
> f x = 2*x+1; g x = x*x; h x = 3;
> map (f+g-h) (1..10);
[1,6,13,22,33,46,61,78,97,118]
```

Note that these rules also handle functions taking multiple arguments, so that you can write, e.g.:

```
> (max-min) 2 5;
3
```

Constructors can be extended in exactly the same way:

```
> f,g = \x -> f x, g x if nargs f > 0 && nargs g > 0;
> (max,min,max-min) 2 5;
5,2,3
```

# Symbolic Rewriting #

In Pure, simple pattern-matching definitions like those of `sum` and `insert` above are in fact just a special kind of rewriting rules. Pure goes well beyond this. Expressions are first-class objects, so you can evaluate symbolic terms, too. Assuming the definitions from above, you'll get:

```
> square (a+b);
(a+b)*(a+b)
> sum [a,b,c];
a+(b+(c+0))
> map f [a,b+c,x*y];
[2*a+1,2*(b+c)+1,2*(x*y)+1]
```

Moreover, the left-hand side of an equation can contain arbitrary functions and operators in the arguments. For instance, here are some symbolic rewriting rules for associativity and distributivity of the `+` and `*` operators:

```
> (x+y)*z = x*z+y*z; x*(y+z) = x*y+x*z;
> x+(y+z) = (x+y)+z; x*(y*z) = (x*y)*z;
> square (a+b);
a*a+a*b+b*a+b*b
> sum [a,b,c];
a+b+c+0
> map f [a,b+c,x*y];
[2*a+1,2*b+2*c+1,2*x*y+1]
```

Note that rewriting rules like the above aren't possible in Haskell or ML, because they violate the "constructor discipline", which only allows pure constructors in the argument patterns. In contrast, Pure doesn't really distinguish between "constructors" and "defined functions". Any function symbol can act as a constructor if there are no equations which apply to a given term, and a pure constructor is just a function symbol without any defining equations, so that applications of this symbol are always in normal form.

Here is another example which allows you to rewrite symbolic logical expressions involving the operators `~`, `&&` and `||` to [disjunctive normal form](http://en.wikipedia.org/wiki/Disjunctive_normal_form), by applying de Morgan's laws together with the laws of distributivity and associativity:

```
~~a = a;

~(a || b) = ~a && ~b;
~(a && b) = ~a || ~b;

a && (b || c) = a && b || a && c;
(a || b) && c = a && c || b && c;

(a && b) && c = a && (b && c);
(a || b) || c = a || (b || c);
```

For instance:

```
> a || ~(b || (c && ~d));
a||~b&&~c||~b&&d
```

# Constructor Equations #

As a bonus, you also get "constructors with equations" for free. E.g., suppose that we want lists to automagically stay sorted and eliminate duplicates. In Pure we can do this by simply adding the following equations for the `:` constructor:

```
> x:y:xs = y:x:xs if x>y; x:y:xs = x:xs if x==y;
> [13,7,9,7,1]+[1,9,7,5];
[1,5,7,9,13]
```

This is possible because rewriting rules aren't subject to the constructor discipline in Pure. Note that as the above definition is at global scope, it will apply to _all_ lists in your program, which might not be what you want. As a remedy, Pure also supports _local_ rewriting rules, as explained in the following section.

# Local Definitions and Rules #

As a convenience, Pure extends the term rewriting calculus with constructs for denoting [lambdas](http://en.wikipedia.org/wiki/Lambda_calculus) (anonymous functions) and local function and variable definitions. For instance, the following definition shows the use of a local function definition (`with ... end`):

```
> fib n = fibs 0 1 n with fibs a b n = if n<1 then a else fibs b (a+b) (n-1) end;
> map fib (0..20);
[0,1,1,2,3,5,8,13,21,34,55,89,144,233,377,610,987,1597,2584,4181,6765]
```

A pattern-matching `case` expression is also provided. These facilities are similar to those available in most modern FPLs. In Pure these constructs are all implemented in terms of the basic term rewriting machinery, using [lambda lifting](http://en.wikipedia.org/wiki/Lambda_lifting) to eliminate local functions, and the following equivalences:

```
\x1 ... xn -> y                      ==  f with f x1 ... xn = y; f _ ... _ = throw failed_match end
case x of y1 = z1; ...; yn = zn end  ==  f x with f y1 = z1; ...; f yn = zn; f _ = throw failed_match end
x when y1 = z1; ...; yn = zn end     ==  x when yn = zn end ... when y1 = z1 end
x when y = z end                     ==  case z of y = x end
```

These constructs are all [lexically scoped](http://en.wikipedia.org/wiki/Scope_(programming)). Pure also provides a way to introduce [dynamic scopes](http://en.wikipedia.org/wiki/Scope_(programming)) which make it possible to reevaluate an expression in the context of a given collection of local rewriting rules. This is similar to Mathematica's [ReplaceAll](http://reference.wolfram.com/mathematica/ref/ReplaceAll.html) operation. For instance:

```
expand = reduce with (a+b)*c = a*c+b*c; a*(b+c) = a*b+a*c end;
factor = reduce with a*c+b*c = (a+b)*c; a*b+a*c = a*(b+c) end;
```

With these definitions you get:

```
> expand ((a+b)*2);
a*2+b*2
> factor (a*2+b*2);
(a+b)*2
```

It goes without saying that this construct is rather essential when doing computer algebra stuff, but it can also be useful in less esoteric circumstances. For instance, here's a quick and dirty way to implement a simple (and rather inefficient) kind of insertion sort algorithm using one of the list constructor equations discussed in the previous section:

```
> isort = reduce with x:y:xs = y:x:xs if x>y end;
> isort [1,7,2,13,9,7];
[1,2,7,7,9,13]
```

# The Evaluation Process #

Evaluating an expression in Pure means that the expression is rewritten using the equations of your program (including the prelude and other modules which may be loaded). This process is repeated until no more equations apply, in which case the resulting expression is in "normal form" and is taken to be the "value" of the original expression. (Note that we don't talk about "functions" here at all. All the Pure interpreter worries about are the expressions to be evaluated and the equations which can be applied to them.)

Expressions are normally evaluated in a [leftmost-innermost](http://en.wikipedia.org/wiki/Evaluation_strategy#Applicative_order) fashion, using [call-by-value](http://en.wikipedia.org/wiki/Evaluation_strategy#Call_by_value) semantics. (Pure also provides means to do [call-by-need](http://en.wikipedia.org/wiki/Evaluation_strategy#Call_by_need) evaluation, but we won't go into that here.) So, for instance, in order to evaluate `square (5+8)`, the interpreter does the following reductions:

```
square (5+8) => square 13 => 13*13 => 169
```

Conceptually, for each reduction step the interpreter has to perform the following operations:

  1. Match the subject expression against the left-hand sides of equations. If more than one equation matches, they are tried in the order in which they are listed in the program. If no equation matches, the expression is already in normal form and we're done.
  1. Bind the variables in the matching equation to their corresponding values.
  1. For conditional equations like `abs x = x if x>=0`, evaluate the guard using the variable binding determined in step 2. If the guard fails, try the next equation. Otherwise proceed with step 4.
  1. Evaluate the right-hand side of the equation using the variable binding determined in step 2.

This might seem inefficient, but luckily the interpreter compiles your program to fast native code before executing it. The pattern-matching code uses a kind of optimal decision tree which only needs a single, non-backtracking left-to-right scan of the subject term (you can find a description of the algorithm [here](http://wiki.pure-lang.googlecode.com/hg/docs/rtapaper.pdf)). In most cases the matching overhead is barely noticable, unless you discriminate over huge sets of heavily overlapping patterns. Using these techniques and native compilation, the Pure interpreter is able to achieve very good performance, offering execution speeds in the same ballpark as good Lisp interpreters. It seems to be one of the fastest implementations of term rewriting as a programming language right now, and is certainly good enough for most programming tasks except maybe the most demanding number crunching applications (and even these can be tackled by interfacing to Fortran or C).

# Further Information #

You might wonder how powerful term rewriting is as a model of computation. The answer is that it's not only Turing-complete, but also a very convenient way to write programs in a high-level, algebraic style. Term rewriting has long been the workhorse of computer algebra systems and equational theorem provers, and its use as a programming language was explored already in the 1980s by [Michael O'Donnell](http://www.cs.uchicago.edu/people/odonnell). More information can be found at the [Rewriting Home Page](http://rewriting.loria.fr/) of the IFIP Working Group 1.6.

Here are some links to other software which also uses term rewriting as an underlying computational model:

  * [Aardappel](http://wouter.fov120.com/aardappel/): Wouter van Oortmerssen's visual programming language based on term rewriting.
  * [Bertrand](http://groups.google.com/group/bertrand-constraint): Wm Leler's constraint programming language which uses augmented term rewriting to build numeric constraint solving systems.
  * [ELAN](http://www.loria.fr/equipes/protheo/SOFTWARES/ELAN/): An environment for specifying and prototyping deduction systems in a language based on rewrite rules controlled by strategies.
  * [EPIC](http://www.babelfish.nl/intro.html): A reduction engine intended as a target for term rewriting compilers.
  * [Escher](http://en.wikipedia.org/wiki/Escher_(programming_language)): An experimental functional+logic programming language.
  * [Mathematica](http://www.wolfram.com/products/mathematica/): Arguably one of the most popular computer algebra systems out there. Other well-known examples are [Maple](http://www.maplesoft.com/) and [REDUCE](http://reduce-algebra.com/). (A module which embeds REDUCE in Pure is currently under development, see [here](Addons#pure-reduce.md).)
  * [Maude](http://maude.cs.uiuc.edu/): A programming language supporting both equational and rewriting logic.
  * [Mozart/Oz](http://www.mozart-oz.org/): A multi-paradigm constraint/logic programming language which also supports pattern-matching and rewriting.
  * [OBJ](http://www.cs.ucsd.edu/users/goguen/sys/obj.html): The mother of all algebraic programming and specification languages (actually, an entire family of languages supporting equational programming and theorem proving).
  * [OPAL](http://uebb.cs.tu-berlin.de/~opal/): A more modern algebraic programming language which also includes support for typical FP features such as currying and higher-order functions.
  * [Otter](http://www.cs.unm.edu/~mccune/otter/): A theorem prover which has an embedded equational programming system.
  * [Q](http://q-lang.sf.net/): Pure's predecessor.
  * [Req](http://www.accesscom.com/%7Edarius/): Darius Bacon's calculator programmable with rewrite rules.
  * [Stratego](http://www.program-transformation.org/Stratego/): A language and toolset for program transformation, useful for creating compilers and program generators.

Term rewriting is an important concept in both mathematical logic and computer science, and a number of nice books have been written on the subject. For instance:

  * Baader, Nipkow: [Term Rewriting and All That](http://www4.informatik.tu-muenchen.de/~nipkow/TRaAT)
  * Terese: [Term Rewriting Systems](http://www.cs.vu.nl/~terese)

Please see the [Rewriting Home Page](http://rewriting.loria.fr/) for a comprehensive list of available literature.