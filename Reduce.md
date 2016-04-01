# Using Pure with Reduce #

[Reduce](http://reduce-algebra.com/) is one of the oldest computer algebra systems which has been around since the 1960s and is widely recognized as a state-of-the-art, powerful and efficient CAS. It is free/open source software distributed under a BSD-style [license](http://www.reduce-algebra.com/license.htm), actively maintained on its [SourceForge website](http://sourceforge.net/projects/reduce-algebra/), and implementations exist for all major computing platforms.

The [pure-reduce](Addons#pure-reduce.md) module embeds Reduce in Pure, so that the functionality of Reduce becomes available in Pure in a seamless way. More background information and a discussion of the interface can be found in the [Embedding REDUCE](http://groups.google.com/group/pure-lang/browse_thread/thread/c11e82ca2e9e8cbb) thread on the Pure mailing list.

Reduce can be used conveniently along with Pure's [Octave](Octave.md) module in the TeXmacs environment. In particular, the Reduce module also provides the math output capabilities for Pure's TeXmacs plugin.