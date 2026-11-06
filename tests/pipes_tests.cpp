// The organ (src/dsp/pipes.hpp), proved from its audio: every rank on its pitch, the stopped
// pipe's odd harmonics, the chiff, the tremulant, the stops, the wind and the chest's two
// sides. Scenarios: features/pipes.feature.
#include "../src/dsp/pipes.hpp"
#include "audio_measure.hpp"
#include "spec.hpp"

#include <vector>

using namespace schrodr::dsp;
using namespace schrodr::dsp::pipes;
using measure::Span;

namespace {
constexpr double kRate = 48000.0;
// One organ per suite: its voicing takes a moment.
Organ& organ() { static Organ* o = [] { auto* p = new Organ; p->prepare(kRate); return p; }(); return *o; }
struct Take { std::vector<float> left, right, mono; };
Take play(int key, Setup s, double hold_s, double tail_s) {
  Organ& o = organ();
  o.reset();
  o.configure(s);
  o.note_on(key, 0.7, 1);
  Take t;
  const size_t n = static_cast<size_t>((hold_s + tail_s) * kRate);
  t.left.resize(n); t.right.resize(n); t.mono.resize(n);
  for (size_t i = 0; i < n; ++i) {
    float l, r; o.render(l, r);
    t.left[i] = l; t.right[i] = r; t.mono[i] = 0.5f * (l + r);
    if (i + 1 == static_cast<size_t>(hold_s * kRate)) o.note_off(key);
  }
  return t;
}
Setup only(int rank) { Setup s; s.drawn = {rank == 0, rank == 1, rank == 2, rank == 3, rank == 4}; return s; }
double rms_db(const std::vector<float>& x, double from_s, double length_s) { return measure::db(measure::rms(Span(x, static_cast<size_t>(from_s * kRate), static_cast<size_t>(length_s * kRate)))); }
double harmonic_db(const std::vector<float>& x, double f, int n) {
  const Span s(x, 24000, 16384);
  return measure::db(measure::tone_amplitude(s, kRate, n * f) / std::max(measure::tone_amplitude(s, kRate, f), 1e-9));
}
}  // namespace

SCENARIO(every_rank_speaks_on_its_pitch) {
  GIVEN("each of the five ranks on C3, C4 and C5");
  double worst = 0.0;
  for (int rank = 0; rank < kRanks; ++rank)
    for (int key : {48, 60, 72}) {
      const Take t = play(key, only(rank), 1.2, 0.1);
      const double want = organ().pitch_at(key, rank);
      const double hz = measure::pitch_hz(Span(t.mono, 24000, 24000), kRate, 40.0, 6000.0, 0.3);
      const double cents = hz > 0.0 ? 1200.0 * std::log2(hz / want) : 1200.0;
      if (std::abs(cents) > 8.0) std::printf("    %s key %d: %.1f Hz against %.1f (%+.1f cents)\n", ranks()[static_cast<size_t>(rank)].name, key, hz, want, cents);
      worst = std::max(worst, std::abs(cents));
    }
  THEN("each sounds within 8 cents of the key's pitch at its footage, after the attack");
  CHECK(worst <= 8.0, ::spec::fmt("worst %.1f cents", worst));
  THEN("the 4' speaks an octave above the 8', the 2' two, the quint a twelfth");
  CHECK_NEAR(organ().pitch_at(60, 2) / organ().pitch_at(60, 0), 2.0, 1e-9, "4'");
  CHECK_NEAR(organ().pitch_at(60, 3) / organ().pitch_at(60, 0), 4.0, 1e-9, "2'");
  CHECK_NEAR(organ().pitch_at(60, 4) / organ().pitch_at(60, 0), 3.0, 1e-9, "2 2/3'");
}

SCENARIO(a_stopped_pipe_sounds_the_odd_harmonics) {
  GIVEN("C4 on the gedackt and on the open diapason");
  const Take gedackt = play(60, only(1), 1.2, 0.1), diapason = play(60, only(0), 1.2, 0.1);
  const double f = organ().pitch_at(60, 0);
  THEN("the gedackt's third harmonic stands more than 10 dB over its second; the diapason's second harmonic is within 30 dB of its fundamental");
  CHECK(harmonic_db(gedackt.mono, f, 3) - harmonic_db(gedackt.mono, f, 2) > 10.0, ::spec::fmt("gedackt h2 %.1f, h3 %.1f dB", harmonic_db(gedackt.mono, f, 2), harmonic_db(gedackt.mono, f, 3)));
  CHECK(harmonic_db(diapason.mono, f, 2) > -30.0, ::spec::fmt("diapason h2 %.1f dB", harmonic_db(diapason.mono, f, 2)));
  THEN("the stopped pipe's bore is half the open pipe's for the same pitch");
  CHECK_NEAR(organ().pipe(60, 1).bore / organ().pipe(60, 0).bore, 0.5, 0.06, "half the length");
}

