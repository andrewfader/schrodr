// The struck bars (src/dsp/bars.hpp), proved from their physics and their audio: the beam's
// mode ratios, each bar on its pitch, the length the pitch asks for, the mallet, the celesta's
// box, the tine's bark, dampers. Scenarios: features/bars.feature.
#include "../src/dsp/bars.hpp"
#include "audio_measure.hpp"
#include "spec.hpp"

#include <vector>

using namespace schrodr::dsp;
using namespace schrodr::dsp::bars;
using measure::Span;

namespace {
constexpr double kRate = 48000.0;
std::vector<float> play(int model, int key, double velocity, double hold_s, double tail_s, double hardness = 0.5, double sustain = 0.0) {
  Bars b;
  b.prepare(kRate);
  Setup s; s.model = model; s.hardness = hardness; s.sustain = sustain;
  b.configure(s);
  b.note_on(key, velocity, 3);
  std::vector<float> out(static_cast<size_t>((hold_s + tail_s) * kRate));
  for (size_t i = 0; i < out.size(); ++i) { float l, r; b.render(l, r); out[i] = 0.5f * (l + r); if (i + 1 == static_cast<size_t>(hold_s * kRate)) b.note_off(key); }
  return out;
}
double peak_hz(const std::vector<float>& x, double from_s, double length_s, double near, double span = 40.0) {
  const size_t a = static_cast<size_t>(from_s * kRate), n = static_cast<size_t>(length_s * kRate);
  double best = -1.0, best_hz = near;
  for (double c = -span; c <= span; c += 0.5) {
    const double f = near * std::exp2(c / 1200.0), step = kTau * f / kRate;
    double re = 0.0, im = 0.0;
    for (size_t i = 0; i < n && a + i < x.size(); ++i) {
      const double w = 0.5 - 0.5 * std::cos(kTau * static_cast<double>(i) / static_cast<double>(n - 1));
      re += w * x[a + i] * std::cos(step * static_cast<double>(i)); im -= w * x[a + i] * std::sin(step * static_cast<double>(i));
    }
    if (re * re + im * im > best) { best = re * re + im * im; best_hz = f; }
  }
  return best_hz;
}
double rms_db(const std::vector<float>& x, double from_s, double length_s) { return measure::db(measure::rms(Span(x, static_cast<size_t>(from_s * kRate), static_cast<size_t>(length_s * kRate)))); }
}  // namespace

SCENARIO(a_beams_modes_fall_where_euler_and_timoshenko_put_them) {
  GIVEN("a celesta plate and an electric piano tine designed for A4");
  const Design plate = design(bars::spec(kCelesta), 440.0), tine = design(bars::spec(kElectricPiano), 440.0);
  THEN("the free-free plate's modes sit at 1 : 2.76 : 5.40 of its pitch (a little under, for a thick plate), the clamped-free tine's at 1 : 6.27 : 17.5");
  CHECK_NEAR(plate.hz[0], 440.0, 0.01, "plate pitch");
  CHECK(plate.hz[1] / plate.hz[0] > 2.6 && plate.hz[1] / plate.hz[0] <= 2.757, ::spec::fmt("plate mode 2 at %.3f", plate.hz[1] / plate.hz[0]));
  CHECK(plate.hz[2] / plate.hz[0] > 5.0 && plate.hz[2] / plate.hz[0] <= 5.405, ::spec::fmt("plate mode 3 at %.3f", plate.hz[2] / plate.hz[0]));
  CHECK_NEAR(tine.hz[0], 440.0, 0.01, "tine pitch");
  CHECK(tine.hz[1] / tine.hz[0] > 6.0 && tine.hz[1] / tine.hz[0] <= 6.27, ::spec::fmt("tine mode 2 at %.3f", tine.hz[1] / tine.hz[0]));
  THEN("the lengths the pitch asks for are a celesta plate's and a tine's: about 20 cm and 7 cm");
  CHECK(plate.length_m > 0.12 && plate.length_m < 0.25, ::spec::fmt("plate %.3f m", plate.length_m));
  CHECK(tine.length_m > 0.05 && tine.length_m < 0.10, ::spec::fmt("tine %.3f m", tine.length_m));
  THEN("struck in the middle, a free-free plate's second mode (a node there) is not excited");
  CHECK(std::abs(plate.phi_strike[1]) < 0.05 && std::abs(plate.phi_strike[0]) > 0.5, "node of mode 2 at the centre");
}

SCENARIO(every_bar_sounds_at_its_pitch) {
  GIVEN("each of the five bar instruments struck softly at three keys of its compass");
  double worst = 0.0;
  for (int model = 0; model < kModelCount; ++model) {
    const Spec& s = bars::spec(model);
    for (int key : {s.low + 2, (s.low + s.high) / 2, s.high - 2}) {
      Bars b; b.prepare(kRate); Setup set; set.model = model; b.configure(set);
      b.note_on(key, 0.2, 1);
      std::vector<float> out(48000);
      for (auto& x : out) { float l, r; b.render(l, r); x = l; }
      const double cents = 1200.0 * std::log2(peak_hz(out, 0.1, 0.8, b.pitch_at(key)) / b.pitch_at(key));
      if (std::abs(cents) > 3.0) std::printf("    %s key %d: %+.1f cents\n", s.name, key, cents);
      worst = std::max(worst, std::abs(cents));
    }
  }
  THEN("each reads within 3 cents of its pitch");
  CHECK(worst <= 3.0, ::spec::fmt("worst %.1f cents", worst));
}

