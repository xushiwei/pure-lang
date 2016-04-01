

# Term rewriting #

The basic data type in PURE is the _symbolic expression_, and the basic operation performed on an expression is an application of a rewriting rule. Formally speaking, a PURE program is a list of rewriting rules and some expressions, to which the rules are to be applied. The task of the programmer is to specify the rules and then to specify the expressions to which the rules will be applied; the task of the PURE interpreter is to perform the rule applications. In this tutorial we will look at more in-depth examples of programming based on rewriting.

## Transforming symbolic data ##

Term rewriting in PURE gives us a straightforward way of implementing transformations of symbolic expressions, as well as a facility for defining a custom syntax for expressions.

As a toy example, let us try to implement a system that will transform a simple English phrase into a question. What we will get is this:
```
> Tom is tall;
Is Tom tall?
> barbarians are coming;
Are barbarians coming?
```

The idea of the implementation is that we will declare the words "`is`" and "`are`" as infix operators, and assume that all other English words to be used (e.g. `Tom`, `tall`) are free (i.e. undefined and unassigned) symbols. Then the input `Tom is tall` will be actually a well-defined PURE expression, understood by PURE as the application of the function "`is`" to the arguments `Tom` and `tall`.

The use of a word, "`is`", as an infix operator in the expression `Tom is tall` might appear strange. However, in PURE this is no different than an expression like "`b+2`", which is the application of the operator "`+`" to `b` and `2`. The operator "`+`" in the expression "`b+2`" is not a special built-in operator (unlike the case in many other programming languages). Rather, "`+`" is defined in the standard prelude as an infix operator. In the opinion of the PURE interpreter, "`+`" is just a _user-defined_ operator. (The interpreter knows how to add numbers, of course, and the standard syntax of "`+`" is automatically loaded by the interpreter at startup. However, it is possible to redefine "`+`" in a different namespace.)

Note that the operator "`+`" is a _function_: it has arguments, it can have special rewriting rules, and it can be passed as an argument to other functions. In order to pass the operator "`+`" as argument to another function, we write `(+)` in parentheses.

In PURE, "**operator**" means merely a _function_ for which the user has requested a special syntax convention.

Thus, it is possible to write infix expressions such as `1+2` in the standard function syntax: `(+) 1 2`.
```
> (+) 1 2;
3
```
However, it is certainly easier for humans to read `b+2` than `(+) b 2`. Once "`+`" is declared as an infix operator (this is done in the prelude), PURE will always print and recognize expressions containing "`+`" using the infix syntax. This is also the case for any other user-defined operators.
```
> (+) b 2;
b+2
```

Let us return to our example, "`Tom is tall`". Having declared "`is`" as an infix operator, we will define a rule for "`is`" so that it reverses the word order and adds a question mark at the end. The question mark, of course, needs to be declared as a _postfix_ operator. That's it!

Here is the code entered directly in the PURE interpreter:
```
> postfix 500 ?; infix 500 is are;
> x is y = Is x y? ;
> x are y = Are x y? ;
> foo is bar;
Is foo bar?
> barbarians are coming;
Are barbarians coming?
```

Remarks:

  * We have not defined any rules for the functions `?`, `Is`, and `Are`. Therefore, these functions remain unevaluated in the final expression. (This is exactly as we wanted it to be.)
  * The rules for the functions `are` and `is` are written using the infix syntax, after we defined that syntax. This is quite convenient and visually appealing. We could have also written the rules using the standard function application syntax, but then we would have to put the operator names in parentheses:
```
(is) x y = (?) (Is x y);  // equivalent to the above
(are) x y = (?) (Are x y);
```
  * We have defined both "`?`" and "`is`" with precedence level 500. The higher the precedence, the tighter the binding (i.e. operators with higher precedence bind first). When the precedence is equal, postfix operators bind tighter than prefix operators, which in turn bind tighter than infix operators (this is a special convention in PURE).

For example, in an expression that uses both "`is`" and "`?`", such as "`x is y ?`", the "`?`" binds to "`y`" rather than to the whole expression. This is so because postfix operators bind tighter at equal precedence, so "`?`" binds tighter than "`is`". Here is another illustration of this behavior: let us write the operator "`?`" after the expression "`Tom is tall`".
```
> Tom is tall? ;
Is Tom (tall?)?
```
This result might appear to be unexpected! The evaluation of this expression can be explained in the standard PURE function syntax like this: since "`?`" binds tighter than "`is`", the expression `Tom is tall?` means the same as
```
(is) Tom ( (?) tall )
```
and is then rewritten into
```
(?) (Is Tom ( (?) tall) )
```
which is shown on the screen as `Is Tom (tall?)?`.

If we defined `infix 1000 is`, we would have the result that "`is`" binds tighter, so `?` would bind to the whole expression:
```
> // new PURE session
> infix 1000 is; postfix 500 ?;
> x is y = Is x y ?;
> Tom is tall?;
(Is Tom tall?)?
```
The evaluation of this example can be explained like this: `Tom is tall?` is now the same as
```
(?) ( (is) Tom tall )
```
so the argument of "`?`"  is the entire expression `Tom is tall`. The arguments of a function are evaluated before the function; so `Tom is tall` is rewritten into `Is Tom tall?` and hence the result is
```
(?) ( (?) (Is Tom tall) )
```
which is shown on the screen as
```
(Is Tom tall?)?
```
The extra parentheses are shown in order to separate the two operators `?` visually from each other. For example,
```
> x??;
(x?)?
```
Presently, "`x??`" is the same as "`x ? ?`". However, if we define a postfix operator "`??`" then "`x??`" will not be the same as "`x? ?`". The PURE interpreter puts in the extra parentheses and writes "`(x?)?`" just in case.

