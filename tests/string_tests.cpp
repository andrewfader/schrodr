// Executable scenarios for features/strings.feature: one string's physics and its waveguide,
// proved in newtons, cents and seconds: tension from the wire and the pitch, inharmonicity,
// the loss fit's decay rates, the dispersion fit's partials and the loop's tuning.
#include "../src/dsp/string.hpp"
#include "audio_measure.hpp"
#include "spec.hpp"

#include <vector>

using namespace schrodr::dsp;
using namespace schrodr::dsp::string;

namespace {
constexpr double kRate = 48000.0;

// A bare loop with a rigid bridge, given a bump near the nut; returns the wave arriving at the
// bridge, sample by sample.
std::vector<float> ring(const LoopDesign& d, double period, double seconds, double bump = 1e-3) {
  Plane p;
  p.prepare(4096);
  p.design(d);
  const double fd = loop_delay(d, kTau / period);
  const size_t lower = static_cast<size_t>(std::max(2.0, std::floor(period * 0.5 - fd - 1.0)));
  const double upper = std::max(1.0, period - 1.0 - static_cast<double>(lower) - fd);
  for (size_t i = 0; i < 12 && i < static_cast<size_t>(period * 0.05); ++i) p.upper().slot(i) += static_cast<float>(bump);
  std::vector<float> out(static_cast<size_t>(seconds * kRate));
  const double bridge = std::floor(upper), nut_frac = upper - bridge;
  for (auto& x : out) { float f; x = p.arrive(bridge, f); p.finish(-f, lower, nut_frac, 1.0f, 0.0f); }
  return out;
}

double peak_hz(const std::vector<float>& x, double from_s, double length_s, double near, double span = 40.0) {
  const size_t a = static_cast<size_t>(from_s * kRate), n = static_cast<size_t>(length_s * kRate);
  double best = -1.0, best_hz = near;
  for (double c = -span; c <= span; c += 0.25) {
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
}  // namespace

SCENARIO(a_strings_tension_and_stiffness_follow_from_its_wire_and_pitch) {
  GIVEN("middle C's string on a concert grand: 0.62 m of 1.0 mm music wire at 261.6 Hz");
  const double d = 1.0e-3, mu = 7850.0 * kPi * d * d / 4.0;
  const Physics p = physics(0.62, 261.63, d, mu, Core::Steel, false);
  THEN("its tension is mu (2 L f)^2 within the stiffness term's percent, about 650 N, and its impedance sqrt(T mu)");
  CHECK_NEAR(p.tension, mu * std::pow(2.0 * 0.62 * 261.63, 2.0), 0.02 * p.tension, "tension");
  CHECK(p.tension > 600.0 && p.tension < 720.0, ::spec::fmt("%.0f N", p.tension));
  CHECK_NEAR(p.impedance, std::sqrt(p.tension * mu), 1e-6, "impedance");
  THEN("its inharmonicity B = pi^2 E I / (T L^2) is a few times 1e-4, as measured on grands");
  CHECK_NEAR(p.inharmonicity, kPi * kPi * p.ei / (p.tension * 0.62 * 0.62), 1e-9, "B formula");
  CHECK(p.inharmonicity > 2e-4 && p.inharmonicity < 6e-4, ::spec::fmt("B = %.2e", p.inharmonicity));
  THEN("its 8th partial sits sharp by 1200 log2(sqrt((1 + 64 B) / (1 + B))) cents of eight times the fundamental");
  CHECK_NEAR(1200.0 * std::log2(partial_hz(261.63, p.inharmonicity, 8) / (8.0 * 261.63)), 1200.0 * std::log2(std::sqrt((1.0 + 64.0 * p.inharmonicity) / (1.0 + p.inharmonicity))), 1e-6, "stretch of partial 8");
  GIVEN("a harpsichord's c'' string: 0.36 m of 0.25 mm iron at 523 Hz");
  const double di = 0.25e-3, mui = 7800.0 * kPi * di * di / 4.0;
  const Physics h = physics(0.36, 523.25, di, mui, Core::Iron, false);
  THEN("it pulls about 55 N, a tenth of the piano's, and its B is a few times 1e-5");
  CHECK(h.tension > 45.0 && h.tension < 65.0, ::spec::fmt("%.1f N", h.tension));
  CHECK(h.inharmonicity < 1e-4, ::spec::fmt("B = %.2e", h.inharmonicity));
}

SCENARIO(a_strings_own_losses_rise_with_frequency) {
  GIVEN("the grand's C4 string (plain) and A1 string (wound) from their physics");
  const double d = 1.0e-3, mu = 7850.0 * kPi * d * d / 4.0;
  const Physics plain = physics(0.62, 261.63, d, mu, Core::Steel, false);
  const Physics wound = physics(1.75, 55.0, 1.1e-3, 0.03, Core::Steel, true, 2.5);
  THEN("the decay rate of the 8th partial exceeds the fundamental's on both, the wound string's by the more (its winding rubs)");
  const double p1 = own_rate(plain, plain.hz), p8 = own_rate(plain, 8.0 * plain.hz);
  const double w1 = own_rate(wound, wound.hz), w8 = own_rate(wound, 8.0 * wound.hz);
  CHECK(p8 > p1 && w8 > w1, ::spec::fmt("plain %.3f -> %.3f s^-1, wound rises %.1f-fold", p1, p8, w8 / w1));
  CHECK(w8 / w1 > p8 / p1, "the winding's friction grows with frequency");
  THEN("a buff stop's leather adds loss in proportion to frequency");
  CHECK(own_rate(plain, 2000.0, 40.0) - own_rate(plain, 2000.0) > own_rate(plain, 500.0, 40.0) - own_rate(plain, 500.0), "extra loss grows with frequency");
}

SCENARIO(the_loss_fit_makes_a_loop_decay_at_the_strings_rates) {
  GIVEN("a 220 Hz loop whose loss filter is fitted to decay rates of 1.5 s^-1 at the fundamental rising to 12 s^-1 at the 8th partial");
  LoopDesign d;
  std::array<double, 16> rates{};
  for (int n = 1; n <= 16; ++n) rates[static_cast<size_t>(n - 1)] = 1.5 + 1.5 * (n - 1);
  fit_loss_curve(d, 220.0, rates.data(), 16, kRate);
  const auto out = ring(d, kRate / 220.0, 3.0);
  const auto t60 = [&](double hz) {
    const double a = measure::tone_amplitude(measure::Span(out, 4800, 14400), kRate, hz), b = measure::tone_amplitude(measure::Span(out, 24000, 14400), kRate, hz);
    return 6.91 * (0.4 / std::log(a / b));  // 1 / rate
  };
  THEN("the fundamental's decay rate comes out within 15% and the 8th partial's within 30%");
  CHECK_NEAR(6.91 / t60(220.0), 1.5, 0.23, "fundamental rate");
  CHECK_NEAR(6.91 / t60(8.0 * 220.0), 12.0, 3.6, "8th partial rate");
  THEN("no frequency comes round with more than it went in");
  CHECK(d.gain * (1.0 - d.pole) * (1.0 - d.lp) / (1.0 + d.zero) * (1.0 + d.zero) / ((1.0 - d.pole) * (1.0 - d.lp)) <= 1.0 + 1e-6, "passive at DC");
}

SCENARIO(the_dispersion_fit_puts_the_partials_where_the_stiff_string_has_them) {
  GIVEN("loops at 261.6 Hz with B = 4e-4 (a grand's middle C) and at 2093 Hz with B = 7e-3 (its top octave)");
  for (auto [f1, b, tolerance] : {std::tuple{261.63, 4.0e-4, 4.0}, std::tuple{2093.0, 7.0e-3, 8.0}}) {
    LoopDesign d;
    d.gain = 0.9995;
    fit_dispersion(d, f1, b, kRate);
    const auto out = ring(d, kRate / f1, 1.5);
    double worst = 0.0;
    for (int n = 2; n <= 6; ++n) {
      const double want = partial_hz(f1, b, n);
      if (want > kRate * 0.45) break;
      const double got = peak_hz(out, 0.1, 1.2, want, 60.0);
      worst = std::max(worst, std::abs(1200.0 * std::log2(got / want)));
    }
    THEN("partials 2 to 6 land within tolerance of n f1 sqrt((1 + B n^2) / (1 + B))");
    CHECK(worst <= tolerance, ::spec::fmt("f1 %.0f: worst %.1f cents with %.0f sections", f1, worst, static_cast<double>(d.sections)));
    THEN("and the fundamental itself is tuned by the delay, within a cent");
    CHECK(std::abs(1200.0 * std::log2(peak_hz(out, 0.1, 1.2, f1) / f1)) < 1.0, "fundamental");
  }
  THEN("a string with no stiffness to speak of gets no allpasses");
  LoopDesign none;
  fit_dispersion(none, 261.63, 1e-7, kRate);
  CHECK(none.sections == 0, "no sections for a harmonic string");
}

SCENARIO(a_force_at_a_point_launches_a_plateau_both_ways) {
  GIVEN("a 300-sample string at rest, pushed once at 40 cells from the nut by a displacement of 1, laid to land on the point at the next sample");
  Plane p;
  p.prepare(1024);
  LoopDesign d; d.gain = 1.0;
  p.design(d);
  const size_t lower = 149, bridge = 150;
  // The plateau as the keyboard lays it: +delta on the upper rail from the nut to one cell short
  // of the point, -delta on the lower rail from the point's own cell to the nut.
  for (size_t i = 0; i + 1 <= 40; ++i) p.upper().slot(i) += 1.0f;
  for (size_t i = bridge - 40; i <= lower; ++i) p.lower().slot(i) -= 1.0f;
  WHEN("one sample passes");
  { float f; p.arrive(static_cast<double>(bridge), f); p.finish(-f, lower, 0.0, 1.0f, 0.0f); }
  THEN("the displacement at the point reads 1, its neighbours 0, and 20 cells toward the bridge 0: nothing has arrived there yet");
  CHECK_NEAR(static_cast<double>(p.at_point(40.0, bridge)), 1.0, 1e-6, "at the point");
  CHECK_NEAR(static_cast<double>(p.at_point(39.0, bridge)), 0.0, 1e-6, "one cell nutward");
  CHECK_NEAR(static_cast<double>(p.at_point(41.0, bridge)), 0.0, 1e-6, "one cell bridgeward");
  CHECK_NEAR(static_cast<double>(p.at_point(60.0, bridge)), 0.0, 1e-6, "ahead of the front");
  WHEN("20 more samples pass");
  for (int i = 0; i < 20; ++i) { float f; p.arrive(static_cast<double>(bridge), f); p.finish(-f, lower, 0.0, 1.0f, 0.0f); }
  THEN("the plateau has spread 20 cells each way: the point still reads 1, 60 cells from the nut reads 1, 61 reads 0");
  CHECK_NEAR(static_cast<double>(p.at_point(60.0, bridge)), 1.0, 1e-5, "the front has passed");
  CHECK_NEAR(static_cast<double>(p.at_point(40.0, bridge)), 1.0, 1e-5, "the point");
  CHECK_NEAR(static_cast<double>(p.at_point(61.0, bridge)), 0.0, 1e-5, "ahead of the front");
  GIVEN("the same string with a viscous damper of the string's own impedance at 73 cells, plucked");
  Plane q; q.prepare(1024); q.design(d);
  const double x = 73.0, z = 0.156, rate = 48000.0;
  const auto push = [&](float delta) { const size_t to = bridge - 73; for (size_t i = 0; i + 1 <= to; ++i) q.lower().slot(i) += delta; for (size_t i = 73; i <= bridge; ++i) q.upper().slot(i) -= delta; };
  push(1e-4f);
  const auto energy = [&]() { double e = 0.0; for (size_t k = 0; k <= bridge; ++k) { const double a = q.at_point(static_cast<double>(k), bridge); e += a * a; } return e; };
  const double before = energy();
  for (int t = 0; t < 300 * 12; ++t) {
    const double y_now = q.at_point(x, bridge), y_next = q.upper().linear(x - 1.0) + q.lower().linear(static_cast<double>(bridge) - x - 1.0);
    const double v_h = (y_next - y_now) * rate, f = -z * v_h / (1.0 + 0.5);
    push(static_cast<float>(f / (2.0 * z) / rate));
    float g; q.arrive(static_cast<double>(bridge), g); q.finish(-g, lower, 0.0, 1.0f, 0.0f);
  }
  THEN("twelve periods on, the damper has taken most of the energy: the model is passive at a point (a push laid where the point is now would feed it)");
  CHECK(energy() < 0.1 * before, ::spec::fmt("energy %.2e of %.2e", energy(), before));
}

int main(int argc, char** argv) { return spec::run(argc, argv); }
