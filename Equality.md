Let's review the varieties of equality predicates native to Pure and some other
languages.


## Python, OCaml, and Scheme ##

Python has two such predicates, `is` and `==`. The first is the finer-grained
notion, sometimes referred to as "physical identity". It loosely corresponds to
what in my academic discipline we call "numerical identity." The second
corresponds to what we call "temporary qualitative identity."

For the simplest immutable objects the two notions coincide. For mutable
containers like lists, `==` tracks whether the members are themselves `==`,
recursively, until we reach the simplest immutable elements. `is` on the other
hand tracks whether we're dealing with the same allocated hunk of memory.
Hence, in Python, this code:

```
a = [1]
b = a
c = [1]
a is b, a is c, a == c
```

will evaluate to: True, False, True.

What about for immutable containers, like Python tuples? These behave the same
as the mutable containers: `(1,) == (1,)` will be True, and `(1,) is (1,)` will
be False.

One justification for this might be that although Python's native tuples are
immutable, tuples can be subclassed and instances of the subclass might both be
mutable and still count as tuples. But the driving motivation is probably just
that this is the most efficient way to implement a finer equality predicate
than `==`. The two tuples `(1,)` and `(1,)` occupy different locations in the
heap, and it's easiest for that reason to count them as not being `is` each
other, without trying to sort out what relations their contents bear to each
other.

Another difference between these predicates in Python is that `==` is
extensible (for new classes the user introduces) but `is` is not. If not
overridden, the default semantics for `==` corresponds to that for `is`.

OCaml is different from Python in many ways, including its static typing and
the fact that its mutation goes via explicit reference cells rather than all
variables having reassignable bindings. But with respect to its equality
predicates, it is quite similar. One pair of predicates `=` and `<>` tracks
equality in a way that counts physically distinct reference cells with the same
contents as equal. Another pair `==` and `!=` is finer, and counts such
cells as unequal. All of these predicates are "extensible" only in the sense
that their names can be rebound to other functions.

Scheme is semantically closer to Python, but it turns out to have _three_
equality predicates, `eq?` `eqv?` and `equal?` (as well as a variety of
type-specific equality predicates like `=` for numbers, `string=?`, and so on).
`equal?` is like Python's `==` and OCaml's `=`, and as in Python, some Scheme
implementations permit its definition to be extended by the user for new types
she introduces. `eq?` corresponds to Python's `is` and OCaml's `==`. In
particular, two mutable Scheme pairs will fail to be `eq?` even when they have
`equal?` members and so the pairs themselves count as `equal?`.

But now what about `eqv?`

This corrects some warts in the semantics of `eq?`, at the cost of some
efficiency. For as it turns out, some Scheme objects that we'd intuitively
expect to be equal even in a fine-grained sense turn out not to be `eq?`,
because of details in the underlying implementation that the user might not be
attending to, or even be expected to know about. For example, on my machine,
this evaluates to `#t` in Racket:

```
(eq? 4611686018427387903 4611686018427387903)
```

but if you add one more to each side, it comes out `#f`. Also, "a" represents an
immutable string in Racket, but nonetheless

```
(eq? "a" "a")
```

comes out false. So too with Racket's immutable pairs.

Some of these warts, such as the one with bigints, are corrected by `eqv?` But
not all of them are.

Once you start attending to these details, it's natural to hanker for an
equality predicate that's fine-grained like `eq?` when it comes to mutable things,
but ignores factors like where a value is represented in the heap when it comes
to immutable things. `eqv?` goes a bit in that direction compared to `eq?`, but
not far enough. It's natural to expect a predicate that ignores physical
identity across the board---but just when dealing with the immutable.

For example, suppose we define `cell1`, `cell2`, and `cell3` to be three
mutable containers holding the single value `0`. And suppose we also have three
_immutable_ pairs, the first of which holds `cell1` and `cell2`, the second of
which also holds `cell1` and `cell2`, and the third of which holds `cell1` and
`cell3`. All three of the languages just surveyed give us an equality predicate
that counts all three of these immutable pairs as equal. But it's natural
_also_ to expect a predicate that counts the first two as equal and the third
not. None of those languages provide such a predicate natively.


## Baker's EGAL ##

