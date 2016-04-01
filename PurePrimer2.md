

This tutorial is for people who may have very little or no experience with functional programming. You need to have PURE installed so that you can try the examples.

# Functional programming #

PURE is a language well suited for symbolic and functional programming. Let us begin with the "functional" part.

**Functional** programming means working with functions. A function can be roughly understood as a block of computer code that takes some input values and returns an output value. The idea is that the output must depend only on the inputs; i.e. the output is a _function_ of the inputs. (This is quite similar to the idea of functions in mathematics.)

## Defining functions in PURE ##

Recall that function calls in PURE are usually written not as `func(x,y)` as in many other programming languages, but as `func x y`, without commas and without parentheses (although parentheses will be used around `x` if `x` is itself another expression). This syntax is similar to LISP where function calls are written as `(func x y)`, except we can omit the parentheses. The syntax `func x y` is also similar to the that of Unix shell commands, such as `cp file1 file2 dir/`, where the arguments are separated from the command name simply by spaces.

**Question**: Can I write `func(x,y)` in PURE, instead of `func x y`?

**Answer**: You can, but this means the function `func` applied to a _single_ argument, which is a pair `(x,y)`. In many cases, this is not much different than `func x y`, except that you will have to write the parentheses and the comma every time. I will save myself the trouble. Also, the syntax `func x y` allows "currying" (see below).

**Question**: The function syntax `func x y` is confusing if combined with arithmetic. How can I understand the expression `func x + y`?

**Answer**: This expression means `(func x) + y`. Function application always binds first, before any arithmetic or other operators. You can think about this as being similar to traditional mathematical notation such as "cos _x_ + _y_", where "cos" always binds first. In PURE, this rule is extended to functions of many arguments. Thus, `f x y + z` means the same as `(f x y) + z`. To apply the function `cos` to the expression `x+y`, we need to write `cos(x+y)`, just as in mathematics, putting `x+y` in parentheses.

For defining new functions in PURE, we have several options. One option is to say, essentially, "let `func1` be the function that takes `x` and `y` and returns `x*2+1-y`". This can be written in PURE as follows,
```
func1 = \ x y ->  x*2 + 1 - y;
```
The backslash and the arrow are special operators. This is called a **lambda-construction**; the backslash is supposed to resemble the Greek letter _lambda_ (and to remind us of A. Church's "lambda-calculus"; the word "lambda" here means nothing by itself, and is used for historical reasons to denote functions created in this way).

A lambda-construction can be seen as a quick way of creating a function "on the fly." A lambda-construction can be written in the middle of an expression, passed as an argument to another function, assigned to a variable, etc. This kind of manipulation with functions is a standard tool in the functional programming paradigm.

For a beginner in functional programming, lambda-constructions may appear strange. It is perhaps easier to understand the equivalent code that looks like this,
```
func1 x y = x*2 + 1 - y;
```
This is also perfectly acceptable in PURE. In the left-hand side of an equation, one writes the function application as it would normally appear in code, namely `func1 x y`. In the right-hand side of the equation, one writes the **body** of the function. The body is simply an expression -- in fact, nothing but a _single_ expression is allowed in the function body. For clarity, in this tutorial I will refer to the value of this expression as the **return value** of the function. (Note that there is no `return` keyword in PURE.)

**Question**: I can write only a _single_ expression?? So I cannot define a function like I do in C or Python, by declaring local variables, doing loops, writing code like `if (x==0) return y; else y=y+2;` and so on?

**Answer**: That's right: in PURE you cannot have loops, or return from the middle of the function, or update variables by writing `y=y+2`. Think about functions as _mathematical_ functions, like `f(x,y)=x*2+1-y`. In such a function, you need to define how the result is computed using the input values. In mathematics, one defines functions without using loops and without writing things like `y=y+2`. Functional programming is like mathematics in this sense. However, for people who first learned programming in languages like C or Python, it may appear that writing code without loops or updates is difficult. Actually it is easier than with loops! You just need to become familiar with the tools that are available in functional programming languages, such as PURE, for the purposes of defining functions.

**Question**: OK, so it is easy to define a function that is just a straightforward formula, like the `func1` shown above, either with a lambda-construction or with an equation. However, the requirement that the function body is a _single expression_ seems to be a very severe limitation! It is not clear what to do in PURE if I need one of the following:
  1. use if/then/else;
  1. declare local variables to compute and store intermediate results of computations;
  1. repeat some computation many times in a loop, or until some condition is reached;
  1. return from the middle of the function body if needed (where is the `return` keyword?);
  1. update the value of a global variable;
  1. allocate more memory or free up some unused memory;
  1. print something, read from a file, etc.
How can I do this, within a single expression?

**Answers in brief**:
  1. We _do_ have if/then/else in PURE - in fact, there are many convenient forms of these, readily usable in expressions. Think of the C operator `(x==0)? y : z`, which is an if/then/else expression in disguise. PURE has a much more powerful form of this operator (the pattern matching construction), which we will talk about later.
  1. There are convenient ways of declaring local variables and/or local _functions_ if you need to use them in your function body.
  1. Recursion can _always_ replace a loop. See below for more explanations!
  1. You can easily have the effect of returning from the middle of the function body -- without a `return` keyword. This can be accomplished, for instance, with if/then/else.
  1. Most of the time you can write code without global variables whose value is updated from within some functions; but you _can_ have them in PURE.
  1. Memory management is automatic, you will not need to allocate or free memory yourself. For example, lists grow automatically as needed.
  1. You can read and write files within a single expression. For instance, the `when ... end` clause with local variables can be used for this purpose.

It is best at this point to look at some self-explanatory examples of PURE code.

The if/then/else construction and its variants (the `case` construction and the `if` guards on the rules) are expressions. The PURE expression `if x==0 then 1 else 2` is completely analogous to the C code `((x==0) ? 1 : 2)`. Note that it is illegal to omit "else" because the result must be _something_ (some expression), whether or not the condition holds. This is just like in C where it is illegal to omit "`: 2`" in the C expression `((x==0) ? 1 : 2)`.

Here is an example of using if/then/else in a function definition:
```
func2 x = 
   if x<=0 
   then 1-x 
   else if 0<x && x<=1 
        then 1
        else x-1;
```
The same function can be written in a different syntax (more visually clear) using guards and omitted left-hand sides:
```
func2 x = 1-x  if x<=0;
        = 1    if 0<x && x<=1;
        = x-1  otherwise;
```

Local variables can be used to store intermediate results, and local functions can be defined for convenience. Use the `when ... end` construction for variables and `with ... end` for functions.
```
func3 x = (x-y)/(x+y) 
  when a= x*2; b= a*a; y = b*b+func1 x; 
 end
  with func1 q = q+2*sqrt q;
 end;
```
It perhaps feels a little strange to use the variable or function before it is defined; to understand this program, we should first read the `with` and `when` constructions. This is similar to mathematical literature where we see sentences like "This value is equal to _x_+_z_ where _z_ is such that ...". First we read about _z_, and then we understand the whole sentence.

Another important difference from languages like C or Python is that local variables (the ones defined in the `when` construction) are **immutable**: once defined, they cannot be redefined or updated. At first sight, this appears to be a serious limitation, but this is not really so, as will be explained in more detail below. Briefly, the reason is that the functional style of programming helps us to split the code into single expressions rather than into "blocks of code". Constructions such as loops or "do" blocks, familiar to us from FORTRAN and C, are not used in PURE; instead of loops, we use recursion, and instead of "do" blocks, we use separate functions. Each function evaluates a single expression. Within a single expression, there is no possibility of looping and thus no need for updating variables.

**Question**: What if I redefine a variable within a `when` clause:
```
> func x = x+y
>   when y=1; y=y+1; a=3; y=y+a; end;
> func 2;
7
```
You said that the variables are immutable, but here I have changed the value of `y` twice, and the result is correct: the final value of `y` is 5, so `func 2` returns 7.

**Answer**: Sure, you can write code like that. However, this code is fully equivalent to the following:
```
func1 x = x+t
 when y=1; z=y+1; a=3; t=z+a; end; 
```
Here I renamed `y` to `z` and to `t` every time you "updated" `y`. Now each variable is only assigned once and then stays constant. This code is preferable because its logic is easier to understand. Why don't we write this in C? In imperative languages such as C, variable updates are normally used in a loop such as `while(...) { y=y+1; ...}`. In C, we could not have renamed `y` to `z`  and replaced this with `z=y+1` because we want to come back to `y=y+1` many times in a loop: each time we want to access the _currently updated_ value of `y`. In PURE, while you are writing code within a `when` clause, there is no possibility of looping or of going back to a "previous place" in the code of that clause. All definitions within the body of `func1` are really just parts of a single expression! So yes, we may write something like `... when y=1; y=y+1; y=y+2; ...`, but there is no possibility, within a `when` clause, of repeating the assignment `y=y+1` after `y` has been redefined. Nor there is any possibility of accessing the "old" value of `y`. The `when` clause is evaluated just once, left to right in the order given. Only the final value of `y` is accessible from outside the `when` clause; there is no way of accessing any "intermediate" value of `y` as seen in the middle of the `when` clause. Therefore, our PURE program will never be able to make use of the fact that we are re-using the name `y`  (writing `y=y+1`) instead of introducing a new temporary name, (writing `z=y+1`). For this reason, the PURE compiler allows you to re-use the name. The difference between `func` and `func1` is only in using two more names for temporary local variables; a good optimizing compiler should recognize this and compile `func1` essentially into the same machine code as `func`.

Here is a basic illustration of using recursion instead of loops. The sum of integers from 1 to _n_ could be computed by looping while _x_>0: (this code is in C)
```
int func4(int n) {
   int x, sum; /* temporary variables */
   if (n<=0)
      return 0;
   for (x=n, sum=0; x>0; x--) /* update sum and x in a loop */
      sum += x;
   return sum;
}
```
In PURE, we use recursion instead of a loop.
```
func4 n = 
   if n<=0 then 0 
   else n + func4 (n-1); 
```
Note that we do not need to update local variables or to return from the middle of the function body. Instead, we supply an if/then/else construction that gives the result, `0`, right away if the condition `n<=0` holds. The second case requires a recursive calculation and is written in the "else" clause. The code is shorter and cleaner overall.

Test this in the PURE interpreter:
```
> func4 10;
55
```

The function `func4` can be rewritten in a more visually clear style like this:
```
func4 n = 0 if n<=0;
        = n + func4 (n-1) otherwise;
```

This style of programming is indeed quite different from that of C and other such languages. It takes time to get used to, but it is well worth getting used to!

**Question**: I see that there is no "`end if`" keyword. I understand that the "`else`" clause is mandatory. However, once I write "`if`", "`then`", and "`else`", how can I get out of the `if/then/else` construction and continue writing code for this function?

**Answer**: Actually, there is no _need_ for an "`end if`" keyword! Once you are in the "else" clause, there is no escape: The entire remaining function body will have to be inside that "else" clause. But that's all right: There is no reason why the rest of the expression cannot fit into the "else" clause. You will never need to get out of `if/then/else` and to "continue writing code" because _all_ your remaining code for that function is going to be a _single expression_. You are just going to compue the _value_ of the `if/then/else` construction, and that's it (unless the `if/then/else` construction is a part of a larger expression). The `if/then/else` construction is like a mathematical function that is defined piece-wise for different cases. Here is an example (with no deep mathematical significance):
```
func5 x = (if x<0 then x/2 else if x<1 then -x else x-2)*x - 1;
```
Also, the definition of `func2` above shows an example of using `if/then/else`.

**Exercise:** Using the `if/then/else` and the `when` constructions as needed, define a function `nroots a b c` that computes the number of real roots of a cubic equation x<sup>3</sup> + a\*x<sup>2</sup> + b\*x + c with real coefficients.

**Answer:**
```
nroots a b c = if d>0 then 3 else if d==0 then 2 else 1
when
  d=18*a*b*c-4*a^3*c+a^2*b^2-4b^3-27*c^2
end;
```

**Question**: I feel burdened and constrained in my coding when I think that I cannot escape from the "else" clause!

**Answer**: Take heart. Instead you could say to yourself: Oh, I'm already in the "else" clause, so there is only one expression left -- the code for this function is soon to be finished!

The `if/then/else` construction can be seen as merely syntactic sugar for a _function_ that returns one value or another depending on a condition. The LISP language has this function called `COND`. We can define this function ourselves:
```
cond true x y = x;
cond false x y = y;
```
Then `if p then x else y` is similar to `cond p x y`.

I say "similar" because in PURE there is a difference between this `cond` and the actual `if/then/else` construction. The `cond` function will always _first_ evaluate all of its arguments, including `x` and `y`, and only then decide which of `x` or `y` to return. The `if/then/else` construction will always evaluate `p` but will actually _not evaluate_ `y` if the condition is true (or `x` if the condition is false). For example:
```
> loop = loop + 1; // this is a circular calculation
> if true then 1 else loop; // ok
1
> if false then loop else 2; // ok
2
> cond true 1 loop; // will cause stack overflow
Segmentation fault
```

You _can_ define functions that do not always evaluate their arguments. This can be done using **special forms** and **macros** in PURE. The `if/then/else` construction is itself a built-in "special form," i.e. a function that does not evaluate some of its arguments.

## Lists and recursion ##

The main built-in value types in PURE are integers, floating-point numbers, text strings, and lists. Let us briefly talk about lists, so that we learn the syntax and will be ready to proceed to some programming examples.

Lists are especially well-suited for recursive programming because lists are _defined_ recursively. A list is either an empty list `[]` or a value (say `x`) followed by another list (say `xxx`); this is written as `x : xxx`. I chose the name `xxx` to suggest that this is "a list having many `x`". Many books on functional programming use the names `x : xs` instead; I find `xxx` more visual.

Note that the operator "`:`" does not evaluate to anything, so it plays the role of a "constructor" for unevaluated expressions such as `1 : 2 : a : []`. These expressions are, by definition, lists.

To make the notation nicer, we write lists also by enclosing the elements in square brackets. Thus, we write `[1]` instead of `1:[]`. Then, `[1,2,3]` is the same as `1:[2,3]`, which is also the same as `1:2:3:[]`. Thus, lists can be seen as expressions built using the operator `:` and the empty list `[]`.

Standard functions operating on lists are `head` and `tail`. These functions operate by using the recursive definition of the list: The "head" of `x:xxx` is `x`, and the "tail" of `x:xxx` is `xxx`. In other words, the functions `head` and `tail` can be defined immediately in PURE, as follows:
```
head (x:xxx) = x; // parentheses around (x:xxx) are required!
tail (x:xxx) = xxx;
```
All other functions for list processing can be defined, in principle, through `head` and `tail`.

**Question**: In C, we learn about implementing lists through pointers. We learn to implement memory allocation, single-linked and double-linked lists, iterators on lists, and so on. What is a list in PURE in terms of C?

**Answer**: It is a single-linked list with automatic memory allocation. Note that we can only obtain the tail of a list; so it's a forward-linked list. There is no notion of a "pointer to a list element" because all objects are regarded as immutable values. (What would it mean to have a pointer to `b` in the list `[a,b,c]` if you can't change the list?) Since we have no pointers to elements, we cannot have list iterators, and thus we cannot go backwards in a list. It is impossible to implement a double-linked list within the pure functional paradigm, i.e. by using some recursive functions on immutable values. Instead, one uses single-linked (forward-linked) lists. It turns out that almost all tasks can be implemented with techniques involving only forward-linked lists.

