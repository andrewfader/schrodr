// SCHRODR held to real recordings (features/reference_recordings.feature, docs/QUALITY.md "The
// reference recordings"). The recordings' numbers are in reference_data.hpp, taken by
// schrodr-reference-extract with the measurements in reference_measure.hpp; this suite renders
// the same notes, measures them the same way and holds them to reference_compare.hpp's bar.
//
// A comparison SCHRODR does not yet meet is listed in reference_gaps.hpp with the reason. A gap
// that closes fails the suite until it is struck from the list, so the bar only ever rises.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../src/wav.hpp"
#include "reference_compare.hpp"
#include "reference_gaps.hpp"
#include "spec.hpp"

namespace {

using namespace reference;

constexpr double kTestRate = 44100.0;

// A deterministic floor 95 dB down (kSyntheticFloorDb RMS), so a synthetic note has a
// recording's noise under it. The measurements are told the floor, as the extractor tells them
// a recording's.
constexpr double kSyntheticFloorDb = -95.2;
std::vector<float> floor_noise(size_t n, uint32_t seed = 1) {
  std::vector<float> x(n);
  for (float& v : x) { seed = seed * 1664525u + 1013904223u; v = static_cast<float>((static_cast<double>(seed >> 8) / 16777216.0 - 0.5) * 6e-5); }
  return x;
}

// A stiff string's partials f_n = n F sqrt(1 + B n^2), each 1/n, under a two-stage decay.
std::vector<float> stiff_string(double f, double b, double early_db_s, double late_db_s) {
  std::vector<float> x = floor_noise(static_cast<size_t>(5.0 * kTestRate));
  const size_t on = static_cast<size_t>(0.1 * kTestRate);
  for (size_t i = on; i < x.size(); ++i) {
    const double t = static_cast<double>(i - on) / kTestRate;
    const double gain_db = t < 0.5 ? -early_db_s * t : -early_db_s * 0.5 - late_db_s * (t - 0.5);
    double v = 0.0;
    for (int n = 1; n <= 20 && n * f < 15000.0; ++n) v += std::sin(2.0 * measure::kPi * n * f * std::sqrt(1.0 + b * n * n) * t) / n;
    x[i] += static_cast<float>(0.2 * v * std::pow(10.0, gain_db / 20.0));
  }
  return x;
}

std::vector<float> bowed_tone(double f, double vibrato_cents, double vibrato_hz, double noise, uint32_t seed) {
  std::vector<float> x = floor_noise(static_cast<size_t>(3.0 * kTestRate), seed);
  uint32_t s = seed * 7 + 3;
  double phase = 0.0;
  const size_t on = static_cast<size_t>(0.1 * kTestRate), off = static_cast<size_t>(2.8 * kTestRate);
  for (size_t i = on; i < off; ++i) {
    const double t = static_cast<double>(i - on) / kTestRate;
    phase += 2.0 * measure::kPi * f * std::exp2(vibrato_cents / 1200.0 * std::sin(2.0 * measure::kPi * vibrato_hz * t)) / kTestRate;
    double v = 0.0;
    for (int n = 1; n <= 30 && n * f < 15000.0; ++n) v += std::sin(n * phase) / n;
    s = s * 1664525u + 1013904223u;
    v += noise * (static_cast<double>(s >> 8) / 16777216.0 - 0.5);
    x[i] += static_cast<float>(0.2 * v * std::min(1.0, t / 0.05));
  }
  return x;
}

std::vector<Ours>& ours() {
  static std::vector<Ours> all = measure_all();
  return all;
}

// Every comparison for one instrument: a listed gap is reported and not checked here (the last
// scenario checks that it is still a gap); anything else must meet the bar.
void hold_to_the_recordings(const char* instrument) {
  int compared = 0, gaps = 0;
  for (const Ours& o : ours()) {
    if (std::string(o.rec->instrument) != instrument) continue;
    for (const Check& c : compare(*o.rec, o.note, o.has_soft ? &o.soft : nullptr)) {
      ++compared;
      if (is_known_gap(c.id)) {
        ++gaps;
        std::printf("    gap  %-34s %s\n", c.id.c_str(), c.text.c_str());
        continue;
      }
      std::printf("    %s %-34s %s\n", c.pass ? "ok  " : "FAIL", c.id.c_str(), c.text.c_str());
      CHECK(c.pass, c.id + ": " + c.text);
    }
  }
  std::printf("    %d comparison(s), %d of them known gaps\n", compared, gaps);
  CHECK(compared > 0, std::string("there are recordings of the ") + instrument);
}

}  // namespace

