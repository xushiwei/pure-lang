# Using Pure with Faust #

Yann Orlarey's [Faust](http://faust.grame.fr/) is a functional programming language for real-time sound processing and synthesis developed at [Grame](http://www.grame.fr/) and distributed as GPL'ed software. With Faust you can develop signal processing algorithms in a high-level functional language, but still achieve a performance comparable to carefully handcrafted C routines.

Pure provides tight integration with Faust through its [built-in Faust interface](http://docs.pure-lang.googlecode.com/hg/pure.html#interfacing-to-faust) which can be used both to inline Faust code in Pure scripts and to load external Faust modules in LLVM bitcode format dynamically at runtime. Please note that the built-in interface requires [Faust2](http://www.grame.fr/~letz/faust_llvm.html), the latest Faust incarnation from Faust's git repository, which is still under development.

There's also a separate [pure-faust](Addons#pure-faust.md) addon module which can be used either with the built-in Faust interface or with external Faust modules available in shared library format. The latter also works with the mainline (stable) Faust version.