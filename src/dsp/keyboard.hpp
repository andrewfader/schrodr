// The string keyboards: pianos, harpsichords, clavichords and the wheel-bowed instruments,
// simulated from their physics rather than imitated. Every note is a course of real strings
// (one, two or three to a note, or an 8' and a 4' register) of a real length, wire and tension,
// over a real bridge on a real soundboard; the action strikes, plucks, presses or bows them.
//
// - The strings are src/dsp/string.hpp waveguides, two planes each. At the bridge every string
//   of a note meets the same junction: the bridge moves with the sum of their pushes, through
//   the bridge's mobility (the soundboard's low modes and a floor rising with frequency), and
//   every string sees that motion. That is Weinreich's coupled piano strings (1977): unison
//   strings moving together drive the soundboard and die fast, moving against each other they
//   leave it still and ring on, and a cent of mistuning between them beats and bends the decay.
//   The soundboard as a whole is a modal bank driven by every bridge; what it does comes back to
//   every undamped string at a transfer coupling (sympathetic resonance) and radiates as sound.
// - A hammer is a mass with a felt (or leather, or paper and leather) whose force grows as a
//   power of its compression (Hall; Chaigne and Askenfelt 1994: F = K u^p, p from 2 to 3 and
//   K from 10^8 to 10^10 N/m^p, hardening up the compass) with a little hysteresis; it meets all
//   the strings of the note, is pushed back by them, and leaves when the strings throw it off.
//   A hard blow compresses the felt more and so is brighter, and the contact is shorter.
// - A plectrum (a jack's quill or delrin) lifts the string until it slips off at a deflection the
//   plectrum's stiffness sets, almost whatever the key's speed: a harpsichord's note is the shape
//   it leaves the string in, let go (the same initial condition as a plucked guitar string).
// - A tangent (a clavichord's brass blade) strikes the string from below and stays, dividing it:
//   the sounding length runs from the tangent to the bridge, the other part is muted by cloth.
//   Pressing harder lifts the string further and stretches it, so a hard stroke sounds sharper
//   and the finger's pressure (bebung) bends the pitch.
// - A rosined wheel bows the string with a stick-slip friction curve (McIntyre, Schumacher and
//   Woodhouse 1983): the hurdy-gurdy's chanterelles and drones, the Geigenwerk's strings.
// - Dampers stop a released string over the damper's time; the sustain pedal lifts them all,
//   the sostenuto those held when it went down; the top notes of a piano have none. Lifted
//   strings whose partials coincide with a struck note's are woken and ring in sympathy.
//
// Sources: Fletcher and Rossing, The Physics of Musical Instruments, chs. 9 (harpsichord and
// clavichord) and 12 (piano); Conklin, "Design and tone in the mechanoacoustic piano" I-III
// (1996); Giordano, Physics of the Piano (2010); Weinreich, "Coupled piano strings" (1977);
// Suzuki, "Vibration and sound radiation of a piano soundboard" (1986); Hall, "Piano string
// excitation" (1986-1992); Chaigne and Askenfelt (1994); Askenfelt and Jansson, "From touch to
// string vibrations" (1990-1993); O'Brien, Ruckers (1990) for Flemish scalings; Kottick, A
// History of the Harpsichord (2003); Brauchli, The Clavichord (1998); Pollens, The Early
// Pianoforte (1995) for Cristofori; Palmer, The Hurdy-Gurdy (1980).
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <complex>
#include <cstdint>
#include <vector>

#include "air.hpp"
#include "core.hpp"
#include "filters.hpp"
#include "string.hpp"
#include "temperament.hpp"

