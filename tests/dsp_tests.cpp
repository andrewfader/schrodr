// Executable scenarios for features/dsp_building_blocks.feature: the temperaments, the limiter,
// the modal bank and the plucked shape, measured in cents, dB and samples.
#include "../src/dsp/core.hpp"
#include "../src/dsp/string.hpp"
#include "../src/dsp/temperament.hpp"
#include "../src/engine.hpp"
#include "audio_measure.hpp"
#include "spec.hpp"

#include <vector>

using namespace schrodr::dsp;

SCENARIO(temperaments_lay_out_the_octave_as_the_treatises_give) {
  GIVEN("the six temperaments laid out from C with A at 440 Hz");
  THEN("equal temperament puts every note on its equal-tempered pitch");
  bool equal = true;
  for (int n = 36; n <= 96; ++n) equal = equal && std::abs(temperament::hz(temperament::kEqual, 0, 440.0, n) - midi_to_hz(n)) < 1e-6;
  CHECK(equal, "equal");
  THEN("Pythagorean fifths are pure (702 cents) and its major third wide (408)");
  const auto cents = [](int kind, int root, int lo, int hi) { return 1200.0 * std::log2(temperament::hz(kind, root, 440.0, hi) / temperament::hz(kind, root, 440.0, lo)); };
  CHECK_NEAR(cents(temperament::kPythagorean, 0, 60, 67), 701.955, 0.1, "pure fifth");
  CHECK_NEAR(cents(temperament::kPythagorean, 0, 60, 64), 407.82, 0.1, "Pythagorean third");
  THEN("quarter-comma meantone's major third C-E is pure (386 cents) and its wolf G#-Eb howls (737)");
  CHECK_NEAR(cents(temperament::kMeantone, 0, 60, 64), 386.31, 0.1, "pure third");
  CHECK_NEAR(cents(temperament::kMeantone, 0, 68, 75), 737.6, 0.5, "the wolf");
  THEN("Werckmeister III keeps C-E at 390 cents and no fifth narrower than 696");
  CHECK_NEAR(cents(temperament::kWerckmeister, 0, 60, 64), 390.2, 0.3, "Werckmeister third");
  double narrowest = 800.0;
  for (int n = 60; n < 72; ++n) narrowest = std::min(narrowest, cents(temperament::kWerckmeister, 0, n, n + 7));
  CHECK(narrowest > 695.5, spec::fmt("narrowest fifth %.1f cents", narrowest));
  THEN("in every temperament and key, A4 stays on the fork");
  bool a_stays = true;
  for (int kind = 0; kind < temperament::kCount; ++kind) for (int root = 0; root < 12; ++root) a_stays = a_stays && std::abs(temperament::hz(kind, root, 440.0, 69) - 440.0) < 1e-6;
  CHECK(a_stays, "A4 at 440");
  WHEN("meantone is laid out from D instead of C");
  THEN("D-F# is the pure third and the wolf moves");
  CHECK_NEAR(cents(temperament::kMeantone, 2, 62, 66), 386.31, 0.1, "pure third from D");
  CHECK(std::abs(cents(temperament::kMeantone, 2, 68, 75) - 737.6) > 10.0, "the wolf is elsewhere");
}

