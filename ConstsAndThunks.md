## Introduction ##

Sometimes there are several subtly different mechanisms in Pure where there is
only a single notion in other languages. This page attempts to explain the key
differences.

Much of the explanation is drawn from the Pure manual (often verbatim),
or from a mailing list thread "Top-level definitions", 14 June 2012.


## Difference between local and global definitions of variables and functions ##

You can bind a global variable to a new value by just entering a corresponding
let command:
```
> foo x = c*x;
> foo 99;
c*99
> let c = 2; foo 99;
198
> let c = 3; foo 99;
297
```
This works like global variables in imperative languages, but note that in Pure
the value of a global variable can only be changed with a let command at the
toplevel. Thus referential transparency is unimpaired; while the value of a
global variable may change between different toplevel expressions, it will
always take the same value in a single evaluation.

Similarly, you can also add new equations to an existing function at any time.

So whereas with _local_ symbols, the meaning never changes once their
definition has been processed, _toplevel_ variable and function definitions may
evolve while the program is being processed, and the interpreter will always
use the latest definitions.

A consequence of this is that---both in the interpreter and in batch-compiled
scripts---you have to define all symbols needed in an evaluation before
entering the expression to be evaluated.


## Difference between global variables, consts, nullary macros ##

<ul><li>
The toplevel variable definition:<br>
<pre><code>let PATTERN = EXPRESSION;<br>
</code></pre>
binds every variable in PATTERN (if the value of EXPRESSION matches PATTERN).<br>
This binding is used by all free/global variables in any (existing or future)<br>
function definitions or in any (future) variable definitions.<br>
<br>
Unlike constants, variable definitions introduce bindings dynamically or at runtime: a<br>
referring occurrence of the variable denotes the value the variable has where<br>
that occurrence is evaluated.<br>
<br>
A variable can be (but needn't be) redefined, both in the interpreter and in<br>
batch-compiled mode. But---as noted above---since it can only be defined at the<br>
toplevel, it has a single value when evaluating any single toplevel expression.<br>
<br>
</li><li>
On the other hand, a "nullary" or parameterless macro:<br>
<pre><code>def alpha = EXPRESSION;<br>
</code></pre>
introduces a compile-time binding. Its defining EXPRESSION (or more carefully,<br>
the macro-expansion of that EXPRESSION) will be inlined into any future<br>
function or variable definitions.<br>
<br>
Unlike variables, macro definitions can't be changed, at least not in<br>
batch-compiled code. In the interpreter, you <i>can</i> <code>clear</code> a macro definition,<br>
and bind the cleared identifiers to new macros (or other sorts of definitions).<br>
But unlike variables, this won't have any effect on existing definitions that<br>
used the macro.<br>
<br>
Examples:<br>
<pre><code>&gt; foo x = x + alpha;  // a function definition that pre-dates the macro<br>
&gt; def alpha = id expanded;<br>
<br>
&gt; show foo  // existing definitions are unaffected<br>
foo x = x+alpha;<br>
&gt; foo 1;    // even when they're applied<br>
1+alpha<br>
&gt; eval ans; // but alpha's expansion is used by future evaluations<br>
1+expanded<br>
<br>
&gt; let v = 20; // variable<br>
&gt; bar x = x + alpha + v;<br>
&gt; show bar  // alpha's expansion is inlined into future definitions<br>
bar x = x+id expanded+v;<br>
&gt; clear alpha // so it persists even if alpha's binding is cleared in the interpreter<br>
&gt; show bar<br>
bar x = x+id expanded+v<br>
&gt; // bar remembers the original definition of the macro but honors updated variable bindings<br>
&gt; let v = 30;<br>
&gt; bar 1;<br>
1+expanded+30<br>
</code></pre>

</li><li>
<code>const</code> definitions are semantically very similar to nullary macros. They also<br>
introduce compile-time bindings, and (at least sometimes) are inlined into<br>
future function or variable definitions.<br>
<br>
If we run the same sequence as above with a <code>const</code> definition of <code>alpha</code> instead of a nullary macro, we get similar results:<br>
<pre><code>&gt; foo x = x + alpha;<br>
&gt; def alpha = id expanded;<br>
<br>
&gt; show foo<br>
foo x = x+alpha;<br>
&gt; foo 1;<br>
1+alpha<br>
&gt; eval ans;<br>
1+expanded<br>
<br>
&gt; let v = 20;<br>
&gt; bar x = x + alpha + v;<br>
&gt; show bar<br>
bar x = x+expanded+v<br>
</code></pre>

This is the first difference: the const <code>alpha</code> is bound to the result of<br>
<i>evaluating</i> <code>id expanded</code> (namely, just <code>expanded</code>), whereas the macro was<br>
bound to that whole unevaluated expression.<br>
<br>
Not every const definition can be fully evaluated at compile time (for<br>
instance, <code>const p = malloc 10;</code> can't be, nor can definitions that assign a<br>
local closure). But they are evaluated then so far as they can be.<br>
<br>
This means that a complex computation, such as:<br>
<pre><code>const some_primes = take 1000 ...;<br>
</code></pre>
will be performed at compile time and the value stored in a batch-compiled<br>
executable. If one prefers these values to be computed at runtime instead<br>
(giving smaller executable code but a longer startup time), that can be<br>
specified with the <code>--noconst</code> pragma/command-line option (or PURE_NOCONST<br>
environment variable).<br>
<br>
Another subtle difference here is that although the interpreter's <code>show</code>
command always <i>displays</i> <code>alpha</code>'s value as having been inlined---and with macros<br>
it always is---with consts the inlining is in fact only performed with<br>
scalar values (numbers, strings, pointers). When the const is bound to other<br>
values, such as aggregates or even bare symbols, it's instead compiled as a<br>
read-only global variable. But these are compiler optimizations the Pure<br>
programmer can ignore. The difference between cases where a const has been<br>
inlined and cases where it hasn't is visible to introspective facilities like<br>
<code>get_fundef</code>. But those facilities aside, the interpreter and batch-compiled code<br>
will always <i>treat</i> the const as though it were inlined. Even when one clears<br>
the const binding in the interpreter:<br>
<pre><code>&gt; clear alpha<br>
&gt; show bar<br>
bar x = x+expanded;<br>
&gt; bar 1;<br>
1+expanded<br>
</code></pre>
There is a bug in the interpreter affecting the last line; this will be fixed in<br>
Pure >= 0.56. The intended behavior is for <code>bar</code> in these cases to behave just as<br>
displayed.<br>
<br>
A third difference between consts and nullary macros arises when the<br>
symbols with those bindings have also been syntactically declared as<br>
<code>nonfix</code> operators. More on this below. When a nonfix with a macro binding<br>
appears in a pattern, just the nonfix symbol is used; but<br>
when a nonfix with a const binding appears in a pattern, that const's<br>
value is inlined (or at least interpreted as inlined).<br>
<br>
Examples:<br>
<pre><code>&gt; let x = 100;<br>
&gt; const c = [x,y,z]; // this will use the current value of x, but y and z will be unevaluated symbols<br>
&gt; let y = 200;<br>
&gt; show c<br>
const c = [100,y,z];  // notice y is still unevaluated in c<br>
&gt; c;<br>
[100,y,z]<br>
&gt; eval ans;<br>
[100,200,z]<br>
<br>
&gt; foo x = case x of c = c end; // here c is just treated as a pattern variable<br>
&gt; show foo<br>
foo x = case x of c = c end;<br>
&gt; foo 1;<br>
1<br>
<br>
&gt; nonfix c;<br>
<br>
&gt; clear foo<br>
&gt; foo x = case x of c = c end; // but here c's value is inlined into the pattern<br>
&gt; // and, because c is no longer bound by the pattern, also into the rhs expression<br>
&gt; show foo<br>
foo x = case x of [100,y,z] = [100,y,z] end;<br>
&gt; foo [100,201,300];<br>
[100,y,z]<br>
&gt; // notice that y is still unevaluated in the result<br>
&gt; // interestingly, the y on the lhs of the case rule doesn't bind the y on the rhs<br>
&gt; // neither is the y on the rhs bound by the global variable binding for y<br>
&gt; eval ans; // unless we re-evaluate the expression we got back<br>
[100,200,z]<br>
&gt; foo 1;<br>
&lt;stdin&gt;, line 19: unhandled exception 'failed_match' while evaluating 'foo 1'<br>
<br>
&gt; // contrast nonfix nullary macros<br>
&gt; def m = [x,y,z];<br>
&gt; nonfix m;<br>
&gt; clear foo<br>
&gt; foo x = case x of m = m end;<br>
&gt; show foo<br>
foo x = case x of m = [x,y,z] end;<br>
&gt; // m is used symbolically on the lhs of the case clause<br>
&gt; // and its defined value is substituted on the rhs<br>
&gt; // the rhs expansion displays here as [x,y,z], but it will honor global bindings for those symbols<br>
&gt; foo 1;<br>
&lt;stdin&gt;, line 6: unhandled exception 'failed_match' while evaluating 'foo 1'<br>
&gt; foo ('m);<br>
[100,200,z]<br>
</code></pre>

For more about this behavior, see Quoting, especially<br>
<a href='http://code.google.com/p/pure-lang/wiki/Quoting?#9._Pattern_variables_and_literals'>Section 9</a> and<br>
<a href='http://code.google.com/p/pure-lang/wiki/Quoting#11._Matching_named_functions'>Section 11</a>.<br>
<br>
</li><li>
Nonfixity is a syntactic notion. Punctuation sequences won't be parsed at all until they're declared as operators; and nonfixes are one kind of operator to be.<br>
Alphanumeric symbols can be declared as operators too (else they will be parsed as identifiers).<br>
<br>
Nonfixes are more like identifiers than other operators are: for example, even<br>
when they're composed of punctuation, nonfixes don't need to be escaped with<br>
parentheses in contexts where other operators do (think of <code>[]</code>).<br>
<br>
Along with other operators, but in contrast to identifiers, nonfixes will<br>
always be interpreted symbolically in patterns. Identifiers are only<br>
interpreted symbolically in patterns in certain cases---notably when they're in<br>
head positions.<br>
<br>
Semantically, nonfixes and other operators behave like other symbols. They can<br>
be bound to rules, or they can be used as global variables or constants.<br>
However, if they're to be used in the latter ways, that has to be done <i>before</i>
declaring them as a nonfix (or other kind of operator). For the lhs of a const definition is a<br>
pattern---that's why you can say:<br>
<br>
<pre><code>const [a,b] = [10,20]; // will bind the pattern variables a and b<br>
</code></pre>

And recall that once c has been declared a nonfix, <i>it will be<br>
interpreted symbolically</i> in a pattern, rather than as a pattern variable.<br>
Hence this will be a match failure:<br>
<br>
<pre><code>nonfix c; const c = 100;<br>
</code></pre>

because <code>100</code> doesn't match the symbol <code>c</code>.<br>
<br>
If you want to use <code>c</code> as a nonfix const, you instead need to <i>first</i> specify<br>
the const definition, and <i>then</i> declare <code>c</code> as a nonfix.<br>
<br>
</li></ul>




## Difference between global variables and (different kinds of) "thunk" ##

There are several kinds of construction in Pure that programmers call "thunks,"
though Pure itself only applies this term to one of them. In some respects these are
similar to global variables and to each other, and in other respects they all differ.

Let's consider:

```
let beta = succ 0;    // ordinary global variable, with an int value
let gamma = succ 0 &; // this variable takes what Pure calls a thunk value
delta = succ 0;
zeta () = succ 0;
const eta = \() -> succ 0;
```

First, let's address the last two of these. `eta` differs from `zeta` only in a
few respects: (1) `zeta`'s rhs is permitted to recursively refer to functions
that aren't yet defined, whereas `eta` can only refer to functions that have
already at least partly been defined. (If you want an anonymous closure to be
able to refer recursively to itself, use the `fix` combinator.) In this case,
`zeta`'s actual rhs doesn't make use of recursion. (2) `zeta`'s definition can
still be extended, but `eta`'s can't. However, we can imagine that `zeta` was
defined after a `--defined zeta` pragma, or that we gave it an additional rule
that catches everything so far unmatched and throws `failed_match`. That would
bring `zeta` and `eta`'s behaviors closer to each other. (3) `zeta` and `eta`
behave somewhat differently wrt pattern-matching. In particular, `zeta` matches
against symbolic uses of `zeta` in a pattern, whereas `eta` will only match
against the throwaway pattern variable `_`. This is discussed further in
[Quoting Section 11](http://code.google.com/p/pure-lang/wiki/Quoting#11._Matching_named_functions).
(4) Lastly, `zeta` and `eta` are printed
differently by the interpreter, and are discernable by `show` and to Pure's
introspective facilities.

For most purposes, however, a programmer could use `zeta` and `eta` interchangeably.

Now, what about the differences between the other four?

Well, the expression assigned to the variable `beta` will be evaluated only once, at the point
when the definition is executed.

The body of the thunk assigned to the variable `gamma` will also be evaluated only
once, and memoized, but at a later point, not yet when the definition is
executed. For some type-checking purposes, `gamma` will count as satisfying the
`thunk` type when it's first defined, and then later after it's implicitly or
explicitly forced, it will count as satisfying whatever type its delayed value
is---in this case, `int`.

Note that the `&` postfix operator binds more tightly than all standard operators, even quote.
So while `succ 0 &` is parsed as `(succ 0) &`, that's only because the application of
ordinary functions binds even more tightly still. If you want to say something like
`(1 + 2) &`, you'll have to explicitly include the parenthesis to get the desired parsing.

`delta` and `zeta` are regarded by the interpreter as functions, not variables. This
is manifest in how they're presented by `show`, and in the results of
`get_fundef ('delta) 0` and the like. However, whereas `nargs zeta` returns 1,
and `zeta` satisfies the type `fun` (and so too its supertypes `closure` and
`function`), `nargs delta` doesn't return 1, nor even 0. And `delta` doesn't satisfy
any of the types `zeta` does. Indeed for runtime purposes, with `delta` you can only
get at either the symbol `delta`, or whatever value is returned by the nullary
function that symbol is bound to. The nullary function itself is not a value you
can refer to or pass around.

However, `delta` and `zeta` are alike in that each time they are applied---either by
evaluating `zeta ()` or by evaluating a bare occurrence of `delta`---the bodies
of their definitions are evaluated anew. If those computations take a long time, it
will take a long time for each evaluation of `zeta ()` or `delta`. If those computations
have side-effects, they will be invoked each time.

One case the Pure manual describes in which you may want to use nullary
functions like `delta` is with the `reduce` construction. This dynamically binds
symbols in an expression being reduced with local rules specified in an attached
`with` clause. It _doesn't_ substitute in local variables from a `when` clause; if
you want to dynamically bind a symbol, you have to do so in the attached
`with` clause. Hence:

```
reduce EXPRESSION with
  delta = succ 0;
end;
```

would dynamically bind free occurrences of the symbol `delta` in EXPRESSION to the expression
`succ 0`. This expression may be evaluated afresh several times in EXPRESSION. If you wanted
to avoid that, because `delta` had a more expensive rhs or one whose side-effects you
didn't want to repeat, you'd have to:

```
reduce EXPRESSION with
  delta = d;
end when
  d = succ 0;
end;
```

## More details about `succ 1 &` and streams ##

Finally, here are some more details about the kinds of values one creates using `&`, and
that Pure's documentation _calls_ "thunks".

These can be eval'd and assigned and subjected to some (non-deconstructing) pattern matches
without forcing them. They can also be subjected to some type tests without forcing them.

But other operations on them automatically force the thunk. I got the
impression from the documents that passing them as an argument to any C call
would do so, however this is not so. You can at least apply `===` to a thunk
without forcing it. However any pattern-matching that needs to inspect the
structure of the matched value with force the thunk. This is so even in cases
where you'd think the thunk had already been caught:


```
// this will completely force a stream, despite the check for thunks coming first
forced_len xs = len xs 0 with
    len xs n     = n if thunkp xs;
    len [] n     = n;
    len (x:xs) n = len xs (n+1);
end;

// but this won't
unforced_len xs = len xs 0 with
    len xs n     = n if thunkp xs;
    len xs n     = case xs of
                     []  = n;
                     (x:xs) = len xs (n+1);
                   end;
end;
```

Moreover, merely _adding_ a thunk to a comma-tuple will force it, even before you perform any pattern-matching on the result. (This is because `(,)` does such pattern-matching internally, to make sure comma-tuples are always "flat.")

Thunks can also be explicitly forced with `force`.

Unforced thunks won't be recognized as being of type `closure`, but will be
recognized as being of type `thunk`. Neither of those type tests force the thunk.
Neither do some primitive type tests like `int` or `string`. (But I'm not sure that
non-forcing is consistent with what comes next.) However, if a
thunk is type-checked against the types `function` or `appl` or `list` or the like,
it will be forced and the result of the check will depend on what is the type
of the value the thunk forces to.


Thunks are used to build Pure's streams. These are "odd streams" in the sense of:

  * Philip Wadler, Walid Taha, and David MacQueen. "How to add laziness to a strict language, without even being odd", Workshop on Standard ML, Baltimore, September 1998
  * http://srfi.schemers.org/srfi-45/

unlike the "even streams" advocated in those sources, and used in Haskell.

Most of Pure's list operations like `map` and `take` and `foldr` operate in a
lazy way on streams, and return stream results, forcing only the minimum
number of cells required. But some list operations (`#`, `reverse`, `foldl`)
are inherently eager and will force an entire stream.

Pure's list functions never produce any genuinely cyclic structures in the underlying
implementation (and if they did, Pure wouldn't know how to GC them). Functions like
`cycle [1]` and `fix (1:)` work as expected, returning stream results, but
memory usage may increase linearly as they are traversed.

When a stream is bound to a variable, its already-computed prefix is memoized.

If a thunk, or a structure it's part of, is bound to a variable whose binding
is still visible, then its forced value will be memoized and won't need to be
recomputed. But neither will that space be released, and sometimes you'll want
to avoid the memoization. For example, if you're traversing a stream and it's
assigned to a local variable, you'll want the traversal to be invoked as a tail
call. Otherwise, the whole initial part of the stream will be kept in memory
during the traversal, giving nasty memory leaks. Example:

```
// notice all_primes is a nullary function like delta
all_primes      = sieve (2..inf) with
  sieve (p:qs)  = p : sieve [q | q = qs; q mod p] &;
end;

> let P = all_primes;
> P!!(0..20);
[2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73]
> P!299;
1987
```

You can also just print the entire stream. This will run forever, so hit
Ctrl-c when you get bored:

```
> using system;
> do (printf "%d\n") all_primes;
2
3
5
...
```

Make sure that you really use `all_primes` instead of the `P` variable when
executing `do (printf ...) ...`. Otherwise, because of memoization the stream
stored in `P` will grow with the number of elements printed until memory
is exhausted. Calling `do` on a fresh instance of `all_primes` instead
permits `do` to discard each forced cell after printing the corresponding
element.


Laziness and side effects don't go well together, as most of the time
you can't be sure when a given thunk will be executed. So as a general
guideline you should avoid side effects in thunked data structures. If
you can't avoid them, then at least make sure that all accesses to the
affected resources are done through a single instance of the thunked
data structure. E.g., the following definition lets you create a stream
of random numbers:

```
> using math;
> let xs = [random | _ = 1..inf];
```

This works as expected if only a single stream created with `random`
exists in your program. However, as the `random` function in the `math`
module modifies an internal data structure to produce a sequence of
pseudorandom numbers, using two or more such streams in your program
will in fact modify the same underlying data structure and thus produce
two disjoint subsequences of the same underlying pseudorandom sequence
which might not be distributed uniformly any more.