namespace schrodr::dsp::keyboard {

using string::Core;
using string::Physics;

constexpr int kMaxCourses = 3;   // strings to a note: unisons, or the 8' pair and the 4'
constexpr int kLowModes = 10;
constexpr int kDenseModes = 120;
constexpr int kFeedbackDense = 30;

enum class Exciter { Hammer, Plectrum, Tangent, Wheel };

// A soundboard resonance as the bridge sees it: frequency, quality factor and the peak of its
// mobility (velocity over force, s/kg).
struct Mode { double hz, q, mobility; };

// String scaling: the designer's lengths and tensions at a few notes, log-interpolated between.
struct Anchor { int note; double length_m, tension_n; };
constexpr int kMaxAnchors = 8;

struct Spec {
  const char* name;
  Exciter exciter;
  int low, high;                       // the compass, MIDI
  std::array<Anchor, kMaxAnchors> anchors;
  int anchor_count;
  Core treble_core, bass_core;
  int bass_core_below;                 // notes below this take the bass wire (brass, or overspun gut)
  double plain_max_mm;                 // thicker than this and the string is wound on a core
  // Strings to a note by range: `unisons_low` below `double_from`, `unisons_mid` from there,
  // `unisons_high` from `triple_from`. Harpsichords use the registers instead (one string each).
  int unisons_low, unisons_mid, unisons_high, double_from, triple_from;
  // The hammer (or tangent) across the compass: head mass (g), felt stiffness K (N/m^p),
  // exponent p, strike point as a fraction of the speaking length from the agraffe, each
  // interpolated (log for mass, K and the strike point) from the lowest to the highest note.
  double mass_low_g, mass_high_g, k_low, k_high, p_low, p_high, strike_low, strike_high;
  double hammer_hysteresis;            // s/m: F = K u^p (1 + h du/dt)
  double hammer_angle_deg;             // off the normal to the soundboard
  // Plectra: the jacks' distance from the nut (mm) for the back 8', front 8', 4' and lute rows
  // in the treble, growing toward the bass by `jack_skew` of itself at the lowest note; the
  // deflection at which the plectrum slips off (mm); 0 for a muselar-style ratio (`pluck_ratio`).
  double jack_back_mm, jack_front_mm, jack_four_mm, jack_lute_mm, jack_skew, release_mm, pluck_ratio_low, pluck_ratio_high;
  bool has_four_foot;                  // a 4' register with its own strings and bridge
  // The wheel: its radius (m) and where it touches the string, as a fraction from the bridge.
  double wheel_radius_m, wheel_ratio;
  // The soundboard: the low modes, a mobility floor rising by `floor_rise` at 1.5 kHz, then a
  // generated dense set, the radiation's rise (sound pressure follows acceleration below the
  // board's coincidence) and the high-frequency roll-off of a wooden plate.
  std::array<Mode, kLowModes> modes;
  double floor_mobility, floor_rise, dense_from_hz, dense_to_hz, dense_spacing_hz, dense_q, dense_mobility, acceleration_hz, rolloff_hz;
  double sympathetic;                  // transfer coupling through the soundboard
  double polarization_split_cents, horizontal_coupling, plane_exchange;
  int damper_top;                      // notes from here up have no dampers
  double damper_s;                     // time the damper takes to stop a string, 60 dB
  bool sostenuto, una_corda, moderator;
};

inline double interp_log(double a, double b, double t) { return a * std::pow(b / a, std::clamp(t, 0.0, 1.0)); }

// Position of `note` across the compass, 0 at the bottom, 1 at the top.
inline double across(const Spec& s, int note) { return std::clamp(static_cast<double>(note - s.low) / std::max(1, s.high - s.low), 0.0, 1.0); }

// The scaling's length and tension at a note.
inline double anchor_length(const Spec& s, int note) {
  const Anchor* a = s.anchors.data();
  const int n = s.anchor_count;
  if (note <= a[0].note) return a[0].length_m * std::exp2((a[0].note - note) / 12.0 * 0.4);
  if (note >= a[n - 1].note) return a[n - 1].length_m * std::exp2(-(note - a[n - 1].note) / 12.0 * 0.95);
  for (int i = 0; i + 1 < n; ++i)
    if (note >= a[i].note && note <= a[i + 1].note) {
      const double t = static_cast<double>(note - a[i].note) / (a[i + 1].note - a[i].note);
      return interp_log(a[i].length_m, a[i + 1].length_m, t);
    }
  return a[n - 1].length_m;
}
inline double anchor_tension(const Spec& s, int note) {
  const Anchor* a = s.anchors.data();
  const int n = s.anchor_count;
  if (note <= a[0].note) return a[0].tension_n;
  if (note >= a[n - 1].note) return a[n - 1].tension_n;
  for (int i = 0; i + 1 < n; ++i)
    if (note >= a[i].note && note <= a[i + 1].note) {
      const double t = static_cast<double>(note - a[i].note) / (a[i + 1].note - a[i].note);
      return interp_log(a[i].tension_n, a[i + 1].tension_n, t);
    }
  return a[n - 1].tension_n;
}

constexpr int kModelCount = 16;
enum Model : int {
  kGrand, kUpright, kFortepiano, kSquare, kCristofori, kTangent, kFlemish, kItalian, kFrench, kVirginal,
  kSpinet, kLautenwerck, kFrettedClavichord, kUnfrettedClavichord, kHurdyGurdy, kGeigenwerk
};

inline const Spec& spec(int model) {
  static const std::array<Spec, kModelCount> specs = [] {
    std::array<Spec, kModelCount> g{};
    // Soundboards, as bridges see them. A concert grand's: Suzuki's first modes near 50 Hz
    // and the dense set above 200 Hz, mobility about 1e-3 s/kg away from the modes; a
    // fortepiano's thinner, smaller board starts near 100 Hz and gives more readily; a
    // harpsichord's 3 mm board starts above 100 Hz and gives more still; a clavichord's tiny one
    // higher again.
    const std::array<Mode, kLowModes> grand_modes{{{49, 30, 0.010}, {62, 35, 0.007}, {83, 35, 0.006}, {95, 40, 0.005}, {112, 40, 0.005},
                                                   {125, 40, 0.004}, {140, 45, 0.0035}, {160, 45, 0.003}, {178, 45, 0.003}, {195, 45, 0.0025}}};
    const std::array<Mode, kLowModes> fortepiano_modes{{{95, 25, 0.030}, {130, 28, 0.025}, {165, 30, 0.020}, {200, 30, 0.018}, {240, 32, 0.015},
                                                        {290, 32, 0.012}, {340, 35, 0.010}, {400, 35, 0.009}, {470, 35, 0.008}, {540, 35, 0.007}}};
    const std::array<Mode, kLowModes> square_modes{{{120, 25, 0.035}, {160, 28, 0.030}, {210, 30, 0.025}, {260, 30, 0.020}, {310, 32, 0.018},
                                                    {370, 32, 0.015}, {430, 35, 0.012}, {500, 35, 0.010}, {580, 35, 0.009}, {660, 35, 0.008}}};
    const std::array<Mode, kLowModes> flemish_modes{{{105, 20, 0.050}, {140, 22, 0.045}, {180, 25, 0.040}, {215, 25, 0.035}, {260, 28, 0.030},
                                                     {310, 28, 0.026}, {360, 30, 0.022}, {420, 30, 0.019}, {480, 30, 0.017}, {550, 30, 0.015}}};
    const std::array<Mode, kLowModes> french_modes{{{85, 22, 0.045}, {115, 25, 0.040}, {150, 25, 0.035}, {185, 28, 0.030}, {225, 28, 0.027},
                                                    {270, 30, 0.024}, {320, 30, 0.020}, {375, 30, 0.018}, {435, 30, 0.016}, {500, 30, 0.014}}};
    const std::array<Mode, kLowModes> clavichord_modes{{{160, 18, 0.080}, {220, 20, 0.070}, {290, 22, 0.060}, {360, 22, 0.050}, {440, 25, 0.045},
                                                        {520, 25, 0.040}, {600, 25, 0.035}, {700, 28, 0.030}, {800, 28, 0.027}, {900, 28, 0.025}}};
    const std::array<Mode, kLowModes> gurdy_modes{{{190, 22, 0.060}, {260, 25, 0.050}, {330, 25, 0.045}, {400, 28, 0.040}, {480, 28, 0.035},
                                                   {560, 30, 0.030}, {650, 30, 0.027}, {750, 30, 0.024}, {860, 30, 0.020}, {980, 30, 0.018}}};
    const auto scaled = [](const std::array<Mode, kLowModes>& m, double hz, double mobility) {
      std::array<Mode, kLowModes> out = m;
      for (Mode& x : out) { x.hz *= hz; x.mobility *= mobility; }
      return out;
    };
    // A concert grand (Steinway D pattern): 2.0 m bass strings down to 51 mm at C8, tensions near
    // 700 to 950 N (Conklin), music wire to 1.5 mm then copper-wound cores, single strings to E1,
    // pairs to E2, triples above; hammers from 12 g to 4 g, felt K 3e8 to 1.5e10, p 2.2 to 3.0,
    // striking an eighth of the string in the bass and a twelfth at the top; dampers to F#6.
    g[kGrand] = {"Concert grand", Exciter::Hammer, 21, 108,
                 {{{21, 2.00, 950}, {33, 1.75, 900}, {45, 1.35, 850}, {60, 0.62, 700}, {72, 0.345, 720}, {84, 0.19, 750}, {96, 0.098, 760}, {108, 0.051, 700}}}, 8,
                 Core::Steel, Core::Steel, 0, 1.5, 1, 2, 3, 29, 41,
                 12.0, 4.0, 3.0e8, 1.5e10, 2.2, 3.0, 0.125, 0.082, 0.1, 20.0,
                 0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0,
                 grand_modes, 0.0003, 2.0, 210, 9000, 14, 45, 0.002, 400, 4500,
                 0.35, 1.2, 0.25, 0.004, 90, 0.06, true, true, false};
    // An upright (Yamaha U1 pattern): the bass folded to 1.2 m, a smaller board whose modes sit
    // higher and give more, dampers to E6.
    g[kUpright] = {"Upright piano", Exciter::Hammer, 21, 108,
                   {{{21, 1.22, 700}, {33, 1.15, 700}, {45, 1.05, 700}, {60, 0.60, 650}, {72, 0.33, 680}, {84, 0.185, 700}, {96, 0.097, 700}, {108, 0.052, 680}}}, 8,
                   Core::Steel, Core::Steel, 0, 1.5, 1, 2, 3, 33, 45,
                   10.0, 4.0, 4.0e8, 1.2e10, 2.3, 2.9, 0.12, 0.085, 0.1, 20.0,
                   0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0,
                   scaled(grand_modes, 1.25, 1.3), 0.0004, 2.0, 260, 9000, 16, 40, 0.0026, 450, 4000,
                   0.35, 1.2, 0.25, 0.004, 88, 0.08, false, true, false};
    // A Viennese fortepiano (Anton Walter, 1795): FF to g''', iron trebles and brass basses at a
    // tenth of a modern tension, bichord brass bass and trichord iron, leather-covered hammers
    // of a few grams (K 1.5e8 to 1.5e9, p near 2), a thin spruce board, knee levers for the
    // dampers and the moderator.
    g[kFortepiano] = {"Viennese fortepiano", Exciter::Hammer, 29, 91,
                      {{{29, 1.78, 120}, {36, 1.55, 130}, {48, 1.00, 140}, {60, 0.55, 140}, {72, 0.285, 135}, {84, 0.145, 130}, {91, 0.095, 125}}}, 7,
                      Core::Iron, Core::Brass, 45, 1.2, 2, 2, 3, 29, 48,
                      3.5, 1.5, 1.5e8, 1.5e9, 1.8, 2.2, 0.14, 0.09, 0.15, 3.0,
                      0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0,
                      fortepiano_modes, 0.004, 1.6, 600, 8000, 22, 35, 0.006, 500, 5000,
                      0.4, 1.5, 0.15, 0.004, 200, 0.05, false, true, true};
    // An English square piano (Broadwood, 1795): FF to f''', short bichord strings on a small
    // boxy board, tiny leather hammers.
    g[kSquare] = {"Square piano", Exciter::Hammer, 29, 89,
                  {{{29, 1.30, 90}, {36, 1.22, 95}, {48, 0.90, 100}, {60, 0.50, 100}, {72, 0.265, 100}, {84, 0.135, 95}, {89, 0.10, 90}}}, 7,
                  Core::Iron, Core::Brass, 48, 1.0, 1, 2, 2, 36, 200,
                  2.5, 1.2, 1.0e8, 1.0e9, 1.8, 2.1, 0.13, 0.10, 0.15, 3.0,
                  0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0,
                  square_modes, 0.005, 1.5, 720, 7000, 28, 30, 0.007, 600, 4500,
                  0.4, 1.5, 0.15, 0.004, 200, 0.06, false, false, true};
    // Cristofori's gravicembalo col piano e forte (1720): four octaves C to c''', brass and
    // iron at harpsichord tensions, bichord, hammers of rolled paper tipped with leather (the
    // lightest and softest of all), a harpsichord's board, an una corda by shifting the keyboard.
    g[kCristofori] = {"Cristofori 1720", Exciter::Hammer, 36, 84,
                      {{{36, 1.60, 70}, {48, 1.15, 75}, {60, 0.62, 80}, {72, 0.32, 80}, {84, 0.17, 80}}}, 5,
                      Core::Iron, Core::Brass, 55, 0.8, 2, 2, 2, 0, 200,
                      1.5, 0.8, 8.0e7, 6.0e8, 1.7, 2.0, 0.12, 0.09, 0.15, 3.0,
                      0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0,
                      scaled(flemish_modes, 1.05, 0.8), 0.006, 1.5, 700, 7000, 26, 30, 0.008, 600, 4500,
                      0.45, 2.0, 0.15, 0.004, 200, 0.05, false, true, false};
    // A tangent piano (Spaeth and Schmahl, 1790): a fortepiano's strings struck by bare wooden
    // slips thrown at them (hard: K 1e9 to 4e9, p near 1.7) and let fall back, with a moderator.
    g[kTangent] = {"Tangent piano", Exciter::Hammer, 29, 91,
                   {{{29, 1.78, 120}, {36, 1.55, 130}, {48, 1.00, 140}, {60, 0.55, 140}, {72, 0.285, 135}, {84, 0.145, 130}, {91, 0.095, 125}}}, 7,
                   Core::Iron, Core::Brass, 45, 1.2, 2, 2, 2, 0, 200,
                   2.0, 1.0, 1.0e9, 4.0e9, 1.6, 1.8, 0.12, 0.09, 0.05, 3.0,
                   0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0,
                   fortepiano_modes, 0.004, 1.6, 600, 8000, 22, 35, 0.006, 500, 5000,
                   0.4, 1.5, 0.15, 0.004, 200, 0.05, false, false, true};
    // A Flemish harpsichord (Ruckers, 1640): GG to d''', 2 x 8' and 4', c'' at 36 cm (the
    // Flemish scale), iron down to c and yellow brass below, the back 8' jacks 75 mm from the
    // nut and the front 8' 48 mm, the 4' 30 mm; quills releasing at 1.5 mm.
    g[kFlemish] = {"Flemish harpsichord", Exciter::Plectrum, 31, 86,
                   {{{31, 1.75, 55}, {36, 1.68, 60}, {48, 1.30, 62}, {60, 0.72, 62}, {72, 0.36, 62}, {84, 0.18, 58}, {86, 0.165, 56}}}, 7,
                   Core::Iron, Core::Brass, 48, 0.6, 1, 1, 1, 0, 200,
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   75, 48, 42, 12, 0.4, 1.5, 0, 0, true, 0, 0,
                   flemish_modes, 0.008, 1.5, 620, 9000, 24, 28, 0.010, 500, 6000,
                   0.5, 2.5, 0.18, 0.004, 200, 0.04, false, false, false};
    // An Italian harpsichord (17th century, cypress): C to c''', 2 x 8', a short scale strung in
    // brass throughout at low tension, a very thin board: a quick, bright, short-ringing voice.
    g[kItalian] = {"Italian harpsichord", Exciter::Plectrum, 36, 84,
                   {{{36, 1.70, 42}, {48, 1.22, 44}, {60, 0.60, 44}, {72, 0.30, 42}, {84, 0.15, 40}}}, 5,
                   Core::Brass, Core::Brass, 200, 0.7, 1, 1, 1, 0, 200,
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   65, 40, 30, 14, 0.45, 1.4, 0, 0, false, 0, 0,
                   scaled(flemish_modes, 1.1, 1.2), 0.010, 1.5, 680, 9000, 26, 26, 0.012, 550, 6000,
                   0.5, 2.5, 0.18, 0.004, 200, 0.04, false, false, false};
    // A French double (Taskin, 1769): FF to f''', 2 x 8', 4', peau de buffle, lute and buff
    // stops, a larger board with a lower first mode.
    g[kFrench] = {"French harpsichord", Exciter::Plectrum, 29, 89,
                  {{{29, 1.85, 58}, {36, 1.72, 62}, {48, 1.32, 64}, {60, 0.74, 64}, {72, 0.37, 64}, {84, 0.185, 60}, {89, 0.14, 58}}}, 7,
                  Core::Iron, Core::Brass, 48, 0.6, 1, 1, 1, 0, 200,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  78, 50, 32, 15, 0.4, 1.5, 0, 0, true, 0, 0,
                  french_modes, 0.007, 1.5, 560, 9000, 22, 30, 0.009, 450, 6000,
                  0.5, 2.5, 0.18, 0.004, 200, 0.04, false, false, false};
    // A Flemish muselar virginal (Ruckers): C to c''', one 8' plucked near the middle of the
    // string (two fifths of it in the bass, a third in the treble), the whole case a soundboard:
    // a hollow, fluty voice.
    g[kVirginal] = {"Virginal", Exciter::Plectrum, 36, 84,
                    {{{36, 1.60, 56}, {48, 1.25, 60}, {60, 0.70, 60}, {72, 0.35, 60}, {84, 0.175, 56}}}, 5,
                    Core::Iron, Core::Brass, 48, 0.6, 1, 1, 1, 0, 200,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 14, 0.0, 1.5, 0.42, 0.32, false, 0, 0,
                    scaled(flemish_modes, 0.9, 1.1), 0.008, 1.5, 560, 9000, 22, 28, 0.010, 450, 5500,
                    0.45, 2.5, 0.18, 0.004, 200, 0.04, false, false, false};
    // A bentside spinet (English, 1700s): GG to g''', one 8' plucked close to the nut on short
    // strings over a small board: nasal and bright.
    g[kSpinet] = {"Spinet", Exciter::Plectrum, 31, 91,
                  {{{31, 1.45, 45}, {36, 1.38, 48}, {48, 1.10, 50}, {60, 0.60, 52}, {72, 0.30, 52}, {84, 0.15, 50}, {91, 0.10, 48}}}, 7,
                  Core::Iron, Core::Brass, 48, 0.6, 1, 1, 1, 0, 200,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  0, 45, 0, 14, 0.4, 1.4, 0, 0, false, 0, 0,
                  scaled(flemish_modes, 1.3, 1.2), 0.010, 1.5, 800, 9000, 30, 26, 0.012, 650, 6000,
                  0.45, 2.5, 0.18, 0.004, 200, 0.04, false, false, false};
    // A Lautenwerck (gut-strung harpsichord, the kind Bach owned): C to c''', 2 x 8' of gut,
    // overspun in the bass, plucked with soft leather: a lute's quick decay and warmth.
    g[kLautenwerck] = {"Lautenwerck", Exciter::Plectrum, 36, 84,
                       {{{36, 1.40, 40}, {48, 1.05, 42}, {60, 0.58, 45}, {72, 0.29, 45}, {84, 0.15, 42}}}, 5,
                       Core::Gut, Core::Gut, 0, 1.2, 1, 1, 1, 0, 200,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       80, 50, 30, 14, 0.4, 2.0, 0, 0, false, 0, 0,
                       scaled(flemish_modes, 1.1, 1.0), 0.009, 1.5, 680, 8000, 26, 26, 0.011, 550, 5000,
                       0.5, 3.0, 0.2, 0.004, 200, 0.05, false, false, false};
    // A fretted clavichord (German, 1750): C to d''', pairs of brass strings at a few kilograms'
    // tension, chromatic neighbours sharing a pair (C# and D, D# and E, F# and G, G# and A,
    // A# and B), struck by brass tangents that stay on the string; the quietest instrument there
    // is, on the smallest board.
    g[kFrettedClavichord] = {"Fretted clavichord", Exciter::Tangent, 36, 86,
                             {{{36, 1.30, 22}, {48, 1.00, 24}, {60, 0.55, 26}, {72, 0.28, 26}, {86, 0.12, 24}}}, 5,
                             Core::Brass, Core::Brass, 200, 0.6, 2, 2, 2, 0, 200,
                             15.0, 15.0, 2.0e9, 2.0e9, 1.5, 1.5, 0.02, 0.02, 0.05, 2.0,
                             0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0,
                             clavichord_modes, 0.012, 1.4, 1000, 8000, 35, 25, 0.015, 700, 5000,
                             0.4, 2.0, 0.2, 0.004, 200, 0.01, false, false, false};
    // An unfretted clavichord (Hass, 1760): FF to f''', a pair to every key, a larger case.
    g[kUnfrettedClavichord] = {"Unfretted clavichord", Exciter::Tangent, 29, 89,
                               {{{29, 1.55, 24}, {36, 1.45, 26}, {48, 1.10, 28}, {60, 0.58, 28}, {72, 0.29, 28}, {84, 0.15, 26}, {89, 0.12, 24}}}, 7,
                               Core::Brass, Core::Brass, 200, 0.6, 2, 2, 2, 0, 200,
                               15.0, 15.0, 2.0e9, 2.0e9, 1.5, 1.5, 0.02, 0.02, 0.05, 2.0,
                               0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0,
                               scaled(clavichord_modes, 0.85, 0.9), 0.011, 1.4, 850, 8000, 32, 25, 0.014, 650, 5000,
                               0.4, 2.0, 0.2, 0.004, 200, 0.01, false, false, false};
    // A hurdy-gurdy (vielle a roue, G/C): two gut chanterelles tuned g' and shortened by the
    // keys' tangents over two octaves, bowed by a rosined wheel of 75 mm radius a sixth of the
    // way from the bridge, with drones (gros bourdon C, petit bourdon c, trompette c' on its
    // buzzing dog) on a lute-like body.
    g[kHurdyGurdy] = {"Hurdy-gurdy", Exciter::Wheel, 67, 91,
                      {{{67, 0.345, 40}, {91, 0.0862, 40}}}, 2,
                      Core::Gut, Core::Gut, 0, 1.2, 2, 2, 2, 0, 200,
                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                      0, 0, 0, 0, 0, 0, 0, 0, false, 0.075, 0.17,
                      gurdy_modes, 0.010, 1.4, 1100, 8000, 30, 25, 0.012, 700, 5000,
                      0.4, 2.5, 0.2, 0.004, 0, 0.2, false, false, false};
    // A Geigenwerk (Hans Haiden, 1575): C to c''', single iron and brass strings a harpsichord's
    // length, each pressed by its key onto one of five rosined wheels: a bowed keyboard.
    g[kGeigenwerk] = {"Geigenwerk", Exciter::Wheel, 36, 84,
                      {{{36, 1.35, 40}, {48, 1.00, 42}, {60, 0.55, 45}, {72, 0.28, 45}, {84, 0.14, 42}}}, 5,
                      Core::Iron, Core::Brass, 50, 0.6, 1, 1, 1, 0, 200,
                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                      0, 0, 0, 0, 0, 0, 0, 0, false, 0.09, 0.2,
                      scaled(flemish_modes, 1.1, 1.0), 0.009, 1.5, 680, 8000, 26, 26, 0.011, 550, 5500,
                      0.45, 2.5, 0.18, 0.004, 200, 0.05, false, false, false};
    return g;
  }();
  return specs[static_cast<size_t>(std::clamp(model, 0, kModelCount - 1))];
}

// Which pair of strings a clavichord key's tangent falls on: on a fretted instrument the
// chromatic note shares its natural neighbour's pair (and cannot sound together with it).
inline int clavichord_course_key(int model, int key) {
  if (model != kFrettedClavichord) return key;
  const int pc = ((key % 12) + 12) % 12;
  return (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10) ? key + 1 : key;
}

// The hammer that plays a note.
struct Hammer {
  double mass = 0.006;     // kg
  double k = 1e9;          // N/m^p
  double p = 2.5;
  double strike = 0.12;    // fraction of the speaking length from the agraffe
  double hysteresis = 0.1; // s/m
};

inline Hammer hammer_at(const Spec& s, int note, double hardness, double strike_scale) {
  const double t = across(s, note);
  Hammer h;
  h.mass = interp_log(s.mass_low_g, s.mass_high_g, t) * 1e-3;
  // Voicing: a hard, packed felt is stiffer, a needled one softer: over the knob K spans a
  // factor of thirty either way of the maker's.
  h.k = interp_log(s.k_low, s.k_high, t) * std::pow(10.0, 3.0 * (std::clamp(hardness, 0.0, 1.0) - 0.5));
  h.p = lerp(s.p_low, s.p_high, t);
  h.strike = std::clamp(interp_log(s.strike_low, s.strike_high, t) * strike_scale, 0.02, 0.5);
  h.hysteresis = s.hammer_hysteresis;
  return h;
}

// The string set of one note on one instrument: the course's length, wire and tension. `course`
// counts unisons (or, on a harpsichord, 0 the back 8', 1 the front 8', 2 the 4').
inline Physics string_at(const Spec& s, int model, int note, int course, double hz, double temperature_c) {
  (void)model;
  const bool four = s.has_four_foot && course == 2;
  const double length = anchor_length(s, note) * (four ? 0.5 : 1.0);
  const double tension = anchor_tension(s, note) * (four ? 0.85 : 1.0);
  const double f = hz * (four ? 2.0 : 1.0);
  const double mu = tension / std::pow(2.0 * length * f, 2.0);
  const Core core = note < s.bass_core_below ? s.bass_core : s.treble_core;
  const string::Material m = string::material(core);
  const double plain_d = std::sqrt(4.0 * mu / (kPi * m.density));
  const double plain_max = s.plain_max_mm * 1e-3;
  double core_d = plain_d;
  bool wound = false;
  double wrap_loss = 1.0;
  if (plain_d > plain_max) {
    // Overspun: a core thinner than the thickest plain wire carries the stiffness, the winding
    // the rest of the mass. Deeper in the bass the core grows a little; the winding's turns rub.
    wound = true;
    const double deep = std::clamp(std::log(plain_d / plain_max) / std::log(3.0), 0.0, 1.0);
    core_d = plain_max * (0.62 + 0.25 * deep);
    wrap_loss = 2.5;
  }
  // Strings tuned at 20 C: a warm room slackens a steel string against its iron frame or wooden
  // case by E A (alpha_string - alpha_frame) dT, a cent or so per degree.
  Physics p = string::physics(length, f, core_d, mu, core, wound, wrap_loss);
  const double alpha = core == Core::Gut || core == Core::Nylon ? 8.0e-5 : (core == Core::Brass ? 1.9e-5 : 1.17e-5);
  const double frame = model <= kUpright ? 1.2e-5 : 3.8e-6;
  const double tuned = p.tension;
  p.tension = std::max(0.2 * p.tension, p.tension - p.ea * (alpha - frame) * (temperature_c - 20.0));
  p.hz *= std::sqrt(p.tension / tuned);
  p.impedance = std::sqrt(p.tension * p.mu);
  p.inharmonicity = kPi * kPi * p.ei / (p.tension * p.length * p.length);
  return p;
}

// The player's settings for a block.
struct Setup {
  int model = kGrand;
  double a4 = 440.0;
  int temperament = 0, root = 0;
  double stretch = 1.0;
  double unison_cents = 1.0;
  double hardness = 0.5, strike_scale = 1.0, plectrum = 0.4, mechanics = 0.5;
  double release_s = 0.08;
  double bebung_cents = 12.0;
  double wheel_turns = 1.0, wheel_pressure = 0.5, trompette = 0.5;
  bool drones = true;
  bool back8 = true, front8 = false, four = false, lute = false, buff = false;
  double sustain = 0.0;   // 0..1, the pedal's travel (half-pedalling lets the dampers graze)
  bool sostenuto = false;
  double una_corda = 0.0, moderator = 0.0, sympathetic = 1.0;
  double humidity = 0.45, celsius = 20.0;
  int debug = 0;  // probe only
};

}  // namespace schrodr::dsp::keyboard