SCENARIO(a_glockenspiel_rings_and_a_celesta_is_damped) {
  GIVEN("C7 on the glockenspiel and C6 on the celesta, each released after half a second");
  const auto glock = play(kGlockenspiel, 96, 0.6, 0.5, 2.5), celesta = play(kCelesta, 84, 0.6, 0.5, 1.5), pedalled = play(kCelesta, 84, 0.6, 0.5, 1.5, 0.5, 1.0);
  THEN("the glockenspiel, with nothing to damp it, is still within 20 dB of its first half second two seconds later");
  CHECK(rms_db(glock, 2.5, 0.3) > rms_db(glock, 0.1, 0.3) - 20.0, ::spec::fmt("glockenspiel %.1f dB down", rms_db(glock, 2.5, 0.3) - rms_db(glock, 0.1, 0.3)));
  THEN("the celesta's damper stops its plate: half a second after release it is 40 dB down, unless the pedal holds the dampers up");
  CHECK(rms_db(celesta, 1.0, 0.2) < rms_db(celesta, 0.1, 0.3) - 40.0, ::spec::fmt("celesta %.1f dB down", rms_db(celesta, 1.0, 0.2) - rms_db(celesta, 0.1, 0.3)));
  CHECK(rms_db(pedalled, 1.0, 0.2) > rms_db(pedalled, 0.1, 0.3) - 20.0, "the pedal lets it ring");
  THEN("the celesta's resonator box carries the fundamental: it stands more than 10 dB over the plate's third mode");
  CHECK(measure::db(measure::tone_amplitude(Span(celesta, 4800, 16384), kRate, 1046.5) / std::max(measure::tone_amplitude(Span(celesta, 4800, 16384), kRate, 1046.5 * 5.1), 1e-9)) > 10.0, "fundamental over the clank");
}

SCENARIO(the_tine_barks_and_the_toy_piano_clanks) {
  GIVEN("C4 on the electric piano struck softly and as hard as it goes");
  const auto soft = play(kElectricPiano, 60, 0.15, 0.6, 0.1), hard = play(kElectricPiano, 60, 1.0, 0.6, 0.1);
  const auto ratio = [](const std::vector<float>& x) {
    const Span s(x, 2400, 16384);
    return measure::tone_amplitude(s, kRate, 2.0 * 261.63) / std::max(measure::tone_amplitude(s, kRate, 261.63), 1e-9);
  };
  THEN("the hard note's second harmonic stands at least 6 dB higher against its fundamental: the pickup's field folds the tine's swing");
  CHECK(measure::db(ratio(hard) / std::max(ratio(soft), 1e-9)) > 6.0, ::spec::fmt("bark: 2nd/1st %.3f against %.3f", ratio(hard), ratio(soft)));
  GIVEN("A4 on the toy piano");
  const auto toy = play(kToyPiano, 69, 0.7, 0.5, 0.5);
  THEN("its rod's second mode rings at 6.27 times the pitch, within 25 dB of it: the clank a toy piano has and a piano does not");
  const Span s(toy, 1200, 8192);
  const double clank = measure::tone_amplitude(s, kRate, design(bars::spec(kToyPiano), 440.0).hz[1]);
  CHECK(measure::db(clank / std::max(measure::tone_amplitude(s, kRate, 440.0), 1e-9)) > -25.0, ::spec::fmt("clank %.1f dB re the pitch", measure::db(clank / std::max(measure::tone_amplitude(s, kRate, 440.0), 1e-9))));
}

SCENARIO(a_harder_mallet_is_brighter_and_the_contact_shorter) {
  GIVEN("C5 on the celesta with a soft and a hard mallet");
  Bars soft, hard;
  soft.prepare(kRate); hard.prepare(kRate);
  Setup s; s.model = kCelesta; s.hardness = 0.1; soft.configure(s);
  s.hardness = 0.9; hard.configure(s);
  soft.note_on(72, 0.6, 1); hard.note_on(72, 0.6, 1);
  std::vector<float> a(24000), b(24000);
  for (size_t i = 0; i < a.size(); ++i) { float l, r; soft.render(l, r); a[i] = l; hard.render(l, r); b[i] = l; }
  THEN("the hard mallet stays on the plate for less time and its attack carries the plate's third mode (the tink) at least 10 dB stronger");
  CHECK(hard.last_contact_s() < soft.last_contact_s(), ::spec::fmt("contact %.2f ms hard, %.2f ms soft", hard.last_contact_s() * 1000.0, soft.last_contact_s() * 1000.0));
  const auto d = design(bars::spec(kCelesta), 523.25);
  const auto tink = [&](const std::vector<float>& x) { const Span att(x, 0, 2048); return measure::db(measure::tone_amplitude(att, kRate, d.hz[2]) / std::max(measure::tone_amplitude(att, kRate, d.hz[0]), 1e-12)); };
  CHECK(tink(b) > tink(a) + 10.0, ::spec::fmt("third mode re the fundamental: %.1f dB hard, %.1f dB soft", tink(b), tink(a)));
}

int main(int argc, char** argv) { return spec::run(argc, argv); }