Note also that "`Is`" has not been declared as an infix or other operator, so it still has the standard function syntax and thus bind tighter than any operators. Hence, regardless of the precedence of the operator "`?`", the expression `Is Tom tall?` means the operator "`?`" applied to the expression `Is Tom tall`.

**Question**: Suppose I get an expression such as `Is Tom tall?`. How can I determine the type of this expression, especially within a PURE program?

**Answer**: According to what we defined, `Is Tom tall?` is an application of the function `?` on the argument `Is Tom tall`, which itself is an application of the function `Is` on the two arguments `Tom` and `tall`. This expression remains unevaluated since we have not defined any rewriting rules for the functions `?` and `Is`, as well as for the symbols `Tom` and `tall`. Within a program, we can test the type of this expression using the standard predicates such as `varp` and `applp`:
```
> varp x; // whether x is a free symbol ("unassigned variable")
1
> const x = Tom is tall;
> x;
Is Tom tall?
> varp x; // x is not a free symbol any more
0
> applp x; // whether the value of x is a function application
1
```
We find that the value of `x` (`Is Tom tall?`) is indeed a function application, i.e. an unevaluated symbolic expression. Later we will see how to split an arbitrary symbolic expression into parts by using PURE's pattern matching facilities.

## Computing cosines symbolically ##

The next example of symbolic computation through term rewriting concerns the mathematical cosine function (`cos`). PURE already includes a facility for computing elementary functions through the `math` module:
```
> using math;
> cos 1;
0.54030230586814
> cos pi; // 'pi' is defined in the math module
-1.0
> cos (1000 * pi);
1.0
```
However, the math module computes everything with standard `double`-precision floats (about 16 decimal digits). When this precision is insufficient, wrong answers will be given:
```
> cos (10^10 * pi); // partial loss of precision
0.999999999997493
> cos (10^30 * pi); // total loss of precision
-0.489415668224579
```

We will now try to implement a _symbolic_ computation facility for `cos`. The goal is to have the cosine of an even multiple of π _exactly_ (rather than approximately) equal to the integer 1. We will introduce a symbolic name `Pi` for π, such that `Pi` will not be automatically converted to a limited-precision number but will remain an unevaluated constant. Further, we may define rules such that the cosine of certain multiples of "`Pi`" will be exactly simplified to integers.

In order to avoid conflicts with the `math` module, let us denote the symbolic cosine function by `Cos`. What we want is the following behavior:
```
> Pi;
Pi
> Cos Pi;
-1
> Cos (10^1000 * Pi);
1
> Cos (Pi/3);
1%2
```

We need to define `Pi` as a symbolic constant. In PURE, this can be implemented by declaring "`Pi`" to be an operator without arguments. (This is called a **nonfix** operator.) The declaration looks like this:
```
nonfix Pi;
```

**Question**: Why not simply have `Pi` as a global variable without value (as it is by default)? Then, without any extra work on our part, PURE will leave `Pi` unevaluated in every expression.

**Answer**: If we left `Pi` as an unassigned variable (this is called a **free symbol** or a **global symbol** in PURE), we will indeed have the effect that `Pi` is left unevaluated in all expressions. However, we will not be able to define any rules specifically for functions whose arguments contain `Pi` (rather than some other unassigned variable). Here is why. Suppose we want a rule that simplifies `cos (2*Pi)` to `1`. We may try to write the rule like this,
```
Cos (2*Pi) = 1;
```
But this will not work. Without the `nonfix` declaration shown above, the symbol `Pi` is no different from any other free symbol. Then, the variable "`Pi`" in the definition `Cos (2*Pi) = 1` would be interpreted as a pattern variable that matches any expression. The effect would be the same as if we wrote `Cos (2*x) = 1`, meaning that `Cos` of twice _anything_ is to be replaced with `1`. This is not at all what we wanted!

After the "`nonfix Pi`" declaration, the symbol "`Pi`" means the name of a function with _zero_ arguments. Then the symbol "`Pi`" in left-hand sides of patterns will never be interpreted as a pattern variable: instead it will be interpreted as a function name, like "`+`" or "Cos". We will not supply any evaluation rules for `Pi` itself, so `Pi` will also remain unevaluated in expressions like `2*Pi`. Then "`Pi`" becomes, in effect, a special reserved symbol. Any rules involving `Pi` in the left-hand side will only apply to expressions actually containing the unevaluated symbol `Pi`. The rule "`Cos(2*Pi)=1`" will then work as intended.

We may now define the following obviously useful rules for expressions with `Pi`:
```
double Pi = pi; // convert Pi to double
Cos Pi = -1;
Cos 0 = 1;
```

The next step is to write rules for `Cos` with more complicated arguments. This can be done on a case-by-case basis. We may simply write an expression as the argument of `Cos`. The PURE interpreter will perform pattern-matching on the expression, assigning any free variables.
```
Cos (- x) = Cos x;
Cos (Pi/3) = 1%2;
Cos (x::int * Pi) = if x mod 2 then -1 else 1;
Cos (x::int * Pi + y) = if x mod 2 then - Cos y else Cos y ;
```