SCENARIO(the_pipe_speaks_with_a_chiff_and_stops_with_the_wind) {
  GIVEN("C4 on the diapason with the chiff full and with none");
  Setup full = only(0); full.chiff = 1.0;
  Setup none = only(0); none.chiff = 0.0;
  const Take chiff = play(60, full, 0.6, 0.3), clean = play(60, none, 0.6, 0.3);
  THEN("the first 40 ms carry at least 10 dB more noise with the chiff, most of it above 3 kHz");
  CHECK(rms_db(chiff.mono, 0.0, 0.04) > rms_db(clean.mono, 0.0, 0.04) + 10.0, ::spec::fmt("chiff: attack %.1f dBFS against %.1f", rms_db(chiff.mono, 0.0, 0.04), rms_db(clean.mono, 0.0, 0.04)));
  CHECK(measure::band_fraction(Span(chiff.mono, 0, 1920), kRate, 3000.0, 20000.0) > 0.5, "mostly above 3 kHz");
  THEN("the pipe reaches its level within 150 ms and is 40 dB down 150 ms after the key is released");
  CHECK(rms_db(clean.mono, 0.15, 0.1) > rms_db(clean.mono, 0.45, 0.1) - 6.0, ::spec::fmt("speaks within 150 ms: %.1f dB against the held level", rms_db(clean.mono, 0.15, 0.1) - rms_db(clean.mono, 0.45, 0.1)));
  CHECK(rms_db(clean.mono, 0.75, 0.1) < rms_db(clean.mono, 0.45, 0.1) - 40.0, ::spec::fmt("%.1f dB down after release", rms_db(clean.mono, 0.75, 0.1) - rms_db(clean.mono, 0.45, 0.1)));
}

SCENARIO(the_tremulant_shakes_the_wind_and_the_stops_add_ranks) {
  GIVEN("C4 on the gedackt with the tremulant at 5 Hz");
  Setup trem = only(1); trem.tremulant = 1.0; trem.tremulant_hz = 5.0;
  const Take shaken = play(60, trem, 2.5, 0.1);
  std::vector<double> linear;
  for (double v : measure::contour_db(std::vector<float>(shaken.mono.begin() + 24000, shaken.mono.end()), kRate, 0.005, 0.01)) linear.push_back(std::pow(10.0, v / 20.0));
  THEN("its level wavers at 5 Hz");
  CHECK_NEAR(measure::modulation_hz(linear, 0.005, 2.0, 12.0), 5.0, 0.8, "tremulant rate");
  GIVEN("C4 on the diapason alone and with the 4' drawn");
  const Take one = play(60, only(0), 1.2, 0.1);
  Setup two = only(0); two.drawn[2] = true;
  const Take both = play(60, two, 1.2, 0.1);
  const double f = organ().pitch_at(60, 0);
  THEN("the octave above the key rises by more than 6 dB");
  CHECK(measure::db(measure::tone_amplitude(Span(both.mono, 24000, 16384), kRate, 2.0 * f) / std::max(measure::tone_amplitude(Span(one.mono, 24000, 16384), kRate, 2.0 * f), 1e-9)) > 6.0, "the 4' adds the octave");
  GIVEN("C4 and C#4 on the diapason, as a stereo pair");
  const Take c = play(60, only(0), 1.0, 0.1), cs = play(61, only(0), 1.0, 0.1);
  THEN("C stands on one side of the chest and C# on the other");
  const double c_bal = rms_db(c.left, 0.5, 0.4) - rms_db(c.right, 0.5, 0.4), cs_bal = rms_db(cs.left, 0.5, 0.4) - rms_db(cs.right, 0.5, 0.4);
  CHECK(c_bal * cs_bal < 0.0 && std::abs(c_bal) > 1.0, ::spec::fmt("C %+.1f dB, C# %+.1f dB left over right", c_bal, cs_bal));
}

SCENARIO(more_wind_is_louder_and_brighter) {
  GIVEN("C4 on the diapason at 70% and 130% wind");
  Setup low = only(0); low.wind = 0.7;
  Setup high = only(0); high.wind = 1.3;
  const Take soft = play(60, low, 1.2, 0.1), hard = play(60, high, 1.2, 0.1);
  THEN("the harder wind is louder and its spectrum sits higher");
  CHECK(rms_db(hard.mono, 0.5, 0.6) > rms_db(soft.mono, 0.5, 0.6) + 1.0, ::spec::fmt("%.1f dB louder", rms_db(hard.mono, 0.5, 0.6) - rms_db(soft.mono, 0.5, 0.6)));
  CHECK(measure::spectral_centroid(Span(hard.mono, 24000, 16384), kRate, 12) > measure::spectral_centroid(Span(soft.mono, 24000, 16384), kRate, 12), "brighter");
}

int main(int argc, char** argv) { return spec::run(argc, argv); }