Let us see how one can use recursion to implement typical list processing techniques. The first example is a function that counts the number of elements in the list. In order to implement this function in PURE, we just need to think about the definition of this function (in the mathematical sense). If a list is defined as either `[]` or `x:xxx` where `xxx` is a list, then what is the length of a list? The length of `[]` is zero, and the length of `x:xxx` is `1+`(length of `xxx`).

These words are translated into PURE code as follows:
```
length_of_list [] = 0;
length_of_list (x:xxx) = 1 + length_of_list xxx;
```
We seem to give two different definitions of `length_of_list`. This is perfectly admissible in PURE; the two definitions may be called **rules** applicable to the expression `length_of_list z`. Note that the arguments of `length_of_list` in these two rules are different. When we call `length_of_list z` on some `z`, PURE performs a matching procedure to determine which rule actually applies. If `z` is an empty list then the first rule will be used. If `z` matches the expression `(x:xxx)`, the second rule will be used.

Let us look at the second rule in more detail. The argument of `length_of_list` is `(x:xxx)`. This expression, `(x:xxx)`, is understood as a **pattern**: the second rule is used when `length_of_list` is applied to some expression of the form `(x:xxx)` where `x` and `xxx` are arbitrary expressions. Thus, if we evaluate `length_of_list [1,2]`, this pattern will match, and in particular, the symbols `x` and `xxx` will match `1` and `[2]`. (Recall that `[1,2]` is the same as `1:[2]` or `1:2:[]`.) In this case, the second rule will be used for evaluating the expression `length_of_list [1,2]`. But the pattern `(x:xxx)` will not match if we evaluate `length_of_list 123` because `123` is not a list, i.e. not an expression of the form "something" `:` "something". In that case, actually, none of the two rules will apply, so `length_of_list 123` will evaluate simply to itself.

It is important to realize that after a successful match of `(x:xxx)` to, say, `[1,2,3,4]`, the symbols `x` and `xxx` will be _assigned_ in the right-hand side of the rule with values `x=1` and `xxx=[2,3,4]`. Therefore, the symbols `x` and `xxx` in the pattern `(x:xxx)` behave like temporary, automatically introduced **pattern variables**. Pattern variables get values after a successful matching of the pattern, here `(x:xxx)`, with the actual argument of the function. So we may use these variables, here `x` and `xxx`, when we define the body of the rule. This is what we have done in the example above: we defined the body of the rule using the pattern variable `xxx`. (In this case, we did not need to use the variable `x`.)

Note the superficial similarity of the rule definition with the ordinary function definition in a language like C, where we can write `int func(int x) { ... }`. In this case, `x` is an automatically introduced temporary variable that is available for us to use in the body of the function. The convention in PURE is exactly the same: anything that appears in the left-hand side, except function symbols, is a variable symbol. Just like in C, it does not matter which symbol we choose for the variable; the identifiers `x` or `xxx` are just as good as `A2` or `initial_element_of_the_list`.

