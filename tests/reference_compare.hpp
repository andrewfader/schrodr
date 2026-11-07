// The bar a SCHRODR note is held to against its recording (docs/QUALITY.md, "The reference
// recordings"). Each tolerance is calibrated on the recordings themselves: SCHRODR's note must
// come as close to the recorded note as that real instrument's own notes up to an octave away
// come to each other (schrodr-reference-extract prints these spreads; the medians are given).
//
//  - pitch: within 25 cents of the key (a check on the render, not on the recording's tuning);
//  - colour: spectral envelopes within 6 dB RMS once their levels are matched (neighbouring real
//    notes: 5.6 to 6.5 dB);
//  - brightness: spectral centroids within a factor 1.4 (neighbouring real notes: 1.35 to 2.2);
//  - dynamics: the rise from the softest recorded marking to ff within 5 dB of the recording's
//    (the Iowa players' own pp to ff rises spread over 11 to 17.5 dB on one cello);
//  - struck strings: B within a factor 1.5 (taken at the loudest marking recorded, where every
//    partial stands clearest), the prompt and the aftersound decay rates within a factor 1.5;
//  - bars: the first overtone's ratio within 5%, the fundamental's decay within a factor 1.5;
//  - bowed strings: the bow noise within 6 dB (neighbouring real notes: 2.5 to 7 dB).
//
// Shared by reference_tests and schrodr-reference-extract.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "reference_data.hpp"
#include "reference_measure.hpp"
#include "reference_render.hpp"

namespace reference {

struct Check {
  std::string id;      // "<recording id>:<measure>", the name a known gap is listed under
  std::string text;    // what was compared, with both numbers
  bool pass;
};

inline bool within_factor(double a, double b, double factor) {
  return a > 0.0 && b > 0.0 && std::max(a, b) / std::min(a, b) <= factor;
}

// Whether `rec` is the loudest marking recorded of its note.
inline bool loudest_of_its_note(const Recorded& rec) {
  for (const Recorded& r : recorded())
    if (std::string(r.instrument) == rec.instrument && r.key == rec.key && r.dynamic > rec.dynamic) return false;
  return true;
}

// For the loudest marking recorded of a note, the softest one recorded of it, or nullptr.
inline const Recorded* softest_partner(const Recorded& loud) {
  if (!loudest_of_its_note(loud)) return nullptr;
  const Recorded* best = nullptr;
  for (const Recorded& r : recorded())
    if (std::string(r.instrument) == loud.instrument && r.key == loud.key && r.dynamic < loud.dynamic && (!best || r.dynamic < best->dynamic)) best = &r;
  return best;
}

// Compares one recording with SCHRODR's note. `ours_soft` is SCHRODR's note at the marking of
// softest_partner(rec) (only read for the loudest recording of a note that has a softer one).
inline std::vector<Check> compare(const Recorded& rec, const Features& ours, const Features* ours_soft) {
  const Instrument& inst = *instrument(rec.instrument);
  const Features& them = rec.features;
  std::vector<Check> out;
  char text[256];
  const auto add = [&](const char* measure, bool pass) { out.push_back({std::string(rec.id) + ":" + measure, text, pass}); };

  const double off = measure::cents(ours.f0, nominal_hz(rec.key));
  std::snprintf(text, sizeof(text), "sounds %.1f Hz, %+.0f cents from the key", ours.f0, off);
  add("pitch", ours.ok && std::abs(off) <= 25.0);

  const double distance = envelope_distance(ours.bands, them.bands);
  if (inst.colour && distance >= 0.0) {
    std::snprintf(text, sizeof(text), "spectral envelope %.1f dB RMS from the recording's (bar %g)", distance, inst.colour_db);
    add("colour", distance <= inst.colour_db);
  }
  std::snprintf(text, sizeof(text), "centroid %.2f harmonics, the recording %.2f (bar x%g)", ours.centroid, them.centroid, inst.brightness);
  add("brightness", within_factor(ours.centroid, them.centroid, inst.brightness));

  if (ours_soft && inst.dynamics) {
    if (const Recorded* soft = softest_partner(rec)) {
      const double mine = ours.level_db - ours_soft->level_db, theirs = them.level_db - soft->features.level_db;
      std::snprintf(text, sizeof(text), "%s to %s rises %.1f dB, the recording %.1f dB (bar 5)", dynamic_name(soft->dynamic), dynamic_name(rec.dynamic), mine, theirs);
      add("dynamics", std::abs(mine - theirs) <= 5.0);
    }
  }

  switch (inst.kind) {
    case Kind::Struck:
      if (loudest_of_its_note(rec)) {
        std::snprintf(text, sizeof(text), "B %.2e, the recording %.2e (bar x1.5)", ours.inharmonicity, them.inharmonicity);
        add("inharmonicity", within_factor(ours.inharmonicity, them.inharmonicity, 1.5));
      }
      std::snprintf(text, sizeof(text), "prompt sound decays %.1f dB/s, the recording %.1f (bar x1.5)", ours.decay_early, them.decay_early);
      add("prompt decay", within_factor(ours.decay_early, them.decay_early, 1.5));
      if (them.decay_late > 0.0 && inst.tail_trusted) {
        std::snprintf(text, sizeof(text), "aftersound decays %.1f dB/s, the recording %.1f (bar x1.5)", ours.decay_late, them.decay_late);
        add("aftersound decay", within_factor(ours.decay_late, them.decay_late, 1.5));
      }
      break;
    case Kind::Bar:
      if (inst.overtone) {
        std::snprintf(text, sizeof(text), "first overtone at %.3f f1, the recording %.3f (bar 5%%)", ours.overtone_ratio, them.overtone_ratio);
        add("overtone", std::abs(ours.overtone_ratio / them.overtone_ratio - 1.0) <= 0.05);
      }
      if (them.decay_early > 0.0) {  // none where the recording's fundamental gave no clean slope
        std::snprintf(text, sizeof(text), "fundamental decays %.1f dB/s, the recording %.1f (bar x1.5)", ours.decay_early, them.decay_early);
        add("decay", within_factor(ours.decay_early, them.decay_early, 1.5));
      }
      break;
    case Kind::Bowed:
      std::snprintf(text, sizeof(text), "bow noise %.1f dB under the harmonics, the recording %.1f (bar 6)", -ours.noise_db, -them.noise_db);
      add("bow noise", std::abs(ours.noise_db - them.noise_db) <= 6.0);
      break;
    case Kind::Pipe:
      break;  // a pipe's steady tone: its colour and brightness above say it
  }
  return out;
}

// Renders and measures SCHRODR's side of every recording, each note once per marking.
struct Ours { const Recorded* rec; Features note, soft; bool has_soft; };
inline std::vector<Ours> measure_all(const std::string& filter = "") {
  std::vector<Ours> out;
  for (const Recorded& r : recorded()) {
    if (!filter.empty() && std::string(r.id).find(filter) == std::string::npos) continue;
    const Instrument& inst = *instrument(r.instrument);
    Ours o{&r, measure_render(inst, r.key, r.dynamic), {}, false};
    if (const Recorded* soft = softest_partner(r)) { o.soft = measure_render(inst, r.key, soft->dynamic); o.has_soft = true; }
    out.push_back(o);
  }
  return out;
}

}  // namespace reference
