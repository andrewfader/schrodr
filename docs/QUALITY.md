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
