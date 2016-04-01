

This section is intended for people already familiar with some programming languages, such as C or Python.

# Summary #

PURE can be described as a high-level, dynamically typed, functional programming language that has user-defined syntax, macros, term rewriting, multiple-precision numbers, direct binding to C libraries, and compilation to native code through the [LLVM](http://llvm.org).

PURE allows for rapid prototyping of algorithms since it comes with an interpreter and debugger, provides automatic memory management, and has powerful functional and symbolic programming capabilities as well as interface to C libraries (e.g. for numerics, low-level protocols, etc.). Although PURE is primarily a language for functional programming and term rewriting, it also has primitives for manipulating pointers and references to objects, throwing and catching exceptions, and deferring the evaluation of expressions ("lazy" evaluation). At the same time, PURE is a "small" language designed from scratch; its interpreter is not large, and the library modules are written in PURE itself (as is often the case with high-level languages).

PURE is not particularly difficult to learn and use; however, one must note that PURE is quite different from popular programming languages such as C/C++, Java, or Python (these are all basically **imperative** languages). If these are the only languages you are familiar with, you will need some time to get used to PURE, because PURE heavily uses _term rewriting_ and _functional programming_ -- both being nontrivial concepts quite foreign to languages such as C. You will find a brief introduction to these concepts below, and more detailed tutorials in later chapters.

Learning and practicing the art of functional programming and term rewriting is a refreshing intellectual experience that forces us to develop a totally different coding strategy and to rethink previously learned approaches to algorithms and data structures. In many cases, the functional approach allows us to write shorter programs that work the first time and are easier to maintain. This experience makes us better programmers in any language.

## Example of code in PURE ##

To show you what PURE code looks like, here is a short program that implements a simple "equation solver" (numerical root finding) algorithm. The input is a numeric function `f` of one numeric argument; two numbers `x1`, `x2` bracketing the root of _f(x)_=0; and a tolerance `eps`. (**Bracketing** means that _f(x1)_ and _f(x2)_ have opposite signs.) The output is a numerical approximation to the root.

```
 solve f x1 x2 eps = secant_step x1 (f x1) x2 (f x2)
  with                  // local function that has fixed f and eps
   secant_step x1 f1 x2 f2 =
                         // check whether we already have the root
    if abs(x1-x2) < eps 
    then  x1 // will return x1 also when the root is not bracketed!
    else if f3 == 0.0
    then  x3 
    else if f1*f3 < 0.0   // check which interval contains the root
    then  secant_step x1 f1 x3 f3
    else  secant_step x2 f2 x3 f3
                                // definitions local to secant_step
        when  x3 = secant x1 f1 x2 f2;  f3 = f x3    end
                                // compute the secant approximation
        with  secant x1 f1 x2 f2 = x1 - (x1-x2)*f1/(f1-f2)   end
  end;
```

To test this code, we save it to a file `solve1.pure`, and try to solve the equation sin(_x_)=_x_/2, looking for the root _x_ that is bracketed between 1 and 2, with precision of 10 digits. We get the following result in the PURE interpreter (our input is prefixed with `>`):
```
> run solve1            // will find 'solve1.pure' in current dir
> using math;                  // need this for the sine function
> func1 = \x -> x/2 - sin x; // this is the equation to be solved
> solve  func1  1  2  1e-10;
1.89549426703398
```
This result is actually good to 15 digits.

If you have trouble understanding the code at this point, keep reading -- everything will be explained in the tutorials that follow.

This code illustrates some of the features of PURE: function application syntax, use of recursion instead of loops, local definitions, and "single assignment" semantics (there are no variables that we update with new values). The logic of the code is simple: If a good approximation is not yet obtained, we call the function `secant_step` again, on a smaller interval. Note that the calls are tail-recursive, and thus will not consume stack space.

**Question**: This code looks a lot like Haskell, except for "with/when/end". Is PURE a variant of Haskell?

**Answer**: Not really: the similarity is mostly in the syntax and in the fact that both are functional languages. PURE is a simple, dynamic language based on term rewriting; Haskell is much more complex, has static typing, and is not based on term rewriting. However, it will be quite easy for you to learn and use PURE if you have some experience with Haskell.

# Details #

Here are some basic explanations about the PURE language as compared with other programming languages.

## Values and types ##

PURE is a high-level language with dynamic typing. A **high-level** language with **dynamic typing** and an **interpreter** allows us to write programs much faster than a language like C or C++ because the code is much shorter. Examples of such languages are Perl, Python, and Unix shell. PURE has the following features typical of a high-level language:

  * We do not have to declare the types of variables and functions separately; we just write code that sets the values and computes the results. (The type of a given value - number, string, etc. - can be determined, of course.)
  * A function can work on, say, numerical values of _any_ type as long as they can be added and multiplied; we do not have to worry about all the possible types that may be used. A function does not have a fixed type of the return value. (If desired, a function can be specialized to arguments of a fixed type, say, machine integers; this will generate faster code.)
  * We do not have to allocate memory explicitly; we just say something like "let X be a resizable array" and that's it. Memory is automatically deallocated when not needed.
  * Without writing low-level code or using special libraries, we can quickly create and use sophisticated and automatically managed data structures, such as text strings, lists, or hash tables (dictionaries).
  * We can write a small portion of code in the interpreter and see right away whether the code works as intended. A built-in debugger is available.
  * Interface to the operating system (e.g. reading files and directories or networking) is transparent and (mostly) independent of the OS.

A data value in PURE can be a number (integers and floating-point numbers), a text string (Unicode), a symbolic expression (see below), a reference to a global symbol, or a pointer to an external C object. In addition, a value can be a _function_, as the next section explains.

## Functional programming ##

PURE is a functional programming language. A **functional** language includes functions as a basic object, on par with numbers or text strings. Moreover, a functional language usually consists only of function definitions and of expressions, which are evaluated by applying functions to some input values. Functional languages usually do not have "procedures" or "subroutines" (i.e. portions of code that is executed but does not return a value); each function must take arguments and return a value. Functional languages usually do not have loops; instead, they use recursion.

If you are not familiar with functional languages, it will take you some time to get used to this concept. PURE provides an easy way to experiment with and learn functional programming. This part of the tutorial is an introductory, brief explanation of functional programming.

### Functions and functional programming ###

In traditional imperative languages such as FORTRAN or C, a **function** is a portion of executable code that takes some values, computes something, and returns the resulting value. Typically, one gives each function a name and defines it using a special language construction, for example (FORTRAN):
```
 SUBROUTINE DQLBCD(A,A1,B,B1,X,X1,Y,Y1,D)
 INTEGER I,J
 ...
 END
```

Functional languages make it possible for us to work with a _function_ as easily as with any other data structure. We do not have to concern ourselves with the details of the low-level representation of functions (where the code is, its size, problems with code relocation on copying, and so on). We can store a function in a variable as easily as this code shows: (PURE)
```
 // func is previously defined
 let a = func ;
```
We can also define new nameless functions and store them in variables:
```
 let func1 = \x -> 2*x+1 ;
```
If we want, we can have an array of functions; we can "apply" the array of functions to an array of input values and get an array of output values. Moreover, we can pass functions to other functions as arguments, or combine several functions in arbitrary ways, -- for example, using data supplied by the user at runtime, -- and return _newly defined_ functions as resulting "values". This is an extremely powerful programming technique widely used in functional programming languages such as LISP, ML, or Haskell.

An immediate example frequently needed in scientific computations is an "equation solver." Suppose we have a mathematical function _f(x,y)_ defined by a formula, and we need to find the number _X_ such that _f(X,y)_=0 for a given _y_. The result, _X_, depends on the value of _y_; in other words, _X_ is a function of _y_. If _f(x,y)_ is complicated, we need to use a numerical method to solve _f(x,y)=0_. In a functional language, it is straightforward to write code for such an "equation solver." One first writes code that solves numerically the equation _g(x)_=0, where _g(x)_ is a function given as the argument of the solver:
```
// take function g as argument and return a root x of g(x)=0
solve g = ...; // numerical code here
```
Then one calls this solver on a temporarily created function, `\x -> f x y`, and obtains _X(y)_ as a _function_ object:
```
// take f(x,y) and return a function \y -> X(y)
solve2 f = \y -> solve (\x -> f x y); // that's it!
// testing
f a b = sin(a*b)-exp(b)-a; // define f with two arguments
X = solve2 f; // obtain X(y)
X 3; // evaluate X(3)
```
Note that `solve2` does not return the _value_ of _X(y)_ for any particular value of _y_; instead it returns the _function itself_. That is, the function `solve2` actually returns a certain complicated data structure containing a portion of executable code that can be later executed to evaluate _X(y)_ for any given value of _y_.

Of course, an implementation of a similar technique is possible in C, C++, Java, or even FORTRAN. For instance, the scientific computation library GSL defines function objects with one, two, and three arguments. However, in all these languages one needs to use special code to get around the basic limitation that these languages have: namely, that functions are much more difficult to handle than other data types, and one needs low-level code to manipulate functions.

For example, in FORTRAN, C, and C++ one needs to use a _pointer_ to the function code if one wants to pass a function as a value to another function. Each function must be statically declared with a name, otherwise it is impossible to get a pointer to the function. It is usually not possible to obtain a pointer to a built-in function, such as "`+`". If one wants to create a new function at run time, -- for example, when the function is specified by a formula given in an input file, -- one typically needs to implement a mini-interpreter. In functional languages, all the details of the implementation are safely hidden; it is impossible to make a mistake with function pointers or other low-level details. In order to create a new function, it suffices to write simple-looking code such as `\x y -> x+2*y`. Yet, the compiled code for this function will be as efficient as for a statically declared function.

PURE includes the standard features of functional programming languages, such as ML or Haskell. Thus, one can manipulate functions in essentially arbitrary ways. The basic data structure of Pure is an "expression," which is an application of some function to some arguments (which, in turn, may be expressions). Both arguments and values of functions can be functions; no special notation is necessary to pass a function as an input "value" to another function, or to return a function as a resulting "value."

### Syntax of function calls ###

Following the tradition of several modern functional programming languages such as Haskell, the PURE language uses the no-parentheses syntax for function calls:
```
func x y z ;
```
which means "apply the function `func` to the arguments `x`, `y`, and `z`." In traditional imperative programming languages, this function call would be written as `func(x,y,z)`. In LISP, this would be written as a list, `(func x y z)`, where the first symbol in the list is the function symbol (called the **function head**). The same syntax rule is adopted in PURE: the first symbol is the function head (see, however, the section about programmable syntax).

So PURE's notation for function calls can be seen as "LISP without parentheses." Of course, parentheses may be used at any place, so we _could_ write `(func x y z);` in PURE, if we wanted the code to look a little more like LISP.

We actually _need_ to use parentheses if we want to apply `func` to a complicated set of expressions, for example:
```
func1 (func2 x y) (func3 z t) x;
```

Spaces are used in PURE for separating function heads from arguments, although parentheses could do as well: `(f)(x)(y);` is the same as `f x y` but looks more cumbersome. Otherwise, spaces (and newlines) are irrelevant in PURE. Each PURE expression or declaration must be terminated with a semicolon.

The no-parentheses syntax for function calls can be used for currying, as usual in functional languages. For example, if we define `func1` with three arguments as the sum of its arguments, then `func1 2 3` is a function that adds 5 to its argument, and `func 8` can accept two arguments, which it will add to 8:
```
> func1 x y z = x+y+z;
> b = func1 8;
> b 5 6;
19
```


### Functional purity and impurity ###

A computer program in a traditional imperative language, such as C or Python, generally consists of **declarations** (types of variables, functions, etc.) followed by _a sequence of commands_ (**statements**) that are executed in a particular order. Each command is an action that _changes_ something in the computer's memory; commands modify some values in memory, print or read files, etc. Typical commands are (in C syntax)
```
...
x = x+2;
b = b*6+x;
...
```
The programmer's job is to arrange the commands such that the program will eventually produce a desired output from given input. The resulting value of `x=x+2` is discarded; the statement `x=x+2` is evaluated only for the sake of the **side effect**, that is, in order to modify the contents of memory corresponding to `x`.

It is crucial in languages such as C or Python that the commands are executed in the right order. In the example shown above, the result would be different if `b=b*6+x` were executed before `x=x+2`. To arrange the order of commands, one uses loops, "if", "goto", or "jump" statements, procedures (collections of commands), and functions (collections of commands that return a value).

The paradigm of "pure functional programming" adopts a quite different view: A computer program is viewed as a _function_ that takes some input values and produces some output values. This is like mathematics, where "functions" are maps from inputs to outputs. Just as in mathematics, it is nonsensical to write `x=x+2`. In a mathematics text, one does not say "let `x` be the root of _f_(_x_)=0" and two pages later, "now we _change_ the value of `x` to `x+2`." This would be very confusing and contradictory to the previous definition of `x`. This is simply _bad practice_! Instead, in a mathematics text one introduces a _new symbol_, such as `y`, if one really needs to have a symbol for "2+x".

The pure functional paradigm of programming can be seen as the approach to writing programs that is similar to writing a mathematical text. In the pure functional paradigm, the only allowed executable constructions are _expressions_ and _definitions_ of functions. An expression is an application of functions to values (which may be, in turn, expressions). There is no concept of "location in memory," of a "loop," or of "the value of the variable `x` at this place in the program." Recursion is used instead of loops, and values of variables are usually defined only once and stay constant within a single block of code.

The paradigm of "pure functional programming" is a powerful approach to designing software that allows one to write very high-level, abstract code that nevertheless compiles into efficient machine code. Also, it is easier to understand the logic of a purely functional program because of the simplified control flow: every function just evaluates an expression using some input values. Another advantage of pure functional programming is that any function can be evaluated before, after, or in parallel with any other function: since variables are never updated, no synchronization conflicts or "race conditions" will arise. This is a promising feature since, in principle, a compiler can then _automatically_ parallelize a given program for running on several CPUs.

Some functional languages go so far as to prohibit an explicit side effect like `a=a+2`. For instance, the Haskell language does not allow changing of the value of any variable. (Side effects, such as updating a global state or printing to files, are of course possible, but the language is organized in such a way that these operations do not look like side effects to the Haskell compiler!) Such programming languages are called "pure" functional languages.

Despite its name, PURE is not a pure functional language because it allows the user to change the value of a variable (albeit in a limited way: using references to top-level global symbols). However, in many respects PURE adopts the pure functional paradigm. PURE does not have loops or other control statements typical of C or Python. PURE programs consist of three kinds of statements: syntactic and lexical scope declarations; definitions of functions (more precisely, rewriting rules); and expressions. It is possible (and preferable) to write code in PURE using the pure functional style of programming.

For instance, `x=x+2` is understood by PURE not as a command to change `x`, but as a definition of a _rule_ that substitutes `x+2` for the symbol `x`. This definition is invalid because it is a circular, never-ending recursion (`x` is defined through `x` and never reduces to any actual value). However, the compiler will not find this to be an error; it will simply _compile_ this definition, i.e. produce the code that can substitute `x+2` instead of `x` in any given expression. The compiler will then proceed to compile other definitions. When the program is run, the code will have to evaluate some expressions. If no expression using `x` is ever evaluated in that PURE program, the definition `x=x+2` will remain _unused_ and thus harmless! However, as soon as some expression involving `x` is encountered, the circular substitution will produce an infinite loop, and eventually a stack overflow will occur or memory will be exhausted.

A valid definition would have been `b=x+2`. That definition says, roughly speaking, that the symbol `b` can be replaced by the expression `x+2`. This is like a mathematical definition, explaining how to evaluate `b`. Note that it makes no sense to repeat `b=x+2` "several times"! (The result will always be the same, namely that `b` is defined as `x+2`, because it is impossible to change the value of `x`.) This situation is the same as in mathematics, where we cannot "repeat" a definition -- we define _b_=_x_+2 only _once_, and we do not change the definition of a symbol such as _x_ within one calculation.

If you try to run PURE code such as
```
b=x+2;
// ... more code ...
b=x+2;
b;
```
you will get a warning message telling you that one of your definitions is going to remain unused.

This contrasts with imperative languages, such as C or Python, where `x=x+2` is not a definition of `x` in the mathematical sense but a command to _change something in memory_. So `x=x+2` is perfectly valid in C or Python; it is a statement with a side effect. The resulting value (and the side effect) both depend on the previous contents of the variable `x`. The statement `x=x+2` may be executed several times, changing the value of `x` every time. Similarly, it may be useful to execute `b=x+2` several times because the current value of `x` could change.

People unfamiliar with functional programming may think that the pure functional paradigm -- programming without side effects and without changeable variables -- is very inconvenient. However, it appears inconvenient largely because it is new and unfamiliar and needs getting used to.

## Programmable syntax ##

We have seen that PURE uses a no-parentheses syntax for functions, which can be seen as a "LISP without parentheses."

However, in many cases it is more convenient to use the **infix** syntax and write `x+y*2` instead of the cumbersome (but legal) syntax
```
(+) x ( (*) y 2 ) ;
```
"Infix" means that the function symbols, such as `+` or `*`, are written _between_ the arguments of the function. For example, `*` in `y*2` is an "binary infix operator". We say "operator" instead of "function" to emphasize the fact that a special syntax is used for that function. There is otherwise no difference between "functions" and "operators".

"Postfix" means that the function symbol is written "after" the argument, and "prefix" means "before the argument". For example, the C++ operator `--` is used as a postfix in `a--` and as a prefix in `--a`.

PURE gives us the utmost flexibility in using infix, prefix, and postfix operators because we can _program_ the syntax ourselves.

Most programming languages have a fixed syntax. For instance, a function _f_ is applied to input values _x_ and _y_ as `f(x,y)`, while certain frequently used operations are denoted by special symbols, such as "+" in `x+y`. An expression such as `x+y*2` is understood as twice _y_ plus _x_ because the operator `*` binds tighter (has **higher precedence**) than `+` in an expression. Although some languages (like C++) allow to "overload" these operators and thus make `+` an arbitrary function call, it is impossible in C++ to change the syntax and to make, say, `+` bind tighter than `*`, or to define an expression such as `x**` with a "postfix double star" operator.

PURE allows to define _arbitrary_ operators (prefix, infix, postfix, and bracketing) with arbitrary precedence. The kernel of PURE does not actually know the syntax of `+` and `*`; these operators are _defined_ in the initialization script (the "prelude"). Since these operators are _user-defined_, one could delete these definitions from the prelude and declare a different precedence for the operators `+` and `*`. One can also define other operators, such as `:==:` or `!!!`, with arbitrary syntactic properties, and bound to arbitrary functions. This allows us to adapt the notation to the problem at hand, and at the same time to make the code shorter.

To the kernel of PURE, the only data structure is an expression, which consists of applications of functions. The default syntax for functions is `func x y z`, but a different syntax can be defined for each particular function. A function could be declared as prefix, infix, postfix, or a bracket ("outfix"). Expressions such as `x+y*2` or `|: x ... y :|` can be made valid with correct declarations of syntax for the operators `+`, `*`, `...`, and the brackets `|:`, `:|`. Once defined, these operators will be accepted by the PURE interpreter and will be automatically used to print the expressions.  We can write
```
(+) x ( (*) y 2 );
```
or we can write `x+y*2`; to PURE, these expressions are the same.

The programmable syntax feature in PURE is similar to that of Haskell. However, the maximum number of allowed precedence levels is 2^24 (compared with 9 in Haskell). Also, Haskell allows only user-defined infix operators, while PURE has also prefix, postfix, and outfix operators (brackets).

## Macros ##

Macros can be seen as a text replacement facility. Suppose in your code you have to repeat a certain construction many times:

```
...  foo (\x -> 2*a*x+1) a ...
... bar (\x -> 2*b*x+1) b ...
```

You can save time and avoid typing errors by defining a macro:

```
 def make_it f y = f (\x -> 2*y*x+1) y; 
```

Thereafter, `make_it foo a` will be replaced by `foo (\x -> 2*a*x+1) a` everywhere in the program text.

The macros in PURE are safer than, say, C preprocessor macros. For example, the definition of `make_it` uses the names `f`, `x`, and `y`. However, these names are encapsulated in the definition of `make_it`, so we can write `make_it f x` with correct results. (This feature is called **hygienic macros**.) For example, here is a definition of a macro that involves a function and a free symbol ("global variable"). :
```
> def add_y x = \z -> x+y+z;
> add_y 3 a;
3+y+a
> add_y z x;
z+y+x
> add_y y z;
y+y+z
```
The macro `add_y x` generates a function that adds `x+y` to its argument. As we see, the results are always as intended, even if we use this macro with symbols named `x`, `y`, and `z`.

The difference between macros and functions is that macros are not compiled but expanded at compile time, and the resulting code is then compiled. (This is the usual reason to introduce macros.) In this way, macros can be used for adding optimization to the code as well as for avoiding repetitive code.

## Term rewriting ##

PURE adopts an unusual view of computations, called "term rewriting." If you are not familiar with term rewriting, you may find this approach to programming new and refreshing.

Let us first talk about the main type of data that PURE manipulates.


### Symbolic (unevaluated) expressions ###

PURE's basic data type is an _expression_. Expressions are either direct values (such as `42` or `"A Text String"`), symbols without an assigned value (such as `z` or `this_time`), or _unevaluated function calls_.

**Symbols** are easy to understand: they are like variables that may or may not yet have a value. Assigning a value to a symbol is like "defining" a value for a mathematical variable; before that, the variable is kept in symbolic form. For example, if `x` is a symbol that has not been assigned a value, then `x+1` will remain `x+1` in PURE. Here is what it looks like in the interpreter:
```
> a = x+1;
> a;
x+1
> x = 4;     // now 'x' has a value
> a;
5
```

The expression `x+1` is an example of an **unevaluated expression**. (Since `x+1` is actually a call to the function `+` on the arguments `x` and `1`, we can refer to `x+1` also as an "unevaluated function call".) This is an important concept in PURE, so let us consider it in some more detail.

Unevaluated expressions may be familiar to you if you have ever used a computer algebra system (CAS) such as `Maple` or `Mathematica`. For example, `sqrt(2)` in a typical CAS will not give you `1.4142135...` as you might expect from your experience of working with a pocket calculator. Instead, the CAS will leave `sqrt(2)` unevaluated. Most CASes do this because they want to be able to simplify `sqrt(2)*sqrt(2)` into an exact number 2 without loss of precision. Most CASes will leave mathematical expressions such as `sqrt(2)` or `sin(5)` unevaluated, simply because there is no simpler formula for such numbers, while converting to a floating-point value will irrevocably lose precision.

Similarly, you can typically enter `f(2*x)` into a CAS without specifying what the function `f` is, and then evaluate the derivative of this with respect to `x`. The result will be an unevaluated expression such as `2*f'(2*x)` where, again, the function `f` and the value of `x` are not specified.

Like computer algebra systems, PURE will leave an expression in a symbolic (unevaluated) form if not enough information is available to evaluate it.

In PURE, an expression involving function applications, such as
```
func (x+19) 17;
```
could remain unevaluated for the following reasons:

  * Some function symbol (here `func`) was not defined and/or has no rewriting rules that can be applied in this particular expression.
  * Some function symbol is perhaps defined on specific values, but is now being applied to an unevaluated symbolic expression. In the example here, the subexpression `x+19` contains the function `+` applied to an unevaluated symbol `x`, thus `x+19` must remain unevaluated, -- and with it the whole expression stays unevaluated.
  * The user explicitly requested that the expression should be left unevaluated. (This can be done using the `quote` operation, which may be familiar to you if you know some LISP.) In this case, the expression could be later evaluated (if possible) using the `eval` operation.

A function application with specific values and defined function symbols, such as `1+2`, can be immediately evaluated (and usually is). If a subexpression can be evaluated, it usually will be. We will study this in more detail in a later tutorial.

### Term rewriting ###

**Term rewriting** means replacing parts of expressions with other expressions according to given rules. PURE regards the user's code as a set of "rules for replacement," i.e. a set of instructions for replacing input values with output values. For example, suppose that the user defines the following two functions,
```
 f1 x = 2*x+1;
 f2 x = x+y;
```
and then asks for the value of the expression `f1 (f2 z)` without defining the values of `z` and `y`. The result will be obtained like this: First, the subexpression `f2 z` will be replaced with `z+y`; the total resulting expression is `f1 (z+y)` at that point. Second, the expression `f1 (z+y)` will be replaced by `2*(z+y)+1`. At this point, any further evaluation will be impossible since `z` and `y` are not set equal to anything. Thus, the result of `f1 (f2 z)` is the unevaluated expression `2*(z+y)+1`. We could also say that this is a "symbolic expression" since it contains functions applied to values and symbols.

It must be stressed that the result, `2*(z+y)+1`, is not a number, not a function, and not a variable; rather, it is an **expression** that contains _symbols_ such as `z` and function names such as `*` or `+`. This expression cannot be simplified any further unless `z` and `y` are replaced by some numbers. If `z` and `y` have not been defined, PURE will simply leave the expression `2*(z+y)+1` as it is, _unevaluated_.

In this sense, the calculation shown here is a **symbolic** calculation, i.e. a calculation that operates with "symbolic" (unevaluated, or incompletely evaluated) expressions -- rather than with numbers or other structures made of numbers. One of the main design features of PURE is that it allows us to work directly with symbolic expressions such as `y*2`, where -- this needs to be stressed -- the symbol `y` is not set equal to any value.

In other words, unevaluated expressions are the basic data type in PURE, on which all functions operate.

The **term rewriting** model implemented in PURE means that an initial expression is transformed into a final expression by applying replacement rules. Rules for replacing expressions are called **term rewriting rules** because the application of a rule means erasing some part of the expression (some **term**) and inserting in its place some other term. Rewriting rules can be given by the user or can be built into the PURE system.

In the `f1`,`f2` example above, the rewriting rules consisted of replacing a _single_ function application by another expression. Therefore, such rules are "function definitions" in the sense of ordinary programming languages, i.e. we have simply defined the functions `f1` and `f2` for all possible arguments. However, PURE goes beyond that: It is possible in PURE to specify rewriting rules that apply only under certain conditions or apply only to certain kinds of arguments, or rewriting rules that involve _several_ functions at once.

For example, one could implement a "simplification" of the expression `2*(z+y)+1` if one expands the brackets. This is given by the rule
```
a*(b+c) = a*b + a*c;
```
Note that this rule cannot be interpreted as a definition of a function; rather, this rule is an example of **pattern matching**. In other words, this rule says that a particular pattern of functions and symbols is to be replaced by another pattern. This rule will apply to `2*(z+y)+1` and produce a "simplified" result, `2*z+2*y+1`. However, this rule will not apply to `(z+y)*2+1` since that pattern is formally different.

Rules that cover several functions at once is a feature of PURE that goes beyond what is available in standard functional languages such as Haskell.

Of course, PURE is not limited to expressions involving arithmetic functions; an expression can contain any function, including a "symbolic" (not yet defined) function. Through term rewriting and pattern matching, PURE allows one to program symbolic calculations in a flexible way.

PURE evaluates all expressions by repeated rewriting, using all rules that match a given expression. In the terminology adopted in computer science, an application of a rewriting rule is called a **reduction**, and the final result of rewriting (after all possible reductions) is called a **normal form**. In other words, a "normal form" is an expression that cannot be "simplified" or "evaluated" or "reduced" any further because no existing rewriting rules can be applied any more. A normal form may be a single-term expression like `1` or `x`, or it could be a complicated expression like `func (x+19) 17` shown above.

PURE makes no explicit distinction between functions for which rules exist ("defined" functions) and symbols without rules ("undefined" functions). Evaluation proceeds strictly according to the model of term rewriting; if there is a term involving a certain function and a rule that mathes that term, the rule will be applied and the function will be "evaluated." It is _up to the user_ to decide which functions will have rules and which expressions will remain unevaluated, and to write code accordingly.

### Data structures ###

The basic built-in data structure in PURE is a single-linked list structure. (Even though this structure can be defined in PURE itself, it is implemented instead in C++ for efficiency. PURE also has a few other "built-in" data structures, that is data structures implemented in C++, notably matrices and expression pointers.)

PURE does not provide any special way to build user-defined data structures (there is no analog of "structures" from C and "classes" from C++). Instead, user-defined data structures can be built in PURE using unevaluated ("symbolic") expressions.

Here is a simple example of a user-defined data structure. Suppose we write expressions such as `Point 1 2` or `Point x y`. According to the PURE syntax convention, `Point x y` is the application of a function `Point` to two arguments. However, let us suppose that we never give any rules for evaluating `Point x y` to a number, or for rewriting it to a simpler expression. Then our program will always keep these function calls unevaluated!

In a sense, the symbol `Point` then becomes simply a typographical separator, and the expression denoted by `Point x y` is no more than a pair `(x,y)` with a fancy label written next to it. (This is like in Haskell, except that the function symbol does not have to start with the capital letter. Let us, nevertheless, keep the capital letter in `Point` just in this example.)

We can interpret the expression `Point x y` in our program to mean "the point in the plane with coordinates _x_ and _y_". Now we can define rules that say what to do with the sum of two "Points", for example:
```
(Point x1 y1) + (Point x2 y2) = Point (x1+x2) (y1+y2);
```
We can define a function that will extract the _x_ coordinate of a point:
```
get_x (Point x1 y1) = x1;
```
In this way, we can define a new data structure and some operations on it.

In the terminology adopted in Haskell, the function symbol `Point` is called a **constructor** symbol. When we say that `Point` is a constructor symbol, we mean that the symbol is used syntactically as a function head but the function will never evaluate to anything by itself. There will never be any "equations" for the constructor symbol, i.e. any rules of the form
```
Point x y = ... ; // evaluate a Point to something else???
```
In other words, the symbol `Point` is a constructor symbol if it is used only to "construct" an unevaluated expression.

Unlike Haskell, there is no rule in PURE that the function symbol (`Point` in this example) should start with a capital letter. There is no difference in PURE between "constructor" symbols and ordinary function symbols. It is not necessary in PURE to declare constructor symbols (except for constructor symbols without arguments; those can be declared "nonfix" and used as names for an enumerated type). Haskell _needs_ the capital-letter rule to do things that PURE does not do. As the author of PURE says, "if you really need Haskell, you know where to find it."

### Object-oriented programming ###

The example shown above, with the data structure `Point x y` and the function `get_x`, may remind you of a typical example in object-oriented programming, where one defines a class "Point" with some "methods." It must be stressed that the basic computational model of PURE is term rewriting rather than object-oriented computation. Nevertheless, a certain object-oriented functionality can be implemented using rewrite rules and constructor symbols.

For example, the function `get_x` was defined above for objects of type `Point x y`, similarly to a method that might be defined for a class "Point". Note that the function `get_x` will remain undefined on anything else; for instance, `get_x 1` will remain unevaluated.

This kind of object-oriented facility is limited because, for instance, inheritance cannot be immediately implemented. If new data structures are defined later, for instance `Point x y z`, one will need to add a new rule for `get_x` to accomodate the new structure.

## Bindings to C code ##

PURE contains a direct facility to call functions from external C libraries. It is sufficient to write a function declaration such as `extern double foo(double x)`, link with the library, which can be done also at run time (`using "lib:/path/libfoo.so"`), and the function becomes available in your PURE script. The most frequently used C data types (`int`, `double`, `void*`), as well as matrix data types used in the GSL library, are directly mapped to PURE data types. External libraries can be written for manipulating PURE expressions through "expression pointers" (a special data type in PURE). There is also a "generic object" type -- essentially, a pointer to an arbitrary C structure manipulated by an external library. Thus, one can directly interface a PURE program with  existing libraries with a C interface. A number of interface modules have been developed, e.g. for GUI development (Gtk+), database (ODBC), POSIX-like operating system functions, and numerics (GSL).

## Compilation to native code through LLVM ##

When you type code in the PURE interpreter, your code is not interpreted line-by-line. Instead, your code is first translated to an intermediate form, which is a kind of CPU-independent assembly language defined by the LLVM engine. (LLVM, "Low Level Virtual Machine", is an optimizing compiler for a CPU-independent assembly-like language. LLVM is in active development and has become widely accepted among free software virtual machines. Google started a project to compile Python to LLVM, expecting a significant performance boost.) The PURE interpreter will then call LLVM to compile the intermediate code directly to CPU-specific machine code (this is called JIT, or "Just-In-Time", compilation). The resulting CPU-specific machine code will be run. The PURE interpreter will have to recompile your code every time you define some new functions or undefine old functions.

The LLVM assembly code can be also byte-compiled and run on the LLVM virtual machine, or it can be compiled to an object file (`.o`) if you need to make a library out of it. Thus the PURE code can be compiled through LLVM to standalone executable machine code for a particular CPU, or to machine-code object files (`.o`), or to any other target supported by LLVM.

If you compile your PURE code to a standalone executable, the running speed will not be very different since the same LLVM-compiled native code is being run. However, the startup time of a standalone executable may be significantly lower because the script doesn't have to be compiled at runtime.

## Multiple-precision numbers ##

PURE includes direct support of arbitrary-precision numbers through the GMP library. This may be useful for applications in computational mathematics and computer algebra. Right now the support of arbitrary-precision numbers is limited to integers.

## Lazy and eager evaluation ##

In the Haskell language, which fully adheres to the pure functional paradigm, the compiler will give an error message if a variable is assigned more than once. The Haskell compiler will also avoid evaluating the same _expression_ more than once. Indeed, there is no reason to do so; the result is guaranteed to be the same every time since no variables can change. Also, Haskell will not evaluate expressions whose result is not used. Haskell will evaluate only expressions whose results are needed as arguments of some function while evaluating some other expressions. If the result of an expression seems to remain unused, the expression will be simply ignored. For example, the argument `x` in the function `f` defined by
```
f x y = y+1;
```
is not used. Therefore, if we evaluate `f (1+a*b) y`, the Haskell compiler will not compile the code for evaluating `1+a*b`. It is safe in Haskell to omit expressions because expressions cannot have any side-effects.

This approach to evaluating expressions is called **lazy evaluation**. (If one wants to evaluate literally the same expression many times and discard all results, e.g. if one wants to measure the speed of the program, one needs to trick the Haskell compiler into thinking that each time a new expression is being evaluated and that each result is being used to evaluate the next result.)

Unlike Haskell, the PURE language adopts the **eager** or **strict** discipline of evaluation. PURE does not try to determine which expressions are actually needed to get the final results. PURE will evaluate _all_ given expressions as completely as possible, even if the result of one expression is discarded before evaluating the next one. PURE evaluates the arguments of functions _before_ evaluating the function. Of course, PURE evaluates expressions only as much as it is possible; an expression could remain unevaluated if, for instance, some symbol does not have a value.

PURE will also evaluate all the expressions that define local variables in a function before evaluating the function. The expressions will be evaluated in the order given. This feature can be used to perform actions with side-effects, such as reading or writing files, from within an expression because one can fully control the order of the side-effects.

### Streams (lazy lists) and thunks ###

It is also possible to have "lazy" evaluation in PURE, although the default behavior of PURE is eager evaluation.

In order to implement lazy evaluation of data structures, PURE uses the concept of a "postponed function" or a **thunk**. This is a function whose evaluation is suspended until some future time when the thunk is **forced**, i.e. actually evaluated.

This technique allows one to build structures whose evaluation is only partially completed, while the rest of the evaluation is postponed until the results are actually requested, i.e. to implement **lazy data structures**.

In the standard library, PURE implements **lazy lists**, also called **streams**. A stream is a list of _unknown_ length, such that only an initial portion of the stream (often, just the first element) has been already evaluated. Further elements of the list are waiting to be evaluated.

Streams can be used in several ways. Suppose that for some calculation we need a large set of random numbers, but we do not know in advance how many. One possibility is to define a stream of random numbers and then to evaluate successive elements of this stream when we need them. (Sometimes one reads that lazy lists are "infinite lists", but this is, strictly speaking, not true: of course, no infinite structures are actually present in memory.)

Streams are implemented using the mechanism for delayed evaluation of functions ("thunking"). Each new element of the stream is computed by applying a certain function to the previous elements. When a stream is defined, only a finite initial part of the stream is computed. The function that computes further elements is not evaluated right away. The evaluation of this function is explicitly postponed by the "thunking" operator `&`. In this way, the PURE program will perform more computations only when (and if) we need more numbers from the lazy list. The memory for the further elements of the stream will be allocated only if these elements are requested and evaluated (when "the thunk is forced"). The already computed elements of the stream will stay in memory (they are "**memoized**"), so they do not need to be recalculated if we decide to request them again.

PURE supports all basic functional operations with lazy lists, such as iterating over them or applying functions to them.


## Pointers and references ##

Functional languages avoid low-level techniques such as pointer manipulation or using references to objects. These techniques are sources of many programming errors and make programs difficult to write and to debug. In functional languages, most of the time one does not need pointers or references because all variables are unchangeable and all large enough objects are passed to functions by (constant) references automatically.

PURE encourages programming in the pure functional style. However, PURE does include a facility to declare a reference to a global variable, and then to change the value of that variable. There is also a possibility to obtain a pointer to a PURE expression.

## Files and I/O ##

It was realized quite early on that input/output operations cannot be represented directly in the pure functional paradigm. To understand why this is so, consider a program that reads two numbers from the user and prints the sum of these numbers. This program needs to use a function that gets some text from the user. If this function is called `gets` then code such as `a = gets` means that `a` is defined as "the line typed by the user." However, there is no such unique line; the user may type several lines. In the pure functional paradigm (and in Haskell), it is impossible to have a function, `gets`, that does not take any arguments but returns different results every time it is called. The return value of a function can depend only on the arguments of that function; if a function has no arguments, it must always return the same value (or so the Haskell compiler thinks). So the definitions `a=gets; b=gets;` are taken to mean that `a` and `b` are _always equal_ because `gets` has no arguments and thus is assumed to return the same value every time. Thus the Haskell compiler will optimize these definitions to something like `a=gets; b=a;`, which is of course not what was meant. (In the Haskell language, special techniques are used in this case to trick the compiler into thinking that several calls to `gets` need to be compiled in a certain order.)

The PURE language does not precisely follow the "pure" functional paradigm. In the PURE language, the definitions
```
line1 = gets;
line2 = gets;
```
do not mean that the values of `line1` and `line2` are always equal. However, PURE is a functional language quite different from C or Python. In C or Python, if we write code similar to that shown above,
```
// this could be C++
line1 = gets();
line2 = gets();
```
the code would read two _different_ lines from the user, one after another, and assign them to the variables `line1` and `line2`.

In PURE, the meaning of the definitions
```
line1 = gets;
line2 = gets;
```
is quite different. These definitions by themselves do not read anything from the user because they merely _define_ the function symbols `line1` and `line2`. These definitions do not evaluate anything but merely specify that any expression involving `line1` as well as any expression involving `line2` will be evaluated by replacing `line1` and `line2` with `gets`.

In PURE, a call to `gets` _always has the side effect_ of reading a line from the user. Thus, the expression `line1 + line2` will be exactly the same as `gets + gets`, and also equivalent to `line1 + line1`. However, no actual calls to `gets` will be executed until at least some _expression_ involving `line1` or `line2` is _evaluated_. The PURE compiler will evaluate any calls to `gets` in the order dictated by the evaluation of an expression, rather than by the order of definition of `line1` and `line2`.

Thus, we can have the following interactive session in PURE:
```
> using system;  // to make 'gets' available
> a=gets; // define `a`, read nothing
> b=gets; // define `b`, read nothing
> b+a+b; // will read 3 lines and concatenate
A
B
C
"ABC"
```
The result is `"ABC"` even though the expression evaluated was `b+a+b`. The reason is that both `a` and `b` are simply replaced by `gets` when `b+a+b` is evaluated. In some sense, the PURE compiler _does not know_ that `gets` has a side effect! The PURE compiler simply evaluates all expressions as given, using the given rewriting rules.

In fact, the precise order of evaluation of `b+a+b` is implementation-dependent. What if we need to guarantee the evaluation of several calls to `gets` in a particular order? There are two ways of fixing the order of input/output calls. One is to use the `when` clause -- because expressions in the `when` clause are _variables_, not functions, and are evaluated and assigned in the fixed order as given, left to right.
```
> using system;
> fff = b 
>  when a = gets; // the 1st `gets`
>     b = gets + a; // the 2nd `gets`
>  end;
> fff; // reads two lines and prints their reverse concatenation
A
B
"BA"
```
The resulting value, `"BA"`, shows that the call to `gets` written first is actually executed first, so `a` becomes `"A"`. Then `b` is reduced to `gets + a`, but by this time `a` has been already evaluated, so `b` becomes `gets + "A"` rather than `gets+gets`. After the second `gets` reads `"B"`, the value of `b` becomes defined as `"BA"`. Then the function `fff` returns this `b`.

The second way of fixing the order of evaluation is through the `$$` ("sequence") operator. For example,
```
> using system; // to make 'puts' and 'gets' available
> bbb = puts "Unit:" $$ gets;
> "1 " + bbb; // prints "Unit:" and reads one line
Unit:
kg
"1 kg"
```
The `$$` operator in `x $$ y` forces the evaluation of `x`, discards the value, and then forces the evaluation of `y`.

Note that in both cases _nothing is read or printed_ immediately as the functions `fff` and `bbb` are defined! These definitions merely label the necessary input/output actions by the symbols `fff` and `bbb`. The input/output actions will be actually performed only when we request to evaluate an expression involving these symbols.

**Question:** While discussing these examples, you said that `line1`, `line2`, `fff`, and `bbb` are _functions_ that you define through `gets` and `puts`. But they look like _variables_ to me. Why is it that the PURE statement "`line1=gets;`" does not define a _variable_ called `line1`?

**Answer:** When you are new to PURE, there is indeed a danger of confusing variables with functions. A PURE statement such as "`a=...`" can define a variable only if it appears within a `when ... end` clause; this clause is limited to variables, just as the `with ... end` clause is limited to functions. In any other case, you are not within an expression, in other words you are at the "**top level**" (perhaps, within some module or within the main program). At top level, the statement "`a=...`" defines a function, called "`a`", with no arguments. You can define variables at top level using the constructions "const a=..." (a constant value) and "`let a=...`" (a potentially mutable value).

For this reason, indeed `line1` and `line2` are _functions_, not variables, if we define them as shown in the example above! These functions have no arguments and probably look like variables to you, but in PURE they are not variables. The PURE statement "`line1=gets;`" at top level is a _rewrite rule_. Therefore, `line1` is a symbol for which rewrite rules are available, i.e., it is a function. The statement "`line1=gets;`" gives a rewriting rule for this function and does not require an immediate evaluation of the right hand side of the rule, like with any other function.

To define variables, we would have to write "`const a=gets;`" or "`let a=gets;`. You cannot write this inside an expression unless you are in the `when ... end` clause; so you can write "`const a=...`" or "`let a=...`" only at "top level", either in the interactive session or in your script.

If you try "`let a=gets;`" or "`const a=gets;" in an interactive session, the PURE intepreter will evaluate a call to `gets` right away, because "`a`" is declared as a variable, and therefore its value needs to be evaluated right away! You will see this evaluation happening because the interpreter will pause until you enter a line:
```
> let a=gets; // interpreter waits for you
 I typed this
> a;
" I typed this"
```