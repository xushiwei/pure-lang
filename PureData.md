# Using Pure with PureData #

<a href='http://wiki.pure-lang.googlecode.com/hg/pd-faust.png'><img src='http://wiki.pure-lang.googlecode.com/hg/pd-faust-mini.png' align='right' /></a>

[PureData](http://puredata.info/) (Pd for short) is Miller Puckette's computer music and multimedia software which enables you to create interactive realtime applications in a convenient graphical programming environment, by just "patching" (connecting) basic signal and control processing objects. The data flow through the connected objects is handled by Pd in realtime, and Pd also provides the necessary interfaces to handle both control data (such as MIDI and OSC) and synchronous (sample based) signal data (usually audio, but support for rendering 3D graphics and videos is provided through 3rd party plugins).

The [pd-pure](Addons#pd-pure.md) module is a Pure "loader" plugin for Pd, which allows you to write Pd control and signal processing objects in Pure. If you're into graphical/visual programming then this module provides you with an interesting way to run Pure in Pd by connecting Pure functions and other Pd objects to do complex signal processing tasks in realtime.

This module is often used in conjunction with Pure's [Faust](Faust.md) interface which enables you to handle signal data in an efficient way. In particular, there's also a [pd-faust](Addons#pd-faust.md) module available which lets you load Faust modules dynamically in Pd, and provides for MIDI and OSC message processing, as well as generating Pd GUI interfaces for Faust dsps in an automatic fashion.