After these definitions, a certain class of expressions involving `Cos` and `Pi` will be simplified:
```
> Cos (- Pi);
-1
> Cos (3*Pi + Pi/3);
-1%2
> Cos (2*Pi + 3);
Cos 3
```
Note that `Cos 3` remains unevaluated; this is so because we have not given any rules for rewriting this kind of expression. This behavior is perhaps useful since we cannot simplify `Cos 3` symbolically, in a mathematically exact fashion (without converting to an approximate double-precision number).

The example with `Cos (10^100*Pi)` does not work yet:
```
> Cos (10^100 * Pi);
Cos (10000000000.0*Pi)
```
Clearly the problem is that `10^100` is transformed into a floating-point number. This is not desirable; instead we can use the standard function `pow`. That function will automatically upgrade integers to long integers (called "`bigint`"s in PURE).
```
> pow 10 10;
10000000000L
```
It remains to define rules for `Cos` with `bigint`s.

While doing this, we need to take care that `x mod 2` will return a `bigint`, which cannot be used in a conditional! We get an error message when we try to use `if` on a `bigint`. Here is a simple example of what happens:
```
> if 1L then yes else no;
<stdin>, line 66: unhandled exception 'failed_cond' while evaluating 'if 1L then yes else no'
```
The condition _must_ evaluate to `int`, or else an exception occurs.
With usual `int` values, `true` is `1` and `false` is `0`, but `1L` or `0L` are not interpreted as "true" or "false". Therefore, we need to write the condition more explicitly:
```
Cos (x::bigint * Pi) = if x mod 2 == 1L then -1 else 1;
Cos (x::bigint * Pi + y) = if x mod 2 == 1L then - Cos y else Cos y ;
```

Now the example works as intended:
```
> Cos ( pow 10 1000  * Pi );
1
> Cos ( (1+ pow 10 1000)  * Pi + x);
-Cos x
```

It must be noted that our example is far from a complete symbolic simplification of the cosine. For instance, `Cos(3*Pi)` and `Cos(2*Pi+x)` are simplified but `Cos(Pi*3)` or `Cos(Pi+x)` or `Cos(x+2*Pi)` will not be simplified. It is impossible to define direct rules for `Cos` in every possible case: what about `Cos (2*(Pi*(3-Pi*(...)`? A workable approach is first to create a set of rules that will simplify the argument of `Cos` to some canonical form, e.g. to `x*Pi + y` where `y` does not contain any further multiples of `Pi` while `x` is a rational number (perhaps equal to 1), and then define rules for `Cos` acting only on that canonical form. Below we will describe how this can be accomplished using term rewriting and pattern matching.


# Pattern matching #

In other programming languages such as C and Python, "pattern matching" usually means a special operation where we search for a certain pattern, say, in a string of text. We would usually need to call some library function if we want to perform a search-and-replace operation. In PURE, pattern matching is a built-in feature for direct manipulation of expressions. The computational model adopted in PURE can be seen as a general kind of symbolic search-and-replace on expressions and parts of expressions.

Let us look into the pattern matching facilities of PURE in more detail.

## Defining functions by patterns ##

It is important to realize the difference between the code for the `Cos` function, as shown above, and code that we would have to write in other programming languages. Ordinarily we would define the `Cos` function in a _single_ function body, like this (Python):
```
 def Cos (expr):
   """ This function computes the cosine
  of a symbolic argument, in cases when
  a simplification is possible."""
  # lots of code
# end of function body
```
In the function body, we would have to decide on the possibility of simplification and go over all possible cases. If we later need to implement another case, we would have to revise this function body.

In PURE, we are not restricted to having a single function body. If it is convenient for us, we may give the definition of a function in many parts, each part corresponding to a different pattern of an expression in which this function can be found. This way of defining functions is illustrated by the `Cos` example above: We gradually added new rules for `Cos` and extended the functionality.

The pattern-matching approach goes further than that. There may be many rules involving `Cos` in different ways, so our definitions for `Cos` are not necessarily "parts" of the single definition of the function `Cos`. Rather, these definitions are _different rewriting rules_ that determine the evaluation of various expressions that involves the function symbol `Cos` and other functions. The rule about `Cos (x+2*Pi)` could be equally seen as a new rule about the function "`+`" or about the symbol "`Pi`".

Rewriting rules are defined by specifying _patterns_ of expressions. Patterns are like a schematic of an expression, like `Cos (x*Pi+y)`, where `x` and `y` are considered to be free symbols (**pattern variables**) and stand for _arbitrary sub-expressions_.

Each rewriting rule applies when its pattern exactly matches some part of the expression being evaluated, while pattern variables should match some subexpressions. Thus, the pattern `Cos (x*Pi+y)` will match `Cos (2*Pi + Sin a)`; namely, `x` matches the subexpression `2` and `y` matches the subexpression `Sin a`. This pattern will not match `Cos(3+2*Pi)` since the arguments of `+` are in the wrong order. Mathematical properties of `+` are not known to the pattern matching process because "`+`" in PURE is simply a user-defined function. The pattern must match exactly as given.

Thus, functions in PURE are defined by specifying a left-hand side pattern and a right-hand side "result expression." Typically, we will use the free symbols in the pattern (the pattern variables) to build the "result expression." This is what we have done in the `Cos` example above.

**Question**: Will the pattern `Cos(3+x)` match the symbolic expression `Cos(3+y)` or only `Cos(3+x)`?

**Answer**: The symbol `x` in the pattern `Cos(3+x)` is a **pattern variable**, which will match any expression whatsoever -- including `x`, `y`, `a*b*c`, or anything else. The basic convention in PURE is that all symbols appearing in the left-hand side of the pattern are considered to be pattern variables, except for symbols representing functions (i.e. the _function head_ symbols). Thus, `Cos` and `+` are _not_ considered to be pattern variables. Symbols declared "nonfix" are also "function heads" and thus are not considered to be pattern variables.