If there exist several rules for the same function, the PURE engine will try each of these rules in the order they are declared, until one of the patterns matches successfully. If none of the patterns matches successfully, the expression will be left unevaluated (no error occurs)!

You also have to take care that all the different patterns given for one function will actually match with _different_ kinds of expressions. If you make a mistake in this, you might get a warning that "the rule was never reduced," meaning that a different rule already covered some pattern, so this rule is forever inactive. For example, this will occur if you try to define `length_of_list` like this:
```
length_of_list [] = 0; // 1st rule
length_of_list x = false; // 2nd rule
length_of_list (x:xxx) = 1 + length_of_list xxx; // 3rd rule
```
We intended to get `false` when `length_of_list` is called on a non-list object. But actually the second rule will already match _any_ expression to `x`! It is a catch-all rule that will surely succeed if the argument is not `[]`. Therefore the third rule will be never activated. If you try this in the PURE interpreter, you get the wrong answer and a warning message:
```
length_of_list [1,2];
warning: rule never reduced: length_of_list (x:xxx) = 1+length_of_list xxx;
0
```
(Note: In PURE, the constant `false` is the same as the integer 0.) The right approach is to write the catch-all rule as the _last_ rule, so that it is applied only when all other rules failed to match.

**Question**: Why can we write `1 + length_of_list xxx`? It seems to me that we need parentheses: `1 + (length_of_list xxx)`.

**Answer**: This would be perfectly all right, but not necessary: a function application binds tighter than infix operators. Think about mathematical notation, `1+cos x`, where "cos x" always binds tighter.

**Question**: Will `(x:xxx)` match a list with _one_ element? What will `xxx` match in that case?

**Answer**: Try it in the PURE interpreter before asking! (Yes, it will match, and `xxx` will become `[]`.)

**Question**: It is confusing that a list, notated as `[1,2]` in PURE, needs to be matched with `(x:xxx)` rather than with just `x:xxx` or with something more obvious-looking, like `[x,xxx]`.

**Answer**: This is because `[x,xxx]` matches only a list consisting of _exactly two_ elements, while `(x:xxx)` matches a list consisting of any number of elements (at least one), the first element being `x`. The parentheses around `(x:xxx)` are required because function application binds tighter than the infix operator `:`, so "`f x:xxx`" means the same as "`(f x):xxx`" - certainly not what we wanted! In  general it is a good habit to write parentheses around `(x:xxx)`.

Our second example is a function that takes a list and returns only the elements numbered 1, 3, 5, ...
```
take_odd [] = [];
take_odd [x] = [x];
take_odd (x:y:xxx) = x : take_odd xxx;
```
We follow the same method as before, building the function case by case and recursively calling the same function on a smaller list.

Note that `[x]` matches a list with a single element, and then the temporary pattern variable `x` gets the value of that element. Also, it is important to note that `(x:y:xxx)` matches a list with at least two elements -- it will not match `[1]`, say. Therefore, we need to define two cases by hand -- a list of zero and of one elements -- and we can use recursion for all other cases.

The recursive definition always reduces the computation to the same computation with the list `xxx`, which is two elements shorter. Therefore the recursion is guaranteed to terminate. Note that there is pretty much no room for off-by-one errors or other errors involving pointers, which are common when programming list functions in languages like C.

Testing `take_odd` in the PURE interpreter:
```
> take_odd [1,2,3,4];
[1,3]
> take_odd [1,2,3,4,5];
[1,3,5]
```

**Exercise**: Write a function that swaps elements of the list like this:
```
> swap2 [1,2,3,4];
[2,1,4,3]
> swap2 [1,2,3,4,5];
[2,1,4,3,5]
```
**Answer**:
```
swap2 [] = [];
swap2 [x] = [x];
swap2 (x:y:xxx) = y : x : swap2 xxx;
```

## Higher-order functions ##

The last two examples -- `take_odd` and `swap2` -- show two functions whose definitions are quite similar. One of the strengths of functional programming is that code duplication of this kind can be eliminated. One can take the common features in the definitions of these two functions and put them into a "function factory" that will _generate_ the functions `take_odd` or `swap2` as _return values_, given the right inputs. In other words, we can write a function that will define new functions <i>at run time</i>. A function that returns newly defined functions as its return values is called a **higher-order function**. Let us see how this works with the examples above.

The definitions of `take_odd` and `swap2` differ only in the way we process the leading two list elements, `x` and `y`. We would like to _abstract_ this processing; in other words, we would like to define a function, say `proc2`, that will be quite similar to `take_odd` or `swap2`, except for the final rule where we define the result of `proc2 (x:y:xxx)` in terms of `x`, `y`, and `(proc2 xxx)`. How can we make this last rule arbitrary? We can substitute an arbitrary _function_ of `x`, `y`, and `(proc2 xxx)`. Let us call this arbitrary function `last_rule`. Thus, we would like to have a definition of `proc2` that looks like this:
```
proc2 [] = [];
proc2 [x] = [x];
proc2 (x:y:xxx) = last_rule x y (proc2 xxx);
```
Now we can define `take_odd`, `swap2`, and many other similar functions by simply substituting different functions instead of `last_rule`. For example, `take_odd` is obtained if we define
```
last_rule1 x y zzz = x : zzz;
```
while `swap2` will be obtained if we define
```
last_rule2 x y zzz = y : x : zzz;
```

Now we come to a crucial step. What needs to be done in the last rule is  specified by a function that we called `last_rule`. Let us now write a function, `f_proc2`, that will take `last_rule` as an _argument_, and return the resulting `proc2` as the _return value_:
```
f_proc2 given_last_rule = f
  with f [] = [];
       f [x] = [x];
       f (x:y:xxx) = given_last_rule x y (f xxx);
  end;
```
Notes:
  * The definition of `f` is the same as that of `proc2` above.
  * The object `given_last_rule`, which is a parameter of `f_proc2`, is expected to be a function since it is applied to the arguments `x`, `y`, and `(f xxx)`. If we pass something other than a function to `f_proc2`, an unevaluated expression would result, which is not what we want. (Right now we will not try to prevent this error.)
  * The return value of `f_proc2` is the function called simply `f`; this is a _local_ function, i.e. a function whose name is visible only within `f_proc2`. Local functions are defined using the `with ... end` construction. (Local _variables_ are defined using the `when ... end` construction.) The body of `f_proc2` is thus of the form `f with ... end`, which is still considered a _single expression_ in PURE, because `f` is a single expression, even though `f` is defined separately using many lines of code. In general, a definition of a function in PURE can be
```
func x = y 
with ... end // local functions
when y = ... end; // local variables
```
The body of this function is still a _single_ expression, `y`, that uses some auxiliary local functions or variables.
  * The definition of `f` uses `given_last_rule`, which is an argument of `f_proc2` but not an argument of `f`. This is allowed because `f` is defined _within_ the body of `f_proc2`, i.e. within an environment where `given_last_rule` already has a value.

**Question**: Why is `f_proc2` allowed to return its _local_ function `f`? Isn't the local function "confined" to the body of `f_proc2`?

**Answer**: The _name_ of `f` is confined; the _value_ of `f` is a function that we can return _by value_. This is no different then returning the value of a local variable whose value is, say, the number 12. The variable's _name_ is confined to the body of the function, but the _value_ of that variable (the number 12) cannot be "confined." So it is quite all right that we return the value of a locally defined object. You may also imagine that a _copy_ of the function `f` is returned. (In the machine code compiled from PURE, there is of course no need to copy the entire data structure representing the function `f`. Since functions cannot be changed, it suffices to return a _pointer_ to that data structure.)

Now the code of `f_proc2` should have become clear. Let us test the code with the PURE interpreter. Since the argument of `f_proc2` must be a function, it is convenient to use a lambda-construction, rather than having to invent a new name for every temporarily needed function:
```
> f1 = f_proc2 (\x y zzz -> y:x:zzz);
> f1 [1,2,3,4,5];
[2,1,4,3,5]
> f2 = f_proc2 (\x y zzz -> x:zzz);
> f2 [1,2,3,4,5];
[1,3,5]
```
We have successfully used the higher-order function `f_proc2` and obtained (as its "return values") the functions `f1` and `f2` that are fully equivalent to `swap2` and `take_odd`. We can then use `f1` and `f2` as we would use ordinary functions.

We can also use `f_proc2` to make other list-processing functions:
```
> f3 = f_proc2 (\x y zzz -> x+y:zzz); f3 [1,2,3,4,5];
[3,7,5]
> f4 = f_proc2 (\x y zzz -> (x*y):zzz); f4 [1,2,3,4,5,6];
[2,12,30]
```
This example shows how higher-order functions can help us eliminate code duplication. We have abstracted the common features of all "pairwise" list processing functions into a single "function factory" `f_proc2`. Now we can generate many such functions in a concise way. This trick significantly shortens programs written in the functional paradigm.

## "Currying" ##

Let us think for a moment about the function call `f2 [1,2,3,4,5]` shown above. Since `f2` is the same as `f_proc2 (\x y zzz -> x:zzz)`, why don't we just tack on the last argument and write
```
> h = \x y zzz -> x:zzz ;
> f_proc2 h [1,2,3,4,5];
[1,3,5]
```
Indeed, it works! This trick can be understood as follows: `f_proc2 h` returns a function that operates on a list. Therefore, `f_proc2 h lst` is the application of `(f_proc2 h)` to the list `lst`.

