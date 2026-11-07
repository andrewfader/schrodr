// How SCHRODR plays a reference note (docs/QUALITY.md, "The reference recordings"): the instrument
// with its default settings, the room off and the master at 0 dB, so what is compared is the
// instrument and not a preset's room; one key at the touch the recording's dynamic marking asks
// for. Shared by schrodr-reference-extract and reference_tests.
#pragma once

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "../src/engine.hpp"
#include "reference_measure.hpp"

namespace reference {

enum Dynamic : int { kPp, kMf, kF, kFf };
inline const char* dynamic_name(int d) { return d == kPp ? "pp" : d == kMf ? "mf" : d == kF ? "f" : "ff"; }
inline int dynamic_from(const std::string& s) { return s == "pp" ? kPp : s == "mf" ? kMf : s == "f" ? kF : kFf; }
// The usual MIDI readings of the markings (pp 33, mf 80, f 96, ff 112 of 127).
inline double velocity(int d) { return (d == kPp ? 33.0 : d == kMf ? 80.0 : d == kF ? 96.0 : 112.0) / 127.0; }

// What a recording can be held to, and how closely. The colour and brightness bars are the
// spread between that instrument's own recorded notes up to an octave apart (the medians
// schrodr-reference-extract prints), never tighter than 6 dB and a factor 1.4.
//  - `tail_trusted` is false where the recordings were noise-reduced, which eats a decaying
//    note's tail: their aftersound is not judged;
//  - `dynamics` is false where the recordings' levels were set by the sampler, not the player;
//  - `overtone` is false where the 2.2-4.5 f1 search finds no bar mode (an electric piano's
//    pickup makes harmonics there) or finds none consistently in the recording (the celesta);
//  - `colour` is false for bars, whose few inharmonic partials are not an envelope;
//  - `settings` set what the recording used where the instrument's default differs.
struct Instrument {
  const char* name; int id; Kind kind;
  double colour_db = 6.0, brightness = 1.4;
  bool tail_trusted = true, dynamics = true, overtone = true, colour = true;
  std::vector<std::pair<int, double>> settings = {};
};
inline const std::vector<Instrument>& instruments() {
  using namespace schrodr::params;
  static const std::vector<Instrument> all{
      {.name = "grand", .id = kGrand, .kind = Kind::Struck},
      {.name = "upright", .id = kUpright, .kind = Kind::Struck, .colour_db = 8.5, .brightness = 1.45},
      {.name = "flemish", .id = kFlemish, .kind = Kind::Struck, .colour_db = 9.5},
      {.name = "italian", .id = kItalian, .kind = Kind::Struck, .colour_db = 9.0, .brightness = 1.6},
      {.name = "french", .id = kFrench, .kind = Kind::Struck, .colour_db = 7.5, .brightness = 1.75},
      {.name = "clavichord", .id = kUnfrettedClavichord, .kind = Kind::Struck, .tail_trusted = false},
      {.name = "toy piano", .id = kToyPiano, .kind = Kind::Bar, .brightness = 2.0, .colour = false},
      {.name = "celesta", .id = kCelesta, .kind = Kind::Bar, .dynamics = false, .overtone = false, .colour = false},
      {.name = "glockenspiel", .id = kGlockenspiel, .kind = Kind::Bar, .colour = false},
      {.name = "electric piano", .id = kElectricPiano, .kind = Kind::Bar, .dynamics = false, .overtone = false},
      {.name = "organ", .id = kOrgan, .kind = Kind::Pipe, .colour_db = 10.5, .settings = {{kDiapason8, 0}, {kGedackt8, 1}}},
      {.name = "violin", .id = kViolin, .kind = Kind::Bowed},
      {.name = "viola", .id = kViola, .kind = Kind::Bowed},
      {.name = "cello", .id = kCello, .kind = Kind::Bowed, .brightness = 2.25},
  };
  return all;
}
inline const Instrument* instrument(const std::string& name) {
  for (const Instrument& i : instruments()) if (name == i.name) return &i;
  return nullptr;
}

constexpr double kRate = 48000.0;
constexpr double kPreRoll = 0.1;

// How long the key is held: a struck note long enough for its aftersound, a bowed or blown one
// about as long as the recorded scales' notes.
inline double hold_seconds(Kind k) { return k == Kind::Struck ? 8.0 : k == Kind::Bar ? 2.0 : 2.6; }

inline std::vector<float> render(const Instrument& inst, int key, int dynamic) {
  schrodr::Engine engine;
  engine.prepare(kRate);
  schrodr::Settings s = schrodr::Settings::defaults();
  s.v[schrodr::params::kInstrument] = inst.id;
  s.v[schrodr::params::kMasterDb] = 0.0;
  s.v[schrodr::params::kRoomSize] = 0;
  // The Iowa strings were recorded without vibrato (it measures under 6 cents there).
  s.v[schrodr::params::kVibrato] = 0;
  for (const auto& [id, value] : inst.settings) s.v[static_cast<size_t>(id)] = value;
  const double hold = hold_seconds(inst.kind), tail = 0.6;
  const size_t frames = static_cast<size_t>((kPreRoll + hold + tail) * kRate);
  std::vector<float> l(frames), r(frames), mono(frames);
  const size_t on = static_cast<size_t>(kPreRoll * kRate), off = static_cast<size_t>((kPreRoll + hold) * kRate);
  constexpr size_t kBlock = 256;
  for (size_t at = 0; at < frames; at += kBlock) {
    const uint32_t n = static_cast<uint32_t>(std::min(kBlock, frames - at));
    if (at <= on && on < at + n) engine.note_on(s, 0, key, -1, velocity(dynamic));
    if (at <= off && off < at + n) engine.note_off(s, 0, key, -1);
    engine.render(s, l.data() + at, r.data() + at, n);
  }
  for (size_t i = 0; i < frames; ++i) mono[i] = 0.5f * (l[i] + r[i]);
  return mono;
}

// A render has no noise floor of its own; it is measured as if over a 24-bit converter's.
constexpr double kRenderFloorDb = -140.0;
inline Features measure_render(const Instrument& inst, int key, int dynamic) {
  return measure_note(render(inst, key, dynamic), kRate, inst.kind, key, kRenderFloorDb,
                      static_cast<size_t>((kPreRoll + hold_seconds(inst.kind)) * kRate));
}

}  // namespace reference