Since rules are defined by patterns and are not strictly tied with a particular function, several rules may be given for the same function. In that case, how does the pattern matcher decide which rule to apply? The answer is that all the rules are tried _in the order they appear in your code_. For example, we can write
```
Sin 0 = 0; // rule 1
Sin Pi = 0; // rule 2
Sin x::double = sin x; // rule 3: call numerical `sin`
```
This code will transform `Sin Pi` to 0 rather than to `sin Pi`. First, the code will try to match with the rule labeled "rule 1"; this rule does not match the symbol `Pi` (since we declared that symbol as "nonfix"). Then "rule 2" is tried and matches. The third rule will never be tried when evaluating `Sin Pi`.

Thus, the programmer needs to pay attention to the ordering of the rewriting rules. It is convenient to start with the most specific rules and postpone more general or catch-all rules to the end. Here is an example: We want to define a function that returns the square of a number; when the argument is not a number, the function should return "`-1`". A natural way to code this function in PURE is to use a **guard**, which is an "if" construction placed after the rule. (The standard predicate `numberp` returns `true` when its argument is a number.)
```
> square1 x = x*x if numberp x; // rule for numbers
> square1 _ = -1; // catch-all rule
> square1 345;
119025
> square1 "abc";
-1
```
When `square1 x` is evaluated, the "rule for numbers" is tried first because this rule is first in the code. If the guard condition (`numberp x`) fails to evaluate to `true`, the next rule is applied (in this case, the next rule is the "catch-all rule").

An equivalent but somewhat more visually appealing way to write `square1` is this:
```
> square1a x = x*x    if numberp x;
>            = -1     otherwise;
```
Note that the common part of the function body was omitted in the second rule. The keyword "`otherwise`" is actually optional and serves only to make our intent clear to ourselves (the last rule will be tried only when all other rules already failed).


**Question**: What if I define a more specific rule after a catch-all rule?

**Answer**: Then the catch-all rule will be always applied! You will also get a message to the effect that "rule never reduced." In other words, the PURE interpreter was able to figure out that one of your rules is _never_ going to be applied to _any_ expression. Most probably, this indicates an error in the program logic.
```
> square2 x::int = pow x 2; // rule for int's, promotes to bigint
> square2 _ = false; // catch-all rule
> square2 x::double = x*x; // rule for double's, remains double
> square2 10;
warning: rule never reduced: square2 x::double = x*x;
100L     // answer is correct for int's
> square2 1.23;
0     // answer is wrong for double's
```
What was probably intended in this case is to place the rules for `int` and `double` _before_ the catch-all rule.

## Walking through expressions ##

One of the deficiencies of our `Cos` example above is that we can recognize `Cos(2*Pi+x)` but not `Cos(x+2*Pi)`. In order to correct this, we would like to "simplify" an arithmetic expression into a small number of "canonical forms". For each of the possible canonical forms of `x`, we will write a rule for `Cos x`. Hopefully, there will not be too many of these canonical forms.

To keep this tutorial reasonably short, we will assume that the only allowed expressions are built from integers, the symbol "Pi", free variables like `x`, and arithmetic operations `+` and `*`. If we wanted to support non-integers, subtraction, division, powers, and other more complicated functions, we would have to continue developing the rule system in the direction that will become clear after this tutorial.

The first important task is to be able to "walk through" an arbitrary algebraic expression, such as `a*2+Pi*(b-2+c*(Pi-1))`, and to perform some task on that expression. This can be accomplished using the recursion and pattern-matching abilities of PURE.

### Detecting `Pi` ###

Let us define a predicate that checks whether a given expression contains the symbol `Pi` at all. This may save us work: If there is no `Pi` in the expression, we will not have to simplify that expression.

We already defined `Pi` as a nonfix symbol. The predicate `hasPi` is then defined recursively, case by case:
```
hasPi Pi = true; // rule no.1: Pi itself
hasPi (a+b) = hasPi a || hasPi b; // rule no.2: sum
hasPi (a*b) = hasPi a || hasPi b; // rule no.3: product
hasPi _ = false; // rule no.4: previous rules found no Pi
```

How does this work? Consider the evaluation of `hasPi (2*(z+Pi))`. The evaluation starts by trying rule no.1, but this rule does not match since the argument is not equal to `Pi`. Then rule no.2 is tried; it also does not match since the expression `(2*(z+Pi))` has the function "`*`" as the top function, so the pattern `(a+b)` does not match. Then rule no.3 is tried and matches, setting "`a`" to 2 and "`b`" to "`z+Pi`". Since rule no.3 matches, rule no.4 will not be tried now; instead, the expression `hasPi (2*(z+Pi))` is now rewritten to `hasPi 2 || hasPi (z+Pi)`.

The evaluation continues. Now `hasPi 2` returns `false` because rules no.1 to no.3 fail to match, so rule no.4 is the only one that matches. Therefore, the expression becomes `false || hasPi (z+Pi)`. Since the first argument of "`||`" is `false`, the second argument will be evaluated (otherwise the operator `||` would have stopped the evaluation and returned `true`). Therefore, we come to evaluating `hasPi (z+Pi)`.

Here rule no.2 matches and rewrites this expression to `hasPi z || hasPi Pi`. This evaluates to `false || true`, which in turn evaluates to `true`. Hence, `hasPi (2*(z+Pi))` evaluates to `true`.