What we're hankering after here is in the direction of what Henry Baker calls
EGAL in his 1993 paper
[Equal Rights for Functional Objects or, The More Things Change, The More They Are the Same](http://home.pipeline.com/~hbaker1/ObjectIdentity.html).

Immutable structures count as EGAL when they are structurally the same, and
their elements are EGAL. Mutable structures (OCaml and Pure reference cells
being the simplest examples of these) count as EGAL only when they are
physically identical---or as Baker operationally defines it, when changing the
one of them would directly change the other. (I say "directly" to exclude any
funny business like having a background thread watching the one value and
updating the other to match it.)

For simple objects like ints we assume some antecedent understanding of when
they are EGAL.

Baker only specifies the behavior of EGAL for types all of whose structural
features are essential. He does however envisage letting the definition be
extended for abstract types. Now what if we're dealing with a immutable set
type, implemented as a balanced tree, and we have two sets whose members are
already known to be EGAL. I'd then expect the sets to be EGAL, too. But if they
were constructed by adding members in different orders, they may well be built
out of structurally different balanced trees. From the user's perspective, such
implementation details would be irrelevant. But when designing our code, we may
have to think about the difference between a more concrete and a more abstract
notion of egality. We will return to this.

I said earlier that physical identity loosely corresponds to what my discipline
calls "numerical identity." In fact, I think that term corresponds better to
egality. When nothing funny happens in between, we count a person who goes to
sleep on Monday as numerically identical to the person who wakes up on Tuesday,
even though some molecules that make them up have then come and gone. In the
same way, lists or sets with the same members should count as numerically
identical, even if the computer realizes or implements those sets using
different memory molecules.


## Pure's `===` ##

Now like the other languages we surveyed, Pure also has two native equality
predicates. The finer of these is called both `===` and `same`, and
dispatches to the function `same` in Pure's C++ backend. Some observations
about this function:

<ol>

<li>It is totally defined; the user can rebind the symbols but cannot<br>
(meaningfully) extend the definition they're currently bound to.<br>
<br>
</li><li>This function works like Baker's EGAL, or more specifically, like what I<br>
called "a more concrete notion of egality." Reference cells count as <code>===</code>
iff they are physically identical. So too some other runtime objects, like<br>
malloc'd pointers, and lambda-generated function closures. However for most<br>
immutable allocated values, physical identity is ignored and these are instead<br>
recursively descended, comparing their components.<br>
<br>
Recall our earlier example of three different reference cells <code>cell1</code>, <code>cell2</code>, and<br>
<code>cell3</code>, that happen all to hold the int value <code>0</code>. And three different immutable<br>
pairs, the first being <code>(cell1, cell2)</code>, the second also being <code>(cell1, cell2)</code>,<br>
and the third being <code>(cell1, cell3)</code>. I complained that the other languages give<br>
us a way to detect what's in common to all three of the pairs, but not a way to<br>
determine what's in common to the first two but different in the third. Pure on<br>
the other hand does give us the latter notion, with <code>===</code>. The first pair and<br>
the second pair count as <code>===</code>, even when the C++ backend represents them as<br>
separate objects occupying different hunks of memory. And they are both <code>~==</code>
the third pair.<br>
<br>
In Pure it turns out to be harder to express the former notion, that tracks<br>
what all three of the pairs have in common.<br>
<br>
I said that Pure's <code>===</code> works "like" Baker's EGAL. As Albert Graef<br>
<a href='https://groups.google.com/d/msg/pure-lang/TtTvm3Bskcg/GNGi0ljdKAAJ'>points out in a mailing list thread</a>,<br>
there are some minor differences having to do with<br>
how it understands the egality of function closures. But we won't dwell on<br>
that.<br>
<br>
I also said that Pure's <code>===</code> works like the "more concrete" notion of<br>
egality. It doesn't ignore structural differences in how two sets happen to be<br>
concretely implemented. In Pure, <code>set [1,2] === set [2,1]</code> evaluates to false.<br>
(Though <code>insert (set [1,2]) 3 === insert (set [2,1]) 3</code> happens again to<br>
evaluate as true.)<br>
<br>
<br>
</li><li>The function <code>===</code> correlates with <i>matching during runtime against a<br>
repeated pattern variable.</i> That is, when <code>a === b</code>, the list <code>[a, b]</code> will<br>
match the pattern <code>[var, var]</code>. And the converse is true as well, which we can<br>
see as follows. You can decorate parts of a pattern using the <code>patternvar@...</code>
syntax. This permits us to write patterns like <code>[var1, var2@(_,_)]</code>. It also<br>
permits us to write the pattern <code>[var1, var1@var2]</code>. When it matches a value,<br>
this latter pattern will bind both of the variables <code>var1</code> and <code>var2</code> to the<br>
appropriate components of that value. But also, because the pattern variable<br>
<code>var1</code> is repeated, the value that <code>var2</code> matches and gets bound to has to be<br>
the same as the one <code>var1</code> matches and gets bound to. "Same" here means same in<br>
the sense of <code>===</code>: whenever the pattern <code>[var1, var1@var2]</code> matches, we can<br>
rely on <code>var1 === var2</code> being true.<br>
<br>
There is an interesting wrinkle here that came up in recent discussions on<br>
the bugtracker and mailing list. (See also<br>
<a href='http://code.google.com/p/pure-lang/wiki/Quoting#11._Matching_named_functions'>this wiki link</a>.)<br>
As well as pattern variables, Pure patterns can include<br>
literal symbols. There are some details about when some bit of source code is<br>
interpreted as one and when as the other, but the dominant rule is that head<br>
positions in a pattern are interpreted symbolically, while non-head positions are<br>
interpreted as pattern variables. When a head position is decorated with<br>
<code>var@...</code>, that's also interpreted as a pattern variable. So the pattern<br>
<code>[(head _), (head _), (head _)]</code> matches a list of three values, each of which<br>
consists of something matching the literal symbol <code>head</code> applied to an<br>
arbitrary value. But now what matches the literal symbol <code>head</code>?<br>
<br>
Here is where things get subtle.<br>
<br>
Symbols can exist in Pure as unevaluated atoms, or they can have reduction<br>
rules bound to them, either globally or locally. When a symbol with reduction<br>
rules bound to it is in head position, it might nonetheless fail to reduce,<br>
perhaps because it's been applied to a value that doesn't match any of the<br>
patterns in the rules that define it. Or perhaps because it's only so far been<br>
partially applied. In all these cases the symbol still remains bound to the<br>
rules (that's why completing a partial application later works).<br>
<br>
A symbol with reduction rules bound to it might <i>also</i> fail to reduce<br>
because it's been quoted; in this case the symbol gets interpreted as bound to<br>
itself, rather than to any rules. So for example:<br>
<br>
<pre><code>foo x y = y;<br>
let f2 = foo 2;<br>
// now f2 evaluates to `foo 2`: that is, the head `foo` applied to the value `2`<br>
<br>
case f2 of foo var2 = [foo, var2]; _ = false end; // evaluates to `[foo, 2]`<br>
<br>
case f2 of var1@_ var2 = [var1, var2]; _ = false end; // also evaluates to `[foo, 2]`<br>
<br>
case foo 2 3 of var1@_ var2 var3 = [var1, var2, var3]; _ = false end;<br>
// this evaluates to `false`, because `foo 2 3` reduces to an atomic value<br>
// rather than to a head applied to two other values<br>
<br>
f2 3;<br>
// since foo in f2 is still bound to its defining rule, this reduces to `3`<br>
<br>
case f2 3 of var1@_ var2 var3 = [var1, var2, var3]; _ = false end;<br>
// this also evaluates to `false`<br>
<br>
let g = 'foo;<br>
// g is now bound to the unevaluated symbol `foo`<br>
// rather than to the rules that symbol is associated with<br>
<br>
g 2 3;<br>
// this doesn't reduce; it remains an application of the symbol `foo` to two other values<br>
<br>
case g 2 3 of var1@_ var2 var3 = [var1, var2, var3]; _ = false end;<br>
/ this evaluates to a list which prints as "[foo, 2, 3]"<br>
<br>
foo === 'foo;<br>
// this evaluates to false, because the lhs and rhs are bound to different things:<br>
// the lhs symbol to some rules, the rhs expression to the lhs symbol<br>
<br>
g === 'g;<br>
// this evaluates to true, because the lhs symbol is bound to itself<br>
</code></pre>

One can sometimes miss the differences between the rules-value that symbol<br>
<code>foo</code> is bound to, and the symbol-value that <code>'foo</code> and <code>g</code> are bound to,<br>
because the Pure interpreter prints both of these the same way. It will also<br>
print the same way a different rules-value that is locally defined using the<br>
same symbol. For example:<br>
<br>
<pre><code>let h = foo with foo x y = x end;<br>
[foo, g, h]; // prints as "[foo, foo, foo]"<br>
foo === g, foo === h, g === h; // but all three evaluate to false<br>
</code></pre>

Now the subtlety I mentioned is that <i>all three</i> of these different values<br>
are eligible to match the literal symbol <code>foo</code> in a Pure pattern, even though<br>
those three values are not <code>===</code>. This evaluates to true:<br>
<br>
<pre><code>case [foo 2, g 2, h 2] of [(foo _), (foo _), (foo _)] = true; _ = false end;<br>
</code></pre>

Yet even here, <i>repetition of a pattern variable</i> instead of a literal<br>
symbol does remain correlated with <code>===</code>. So both of these evaluate to <code>false</code>:<br>
<br>
<pre><code>case [foo 2, g 2, h 2] of [(var@_ _), (var@_ _), (var@_ _)] = true; _ = false end;<br>
case [foo 2, g 2, h 2] of [(var@foo _), (var@foo _), (var@foo _)] = true; _ = false end;<br>
</code></pre>

</li><li>The Pure documentation refers to <code>===</code> as "syntactic equality". This may<br>
mislead, for on the one hand it'd be natural to think of the two elements<br>
in this list:<br>
<br>
<pre><code>[(\x -&gt; x), (\x -&gt; x)]<br>
</code></pre>

as "syntactically equal", yet they are not <code>===</code>. And on the other hand,<br>
Pure evaluates the following expression as true:<br>
<br>
<pre><code>x === head [x];<br>
</code></pre>

yet the two operands look to be syntactically distinct. The key observation<br>
for the latter case is that <code>===</code> is an ordinary Pure function, that takes its<br>
arguments as already-reduced values. To detect the "syntactic difference"<br>
between <code>head [x]</code> and <code>x</code>, we'd instead have to write a macro, like this:<br>
<br>
<pre><code>def syntaxeq x x = 1;<br>
def syntaxeq _ _ = 0;<br>
</code></pre>

Then:<br>
<br>
<pre><code>syntaxeq x (head [x]); // evaluates as false<br>
</code></pre>

Some observations. First, two symbolically different variables count as<br>
syntactically distinct, even if they hold physically the same value:<br>
<br>
<pre><code>let a = 0;<br>
let b = a;<br>
syntaxeq a b; // evaluates as false<br>
</code></pre>

Second, the syntaxeq macro can detect the syntactic equality of lambda<br>
expressions, but only when they are supplied as its literal operands:<br>
<br>
<pre><code>syntaxeq (\x -&gt; x) (\x -&gt; x); // evaluates as true<br>
<br>
let f = \x -&gt; x;<br>
let g = f;<br>
syntaxeq f (\x -&gt; x);<br>
// evaluates as false, and `f === (\x -&gt; x)` also evaluates as false<br>
<br>
syntaxeq f g;<br>
// evaluates as false, though `f === g` evaluates as true<br>
</code></pre>

In the last expression you are really asking whether the variables <code>f</code> and<br>
<code>g</code> are syntactically equal; the macro doesn't know what values they are bound<br>
to at runtime.<br>
<br>
Third, when comparing variable-binding expressions, Pure respects their<br>
alphabetic realization, not just their "de Bruijnification":<br>
<br>
<pre><code>syntaxeq (\x -&gt; x) (\y -&gt; y); // evaluates as false<br>
</code></pre>

What if we try to only partially apply <code>syntaxeq</code>, for instance if we do<br>
this:<br>
<br>
<pre><code>eqf g when eqf = syntaxeq f end;<br>
</code></pre>

In that case, mightn't the macro be forced to use the value of its <code>f</code>
operand, and forget about how that value happened to be syntactically supplied?<br>
The answer is that you can't partially apply macros in this way. Pure will in<br>
this case honor only runtime bindings for <code>syntaxeq</code>, and count its macro<br>
bindings as unmatched, even when the <code>g</code> operand is supplied later.<br>
<br>
Like <code>===</code>, the <code>syntaxeq</code> notion we defined here also corresponds with<br>
<i>matching a repeated pattern variable</i>, however in this case it's <i>macro-time</i>
matching which matters, not runtime matching.<br>
<br>
Perhaps this notion might also be defined as:<br>
<br>
<pre><code>#! --quoteargs syntaxeq2 // this is required for syntaxeq2 to properly operate on block expressions<br>
def syntaxeq2 x y = 'x === 'y;<br>
</code></pre>

I haven't yet been able to think of any cases where syntaxeq and syntaxeq2<br>
will give different answers.<br>
<br>
</li></ol>

In summary, I'd discourage thinking of `===` as expressing "syntactic
identity." That terminology can be motivated, but it can also mislead. It's
better to think of `===` as expressing the "more concrete" version of Baker's
egality.

So far, then, we've encountered four natural concepts of equality. I'm
disregarding the differences between `eqv?` and concrete-vs-abstract notions of
egality, and disregarding also Scheme's type-specific notions like `=` and
`string=?`. In broad brushstrokes, we have:

  * the syntactic equality I macro-defined above
  * physical identity, represented by Python's `is`, OCaml's `==` and Scheme's `eq?`
  * egality, so far most closely represented by Pure's `===`
  * and the notion represented by Python's `==`, OCaml's `=` and Scheme's `equal?`

I mentioned already that it's not straightforward how to express the last of
these notions in Pure.

The second of these notions is also a bit elusive. `===` coincides with
physical identity for reference cells, but not more generally. It will be
possible to define a predicate in Pure that tracks physical identity in
general, though this will take some discussion. We will come back later to the
question how to express that in Pure.

It will emerge below that I think there are interesting fifth and sixth concepts of equality.

We have also yet to discuss Pure's `==`.



## Pure's `==` ##

Pure has a second native equality predicate `==`. Though it's different than
all of the preceding, I wouldn't say it yet represents a further "concept" of
equality. It instead represents a partially-defined predicate that could be
extended in several of the ways we've already surveyed.

That is, `==` is like Python's `==` and Scheme's `equal?` in that the user is
permitted to (expected to) extend it for new types of values she introduces.
However it does not have the same semantics as those other predicates. Two
reference cells in Pure don't count as `==` when they have the same contents.
Neither do they count as `~=` when they have different contents. They don't
even count as `==` when they are physically one and the same cell. `==` is
simply not defined on reference cells. Nor is it defined on malloc'd pointers,
lambda-generated closures, and some other values.

The intended design for `==` seems to be for it be somewhat coarser than `===`
is. For example, the standard libraries count `set [1, 2]` and `set [2, 1]` as
`==` because they have `==` members, even though, as it happens, they get
implemented behind the scenes by different balanced trees.

However, there are also cases in which `===` holds but `==` does not. For instance:

```
let cell1 = ref 0;
[cell1] === [cell1];
// evaluates as true, though `[cell1] === [ref 0]` evaluates as false

[cell1] == [cell1];
// doesn't fully reduce, because `cell1 == cell1` isn't defined
```

Now one could extend the definition of `==` to count reference cells as `==`
iff they are physically identical, or alternatively one could extend it to
count them as `==` iff their contents are `==`. So as not to break other code,
we'll do this to new value types that wrap the native cells, instead of the
cells themselves. I'll call the first notion a "box", because when it comes to
`==` the contents aren't visible; and the second notion a "cage" because the
contents are visible.

```
public Box;
type box (Box _);
box x = Box (ref x);
unbox (Box cell) = get cell;
x::box == y::box = x === y;
x::box ~= y::box = x ~== y;

public Cage;
type cage (Cage _);
cage x = Cage (ref x);
uncage (Cage cell) = get cell;
x::cage == y::cage = uncage x == uncage y;
x::cage ~= y::cage = uncage x ~= uncage y;
```

Now if we have:

```
let b1, b2, b3 = box 0, box 0, box 1;
let c1, c2, c3 = cage 0, cage 0, cage 1;
```

Then:

```
b1 == b2, c1 == c2; // evaluates to false, true
[b1] == [b1], [c1] == [c1]; // evaluates to true, true
[b1] == [b2], [c1] == [c2]; // evaluates to false, true
[b1] == [b3], [c1] == [c3]; // evaluates to false, false
```

So far, so good.

Now what about sets of these things? As it stands, we can't use the ordinary
sets, because our boxes and cages aren't ordered. We might add an ordering to
boxes, based on the container's hash. But this is essentially what hsets do
anyway so let's use that existing apparatus.

We might consider adding a different ordering to cages, based not on the
physical identity of the containers but rather on the ordering of their
contents. But certain problems arise if we do this. First, the sets will break
if their members' orderings (according to `<`) ever change during the set's
lifetime. Also, note that `==` and `<=` for ordered sets are defined not in
terms of `==` for their members, but instead in terms of how the members are
ordered by `<`. This is what I had in mind as a "fifth" interesting concept of
equality. For appropriate choices of x and y, they count as equal in this fifth
sense when:

```
~(x < y) && ~(y < x)
```

We can require that `x < y` evaluates to false, and not merely happens to be
undefined; so too for `y < x`. Even granting that, though, there are still
going to be substantial limits to what values we can expect this to give good
results for. After all, both of these evaluate false:

```
set [0] < set [1];
set [1] < set [0];
```

Yet we wouldn't want to count those sets as equal. With ordered sets, though,
it is expected that the members can be counted as equal when they are ordered
in this way. And we'd naturally want this notion to line up with any `==` we
independently define on the members. It would be surprising to have one of:

```
x == y
set [x] == set [y]
```

evaluate true and the other false. But now, if we can't mutate our cages for
fear of changing their ordering, and if ordering and equality for them would
just coincide with ordering and equality for their members, what would we gain by
working with the cages in the first place?

I'll assume that we do want to be able to mutate cages even after they're
inserted into sets; so if we want sets of theem to continue working, their
ordering had better be independent of their current contents. So we might as
well forget about ordering them ourselves, and work with hsets of them too,
which do the ordering implicitly. This initially looks good:

```
hset [b1,b2] === hset [b2,b1], hset [b1,b2] == hset [b2,b1];
// evaluates to false, true

hset [c1,c2] === hset [c2,c1], hset [c1,c2] == hset [c2,c1];
// evaluates to false, true
```

But note that, although `c1 == c2`, `#hset [c1,c2]` evaluates to `2` not to `1`. Note also that:

```
hset [c1] <= hset [c2] && hset [c2] <= hset [c1]; // evaluates to false
hset [c1] == hset [c2]; // evaluates to false
```

This is because `c1` and `c2` will probably have different hashes. And when we
think about it, we realize our cages have broken a fundamental convention about
the relation between hashes and equality: we have `hash c1 ~= hash c2` even
though `c1 == c2`. If we proceed this way, we'd encounter all sorts of
difficulties.

Hsets of boxes work out OK, though. And in fact we didn't even need to move
to boxes, we could have worked with ordinary reference cells. Pure's
implementation of hsets automatically falls back to the `===`ity of its
elements when their `==`ity isn't defined. Summarizing , then: hsets of
reference cells work fine, without needing the extended notion of `==`
introduced by boxes. And neither sets nor hsets of cages look useful.

When it comes to the useful setlike containers, `===` and `==` look like the
more concrete and more abstract notions of egality we discussed before. `==`
abstracts away irrelevant structural details of the implementing balanced tree,
and just tracks whether corresponding set members are `==`---or when that's
undefined as it is for reference cells, whether they are `===`. For reference
cells, this reduces to their being physically identical. `===` works similarly
but doesn't ignore the irrelevant structure details.

This pattern doesn't hold across the standard library, though. As we already
saw, `[cell1] == [cell1]` doesn't evaluate to true, though by both the finer
and coarser notions of egality it should.

Perhaps the standard library should define `==` for lists as coinciding with
`===`. Or more generally, perhaps it should define any pair of objects as `==`
when they are `===`:


```
// proposal 1: add to prelude?
x == y = 1 if x === y;
x ~= y = 0 if x === y;
```

And in the case of reference cells, perhaps it should also define them as `~=`
when they are `~==`:

```
// proposal 2: add to prelude?
x == y = x === y if refp x && refp y;
x == _ | _ == x = 0 if refp x;
x ~= y = x ~== y if refp x && refp y;
x ~= _ | _ ~= x = 1 if refp x;
```

This would make reference cells coincide in behavior with our boxes. Or perhaps
the standard library shouldn't do any of these things. These seem to be reasonable
proposals to think about, though.

Minimally, it seems to be useful to have a notion that does fully capture the
more abstract notion of egality, one that could be used on hsets and lists
alike, even if they might contain reference cells. The backend balanced-tree
implementations of Pure's sets and dicts make use of such a predicate
internally. They call it `eq`. That may be too short and common a name to
introduce at this point into the standard library. Other reasonable candidates
include: `eqv`, `equal`, `equiv`, `egal`, and `gleich`. I hope others will
weigh in on this, but for the time being, I propose to name this predicate
`egal`:

```
// proposal 3: at least add this to prelude?
egal x y = case x == y of
             res::int = res;
             _ = same x y; // same as: `x === y`
           end;
```

Unlike `==`, this predicate is totally defined---but its behavior is
nonetheless still user-extensible. It piggybacks on the extensibility of `==`.

With a predicate like this in hand, we can say:

```
let cell1 = ref 0;
let cell2 = ref 0;
egal [cell1] [cell1]; // true
// those would also be (===) but not (==)

egal [cell1] [cell2]; // false

hset [1, cell1] === hset [cell1, 1]; // false
egal (hset [1, cell1]) (hset [cell1, 1]); // true
// those would also be (==) but not (===)

egal (hset [cell1]) (hset [cell2]); // false

any (egal cell1) [cell1]; // true
any (egal unbound_symbol) [unbound_symbol]; // true
```

I'm not sure why the operation in the last two examples isn't already provided
in the prelude as an implementation of `member` for lists. I'd also advocate
writing the function `index` (which is implemented in the prelude for lists) in
terms of `egal` rather than `==`.



## Scheme's `equal?` in Pure ##

Let's return to the question of how we should express in Pure a notion like
Scheme's `equal?`---given that cages don't look like the way to go.

Something like this might work:

```
public Parole;
parole cell = Parole (get cell) if refp cell;
parole (h@_ v) = (parole h) (parole v);
parole v = v;
Parole x == Parole y = egal x y;
Parole _ == _ | _ == Parole _ = 0;
Parole x == Parole y = ~(egal x y);
Parole _ ~= _ | _ ~= Parole _ = 1;
```

This is similar to cages, roughly in that "paroled reference cells count as
equal iff their contents do". But whereas a caged reference cell _included_
that reference cell, paroling is _an operation on_ reference cells. Paroles are
ordinary immutable containers. Their role is just to distinguish the case of a
value that came from a reference cell and one that didn't. We will never have:

```
parole (ref x) == parole x;
```

unless `x` happens to point cyclically to itself. We will however always have:

```
parole (ref x) == parole (ref x);
```

despite the physical distinctness of the two reference cells. Given this notion,
we can compare two lists that may contain reference cells like this:

```
map parole lst1 == map parole lst2;
```

That will evaluate as true when the lists contain reference cells in
corresponding positions that have egal contents. If the lists may contain other
lists, we'd have to arrange to recursively descend them.

Given an implementation of `map` for hsets, we could use the same technique to
do Scheme `equal?`-like comparisons on them, too.



## Scheme's `eq?` in Pure ##

Another question we left unanswered is how to generally express physical
identity in Pure.

For reference cells this coincides with `===`, but as we said for other allocated objects like lists those notions come apart.

Reference cells aside, is the difference in physical identity observable at all
in Pure? It turns out it is, because of the `sentry` facilities.

Here is a session in the Pure interpreter:

```
> using system;
> let x = sentry (\x -> puts "x is dying") [1];
> let y = sentry (\x -> puts "y is dying") [1];
> x === y;
1
> let x = 0;
x is dying
> let y = 0;
y is dying
```

As you can see, the two list values have different sentries attached to them,
which get executed (what's the right verb here?) at different times. Yet the
values are still counted as `===`.

Theoretical interest aside, it should hardly ever be necessary for Pure code to
track the physical identity or difference of two values. `===`ity is likely to
be the finest-grained equality predicate you'll ever need to work with. But,
pursuing the theoretical interest of this, how might one test for physical
identity?

Albert Graef provided this method on the pure-lang mailing list. It's based on
one of the fundamental ideas in Baker's paper:

```
extern bool pure_has_sentry(expr *x) = has_sentry;

identical x y = res when
    restore = if has_sentry x then sentry (get_sentry x) else clear_sentry;
    unique = \x->x;                // we rely on the fact that no physically distinct
                                   // Pure value will be `===` a lambda-generated closure
    sentry unique x;               // we temporarily mutate x by giving it a new sentry
    res = get_sentry y === unique; // was y mutated in the same way?
    restore x; // put x back the way it was
end;
```


Another way to test for physical identity uses Pure's C API:

```
identical x y =
    __C::pointer_get_pointer (ref x) === __C::pointer_get_pointer (ref y);
```

Essentially, this is comparing the memory address where `x` is represented
to the address where `y` is represented.



## A sixth concept of equality ##

So far we've encountered five interestingly different concepts. I wouldn't
claim that these are all species of a single idea of "equality", however they
do all invite similar vocabulary and formal notation, and it's worth keeping
their difference clearly in mind:

  * the syntactic equality I macro-defined as `syntaxeq`
  * physical identity, represented by Python's `is`, OCaml's `==` and Scheme's `eq?`
  * egality, represented concretely by Pure's `===` and more abstractly by our `egal`
  * the notion represented by Python's `==`, OCaml's `=` and Scheme's `equal?`
  * the notion used by Pure's ordered sets: `~(x < y) && ~(y < x)`

As I said before, I don't see Pure's `==` as representing a new addition to
this list, but rather a partially-defined predicate that could be extended in
several of the ways already mentioned.

However, I do think there is a sixth notion that belongs on the list. This notion
is of special theoretical interest to me, and it does stick its head above water
in several places in programming theory. However, it hasn't to my knowledge yet
been focused target of research. And I'm doubtful it's even expressible in Pure. So
you can regard what follows as just an intellectual curiosity.

I'll limit myself here to explaining what this sixth notion is, and leave for elsewhere
a discussion of why I think it's so theoretically interesting.

To help fix ideas, what we're about to discuss is related to the following phenomena:

  * sameness of "lvalues"
  * calling by reference, as opposed to calling by value (including calling by reference values with the latter)
  * Scheme's predicate `free-identifier=?`, which is used to compare macro arguments against symbols that are declared in the macro definition as literals

I will illustrate this notion by way of its interaction with mutation, though
ultimately I think its theoretical interest doesn't depend on the availability
of mutation.

Since it's unclear or doubtful that this can be expressed in Pure, we'll instead use Scheme.

We assume our Scheme has mutable boxes and immutable sets. First let's get two physically distinct boxes with equal values:

```
(define one (box 1))
(define uno (box 1))
```

Next, we'll make an immutable set:

```
(define w (make-set one 2))
```

Now we'll bind a series of other symbols to values in a way that gets us
closer and closer to the original.

```
(define z (make-set uno 2))
```

That will be `equal?` to `w`, but no better, since the two sets contain
physically distinct mutable boxes.

```
(define x (make-set one 2))
```

Here we have a set made up of the same integer and physically the identical box
as is in `w`. This set may be represented at a different location in memory
than `w` is, but they will be `egal`.

```
(define v w)
```

Now `v` will be not merely `egal` to `w`. It will be bound to physically the
same value, residing at the same place in memory. Now surely, this is the best
we can do, right?

Not quite. It's possible to introduce an even more intimate relation between
`w` and another variable. For observe:

```
(eq? v w) ; evaluates to #t
(set! w 'something_else)
(eq? v w) ; no longer #t
```

But now, suppose we had introduced calling-by-reference into our Scheme. This
is easy to do with macros, and is demonstrated in several textbooks:
[here is one implementation](http://docs.racket-lang.org/guide/pattern-macros.html?q=guide#%28part._pattern-macro-example%29).
With calling-by-reference, we can observe an even closer relationship to w:

```
(define w 'original)
(define-call-by-ref (f u)
  (let* ((res1 (eq? u w))
         (_ (set! w 'something_else))
         (res2 (eq? u w)))
        (values res1 res2)))
(f w) ; will evaluate to #t #t
```

We might even arrange to introduce this relationship at the same scope where
`w` was itself introduced. Racket has a special low-level macro function that
makes this trivial:

```
(define w 'original)
(define-syntax u (make-rename-transformer #'w))
(eq? u w) ; evaluates to #t
(set! w 'something_else)
(eq? u w) ; evaluates to #t
```

If only for theoretical interest, we might introduce a predicate that tests for
this kind of relationship between two variables. As I said, I think this is
approximated by Scheme's `free-identifier=?`. But the details of that predicate
depend on particular choices that were made in designing Scheme macro systems.
Those choices could well have gone differently. In other words, I don't see the
equality predicate we're discussing here as essential to performing that job
role in a macro system.

We could define the predicate in a manner like the Baker-inspired definition
of physical identity we saw before. That is, you change `w` and see whether `u`
changes as well. However, in Baker's test what was mutated was _the value_ that
`w`, and possibly `u` also, happened to be bound to. To test for the present
relationship, we'd have to _change (not shadow) `w`'s binding_, rather than
mutating the value it was bound to, and see whether `u`'s binding also ended up
changed. I can only see how to do this in languages with "implicit" mutatation,
that is, reassignable variables, like we have in Scheme.

In other places, I've called this relationship "hyper-equals". We might also
call it "aliasing" or "left-equality", because it operationally consists in `w`
and `u` being equivalent lvalues.

Interestingly, if you have structured lvalues in a language, as
[SRFI 17](http://srfi.schemers.org/srfi-17/srfi-17.html)
adds to Scheme, then two expressions can be the same lvalue even if all their
components aren't. For instance, let's define `w` to be a mutable pair (which
Racket calls "mpair"s):

```
(require racket/mpair)
(define w (mcons 0 0))
```

Then make `v` hold physically the same value:

```
(define v w)
(values (eq? (mcar w) (mcar v)) (eq? w v)) ; both are now #t
```

But as we already observed, `v` and `w` will not at this point be the same
lvalue. If we `(set! w 'something_else)` we can't expect `v` to follow suit.
However if we do this:

```
(require srfi/17)
; that lets us do this
(set! (mcar w) 1)
; and now, voila...
(values (eq? (mcar w) (mcar v)) (eq? w v)) ; both still #t
```


