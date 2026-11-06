# What is simulated

SCHRODR's instruments are simulated from their physics rather than sampled or imitated. This
document says what each model is, where its numbers come from, how the tests hold it to the
physics, and what is left out. The models are `src/dsp/keyboard.hpp` (string keyboards),
`src/dsp/bars.hpp` (struck bars), `src/dsp/pipes.hpp` (the organ), `src/dsp/string.hpp` (one
string) and `src/dsp/air.hpp` (the room); `src/engine.hpp` plays them from the keys.

## One string

A string is a digital waveguide (Smith; Karjalainen, Välimäki and Tolonen): displacement waves on
two rails, nut to bridge and back, in two transverse planes (normal to the soundboard, which
drives it and dies fast, and parallel to it, which rings on, a cent or two apart). In the loop:

- **Losses fitted to the wire.** Air drag (Stokes), the wire's internal friction and bending
  losses (Woodhouse's form; a winding's turns rub, a buff stop's leather adds loss in proportion
  to frequency) give a decay rate for every partial, and a shelf-plus-low-pass filter is fitted
  to them in least squares on the dB lost per round trip. `string_tests` holds the fitted loop's
  fundamental decay within 15% and its 8th partial within 30% of the rates asked for.
- **Stiffness dispersion.** Identical first-order allpasses (2 to 16 of them, chosen with the
  coefficient) are fitted so the partials sit at n f1 sqrt((1 + B n^2) / (1 + B)) with
  B = pi^2 E I / (T L^2) from the core's diameter, within the first 24 partials below 10 kHz
  (`string_tests`: within 4 cents at middle C, 8 cents at C7).