This trick is based on the fact that function application is a kind of _left-associative_ operation: that is,
```
f_proc2 h lst
```
is the same as
```
(f_proc2 h) lst
```
but not the same as `f_proc2 (h lst)`! Indeed, most functional languages (and PURE) adopt the left associative property of function applications in this sense.

**Question**: Something seems wrong here... We defined `f_proc2` as a function taking _one_ argument. So I understand that `f_proc2 h` is a valid function call. But now, all of a sudden, we call `f_proc2` with _two_ arguments, `h` and `lst`! We call a function with more arguments than shown in the function's definition. Why is this allowed _at all_?

**Answer**: This is not allowed in languages like C. However, in functional languages (such as ML, Haskell, PURE) a function can define another function and return it as a resulting "value." This resulting function can be applied to more arguments and again return a function, which could again be applied to further arguments. So a definition `func x y z = ...` does not mean that `func` always must take three arguments. If `func x y z` returns, say, a new function taking two arguments, we can give _five_ arguments to `func` and obtain meaningful results.

We can also give _fewer_ arguments than shown in the definition. Consider the function
```
func1 x y = x*2 + 1 - y;
```
The expression `func1` -- without any arguments -- is already a valid expression, meaning the function itself (the "function object"). The expression `func1 x y` is also valid according to the definition just given (it is evaluated to `x*2+1-y`). By the left associative property of function application syntax, the expression `func1 x y` must be the same as `(func1 x) y`. Now, the expression `(func1 x) y` means that we apply the object `(func1 x)` to the object `y`, -- as if `(func1 x)` were a function. Therefore, if we want to remain consistent here, the expression `func1 x` _must_ be a function! Indeed, it is a function that can accept one argument, say `y`, and will then return `x*2+1-y`. For example, `func1 5` evaluated on `y` will return `11-y`. So `func1 x` is a weird kind of function: it is `func1` with the first argument "frozen in," and it returns a function that "is waiting for the second argument."

This kind of operation on functions, when we supply some arguments but leave out others, is called **currying**. Thus, `func1 5` is a curried version of `func1` with the first argument set to 5.

It is important to realize that the value of `func1 5` is itself still a function. So `func1` can be seen as a function with _one_ argument, returning a function. Moreover, _every_ function can be seen as a function of just one argument, returning perhaps another function. Conversely, every function application such as `func a b c` can be seen as an application of a function, `func a b`, to just a _single_ argument, `c`. This way of looking at functions is an important conceptual simplification.

Currying is one of the reasons for using the parenthesis-free syntax for function application. With currying, it is easy to create functions such as _f(x,y)_ for fixed _x_, which may be useful for numerical calculations as well as in other contexts. We can also consider _every_ function as if it accepts just one argument (but may return a function). In this way, currying simplifies the concept of a function in functional languages.

Currying is also useful in conjunction with the arithmetic operators. For example, the addition operator, `+`, can be used in the curried form simply as `(x+)`:
```
> q = (2+);
> q 3;
5
```
The notation `(2+)` can be visualized as a function that takes _one_ argument and inserts it into the vacant place after the `+` operator. We could write this function more verbosely in the lambda notation as `\x->2+x`.

**Question**: If a function such as `func1` is defined with two arguments, it can be used with two arguments or fewer, thanks to currying. Can `func1` _always_ be used with more arguments, say with _three_ arguments?

**Answer**: Yes if the value of `func1` is a function. Otherwise, you may get a useless unevaluated expression. For example, if `func1 64` returns the number 2, you can evaluate `func1 64 y` and get the expression `2 y`, that is, the symbol `2` applied to the symbol `y`. Chances are that the expression `2 y` will never be evaluated to anything else because the symbol `2` is not a function and, most probably, will not be defined as a function in your program. Nevertheless, `2 y` is a valid PURE expression.

Actually, to help you avoid mistakes, the PURE interpreter will warn you with an error message if you try to _define a rule_ for a function, say, with three arguments if previously a rule was given with two arguments. But there will be no warning if you try to _call_ a function with the "wrong" number of arguments. PURE assumes that you know what you are doing! For example, the following could very well be a valid definition and usage, if `x` is itself a function:
```
> h x = x x;
> h x y;
x x y
```
Generally, there is no "correct" number of arguments that we must give to a particular function in order to obtain a valid expression. One can always give fewer arguments than shown in the definition of a function; and sometimes one can give more arguments and still obtain meaningful results.

Usually, each function will require a certain number and type of arguments in order to compute some final result. For example, the function "`+`" requires two numeric arguments in order to produce a result. When we give exactly as many arguments as required, like in the expression "`a+2`", the function call is **saturated**. If fewer arguments are given, the function call is **curried**. For example, if we define `func x y = x+y` then the function call `func 2` is curried while `func a b` is saturated.

There is a built-in function, `nargs`, that determines whether a given function call is saturated. Here is what it returns on the examples we just discussed:
```
> nargs (a+2);
0
> nargs (2 a);
-1
> func 0 y = x+y;
> nargs func, nargs (func x), nargs (func x y), nargs (func x y z);
2,1,0,-1
```
Note that `func x y` returns `x+y` in the example above; the `nargs` is applied to the result, `x+y`, and yields 2. When applied to something that is not an application of a declared function, `nargs` yields -1.

**Question:** With currying, I can freeze the first argument of a function. Can I also freeze the _second_ argument, without freezing the first? The "currying" syntax does not seem to do this.

**Answer:** It is possible, of course, to freeze an argument by declaring another function explicitly:
```
func1 a b c = a+b+c;
func2 x y = func1 x 7 y;
```
This kind of transformation can be also encapsulated into a "function factory", for example:
```
freeze_arg2_f2 f c2 = \x -> f x c2;
freeze_arg2_f3 f c2 = \x y -> f x c2 y;
// similarly for more arguments 
```
Finally, there is a standard function `flip`, defined in the PURE prelude, which exchanges the first two arguments of a function. This function allows one to curry the second argument like this,
```
> // flip f = \x y -> f y x; // already defined in the prelude
> f2 x y = (x+1)*y;
> f3 = (flip f2) 3; // f3 x = (x+1)*3
> f3 2;
9
```


### Example: inverting a function numerically ###

Let us consider a semi-realistic example of a numerical calculation where we can naturally use curried functions.

Recall the "equation solver" displayed in the first tutorial. This is a function with the call signature `solve f x1 x2 eps`. The first argument is a function `f` expecting a single numerical argument. Suppose we need to define the function _y(x)_ by solving for _y_ the equation _f(x,y)_=0. We can use the simple "equation solver" and define a function, say `invert2`, that takes _f(x,y)_ as the argument and yields _y(x)_ as the return value. (The function `invert2` will be then a "higher-order function.")

For simplicity, let us assume that for any relevant values of _x_ the equation _f(x,y)_=0 always has a single root _y(x)_ bracketed between some known values `x1` and `x2`. (It is in general far from easy to guess or to predict the bracketing interval, but we will assume for our example that the bracketing interval is known in advance.) Then, for any _x_, the value of _y_ that solves _f(x,y)_=0 will be found numerically to 10 digits as
```
solve (f x) x1 x2 1.0e-10;
```
Note here that `(f x)` is a curried form of `f` with the frozen `x` argument. This curried function is being used as a function of a single argument, `y`, to yield the same value as `f x y`. The simple equation solver, `solve`, never needs to know that it is being applied to a function `f` with two arguments; it can only see the curried function, `(f x)`, which is being given to it.

Our wish is to define a function that will take _any_ _x_ and produce the corresponding _y(x)_. We translate this wish into the following PURE code:
```
invert2 f x1 x2 eps = \x -> solve (f x) x1 x2 eps;
```
That's it! This is how functional programming helps us solve real-world problems. No need to define complicated data structures with pointers, no need for memory allocation, and no room for hard-to-find mistakes.

Let us test this function on an example, which is chosen such that our simplistic equation solver is numerically adequate. We will invert a particular function _f_ (defined below) within the interval of _x_ from 0 to 3.
```
> using math;
> f x y = x*0.1 + y*0.5 - sin y;
> inv_f = invert2 f 1 2 1.0e-10;
> map inv_f [0.0,0.5,1.0,1.5,2.0,2.5,3.0];
[1.89549426703398,1.83210528518513,1.76312396157946,
1.68660364663262,1.59919364642736,
1.49412427587496,1.35256288220115]
```
Remarks:
  * The parameters of the inversion function (notably, the bracket interval from 1 to 2) are chosen by hand! In general, one needs to have studied the function _f(x,y)_ to be able to choose the bracket interval.
  * The equation solver `solve` is not robust and will fail on some inputs; for example, `inv_f 2.4` goes into an infinite loop, because the code does not ensure that the next approximation remains within the given bracket. This code is intentionally simplified and is used only for illustration of the programming techniques!
  * The `map` function comes from the standard PURE prelude; `map f lst` applies the function `f` to each element of the list `lst` and produces a list of results.

## Tail recursion ##

