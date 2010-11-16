================
Pure Domain Test
================

Passes test if all links in `Test cases`_ below are valid.

Sample descriptions
===================

.. default-role:: emphasis

Default Namespace
-----------------

This is a function in the default namespace:

.. pure:function:: sort p xs

   Sorts the elements of the list `xs` in ascending order according to the
   given predicate `p`, using the C `qsort` function. The predicate `p` is
   invoked with two arguments and should return a truth value indicating
   whether the first argument is "less than" the second. An exception is
   raised if the result of a comparison is not a machine integer. Example::

     > sort (>) (1..10);
     [10,9,8,7,6,5,4,3,2,1]
     > sort (<) ans;
     [1,2,3,4,5,6,7,8,9,10]

   This function also works with matrices, see :pure:func:`sort/matrix` below.

Another entry for the same function. This happens a lot in Pure, as functions
are frequently overloaded. The Pure domain handles these gently, if you
specify a unique tag of the form ``/id`` after the function name for each
overloaded instance. Also note that in contrast to the other Sphinx domains,
parameter lists are generally written in Pure's curried style and they may
contain arbitrary extra annotations such as type tags.

.. pure:function:: sort /matrix p x::matrix

   Sorts the elements of a matrix `x` (non-destructively, i.e., without
   changing the original matrix) according to the given predicate `p`, using
   the C `qsort` function. This works exactly the same as with lists (see
   :pure:func:`sort` above), except that it takes and returns a matrix instead
   of a list. Note that the function sorts *all* elements of the matrix in one
   go (regardless of the dimensions), as if the matrix was a single big
   vector. The result matrix has the same dimensions as the input
   matrix. Example::

     > sort (<) {10,9;8,7;6,5};
     {5,6;7,8;9,10}

Operator notation is supported as well. To these ends, add one of ``prefix``,
``postfix``, ``infix`` and ``outfix`` at the beginning of the function
signature.

.. pure:function:: infix + x y

   Add two values. This works with numbers, strings and lists.

.. pure:function:: prefix - x

   Unary minus. Negates a number. This operator is special in Pure, as it's
   the only infix operator which is also used as a prefix operator.

.. pure:function:: postfix & x

   Creates a thunk which gets evaluated lazily. This is the only postfix
   operator defined in the prelude.

.. pure:function:: outfix {: :} x

   An outfix operator.

.. pure:module:: gsl

Custom Namespace
----------------

The following functions are declared in the :pure:mod:`gsl` namespace. (The
rst markup used here is the same as in the Python domain, but note that it
actually refers to Pure *namespaces* instead of source modules.)

.. pure:function:: sort_vector m::matrix

   Implements `gsl_sort` and `gsl_sort_int` without `stride` and `n`
   parameters.

.. pure:function:: sort_vector_index m::matrix

   Implements `gsl_sort_index` and `gsl_sort_int_index` without `stride`
   and `n` parameters.

More markup
-----------

.. pure:currentmodule:: None

The usual markup for embellished function descriptions is supported as well,
e.g.:

.. pure:function:: foldl f a xs -> b

   Accumulate the binary function `f` over all members of `xs`, starting from
   the initial value `a` and working from the front of the list towards its
   end.
   
   :param f:  accumulating function
   :type  f:  closure
   :param a:  initial value
   :type  a:  any type
   :param xs: values to be accumulated
   :type  xs: list
   :return:   accumulated value `b`
   :rtype:    any (usually the same as `a`, but that depends on `f`)

There's also special markup for extern functions, macros, variables and
constants.

.. pure:extern:: puts s::string

   Output a string on the terminal.

.. pure:macro:: void (catmap f x) = do f x
   		void (listmap f x) = do f x

   Helper macro to execute a list comprehension which is evaluated solely for
   its side-effects. This turns the implicit `map` of the comprehension into a
   `do` and thereby optimizes away the intermediate list result.

.. pure:variable:: stdin

   The standard input stream. This is a built-in variable.

.. pure:constant:: c = 299792

   A constant. The speed of light, what else?

Test cases
==========

Unqualified access in the default namespace
-------------------------------------------

See :pure:func:`foldl` and :pure:func:`sort` above. Also see
:pure:func:`sort/matrix` for the matrix version. Also see :pure:macro:`void`,
:pure:var:`stdin`, :pure:const:`c`.

This should work just as well (using ``pure:obj`` as the default role):

.. default-role:: pure:obj

See `foldl` and `sort` above. Also see `sort/matrix` for the matrix
version. Also see `void`, `stdin`, `c`.

Unqualified access in a custom namespace
----------------------------------------

.. pure:currentmodule:: gsl

Current namespace is `gsl`.

See `sort_vector` and `sort_vector_index` above.

.. pure:module:: dummy

Qualified access in a different namespace
-----------------------------------------

See `gsl::sort_vector` and `gsl::sort_vector_index` above.

See `::foldl` and `::sort` above. Also see `::sort/matrix` for the matrix
version. Also see `::void`, `::stdin`, `::c`.

Qualified access suppressing the namespace
------------------------------------------

See `~gsl::sort_vector` and `~gsl::sort_vector_index` above.