This is the technique of recursive application of pattern-matching rules. Since it is important for you to understand this rewriting technique, let me reiterate some of the main features of the example we just considered.

  * The evaluation of `hasPi (2*(z+Pi))` can be pictured as a sequence of rewriting steps like this:
```
has_Pi (2*(z+Pi))
// apply rule no.3
has_Pi 2 || has_Pi (z+Pi)
// apply rule no.4 to the first term
false || has_Pi (z+Pi)
// drop "false" out of the logical "or"
has_Pi (z+Pi)
// apply rule no.2
has_Pi z || has_Pi Pi
// apply rule no.4 to the first term
false || has_Pi Pi
// drop "false" out of the logical "or"
has_Pi Pi
// apply rule no.1
true
// finished
```
  * Since the definition of `has_Pi` is recursive, the program will recursively walk through the given expression, examining every term as needed. We do not have to specify how many times the rules need to be applied; they are applied automatically whenever the patterns match, as many times as possible. We only specify the rules themselves, in the correct order.
  * A rewriting rule (e.g. rule no.1 - rule no.4, or a rule such as `a*(b+c)=a*b+a*c`) is to be understood as an _instruction for rewriting an expression_, not as  a mathematical identity. The order of the rules in the code need to be correct because the pattern-matcher will try to apply the rules in the specified order.

This example shows the power of the method of defining functions by pattern-matching and recursion. With just four simple rules, the function `hasPi` can handle arbitrarily complicated algebraic expressions involving symbols and the operations `+`, `*`.

Testing:
```
> hasPi  (x+y);
0
> hasPi ( 2*(z+Pi) + x*(y+1) );
1
```

**Exercise**: Explain why a PURE user got a strange result when evaluating the function `hasPi` on this input:
```
> hasPi Pi*Pi;
1*Pi
```

Below we will see many other examples of "walking through expressions." This is an important and powerful technique in term rewriting.

### Simplifying polynomials in Pi: first failed attempts ###

If an expression contains Pi, the next step is to bring the expression to the form "something plus a multiple of `Pi`." However, what if `Pi` is buried in the middle of a formula, like `2*x+3*(Pi-x)*4`? We need to implement rules that will expand all brackets and move `Pi` to the _outer_ "`+`" operation.

Recall that an expression such as `a+b+c` is really `(a+b)+c` in PURE. If we write a rule matching `x+y` against `a+b+c`, then `x` will match `a+b` and `y` will match `c`. Thus, to make matching easier, we would like the end result of our manipulations to be of the form `x+(N*Pi)`, where `N` is an integer and `x` is an expression not containing an integer multiple of `Pi`.

We could write some rules that move numerical factors to the left, rules that simplify numerical factors, rules that move additive numerical terms to the right, and rules that simplify additive terms:
```
x*n = n*x if numberp n;
// here n*m will be computed to a number
n*(m*x)=(n*m)*x if numberp n && numberp m;
n+x = x+n if numberp n;
(x+m)+n = x+(m+n) if numberp n && numberp m;
```

After this, we already have useful behavior, although not everything is simplified:
```
> 2*Pi*3;
6*Pi
> Pi+2*Pi*3+1;
Pi+6*Pi+1
```

We need to move all factors of Pi together. We could now try to write lots of rules so that all possible factors of Pi are moved to the right of the sum and combined into one term with Pi:
```
(x+m*Pi)+n*Pi = x+(m+n)*Pi if numberp n && numberp m;
m*Pi+n*Pi = (m+n)*Pi if numberp n && numberp m;
1*Pi = Pi; 0*Pi = 0; 
(a+n*Pi)+b = a+b+n*Pi if numberp n;
// lots more rules needed...
```
This approach seems to require a large number of rules. It also has another important drawback: If we redefine the rules for the operators "`+`" and "`*`", these new rules will be applied to _every_ algebraic expression, whether or not we actually need to look for `Pi` in that expression.

The lesson is that the PURE compiler does not by itself know mathematics or logic; we have to program all needed transformations ourselves. This is a difficult task. Clearly, a different strategy is needed.

Let us separate the task of expanding an expression from the task of gathering the factors of `Pi` in an expression. We will first create a function that expands brackets in algebraic expressions, so that there are no nested brackets. Then a second function will collect the factors containing a given variable. In this way, we will create functions that are generally useful in the context of symbolic algebra. We can then use these functions for collecting the factors of `Pi`.

## Expanding brackets in expressions ##

Our present goal is to write a function that expands all brackets in algebraic expressions containing the operators "`+`" and "`*`". Let us call this function `expand`. This function should transform, for example, `a*(b+c)*d` into `a*b*d + a*c*d`.

We could have defined the rules for the operators "`+`" and "`*`" such that the expansion of brackets is always automatically performed:
```
 a*(b+c)=a*b+a*c; (a+b)*c=a*c+b*c;
```
However, since the "automatic expansion" rules are global, they will result in a global change of the behavior of the operators "`+`" and "`*`" so that all brackets will be expanded in every expression, every time. What we need instead is a function that will expand the brackets only when required.

There are three basic approaches to solving this problem without modifying the global behavior of "`+`" and "`*`":

1. A simple-minded approach: Define the function "`expand X`" and write some rules with left-hand sides such as
```
expand(a*(b+c))=expand(a*b)+expand(b*c);
expand((b+c)*a)=expand(b*a)+expand(c*a);
// more rules...
```
so that `expand(X)` will be eventually rewritten to `X` with all brackets expanded.

