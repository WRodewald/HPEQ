# HPEQ

HPEQ is a convolution software tool designed to equalize or simulate the frequency response of headphones. It’s written with JUCE and thus can be exported as a standalone, VST or AU plugin. It supports any stereo WAV-file impulse response up to a length of 16384 samples, however that limit can easily be changed in source code. The inspiration to build this tool came from the preexisting FABIAN HRTF database from the Audiocommunication Group at TU Berlin which contains the stereo impulse responses of 41 common headphone models. Using the inverse transfer function allows the equaliziation of a desired headphone model. If an additional transfer function of any headphone model is added afterwardsthis allows an approximate simulation of that headphone models response. Since HPEQ is merely a convolution plugin for static impulse responses other use cases, such asr everberation for aesthetic purposes can also be considered.

# Build

The repository is mostly self contained. The juce library is included as a sub module. The only requirement is the Projucer application that has to be downloaded from Juce.com to generate IDE projects from the jucer project file. The source code is tested on Windows and MacOS.

# Impulse Response Data

The FABIAN head-related transfer function data base can be found here.https://depositonce.tu-berlin.de/handle/11303/6153.2
It contains a number of impulse responses of different headphones that were used to develop HPEQ. 

# ParFilt Implementation

Currently, HPEQ implements a work in progress version of the algorithm described in Bank 2007 "Direct Design of Parallel Second-order Filters for Instrument Body Modeling". The implementation is disabled in source but can be enabled by defining ENABLE_PARFILT_WIP. 
