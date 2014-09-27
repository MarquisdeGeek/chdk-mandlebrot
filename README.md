chdk-mandlebrot
===============

A mandelbrot set generator, for Canon cameras using the CHDK


# Copyright 2014 Steven Goodwin

Released under the GNU GPL, version 2

Version 1.0 - 28th September 2014

## Binary

A binary, in the form of mandelbrot.flt is provided, which can be run directly from the file browser.

## Compilation

If you're smart enough to get the CHDK working, you're smart enough to work out this step! 

What I did was add the files into the games directly, and amend the Makefile by replace all instances of the word 'snake' with 'mandelbrot'. There's probably a simpler command line incanation though...

# Using it

When run, it initializes the high level mandelbrot that everyone is used to. Once it's complete, use the cursors to move the cursor around the screen. (On my A550, up=ISO, left=macro, right=flash, down=timer.) Once it's in position, his 'func/set' to zoom in. You can zoom out to the default mandelbrot by pressing 'disp'.

Whilst it is rendering, you may pause with the 'func/set' key.

# Future version

Scroll around the set
Zoom out, with history
Other fractal types
Optimized renderer