2. Everywhere within the given expression `X`, replace the operators "`+`" and "`*`" by temporary operators, say "`plus`" and "`times`". This gives a new expression, say `Y`. Define the automatic expansion rules  for these temporary operators as shown above, so that all brackets are expanded; then evaluate `Y`. This gives a new expression, `Z`. In this expression, replace the operators `plus` and `times` by "`+`" and "`*`".

3. Define the automatic bracket expansion rules for "`+`" and "`*`" as _local rules_ and re-evaluate the expression `X` using the built-in function "`reduce`". (The ability to re-evaluate expressions using local rules is a powerful trick.) The global behavior of "`+`" and "`*`" is unchanged; the local rules are in effect only while evaluating a certain single expression to which they are attached.

The last approach is the most efficient and concise. However, seeing how the other two approaches work is instructive and will help us learn the techniques of programming with rewriting rules and pattern matching.

### Expanding brackets by straightforward rules ###

The first approach to expanding brackets is to define a function `expand` with sufficiently smart rewriting rules. Let us see what problems arise in this approach and how they can be solved.

We begin with the rules
```
expand (a*(b+c)) = expand (a*b) + expand (a*c); // 1
expand ((b+c)*a) = expand (b*a) + expand (c*a); // 2
```
These rules will expand brackets not only in expressions such as `(x+y)*2`, but also in expressions such as `(x+1)*(y+2)`. How does this work? The expression `(x+1)*(y+2)` is an application of "`*`" to two arguments, `(x+1)` and `(y+2)`. Rule 1 will match, so the expression `expand ((x+1)*(y+2))` will be rewritten as `expand ((x+1)*y) + expand ((x+1)*2)`. Now the second rule will match in both cases; the result will be `expand (x*y) + expand(1*y) + (expand(x*2) + expand(1*2))`. At this point, `1*2` will be simplified to `2` but no more simplifications occur. We don't yet have any rules for `expand (x*y)` or `expand(2)`. However, the bulk of the work is finished: We will later add a catch-all rule, such as
```
expand x = x; // catch-all rule, must be last in the code
```
in order to remove the "`expand`" function from terms that cannot be expanded.

Note that it is important to define the rules 1 and 2 recursively, i.e. using "`expand`" in the right-hand sides. This recursive technique ("walking through expressions") allows us to break expressions into pieces and to continue working on all subexpressions whenever possible.

It is actually too early to write the catch-all rule. The rules 1 and 2 are not yet sufficient because expressions with nested multiplications, such as `a*(b*(c+d))` or `x*(a+b)*y` will not be expanded by these rules (the sum is not sufficiently near the top level of the expression). To solve this problem, we have to add more rules.

For instance, we need to handle expressions of the form `a*(b*c)`, which are syntactically different from `(a*b)*c` although algebraically equal. "Expansion of brackets" usually means suppressing all such unnecessary brackets as well; note that the expression `a*b*c` means `(a*b)*c`. So let us write the rule that will remove the unnecessary brackets.
```
expand (a*(b*c)) = expand ((a*b)*c); // 3
```
This rule will also enable us to handle all expressions of the kind `a*(b*c)`, once we know how to handle `(a*b)*c`.

We note that the rules 1 to 3 will expand an expression of the form `(a*b)*c` whenever "`c`" is a sum, but not otherwise. For instance, one of "`a`" or "`b`" could contain a sum, but the rules do not detect that.

One quick solution is to expand "`a*b`" within `(a*b)*c)` by a recursive call to `expand`. We can try this:
```
expand ((a*b)*c)) = expand (expand(a*b)*c); // is this ok ??
```
However, this will fail when `a*b` cannot be expanded, e.g. when we evaluate `expand (2*Pi)*3`, because then `expand ((2*Pi)*3)` is rewritten again into `expand ((2*Pi)*3)`, causing an endless loop. To prevent this, we need to check whether `a*b` transformed to the same expression. If it is, then we do not need the recursive call to `expand`. So we replace the above rule by
```
expand ((a*b)*c)) = 
 if x === a*b then x*c else expand (x*c) 
  when x = expand(a*b) end; // 4
```
After this rule, "`x`" will contain an explicit, top-level sum if "`a*b`" were expandible; thus one of the first two rules will apply. The expression "`c`" will eventually be expanded as well. If, on the other hand, "`a*b`" were not expandible then   we return "`x*c`" rather than `expand(x*c)`, to avoid the infinite loop. We need to make sure that this is correct, and we use the following argument: the expression "`c`" cannot be a sum, so either it is a symbol or a product of some factors. If "`c`" were a product of some factors, say `x*y`, it means that we are rewriting the expression `expand ((a*b)*(x*y))`. However, this expression is already handled by the previous rule and would have been rewritten into `expand (a*b*x*y)`. Therefore, "`c`" cannot be a product, so is a symbol, and returning "`x*c`" is correct.

Note that we used the _syntactic equality_ operator "`===`". If we used an ordinary equality operator, "`==`", we would have gotten an error because, for instance, `a*b+a*c==a*(b+c)`" does not evaluate to `true` or `false`.

The "`expand`" function will gradually disassemble every part of the expression until it comes to the final simple terms (symbols or numbers). So we need a trivial rule that will handle these terms.
```
expand x = x; // catch-all rule, must be after rules 1-4
```

