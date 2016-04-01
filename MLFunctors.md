# ML-style Functors in Pure #

OCaml (and other ML-type languages) organizes its types and definitions into
_modules_. And it has a powerful facility called _functors_ which basically are
functions that map one or more modules to a new module. This permits one to
write generic algorithms for say a Set, parameterized on some variable type of
element and ordering relation on those elements. Then when one wants a
particular implementation of this, say a Set module for integers, one just
applies the Set functor to a module that exposes the `int` type and its ordering
relation in the right way, and one gets one's Set module for integers as a
result. Applying the Set functor to a different module might give one a Set
module for strings, compared in the usual way. Applying it to a different
module might give one a _different_ Set module for strings, compared in
different way---perhaps case-insensitively.

The ability to parameterize on a single type in multiple ways is an advantage
that ML-style functors have over Haskell's typeclasses.

These capabilities are important for generic ML programming, but much of
their effect can be more naturally achieved in a dynamically-typed language
like Pure more straightforwardly. One doesn't have to say in advance whether
one's implementing a Set of integers or a set of strings---or whether one's
using, say, a list of integers or a list of strings to do so. One can just
build up a list of elements of whatever type one's given, perhaps even elements
of heterogeneous types. And one might just use a generic ordering relation like
`<`, and trust it to be defined on all the element types one's given.
Alternatively, one could expect the client to supply her own ordering relation,
appropriate to the elements the Set will contain. There are a variety of
strategies here, more or less approximating what one does in ML with functors.

I will summarize these strategies and point out their limits, and then describe
how something closer to what's in ML might look in Pure. I'll say what that
would make possible that isn't already. Whether it'd be _better_ for the
language to evolve to include this extension, or whether it'd be better to just
find ad hoc workarounds for the limits with the existing strategies, I'm not
sure. But I thought I'd write up the problem space for others to think about
and discuss too.

Let our example use-case be the two Sets of strings described in the first
paragraph. One would like to have some generic Set constructors and operations
that permitted one sometimes to work with strings compared in the usual way,
sometimes with strings compared case-insensitively. (We don't here consider the
possibility that one might want a single structure sometimes able to be used in
either way.)

First we'll look at how one does this in OCaml.

## OCaml Example ##

A brief overview of OCaml syntax: It's quite similar to Haskell's and Pure's
with the salient differences that:

  * in OCaml, `:` divides values from their types and `::` is the list constructor.
  * `match value with | pat1 -> val1 | pat2 -> value2` is OCaml's version of Haskell and Pure's `case` construct.
  * `let pat = expr in value` is OCaml's version of Pure's `value when pat = expr end`
  * in Haskell, one writes the type of a list of `t`s as `[t]`; in OCaml it's instead `t list`. In Pure, of course, one by default only has `rlist`. (But one could define a type predicate for a list of a specific type of element if one wanted to.)
<ul>
<li>in OCaml, one writes the implementation of a module/struct as:<br>
<pre><code>module Foo = struct<br>
  ...<br>
  let f = ... (* defining a function *)<br>
