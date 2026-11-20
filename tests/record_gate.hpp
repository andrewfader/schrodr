// The record gate's cases, phrases and verdict, shared by record_gate_tests and the tuning tool
// (tools/record_gate_try.cpp). See record_gate_tests.cpp for what the gate holds presets to.
#pragma once

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/engine.hpp"
#include "../src/presets.hpp"
#include "record_features.hpp"
#include "record_reference.hpp"

namespace record_gate {

constexpr double kRate = 44100.0;  // the corpus's rate
constexpr double kSeconds = 16.0;
constexpr uint32_t kBlock = 256;

enum class Phrase { Chords, Melody };

struct Note { double at_s; int key; double velocity; double length_s; };

// Sixteen seconds at 120 BPM. Chords: a home-recorded pop piano part, I-vi-IV-V in C twice, the
// bass held through each bar and the right hand's triad struck on every beat. Melody: a celesta
// part over the same changes, a line from B4 to A5 in quarter notes over the triad, all struck
// short (0.3 s) without the pedal.
inline std::vector<Note> notes(Phrase phrase) {
  std::vector<Note> n;
  constexpr double beat = 0.5;
  static constexpr int roots[4] = {48, 45, 41, 43};
  static constexpr int triads[4][3] = {{60, 64, 67}, {60, 64, 69}, {60, 65, 69}, {59, 62, 67}};
  if (phrase == Phrase::Chords) {
    for (int bar = 0; bar < 8; ++bar) {
      const double t = bar * 4 * beat;
      const int c = bar % 4;
      n.push_back({t, roots[c], 0.75, 4 * beat - 0.05});
      for (int b = 0; b < 4; ++b)
        for (int key : triads[c]) n.push_back({t + b * beat, key, b == 0 ? 0.75 : 0.6, beat - 0.08});
    }
  } else {
    static constexpr int line[4][4] = {{76, 79, 77, 76}, {72, 76, 81, 79}, {77, 76, 74, 72}, {74, 71, 74, 79}};
    for (int bar = 0; bar < 8; ++bar) {
      const double t = bar * 4 * beat;
      const int c = bar % 4;
      for (int b = 0; b < 4; ++b) {
        n.push_back({t + b * beat, line[c][b], b % 2 == 0 ? 0.8 : 0.7, 0.3});
        for (int key : triads[c]) n.push_back({t + b * beat, key, 0.65, 0.3});
      }
    }
  }
  return n;
}

// A preset's case: the records and stem it stands in for, and what it plays.
struct Case { const char* preset; const char* group; const char* stem; Phrase phrase; };

inline const std::vector<Case>& cases() {
  static const std::vector<Case> c{
      {"Upright, four-track cassette", "GBV_lofi", "piano", Phrase::Chords},
      {"Upright, home studio tape", "OTC", "piano", Phrase::Chords},
      {"Celesta, home studio tape", "OTC", "piano", Phrase::Melody},
      {"Celesta, home studio tape", "OTC", "other", Phrase::Melody},
  };
  return c;
}

inline const record::Reference* reference(const char* group, const char* stem) {
  for (const auto& r : record::kReferences)
    if (std::strcmp(r.group, group) == 0 && std::strcmp(r.stem, stem) == 0) return &r;
  return nullptr;
}

inline long preset_index(const char* name) {
  const auto& all = schrodr::presets::all();
  for (size_t i = 0; i < all.size(); ++i) if (std::strcmp(all[i].name, name) == 0) return static_cast<long>(i);
  return -1;
}

struct Stereo { std::vector<float> left, right; };

inline Stereo play(const schrodr::Settings& s, Phrase phrase) {
  schrodr::Engine engine;
  engine.prepare(kRate);
  const size_t frames = static_cast<size_t>(kSeconds * kRate);
  Stereo out;
  out.left.assign(frames, 0.0f); out.right.assign(frames, 0.0f);
  struct Event { size_t at; int key; bool on; double velocity; };
  std::vector<Event> events;
  for (const Note& n : notes(phrase)) {
    events.push_back({static_cast<size_t>(n.at_s * kRate), n.key, true, n.velocity});
    events.push_back({static_cast<size_t>((n.at_s + n.length_s) * kRate), n.key, false, 0.0});
  }
  std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.at < b.at; });
  size_t next = 0;
  for (size_t done = 0; done < frames; done += kBlock) {
    const uint32_t n = static_cast<uint32_t>(std::min<size_t>(kBlock, frames - done));
    while (next < events.size() && events[next].at < done + n) {
      if (events[next].on) engine.note_on(s, 0, events[next].key, -1, events[next].velocity);
      else engine.note_off(s, 0, events[next].key, -1);
      ++next;
    }
    engine.render(s, out.left.data() + done, out.right.data() + done, n);
  }
  return out;
}

// Each measure has to land inside the records' interquartile range.
inline std::string judge(const record::Features& f, const record::Reference& r) {
  std::string failures;
  const auto within = [&](const char* name, double value, const record::Stat& s) {
    if (value >= s.q25 && value <= s.q75) return;
    char line[160];
    std::snprintf(line, sizeof(line), "%s %.4g outside %.4g..%.4g (records' median %.4g); ", name, value, s.q25, s.q75, s.median);
    failures += line;
  };
  within("centroid Hz", f.centroid_hz, r.centroid_hz);
  within("roll-off Hz", f.rolloff85_hz, r.rolloff85_hz);
  within("above 3 kHz dB", f.hf_share_db, r.hf_share_db);
  within("crest dB", f.crest100_db, r.crest100_db);
  within("L/R correlation", f.corr, r.corr);
  if (!f.has_wobble) failures += "no wobble measured (too few voiced runs); ";
  else within("wobble cents", f.wobble_cents, r.wobble_cents);
  return failures;
}

}  // namespace record_gate