**Question**: It seems that recursion is inefficient. Take, for example, the function `length_of_list` shown before: it will call itself recursively as many times as there are elements in the list. If we apply `length_of_list` to a list of 10,000 elements, we will get 10,000 _nested_ function calls. One function has not yet finished and is calling the second, which is calling the third, etc. A lot of memory will be wasted to store the arguments of each function. A loop would be faster (no function calls necessary) and will not waste memory.

**Answer**: Yes, the implementation of `length_of_list` as shown above is very inefficient because it uses a lot of memory to store the intermediate arguments of all the nested functions. (This memory is called **stack space** because the arguments for nested function calls are typically stored in a _stack_ data structure, "on top of each other.") The implementation of `length_of_list` can be improved using a technique called **tail recursion**.

The idea of tail recursion is to write code for our function in a special way: The code should be everywhere _not_ recursive (i.e. not calling itself) except for places where we _directly return_ the value obtained from a recursive call. This kind of code can be visualized as follows:
```
func x y = 
    if ... then ... // whatever, but we must not have
    else if ...     // any calls of `func` here.
    then func x1 y1 // recursive call, returning directly
                    // the value of `func x1 y1` as the result.
    else if ... then ... // no calls of `func` here.
    else func x2 y2 // recursive call, again returning directly.
      when x1 = ...; x2 = ...;  // no calls of `func` here; we may
           y1 = ...; y2 = ...;  // call other functions if they do
      end;                      // not call `func`.
```
This organization of function calls is what one means by **tail-recursive** code. Since in this case the recursive call is always the _last_ operation before the function returns, all the local environment of the function (arguments `x`, `y`, and any other local variables) will not be used any more. So the compiler can optimize the recursive calls by _re-using_ the memory occupied by the arguments `x,y` and by any local variables. The compiler does not have to allocate any new stack space either for the local variables or for the arguments of the recursive calls, such as `x1,y1` or `x2,y2` shown above. Thus, no matter how many nested recursive calls we need, the compiler can use constant stack space and optimize function calls to simple `goto`s in the assembly code. When this optimization is in effect, tail-recursive code is executed as quickly as an ordinary loop and does not use any extra stack space.

The PURE compiler is based on the LLVM engine, which supports tail recursion. Therefore, tail-recursive PURE code is executed quickly and in constant stack space.

Now, the code for the function `length_of_list`, as shown above, is _not_ tail-recursive because the expression involving tail recursion is `1+length_of_list xxx`, which is the call to the function `+`, not a recursive call to the function `length_of_list`. Evaluating `length_of_list [1,2,3]` creates an expression `1+length_of_list [2,3]`, which in turn creates `1+(1+length_of_list [3])`, which in turn creates `1+(1+(1+0)))`, and only at this last point the expression can be evaluated. It is clear that a lot of temporary space is being wasted. The code would have been tail-recursive if the expression involving recursion was `length_of_list ...`, i.e. a call to the function itself (perhaps with different arguments), so that the returned value is directly equal to the value of that call.

Let us see how we can replace non-tail-recursive code, like that of `length_of_list`, by tail-recursive code. The main technique for doing this involves the so-called **accumulator arguments**.

Let us name the new function provisionally by `length`. We can come up with the accumulator technique if we think logically about what the tail-recursive code of `length` needs to accomplish. We need code that _must_ look like this,
```
length [] = 0;
length (x:xxx) = some_func xxx ???; // not clear what some_func is,
                                  // but it must be tail-recursive!
```
Now let's think about implementing `some_func`. When we apply `some_func` to `xxx`, we must return 1+length of `xxx`. This value _must_ be the result of `some_func` applied to some new arguments, otherwise we will not have tail recursion. So we can't add 1 to the length of `xxx` directly in the body of `some_func`. But we can pass the "1" to `some_func` as an _additional_ argument, so that the addition of 1 and length of `xxx` will be done within that call! In this way we avoid having to add 1 to the length of `xxx` ourselves, which was the main reason for the lack of tail recursion.

Thus we get the idea of defining `some_func` with _two_ arguments; we would like to express `length` through `some_func` as
```
length (x:xxx) = some_func 1 xxx;
```
This will work if `some_func n lst` returns `n` + length of `lst`. The implementation of this idea is straightforward:
```
some_func n [] = n;
some_func n (x:xxx) = some_func (n+1) xxx; // this is tail-recursive!
```
It is clear that `some_func 0 lst` evaluates the length of the list `lst` in purely tail-recursive way. We are done!

The first argument of `some_func` is called the **accumulator** argument because it is updated as we proceed with the computation. It is analogous to an accumulator variable that we might have used in a C loop such as
```
int length(list* lst) {
  for (n=0; list_not_empty(lst); ) {
    lst = next_node(lst); // imitate tail(lst)
    n++;
  }
return n;
```
Note that the C loop shown above is prone to off-by-one errors. Should we initialize `n=1` or `n=0`? In which order should we write the statements inside the loop? Is the termination condition correct? One has to spend some time verifying that this C code works as intended. To guard against off-by-one errors, one can introduce an "invariant", that is, a condition that holds after each iteration of the loop, and verify that it then holds after the next iteration and at the end of the loop.

It is much easier to ensure the correctness of the tail-recursive code for `length`. Let us show the complete, somewhat streamlined PURE code for comparison with the C code above:
```
length lst = length_acc 0 lst
  with length_acc n [] = n; 
       length_acc n (x:xxx) = length_acc (n+1) xxx;
  end;
```
It is easy to see using induction that this code is correct. (The function call `length_acc n list` returns `n+`length of `list`.)


We will see that the "accumulator technique" is quite general and applies to all cases where one uses loops in C.

### Example: splitting a list ###

We now consider the following task. Given a non-empty list of numbers, `lst`, of length _n_, we need to split it into two contiguous sub-lists (say from 1 to _m_ and from _m_+1 to _n_) such that the sum of the first part is equal to the sum of the second part. If such a splitting exists, we need to return the first sub-list. Otherwise we need to return `false`.

Let us implement this task as a general function `split_list` that takes as an argument the condition function `cond`. Thus, the first function will be occupied only with the list splitting, while the second function will know what kind of sub-list we are looking for. Let us start with the second function:

```
// the condition that two lists have equal sum
parts_equal list1 list2 = (sum list1) == (sum list2);
// sum of the list entries: use tail recursion with accumulator
sum lst = sum_rec 0 lst
 with
  sum_rec s [] = s;
  sum_rec s (x : xxx) = sum_rec (s+x) xxx;
 end;
```

The function `split_list` will accept `cond` as an argument. We need to try the sub-list from 1 to _m_ and the sub-list from _m_+1 to _n_, for each _m_. In FORTRAN or C, we would have written a loop over _m_. Since PURE has no loop statement, we need to write a recursive function. How can we design it?

In order to replace a loop by a recursive function, we need to imagine a recursive call of this function at some intermediate point during computation. The function may use the results of previous iterations; the goal is to return the _final_ result, and the function is allowed to call itself tail-recursively. Usually, the function needs to check whether the iterations are finished; if so, the result can be returned immediately. Otherwise, the function needs to compute one more iteration and to pass the results to the next tail-recursive call. The entire computation will be done by calling this recursive function on some initial value.

This method of computation is quite similar to mathematical induction. One starts with the basis of induction (usually, some value where the computation is trivial, e.g. an empty list) and writes code for this value; it is usually easy to check that the answer is correct. Then, one implements the induction step by writing code that computes the next case assuming that the previous case was computed correctly (by the recursive call).

In our example with splitting the list, the recursive function will assume that we have already tried all the initial sub-lists of length less than _m_, and now we need to try the sub-lists of length _m_ or longer and return the result if found. Using the accumulator technique, we find that the function should have the _two_ previously tried parts of the list as arguments. Once we realize this, the implementation is straightforward. The following code will also illustrate some convenient PURE idioms:

```
split_list cond (x:xxx) = split_list_rec cond [x] xxx
 with
  split_list_rec cond list1 [] = false; // already tried all sub-lists
  split_list_rec cond list1 list2@(x:xxx)
     = list1 if cond list1 list2; // return the initial list on success
     = split_list_rec cond (cat [list1, [x]]) xxx   otherwise; // continue
 end;
```
Note the `list2@` pattern. This pattern introduces at once _three_ temporary pattern variables: the variable `list2` is equal to the _entire_ second argument, and the variables `x` and `xxx` refer to the parts of the list as they do usually, when we use the pattern `(x:xxx)`.

The standard library function `cat` concatenates lists given in its argument (which must be a list of lists).

Testing this code in the PURE interpreter, we get:
```
> split_list parts_equal [1,2,3,6];
[1,2,3]
> split_list parts_equal [1,2,6]; // no splitting exists, should return `false`
0
```

## Replacing loops with recursion ##

**Question**: In the example with `length`, it took some time to discover the "accumulator" trick. Suppose I have an algorithm written in FORTRAN or C, with many loops and local variables that are updated in loops. What other tricks do I need in order to implement such an algorithm using recursion, and without the possibility to change the values of local variables? (In PURE, one cannot write `y=y+2` or construct a loop.)

**Answer**: You only need two or three tricks besides the accumulator trick, and that's it! Any code with loops can be more or less mechanically replaced by tail-recursive code in the purely functional paradigm. Let us look at some more examples.