namespace schrodr::dsp::keyboard {

// --- the instrument -----------------------------------------------------------------------------------

class Keyboard {
 public:
  static constexpr int kFirstKey = 21, kLastKey = 108;
  static constexpr int kKeys = kLastKey - kFirstKey + 1;
  static constexpr int kDroneCount = 3;
  static constexpr int kSlots = kKeys + kDroneCount;
  // The most notes that sound at once: beyond this the oldest released note is stopped, as a
  // real-time budget (a concert grand with the pedal down can ring every string; this cannot).
  static constexpr int kMaxSounding = 28;
  static constexpr int kMaxSympathetic = 8;

  struct Course {
    string::Plane v, h;
    Physics physics;
    string::LoopDesign rv, rh;
    double hz = 100.0, base_hz = 100.0, period = 480.0, upper_delay = 240.0, upper_delay_h = 240.0;
    double nut_frac = 0.0, nut_frac_h = 0.0;  // the loop's fractional delay, read at the nut
    double string_delay = 0.0, string_delay_h = 0.0;  // the string's own filters' share of the loop
    double filter_delay = 0.0, filter_delay_h = 0.0, delay_hz = 0.0;
    size_t lower_len = 240;
    float nut_gain = 1.0f, nut_pole = 0.0f;
    double ratio = 1.0, tuned_ratio = -1.0, tension_ratio = 1.0, tension_target = 1.0, elongation = 0.0;
    int elongation_clock = 0;
    double pluck_mm = 0.0;        // where this course's jack plucks, mm from the nut
    double detune_cents = 0.0;    // the tuner's unison setting
    bool active = true;           // the register is drawn, the string present
    int junction = 0;             // 0 the note's bridge, 1 the 4' bridge
    bool designed = false;
    float filtered_v = 0.0f, filtered_h = 0.0f, env = 0.0f;
    // Exciter state: the cumulative displacement a force has pushed in, the hammer's compression
    // and force last sample, the string's displacement at the bow last sample.
    double hammer_u = 0.0, hammer_f = 0.0, bow_y = 0.0, bow_dv = 0.0, bow_s = 0.0;
    float excite_level = 0.0f; int excite_left = 0; double excite_point = 0.0;
    int pluck_in = 0;
    // A tangent string let go: the bridge hears it fade over the key's return while the listing
    // cloth takes it, instead of the plateau the tangent held jumping into the bridge at once.
    float fade = 1.0f, fade_step = 0.0f; int fade_left = 0;
    OnePole excite_tone;
  };
  struct Junction {
    std::array<string::Resonator, kLowModes> bank{};
    std::array<float, kLowModes> gamma{};
    float floor = 0.0f, floor_rise = 0.0f, prev = 0.0f, hp_state = 0.0f;
    double z_sum = 1.0;         // the impedances of the strings on it, summed
  };
  struct Note {
    int key = -1;                 // the key it answers to (a drone's is its own pitch)
    int courses = 0;
    std::array<Course, kMaxCourses> course{};
    std::array<Junction, 2> junction{};
    bool held = false, damping = false, free = false, sostenuto = false, sympathetic = false, awake = false, bowed = false;
    float env = 0.0f;
    double damp_s = 0.06;
    // The hammer in flight: its position relative to the strings' rest, velocity, whether it is a
    // tangent that stays, how long it touched and how hard.
    bool hammer_active = false, hammer_hold = false;
    double hammer_y = 0.0, hammer_v = 0.0, hammer_x = 0.0;
    std::array<double, kMaxCourses> hammer_k_scale{};
    Hammer hammer{};
    int contact_samples = 0, hammer_age = 0;
    double peak_force = 0.0;
    double lift_ratio = 1.0;      // a clavichord tangent's stretch of the string
    double pluck_velocity = 0.0;
    uint32_t seed = 0;
    double pan = 0.0;
    double bow_force = 0.0;       // ramps in as the string meets the wheel
    int coup_left = 0;            // samples of the trompette's buzz after a hard key
    bool dog_up = false;          // the trompette's dog is lifted off the board
    uint64_t age = 0;
    float prev_force = 0.0f;
  };

  void prepare(double rate) {
    rate_ = rate;
    bow_lag_ = 1.0 - std::exp(-1.0 / (0.0002 * rate));
    for (int i = 0; i < kSlots; ++i) {
      Note& n = notes_[static_cast<size_t>(i)];
      n.key = i < kKeys ? kFirstKey + i : 0;
      const double hz = i < kKeys ? midi_to_hz(n.key - 2.5) : midi_to_hz(33.0);
      const size_t capacity = static_cast<size_t>(rate / hz) + 16;
      for (auto& c : n.course) { c.v.prepare(capacity); c.h.prepare(capacity); c.excite_tone.set(2500.0, rate); }
    }
    shape_.prepare(static_cast<size_t>(rate / midi_to_hz(33.0)) + 16);
    env_fall_ = static_cast<float>(std::exp(-1.0 / (0.05 * rate)));
    tension_glide_ = 1.0 - std::exp(-1.0 / (0.002 * rate));
    thump_tone_.set(300.0, rate);
    click_tone_.set(3000.0, rate);
    thump_fall_ = static_cast<float>(std::exp(-1.0 / (0.012 * rate)));
    click_fall_ = static_cast<float>(std::exp(-1.0 / (0.004 * rate)));
    dc_[0].set(10.0, rate); dc_[1].set(10.0, rate);
    configured_ = false;
    configure(setup_);
    reset();
  }

  void reset() {
    for (auto& n : notes_) quiet(n);
    held_keys_.fill(false);
    held_count_ = 0;
    for (auto& b : board_) b.reset();
    feedback_.reset();
    feedback_out_ = 0.0f;
    for (auto& f : accel_) f.reset();
    for (auto& f : rolloff_) f.reset();
    for (auto& d : dc_) d.reset();
    thump_level_ = click_level_ = 0.0f;
    board_env_ = 0.0f;
    crank_left_ = 0;
    aftertouch_ = 0.0;
    bend_ = 1.0;
    age_counter_ = 0;
    wheel_phase_ = 0.0;
  }

  // The player's settings for this block. A new instrument, pitch, temperament or room
  // temperature re-strings and re-tunes the whole instrument; ringing strings keep their pitch
  // until they are played again (idle ones take the new tuning at once).
  void configure(const Setup& in) {
    Setup s = in;
    s.model = std::clamp(s.model, 0, kModelCount - 1);
    s.a4 = std::clamp(s.a4, 380.0, 480.0);
    s.temperament = std::clamp(s.temperament, 0, temperament::kCount - 1);
    s.root = std::clamp(s.root, 0, 11);
    s.stretch = std::clamp(s.stretch, 0.0, 1.0);
    s.humidity = std::clamp(s.humidity, 0.0, 1.0);
    s.celsius = std::clamp(s.celsius, -20.0, 50.0);
    const bool restring = !configured_ || s.model != setup_.model || s.a4 != setup_.a4 || s.temperament != setup_.temperament ||
                          s.root != setup_.root || s.stretch != setup_.stretch || s.unison_cents != setup_.unison_cents ||
                          s.celsius != setup_.celsius || s.back8 != setup_.back8 || s.front8 != setup_.front8 || s.four != setup_.four ||
                          s.lute != setup_.lute || s.buff != setup_.buff;
    const bool reboard = !configured_ || s.model != setup_.model || s.humidity != setup_.humidity || s.celsius != setup_.celsius;
    const Setup before = setup_;
    setup_ = s;
    if (restring) restring_all();
    if (reboard) build_board();
    if (restring || reboard)
      for (auto& n : notes_) if (!n.awake) for (int c = 0; c < n.courses; ++c) if (n.course[static_cast<size_t>(c)].designed) tune(n, n.course[static_cast<size_t>(c)], true);
    if (configured_) pedals(before);
    configured_ = true;
  }