SCENARIO(the_measurements_read_known_signals_correctly) {
  GIVEN("a synthetic stiff string, F 110 Hz and B 4e-4, falling 20 dB/s for half a second and 4 dB/s after");
  const Features s = measure_note(stiff_string(110.0, 4e-4, 20.0, 4.0), kTestRate, Kind::Struck, 45, kSyntheticFloorDb);
  THEN("the fitted B is within 5% of 4e-4 and the two decay rates within 15% of 20 and 4 dB/s");
  CHECK(s.ok, "the string is measured");
  CHECK_NEAR(s.inharmonicity, 4e-4, 2e-5, "B");
  CHECK_NEAR(s.decay_early, 20.0, 3.0, "prompt decay, dB/s");
  CHECK_NEAR(s.decay_late, 4.0, 0.6, "aftersound decay, dB/s");
  THEN("an envelope is 0 dB from itself, and from the same note measured again");
  CHECK_NEAR(envelope_distance(s.bands, s.bands), 0.0, 1e-9, "self distance");
  CHECK(envelope_distance(s.bands, measure_note(stiff_string(110.0, 4e-4, 20.0, 4.0), kTestRate, Kind::Struck, 45, kSyntheticFloorDb).bands) < 0.1,
        "the same note measured twice");

  GIVEN("a bar ringing at 1000 Hz with its first overtone at 2.756 f1, the fundamental falling 6 dB/s");
  std::vector<float> bar = floor_noise(static_cast<size_t>(3.0 * kTestRate));
  for (size_t i = static_cast<size_t>(0.1 * kTestRate); i < bar.size(); ++i) {
    const double t = static_cast<double>(i) / kTestRate - 0.1;
    bar[i] += static_cast<float>(0.3 * std::pow(10.0, -6.0 * t / 20.0) * std::sin(2.0 * measure::kPi * 1000.0 * t) +
                                 0.1 * std::pow(10.0, -40.0 * t / 20.0) * std::sin(2.0 * measure::kPi * 2756.0 * t));
  }
  const Features b = measure_note(bar, kTestRate, Kind::Bar, 84, kSyntheticFloorDb);
  THEN("the overtone reads within 0.5% and the decay within 10%");
  CHECK_NEAR(b.overtone_ratio, 2.756, 0.014, "overtone ratio");
  CHECK_NEAR(b.decay_early, 6.0, 0.6, "fundamental decay, dB/s");

  GIVEN("a bowed tone at 220 Hz with 20 cents of vibrato at 5.5 Hz, and the same with noise 10 dB louder");
  const Features v = measure_note(bowed_tone(220.0, 20.0, 5.5, 0.02, 3), kTestRate, Kind::Bowed, 57, kSyntheticFloorDb);
  const Features louder = measure_note(bowed_tone(220.0, 0.0, 5.5, 0.2, 3), kTestRate, Kind::Bowed, 57, kSyntheticFloorDb);
  const Features quieter = measure_note(bowed_tone(220.0, 0.0, 5.5, 0.2 / std::sqrt(10.0), 3), kTestRate, Kind::Bowed, 57, kSyntheticFloorDb);
  THEN("the vibrato reads 20 cents at 5.5 Hz, the bow noise rises 10 dB with the noise, and a clean tone reads under -45 dB");
  CHECK_NEAR(v.vibrato_cents, 20.0, 3.0, "vibrato depth, cents");
  CHECK_NEAR(v.vibrato_hz, 5.5, 0.3, "vibrato rate, Hz");
  CHECK_NEAR(louder.noise_db - quieter.noise_db, 10.0, 1.5, "bow noise rise, dB");
  CHECK(measure_note(bowed_tone(220.0, 0.0, 5.5, 0.0, 3), kTestRate, Kind::Bowed, 57, kSyntheticFloorDb).noise_db < -45.0, "a clean tone's bow noise");

  GIVEN("a scale of three bowed notes, A3, Bb3 and B3, with silences between them");
  std::vector<float> scale;
  for (int k = 57; k <= 59; ++k) {
    const auto note = bowed_tone(nominal_hz(k), 0.0, 5.5, 0.0, static_cast<uint32_t>(k));
    scale.insert(scale.end(), note.begin(), note.end());
  }
  Segment found{};
  THEN("the note finder picks Bb3 out of the middle");
  CHECK(find_note(scale, kTestRate, 58, found), "Bb3 is found");
  CHECK(found.begin > static_cast<size_t>(3.0 * kTestRate) && found.end <= static_cast<size_t>(6.0 * kTestRate), "in the second note");
}

