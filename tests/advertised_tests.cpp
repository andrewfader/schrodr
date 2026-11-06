// Every parameter, proved against what it advertises, from audio, through the built plug-in.
//
// The plug-in advertises each parameter as a name, a unit and a range. This suite holds one
// proof per parameter ID: a musical context (instrument, a few settings, a note), two values of
// the parameter, and a measurement in the advertised unit comparing the two renders: cents for
// pitch, dB for level, Hz for rates, spectral centroid for brightness, tail energy for length,
// or at the least that the audio changed.
//
// The table is indexed by ID and the suite fails if any parameter has no proof, so a new
// parameter cannot ship without one.
//
//   advertised_tests <SCHRODR.clap> [name-filter]

#include "../src/dsp/air.hpp"
#include "../src/dsp/core.hpp"
#include "../src/dsp/temperament.hpp"
#include "../src/params.hpp"
#include "audio_measure.hpp"
#include "clap_harness.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

using namespace harness;
using measure::Span;
namespace P = schrodr::params;

namespace {

constexpr double kRate = 48000.0;
constexpr uint32_t kBlock = 256;
const char* g_path = nullptr;
double g_note_hz = 261.63;

struct Setting { uint32_t id; double value; };

// The musical situation a parameter is proved in.
struct Context {
  int instrument = P::kGrand;
  std::vector<Setting> settings;
  int key = 60;
  double velocity = 0.7;
  double hold_s = 1.0, tail_s = 0.5;
  bool bend_up = false;          // a full pitch-bend up before the note
  bool pressure = false;         // full channel pressure after the note starts
  int second_key = -1;           // a second note after hold_s / 2
  double late_s = 0.0;           // > 0: the parameter under proof is set this long after the note starts, not before
};

struct Render { std::vector<float> mono, left, right; size_t note_off; };

Render render(const Context& c, uint32_t id, double value) {
  Library lib(g_path);
  const clap_plugin_t* plugin = lib.create();
  set_param(plugin, P::kInstrument, c.instrument);
  for (const Setting& s : c.settings) set_param(plugin, s.id, s.value);
  if (c.late_s <= 0.0) set_param(plugin, id, value);
  bool late_sent = c.late_s <= 0.0;
  Runner runner(plugin, kRate, kBlock);
  Render out;
  const size_t hold = static_cast<size_t>(c.hold_s * kRate), total = hold + static_cast<size_t>(c.tail_s * kRate);
  out.note_off = hold;
  size_t rendered = 0;
  bool second_sent = false, pressed = false;
  Events events;
  if (c.bend_up) events.push(midi_event(0xe0, 0x7f, 0x7f));
  events.push(note_event(CLAP_EVENT_NOTE_ON, 0, static_cast<int16_t>(c.key), -1, c.velocity));
  bool off_sent = false;
  while (rendered < total) {
    if (!off_sent && rendered >= hold) {
      events.push(note_event(CLAP_EVENT_NOTE_OFF, 0, static_cast<int16_t>(c.key), -1, 0.0));
      if (c.second_key >= 0) events.push(note_event(CLAP_EVENT_NOTE_OFF, 0, static_cast<int16_t>(c.second_key), -1, 0.0));
      off_sent = true;
    }
    if (c.pressure && !pressed && rendered >= 2400) { events.push(midi_event(0xd0, 127, 0)); pressed = true; }
    if (!late_sent && rendered >= static_cast<size_t>(c.late_s * kRate)) { events.push(param_event(id, value)); late_sent = true; }
    if (c.second_key >= 0 && !second_sent && rendered >= hold / 2) {
      events.push(note_event(CLAP_EVENT_NOTE_ON, 0, static_cast<int16_t>(c.second_key), -1, c.velocity));
      second_sent = true;
    }
    runner.run(&events);
    events.clear();
    for (uint32_t i = 0; i < kBlock; ++i) {
      out.left.push_back(runner.left()[i]); out.right.push_back(runner.right()[i]);
      out.mono.push_back(0.5f * (runner.left()[i] + runner.right()[i]));
    }
    rendered += kBlock;
  }
  runner.destroy();
  return out;
}

Span held(const Render& r) { return Span(r.mono, 4800, r.note_off > 9600 ? r.note_off - 9600 : 4800); }
Span late(const Render& r) { return Span(r.mono, r.note_off > 14400 ? r.note_off - 9600 : 4800, 9600); }
Span tail(const Render& r) { return Span(r.mono, r.note_off + 2400); }
Span attack(const Render& r) { return Span(r.mono, 0, 4800); }
double centroid(const Span& s) { return measure::spectral_centroid(s, kRate, 12); }
double level_db(const Span& s) { return measure::db(measure::rms(s)); }
// A tuner's reading of the strongest component near a frequency.
double tone_near(const Span& s, double hz, double span_cents = 60.0) {
  double best = 0.0;
  for (double c = -span_cents; c <= span_cents; c += 2.0) best = std::max(best, measure::tone_amplitude(s, kRate, hz * std::exp2(c / 1200.0)));
  return best;
}
double peak_hz(const Span& s, double around, double span_cents = 60.0) {
  double best = -1.0, best_hz = around;
  for (double c = -span_cents; c <= span_cents; c += 0.5) {
    const double f = around * std::exp2(c / 1200.0);
    const double a = measure::tone_amplitude(s, kRate, f);
    if (a > best) { best = a; best_hz = f; }
  }
  return best_hz;
}
bool differs(const Render& a, const Render& b) {
  const size_t n = std::min(a.mono.size(), b.mono.size());
  double diff = 0.0, energy = 0.0;
  for (size_t i = 0; i < n; ++i) { const double d = a.mono[i] - b.mono[i]; diff += d * d; energy += a.mono[i] * a.mono[i]; }
  return diff > 1e-4 * std::max(energy, 1e-6) && diff > 1e-10;
}

struct Proof {
  Context context;
  double low, high;
  std::function<std::string(const Render&, const Render&)> check;
};

std::string ok() { return {}; }
std::string fail(const char* format, double a, double b = 0.0) { char out[256]; std::snprintf(out, sizeof(out), format, a, b); return out; }

auto Differs() { return [](const Render& a, const Render& b) { return differs(a, b) ? ok() : std::string("the audio did not change"); }; }
// The fundamental near the note moves by `expected` cents.
auto PitchCents(double expected, double tolerance = 10.0, double span = 120.0) {
  return [=](const Render& a, const Render& b) {
    const double pa = peak_hz(held(a), g_note_hz, span), pb = peak_hz(held(b), g_note_hz * std::exp2(expected / 1200.0), span);
    const double cents = measure::cents(pb, pa);
    return std::abs(cents - expected) <= tolerance ? ok() : fail("pitch moved %.1f cents, expected %.1f", cents, expected);
  };
}
auto LevelDb(double expected, double tolerance = 2.0) {
  return [=](const Render& a, const Render& b) {
    const double d = level_db(held(b)) - level_db(held(a));
    return std::abs(d - expected) <= tolerance ? ok() : fail("level moved %.1f dB, expected %.1f", d, expected);
  };
}
auto LouderBy(double min_db) {
  return [=](const Render& a, const Render& b) {
    const double d = level_db(held(b)) - level_db(held(a));
    return d >= min_db ? ok() : fail("level moved %.1f dB, expected at least %.1f", d, min_db);
  };
}
auto Brighter(double ratio = 1.15) {
  return [=](const Render& a, const Render& b) {
    const double ca = centroid(held(a)), cb = centroid(held(b));
    return cb > ratio * ca ? ok() : fail("centroid %.0f Hz -> %.0f Hz is not brighter", ca, cb);
  };
}
auto BrighterAttack(double ratio = 1.15) {
  return [=](const Render& a, const Render& b) {
    const double ca = centroid(attack(a)), cb = centroid(attack(b));
    return cb > ratio * ca ? ok() : fail("attack centroid %.0f Hz -> %.0f Hz is not brighter", ca, cb);
  };
}
auto Darker(double ratio = 1.15) {
  return [=](const Render& a, const Render& b) {
    const double ca = centroid(held(a)), cb = centroid(held(b));
    return ca > ratio * cb ? ok() : fail("centroid %.0f Hz -> %.0f Hz is not darker", ca, cb);
  };
}
auto LongerTail(double ratio = 1.5) {
  return [=](const Render& a, const Render& b) {
    const double ta = measure::rms(tail(a)), tb = measure::rms(tail(b));
    return tb > ratio * ta + 1e-6 ? ok() : fail("tail rms %.5f -> %.5f is not longer", ta, tb);
  };
}
auto ShorterTail(double ratio = 1.5) {
  return [=](const Render& a, const Render& b) {
    const double ta = measure::rms(tail(a)), tb = measure::rms(tail(b));
    return ta > ratio * tb + 1e-6 ? ok() : fail("tail rms %.5f -> %.5f is not shorter", ta, tb);
  };
}
auto LouderLate(double min_db) {
  return [=](const Render& a, const Render& b) {
    const double d = level_db(late(b)) - level_db(late(a));
    return d >= min_db ? ok() : fail("late level moved %.1f dB, expected at least %.1f", d, min_db);
  };
}
auto AmpModHz(double expected, double tolerance = 0.8) {
  return [=](const Render&, const Render& b) {
    const std::vector<float> part(b.mono.begin() + 14400, b.mono.begin() + static_cast<long>(b.note_off));
    std::vector<double> linear;
    for (double v : measure::contour_db(part, kRate, 0.005, 0.010)) linear.push_back(std::pow(10.0, v / 20.0));
    const double hz = measure::modulation_hz(linear, 0.005, std::max(0.5, expected * 0.4), expected * 2.5);
    return std::abs(hz - expected) <= tolerance ? ok() : fail("modulation at %.2f Hz, expected %.2f", hz, expected);
  };
}
auto Wider() {
  return [](const Render& a, const Render& b) {
    const double ca = measure::correlation(Span(a.left, 4800, 24000), Span(a.right, 4800, 24000));
    const double cb = measure::correlation(Span(b.left, 4800, 24000), Span(b.right, 4800, 24000));
    return cb < ca - 0.05 ? ok() : fail("L/R correlation %.3f -> %.3f is not wider", ca, cb);
  };
}
auto Narrower() {
  return [](const Render& a, const Render& b) {
    const double ca = measure::correlation(Span(a.left, 4800, 24000), Span(a.right, 4800, 24000));
    const double cb = measure::correlation(Span(b.left, 4800, 24000), Span(b.right, 4800, 24000));
    return cb > ca + 0.05 ? ok() : fail("L/R correlation %.3f -> %.3f is not narrower", ca, cb);
  };
}
auto SidesSwap() {
  return [](const Render& a, const Render& b) {
    const double ba = level_db(Span(a.left, 4800, 24000)) - level_db(Span(a.right, 4800, 24000));
    const double bb = level_db(Span(b.left, 4800, 24000)) - level_db(Span(b.right, 4800, 24000));
    return ba * bb < 0.0 && std::abs(ba) > 0.5 ? ok() : fail("balance %.1f dB -> %.1f dB did not swap sides", ba, bb);
  };
}
auto ToneRises(double ratio_of_note, double factor = 2.0) {
  return [=](const Render& a, const Render& b) {
    const double hz = g_note_hz * ratio_of_note;
    const double ta = tone_near(held(a), hz), tb = tone_near(held(b), hz);
    return tb > factor * ta + 1e-5 ? ok() : fail("tone at the interval %.5f -> %.5f did not rise", ta, tb);
  };
}
auto ToneMovesDb(double ratio_of_note, double expected_db, double tolerance_db) {
  return [=](const Render& a, const Render& b) {
    const double hz = g_note_hz * ratio_of_note;
    const double d = measure::db(tone_near(held(b), hz, 30.0)) - measure::db(tone_near(held(a), hz, 30.0));
    return std::abs(d - expected_db) <= tolerance_db ? ok() : fail("tone moved %.2f dB, expected %.2f", d, expected_db);
  };
}
auto MoreNoise() {
  return [](const Render& a, const Render& b) {
    // The energy above 3 kHz while held: the fraction times the mean square.
    const auto hiss = [](const Render& r) { const Span s = held(r); const double rms = measure::rms(s); return measure::band_fraction(s, kRate, 3000.0, 20000.0) * rms * rms; };
    const double na = hiss(a), nb = hiss(b);
    return nb > 2.0 * na + 1e-12 ? ok() : fail("high-band energy %.2e -> %.2e is not noisier", na, nb);
  };
}
auto MoreAttackNoise() {
  return [](const Render& a, const Render& b) {
    // The energy above 3 kHz in the first 50 ms: the fraction times the mean square.
    const auto hiss = [](const Render& r) { const Span s(r.mono, 0, 2400); const double rms = measure::rms(s); return measure::band_fraction(s, kRate, 3000.0, 20000.0) * rms * rms; };
    const double na = hiss(a), nb = hiss(b);
    return nb > 3.0 * na + 1e-12 ? ok() : fail("attack high-band energy %.2e -> %.2e is not noisier", na, nb);
  };
}
auto MoreBass() {
  return [](const Render& a, const Render& b) {
    const double la = measure::band_fraction(held(a), kRate, 20.0, 160.0), lb = measure::band_fraction(held(b), kRate, 20.0, 160.0);
    return lb > 1.3 * la + 1e-5 ? ok() : fail("low-band fraction %.4f -> %.4f did not rise", la, lb);
  };
}
auto LaterOnset(double expected_ms, double tolerance_ms) {
  return [=](const Render& a, const Render& b) {
    const auto onset = [](const Render& r) {
      const auto c = measure::contour_db(r.mono, kRate, 0.002, 0.004);
      double peak = -200.0; for (size_t i = 0; i < c.size() / 2; ++i) peak = std::max(peak, c[i]);
      return static_cast<double>(measure::first_at_or_above(c, peak - 0.5)) * 2.0;
    };
    const double d = onset(b) - onset(a);
    return std::abs(d - expected_ms) <= tolerance_ms ? ok() : fail("onset moved %.0f ms, expected %.0f", d, expected_ms);
  };
}
// The second render sounds where the first did not (a pedal, a register, a stop).
auto Sounds(double min_db = -60.0) {
  return [=](const Render&, const Render& b) {
    const double l = level_db(held(b));
    return l > min_db ? ok() : fail("level %.1f dBFS, expected over %.1f", l, min_db);
  };
}

Context ctx(int instrument, std::vector<Setting> settings = {}, int key = 60) { Context c; c.instrument = instrument; c.settings = std::move(settings); c.key = key; return c; }

std::array<Proof, P::kParamCount> proofs() {
  using namespace P;
  std::array<Proof, kParamCount> t{};
  const auto set = [&](ParamId id, Context c, double low, double high, auto check) { t[id] = Proof{std::move(c), low, high, check}; };
  // the instrument and its tuning
  set(kInstrument, ctx(kGrand), kGrand, kFlemish, Differs());
  set(kMasterDb, ctx(kGrand), -30, 0, LevelDb(30.0));
  set(kTranspose, ctx(kGrand), 0, 12, PitchCents(1200.0, 30.0, 150.0));
  // A4 from 415 to 440 is 101.6 cents; proved on the Flemish harpsichord's a', whose iron string
  // is all but harmonic and which the tuner lays on the fork.
  set(kPitchA4, ctx(kFlemish, {}, 69), 415, 440, PitchCents(1200.0 * std::log2(440.0 / 415.0), 4.0));
  // Quarter-comma meantone laid from C with A on the fork: E is four narrow fifths from C, A three,
  // so E sits 3.42 cents under equal temperament's. Read on the grand's E4, which lies in the
  // temperament octave the tuner lays before stretching, struck softly on one true unison.
  { Context c = ctx(kGrand, {{kUnisonSpread, 0}}, 64); c.velocity = 0.3; set(kTemperament, c, 0, 2, PitchCents(-3.42, 1.5, 60.0)); }
  // Laid from D instead of C, the wolf moves and E-flat becomes D-sharp, seven narrow fifths up
  // from D instead of three wide ones down from C: from +20.5 to -20.5 cents against A, a move
  // of 41.1 cents, read on the grand's E-flat 4.
  { Context c = ctx(kGrand, {{kTemperament, 2}, {kUnisonSpread, 0}}, 63); c.velocity = 0.3; set(kTemperamentRoot, c, 0, 2, PitchCents(-41.1, 2.0, 80.0)); }
  // The grand's C7 with the tuner's stretch against none: 10 to 45 cents sharp of the octave.
  { Context c = ctx(kGrand, {}, 96); c.velocity = 0.3; set(kStretch, c, 0, 1, [](const Render& a, const Render& b) {
      const double pa = peak_hz(held(a), g_note_hz, 80.0), pb = peak_hz(held(b), g_note_hz * 1.02, 80.0);
      const double cents = measure::cents(pb, pa);
      return cents > 8.0 && cents < 50.0 ? ok() : fail("C7 moved %.1f cents, expected the stretch", cents); }); }
  // Unisons 8 cents apart beat; the held note's level wavers where the true unison holds.
  set(kUnisonSpread, ctx(kGrand), 0, 8, [](const Render& a, const Render& b) {
      const auto wobble = [](const Render& r) {
        double w = 0.0; double prev = -1.0;
        for (size_t at = 4800; at + 2400 <= r.note_off; at += 480) { const double v = measure::tone_amplitude(Span(r.mono, at, 2400), kRate, g_note_hz); if (prev > 0.0) w += std::abs(std::log(v / prev)); prev = v; }
        return w; };
      return wobble(b) > 2.0 * wobble(a) ? ok() : fail("level wobble %.2f -> %.2f: the unisons do not beat", wobble(a), wobble(b)); });
  { Context c = ctx(kGrand); c.velocity = 0.15; set(kDynamics, c, 1, 0, LouderBy(8.0)); }
  // the action
  set(kHammerHardness, ctx(kGrand), 0.1, 0.9, Brighter());
  // C3's hammer sits at 0.110 of the string; at 133% it is at a seventh, where the 7th partial
  // has a node, and at 67% (a fourteenth) that partial comes back.
  { Context c = ctx(kGrand, {{kUnisonSpread, 0}}, 48); set(kStrikePoint, c, 1.33, 0.67, ToneRises(7.0 * std::sqrt((1.0 + 49.0 * 1.9e-4) / (1.0 + 1.9e-4)), 3.0)); }
  set(kPlectrum, ctx(kFlemish), 0, 1, BrighterAttack());
  set(kMechanics, ctx(kFlemish), 0, 1, Differs());
  { Context c = ctx(kGrand); c.tail_s = 0.8; set(kRelease, c, 20, 2000, LongerTail()); }
  { Context c = ctx(kUnfrettedClavichord); c.pressure = true; set(kBebung, c, 0, 40, PitchCents(40.0, 10.0, 100.0)); }
  { Context c = ctx(kHurdyGurdy, {}, 74); c.hold_s = 1.5; set(kWheelSpeed, c, 0.6, 2.0, Differs()); set(kWheelPressure, c, 0.2, 0.9, Differs()); }
  { Context c = ctx(kHurdyGurdy, {{kWheelSpeed, 1.8}}, 74); c.velocity = 1.0; c.hold_s = 1.0; set(kTrompette, c, 0, 1, MoreNoise()); }
  // The drones under d'': the trompette's c' appears where the chanterelle has nothing (the
  // bourdons' gut radiates little of its low fundamental, as on the instrument).
  { Context c = ctx(kHurdyGurdy, {}, 74); c.hold_s = 1.0; set(kDrones, c, 0, 1, ToneRises(261.63 / 587.33, 3.0)); }
  // harpsichord registers: with the back 8' off, each register alone against none
  set(kBack8, ctx(kFlemish, {{kFront8, 0}}), 0, 1, Sounds());
  set(kFront8, ctx(kFlemish, {{kBack8, 0}}), 0, 1, Sounds());
  // The 4' with the 8's off: the octave appears where there was nothing (an in-tune 4' and the
  // 8's second partial interfere, so it is proved on its own strings).
  set(kFour, ctx(kFlemish, {{kBack8, 0}}), 0, 1, ToneRises(2.0, 10.0));
  set(kLute, ctx(kFlemish, {{kBack8, 0}}), 0, 1, Sounds());
  { Context c = ctx(kFlemish); c.tail_s = 0.2; set(kBuff, c, 0, 1, Darker()); }
  // organ stops and wind
  set(kDiapason8, ctx(kOrgan, {{kGedackt8, 0}}), 0, 1, Sounds());
  set(kGedackt8, ctx(kOrgan, {{kDiapason8, 0}}), 0, 1, Sounds());
  set(kPrincipal4, ctx(kOrgan), 0, 1, ToneRises(2.0, 2.0));
  set(kFifteenth2, ctx(kOrgan), 0, 1, ToneRises(4.0, 2.0));
  set(kQuint, ctx(kOrgan), 0, 1, ToneRises(3.0, 2.0));
  { Context c = ctx(kOrgan); c.hold_s = 1.5; set(kWind, c, 0.7, 1.3, LouderBy(1.0)); }
  { Context c = ctx(kOrgan, {{kGedackt8, 1}, {kDiapason8, 0}, {kTremulantRate, 5.0}}); c.hold_s = 3.0; set(kTremulant, c, 0, 1, AmpModHz(5.0)); }
  { Context c = ctx(kOrgan, {{kGedackt8, 1}, {kDiapason8, 0}, {kTremulant, 1.0}}); c.hold_s = 3.0; set(kTremulantRate, c, 3.0, 7.0, AmpModHz(7.0, 1.0)); }
  set(kChiff, ctx(kOrgan), 0, 1, MoreAttackNoise());
  // pedals
  { Context c = ctx(kGrand); c.hold_s = 0.5; c.tail_s = 1.0; set(kSustain, c, 0, 1, LongerTail(3.0)); }
  // Sostenuto down 0.2 s into C4, then E4 played: C4 rings after both are released.
  { Context c = ctx(kGrand); c.second_key = 64; c.hold_s = 0.8; c.tail_s = 1.0; c.late_s = 0.2; set(kSostenuto, c, 0, 1, LongerTail(2.0)); }
  { Context c = ctx(kGrand); c.velocity = 0.5; set(kUnaCorda, c, 0, 1, LevelDb(-4.5, 3.0)); }
  { Context c = ctx(kFortepiano); c.velocity = 0.5; set(kModerator, c, 0, 1, Darker()); }
  // With the pedal down, the strings tuned to C3's partials ring on after it is damped... the
  // sympathetic coupling is heard as a longer, richer tail on a C3 with the pedal released.
  { Context c = ctx(kGrand, {{kSustain, 1}}, 48); c.hold_s = 0.6; c.tail_s = 1.5; set(kSympathetic, c, 0, 1, Differs()); }
  set(kLid, ctx(kGrand), 0, 2, Brighter(1.1));
  // the stereo picture
  set(kStereoWidth, ctx(kGrand), 0, 1, Wider());
  // From the bench the bass is on the left; from the audience on the right.
  set(kPerspective, ctx(kGrand, {{kStereoWidth, 1}}, 36), 0, 1, SidesSwap());
  // the room and the air
  set(kRoomSize, ctx(kGrand, {{kMicDistance, 2.0}}), 0, 3, LongerTail(1.5));
  { Context c = ctx(kOrgan, {{kRoomSize, 5}, {kMicDistance, 3.0}}); c.hold_s = 1.5; c.tail_s = 1.5; set(kRoomAbsorption, c, 0.08, 0.6, ShorterTail(1.5)); }
  // The microphone 10 m away in 20 C air: the note arrives 10 / 343.2 s = 29.1 ms later.
  set(kMicDistance, ctx(kToyPiano, {{kMechanics, 0}}), 0, 10, LaterOnset(29.1, 3.0));
  // Twelve metres of open air from a bright glockenspiel note: its 3rd mode near 5.6 kHz
  // loses what ISO 9613-1 gives at 10% and at 70% humidity, humid air taking less.
  {
    const double hz = 1046.5 * 5.4;
    const double expected = 12.0 * (schrodr::dsp::air::absorption_db_per_m(hz, 20.0, 0.1) - schrodr::dsp::air::absorption_db_per_m(hz, 20.0, 0.7));
    Context c = ctx(kGlockenspiel, {{kMicDistance, 12.0}}, 84);
    set(kHumidity, c, 0.1, 0.7, [=](const Render& a, const Render& b) {
      const double d = measure::db(tone_near(held(b), hz, 40.0)) - measure::db(tone_near(held(a), hz, 40.0));
      return std::abs(d - expected) <= 0.6 ? ok() : fail("tone moved %.2f dB, expected %.2f", d, expected); });
  }
  // Iron strings tuned at 20 C, played at 40 C: iron expands 1.17e-5 per kelvin against the
  // wooden case's 3.8e-6, so the Flemish harpsichord's c' goes flat by what E A dT gives (about
  // 25 cents at its low tension).
  set(kTemperature, ctx(kFlemish, {}, 60), 20, 40, [](const Render& a, const Render& b) {
      const double cents = measure::cents(peak_hz(held(b), g_note_hz, 200.0), peak_hz(held(a), g_note_hz, 200.0));
      return cents < -8.0 && cents > -120.0 ? ok() : fail("pitch moved %.1f cents, expected flat", cents); });
  // A cardioid 5 cm from the grand's A1 against an omni there: the proximity effect lifts the bass.
  set(kMicPattern, ctx(kGrand, {{kMicDistance, 0.05}}, 33), 0, 1, MoreBass());
  set(kMicSetup, ctx(kGrand, {{kRoomSize, 3}, {kMicDistance, 2.0}}), 0, 1, Narrower());
  set(kPreset, ctx(kGrand), 0, 17, Differs());
  return t;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) { std::fprintf(stderr, "usage: advertised_tests <SCHRODR.clap> [filter]\n"); return 2; }
  g_path = argv[1];
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  const char* filter = argc > 2 ? argv[2] : nullptr;
  const auto table = proofs();
  int missing = 0, failed = 0, passed = 0;
  for (uint32_t id = 0; id < P::kParamCount; ++id) {
    const Proof& proof = table[id];
    const char* name = P::kSpecs[id].name;
    if (!proof.check) { std::printf("MISSING  %s has no proof\n", name); ++missing; continue; }
    if (filter && !std::strstr(name, filter)) continue;
    g_note_hz = schrodr::dsp::midi_to_hz(proof.context.key);
    const Render low = render(proof.context, id, proof.low), high = render(proof.context, id, proof.high);
    bool finite = true;
    for (float v : high.mono) finite = finite && std::isfinite(v);
    std::string verdict = finite ? proof.check(low, high) : "output is not finite";
    char lo[64], hi[64];
    P::format_value(id, proof.low, lo, sizeof(lo)); P::format_value(id, proof.high, hi, sizeof(hi));
    if (verdict.empty()) { ++passed; std::printf("ok       %-20s %s -> %s\n", name, lo, hi); }
    else { ++failed; std::printf("FAILED   %-20s %s -> %s: %s\n", name, lo, hi, verdict.c_str()); }
  }
  std::printf("%d proved, %d failed, %d missing of %u parameters\n", passed, failed, missing, static_cast<unsigned>(P::kParamCount));
  return failed == 0 && missing == 0 ? 0 : 1;
}