SCENARIO(the_limiter_holds_the_ceiling_and_recovers) {
  GIVEN("the output limiter fed a sine 6 dB over the ceiling, then one 6 dB under");
  schrodr::Limiter lim;
  lim.prepare(48000.0);
  std::vector<float> hot(24000), cool(24000);
  for (size_t i = 0; i < hot.size(); ++i) { const float x = static_cast<float>(2.0 * schrodr::Limiter::kCeiling * std::sin(kTau * 440.0 * static_cast<double>(i) / 48000.0)); hot[i] = x * lim.gain(x, x); }
  const double held = lim.held_db();
  for (size_t i = 0; i < cool.size(); ++i) { const float x = static_cast<float>(0.5 * schrodr::Limiter::kCeiling * std::sin(kTau * 440.0 * static_cast<double>(i) / 48000.0)); cool[i] = x * lim.gain(x, x); }
  THEN("nothing leaves above the ceiling, the meter reads the 6 dB it took, and the quiet signal passes untouched once the gain has recovered");
  CHECK(measure::peak(measure::Span(hot)) <= schrodr::Limiter::kCeiling + 1e-4, "ceiling held");
  CHECK_NEAR(held, 6.0, 0.3, "reduction metered");
  CHECK_NEAR(measure::db(measure::peak(measure::Span(cool, 12000))), measure::db(0.5 * schrodr::Limiter::kCeiling), 0.1, "quiet signal untouched");
}

SCENARIO(a_modal_bank_is_its_resonators_summed) {
  GIVEN("three resonators at 100, 250 and 1000 Hz and a bank of the same three");
  string::ModalBank<8> bank;
  std::array<string::Resonator, 3> single{};
  const double hz[3] = {100.0, 250.0, 1000.0}, q[3] = {20.0, 35.0, 50.0}, gain[3] = {1.0, 0.5, -0.3};
  for (int i = 0; i < 3; ++i) { bank.add(hz[i], q[i], gain[i], 48000.0); single[static_cast<size_t>(i)].set(hz[i], q[i], gain[i], 48000.0); }
  WHEN("both are struck with an impulse");
  double worst = 0.0, peak = 0.0;
  std::vector<float> ring(48000);
  for (size_t n = 0; n < ring.size(); ++n) {
    const float x = n == 0 ? 1.0f : 0.0f;
    float sum = 0.0f;
    for (auto& r : single) sum += r.process(x);
    ring[n] = bank.process(x);
    worst = std::max(worst, static_cast<double>(std::abs(sum - ring[n])));
    peak = std::max(peak, static_cast<double>(std::abs(sum)));
  }
  THEN("the bank's output is the sum of the three, and the 250 Hz mode rings with its Q (T60 = 6.9 Q / (pi f))");
  CHECK(worst < 1e-6 * std::max(1.0, peak), spec::fmt("worst difference %.2e", worst));
  const double a = measure::tone_amplitude(measure::Span(ring, 0, 4800), 48000.0, 250.0), b = measure::tone_amplitude(measure::Span(ring, 9600, 4800), 48000.0, 250.0);
  const double t60 = 60.0 * 0.2 / measure::db(a / b);
  CHECK_NEAR(t60, 6.91 * 35.0 / (kPi * 250.0), 0.05, "T60 of the 250 Hz mode");
}

SCENARIO(the_plucked_shape_is_a_rounded_triangle_pinned_at_both_ends) {
  GIVEN("a triangle peaked a fifth of the way along a 300-sample string, rounded over 12 samples");
  string::Shape shape;
  shape.prepare(2048);
  shape.build(300.0, 60.0, 12.0);
  THEN("it is zero at both ends, peaks near the pluck point, and has no corner sharper than the rounding allows");
  CHECK(shape.at(0.0) == 0.0f && shape.at(300.0) == 0.0f, "pinned");
  double peak = 0.0; int at = 0;
  for (int i = 0; i <= 300; ++i) if (shape.at(i) > peak) { peak = shape.at(i); at = i; }
  CHECK(std::abs(at - 60) <= 6, spec::fmt("peak at %.0f", static_cast<double>(at)));
  double worst_kink = 0.0;
  for (int i = 2; i < 299; ++i) worst_kink = std::max(worst_kink, static_cast<double>(std::abs(shape.at(i + 1) - 2.0f * shape.at(i) + shape.at(i - 1))));
  CHECK(worst_kink < 2.0 / (60.0 * 12.0) * 1.5, spec::fmt("worst second difference %.5f", worst_kink));
}

int main(int argc, char** argv) { return spec::run(argc, argv); }