  const Setup& setup() const { return setup_; }
  const Spec& spec() const { return keyboard::spec(setup_.model); }
  int low() const { return spec().low; }
  int high() const { return spec().high; }

  // --- playing ---

  // A key goes down at `velocity` (0..1, the player's touch through the dynamics curve).
  void note_on(int key, double velocity, uint32_t seed) {
    const Spec& g = spec();
    key = into_range(key);
    if (key < 0) return;
    velocity = std::clamp(velocity, 0.0, 1.0);
    // Two keys an octave apart can fold onto one of this instrument's: the count follows the
    // keys that are down, not the presses, or a hurdy-gurdy would crank for ever.
    if (!held_keys_[static_cast<size_t>(key)]) { held_keys_[static_cast<size_t>(key)] = true; ++held_count_; }
    if (g.exciter == Exciter::Wheel && setup_.model == kHurdyGurdy) {
      crank_left_ = static_cast<int>(rate_ * 0.6);
      press_tangent(velocity);
      return;
    }
    Note& n = note_for(key);
    rng_.seed(seed ^ 0x6a09e667u);
    n.seed = seed;
    n.age = ++age_counter_;
    budget_sounding();
    // A fretted clavichord's shared pair cannot play two notes: the other key's note is choked.
    if (setup_.model == kFrettedClavichord)
      for (int other = key - 1; other <= key + 1; ++other)
        if (other != key && other >= g.low && other <= g.high && clavichord_course_key(setup_.model, other) == clavichord_course_key(setup_.model, key)) {
          Note& o = note_for(other);
          if (o.awake) { o.held = false; damp(o, 0.01); }
        }
    if (!n.awake) wake(n);
    n.held = true; n.damping = false; n.free = false; n.sympathetic = false;
    // A re-struck ringing note: the hammer meets the moving string; whatever it is doing
    // carries on under the new blow. The dampers lift again.
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      if (!co.designed) design(n, c);
      co.nut_gain = 1.0f; co.nut_pole = 0.0f;
      if (g.exciter == Exciter::Tangent) { co.nut_gain = 0.9995f; co.nut_pole = 0.02f; }
      co.fade = 1.0f; co.fade_left = 0;
    }
    switch (g.exciter) {
      case Exciter::Hammer: strike(n, velocity, false); break;
      case Exciter::Tangent: strike(n, velocity, true); break;
      case Exciter::Plectrum: schedule_pluck(n, velocity); break;
      case Exciter::Wheel: n.bowed = true; n.bow_force = 0.0; break;
    }
    // The key reaching its bed: a soft thud through the case, harder the harder the key is hit.
    thump_level_ += static_cast<float>(setup_.mechanics * velocity * velocity * 0.4);
    if (setup_.sustain >= 0.5 || n.key >= g.damper_top) wake_sympathetic(n);
  }

  void note_off(int key) {
    const Spec& g = spec();
    key = into_range(key);
    if (key < 0) return;
    if (held_keys_[static_cast<size_t>(key)]) { held_keys_[static_cast<size_t>(key)] = false; if (held_count_ > 0) --held_count_; }
    if (g.exciter == Exciter::Wheel && setup_.model == kHurdyGurdy) {
      // The player keeps cranking a while after the last key: the open chanterelle and the
      // drones sound on until the wheel is let go.
      crank_left_ = static_cast<int>(rate_ * 0.6);
      release_tangent();
      return;
    }
    Note& n = note_for(key);
    if (!n.awake || !n.held) return;
    n.held = false;
    if (g.exciter == Exciter::Wheel) { n.bowed = false; }
    if (n.hammer_hold) { n.hammer_active = false; n.hammer_hold = false; }
    if (g.exciter == Exciter::Plectrum) {
      // The jack falls: its plectrum brushes the string on the way down (a touch, the tongue
      // gives), then the damper lands and the jack hits its rail.
      for (int c = 0; c < n.courses; ++c) {
        Course& co = n.course[static_cast<size_t>(c)];
        if (!co.active) continue;
        lay_pluck(n, co, 0.06, 0.5);
      }
      click_level_ += static_cast<float>(setup_.mechanics * 0.5);
    } else if (g.exciter == Exciter::Tangent) {
      click_level_ += static_cast<float>(setup_.mechanics * 0.25);
    } else {
      thump_level_ += static_cast<float>(setup_.mechanics * 0.12);
    }
    if (n.sostenuto || (setup_.sustain >= 0.5 && g.damper_top > 0) || key >= g.damper_top) { n.free = true; return; }
    damp(n, release_time(n));
  }