A general kind of loop may perform some or all of the following operations:
  * go over all elements of a list, computing a list of new values;
  * update some global variables as each element of the list is visited;
  * use a condition to decide whether an element of the list is to be included in the computation or omitted;
  * stop the computation when a certain condition is reached (even if not all elements of the list have been visited so far).

All these operations are straightforwardly translated into purely functional code with tail recursion. The standard prelude of PURE includes higher-order functions that implement these operations. It is important, however, to try implementing them yourself before you start using the standard functions. Implementations are so simple and yet so conceptually important that this exercise is indispensable.

**Exercise**: Write tail-recursive functions that do the following.

1) Go over all elements of a list, apply a function `f` to them, return the list of resulting values. This is called `map` in the standard prelude.

Hint: One of the arguments of the tail-recursive function should be the list of values obtained so far.

A possible solution, called `map1` to avoid conflict with the standard `map`:
```
map1 f list = map_t [] list // delegate to tail recursive function
 with
  map_t lst [] = lst;
  map_t lst (x:xxx) = map_t (cat [lst, [f x]]) xxx;
 end;
```
Testing (the function `f` is undefined!):
```
> map1 f (1..3);
[f 1,f 2,f 3]
```

2) Go over all elements of a list of numbers, add only those that satisfy a predicate `p`, and return the resulting sum.

Hint: One of the argument of the tail-recursive function should be the sum obtained so far.

3) Go over all elements `x` of a list of numbers; ignore those `x` that do not satisfy a predicate `p1 x`; apply a function `f` to each remaining element, keep adding the resulting values of `f x`; stop when a condition `p2 s i` first returns true, where `s` is the sum calculated so far and `i` is the total number of elements not ignored up to now. Return a tuple `(s,i)` with values of `s` and `i` last obtained.

Hint: The arguments of the tail-recursive function should include the values of `s` and `i` obtained so far.

Now we will consider cases when the logic of the code appears to be more complicated. In the following tables, some sample C code involving local variables and loops is shown in the left column and the corresponding PURE code written in the functional style -- involving only tail recursion -- is in the right column. The functions `f1`, `f2`, `cond`, etc. are assumed to be some functions or expressions.

The first example is a fragment of C code that has an assignment statement within a 'while' loop.

<table border='0' title='First example'><tr><td>
<pre><code>int w1(x) { while (cond(x))<br>
  x = f1(x);<br>
  return x; }<br>
</code></pre>
</td><td>
<pre><code>w1 x = if cond x<br>
  then w1 (f1 x)<br>
  else x; <br>
</code></pre>
</td></tr></table>

We could define a higher-order function that replaces this `while` loop and _returns_ the function, such as that denoted above by `w1`. This higher-order function could be understood as a "while-loop factory."
```
make_while_loop cond f = w  // return a function!
 with
  w x = if (cond x)
  then w (f x)
  else x
 end;
```
For example, here is a function that takes an initial number and multiplies it by 3 repeatedly until the result becomes larger than 1000. To achieve this, we merely need to pass the curried predicate `(< 1000)` and the curried multiplication `(* 3)` to the "while-loop factory" we just defined:
```
> f = make_while_loop (< 1000) (* 3);
> f 3;
2187
```

The next example is more involved: it shows what we need to do if the C code has _several_ assignment statements. The trick is to think of all possible "updated versions" of each variable; we name the versions `a`, `a1`, `a2`, `x`, `x1`. Also, the `while` block becomes a separate auxiliary function `w2_1` because it follows an assignment statement, so the initial value of `a` in the `while` loop needs to be supplied to `w2_1` by hand.

<table border='0' title='Second example'><tr><td>
<pre><code>w2(x) {<br>
  int a=f1(x);<br>
  while (cond(x,a)) <br>
  {<br>
    a=f2(x,a); x=f3(x,a); <br>
  }<br>
  return a; }<br>
</code></pre>
</td><td>
<pre><code>w2 x = w2_1 a1 x<br>
 when a1=f1(x) end;<br>
w2_1 a1 x = <br>
   if cond(x,a1)<br>
   then w2_1 a2 x1 // new iteration with updated a,x<br>
   else a1<br>
     when a2=f2(x,a1); x1=f3(x,a2) end;<br>
</code></pre>
</td></tr></table>

For clarity, we did not define `w2_1` as local to `w2`, although we could have.

The final example presents more complicated C code with a `for` loop, a `break`, and a `return` from the middle of the function body.

<table border='0' title='Second example'><tr><td>
<pre><code>w3(x) {<br>
  for (<br>
    int a=f1(x);<br>
    cond1(x,a);<br>
    a=f2(x,a), x=f3(x,a)<br>
  )<br>
  {<br>
    if (cond2(x,a))<br>
      return x;<br>
    else {<br>
       a=f4(x,a); x=f5(x,a);<br>
    } <br>
    if (cond3(x,a))<br>
      break;<br>
  }<br>
  return a; } /* what is returned? */<br>
</code></pre>
</td><td>
<pre><code>w3 x = w3_1 (f1 x) x; // a=f1(x)<br>
w3_1 a x = // 'for' iteration receives the 'current' values<br>
   // of 'a', 'x', i.e. values after the previous iteration.<br>
  if cond1(x,a) <br>
  then // need to compute the result!<br>
    if cond2(x,a)<br>
    then x; // return x<br>
    else // a1, x1 are computed now.<br>
      if cond3(x1,a1) // to implement 'break', we _need_<br>
      then a1; // to repeat 'return a' here as 'a1'.<br>
      else w3_1 a2 x2 // a2, x2 are computed now.<br>
  else a // 'for' loop is finished normally, return 'a'.<br>
// all the assignment statements can go here!<br>
  when <br>
    a1=f4(x,a); x1=f5(x,a1); // from loop body<br>
    a2=f2(x1,a1); x2=f3(x1,a2); // from loop header<br>
  end;<br>
</code></pre>
</td></tr></table>


Let us become acquainted with some terminology adopted in computer science when discussing functional programs. In the **imperative style** of programming, the program is a sequence of **statements**, i.e. commands, and the statements can change the values of variables (`x=x+2`) or directly create and destroy objects in memory. Statements like `x=x+2` are called **destructive updates** (i.e. they "destroy" the previous value of `x` in memory). Silently updating a value in memory while evaluating something else, like in the C expression `y = x++`, is called a **side effect**. A subroutine or a function may have no return value and no arguments but instead have side effects, so e.g. a function call such as `do_something();` might silently change some values in memory (I say "silently" because it is not obvious what is being changed by looking at the function call itself). Similarly, at first sight the statement `a=my_func(x)` should only assign the return value of `my_func` to the variable "`a`". However, in imperative languages this statement could silently change "`x`" (as well as other values in memory) as a side effect within the function `my_func`.

In the **functional style**, local variables can be defined but not changed -- they are **immutable**. Thus, the **pure** functional style means that the program has no destructive updates, no side effects, no loops, and no `return` statements, -- only expressions and functions. The return value of a function depends only on the values of the arguments of the function; the result of calling a function is only that the value has been computed, and one can be sure that no other changes have occurred elsewhere in memory. The result of the program is the value of the single "main expression" that needs to be evaluated.

**Question**: The functional-style code for the last example is long. Is it because we cannot update variables? All our previous examples of PURE code were pleasantly short.

**Answer**: The conciseness of the C code is misleading! It is not easy to understand the logic of that code, with all the possible branches and breaks. On the other hand, each line of the PURE program requires more or less the same mental effort to understand because there is no loops in the flow of control, and because all variables are constant within each function call. In other words, we have replaced _each iteration_ of a loop by a _new_ function call -- a call to a function that evaluates a _single_ expression. Within a single expression, there is no room for looping, updated variables, or "return" statements. The C code updated some variables at the previous iteration, but our function call in PURE already received these updated values, so they _appear to be immutable_ during this iteration. The newly computed values are passed in the temporary arguments of the next recursive call. For this reason, we can actually use _immutable_ local variables to implement algorithms that in C would involve loops with updated local variables.

The examples shown above illustrate how loops are implemented through recursively defined functions in the pure functional paradigm. Each function call computes only a _single iteration_ of the loop. Values computed during a previous iteration are given as arguments to the function. _All_ the future iterations are delegated to the single recursive call of the same function. Thus the logic of a functional program is much cleaner and easier to understand than the logic of an imperative-style program. To verify the correctness of a functional-style program, it is often sufficient to use mathematical induction: If the basis is correct and the step is correct, a tail-recursive function will always return correct results. Verifying the correctness of imperative-style code, like the C code above, is much more difficult. The increased length of the functional-style code in the last example (the function `w3`) indicates quite clearly that the logic of the code is somewhat more complicated.