- **A force at a point.** A hammer, tangent or wheel pushes displacement into the string at
  its point. In displacement waves that is a plateau: +d on the rail heading away from the
  nearer end, over every cell between that end and the point, and -d on the other rail beyond
  the point, so the sum is d at the point and nothing ahead of the fronts (`string_tests`
  checks the plateau's spread and reflection cell by cell). The point force moves its point at
  F / 2Z, the string's driving-point admittance. Three details make this exact enough for a
  bow to work on: the loop's filters are lumped at the bridge and the rest of the period is
  split evenly between the rails (the bridge reads a whole cell; the loop's fraction of a cell
  is read at the nut), so lower cell j is j cells from the bridge on both rails and a push
  lands where it is read back; the push is laid to land on the point at the next sample, when
  the force has acted; and the far block of each plateau, standing for a wave that has reached
  the end of the string already, primes that end's filters with its steady state, or they
  would answer it only after their own delay and leave a notch that long behind every push.
  Laid without these, a point damper on the string feeds it (`string_tests` keeps a damper of
  the string's own impedance passive). A bow pushes one way for as long as it plays, and what
  that adds to both rails in equal and opposite measure is no displacement at all, only the
  two-rail picture's gauge; it is taken out now and then so it cannot swamp the waves.
- **The pluck.** A plectrum leaves the string in a rounded triangle, laid on the rails at rest
  with the bridge filters run over the half that has already reflected.

## The string keyboards

Every note of every instrument is a course of real strings whose length and tension follow the
designer's scaling (anchors at a few notes, log-interpolated), with the wire that gives them:
music wire, 18th-century iron, brass, gut or nylon, plain up to the instrument's thickest plain
gauge and wound on a core beyond it. The tension at pitch, the impedance sqrt(T mu) and B follow.

| Instrument | Compass, strings | Scaling (length and tension at a few keys) | Action | Board |
|---|---|---|---|---|
| Concert grand (Steinway D pattern) | A0-C8; 1, 2, 3 strings from E1 and E2 | 2.00 m / 950 N at A0, 0.62 m / 700 N at C4, 51 mm / 700 N at C8; music wire to 1.5 mm, wound below the tenor break | felt hammers 12 to 4 g, K 3e8 to 1.5e10 N/m^p, p 2.2 to 3.0, striking 1/8 to 1/12 | Suzuki's first modes from 49 Hz, dense above 210 Hz; dampers to F#6 |
| Upright (Yamaha U1 pattern) | A0-C8 | bass folded to 1.22 m | 10 to 4 g, K 4e8 to 1.2e10 | modes a quarter higher, giving more |
| Viennese fortepiano (Walter 1795) | FF-g'''; brass pairs, iron triples from c | 1.78 m / 120 N at FF, 0.55 m / 140 N at c' | leather, 3.5 to 1.5 g, K 1.5e8 to 1.5e9, p 1.8 to 2.2, striking 1/7 to 1/11; moderator | thin spruce, first mode 95 Hz |
| Square piano (Broadwood 1795) | FF-f'''; pairs | 1.30 m / 90 N to 0.10 m / 90 N | leather, 2.5 to 1.2 g, K 1e8 to 1e9 | small, boxy, first mode 120 Hz |
| Cristofori 1720 | C-c'''; pairs | brass and iron at harpsichord tensions | paper and leather, 1.5 to 0.8 g, K 8e7 to 6e8, p 1.7 to 2.0; una corda | a harpsichord's board |
| Tangent piano (Späth and Schmahl 1790) | FF-g'''; pairs | as the fortepiano | bare wood, 2 to 1 g, K 1e9 to 4e9, p 1.6 to 1.8; moderator | as the fortepiano |
| Flemish harpsichord (Ruckers 1640) | GG-d'''; 2 x 8' and 4' | c'' at 36 cm (the Flemish scale), 1.75 m at GG; iron to c, yellow brass below | quills releasing at 1.5 mm; back 8' jacks 75 mm from the nut, front 48, 4' 30, lute 14, skewed toward the bass | 3 mm spruce, first mode 105 Hz |
| Italian harpsichord | C-c'''; 2 x 8' | a short brass scale at 42 N | 65 and 40 mm | thinner, higher |
| French double (Taskin 1769) | FF-f'''; 2 x 8' and 4', lute, buff | 1.85 m at FF | 78, 50, 32, 15 mm | larger, first mode 85 Hz |
| Virginal (Ruckers muselar) | C-c'''; one 8' | plucked at two fifths of the string in the bass, a third in the treble | | the whole case |
| Spinet | GG-g'''; one 8' | short strings, plucked 45 mm from the nut | | small |
| Lautenwerck | C-c'''; 2 x 8' of gut, overspun in the bass | | soft leather plectra | |
| Fretted clavichord | C-d'''; brass pairs, C#/D, D#/E, F#/G, G#/A, A#/B sharing | 1.30 m / 22 N at C, 0.55 m / 26 N at c' | brass tangents, K 2e9, p 1.5 | tiny, first mode 160 Hz |
| Unfretted clavichord (Hass 1760) | FF-f'''; a pair to every key | | | a little larger |
| Hurdy-gurdy (G/C) | g'-g''' on two gut chanterelles; bourdons C and c, trompette c' | 0.345 m chanterelles, drones 0.40 to 0.46 m | a 75 mm wheel a sixth from the bridge | a lute's body |
| Geigenwerk (Haiden 1575) | C-c'''; single iron and brass | a harpsichord's lengths | 90 mm wheels a fifth from the bridge | an Italian harpsichord's |

**The hammer** (Hall; Chaigne and Askenfelt 1994) is a mass whose felt pushes with
F = K u^p (1 + h du/dt): the compression against each string of the note, solved implicitly
with the string's driving-point admittance each sample, the hammer decelerated by the sum and
thrown off when the strings leave it. The hammer meets all the strings of a note; the una corda
shifts it onto softer felt and off the third string; the moderator's cloth makes the felt as
good as thirty times softer and costs the hammer a third of its speed. The
key's velocity reaches the hammer as 0.3 to 5.3 m/s (Askenfelt and Jansson). `keyboard_tests`
holds the contact to 2 to 5 ms at pianissimo and under 2 ms at fortissimo on middle C, the
fortissimo more than 20 dB louder with its centroid 30% higher.

**The plectrum** lifts the string to the quill's release deflection (1.5 mm on the Ruckers),
whatever the key's speed (within 2.5 dB, `keyboard_tests`); a slow key plucks later, and each
jack of a note a little after the last. On release the jack brushes the string on the way down
and its damper lands. The registers are separate strings with their own jacks, regulated to
pluck in turn (back 8', 4', front 8', a millimetre of travel apart), the front row's quills
facing the other way; the 4' has its
own bridge junction.

**The tangent** is a light hard hammer that strikes near the string's end and stays, the
string's rest now on its edge. The key's depth lifts the string by 0.5 to 2.3 mm and stretches
it over the dead length behind the tangent, so a firm stroke sounds 3 to 20 cents sharper, and
aftertouch (bebung) stretches it further. A released string is let go over the key's return
(10 ms) rather than stopped at the nut: the plateau the tangent held would otherwise jump into
the bridge as a thud; the listing cloth has it 40 dB down within 0.1 s, the rest the board's own
short ring. On the fretted instrument a shared pair is choked by the second tangent.

**The wheel** bows with stick-slip friction: mu_s = 0.8 holds the string to the rim while the
pull that takes (2 Z times the string's lag behind the rim, over the coming sample, from the
waves about to arrive at the contact from either side) is within the static limit; past it the
string slides on friction falling from mu_s toward mu_d = 0.25 with the excess lag, taken as
one curve over the lag (the single-valued, thermal picture of rosin, Smith and Woodhouse 2000)
and followed with a 0.2 ms lag, as rosin's friction settles. On a plain string this gives the
Helmholtz motion (one slip a period, lasting the wheel's fraction of it); the hurdy-gurdy and
Geigenwerk hold their pitch within a few cents (`keyboard_tests`, `advertised_tests`). The
wheel's rim speed is its radius times the crank; its pressure wobbles once a turn and the rosin
adds grit. The hurdy-gurdy's keys stop the
chanterelles (the highest key wins; open g' with none) while the drones sound as long as the
player cranks, and the trompette's dog slaps the board when the string pulls hard enough.

**The bridge** is a junction shared by the strings of a note: every string's arriving wave,
weighted by its impedance, drives the bridge through its mobility (a floor rising toward the
treble, plus the soundboard's ten lowest modes at 2 Z Y / (1 + Z Y)), and every string gets the
bridge's motion back. Unison strings moving together drive the board and die fast; moving
against each other they leave it still and ring on: Weinreich's two-stage decay
(`keyboard_tests`: the first third of a second falls more than twice as fast as the second
second), and a cent of mistuning beats. The tuner tunes out the bridge's pull on the pitch
(the loop is tuned through the junction's phase), as a tuner does.

**The soundboard** is a modal bank per channel (the two microphones hear the modes at
different levels), driven by every bridge's force, radiating as acceleration below the board's
coincidence and rolling off above a wooden plate's reach; and a mono bank whose motion feeds
back to every undamped string at a transfer coupling. **Sympathetic strings**: when dampers are
up, the strings whose partials coincide with a struck note's are woken and ring with it
(`keyboard_tests`: C5 and G5 wake under C4 with the pedal down, C#4 does not). The room's
humidity moves the board's modes (Hailwood-Horrobin moisture, Wood Handbook) and its
temperature slackens the strings against their frame (the pitch follows the square root of the tension: from 20 to 40 C the Flemish harpsichord's iron c' goes about 25 cents flat against its wooden case, `advertised_tests`, a gut string nearer half a semitone, and a grand's steel against its iron frame hardly moves).

**Dampers and pedals.** A released string is damped over the damper's time (slower in the
bass); the sustain pedal lifts every damper (half way, they graze the strings); the sostenuto
holds those down when it went down; the top notes of a piano have none. The soundboard carries
the key bed's thud, the dampers' fall, the jacks' click and the pedal's thump at the Mechanics
setting.

**The tuner.** Temperaments are laid out from the chosen key with A at the reference; the
octaves are stretched to the strings' own inharmonicity as a tuner does by ear (4:2 octaves, 6:3 in
the bass), scaled by Stretch; unisons are set a
cent apart with the tuner's scatter. `keyboard_tests` holds every key of three instruments
within 3 cents of its tuned pitch, C7 of the grand 10 to 45 cents over three octaves above C4,
and the partials of middle C within 5 cents of the stiff string's.

**Budget.** At most 28 notes sound at once (the oldest released one is stopped beyond that),
and up to 8 strings are woken in sympathy per strike. A note's loop filters are fitted the
first time it is played after a re-stringing, about a millisecond each.

## The struck bars

A bar is a beam in its bending modes (Euler-Bernoulli eigenvalues, lowered by shear and rotary
inertia for thick bars): free-free for celesta plates and glockenspiel bars (1 : 2.76 : 5.40),
clamped-free for the dulcitone's fork prongs, electric piano tines and toy piano rods
(1 : 6.27 : 17.5). The thickness and material are the instrument's; the length that puts the
fundamental on the key follows. A mallet (the same felt law as the hammer) meets the bar at
its strike point through the mode shapes there; the bar answers through its modal masses; what
is heard is the bar's acceleration at the radiating point (a bar is small against the
wavelengths it makes, so it radiates as a compact source, which is where a hard mallet's high
modes are heard), through the celesta's box tuned to the note, the dulcitone's board, the tine's
tone bar and a pickup that hears the tine's velocity and whose field makes a hard blow bark, or
a toy piano's plastic case. `bars_tests` holds the mode ratios, every bar
within 3 cents, the glockenspiel ringing for seconds while the celesta's damper stops its plate.

## The organ

A flue pipe is an air jet playing a bore. The wave back from the far end (the walls' and the
open end's losses taken off as the pipe's scale sets, inverted at a stopper) bends the jet on
its way to the labium, a travel time of most of a period in the fundamental's regime, shortened
by harder wind; the jet switches its flow into the pipe on a saturating curve, off-centre so the
even harmonics come. An open pipe sounds every harmonic; a stopped one, half the length, the
odd ones. The pipes are voiced to Töpfer's scale (155 mm at 8' C, halving every 17 semitones),
their end corrections found by playing each once at prepare; wind reaches the foot over about
20 ms with a little overshoot and turbulence (the chiff), the chest sags under many pipes, the
tremulant shakes it, and each pipe stands on the C or C# side of the chest. `pipes_tests` holds
every rank within 8 cents, the gedackt's third harmonic over its second, the tremulant's rate.

## The room and the air

See `src/dsp/air.hpp` (adapted from FUZZBOI): ISO 9613-1 absorption fitted at 8 kHz, the speed
of sound, image sources to second order, an eight-line feedback delay network at the Sabine
time, the microphone's pattern and proximity effect, one microphone or a pair. `air_tests` holds
the delay within a sample, the absorption within 1 dB, Sabine's time within 25%.

## What is not modelled

- Longitudinal (phantom) partials of piano strings, the duplex scale and the soundboard's
  coupling to the case; the strings' nonlinear coupling to each other beyond the bridge.
- The hammer's shank flexibility and the hammer's own vibration; the felt's full hysteresis
  history (Stulov): one hysteresis term stands for it.
- Key noise is a thump through the board, not a model of the key.
- The harpsichord's plectrum is a release deflection, not a bending quill: its own stiffness
  and the string's push on it are not simulated.
- The clavichord's tangent is held rigid; its bounce and the key's compliance are not modelled.
- The hurdy-gurdy's dog is a threshold slap, not a rattling mass; the wheel's cotton is not.
- The bars' mallets strike at one point; their stiffness-dependent contact spectra are the felt
  law's. The electric piano's amplifier and its tremolo are not modelled.
- The organ's jet is one-dimensional: no vortex shedding, no edge tones at the mouth, no
  regime changes beyond overblowing, no reed pipes.
- The soundboard's modes are measured low modes and a generated dense set, not a plate model;
  the board does not radiate directionally.