// An AIFF image as the Iowa files are laid out: COMM (channels, frames, bits, the rate as an
// 80-bit extended float), INST, then SSND with big-endian samples.
std::vector<uint8_t> aiff_image(int bits, const std::vector<std::vector<int32_t>>& channels) {
  std::vector<uint8_t> b;
  const auto be = [&](uint64_t v, int bytes) { for (int i = bytes - 1; i >= 0; --i) b.push_back(static_cast<uint8_t>(v >> (8 * i))); };
  const size_t frames = channels[0].size(), bytes_per = static_cast<size_t>(bits / 8);
  const size_t ssnd = 8 + frames * channels.size() * bytes_per;
  b.insert(b.end(), {'F', 'O', 'R', 'M'}); be(4 + 26 + 28 + 8 + ssnd, 4); b.insert(b.end(), {'A', 'I', 'F', 'F'});
  b.insert(b.end(), {'C', 'O', 'M', 'M'}); be(18, 4); be(channels.size(), 2); be(frames, 4); be(static_cast<uint64_t>(bits), 2);
  b.insert(b.end(), {0x40, 0x0e, 0xac, 0x44, 0, 0, 0, 0, 0, 0});  // 44100 Hz
  b.insert(b.end(), {'I', 'N', 'S', 'T'}); be(20, 4); b.insert(b.end(), 20, 0);
  b.insert(b.end(), {'S', 'S', 'N', 'D'}); be(ssnd, 4); be(0, 4); be(0, 4);
  for (size_t f = 0; f < frames; ++f)
    for (const auto& ch : channels) be(static_cast<uint32_t>(ch[f]), static_cast<int>(bytes_per));
  return b;
}

SCENARIO(the_aiff_reader_reads_the_recordings_format) {
  GIVEN("a 16-bit and a 24-bit stereo AIFF at 44.1 kHz, with an INST chunk before the samples");
  schrodr::wav::Audio a16, a24;
  std::string error;
  const bool ok16 = schrodr::wav::parse_aiff(aiff_image(16, {{16384, -32768, 0}, {16384, 0, -1}}), a16, &error);
  const bool ok24 = schrodr::wav::parse_aiff(aiff_image(24, {{4194304, -8388608}, {-4194304, 8388607}}), a24, &error);
  THEN("both read at 44100 Hz, two channels mixed to mono, signs and scale intact");
  CHECK(ok16 && ok24, "both parse: " + error);
  CHECK_NEAR(a16.rate, 44100.0, 1e-9, "16-bit rate");
  CHECK(a16.source_channels == 2 && a16.mono.size() == 3, "16-bit layout");
  CHECK_NEAR(static_cast<double>(a16.mono[0]), 0.5, 1e-6, "16-bit first frame");
  CHECK_NEAR(static_cast<double>(a16.mono[1]), -0.5, 1e-6, "16-bit second frame");
  CHECK_NEAR(a24.rate, 44100.0, 1e-9, "24-bit rate");
  CHECK_NEAR(static_cast<double>(a24.mono[0]), 0.0, 1e-6, "24-bit first frame");
  CHECK_NEAR(static_cast<double>(a24.mono[1]), 0.0, 1e-6, "24-bit second frame (full scale both ways)");
  CHECK(!schrodr::wav::parse_aiff(std::vector<uint8_t>{'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'}, a16, &error), "a WAVE file is refused");
}

SCENARIO(the_recordings_were_measured) {
  GIVEN("the measured recordings in reference_data.hpp");
  std::map<std::string, int> per;
  for (const Recorded& r : recorded()) {
    ++per[r.instrument];
    CHECK(r.features.ok && r.features.f0 > 0.0, std::string(r.id) + " holds a measured note");
    CHECK(std::abs(measure::cents(r.features.f0, nominal_hz(r.key))) < 40.0, std::string(r.id) + " is the note its name says");
  }
  THEN("each is the note its name says, and every instrument held to recordings has at least four");
  for (const Instrument& i : instruments()) CHECK(per[i.name] >= 4, std::string(i.name) + " has recordings");
}