In the functional-style code for `w3_1`, there is one place where we appear to pay a price for having immutable variables. Namely, the C code contained a single statement "`return a`", but the PURE code has _two_ places corresponding to this statement, as indicated by comments in the code: one returning "`a`" and another returning "`a1`". This is so because in the C code, returning "`a`" after a "`break`" actually returns a _differently computed_ value of "`a`" than returning "`a`" after a normal termination of the "`for`" loop. Thus, there are actually two different code paths returning two differently computed values, -- values that _happen_ to be labeled by the same symbol "`a`" in the C code. The C code hides this complexity by writing only a single `return` statement and using a `break` statement in the loop. This clearly shows that a `break` statement (really a "`goto`" in disguise!) hides a complicated program logic that may become hard to follow. The only advantage of this code in C is that the memory for the variable "`a`" is being reused to store the value labeled by "`a1`" in the PURE code. However, this advantage is illusory; a sufficiently smart optimizing compiler for a functional language will be able to detect and use this possibility of optimization automatically, without the  need to make the logic of the code harder to understand. The functional programming paradigm, in some sense, forces the programmer to make the logic of the code more transparent.

After these examples, it should be clear that there is a general method of translating C-style code with loops into the functional style. Informally, the method is this:
  1. Any _constant_ local variables are translated into local constants.
  1. Every _non-constant_ local variable (i.e. a local variable `x` that is being updated, whether in a loop or out of loop) gets a "version number" on each update. For example, `x` is replaced by `x1`, `x2`, etc., so every update is converted into the computation of the "new version" of the variable. Thus, we replace the sequence of statements `x=f(x,a); a=g(x,a);` by `x1=f(x0,a0); a1=g(x1,a0);` etc. Each "versioned variable" becomes a local _constant_ in the body of the recursive function representing this loop or code block.
  1. For _every_ loop-like construction -- any of `for`, `do`, `while`, or a conditional `goto` if it creates a loop -- we introduce a tail-recursive call to a _new_ auxiliary function. (One auxiliary function per loop, including nested loops!)
  1. The auxiliary function represents the computation during a _single_ iteration of a loop. The job of the auxiliary function is to compute the resulting return value of the entire loop, starting from some iteration of the loop (not necessarily the first iteration!). The auxiliary function, therefore, needs to receive _as its arguments_ all the relevant values computed at the previous iteration. This function computes the return value by calling itself tail-recursively with some new arguments.
  1. The arguments of that tail-recursive call are the values of the "last versions" of all the non-constant local variables that are being updated in that loop, i.e. the values these variables would have _at the beginning of a new iteration_ of the loop body.
  1. If one loop follows another loop in the C code, the auxiliary function for the first loop needs to compute the values of all mutable variables needed to start the second loop, and call the second auxiliary function with those values.

It is certainly possible to formalize this method into an automatic procedure for converting, say, a C program into functional-style code with tail recursion. However, we will not attempt to implement this procedure here. Our goal is not to translate C code into PURE but to learn how one formulates algorithms directly in the functional style, _without_ writing any C code first.

**Question**: If functional-style recursion is _equivalent_ to familiar C-style programming with loops, why should I use recursion?

**Answer**: In many cases, recursion is clearer and easier to write. In practice, most algorithms are formulated naturally in mathematical language, which is better suited for a recursive, functional-style implementation than for an implementation with loops. Loops (and imperative-style programming in general) are much more error-prone. Usual errors are a missing or incorrect initialization of variables; off-by-one errors (starting a loop at 1 instead of 0, ending a loop at `n` instead of `n-1` or `n+1`); incorrect order of silent or "hidden" updates (e.g. when passing an expression with a destructive side effect, like "`x++`" in C, to a function that also destructively updates its argument); branching (`break`, `goto`, `return` from the middle of a function) that arrives at a another place in the code where some variables are expected to have different values. Many programming books explicitly warn against these pitfalls of coding, calling them "bad style." It appears that it is precisely these "bad style" programming idioms that require a total rewrite and redesign of the code when we switch to the functional programming style. In short, experience with a functional language _forces_ you into a cleaner style of programming and makes you a better programmer in any language.

# Symbolic programming #

Now we come to the "symbolic" part of the tutorial.

## What is "symbolic computation" ##

The purpose of a **symbolic computation** is to deduce or verify some structural relationships between symbols. (In contrast, **numeric** computations are focused on computing some numbers according to given mathematical equations.)

Here is an example of a programming task in symbolic computation. Although this task is quite simple, it illustrates several important concepts and serves as a good introduction.

## Example: toy theorem prover ##

The task is to implement a "theorem prover" in a simple formal grammar. (The example is the "ABIN system" from the book: W. Robinson, Computers, minds and robots, Temple University Press, 1992. Warning: the book is about _philosophy_.)

The "ABIN system" is defined as follows. We consider text strings made of symbols `A`, `B`, `I`, `N`; these are "expressions". An expression is "well-formed" if it consists of either

  * `B` followed by zero or more of `I`'s, for example `B`, `BIII`; or
  * `N` followed by a well-formed expression; or
  * `A` followed by _two_ well-formed expressions.

This defines a certain set of well-formed expressions; for example, `NBII` and `ABB` are well-formed but `AB` and `NA` are not.

Now, all well-formed expressions starting with `NB` are interpreted as "true" (they are the "axioms" of the system). For example, `NB` and `NBIII` are "true." In addition, there is a single "deduction rule" allowing us to prove "theorems":

  * If `x` and `y` are well-formed and `Nx` is "true" then `NAxy` is also "true".

Using this rule with `x=BI` and `y=B`, we can prove `NABIB` (since `Nx` equals `NBI`, which is an axiom). However, `NANBIB` cannot be proved. Thus, we say that `NABIB` is a theorem but `NANBIB` is not a theorem, in the sense of the ABIN system.

(The idea behind this interpretation is to assume that `N` means the logical "not" and `A` the logical "and". Then `Nx` is "true" when `x` is false, and then `Axy` is false and therefore `NAxy` is true. The "axioms" mean that all `B`, `BI` etc. are some _false_ statements. For example, the statement `NABIB` would mean "it is false that both `B` and `BI` are true", so `NABIB` is true in this interpretation. However, the deductive system of ABIN is too weak to obtain the proof of every statement that is true in this interpretation. We will nevertheless consider a statement as "true" if and only if it can be proved from axioms within the ABIN system.)

The task is to write a program that can test whether a given expression (for example, `NAABABBBI`) is well-formed, and also decide whether this expression is "true" in the ABIN system. This task is a typical example of a "symbolic computation." The purpose of this "computation" is not to compute any numbers but to determine certain structural relationships in the given symbolic input, such as `NAABABBBI`. If we implement this program correctly, it should say that `NAABABBBI` is a "theorem."

## Implementation in PURE ##

It is certainly possible to implement the ABIN theorem prover in any language, including C or FORTRAN. However, the problem is inherently symbolic, so it would be good if we don't have to spend time implementing the low-level details of representing ABIN expressions (pointers, lists, strings, etc.). Also, note that the problem is inherently _recursive_ because many aspects of the ABIN system are defined recursively. For example, `Nx` is well-formed when `x` is well-formed; an expression is a theorem when some other expression is already a theorem, etc. It is awkward to implement the theorem prover using loops, but easy using recursion.

A good approach to this problem is to define **predicates**, that is, functions returning `true` or `false` according to some condition. What we want in the end is to define two predicates, `is_expr` and `is_true`, such that we can then get the following "computations":
```
> "AB" is_expr;
0
> "NABIBI" is_true;
1
```
We will now develop the solution in PURE, step by step. At the same time, we will illustrate some of the important capabilities of PURE.

### First steps ###

**Question**: Functions in PURE are usually written as `func x y`, i.e. the function name is written first. It is not clear why we can have an expression shown above,
```
"NABIBI" is_true;
```
where the function symbol comes last.

**Answer**: The syntax of PURE is very flexible. We will define `is_true` as a **postfix operator**, which is just a function whose symbol is written _after_ the arguments (_post_ means "after" in Latin). Here is the code that defines both predicates as postfix:
```
postfix 1650 is_true is_expr;
```
(We chose 1650 as the precedence level because it is above the logical operators but below arithmetic; we need _some_ precedence level, and for us it does not matter which one we use, as long as it is not too large.) After that we can just write these functions in postfix notation.

Now let's get to work. It is convenient to enter ABIN expressions as text strings such as `"NABIBI"`. However, the coding is easier if each ABIN expression is represented as a list such as `["N", "A", "B", "I", "B", "I"]`. Converting from strings to lists is easy: we use the standard function `list`:
```
> list "NABIBI";
["N", "A", "B", "I", "B", "I"]
```
So we write the following definitions:
```
x::string is_true = (list x) is_true;
x::string is_expr = (list x) is_expr;
```
Now all further work will be performed with _lists_ rather than with strings.

**Question**: What is `x::string`?

**Answer**: This means that the given definitions apply only in case `x` is of type `string`. The suffix `::string` is called a **type tag**.

**Question**: We have not really defined these functions. What happens if we evaluate `"AAB" is_true` in the interpreter?

**Answer**: So far we defined `is_true` on strings. The interpreter will return the unevaluated, i.e. _symbolic_, expression that results from using what we have defined:
```
> "ABB" is_true;
["A","B","B"] is_true;
```
The interpreter has no problem with undefined or partially defined functions; it computes what it can, and leaves all the other parts unevaluated. The resulting expression will be evaluated further when we supply a definition for `is_true` acting on lists.

