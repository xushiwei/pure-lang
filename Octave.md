# Using Pure with Octave #

[Octave](http://www.octave.org/) is a high-level interpreted language for numerical computations. While the LLVM-based, JIT-compiled [Julia](Julia.md) language may eventually take its crown, Octave's comprehensive set of matrix functions, its powerful plotting capabilities and the extensive collection of available [addon packages](http://octave.sourceforge.net/) has made it the premier open source language for doing numerical computations available right now.

The [pure-octave](Addons#pure-octave.md) module embeds the Octave interpreter into your Pure programs. You can execute arbitrary Octave code, exchange data between Pure and Octave, execute Octave functions directly from Pure (including support for Octave "inline" functions), and also call Pure functions from Octave. The mapping between Pure and Octave data such as matrices and strings is straightforward and handled automatically. An interface to Octave's plotting functions is also available.

Octave can be used conveniently along with Pure's [Reduce](Reduce.md) module in the TeXmacs environment. In particular, the Octave module also provides the plotting capabilities for Pure's TeXmacs plugin.