Now "`expand"` works as expected.
```
>  expand (x*(2+y)*((3+y+z)*2));
x*2*3*2+x*y*3*2+(x*2*y*2+x*y*y*2)+(x*2*z*2+x*y*z*2)
```
There are still some extra parentheses around the sums; but there are no parentheses within multiplications.

**Exercise:** Write a function, `exsum`, that will "expand the sums", i.e. remove all the unncessary parentheses within the sums. It should work like this:
```
> exsum ( a*2 + (b*2+(1+b)) + (c+2) );
a*2+b*2+1+b+c+2
```
Note that the expression `a+b+c` is the same as `(a+b)+c`, and the parentheses are not printed in this case because "`+`" is a left-associative operator. However, the expression `a+(b+c)` will be printed with parentheses. So the task is to get rid of all instances of this expression.

**Solution:**
```
exsum ( a+(b+c) ) = exsum (exsum a + exsum b + exsum c);
exsum x = x;
```


### Expanding brackets by replacing functions ###

The second approach to expanding brackets is to replace the operators "`+`" and "`*`" in the expression by temporary operators, `plus` and `times`. The replacements can be written more or less straightforwardly:
```
replace1 (a+b) = plus (replace1 a) (replace1 b);
replace1 (a*b) = times (replace1 a) (replace1 b);
replace1 x = x; // catch-all rule

replace2 (plus a b) = (replace2 a) + (replace2 b);
replace2 (times a b) = (replace2 a) * (replace2 b);
replace2 x = x; // catch-all rule
```

Testing:
```
>replace1 (2*(a+b)*c);
times (times 2 (plus a b)) c
>replace2 ans;
2*(a+b)*c
```

The temporary operators will be defined with such rules as to expand brackets automatically (although the "brackets" have become invisible after `replace1`). We also remove parentheses in left-associative operators.
```
times (plus a b) c = plus (times a c) (times b c);
times c (plus a b) = plus (times c a) (times c b);
times a (times b c) = times (times a b) c;
plus a (plus b c) = plus (plus a b) c;
```

Now we write the `expand` function:
```
>expand X = replace2 (replace1 X);
>expand (2*(a+b)*c);
2*a*c+2*b*c
```

The function `expand` is now defined as a composition of two functions, `replace1` and `replace2`. There is a standard operator for this, the "dot" operator, so that we can rewrite the above as
```
expand = replace2 . replace1;
```

### Replacing function heads in expressions ###

**Question:** The code for `replace1` and `replace2` is quite repetitive. If more operators need to be supported, a lot of cut-and-paste will arise. Can this be avoided?

**Answer:** Yes. The solution is to write a rule that can replace a given _function head_, say "`+`", by another given function head, say "`plus`", everywhere in the expression. This kind of rule cannot be written simple-mindedly as
```
f a b = if f==(+) then (plus a b) else (f a b); // wrong
```
because then "`f`" is assumed to be a function head and not a pattern variable; the rule as written above will only match functions called "`f`", so "`f==(+)`" is always `false`. What we want is to have a pattern variable which is located at the position of the function head.

This can be done in PURE with the following trick: One writes "`_`" instead of the function head. The special symbol "`_`" denotes a pattern variable that matches everything. By a special arrangement in the PURE language, the symbol "`_`" matches not only any expression but also _any function head_. Thus, the rule
```
_ a b = a; // useless but valid
```
will replace _any_ function with two arguments by its first argument. This trick is now combined with the named patterns: if we write "`h@_`", we will match any function head and, at the same time, bind the pattern variable "`h`" to the function head symbol.

Now we can write a general function that will replace all occurrences of a given function "`f`" in an expression by a function "`g`". For simplicity, let us assume that `f` and `g` are binary functions.
```
replace_f f g X = replf X
with
    replf (func@_ a b) = (replf func) (replf a) (replf b);
    replf x = if x==f then g else x;
end;
// now we can implement replace1 and replace2 using currying:
replace1 = replace_f (+) plus $ replace_f (*) times;
replace2 = replace_f plus (+) $ replace_f times (*);
```

Remarks:

  1. In this implementation we use a local function, `repl`, merely for brevity. Otherwise we would have to write `(replace_f f g x)` instead of `replf x` every time. So we save space writing 'f g' all the time; this is of course not essential.
  1. The recursive call of `replf` is applied to _every_ element of every expression, including the function heads and all the function arguments. Thus, `replace1 X` will replace every occurrence of "`+`" by "`plus`", also when the expression X contains the function "`+`" not only as an operator but also as _data_, i.e. as an argument to some other function.  For example, `replace1 (func (+) x (y+z))` evaluates to `func plus x (plus y z)`. If this is not desired, the rules for `replf` need to be modified.
  1. The operator "`$`" applies a function to its argument. It is defined as
```
f $ x = f x;
```
This operator is merely a cosmetic means to reduce the number of parentheses. For example, instead of writing
```
a = f1 x (f2 y z (f3 (f4 u))); 
```
we can write equivalently
```
a = f1 x $ f2 y z $ f3 $ f4 u;
```

The function `replace_f` as defined above is limited to replacing functions of two arguments. Currying is such a powerful mechanism that we can define a replacement function that works on functions with any number of arguments:
```
replace_all f g X = replf X
with
    replf (func@_ a) = (replf func) (replf a);
    replf x = if x==f then g else x;