The next step is to think logically about the nature of the predicates. An expression is "true" in the sense of the ABIN system if it is either an axiom, or a theorem derived with the "deduction rule." We express this idea directly by the following code:
```
postfix 1650 is_axiom is_theorem;
x is_true = x is_axiom || x is_theorem;
```
The operator `||` is the logical "or". We need to put the precedence at 1650 or higher, so that the "or" operator binds less tightly (otherwise we would have to write `(x is_axiom) || (x is_theorem)` everywhere.

### Expressions of type B ###

Now we need to specify what an "axiom" must be. It appears useful to introduce predicates for expressions of different types. For instance, we want to recognize a "b-expression" such as `BIII` or `B` using the predicate `is_b_expr`. If we could recognize all the possible types of expressions, we would then define the predicates for well-formed expressions and for axioms:
```
postfix 1650 is_expr is_a_expr is_b_expr is_n_expr;
x is_expr = x is_b_expr || x is_n_expr || x is_a_expr;
x is_axiom = x is_n_expr && (tail x) is_b_expr;
```

Let us begin with expressions of type B. By definition, a "b-expression" is a `B` followed by zero or more `I`'s. So `x is_b_expr` is true only if the first element of `x` is `"B"` and all other elements (if any) are `"I"`. There is a standard function `all` for checking that every element of the list satisfies some condition; and `tail [1,2,3]` gives `[2,3]`. So here is our first try implementing `is_b_expr`:
```
postfix 10 is_b_expr;
equals_I c = (c=="I");
x is_b_expr = (head x)=="B" && all equals_I (tail x);
```
Note that `head []` is unevaluated, but that's OK: it will never be equal to "B".

Note also that we need to pass a _function_ to `all`. We have therefore defined `equals_I`. But defining this function separately is actually too much work: all we need is to pass the function `==` with one of the arguments set to "B". This is accomplished by using the curried form `(=="I")`. So we can replace the above definition by
```
x is_b_expr = (head x)=="B" && all (== "I") (tail x);
```

This definition works; however, it does not look simple. Here is how we can use tools that are more idiomatic to PURE, namely **pattern matching**. We simply write a bunch of different definitions one after another:
```
   ["B"]  is_b_expr = true;
("B":xs)  is_b_expr = all (== "I") xs;
    x     is_b_expr = false;
```
This looks like a set of different definitions for the function `is_b_expr`, and the definitions complement each other: The first one is for a single-item list consisting only of `["B"]`, the second one for a list consisting of more than one item, and the last one is a "catch-all" case that will match if everything else fails to match. (The order of the definitions is important; matching will be tried in this order!)

Recall that `("B":xs)` is a pattern that matches a list whose first element is `"B"` while the rest of the list is matched by the temporarily introduced variable `xs`. In this way, we avoid having to write `tail` explicitly, and achieve a more visual representation of the second rule.

One little detail: The last rule contains a pattern variable, `x`, that we do not use in the body of the rule. To avoid introducing the name `x` where we do not need any name, the symbol `_` can be used:
```
    _   is_b_expr = false;
```
In this way, we remind ourselves that this is a "catch-all" rule that matches anything.


### Expressions of types N and A ###

Defining the predicate for expressions of the form `Nx` is easy:
```
("N" : xs) is_n_expr = xs is_expr;
_ is_n_expr = false;
```

The remaining type of expression is A. Recognizing this type is less straightforward since, by definition, an "A-expression" is of the form `Axy`. So we need to verify that the tail of the list can be split into two parts, each being a well-formed expression. However, we do not know in advance how long the first expression is. Also, the theorem prover must be able to check that the first part, `x` in `NAxy`, is such that `Nx` is "true".

So we need an auxiliary function that will take an expression of the form `A...` and extract the _first_ well-defined expression, `x`, contained within it, so that the entire expression is `Axy` and both `x` and `y` are well-defined expressions. If such `x` cannot be found, the function should return `false`.

At this point, we note that there is no ambiguity as to where `x` ends and `y` begins, given `Axy`. Indeed, a well-formed expression can end only with a `B` or with an `I`. The expression `y` cannot begin with an `I`, so it must begin with `A`, `B`, or `N`; but neither of these could be tacked on at the end of `x` without breaking the well-formedness of `x`.

Therefore, for parsing `Axy` it is sufficient to determine _any_ well-formed `x` as long as the rest of the string, `y`, is also well-formed. It remains to determine how many elements of the list should be assigned to `x`. Let us determine this by simple search: try the first element as `x`, then the first two, etc., and check every time that `x` and `y` are well-formed. This is perhaps not the most efficient implementation, but it will do as an example.

In an imperative programming language, such as C, this procedure is implemented by a loop over the possible length of `x`; we would have to introduce a loop counter (an iterator), etc. In PURE, there are no loops, and instead we can use recursion.

Actually, we have already implemented a recursive procedure that returns the initial sublist when a condition first becomes true (this is the `split_list` function shown above). This function will return `false` when the condition is not true for any initial sublist. We can use the standard library function `listp` to check whether the return value of `split_list` is a list.
```
check_two_exprs lst = listp (split_list have_two_exprs lst)
 with
  have_two_exprs list1 list2 = list1 is_expr && list2 is_expr
 end;
```

Now it is straightforward to check for an a-expression:

```
("A":xxx) is_a_expr = check_two_exprs xxx;
    _     is_a_expr = false otherwise; // can write 'otherwise' for clarity,
                                     // even without a preceding conditional
```

A "theorem" is some expression of the form `NAxy` provided that `Nx` is "true" (i.e. either an axiom or a theorem).
```
x is_theorem = x is_n_expr 
  && (tail x) is_a_expr 
  &&  has_subtheorem x // whether Nx is "true"
 with
   has_subtheorem list = ("N":first) is_true
     when
      first = split_list have_two_exprs (tail ( tail x))
     end
 end;
```

For testing, let us try a few ABIN statements.
```
const test1 = "NBI"; // this is "true" because it is an axiom
const test2 = "NBIBI"; // this is not a well-formed expression
const test3 = "NABIBI"; // this is "true" because of derivation (NBI, BI) -> NABIBI
const test4 = "NANBBI"; // this is a well-formed expression but is not "true"
const test5 = "ABB"; // this is well-formed but not "true"
const test6 = "NABIBIBI"; // not well-formed
const test7 = "NAABABBBI"; // "true"
const list1 = [test1, test2, test3, test4, test5, test6, test7];
map (is_expr) list1; // need parentheses around the postfix operators!
map (is_true) list1;
```

The result of this is
```
> map (is_expr) list1;
[1,0,1,1,1,0,1]
> map (is_true) list1;
[1,0,1,0,0,0,1]
```
Here for testing purposes we use `map`, a standard library function that applies a given function to each element of the given list. For example:
```
> map (\x->x+2) [10,20];
[12,22]
```

**Exercise**: This implementation of `is_theorem` is not efficient because it traverses the list `x` multiple times as various parts of the list are checked. Optimize `is_theorem` to avoid this.

### Complete code for ABIN ###

The complete source code for this example is found below.

```
// toy theorem prover ("ABIN system")

postfix 1650 is_true is_expr;
x::string is_true = (list x) is_true;
x::string is_expr = (list x) is_expr;

postfix 1650 is_axiom is_theorem;
x is_true = x is_axiom || x is_theorem;

postfix 1650 is_a_expr is_b_expr is_n_expr;

x is_expr = x is_b_expr || x is_n_expr || x is_a_expr;
x is_axiom = x is_n_expr && (tail x) is_b_expr;

   ["B"]  is_b_expr = true;
("B":xs)  is_b_expr = all (== "I") xs;
    _     is_b_expr = false;

("N" : xs) is_n_expr = xs is_expr;
     _     is_n_expr = false;

split_list cond (x:xxx) = split_list_rec cond [x] xxx
 with
  split_list_rec cond list1 [] = false; // already tried all sub-lists
  split_list_rec cond list1 list2@(x:xxx)
     = list1 if cond list1 list2; // return the initial list on success
     = split_list_rec cond (cat [list1, [x]]) xxx   otherwise; // continue
 end;

have_two_exprs list1 list2 = list1 is_expr && list2 is_expr;

check_two_exprs lst = listp (split_list have_two_exprs lst);

("A":xxx) is_a_expr = check_two_exprs xxx;
    _     is_a_expr = false otherwise;

// finally:
x is_theorem = x is_n_expr
  && (tail x) is_a_expr
  &&  has_subtheorem x
 with
   has_subtheorem list = ("N":first) is_true
     when
      first = split_list have_two_exprs (tail (tail x))
     end
 end;

// testing
const test1 = "NBI"; // this is "true" because it is an axiom
const test2 = "NBIBI"; // this is not a well-formed expression
const test3 = "NABIBI"; // this is "true" because of derivation (NBI, BI) -> NABIBI
const test4 = "NANBBI"; // this is a well-formed expression but is not "true"
const test5 = "ABB"; // this is well-formed but not "true"
const test6 = "NABIBIBI"; // not well-formed
const test7 = "NAABABBBI"; // "true"
const list1 = [test1, test2, test3, test4, test5, test6, test7];
map (is_expr) list1;
map (is_true) list1;
// the result should be
//
//
// > map (is_expr) list1;
// [1,0,1,1,1,0,1]
// > map (is_true) list1;
//[1,0,1,0,0,0,1]
```