  // A tangent leaves its string: the key returns in about ten milliseconds, the string
  // following it down onto the listing cloth, so what the bridge hears from it fades over that.
  void let_go(Note& n) {
    const int samples = std::max(1, static_cast<int>(rate_ * 0.010));
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      if (!co.active || co.fade_left > 0) continue;
      co.fade_left = samples;
      co.fade_step = co.fade / static_cast<float>(samples);
    }
  }

  void all_notes_off() {
    for (auto& n : notes_) if (n.awake && n.held) { n.held = false; n.bowed = false; if (!n.free) damp(n, release_time(n)); }
    held_keys_.fill(false);
    held_count_ = 0;
  }
  void all_sound_off() { reset(); }

  // Channel pressure (0..1): a clavichord's bebung, a hurdy-gurdy's coup de poignet.
  void aftertouch(double amount) { aftertouch_ = std::clamp(amount, 0.0, 1.0); }
  void set_bend(double ratio) { bend_ = std::clamp(ratio, 0.5, 2.0); }

  bool awake() const {
    for (const auto& n : notes_) if (n.awake) return true;
    return board_env_ > 1e-7f || thump_level_ > 1e-5f || click_level_ > 1e-5f;
  }
  int sounding() const { int k = 0; for (const auto& n : notes_) k += n.awake; return k; }
  bool held(int key) const { return key >= 0 && key < 128 && held_keys_[static_cast<size_t>(key)]; }

  // --- inspection (tests, the critic) ---
  const Note& note(int key) const { return notes_[static_cast<size_t>(std::clamp(key - kFirstKey, 0, kKeys - 1))]; }
  const Physics& physics(int key, int course = 0) const { return note(key).course[static_cast<size_t>(std::clamp(course, 0, kMaxCourses - 1))].physics; }
  // The pitch the tuner left a key's first string at, after temperament and stretch.
  double pitch_at(int key, int course = 0) const { return note(key).course[static_cast<size_t>(std::clamp(course, 0, kMaxCourses - 1))].physics.hz; }
  double inharmonicity_at(int key) const { return note(key).course[0].physics.inharmonicity; }
  Hammer hammer_at(int key) const { return keyboard::hammer_at(spec(), key, setup_.hardness, setup_.strike_scale); }
  int courses_at(int key) const { return note(key).courses; }
  double last_contact_s() const { return last_contact_ / rate_; }
  double last_peak_force() const { return last_peak_force_; }
  bool note_damped(int key) const { return note(key).damping; }
  bool note_sounding(int key) const { return note(key).awake; }
  bool note_sympathetic(int key) const { return note(key).sympathetic; }
  std::complex<double> bridge_reflection(int key, double hz) const { return junction_response(note(key).junction[0], kTau * hz / rate_); }
  // How long the longest-ringing undamped string takes to fall 60 dB, for the host's tail.
  double longest_decay_s() const {
    const Spec& g = spec();
    double t = 0.5;
    for (int key = g.low; key <= g.high; key += 7) {
      const Physics& p = note(key).course[0].physics;
      const double bridge = 2.0 * p.impedance * g.floor_mobility * p.hz;
      t = std::max(t, 6.91 / std::max(string::own_rate(p, p.hz) + bridge, 1e-3));
    }
    return std::min(t, 40.0);
  }

  // One stereo sample of what the soundboard radiates.
  void render(float& left, float& right) {
    const Spec& g = spec();
    left = right = 0.0f;
    if (crank_left_ > 0) --crank_left_;
    const bool cranking = setup_.model == kHurdyGurdy && (held_count_ > 0 || crank_left_ > 0);
    if (setup_.model == kHurdyGurdy) {
      wheel_phase_ += setup_.wheel_turns / rate_;
      if (wheel_phase_ >= 1.0) wheel_phase_ -= 1.0;
      for (int d = 0; d < kDroneCount; ++d) {
        Note& n = notes_[static_cast<size_t>(kKeys + d)];
        const bool want = cranking && setup_.drones;
        if (want && !n.awake) { wake(n); n.held = true; }
        n.bowed = want;
        if (!want && n.awake && n.held) { n.held = false; damp(n, 0.25); }
      }
      Note& ch = chanterelle();
      if (cranking && !ch.awake) { wake(ch); ch.held = true; }
      ch.bowed = cranking;
      if (!cranking && ch.awake && ch.held) { ch.held = false; damp(ch, 0.25); }
    }
    float force_l = 0.0f, force_r = 0.0f, feedback_in = 0.0f;
    const float feedback = feedback_out_ * static_cast<float>(setup_.sympathetic);
    bool any = false;
    for (int i = 0; i < kSlots; ++i) {
      Note& n = notes_[static_cast<size_t>(i)];
      if (!n.awake) continue;
      any = true;
      tick(n, g, feedback, force_l, force_r, feedback_in);
    }
    // Mechanical noises: the key bed's thud and the jacks' click, through the case.
    if (thump_level_ > 1e-6f || click_level_ > 1e-6f) {
      const float thump = thump_tone_.low(rng_.bipolar()) * thump_level_;
      const float click = click_tone_.high(rng_.bipolar()) * click_level_ * 0.3f;
      thump_level_ *= thump_fall_; click_level_ *= click_fall_;
      force_l += (thump + click) * 0.02f; force_r += (thump + click) * 0.02f;
    } else { thump_level_ = click_level_ = 0.0f; }
    // The soundboard: a modal bank per channel (the two microphones see the modes at different
    // levels), radiating as acceleration below the board's coincidence and rolling off above
    // a wooden plate's reach; and the mono bank whose motion feeds every string back.
    float out_l = board_[0].process(force_l), out_r = board_[1].process(force_r);
    feedback_out_ = feedback_.process(feedback_in);
    out_l = rolloff_[0].low(accel_[0].high(out_l));
    out_r = rolloff_[1].low(accel_[1].high(out_r));
    left = dc_[0].process(out_l * static_cast<float>(output_scale_));
    right = dc_[1].process(out_r * static_cast<float>(output_scale_));
    const float level = std::max(std::abs(left), std::abs(right));
    board_env_ = std::max(level, board_env_ * env_fall_);
    if (!any && board_env_ < 1e-7f) {
      // Everything has rung out: exact zeros, no denormals.
      for (auto& b : board_) b.reset();
      feedback_.reset();
      feedback_out_ = 0.0f;
    }
  }

 private:
  static constexpr float kQuiet = 2e-9f;  // metres: a couple of nanometres is silence

  int into_range(int key) const {
    const Spec& g = spec();
    if (key < 0 || key > 127) return -1;
    while (key < g.low) key += 12;
    while (key > g.high) key -= 12;
    if (key < g.low) return -1;
    return key;
  }
  Note& note_for(int key) { return notes_[static_cast<size_t>(std::clamp(key - kFirstKey, 0, kKeys - 1))]; }
  Note& chanterelle() { return notes_[static_cast<size_t>(67 - kFirstKey)]; }

  // --- stringing and tuning ---

  // Every note's strings from the scaling, and the tuner's pass: the temperament on the
  // reference, then the octaves stretched by the strings' own inharmonicity as a tuner does by
  // ear (4:2 octaves, 6:3 in the bass), scaled by the Stretch setting.
  void restring_all() {
    const Spec& g = spec();
    std::array<double, 128> b{};
    std::array<double, 128> et{}, tuned{};
    for (int key = g.low; key <= g.high; ++key) {
      et[static_cast<size_t>(key)] = temperament::hz(setup_.temperament, setup_.root, setup_.a4, key);
      const Physics p = string_at(g, setup_.model, key, 0, et[static_cast<size_t>(key)], setup_.celsius);
      b[static_cast<size_t>(key)] = p.inharmonicity;
    }
    // The stretch of partial n of a string with inharmonicity b over a harmonic one.
    const auto stretch = [&](int key, int n) { return std::sqrt((1.0 + b[static_cast<size_t>(key)] * n * n) / (1.0 + b[static_cast<size_t>(key)])); };
    // The temperament octave, C#4 to C5, is laid as the temperament says, A4 on the fork.
    for (int key = 61; key <= 72; ++key) if (key >= g.low && key <= g.high) tuned[static_cast<size_t>(key)] = et[static_cast<size_t>(key)];
    // Upward, 4:2: the lower note's fourth partial on the upper's second.
    for (int key = 73; key <= g.high; ++key) {
      const int lower = key - 12;
      const double base = lower >= g.low && tuned[static_cast<size_t>(lower)] > 0.0 ? tuned[static_cast<size_t>(lower)] : et[static_cast<size_t>(lower)];
      tuned[static_cast<size_t>(key)] = base * 2.0 * stretch(lower, 4) / stretch(key, 2) * et[static_cast<size_t>(key)] / (2.0 * et[static_cast<size_t>(lower)]);
    }
    // Downward, 4:2 in the tenor and 6:3 in the bass: the lower note's fourth (sixth) partial on
    // the upper's second (third).
    for (int key = 60; key >= g.low; --key) {
      const int upper = key + 12;
      const double base = upper <= g.high && tuned[static_cast<size_t>(upper)] > 0.0 ? tuned[static_cast<size_t>(upper)] : et[static_cast<size_t>(upper)];
      const int n = key >= 36 ? 4 : 6;
      tuned[static_cast<size_t>(key)] = 0.5 * base * stretch(upper, n / 2) / stretch(key, n) * et[static_cast<size_t>(key)] / (0.5 * et[static_cast<size_t>(upper)]);
    }
    for (int key = g.low; key <= g.high; ++key) {
      const double e = et[static_cast<size_t>(key)], t = tuned[static_cast<size_t>(key)] > 0.0 ? tuned[static_cast<size_t>(key)] : e;
      const double hz = e * std::pow(t / e, setup_.stretch);
      Note& n = note_for(key);
      n.key = key;
      restring(n, key, hz);
    }
    if (setup_.model == kHurdyGurdy) {
      static constexpr int kDroneKeys[kDroneCount] = {36, 48, 60};
      for (int d = 0; d < kDroneCount; ++d) {
        Note& n = notes_[static_cast<size_t>(kKeys + d)];
        n.key = kDroneKeys[d];
        restring_drone(n, d, temperament::hz(setup_.temperament, setup_.root, setup_.a4, kDroneKeys[d]));
      }
    } else {
      for (int d = 0; d < kDroneCount; ++d) { Note& n = notes_[static_cast<size_t>(kKeys + d)]; if (n.awake) quiet(n); n.courses = 0; }
    }
  }

  // The courses of one key: its unison strings, or its registers.
  void restring(Note& n, int key, double hz) {
    const Spec& g = spec();
    n.pan = 2.0 * across(g, key) - 1.0;
    n.damp_s = g.damper_s;
    if (g.exciter == Exciter::Plectrum) {
      n.courses = 2 + (g.has_four_foot ? 1 : 0);
      const double skew = 1.0 + g.jack_skew * (1.0 - across(g, key));
      const double length = anchor_length(g, key);
      for (int c = 0; c < n.courses; ++c) {
        Course& co = n.course[static_cast<size_t>(c)];
        co.junction = c == 2 ? 1 : 0;
        co.active = c == 0 ? (setup_.back8 || g.jack_back_mm == 0.0) : c == 1 ? (setup_.front8 || setup_.lute) : setup_.four;
        if (g.jack_back_mm == 0.0 && g.jack_front_mm == 0.0) {
          // A muselar: one string plucked at a fraction of its length.
          co.active = c == 0;
          co.pluck_mm = lerp(g.pluck_ratio_low, g.pluck_ratio_high, across(g, key)) * length * 1000.0;
        } else if (g.jack_back_mm == 0.0) {
          co.active = c == 0;  // a spinet: one 8', the front row's jacks
          co.pluck_mm = g.jack_front_mm * skew;
        } else {
          co.pluck_mm = (c == 0 ? g.jack_back_mm : c == 1 ? (setup_.lute ? g.jack_lute_mm : g.jack_front_mm) : g.jack_four_mm) * skew;
        }
        co.detune_cents = c == 2 ? 0.0 : (c == 0 ? -0.5 : 0.5) * setup_.unison_cents * 0.5;
        co.physics = string_at(g, setup_.model, key, c, hz, setup_.celsius);
        co.designed = false;
        if (!n.awake) tune(n, co, true);
      }
      return;
    }
    const int unisons = key >= g.triple_from ? g.unisons_high : key >= g.double_from ? g.unisons_mid : g.unisons_low;
    n.courses = std::clamp(unisons, 1, kMaxCourses);
    Rng r(0x51e7u + static_cast<uint32_t>(key) * 977u);
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      co.junction = 0;
      co.active = true;
      co.pluck_mm = 0.0;
      // The tuner's unisons: a pair sits either side of true, a triple has its middle string on
      // it; and no two strings are left exactly where the tuner meant.
      const double spread = n.courses == 1 ? 0.0 : (n.courses == 2 ? (c == 0 ? -0.5 : 0.5) : (c - 1) * 0.5);
      co.detune_cents = setup_.unison_cents * (spread + 0.3 * r.bipolar());
      const int course_key = g.exciter == Exciter::Tangent ? clavichord_course_key(setup_.model, key) : key;
      co.physics = string_at(g, setup_.model, course_key, c, hz, setup_.celsius);
      if (g.exciter == Exciter::Tangent) {
        // The tangent of this key stops its pair at the length that sounds the key: the pair's
        // physics are the course's (wire and tension), the speaking length the key's.
        co.physics.length = anchor_length(g, key);
        co.physics.inharmonicity = kPi * kPi * co.physics.ei / (co.physics.tension * co.physics.length * co.physics.length);
      }
      // physics.hz is the key's pitch as the room's temperature leaves it (string_at was asked
      // for `hz` at 20 C): the tangent's stopping changes the length, not the tuning.
      co.designed = false;
      if (!n.awake) tune(n, co, true);
    }
  }

  // The hurdy-gurdy's drones: gros bourdon, petit bourdon and trompette on their own lengths.
  void restring_drone(Note& n, int which, double hz) {
    const Spec& g = spec();
    static constexpr double kLengths[kDroneCount] = {0.46, 0.46, 0.40};
    n.courses = 1;
    n.pan = which == 2 ? 0.35 : -0.35 - 0.2 * (1 - which);
    n.damp_s = 0.25;
    Course& co = n.course[0];
    co.junction = 0; co.active = true; co.pluck_mm = 0.0; co.detune_cents = 0.0;
    const double tension = 35.0, length = kLengths[which];
    const double mu = tension / std::pow(2.0 * length * hz, 2.0);
    const double plain = std::sqrt(4.0 * mu / (kPi * string::material(Core::Gut).density));
    const bool wound = plain > 1.2e-3;
    co.physics = string::physics(length, hz, wound ? 0.9e-3 : plain, mu, Core::Gut, wound, wound ? 2.0 : 1.0);
    // Gut against the instrument's wooden body in a warm room, as every other string.
    const double tuned = co.physics.tension;
    co.physics.tension = std::max(0.2 * tuned, tuned - co.physics.ea * (8.0e-5 - 3.8e-6) * (setup_.celsius - 20.0));
    co.physics.hz *= std::sqrt(co.physics.tension / tuned);
    co.physics.impedance = std::sqrt(co.physics.tension * co.physics.mu);
    co.designed = false;
    if (!n.awake) tune(n, co, true);
    (void)g;
  }

  // Sets a course's speaking length and pitch and the loop around it. `split` re-divides the
  // loop between the rails (a fresh note); otherwise only the rail into the bridge changes
  // length and the waves on the string travel on (a tangent moving, a chanterelle re-keyed).
  void tune(Note& n, Course& co, bool split) {
    co.base_hz = co.physics.hz * cents_ratio(co.detune_cents);
    co.hz = co.base_hz;
    co.period = rate_ / co.hz;
    co.tuned_ratio = -1.0;
    co.delay_hz = 0.0;
    if (split) {
      // The loop's filters (the string's losses and stiffness, the bridge's phase) are lumped at
      // the bridge; what is left of the period is the string's cells, half on each rail, so the
      // two rails agree on where every point of the string is.
      const double ratio = co.ratio * co.tension_ratio;
      refresh_delays(n, co, std::clamp(co.base_hz * ratio, 15.0, rate_ * 0.45));
      co.lower_len = static_cast<size_t>(std::max(2.0, std::floor(0.5 * (rate_ / std::clamp(co.base_hz * ratio, 15.0, rate_ * 0.45) - 1.0 - co.filter_delay))));
    }
    retune(n, co, co.ratio * co.tension_ratio);
  }
  // The filters' phase delays move slowly with frequency: they are recomputed when the pitch
  // has moved a few cents since they were, not on every sample of a tension glide.
  void refresh_delays(Note& n, Course& co, double hz) {
    if (!(co.delay_hz <= 0.0 || std::abs(hz / co.delay_hz - 1.0) > 2e-4)) return;
    const Spec& g = spec();
    const double w = kTau * hz / rate_;
    const double bridge = junction_delay(n.junction[static_cast<size_t>(co.junction)], w);
    co.string_delay = string::loop_delay(co.rv, w);
    co.string_delay_h = string::loop_delay(co.rh, w);
    co.filter_delay = co.string_delay + bridge;
    co.filter_delay_h = co.string_delay_h + bridge * g.horizontal_coupling;
    co.delay_hz = hz;
  }

  // Fits the loop filters of a course to its physics: the string's own losses (and a buff
  // stop's leather or a moderator's cloth) at every partial in the band, and its stiffness.
  void design(Note& n, int c) {
    const Spec& g = spec();
    Course& co = n.course[static_cast<size_t>(c)];
    const Physics& p = co.physics;
    const double f0 = co.base_hz;
    const int count = std::clamp(static_cast<int>(std::min(12000.0, rate_ * 0.45) / f0), 2, 48);
    const double buff = (setup_.buff && co.junction == 0 && g.exciter == Exciter::Plectrum) ? 40.0 : 0.0;
    // The string's own losses are the same in both planes (the bridge's, which differ, live in
    // the junction), so one fit serves both.
    {
      std::array<double, 48> rates{};
      for (int k = 1; k <= count; ++k) rates[static_cast<size_t>(k - 1)] = string::own_rate(p, k * f0, buff);
      string::fit_loss_curve(co.rv, f0, rates.data(), count, rate_);
      co.rh.gain = co.rv.gain; co.rh.pole = co.rv.pole; co.rh.zero = co.rv.zero; co.rh.lp = co.rv.lp;
    }
    // The parallel plane's own phase: two broad, weak allpass resonances at frequencies set by
    // the string (the same every time it is played), so the partials' polarization splits
    // scatter by a cent or two around the mean split.
    {
      Rng r(0x51e7u + static_cast<uint32_t>(n.key) * 977u + static_cast<uint32_t>(c) * 31u + static_cast<uint32_t>(setup_.model) * 131u);
      const double radius = std::clamp(0.0004 * rate_ / f0, 0.02, 0.35);
      for (int k = 0; k < string::kSplitSections; ++k) {
        const double hz = 400.0 * std::pow(8.0, r.unipolar());
        const double theta = kTau * std::min(hz, rate_ * 0.4) / rate_;
        co.rh.split[static_cast<size_t>(k)] = {-2.0 * radius * std::cos(theta), radius * radius};
      }
    }
    string::LoopDesign fit = co.rv;
    string::fit_dispersion(fit, f0, p.inharmonicity, rate_, 3.0);
    co.rv.allpass = co.rh.allpass = fit.allpass;
    co.rv.sections = co.rh.sections = fit.sections;
    co.v.design(co.rv);
    co.h.design(co.rh);
    co.designed = true;
    co.tuned_ratio = -1.0;
    co.delay_hz = 0.0;
    tune(n, co, !n.awake);
  }

  // The bridge's reflection as this course's loop sees it, at w: the inversion less the bridge's
  // own motion (the floor with its rise, the low modes at their coupling).
  std::complex<double> junction_response(const Junction& j, double w) const {
    const std::complex<double> z1 = std::polar(1.0, -w);
    const std::complex<double> hp = 1.0 - (1.0 - static_cast<double>(floor_hp_)) / (1.0 - static_cast<double>(floor_hp_) * z1);
    std::complex<double> y = static_cast<double>(j.floor) * (1.0 + static_cast<double>(j.floor_rise) * hp);
    for (int m = 0; m < kLowModes; ++m) y += static_cast<double>(j.gamma[static_cast<size_t>(m)]) * j.bank[static_cast<size_t>(m)].response(w);
    // Per unit of a string's own wave: the junction's input carries every string's impedance.
    return -(1.0 - y * j.z_sum);
  }
  // Samples of delay the bridge's reflection adds at w beyond a plain inversion: what a tuner
  // tunes out when the soundboard's mass or spring pulls the string's pitch.
  double junction_delay(const Junction& j, double w) const {
    const std::complex<double> r = junction_response(j, w);
    double phase = std::arg(r);  // -pi for a rigid bridge
    if (phase > 0.0) phase -= kTau;
    return -(phase + kPi) / w;
  }

  void retune(Note& n, Course& co, double ratio) {
    const Spec& g = spec();
    const double lowest = rate_ / (2.0 * static_cast<double>(co.v.upper().capacity()) - 16.0);
    co.hz = std::clamp(co.base_hz * ratio, std::max(15.0, lowest), rate_ * 0.45);
    co.period = rate_ / co.hz;
    refresh_delays(n, co, co.hz);
    const double half_split = cents_ratio(0.5 * g.polarization_split_cents);
    double upper = co.period / half_split - 1.0 - static_cast<double>(co.lower_len) - co.filter_delay;
    if (upper < 1.5) {
      co.lower_len = static_cast<size_t>(std::max(2.0, std::floor(0.5 * (co.period - 1.0 - co.filter_delay))));
      upper = co.period / half_split - 1.0 - static_cast<double>(co.lower_len) - co.filter_delay;
    }
    // The bridge reads a whole cell of the upper rail; the loop's fraction of a cell is read at
    // the nut, so lower slot j is exactly j cells from the bridge and an exciter's push and
    // reading land on the same point of the string.
    upper = std::max(1.0, upper);
    co.upper_delay = std::floor(upper);
    co.nut_frac = upper - co.upper_delay;
    const double upper_h = std::max(1.0, co.period * half_split - 1.0 - static_cast<double>(co.lower_len) - co.filter_delay_h);
    co.upper_delay_h = std::floor(upper_h);
    co.nut_frac_h = upper_h - co.upper_delay_h;
    co.tuned_ratio = ratio;
  }

  // --- the soundboard ---

  void build_board() {
    const Spec& g = spec();
    const air::WoodBody wood = air::wood_body(setup_.humidity, setup_.celsius);
    const double z_ref = 2.5;
    // The feedback bank's input is impedance-weighted too: a mode's factor per unit of Z y+.
    const auto gamma_of = [&](double mobility) { const double zy = z_ref * mobility; return 2.0 * zy / (1.0 + zy) / z_ref; };
    for (int ch = 0; ch < 2; ++ch) {
      Rng r(0xb0d1e5u + static_cast<uint32_t>(setup_.model) * 7u + static_cast<uint32_t>(ch) * 101u);
      string::ModalBank<kLowModes + kDenseModes>& bank = board_[static_cast<size_t>(ch)];
      bank.clear();
      for (const Mode& m : g.modes) {
        // Each microphone sees a mode by the board's shape where it stands: levels scatter by
        // a few dB between the two, differently for every mode.
        const double level = m.mobility * (0.7 + 0.6 * r.unipolar());
        bank.add(m.hz * wood.frequency_ratio, m.q * wood.q_ratio, level * wood.q_ratio, rate_);
      }
      for (double f = g.dense_from_hz; f < g.dense_to_hz && bank.count() < kLowModes + kDenseModes; f += g.dense_spacing_hz * 1.6 * (0.6 + 0.8 * r.unipolar())) {
        const double roll = 1.0 / std::sqrt(1.0 + std::pow(f / g.rolloff_hz, 2.0));
        const double scatter = 0.3 + 0.7 * r.unipolar();
        const double sign = r.unipolar() < 0.5 ? -1.0 : 1.0;
        bank.add(f * wood.frequency_ratio, g.dense_q * (0.7 + 0.6 * r.unipolar()) * wood.q_ratio, g.dense_mobility * roll * scatter * sign * wood.q_ratio, rate_);
      }
    }
    {
      Rng r(0xfeedb0a7u + static_cast<uint32_t>(setup_.model));
      feedback_.clear();
      // Transfer mobility between two points of a board is about half the driving point's.
      for (const Mode& m : g.modes) feedback_.add(m.hz * wood.frequency_ratio, m.q * wood.q_ratio, gamma_of(0.5 * m.mobility * wood.q_ratio), rate_);
      for (double f = g.dense_from_hz; f < g.dense_to_hz && feedback_.count() < kLowModes + kFeedbackDense; f += (g.dense_to_hz - g.dense_from_hz) / kFeedbackDense * (0.6 + 0.8 * r.unipolar()))
        feedback_.add(f * wood.frequency_ratio, g.dense_q * wood.q_ratio, gamma_of(0.3 * g.dense_mobility * wood.q_ratio) * (r.unipolar() < 0.5 ? -1.0 : 1.0), rate_);
    }
    for (int ch = 0; ch < 2; ++ch) {
      accel_[static_cast<size_t>(ch)].set(g.acceleration_hz, rate_);
      rolloff_[static_cast<size_t>(ch)].set(g.rolloff_hz * 2.0, rate_);
    }
    output_scale_ = 14.0;
    // Every note's bridge junction: the low modes at their driving-point mobility against the
    // note's own impedance, exactly for the floor (2 Y / (1 + Y Z)).
    for (auto& n : notes_)
      for (auto& j : n.junction) {
        double z_sum = 0.0;
        for (int c = 0; c < n.courses; ++c) if (n.course[static_cast<size_t>(c)].junction == (&j - n.junction.data())) z_sum += n.course[static_cast<size_t>(c)].physics.impedance;
        if (z_sum <= 0.0) z_sum = z_ref;
        j.z_sum = z_sum;
        const double floor = g.floor_mobility;
        j.floor = static_cast<float>(2.0 * floor / (1.0 + floor * z_sum));
        j.floor_rise = static_cast<float>(g.floor_rise - 1.0);
        for (int m = 0; m < kLowModes; ++m) {
          const Mode& mode = g.modes[static_cast<size_t>(m)];
          j.bank[static_cast<size_t>(m)].set(mode.hz * wood.frequency_ratio, mode.q * wood.q_ratio, 1.0, rate_);
          // The junction's input is impedance-weighted (sum of Z y+), so the mode's factor per
          // unit of wave, 2 Z Y / (1 + Z Y), is applied per unit of Z y+.
          const double zy = z_sum * mode.mobility * wood.q_ratio;
          j.gamma[static_cast<size_t>(m)] = static_cast<float>(2.0 * zy / (1.0 + zy) / z_sum);
        }
      }
    floor_hp_ = static_cast<float>(std::exp(-kTau * 700.0 / rate_));
  }

  // --- pedals ---

  void pedals(const Setup& before) {
    const Spec& g = spec();
    const bool was_down = before.sustain >= 0.5, down = setup_.sustain >= 0.5;
    if (down && !was_down) {
      // The dampers lift: every sounding note is free, and the strings tuned to what is sounding
      // start to ring with it.
      for (auto& n : notes_) if (n.awake && !n.held && n.damping) { n.damping = false; n.free = true; undamp(n); }
      for (auto& n : notes_) if (n.awake && !n.sympathetic) wake_sympathetic(n);
    } else if (!down && was_down) {
      for (auto& n : notes_) if (n.awake && !n.held && !n.sostenuto && n.key < g.damper_top && n.key > 0) { n.free = false; damp(n, n.sympathetic ? 0.5 * release_time(n) : release_time(n)); }
    } else if (down && setup_.sustain < 0.85 && setup_.sustain != before.sustain) {
      // Half-pedalling: the dampers graze the strings and take the top end off them.
      for (auto& n : notes_) if (n.awake && !n.held && n.free) for (int c = 0; c < n.courses; ++c) n.course[static_cast<size_t>(c)].nut_pole = static_cast<float>(0.4 * (0.85 - setup_.sustain) / 0.35);
    }
    if (setup_.sostenuto && !before.sostenuto) { for (auto& n : notes_) if (n.awake && n.held) n.sostenuto = true; }
    if (!setup_.sostenuto && before.sostenuto) {
      for (auto& n : notes_) if (n.sostenuto) { n.sostenuto = false; if (n.awake && !n.held && !down) { n.free = false; damp(n, release_time(n)); } }
    }
    // The pedal's own thump, through the case.
    if (down != was_down) thump_level_ += static_cast<float>(setup_.mechanics * 0.3);
  }

  double release_time(const Note& n) const {
    // Dampers are slower in the bass, where the felt wedges meet thick wound strings.
    const double bass = n.key < 60 ? 1.0 + (60 - n.key) / 40.0 : 1.0;
    return std::max(0.005, setup_.release_s * (n.damp_s / 0.08) * bass);
  }

  void damp(Note& n, double seconds) {
    n.damping = true;
    n.free = false;
    n.bowed = false;
    n.damp_s = std::max(0.005, seconds);
    // A tangent string is not stopped at the nut: the tangent drops and the listing cloth takes
    // the whole string, so it fades at the bridge over the key's return (let_go).
    if (spec().exciter == Exciter::Tangent) { let_go(n); return; }
    apply_damping(n);
  }
  void undamp(Note& n) {
    for (int c = 0; c < n.courses; ++c) { Course& co = n.course[static_cast<size_t>(c)]; co.nut_gain = 1.0f; co.nut_pole = spec().exciter == Exciter::Tangent ? 0.02f : 0.0f; }
  }
  // Damping by the damper's felt (or the listing cloth, or the jack's flag): the loop loses
  // 60 dB in damp_s, the felt taking the top end first.
  void apply_damping(Note& n) {
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      const double g = std::pow(1e-3, 1.0 / std::max(1.0, n.damp_s * co.hz));
      co.nut_gain = std::min(co.nut_gain, static_cast<float>(g));
      co.nut_pole = std::max(co.nut_pole, 0.35f);
    }
  }

  void wake(Note& n) {
    n.env = kQuiet * 4.0f;
    n.damping = false;
    n.age = ++age_counter_;
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      co.env = 0.0f;
      co.hammer_u = co.hammer_f = 0.0; co.bow_y = 0.0; co.bow_dv = 0.0; co.bow_s = 0.8;
      co.fade = 1.0f; co.fade_left = 0;
      // Designed while still asleep, so the loop is split between the rails with its filters'
      // delay known (a sounding note is never re-split: its waves travel on).
      if (!co.designed) design(n, c);
    }
    n.awake = true;
  }

  // A string that has rung out: exact zeros (no denormals), its dampers at rest.
  void quiet(Note& n) {
    n.awake = n.held = n.damping = n.free = n.sympathetic = n.bowed = n.hammer_active = n.hammer_hold = false;
    n.env = 0.0f; n.prev_force = 0.0f; n.coup_left = 0; n.dog_up = false;
    for (auto& co : n.course) {
      co.v.clear(); co.h.clear();
      co.env = 0.0f; co.filtered_v = co.filtered_h = 0.0f;
      co.tension_ratio = co.tension_target = 1.0; co.elongation = 0.0; co.ratio = 1.0;
      co.excite_left = 0; co.pluck_in = 0; co.hammer_u = co.hammer_f = 0.0; co.bow_y = 0.0; co.bow_dv = 0.0; co.bow_s = 0.8;
      co.nut_gain = 1.0f; co.nut_pole = 0.0f; co.fade = 1.0f; co.fade_left = 0;
      if (co.designed) { co.tuned_ratio = -1.0; tune(n, co, true); }
    }
    for (auto& j : n.junction) { for (auto& r : j.bank) r.reset(); j.prev = 0.0f; j.hp_state = 0.0f; }
  }

  // The real-time budget: with too many notes sounding, the oldest released one is stopped.
  void budget_sounding() {
    while (sounding() >= kMaxSounding) {
      Note* oldest = nullptr;
      for (auto& n : notes_) if (n.awake && !n.held && (!oldest || n.age < oldest->age)) oldest = &n;
      if (!oldest) for (auto& n : notes_) if (n.awake && (!oldest || n.age < oldest->age)) oldest = &n;
      if (!oldest) return;
      quiet(*oldest);
    }
  }

  // Strings whose partials coincide with the struck note's, among those whose dampers are up,
  // are set ringing in sympathy (woken, so the soundboard's motion reaches them).
  void wake_sympathetic(Note& struck) {
    const Spec& g = spec();
    if (setup_.sympathetic <= 0.0 || struck.courses == 0) return;
    const double f0 = struck.course[0].base_hz;
    const double b0 = struck.course[0].physics.inharmonicity;
    struct Candidate { Note* note; double score; };
    std::array<Candidate, kKeys> best{};
    int count = 0;
    for (int key = g.low; key <= g.high; ++key) {
      Note& n = note_for(key);
      if (&n == &struck || n.awake || n.courses == 0) continue;
      const bool lifted = setup_.sustain >= 0.5 || key >= g.damper_top || n.sostenuto;
      if (!lifted) continue;
      const double f1 = n.course[0].base_hz, b1 = n.course[0].physics.inharmonicity;
      double score = 0.0;
      for (int a = 1; a <= 8; ++a) {
        const double fa = string::partial_hz(f0, b0, a);
        for (int c = 1; c <= 6; ++c) {
          const double fc = string::partial_hz(f1, b1, c);
          const double cents = std::abs(1200.0 * std::log2(fc / fa));
          if (cents < 20.0) score += (20.0 - cents) / (a * c);
        }
      }
      if (score > 1.0 && count < kKeys) best[static_cast<size_t>(count++)] = {&n, score};
    }
    std::sort(best.begin(), best.begin() + count, [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
    int woken = 0;
    for (int i = 0; i < count && woken < kMaxSympathetic && sounding() < kMaxSounding - 2; ++i) {
      Note& n = *best[static_cast<size_t>(i)].note;
      wake(n);
      n.sympathetic = true; n.free = true; n.held = false;
      undamp(n);
      ++woken;
    }
  }

  // --- the exciters ---

  // The hammer (or tangent) is thrown at the strings: it starts touching the lowest of them and
  // rises at the key's speed into the felt's compression.
  void strike(Note& n, double velocity, bool hold) {
    const Spec& g = spec();
    n.hammer = keyboard::hammer_at(g, n.key, setup_.hardness, setup_.strike_scale);
    // Hammer speed from touch: about half a metre a second for a whisper to six for a blow
    // (Askenfelt and Jansson). A tangent is driven by the finger and moves more slowly.
    double speed = hold ? 0.2 + 1.5 * velocity : 0.3 + 5.0 * std::pow(velocity, 1.6);
    // The moderator's cloth between hammer and string softens the blow and takes much of it:
    // the felt is as good as thirty times softer, and the cloth's drag costs a third of the
    // hammer's speed.
    const double moderator = g.moderator ? std::clamp(setup_.moderator, 0.0, 1.0) : 0.0;
    n.hammer.k *= 1.0 - 0.97 * moderator;
    speed *= 1.0 - 0.35 * moderator;
    const double una = g.una_corda ? std::clamp(setup_.una_corda, 0.0, 1.0) : 0.0;
    // Una corda shifts the action so the hammer meets a softer, unworn part of the felt and,
    // on a triple, misses the third string (on a pair, grazes the second).
    n.hammer.k *= 1.0 - 0.45 * una;
    for (int c = 0; c < kMaxCourses; ++c) n.hammer_k_scale[static_cast<size_t>(c)] = 1.0;
    if (n.courses == 3) n.hammer_k_scale[2] = 1.0 - una;
    else if (n.courses == 2) n.hammer_k_scale[1] = 1.0 - 0.6 * una;
    n.hammer_x = n.hammer.strike;
    double y_min = 1e9;
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      const double x = std::clamp(std::floor(n.hammer_x * (co.upper_delay + 0.5 * co.string_delay)), 1.0, co.upper_delay - 1.0);
      y_min = std::min(y_min, static_cast<double>(co.v.at_point(x, static_cast<size_t>(co.upper_delay))));
      co.hammer_u = 0.0; co.hammer_f = 0.0;
    }
    n.hammer_y = y_min - 1e-6;
    n.hammer_v = speed * (1.0 + 0.03 * rng_.bipolar());
    n.hammer_active = true;
    n.hammer_hold = hold;
    n.contact_samples = 0; n.hammer_age = 0; n.peak_force = 0.0;
    // A tangent lifts the string onto itself and stretches it: a hard stroke sounds sharper.
    if (hold) {
      const Physics& p = n.course[0].physics;
      const double h = (0.3 + 3.3 * velocity) * 1e-3;
      const double dead = 0.05 + 0.2 * p.length;
      const double stretch = 0.5 * h * h * (1.0 / dead + 1.0 / p.length);
      n.lift_ratio = std::sqrt(1.0 + p.ea * stretch / (p.tension * (p.length + dead)));
    } else n.lift_ratio = 1.0;
  }

  void step_hammer(Note& n) {
    const Hammer& h = n.hammer;
    const double dt = 1.0 / rate_;
    double total = 0.0, y_min = 1e9;
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      if (!co.active) continue;
      const double x = std::clamp(std::floor(n.hammer_x * (co.upper_delay + 0.5 * co.string_delay)), 1.0, co.upper_delay - 1.0);
      const double y_in = co.v.at_point(x, static_cast<size_t>(co.upper_delay));
      y_min = std::min(y_min, y_in);
      const double u0 = n.hammer_y - y_in;
      if (u0 <= 0.0) { co.hammer_u = 0.0; co.hammer_f = 0.0; continue; }
      // The felt's force at this compression, with the string pushed by that force within the
      // sample (a point force moves its point at F / 2Z): solved implicitly, safeguarded.
      const double z = co.physics.impedance;
      const double comp = dt / (2.0 * z);
      const double udot = (u0 - co.hammer_u) * rate_;
      const double hyst = std::clamp(1.0 + h.hysteresis * udot, 0.3, 3.0);
      const double k = h.k * n.hammer_k_scale[static_cast<size_t>(c)];
      if (k <= 0.0) { co.hammer_u = u0; co.hammer_f = 0.0; continue; }
      double lo = 0.0, hi = u0 / comp;
      double f = std::clamp(co.hammer_f > 0.0 ? co.hammer_f : k * std::pow(u0, h.p) * hyst, lo, hi);
      // Newton from the last sample's force, bisection whenever a step leaves the bracket. The
      // residual is the test: an iterate that lands on the root exactly must not be mistaken for
      // one outside the bracket and bisected away from.
      for (int it = 0; it < 24; ++it) {
        const double u = u0 - comp * f;
        const double gval = f - k * std::pow(std::max(u, 0.0), h.p) * hyst;
        if (std::abs(gval) <= 1e-7 * std::max(1.0, f)) break;
        if (gval > 0.0) hi = f; else lo = f;
        const double dg = 1.0 + k * h.p * std::pow(std::max(u, 1e-12), h.p - 1.0) * hyst * comp;
        double next = f - gval / dg;
        if (!(next >= lo && next <= hi)) next = 0.5 * (lo + hi);
        if (std::abs(next - f) < 1e-9 * std::max(1.0, f)) { f = next; break; }
        f = next;
      }
      co.hammer_u = u0 - comp * f;
      co.hammer_f = f;
      total += f;
      push_force(co, x, static_cast<float>(f * comp), spec().hammer_angle_deg);
    }
    n.hammer_v -= total / h.mass * dt;
    n.hammer_y += n.hammer_v * dt;
    ++n.hammer_age;
    if (total > 0.0) { ++n.contact_samples; n.peak_force = std::max(n.peak_force, total); }
    const bool leaving = total <= 0.0 && n.hammer_v < 0.0;
    if (leaving && (n.hammer_hold || n.hammer_y < y_min - 2e-5)) {
      // Thrown off (or, a tangent, held where it is by the finger): the blow is over.
      n.hammer_active = false;
      last_contact_ = n.contact_samples;
      last_peak_force_ = n.peak_force;
    }
    if (n.hammer_age > static_cast<int>(rate_ * 0.05)) n.hammer_active = false;  // never met the string
  }

  // A force at `x` cells from the nut has pushed `delta` of displacement into a course: the
  // wave it launches sits behind its fronts on both rails. In displacement waves that is a
  // plateau: the rail heading away from the nearer end carries +delta on the cells between that
  // end and the point, the other rail -delta on the cells beyond the point, so the sum is delta
  // at the point and nothing where the wave has not yet reached. The push is laid so that it
  // lands on the point at the next sample, when the force has acted over the sample: one cell
  // short on the rail heading away from the point's side, one cell on along the other, as the
  // rails will have moved by then. Laid where the point is now, it would reach a cell further
  // than the wave it answers, and an exciter reading the point back would see its own push as
  // a spike coming the other way.
  void push_force(Course& co, double x, float delta, double angle_deg) {
    const float dv = delta * static_cast<float>(std::cos(angle_deg * kPi / 180.0));
    const float dh = delta * static_cast<float>(std::sin(angle_deg * kPi / 180.0));
    const size_t xi = static_cast<size_t>(x);
    const size_t bridge = static_cast<size_t>(co.upper_delay), bridge_h = static_cast<size_t>(co.upper_delay_h);
    // Each plateau's far block runs to an end of the string, standing for a wave that has been
    // arriving there all along: that end's filters are primed with it, or they would answer it
    // only after their own delay and leave a notch that long behind every push.
    if (x <= 0.5 * co.upper_delay) {
      plateau_from_nut(co.v, xi, bridge, co.lower_len, dv);
      co.v.prime_nut_dc(-dv);
      if (dh != 0.0f) { plateau_from_nut(co.h, xi, bridge_h, co.lower_len, dh); co.h.prime_nut_dc(-dh); }
    } else {
      plateau_from_bridge(co.v, xi, bridge, dv);
      co.v.prime_bridge_dc(-dv);
      if (dh != 0.0f) { plateau_from_bridge(co.h, xi, bridge_h, dh); co.h.prime_bridge_dc(-dh); }
    }
  }
  // Upper cells 0..x-1 (at the point and nutward next sample) and lower cells from the point's
  // own (nutward of it next sample) to the nut.
  static void plateau_from_nut(string::Plane& p, size_t x, size_t bridge, size_t lower_len, float delta) {
    for (size_t i = 0; i + 1 <= x; ++i) p.upper().slot(i) += delta;
    for (size_t i = std::min(lower_len, bridge - std::min(bridge, x)); i <= lower_len; ++i) p.lower().slot(i) -= delta;
  }
  // Lower cells from the bridge to one short of the point (at the point and bridgeward next
  // sample) and upper cells from the point's own to the bridge (beyond it next sample).
  static void plateau_from_bridge(string::Plane& p, size_t x, size_t bridge, float delta) {
    const size_t to = bridge - std::min(bridge, x);
    for (size_t i = 0; i + 1 <= to; ++i) p.lower().slot(i) += delta;
    for (size_t i = x; i <= bridge; ++i) p.upper().slot(i) -= delta;
  }

  // The jack rises at the key's speed; its plectrum reaches the string a few milliseconds on, a
  // little later on each course (the jacks' heights differ), and later still from a slow key.
  void schedule_pluck(Note& n, double velocity) {
    n.pluck_velocity = velocity;
    const double key_speed = 0.06 + 0.5 * velocity;
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      if (!co.active) { co.pluck_in = 0; continue; }
      // The rows are regulated to pluck in turn (back 8', then the 4', then the front 8') so the
      // key does not meet every quill at once: about a millimetre of travel between them.
      const double travel = 0.0015 + 0.0004 * rng_.unipolar() + (c == 1 ? 0.0010 : c == 2 ? 0.0005 : 0.0);
      co.pluck_in = std::max(1, static_cast<int>(rate_ * travel / key_speed));
    }
  }

  // The plectrum slips off: the string is let go from the shape the quill left it in, lifted
  // `release` of the plectrum's design deflection at the jack's point, rounded by the quill's
  // width and how quickly it lets go (a soft quill rolls off, delrin snaps).
  void lay_pluck(Note& n, Course& co, double release, double scale) {
    const Spec& g = spec();
    const double length = co.physics.length;
    // The string's length in cells: the rail's, and the half of the filters' travel at the bridge.
    const double half = co.upper_delay + 0.5 * co.string_delay;
    const double where = std::clamp(co.pluck_mm * 1e-3 * (1.0 + 0.01 * rng_.bipolar()), 0.004, length - 0.004);
    const double hardness = std::clamp(setup_.plectrum * (setup_.model == kLautenwerck ? 0.5 : 1.0), 0.0, 1.0);
    const double amplitude = g.release_mm * 1e-3 * release * (0.92 + 0.16 * n.pluck_velocity) * (0.85 + 0.3 * hardness) * scale;
    const double contact = lerp(1.2e-3, 0.5e-3, hardness);
    // A quill lets go in a few hundredths of a millisecond; soft leather (peau de buffle) lets
    // the string slide off it over a third of a millisecond, rounding the corner it leaves.
    const double slip_s = 0.03e-3 * (1.0 + 10.0 * (1.0 - hardness) * (1.0 - hardness));
    const double c = 2.0 * length * co.hz;
    const double width = std::min(contact + c * slip_s, length / 6.0);
    const double angle = (62.0 + 6.0 * rng_.bipolar()) * kPi / 180.0;
    // The front 8' row's jacks face the other way, so its quills pull the string the other way.
    const double side = (&co == &n.course[1] && g.jack_back_mm > 0.0) ? -1.0 : 1.0;
    const double point = (length - where) / length * half;  // samples from the bridge
    shape_.build(half, point, width / length * half);
    shape_.lay(co.v, co.upper_delay, co.string_delay, side * amplitude * std::sin(angle));
    shape_.lay(co.h, co.upper_delay_h, co.string_delay_h, side * amplitude * std::cos(angle));
    co.excite_left = static_cast<int>(rate_ * lerp(0.0025, 0.0012, hardness));
    co.excite_level = static_cast<float>(amplitude * 0.006 * (0.3 + hardness));
    co.excite_point = where / length * half;
    co.excite_tone.set(lerp(1500.0, 4000.0, hardness), rate_);
    co.env = std::max(co.env, static_cast<float>(amplitude * 0.5));
    co.elongation = 0.0; co.elongation_clock = 0;
    measure_elongation(co);
    co.tension_ratio = co.tension_target;
  }

  // The rosined wheel at the string: stick while the string can follow the rim, slip on the
  // falling friction curve when it cannot (McIntyre, Schumacher and Woodhouse), the force on
  // the string found where the curve meets the waveguide's load line.
  void step_bow(Note& n) {
    const Spec& g = spec();
    const double dt = 1.0 / rate_;
    const double rim = kTau * g.wheel_radius_m * setup_.wheel_turns;
    // The wheel is never quite round nor evenly rosined: its pressure wobbles once a turn and
    // the rosin's grain adds a grit the player hears as the wheel's texture.
    const double wobble = 1.0 + 0.08 * std::sin(kTau * wheel_phase_) + 0.02 * rng_.bipolar();
    const double target = (0.25 + 1.3 * std::clamp(setup_.wheel_pressure, 0.0, 1.0)) * wobble;
    n.bow_force += (target - n.bow_force) * std::min(1.0, dt / 0.02);
    const double fb = n.bow_force;
    constexpr double mu_s = 0.8, mu_d = 0.25, v0 = 0.2;
    const auto mu = [&](double dv) { return mu_d + (mu_s - mu_d) * v0 / (v0 + dv); };
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      if (!co.active) continue;
      // On a whole cell: a plateau is laid in whole cells, and the point must read back exactly
      // what it pushed in, or the bow sees a string that is not there.
      const double x = std::clamp(std::floor((1.0 - g.wheel_ratio) * (co.upper_delay + 0.5 * co.string_delay)), 1.0, co.upper_delay - 1.0);
      // The string's velocity under the wheel over the coming sample from the waves about to
      // arrive from either side (the cells one step away on each rail), before any push: known
      // now, so the contact is solved without a sample's delay, which would make the hold an
      // active element at the top of the band.
      const size_t bridge = static_cast<size_t>(co.upper_delay);
      const double y_now = co.v.at_point(x, bridge);
      const double y_next = co.v.upper().linear(std::max(0.0, x - 1.0)) + co.v.lower().linear(std::max(0.0, static_cast<double>(bridge) - x - 1.0));
      const double v_h = (y_next - y_now) * rate_;
      const double z = co.physics.impedance;
      const double lag = rim - v_h;
      // The rim drags the string along while the pull that takes is within the static limit;
      // past it the string slides on friction falling with the excess lag (one curve over the
      // lag behind the rim: the single-valued, thermal picture of rosin, Smith and Woodhouse 2000).
      // Rosin's friction is a thermal thing (Smith and Woodhouse 2000): the falling curve is
      // what it settles to, and it settles over a fraction of a millisecond. Taken at once, the
      // fall would be a negative resistance under the wheel, amplifying every wave that passes
      // through a sliding contact at the frequencies the string hardly damps; followed with
      // its lag, the string slides on friction that cannot flicker, as the real contact does.
      const double hold = mu_s * fb;
      const double needed = 2.0 * z * lag;
      double f;
      if (std::abs(needed) <= hold) {
        f = needed;
        co.bow_dv = 0.0;
        co.bow_s += (mu_s - co.bow_s) * bow_lag_;  // cooling back toward the static friction
      } else {
        const double excess = (std::abs(needed) - hold) / (2.0 * z);
        co.bow_s += (mu(excess) - co.bow_s) * bow_lag_;
        f = (lag >= 0.0 ? 1.0 : -1.0) * fb * std::min(co.bow_s, mu_s);
        co.bow_dv = excess;
      }
      // The displacement the force pushes in this sample, and a trace of its grit.
      push_force(co, x, static_cast<float>(f * dt / (2.0 * z)), 0.0);
      // A bow pushes one way for as long as it plays: the rails' gauge is taken out now and then.
      if (co.elongation_clock == 0) co.v.remove_gauge(bridge, co.lower_len);
      co.bow_y = y_now;
      co.env = std::max(co.env, static_cast<float>(std::abs(f) * dt / (2.0 * z)) * 20.0f);
    }
  }

  // The hurdy-gurdy's keys: the highest held key's tangent stops the chanterelles.
  void press_tangent(double velocity) {
    Note& ch = chanterelle();
    if (!ch.awake) { wake(ch); ch.held = true; }
    ch.bowed = true;
    retune_chanterelle();
    if (velocity > 0.75) ch.coup_left = static_cast<int>(rate_ * 0.15);
    click_level_ += static_cast<float>(setup_.mechanics * 0.15);
  }
  void release_tangent() { retune_chanterelle(); }
  void retune_chanterelle() {
    const Spec& g = spec();
    int top = -1;
    for (int key = g.high; key >= g.low; --key) if (held_keys_[static_cast<size_t>(key)]) { top = key; break; }
    const int sounding = top < 0 ? g.low : top;
    Note& ch = chanterelle();
    for (int c = 0; c < ch.courses; ++c) {
      Course& co = ch.course[static_cast<size_t>(c)];
      co.ratio = std::exp2((sounding - g.low) / 12.0) * cents_ratio(temperament::offset_cents(setup_.temperament, setup_.root, sounding) - temperament::offset_cents(setup_.temperament, setup_.root, g.low));
    }
  }

  // --- one sample of one note ---

  void tick(Note& n, const Spec& g, float feedback, float& force_l, float& force_r, float& feedback_in) {
    if (n.hammer_active) step_hammer(n);
    if (n.bowed || (g.exciter == Exciter::Wheel && n.bowed)) step_bow(n);
    std::array<float, 2> sum{};
    std::array<bool, 2> used{};
    const double bebung = g.exciter == Exciter::Tangent ? n.lift_ratio * cents_ratio(setup_.bebung_cents * aftertouch_) : 1.0;
    const float hc = static_cast<float>(g.horizontal_coupling);
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      if (!co.active) continue;
      if (co.pluck_in > 0 && --co.pluck_in == 0) lay_pluck(n, co, 1.0, 1.0);
      co.tension_ratio += (co.tension_target - co.tension_ratio) * tension_glide_;
      const double ratio = co.ratio * co.tension_ratio * bend_ * bebung;
      if (std::abs(ratio - co.tuned_ratio) > 2e-6) retune(n, co, ratio);
      if (co.excite_left > 0) {
        --co.excite_left;
        const float noise = co.excite_tone.low(rng_.bipolar()) * co.excite_level;
        co.v.upper().add(std::max(0.0, co.excite_point), noise);
        co.h.upper().add(std::max(0.0, co.excite_point), noise * 0.5f);
      }
      co.v.arrive(co.upper_delay, co.filtered_v);
      co.h.arrive(co.upper_delay_h, co.filtered_h);
      if (co.fade_left > 0) {
        co.fade = std::max(0.0f, co.fade - co.fade_step);
        if (--co.fade_left == 0) { co.fade = 0.0f; co.v.clear(); co.h.clear(); co.filtered_v = co.filtered_h = 0.0f; }
      }
      const float z = static_cast<float>(co.physics.impedance) * co.fade;
      sum[static_cast<size_t>(co.junction)] += z * (co.filtered_v + ((setup_.debug & 4) ? 0.0f : hc * co.filtered_h));
      used[static_cast<size_t>(co.junction)] = true;
    }
    std::array<float, 2> yb{};
    float force = 0.0f;
    for (int j = 0; j < 2; ++j) {
      if (!used[static_cast<size_t>(j)]) continue;
      Junction& jn = n.junction[static_cast<size_t>(j)];
      const float f = sum[static_cast<size_t>(j)];
      // The bridge's motion: the floor (rising toward the treble, as a board's mobility does),
      // the low modes at their coupling, and what the whole board brings from the other notes.
      jn.hp_state = floor_hp_ * jn.hp_state + (1.0f - floor_hp_) * f;
      float y = jn.floor * (f + jn.floor_rise * (f - jn.hp_state));
      if (!(setup_.debug & 1)) for (int m = 0; m < kLowModes; ++m) y += jn.gamma[static_cast<size_t>(m)] * jn.bank[static_cast<size_t>(m)].process(f);
      if (!(setup_.debug & 2)) y += feedback * static_cast<float>(g.sympathetic);
      if (setup_.debug & 8) y = 0.0f;
      yb[static_cast<size_t>(j)] = y;
      // Force on the bridge: 2 Z times the arriving wave's velocity.
      force += 2.0f * (f - jn.prev) * static_cast<float>(rate_);
      jn.prev = f;
      feedback_in += f;
    }
    // The trompette's dog: a loose bridge that lifts and slaps the soundboard when the string
    // pulls hard enough, which a crank's jolt (a coup de poignet) or heavy pressure makes it do.
    if (setup_.model == kHurdyGurdy && &n == &notes_[static_cast<size_t>(kKeys + 2)] && setup_.trompette > 0.0) {
      if (n.coup_left > 0) --n.coup_left;
      const float threshold = static_cast<float>((1.15 - setup_.trompette) * (chanterelle().coup_left > 0 ? 0.35 : 1.0) * 0.6e-3 * 2.5);
      // Each time the string's pull passes the threshold the dog lifts, and when it falls back
      // it strikes the board: a click of wood on wood, once a period, which is the buzz.
      if (sum[0] > threshold && !n.dog_up) {
        n.dog_up = true;
        const float slap = (0.5f + 0.5f * rng_.unipolar());
        yb[0] -= 0.5f * slap * (sum[0] - threshold);
      } else if (sum[0] < threshold * 0.7f && n.dog_up) {
        n.dog_up = false;
        const float slap = (0.6f + 0.4f * rng_.unipolar());
        force += slap * 40.0f * threshold * static_cast<float>(rate_);
        click_level_ += 0.02f * slap;
      }
    }
    float level = 0.0f;
    for (int c = 0; c < n.courses; ++c) {
      Course& co = n.course[static_cast<size_t>(c)];
      if (!co.active) continue;
      const float y = yb[static_cast<size_t>(co.junction)];
      co.v.finish(-co.filtered_v + y, co.lower_len, co.nut_frac, co.nut_gain, co.nut_pole);
      co.h.finish(-co.filtered_h + hc * y, co.lower_len, co.nut_frac_h, co.nut_gain, co.nut_pole);
      level = std::max(level, std::max(std::abs(co.filtered_v), std::abs(co.filtered_h)));
      if (++co.elongation_clock >= 32) { co.elongation_clock = 0; measure_elongation(co); }
    }
    n.env = std::max(level, n.env * env_fall_);
    const double angle = (n.pan * 0.5 + 0.5) * kPi * 0.5;
    force_l += force * static_cast<float>(std::cos(angle));
    force_r += force * static_cast<float>(std::sin(angle));
    const bool exciting = n.hammer_active || n.bowed || n.held;
    bool pending = false;
    for (int c = 0; c < n.courses; ++c) pending = pending || n.course[static_cast<size_t>(c)].pluck_in > 0;
    if (!exciting && !pending && n.env < kQuiet * (n.damping ? 10.0f : 1.0f)) quiet(n);
  }

  // The string's stretch from the waves on it: the tension rises by E A times the strain, the
  // pitch by the square root of that (Kirchhoff-Carrier); a hard blow starts sharp and settles.
  void measure_elongation(Course& co) {
    const Physics& p = co.physics;
    const double half = co.upper_delay;
    if (half < 4.0) return;
    const auto slope2 = [](const string::Rail& r, size_t n) {
      double sum = 0.0;
      for (size_t i = 1; i <= n; ++i) { const double d = r.at(i) - r.at(i - 1); sum += d * d; }
      return sum;
    };
    const size_t upper = static_cast<size_t>(co.upper_delay), upper_h = static_cast<size_t>(co.upper_delay_h);
    const double sum = slope2(co.v.upper(), upper) + slope2(co.v.lower(), co.lower_len) + slope2(co.h.upper(), upper_h) + slope2(co.h.lower(), co.lower_len);
    const double dx = p.length / half;
    const double strain = sum / dx / (2.0 * p.length);
    co.elongation += (strain - co.elongation) * (co.elongation == 0.0 ? 1.0 : 0.3);
    co.tension_target = std::sqrt(1.0 + std::min(0.5, p.ea * co.elongation / p.tension));
  }

  double rate_ = 48000.0;
  Setup setup_{};
  bool configured_ = false;
  std::array<Note, kSlots> notes_{};
  std::array<bool, 128> held_keys_{};
  int held_count_ = 0;
  string::Shape shape_;
  std::array<string::ModalBank<kLowModes + kDenseModes>, 2> board_{};
  string::ModalBank<kLowModes + kFeedbackDense> feedback_{};
  float feedback_out_ = 0.0f, floor_hp_ = 0.9f;
  double bow_lag_ = 0.3;
  std::array<OnePole, 2> accel_{}, rolloff_{};
  std::array<DcBlocker, 2> dc_{};
  OnePole thump_tone_, click_tone_;
  float thump_level_ = 0.0f, click_level_ = 0.0f, thump_fall_ = 0.9995f, click_fall_ = 0.998f, board_env_ = 0.0f, env_fall_ = 0.9995f;
  double tension_glide_ = 0.01, output_scale_ = 14.0, aftertouch_ = 0.0, bend_ = 1.0, wheel_phase_ = 0.0;
  int crank_left_ = 0, last_contact_ = 0;
  double last_peak_force_ = 0.0;
  uint64_t age_counter_ = 0;
  Rng rng_{0x67452301u};
};

}  // namespace schrodr::dsp::keyboard