end;;<br>
</code></pre>
and one writes its type as:<br>
<pre><code>module type FOO = sig<br>
  ...<br>
  val f : ... (* specifying f's type *)<br>
end;;<br>
</code></pre>
</li></ul>
  * as I said, a "functor" is a module-constructor, that is, it takes one (or more) modules as arguments and returns a new module/struct as a result.


So here's the OCaml code:

```ocaml

(* type that has to be satisfied by the modules we parameterize on *)
module type COMPARABLE = sig
type t
val (<) : t -> t -> bool
end;;

(* type of the module we construct *)
module type SET = sig
type element   (* abstract types for the Set and its elements *)
type set       (* the concrete implementations are left private *)
val empty : set                    (* type of a constant *)
val insert : set -> element -> set (* types of two functions *)
val member : set -> element -> bool
end;;

module MakeSet = functor (Elt: COMPARABLE)
(* we take a module Elt of module type COMPARABLE as an argument *)
-> struct
(* and we return a structure implemented like this *)
type element = Elt.t
type set = element list
let empty = []
let rec insert s x = match s with
| [] -> [x]
| y::ys -> if Elt.(<) x y then x :: s (* x is smaller than every y *)
else if Elt.(<) y x then y :: insert ys x
else s (* x is already in s *)
let rec member s x = match s with
| [] -> false
| y::ys -> if Elt.(<) x y then false (* x is smaller than every y *)
else if Elt.(<) y x then member ys x
else true
end : SET with type element = Elt.t;;
(* this is the type of the result *)
```

So now how do we use this? Well, we'll prepare two modules to use as arguments
to the MakeSet functor:
```ocaml

(* here is a case-sensitive set of strings *)
module PlainStrings = struct
type t = string
let (<) x y = x < y
end;;

(* here is a case-insensitive set of strings *)
module CaselessStrings = struct
type t = string
let (<) x y = (String.lowercase x) < (String.lowercase y)
end;;
```

and we generate our constructed modules like this:

```ocaml

module StringSet = MakeSet (PlainStrings);;
StringSet.member "bar" (StringSet.add "Bar" StringSet.empty);;
(* result is: false *)

module CaselessStringSet = MakeSet (CaselessStrings);;
CaselessStringSet.member "bar" (CaselessStringSet.add "Bar"
CaselessStringSet.empty);;
(* result is: true *)
```

Notice that OCaml permits us to bind the constructed modules to names (to the
names `StringSet` and `CaselessStringSet`). For years, modules were just
second-class values in OCaml. Recently, they became first-class values and
this brought some interesting extra power to the language. That isn't important
to what we're discussing here, and it's not exemplified in this example. I
just flag the binding that is present here here because some of the strategies
we discuss below won't even emulate that.


Alright, so how might one accomplish the same thing in Pure?

## Existing Pure Strategies ##

Here is one strategy:
```
namespace Set1;
const empty = [];
insert lt []       x = [x];
insert lt s@(y:ys) x = x:s if lt x y; // x is smaller than every y
                     = y:insert lt ys x if lt y x;
                     = s otherwise; // x is already in s
member lt []     x   = false;
member lt (y:ys) x   = member lt ys x if lt y x;
                     = ~(lt x y) otherwise;
```

This works fine, but it requires the client to supply the ordering relation on every use of a set operation. That's a pain for the client to keep track of, and plus it opens up the possibility for the client to substitute a different ordering relation at inappropriate times. (You could argue the client who does so gets what he deserves, but it also means there is an additional burden on the client to know when giving a different argument to this parameter is appropriate and when not.)

The client may want to define some closures which partially applied the functions `insert`, `member` and so on to the parameters he's then operating with. Or our Set library could do that in advance. But then the client needs to keep track of one group of `insert` and `member` functions for his plain string Set, and a different set for his case-insensitive string Set. This isn't the usual flavor of programming in a dynamic language.

So a different strategy would be to bundle the ordering relation into the structure when we first construct it.

Here are three ways to do that:
```
namespace Set2;
nonfix Cons Nil;
type set (Nil _) | set (Cons _ _ _::set);
makeempty lt                = Nil lt;
insert s@(Nil lt)         x = Cons lt x s;
insert s@(Cons lt y ys)   x = Cons lt x s if lt x y;
                            = Cons lt y (insert ys x) if lt y x;
                            = s otherwise;
member (Nil _)            x = false;
member (Cons lt y ys)     x = member ys x if lt y x;
                            = ~(lt x y) otherwise;
```
```
namespace Set3;
nonfix S;
type set (S _ _::list);
makeempty lt                = S lt [];
insert (S lt [])          x = S lt [x];
insert s@(S lt yy@(y:ys)) x = S lt (x:yy) if lt x y;
                            = S lt (y:insert (S lt ys) x) if lt y x;
                            = s otherwise;
member (S _ [])           x = false;
member (S lt (y:ys))      x = member (S lt ys) x if lt y x;
                            = ~(lt x y) otherwise;
```
```
namespace Set4;
nonfix S;
type set (S _ _::list);
makeempty lt                = S lt [];
insert (S lt ys) x = S lt (insert ys x) with
         insert []        x = [x];
         insert yy@(y:ys) x = (x:yy) if lt x y;
                            = (y:insert ys x) if lt y x;
                            = yy otherwise;
end;
member (S lt ys) x = member ys x with
         member []        x = false;
         member (y:ys)    x = member ys x if lt y x;
                            =           ~(lt x y) otherwise;
end;
```

As we see, the functions in `Set2` include the ordering relation in each cell
of the structure. That seems quite wasteful, wouldn't it be more economical
just to include the ordering relation once per structure? Well, that's what
`Set3` does instead. But then that makes `Set3` uneconomical in a different way.
For notice that when the member function in `Set2` recurses, it can do so just on
the already-constructed argument `ys`. Whereas the corresponding function in `Set3`
doesn't have a Set containing just the `ys` to recurse on, so it has to construct
one: it says `member (S lt ys) x`. In other words, every time we traverse the
Set, we'll need to create (and then generally immediately release) a new cell
for each step in the traversal. Each such operation will be simple, but
altogether it may end up taxing the GC more than we'd like.

`Set4` avoids both pitfalls by only storing the ordering relation once per set,
but then not passing the sets themselves to its recursive functions. So there
is in each case a pair of functions: one exposed to the client that accepts
Sets, and an unexposed local function that works more efficiently on the
implementing components. In this case it works out pretty nicely. In a case
where we had more complex parameterizations, it would be messier.


A fifth strategy is to just define a new element type whenever one wants to order elements in a new way. So we might define a type:
```
type istring (I s::string);
```
and then make sure that the generic function `<` is defined on `istrings` in a
case-insensitive way. But then each of our `istrings` would have a extra cell
wrapped around it, and if we had to deal with a lot of them, this might also be
more wasteful than we'd like. Not as wasteful as the first strategy we
described (that one involved an extra cell _plus_ a pointer to the ordering
relation). But depending on the use, perhaps still undesireably wasteful.

## A functor macro for Pure ##

A sixth strategy is to use Pure namespaces as organizing blocks like ML
structures, and to create a macro that takes a namespace name as an argument,
and declares a new paramterized namespace as a result. Here is one way to do
it:

```pure

#! --quoteargs functor
def functor name::var (ts __with__ body) = eval m when
type_decls = types ts;
body = __str__ $ '(0 __with__ body); //'
n = sprintf "namespace %%s with %s using namespace %%s; %s; end;" (join " " type_decls, body!!(7..#body-5));
m = sprintf "def %s M::var S::var = eval (sprintf %s (str M, %s, str S));" (str name, str n, join ", " (repeatn (#type_decls) "str S"));
end with
type1 t = sprintf "type %s = %%s::%s;" (str t, str t);
types (t,ts) = type1 t : types ts;
types t = [type1 t];
end;
```

One uses it like this:

```
functor MakeSet(t with
   // The implementation will use type t and function (<) from the supplied namespace.
   empty = [];
   insert []       x::t = [x];
   insert s@(y:ys) x::t = x:s if x < y;
                        = y:insert ys x if y < x;
                        = s otherwise;
   member []       x::t = false;
   member (y:ys)   x::t = member ys x if y < x;
                        = ~(x < y) otherwise;
end);

// these are the namespaces we use as arguments to the functor
namespace PlainStrings with
   type t = string;
end;

namespace CaselessStrings with
   type t = string;
   extern char tolower(char);
   private lowercase;
   lowercase s = foldl1 (+) $ map (\c -> chr $ tolower $ ord c) s;
   infix (::<) <;
   x < y = lowercase x ::< lowercase y;
end;

// here we generate our constructed namespaces
MakeSet StringSet PlainStrings;
MakeSet CaselessStringSet CaselessStrings;

// here we try them out
StringSet::member (StringSet::insert StringSet::empty "Bar") "bar" // false
CaselessStringSet::member (CaselessStringSet::insert CaselessStringSet::empty "Bar") "bar" // true
```

How did that work? When we said this:

```
functor MakeSet(t,t2,... with
    /* template */
end)
```

we generated a macro `MakeSet` that takes the name of a new namespace and of a
existing namespace as arguments, and generates the new namespace using the
`/* template */` parameterized on types and functions from the specified
existing namespace. Here's that macro in action:

```
MakeSet CaselessStringSet CaselessStrings;
```

will generate the following:

```
namespace CaselessStringSet with
  type t = CaselessStrings::t;
  using namespace CaselessStrings;
  empty = [];
  insert [] x::t = [x];
  ...
end;
```


There are just two or maybe three limitations with this technique.

One is I'm not sure if it'll work properly in batch-compiled code...but I think it should.

Second is that the way I supply the `/* template */` code to the functor macro
requires that code to be valid in a `with`-block context. That means that
toplevel statements like `const` and `type` declarations are excluded, and this
may be a handicap. One could get around that by supplying the body of the code
as a string rather than in the way I do. The downside of that is one would get
less useful syntax-checking from the compiler/interpreter. And also, one would
lose syntax highlighting in the editor.

The third downside is that there doesn't seem to be any way to check that the
namespaces provided to the functor satisfy the constraints that the
implementing body requires. One can argue that this is consistent with the
general dynamic typing strategy of Pure. But in these cases, I think it might
be nicer to have finer-grained control on the type constraints than my macro
makes possible.

I would have liked to be able to do this:

```
interface Comparable with
  x::Comparable < y::Comparable;
end;

functor MakeSet( t=>Comparable, t2=>... with
  /* template */
end;
```

and then have the generated namespace look like this:

```
namespace CaselessStringSet with
  type t v = typep Comparable v && typep CaselessStrings::t v;
  using namespace CaselessStrings;
  empty = [];
  insert [] x::t = [x];
  ...
end;
```

here we restrict the the type `t` supplied by the parameter namespace
(`CaselessStrings`) to the specified interface `Comparable`, so that `::t` tags
used in the template can be relied on to have `Comparable` instances.

However, this won't quite work, because the `<` in the above `interface Comparable ...` declaration is interpreted _statically_. That is, in order to
count as `Comparable`, a type has to implement the `<` relation _visible where
that interface declaration is compiled_. It matters not whether you implement
some other `<` relation defined in another namespace. Whereas what would have
helped us here is if a `PlainString` counted as `Comparable` by virtue of
implementing the `<` visible in the `PlainString` namespace, a `CaselessString`
counted as `Comparable` by virtue of implementing the `<` visible in the
`CaselessString` namespace, and so on.

I expect one could work around this obstacle by metaprogramming, evaluating new
copies of the interface declaration in the `CaselessString` namespace, and so
on. But it doesn't look easy and it doesn't look clean.

## Extending the Pure Language ##

So, that leaves the seventh strategy: extend the language. The least
syntactically intrusive way to do this would be to accept the new constructs:

```
namespace n t1 /*::tag1*/ t2/*::tag2 ... */;
    /* template */
namespace;

namespace n t1 /*::tag1*/ t2/*::tag2 ... */ with
    /* template */
end;

using namespace n m /* (symbols), ... */;
```

The first two constructs would work similarly, so I'll just focus on the second and the third.

The idea is that the `namespace n t1::tag ... with /* template */ end` constructions would create some functions used only by the compiler at compile-time. These functions would be triggered by the `using namespace n m` constructs, to generate namespaces on-the-fly (and start reading from them) that behave just as though something of this form had been explicitly compiled:

```
namespace n with
  using namespace m; /* note that Issue 75 bears on whether one can place this before the following lines */
  type t1 v = typep tag1 v && typep m::t1 v;
  type t2 v = typep tag2 v && typep m::t2 v;
  /* template uses types t1 and t2 and other functions from m that one can rely
     on types tag1 and tag2 implementing */
  ...
end;
```

One advantage of language support for this is that we could write any toplevel statement inside the `namespace ... with ... end` construction. Another is that the language might be able to give interface types the dynamic interpretation in this context that they otherwise lack. That is, if tag1 is an interface type, using it in this context could mean it's re-interpreted to use any function symbols visible from namespace m, even if those are syntactically distinct from the same-named functions visible where the interface type was fist declared. In other words, using interface types in this context is like using an expression in the context:
```
reduce EXPRESSION with
  dynamically_reinterpreted_symbol1 _ _ = ...
  dynamically_reinterpreted_symbol2 = ...
end
```

One _dis\_advantage to the syntax I proposed here is that no names are attached to the generated namespaces. The syntax I proposed here would just have them generated on the fly by the `using namespace n m` statement. (The compiler might cache these internally, though.) In this respect, namespaces would here be not even the "second-class values" that modules were in OCaml._

I don't know in the first place whether there's good enough reason to do any of this in Pure. And if there is, I don't know whether there would be good enough reason to be able to associate names with the generated namespaces.

But I thought I'd write up these ideas for reference, and for others to push off against.