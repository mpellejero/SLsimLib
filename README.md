Documentation                                                        {#mainpage}
=============

*GLAMER* is a very flexible computer code for doing simulations of gravitational
lensing and modeling of gravitational lenses.

This page describes the steps to doing a simple simulation and where in the
documentation to find further information. The installation and usage of
*GLAMER* is described on the [installation page](@ref install). If you would
like to obtain the code, please see the [appropriate section](@ref copying).

The code can be made available to you through GitHub.com upon request.  Requests should be sent to Ben Metcalf at robertbenton.metcalf@unibo.it.

Getting started
---------------

`GLAMER` is in the form of a C++ library that can be linked into your code.  
For instructions on installing and linking the library see 
http://metcalf1.bo.astro.it/wiki/projects/glamer/GLAMER.html


Read in Parameters
------------------

Within your main code the first thing to do is to read in the input parameter
file. Your parameter file should contains all the parameters required to run the
simulation. A sample parameter file is provided in the repository. A particular
simulation will not require all the parameters to be specified. The parameter
file should be read by constructing an `InputParams` object. See the
`InputParams` class for options.  If a parameter is required, but not present in
the parameter file the program should throw an exception and notify you of which
parameter needs to be included. If a parameters is in the parameters file that
is not recognized a warning will be printed.

For a list of currently acceptable parameters with very short descriptions use
`InputParams::sample()`.

### Example

In `main()` one needs to first read in the parameter file.
This is done by constructing a `InputParams` object

~~~{.cpp}
InputParams params(paramfile);
~~~

where `paramfile` is a string containing the path and file name of your
parameter file. A sample parameter file with all allowed parameters can be
generated by `InputParams::sample()`. This can be printed to a usable parameters
file with `InputParams::PrintToFile()`.

~~~{.cpp}
#include "InputParams.h"

int main(int argc, const char* argv[])
{
    // create InputParams with sample values
    InputParams params = InputParams::sample();
    
    // print sample parameter file
    params.PrintToFile("sample_paramfile");
    
    // done
    return EXIT_SUCCESS;
}
~~~


Construct a Source
------------------

The source constructor takes the `InputParams` object and constructs one or many
sources within a MixedVector member. See the class Source and all

`SourceUniform source(params)`
: Makes the source a uniform surface brightness circle

`SourceMultiAnaGalaxy source(params)`
: Makes many individual sources.
  Where they are read in is set in the parameter file.


Construct a Lens
----------------

A lens is constructed

~~~{.cpp}
Lens lens(params, &seed);
~~~

Multiple components to the lens are stored within the lens. See the `Lens` class
documentationfor how to access them. These can be read in from a file which are
generally called "field" lenses. The "main" lens(es) can be set by parameters
in the parameter file or added by hand within `main()`.


Construct the Grid
------------------

The `Grid` structure contains the image points and thier source points along
with other information. Without further grid refinement the `Grid` can be used
to make a map of deflection, convergence, shear or time-delay. `Grid` contains
functions for outputing these.  If no further grid refinement is desired for image or caustic finding,
the `GridMap` structure can be used which requires signifcantly less memory overhead.


Image finding and Grid refinement
---------------------------------

The mean functions used for image finding are

-   `ImageFinding::find_images_kist()` 
-   `ImageFinding::map_images_fixedgrid` and
-   `ImageFinding::map_images()`.
And for finding critical curves / caustics
-   `ImageFinding::find_crit()`

These take a `Grid` and a `Lens` object. The found images are then stored in an
array of `ImageInfo` structures.


Output
------

The `Grid` has some direct output functions for making maps that do not depend
on sources (kappa, gamma, etc.). The ImageInfo structure has some information
about the images found and contains a linked list of all the points within the
image. Each point contains information about the kappa, gamma, source position,
time-delay and in some cases the the surface brightness at that point.
# CosmoLib
