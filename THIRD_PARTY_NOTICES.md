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

## Reference recordings

`tests/reference_data.hpp` holds measurements (numbers, no audio) of recordings of real
instruments. The recordings are never committed: `tools/fetch_references.py` fetches the files
listed in `tests/reference/manifest.tsv` into a local cache, each pinned by SHA-256 (and each git
source by commit). The sources, with their owners' terms, are:
- University of Iowa Musical Instrument Samples by Lawrence Fritts
  (https://theremin.music.uiowa.edu/MIS.html): "may be downloaded and used for any projects,
  without restrictions";
- Versilian Studios Chamber Orchestra 2, Community Edition (https://github.com/sgossner/VSCO-2-CE),
  the upright sampled by Simon Dalzell of Ivy Audio: CC0;
- Versilian Community Sample Library (https://github.com/sgossner/VCSL): CC0;
- the University of Edinburgh's St Cecilia's Hall sound library of an unfretted clavichord of the
  1740s (https://datashare.ed.ac.uk/handle/10283/9170): their DataShare licence;
- Sonatina Symphonic Orchestra (https://github.com/peastman/sso), its celesta from freesound pack
  6166 by stamperadam: CC0;
- jRhodes3d by John Learman (https://github.com/sfzinstruments/jlearman.jRhodes3d): its licence
  in that repository;
- Freesound pack 4565 by beskhu (a Michelsonne toy piano), read from Freesound's previews: CC BY 4.0.