SCENARIO(the_grand_matches_the_iowa_steinway) {
  GIVEN("the grand at pp, mf and ff on B0, C2, C4, C6 and C7, dry, against the Iowa Steinway");
  THEN("its colour, brightness, dynamics, inharmonicity and decays meet the bar");
  hold_to_the_recordings("grand");
}

SCENARIO(the_glockenspiel_matches_the_iowa_orchestra_bells) {
  GIVEN("the glockenspiel's C6, G6 and C7 against the Iowa bells struck with plastic mallets");
  THEN("its brightness, dynamics, first overtone and ring meet the bar");
  hold_to_the_recordings("glockenspiel");
}

SCENARIO(the_violin_matches_the_iowa_violin) {
  GIVEN("the violin bowed without vibrato on each string, against the Iowa violin's non-vibrato scales");
  THEN("its colour, brightness, dynamics and bow noise meet the bar");
  hold_to_the_recordings("violin");
}

SCENARIO(the_viola_matches_the_iowa_viola) {
  GIVEN("the viola bowed without vibrato on each string, against the Iowa viola");
  THEN("its colour, brightness, dynamics and bow noise meet the bar");
  hold_to_the_recordings("viola");
}

SCENARIO(the_cello_matches_the_iowa_cello) {
  GIVEN("the cello bowed without vibrato on each string, against the Iowa cello");
  THEN("its colour, brightness, dynamics and bow noise meet the bar");
  hold_to_the_recordings("cello");
}

SCENARIO(the_upright_matches_the_vsco_upright) {
  GIVEN("the upright at pp, mf and f on C1 to C7, dry, against the VSCO-2 upright");
  THEN("its colour, brightness, dynamics, inharmonicity and decays meet the bar");
  hold_to_the_recordings("upright");
}

SCENARIO(the_harpsichords_match_the_vcsl_harpsichords) {
  GIVEN("the Flemish, Italian and French harpsichords on one 8' choir, C2 to B5, against VCSL's");
  THEN("their colour, brightness, inharmonicity and decays meet the bar");
  hold_to_the_recordings("flemish");
  hold_to_the_recordings("italian");
  hold_to_the_recordings("french");
}

SCENARIO(the_clavichord_matches_the_edinburgh_clavichord) {
  GIVEN("the unfretted clavichord, C2 to C6, against St Cecilia's Hall's of the 1740s at A = 415");
  THEN("its colour, brightness, inharmonicity and prompt decay meet the bar");
  hold_to_the_recordings("clavichord");
}

SCENARIO(the_struck_bars_match_their_recordings) {
  GIVEN("the celesta (Sonatina, C4 to C7), the electric piano (jRhodes3d, F1 to C7) and the toy piano (a Michelsonne, F4 to F5)");
  THEN("their brightness and ring meet the bar, the toy piano's first overtone, and the electric piano's colour");
  hold_to_the_recordings("celesta");
  hold_to_the_recordings("electric piano");
  hold_to_the_recordings("toy piano");
}

SCENARIO(the_organ_matches_the_vcsl_renaissance_organ) {
  GIVEN("the chamber organ's Gedackt 8', F#2 to C6, against VCSL's Renaissance organ 8'");
  THEN("its colour and brightness meet the bar");
  hold_to_the_recordings("organ");
}

SCENARIO(every_listed_gap_is_still_a_gap) {
  GIVEN("the comparisons listed in reference_gaps.hpp as not yet met");
  std::set<std::string> seen;
  for (const Ours& o : ours())
    for (const Check& c : compare(*o.rec, o.note, o.has_soft ? &o.soft : nullptr))
      if (is_known_gap(c.id)) {
        seen.insert(c.id);
        CHECK(!c.pass, c.id + " now meets the bar: strike it from reference_gaps.hpp (" + c.text + ")");
      }
  THEN("each still fails, and each names a comparison that exists");
  for (const Gap& g : known_gaps()) CHECK(seen.count(g.id), std::string(g.id) + " is not a comparison any more");
  std::map<std::string, int> causes;
  for (const Gap& g : known_gaps()) ++causes[g.why];
  std::printf("    %zu known gap(s), by cause:\n", known_gaps().size());
  for (const auto& [why, count] : causes) std::printf("    %3d  %s\n", count, why.c_str());
}

int main(int argc, char** argv) { return spec::run(argc, argv); }
