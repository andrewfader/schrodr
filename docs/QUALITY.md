# The preset quality bar

Every factory preset is judged from audio, not from its settings. `build/schrodr-preset-audit
<dir>` renders each preset with the same phrase (a bar of a C major chord with a bass note at
120 BPM, then three melody notes, then the preset's own tail), writes a stereo WAV per preset,
and prints the measurements below. A preset round is done when every preset meets the bar, the
flagged columns are empty, and the spectrograms (`ffmpeg -lavfi showspectrumpic`) look like the
instrument they claim to be.

The measurable part of the bar is enforced by `engine_tests`
(`every_preset_meets_the_quality_bar`) with the same phrase (`tests/audit_phrase.hpp`), so a
preset cannot ship below it.

| Measurement | Bar | Why |
|---|---|---|
| Loudness (the loudest 400 ms of the phrase, BS.1770 momentary, K-weighted, mono sum) | -18 LUFS, within 2 dB | Presets sit at one level under the fingers: the ear judges a chord by its loudest moment, not by its peak sample, so a clavichord's whisper and a grand's chord are matched here |
| Limiter | the output limiter (ceiling -2 dBFS, instant attack, 50 ms release) reduces by no more than 9 dB anywhere in the phrase; the clip flag stays clear | A fortissimo chord reaches the target by letting the limiter catch its attack; a preset that needs more than that is being squashed and should be set quieter |
| Level of the held chord | above -34 dBFS RMS for sustaining instruments; struck and plucked ones are judged on their loudness above | A chord that drops out of the mix is not playable |
| Melody notes | within 16 dB of the chord's peak | Single notes must not vanish next to a chord |
| DC offset of the held chord | below -40 dBFS | Asymmetric pickups and jets leak DC that thumps on note edges |
| Idle output before the first note | below -55 dBFS RMS | Noise floors stack across tracks |
| Tail | within the engine's advertised tail; below -50 dBFS once it has passed | Hosts stop calling `process` after the tail |
| Finite | every sample | |
| Character | the spectrogram shows the thing named: a hammer's two-stage decay, a harpsichord's even plucks, a stopped pipe's odd harmonics | Numbers cannot see this; a person looks |

Measurements are over the mono sum: the held level over 0.2 s to 1.4 s after the chord's onset,
brightness as the spectral centroid of the same span, width as 1 minus the left/right
correlation, tail as the peak over the last 100 ms of the render.

## Instrument levels

Presets are only comparable if the instruments behind them are. `build/schrodr-preset-audit
--levels` plays one C4 at a firm touch through every instrument with the default settings and
master at 0 dB, and prints the peak, the held RMS and the trim each would still need to reach
-16 dBFS RMS held or -4 dBFS peak, whichever comes first. The trims live in `levels::` in
`src/engine.hpp` and are held by `engine_tests` (`every_instrument_plays_one_note_at_a_comparable_level`:
held -22 to -10 dBFS RMS, peak under the ceiling). When a model's level changes, re-run
`--levels` and move its trim, not the presets.

## How a round goes

1. Build, run the audit, and read the table.
2. Fix the model before fixing the preset: a preset that only sounds right at one level is a
   model that is wrong at the others.
3. Re-render, look at the spectrograms of anything changed, repeat until the flags column is
   empty.
4. Run the complete suite.

## The reference recordings

The bar above holds a preset to levels and a clean tail. It cannot tell whether a grand sounds
like a grand. For that, 13 of the 25 instruments are held to recordings of real ones, note for
note and marking for marking, by `reference_tests` (`features/reference_recordings.feature`).

**The recordings.** The audio is never committed: only the numbers measured from it are, in
`tests/reference_data.hpp`.

| SCHRODR | Recording | Notes |
|---|---|---|
| Concert grand | Iowa MIS Steinway | B0, C2, C4, C6, C7 at pp, mf, ff |
| Upright | VSCO-2 CE "Upright Nr. 1" | C1 to C7 at pp, mf, f |
| Flemish, Italian, French harpsichords | VCSL, one 8' choir each | C2 to B5, eight or nine notes each |
| Unfretted clavichord | Edinburgh, St Cecilia's Hall, 1740s, A = 415 | eleven notes C2 to C6 |
| Celesta | Sonatina (freesound pack 6166), soft and hard | C4 to C7 |
| Glockenspiel | Iowa MIS orchestra bells, plastic mallets | C6, G6, C7 |
| Electric piano | jRhodes3d, a 1977 Rhodes Stage 73, from the harp | F1 to C7, three velocity layers |
| Toy piano | a Michelsonne (freesound pack 4565), Ogg previews | F4 to F5 |
| Chamber organ (Gedackt 8') | VCSL Renaissance organ, stopped 8' | F#2 to C6 |
| Violin, viola, cello | Iowa MIS 2012, no vibrato, anechoic | each on the string SCHRODR's player takes it on |

There are 172 recordings, listed in `tests/reference/manifest.tsv` as `SOURCE:PATH`, each pinned
by SHA-256 (git sources also by commit, the clavichord's archive by its own hash).
`tools/fetch_references.py` fetches them (about 1.1 GB, FLAC and Ogg decoded beside the
originals) into `~/.cache/schrodr/references`. `build/schrodr-reference-extract
tests/reference/manifest.tsv ~/.cache/schrodr/references tests/reference_data.hpp` measures them
and rewrites the table the suite reads. The suite itself needs no network and no audio files.

What was found wrong with the sources, and what was done about it (the manifest records each):
- The 2012 Iowa viola files are 96 kHz audio under a 44.1 kHz header; the manifest gives their
  true rate. `Piano.ff.B0` is the C1 take.
- The clavichord is pitched at A = 415, so each note sounds a semitone under its name; its
  files' own pitch labels come from an automatic detector and are often wrong. Each row is keyed
  by the note it sounds, checked by measurement. The takes without noise reduction stand under
  25 dB over the museum's air conditioning, so the noise-reduced ones are used, and only those
  holding one clean note; their tails are not judged.
- The toy piano is out of tune; its lowest note sounds F4 plus 29 cents and is keyed so.
- The jRhodes3d samples were recorded through EQ and each velocity layer was normalised by the
  sampler, so the Rhodes' colour carries some of the EQ and its dynamics are not judged.
- Quiet takes standing less than 25 dB over their recording's floor are not measured: thirteen
  Iowa takes, the upright's C3 and C7 at pp, the organ's C2. The celesta's top octave runs
  sharp: E7 sounds 41 cents over its key and G#7 is not found within 40 cents of its own.

**What is measured** (`tests/reference_measure.hpp`): one function measures a recorded note and a
rendered one, so the two are always measured the same way.
- **Colour:** the spectral envelope, meaning the partials' levels joined in log frequency and
  read at third-octave centres up to 10 kHz, compared as a shape (RMS dB with the mean removed).
  Third-octave band sums were tried first and dropped: in the bass a third octave is narrower
  than the harmonic spacing, so they measured where the harmonics fell, not the instrument. Not
  judged for bars (celesta, glockenspiel, toy piano), whose few inharmonic partials are not an
  envelope.
- **Brightness:** the spectral centroid in harmonics.
- **Dynamics:** the rise in loudness from the softest recorded marking to the loudest, where
  the session kept one input gain.
- **Struck and plucked strings:**
  - the inharmonicity B, fitted to the partials (at the loudest marking);
  - the prompt sound's decay;
  - the aftersound's decay, taken from its energy decay curve (Schroeder integration with the
    floor subtracted and Lundeby's compensation at the cut-off), which beating unisons cannot
    fool.
- **Bars:** the first overtone over the fundamental, taken from what the blow added (a scale's
  earlier bars are still ringing), and the fundamental's decay. Not the overtone for the
  electric piano (its pickup puts harmonics where the search looks) or the celesta (the
  recording's own notes disagree, 2.2 to 4.4 f1).
- **Bowed strings:** the bow noise, the power between the harmonics over the power on them. The
  attack and vibrato are printed but not judged: in these recordings they are the player's.
- **Organ pipes:** the steady tone's colour and brightness.

The first scenario proves the measurements on synthetic signals whose answers are known.

**The bar** (`tests/reference_compare.hpp`, `tests/reference_render.hpp`) is calibrated on the
recordings themselves. SCHRODR's note must come as close to the recorded note as that real
instrument's own notes, up to an octave apart, come to each other; the extractor prints that
spread. The colour and brightness bars are each instrument's median spread, never tighter than
6 dB and a factor 1.4.

| Comparison | Bar |
|---|---|
| Pitch | within 25 cents of the key |
| Colour | 6 dB RMS; upright 8.5, Flemish 9.5, Italian 9, French 7.5, organ 10.5 (their spreads: 8.2, 9.3, 8.7, 7.5, 10.4; clavichord 5.4, electric piano 5.8, violin 6.5, viola 6.0, cello 5.6) |
| Brightness | factor 1.4; upright 1.45, Italian 1.6, French 1.75, toy piano 2.0, cello 2.25 (their spreads: 1.44, 1.57, 1.75, 2.01, 2.24) |
| Dynamics, softest marking to loudest | 5 dB (one Iowa cello's pp to ff runs 11 to 17.5 dB) |
| Inharmonicity, at the loudest marking | factor 1.5 |
| Prompt and aftersound decay, bar decay | factor 1.5 |
| Bar's first overtone | 5% |
| Bow noise | 6 dB (spreads: violin 4.4, viola 2.5, cello 7.2 dB) |

SCHRODR is rendered with each instrument's defaults, the room off and the master at 0 dB, so
what is compared is the instrument and not a preset's room; the organ draws the Gedackt 8' alone,
as recorded. Bowed notes are rendered without vibrato because the recordings have none (under 6
cents). Velocity follows the usual MIDI readings of the markings: pp 33, mf 80, f 96, ff 112.

**Where SCHRODR stands.** SCHRODR meets 410 of the 775 comparisons. The other 365 are listed in
`tests/reference_gaps.hpp` under 41 causes, each with the numbers. The suite reports them and does
not fail on them. It fails as soon as one passes, until that line is struck off, so the bar only
rises. Each bar has a 2% dead band: a check fails only when it misses by more than 2%, and a gap
counts as closed only when it clears the bar by more than 2%. Without it, a number that lands on
its bar (viola G3 ff's colour, 6.0 dB against 6) passed on Linux and failed on macOS. The largest
causes:
- **Grand:** the bass's wound strings are three to six times too flexible (B); the middle and
  bass lack their low partials, so C4 is twice as bright as the Steinway; the prompt sound falls
  at the wrong rate across the compass.
- **Upright:** the bass and tenor are up to 2.4 times too bright; pp to f rises 12 to 23 dB
  against the real upright's 27 to 33; C6's prompt sound falls three times too fast.
- **Harpsichords:** the strings are two to eight times too flexible below C5; the prompt sound
  falls 1.5 to 4 times too fast in the bass and middle; the colour sits 10 to 16 dB off.
- **Clavichord:** the bass and middle are 1.5 to 6 times too bright and their prompt sound falls
  at half the real rate; C6 sounds 29 cents sharp.
- **Bars:** the celesta's and toy piano's lower notes ring two to five times too long; the toy
  piano's first overtone sits at 4.25 or 3.00 f1 where the real rods' sits near 4.03; the
  glockenspiel's is not where the Iowa bars have it.
- **Electric piano:** the tine rings two to four times too long, and its brightness grows with
  the blow where the Rhodes' bark is in its soft layers.
- **Violin family:**
  - 10 to 27 dB too much bow noise;
  - key velocity barely changes the level (0 to 4 dB from pp to ff against the players' 9 to
    17.5);
  - the cello's low strings lack their fundamental;
  - the cello's E2 at ff leaves Helmholtz motion after 0.9 s and squeals on its 8th harmonic.
- **Organ:** only F#2 misses (too little above its fundamental).

**Not covered.** No downloadable recordings of single notes were found for the Viennese
fortepiano, square and tangent pianos, the Cristofori, the virginal, spinet and Lautenwerck,
the fretted clavichord, the hurdy-gurdy and Geigenwerk, or the dulcitone. For these
instruments, the measurable bar above and the spectrogram review are still all there is.
