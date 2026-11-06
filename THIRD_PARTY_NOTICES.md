# Third-party notices

## CLAP

The CLAP headers (free-audio/clap, tag 1.2.7) are fetched at configure time. MIT License,
Copyright (c) 2021 Alexandre BIQUE.

## Editor engine and test harness

The platform-neutral editor (`src/ui/editor.hpp`, the canvases and window backends), the CLAP
test harness, the audio measurements, the room and air model (`src/dsp/air.hpp`), the waveguide
rails and loss and dispersion fits (`src/dsp/string.hpp`) are adapted from the same author's
FUZZBOI (https://github.com/andrewfader/fuzzboi) and YANES
(https://github.com/andrewfader/yanes) projects, under the same source-available licence as this
repository.

## Published data

The string scalings, hammer parameters, soundboard modes, bar dimensions and pipe scales are
taken or estimated from the published literature named in each source file's header comment
(Fletcher and Rossing, Conklin, Giordano, Weinreich, Suzuki, Chaigne and Askenfelt, Askenfelt and
Jansson, O'Brien, Kottick, Brauchli, Pollens, Palmer, Verge, Cook, Toepfer); none of it is code.