end;
```

**Question:** How does this work? This looks like replacing a function of a single argument.

**Answer:** Remember that with the curried syntax, _any_ function looks like a function of a single argument. Suppose we want to replace the function `f1` with three arguments by the function `g1`. We write
```
> replace_all f1 g1 (f1 1 2 3); 
g1 1 2 3
```
When the pattern "`func@_ a`" is matched against "`f1 1 2 3`", the variable "`func`" gets bound to the curried function "`f1 1 2`" and "`a`" is bound to 3. This is so because the function application "`f1 1 2 3`" is fully equivalent to the application of the curried function, "`f1 1 2`", to the single argument 3. Thus, the pattern "`func@_ a`" will actually match _any_ function application, with an arbitrary number of arguments. (This pattern will not match a single symbol or number; so we need a second rule for `replf`.)

Thus, "`f1 1 2 3`" is rewritten to `(replf (f1 1 2)) 3`. This is then rewritten to `(replf (f1 1) 2) 3` and finally to `((g1 1) 2) 3`, which is the same as simply `g1 1 2 3`.

### Expanding brackets through local rules ###

The third method for expanding brackets is based on the combination of two tricks. The first trick is to use _local rules_,that is, rules introduced in the "`with`" clause. With these rules, we modify the behavior of the functions "`+`" and "`*`" so that all brackets are expanded. The second trick is to use the built-in function "`reduce`". This function forces a new evaluation of the argument.

For example, a function that removes parentheses in sums can be coded like this:
```
> exsum f = reduce f with a+(b+c)=a+b+c; end;
> exsum (x+(y+z));
x+y+z
```

The implementation of "`expand`" is the most concise of all:
```
expand X = reduce X 
with
    a*(b+c)=a*b+a*c; 
    (a+b)*c=a*c+b*c;
    a*(b*c)=a*b*c;
    a+(b+c)=a+b+c;
end;
```

Remarks:

  * This method will not work without the "`reduce`"! A rule such as `expand X = X when... with...` will not change "`X`" at all.
  * The new rules for the functions "`+`" and "`*`" have effect only within the body of the function "`expand`". The global behavior of "`+`" and "`*`" is not affected.
  * Technically speaking, the functions "`+`" and "`*`" inside the "`with`" clause are _different_ from "`+`" and "`*`" outside, because they have different rewrite rules! (Once we added some more rules for these functions, we have effectively changed them into different functions.) We can verify that the inside-defined "`+`" function is indeed a different function by the following code that _returns_ this function:
```
> newplus   =  (+) with a+(b+c)=a+b+c; end;
> newplus a b;  // looks like a "+" operator
a+b
> newplus a (newplus b c); // but expands brackets
a+b+c
> newplus === (+);  // two different functions!
0
```
  * Is the result of "`exsum`" or "`expand`" perhaps an expression that silently uses a different "`+`" operator? No! The result of "`expand`" will use the old, global "`+`" operator. PURE has a special convention when an expression already containing an outside-visible function is `reduce`d with local rules for that function. Namely, the outside-defined "`+`" is temporarily replaced by the inside-defined "`+`"; the expression is evaluated; and the original "`+`" is then restored. To verify this, we can run the following code using the above definition of "`exsum`":
```
> exsum x = reduce x with a+(b+c)=a+b+c; end;
> q = exsum (a+b); // get some expresion
> plus = case q of  
>   f@_ _ _ = f; end;  // extract the function head
> plus === (+);  // the same function!
1
```
Note that the function "`newplus`" defined above will always return the new "`+`", even if we write "`reduce (+)`". This is so because the "`+`" operator is already redefined within the function body. The only way to get the old "`+`" is to use a previously built expression that uses the old "`+`".

Thus, the trick with local rules is essentially equivalent to the second method (replacing all operators with temporary functions) but looks more elegant and executes faster.

## Collecting terms in polynomials ##

Now that we can expand brackets, we can bring every expression into a form that is a sum of products, and the products contain no nested sums. Our goal is to extract the terms proportional to "`Pi`" from the expression, so that we could simplify the cosine.

We could simply collect all terms proportional to "`Pi`", separating these terms from any terms not containing `Pi` or containing higher powers of `Pi` such as `Pi^2`, etc. However, this will not be sufficient. We would sometimes get expressions such as
```
2*Pi*x+3*Pi+Pi*x*(-2)
```
It would then be not sufficient to transform this into
```
Pi*(2*x+3+x*(-2))
```
because we need to recognize that the above is equal to `3*Pi`. We need to be able to gather all similar terms and cancel any expressions that identically vanish, like `2*x+x*(-2)`.

Therefore, a useful next task is to simplify terms in polynomials. The way to do this is to transform an expression to a "canonical form", such as `2+3*x+5*x*y`, where each kind of term can only occur once, so that there are no repeated occurrences of `x*2` and `x*3` anywhere.

tbc

# Extended example 1: `tolatex` #

Let us write a function, `tolatex`, that will convert a symbolic mathematical formula to its `LaTeX` representation. For example, we want the following functionality:
```
> tolatex (sin a * ln(b/c+f*2^(x+1)));
$ (\sin a) \ln\(\frac{b}{c}+f*2^{x+1}\) $
```

There is a standard function, "`str`", for obtaining a string representation of a PURE object. The LaTeX code is not very different from the simple string representation. As a first try, we can simply walk through the expression recursively and replace all PURE terms by their string representations. The "`+`" operator can concatenate strings for us. We build the definition of `tolatex` by recursion, starting from specific cases:
```
tolatex x  = str x if numberp x || varp x; // numbers and symbols
tolatex (a+b) = tolatex a + " + " + tolatex b;
tolatex (a*b) = tolatex a + " " + tolatex b;
tolatex (a^b) = tolatex a + "^{" + tolatex b + "}";
tolatex (a/b) = "\\frac{" + tolatex a + "}{" + tolatex b + "}";
```
Before proceeding further in this direction, let us see if we are on the right way.

tbc


# Extended example 2: Implementing a noncommutative algebra #