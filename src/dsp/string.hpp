// One vibrating string, from its physics: a digital waveguide (Smith; Karjalainen, Välimäki and
// Tolonen) carrying displacement waves on two rails, nut to bridge and back, in each of two
// transverse planes (normal to the soundboard, which drives it and so loses energy fast, and
// parallel to it, which barely moves the bridge and rings on). The loop loses energy through a
// filter fitted to the string's own loss rates (air drag, the wire's internal friction and its
// bending losses) and is dispersed by an allpass chain fitted to the core's bending stiffness, so
// the partials sit sharp by f_n = n f1 sqrt((1 + B n^2) / (1 + B)), the inharmonicity a piano's
// tuner stretches the octaves for. The bridge itself is not in here: a string hands its arriving
// wave to the instrument's bridge, which answers with the bridge's displacement (the same for
// every string on it, which is how unison strings couple and sympathetic strings ring), and the
// string sends the inverted wave plus that displacement back toward the nut.
//
// Exciters read the string's displacement at a point and push force into it there: a hammer, a
// plectrum lifting it, a tangent, a rosined wheel. The string is excited at a physical position
// (metres from the nut) on both rails at once, as a force at a point on a real string sends
// waves both ways.
//
// Sources: Fletcher and Rossing, The Physics of Musical Instruments (string losses, inharmonicity,
// Weinreich's coupled strings); Bank, Physics-based Sound Synthesis of String Instruments (2006);
// Rauhala and Välimäki, "Tunable dispersion filter design for piano synthesis" (2006); Woodhouse,
// "Plucked guitar transients" (2004) for the loss model's form.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "core.hpp"

namespace schrodr::dsp::string {

constexpr double kAirDensity = 1.2;       // kg/m^3 at 20 C
constexpr double kAirViscosity = 1.8e-5;  // Pa s

// --- materials ---------------------------------------------------------------------------------------

enum class Core { Steel, Iron, Brass, Gut, Nylon };

struct Material {
  double young;      // Pa
  double density;    // kg/m^3
  double eta_f;      // frequency-independent internal loss factor
  double eta_b;      // bending (frequency-squared) loss factor
};

// Music wire is drawn high-carbon steel; 18th-century iron wire was softer and lossier; brass
// (and the yellow brass of harpsichord basses) heavier and much lossier; gut lossiest of all.
inline Material material(Core core) {
  switch (core) {
    case Core::Iron: return {2.0e11, 7800.0, 1.8e-4, 4e-4};
    case Core::Brass: return {1.1e11, 8500.0, 3.0e-4, 6e-4};
    case Core::Gut: return {3.0e9, 1300.0, 2.0e-3, 2e-2};
    case Core::Nylon: return {4.5e9, 1150.0, 2.5e-4, 2e-2};
    default: return {2.0e11, 7850.0, 7.0e-5, 3e-4};
  }
}

// --- one string's physics ---------------------------------------------------------------------------

struct Physics {
  double length = 0.6;      // speaking length, m
  double hz = 261.6;        // the fundamental it is tuned to
  double core_diameter = 1e-3;
  double mu = 0.0;          // kg/m, winding included
  double tension = 0.0;     // N
  double ea = 0.0, ei = 0.0;
  double impedance = 0.0;   // sqrt(T mu), kg/s
  double inharmonicity = 0.0;  // B = pi^2 E I / (T L^2)
  bool wound = false;
  Material material{};
  double wrap_loss = 1.0;   // extra internal friction between a winding's turns (1: a plain string)
};

// A string of speaking length `length` tuned to `hz`: the core's diameter sets its stiffness,
// `mu` (the core plus any winding) its mass; the tension follows from the pitch. A string tuned
// over its stiffness vibrates as if a little shorter (it bends over sqrt(E I / T) at each end
// where it is held), so the tension that brings it to pitch is found by a few fixed-point steps.
inline Physics physics(double length, double hz, double core_diameter, double mu, Core core, bool wound, double wrap_loss = 1.0) {
  Physics p;
  p.length = length; p.hz = hz; p.core_diameter = core_diameter; p.mu = mu; p.wound = wound; p.wrap_loss = wrap_loss;
  p.material = material(core);
  const double d = core_diameter;
  p.ea = p.material.young * kPi * d * d / 4.0;
  p.ei = p.material.young * kPi * d * d * d * d / 64.0;
  double tension = mu * std::pow(2.0 * length * hz, 2.0);
  for (int i = 0; i < 6; ++i) {
    const double bend = std::sqrt(p.ei / std::max(tension, 1e-3));
    tension = mu * std::pow(2.0 * std::max(length - 2.0 * 0.5 * bend, 0.2 * length) * hz, 2.0);
  }
  p.tension = tension;
  p.impedance = std::sqrt(tension * mu);
  p.inharmonicity = kPi * kPi * p.ei / (tension * length * length);
  return p;
}

// The pitch of partial n of a stiff string with inharmonicity b tuned so partial 1 is at f1.
inline double partial_hz(double f1, double b, int n) { return n * f1 * std::sqrt((1.0 + b * n * n) / (1.0 + b)); }

// Air drag on a vibrating cylinder (Stokes, in Valette's form), as a decay rate in s^-1.
inline double air_rate(const Physics& p, double hz) {
  const double d = p.wound ? p.core_diameter * 2.2 : p.core_diameter;  // the winding's outer diameter, roughly
  const double r = 2.0 * kPi * kAirViscosity + 2.0 * kPi * d * std::sqrt(kPi * kAirViscosity * kAirDensity * hz);
  return r / (2.0 * p.mu);
}

// The string's own amplitude decay rate (s^-1) of the partial at `hz`, apart from the bridge:
// air drag plus the wire's internal losses (Woodhouse's form, the bending loss weighted by the
// share of the restoring force stiffness provides at that wavenumber), plus any extra damping a
// cloth, a leather buff or a felt applies (`extra`, s^-1 per kHz).
inline double own_rate(const Physics& p, double hz, double extra_per_khz = 0.0) {
  const double w = kTau * hz;
  const double k = kPi * (hz / p.hz) / p.length;
  const double stiff = p.ei * k * k;
  const double eta = (p.tension * p.material.eta_f * p.wrap_loss + stiff * p.material.eta_b) / (p.tension + stiff);
  return air_rate(p, hz) + 0.5 * w * eta + extra_per_khz * hz / 1000.0;
}

// --- signal pieces -------------------------------------------------------------------------------

// One rail of a waveguide: a power-of-two circular buffer read at integer, linear or four-point
// Lagrange fractional delays, writable in place (for the excitations).
class Rail {
 public:
  void prepare(size_t capacity) {
    size_t n = 16;
    while (n < capacity + 8) n <<= 1;
    buffer_.assign(n, 0.0f);
    mask_ = n - 1;
    write_ = 0;
  }
  void clear() { std::fill(buffer_.begin(), buffer_.end(), 0.0f); }
  size_t capacity() const { return buffer_.size() - 8; }
  void push(float x) { write_ = (write_ + 1) & mask_; buffer_[write_] = x; }
  // The sample pushed `k` pushes ago (0: the latest).
  float at(size_t k) const { return buffer_[(write_ - k) & mask_]; }
  float& slot(size_t k) { return buffer_[(write_ - k) & mask_]; }
  float linear(double d) const {
    d = std::clamp(d, 0.0, static_cast<double>(capacity()));
    const size_t i = static_cast<size_t>(d);
    const float f = static_cast<float>(d - static_cast<double>(i));
    return at(i) + (at(i + 1) - at(i)) * f;
  }
  // Third-order Lagrange: flat through the band a string spends its energy in, and smooth under
  // a delay that moves every sample (tension glide, bebung, bend).
  float lagrange(double d) const {
    d = std::clamp(d, 1.0, static_cast<double>(capacity() - 2));
    const size_t i = static_cast<size_t>(d);
    const double t = d - static_cast<double>(i) + 1.0;
    const double h0 = -(t - 1.0) * (t - 2.0) * (t - 3.0) / 6.0;
    const double h1 = t * (t - 2.0) * (t - 3.0) / 2.0;
    const double h2 = -t * (t - 1.0) * (t - 3.0) / 2.0;
    const double h3 = t * (t - 1.0) * (t - 2.0) / 6.0;
    return static_cast<float>(h0 * at(i - 1) + h1 * at(i) + h2 * at(i + 1) + h3 * at(i + 2));
  }
  void add(double d, float x) {
    d = std::clamp(d, 0.0, static_cast<double>(capacity()));
    const size_t i = static_cast<size_t>(d);
    const float f = static_cast<float>(d - static_cast<double>(i));
    slot(i) += x * (1.0f - f);
    slot(i + 1) += x * f;
  }
 private:
  std::vector<float> buffer_;
  size_t mask_ = 0, write_ = 0;
};

// The loop's filters for one plane: a loss filter fitted to the string's decay at every partial
// in the band, then a chain of identical first-order allpasses fitted to its stiffness dispersion.
constexpr int kSplitSections = 2;
constexpr int kMaxDispersion = 16;
struct LoopDesign {
  // Loss: a shelf (pole and zero) times a gentle one-pole low-pass, times a gain:
  //   H = gain (1 - pole) / (1 - pole z^-1) * (1 + zero z^-1) / (1 + zero) * (1 - lp) / (1 - lp z^-1)
  double gain = 0.999, pole = 0.0, zero = 0.0, lp = 0.0;
  double allpass = 0.0;   // dispersion coefficient (negative: high partials come round early)
  int sections = 0;
  // Second-order allpasses (a1, a2) that give one plane a slightly different, irregular phase:
  // each partial's polarization split is its own.
  std::array<std::array<double, 2>, kSplitSections> split{};
};

// Phase delays (samples) at w (rad/sample).
inline double loss_delay(const LoopDesign& r, double w) {
  const double pole = r.pole == 0.0 ? 0.0 : std::atan2(r.pole * std::sin(w), 1.0 - r.pole * std::cos(w));
  const double zero = r.zero == 0.0 ? 0.0 : std::atan2(r.zero * std::sin(w), 1.0 + r.zero * std::cos(w));
  const double lp = r.lp == 0.0 ? 0.0 : std::atan2(r.lp * std::sin(w), 1.0 - r.lp * std::cos(w));
  return (pole + zero + lp) / w;
}
inline double allpass_delay(double c, double w) {
  return 1.0 - 2.0 * std::atan2(c * std::sin(w), 1.0 + c * std::cos(w)) / w;
}
inline double allpass2_delay(double a1, double a2, double w) {
  if (a1 == 0.0 && a2 == 0.0) return 2.0;
  const std::complex<double> z1 = std::polar(1.0, -w), z2 = z1 * z1;
  const std::complex<double> h = (a2 + a1 * z1 + z2) / (1.0 + a1 * z1 + a2 * z2);
  double phase = std::arg(h);
  while (phase > 0.0) phase -= kTau;
  if (w > 1e-9 && phase < -kTau + 1e-12) phase += kTau;
  return -phase / w;
}
inline double loop_delay(const LoopDesign& r, double w) {
  double d = loss_delay(r, w) + (r.sections > 0 ? r.sections * allpass_delay(r.allpass, w) : 0.0);
  for (const auto& s : r.split) if (s[0] != 0.0 || s[1] != 0.0) d += allpass2_delay(s[0], s[1], w);
  return d;
}

// Fits the loss filter so one pass (one round trip of the string) loses exactly the decay rate
// rates[0] at the fundamental and the rates of the partials above it (rates[n - 1] for partial
// n, up to `count`) as closely as it can, in least squares on the dB lost per pass.
inline void fit_loss_curve(LoopDesign& r, double f0, const double* rates, int count, double rate) {
  constexpr int kMax = 96;
  count = std::min(count, kMax);
  std::array<double, kMax + 1> cw{};
  for (int n = 1; n <= count; ++n) cw[static_cast<size_t>(n)] = std::cos(std::min(kTau * f0 * n / rate, kPi * 0.95));
  const auto zero_for = [](double a, double plateau) { return ((1.0 - a) - plateau * (1.0 + a)) / ((1.0 - a) + plateau * (1.0 + a)); };
  const auto mag2 = [](double a, double q, double b, double c) {
    return (1.0 + 2.0 * q * c + q * q) / (1.0 - 2.0 * a * c + a * a) / (1.0 - 2.0 * b * c + b * b);
  };
  const auto cost = [&](double a, double plateau, double b) {
    const double q = zero_for(a, plateau);
    const double ref = mag2(a, q, b, cw[1]);
    const double dc_gain = -0.5 * std::log(mag2(a, q, b, 1.0) / ref) + rates[0] / f0;
    if (dc_gain < 0.0) return 1e6 * (1.0 - dc_gain);
    double sum = 0.0;
    for (int n = 2; n <= count; ++n) {
      const double lost = -0.5 * std::log(mag2(a, q, b, cw[static_cast<size_t>(n)]) / ref);
      const double want = (std::max(rates[n - 1], rates[0]) - rates[0]) / f0;
      const double d = lost - want;
      sum += d * d / n;
    }
    return sum;
  };
  const auto plateau_of = [](double depth) { return 1.0 - std::exp(depth); };
  double ba = 0.9, bd = std::log(0.01), bb = 0.0, best = 1e300;
  for (double a : {0.0, 0.7, 0.9, 0.97, 0.99})
    for (double d : {-8.0, -5.5, -4.0, -2.5})
      for (double b : {0.0, 0.3, 0.6}) {
        const double c = cost(a, plateau_of(d), b);
        if (c < best) { best = c; ba = a; bd = d; bb = b; }
      }
  for (double step = 0.1; step > 4e-4; step *= 0.5)
    for (int round = 0; round < 5; ++round) {
      bool moved = false;
      const std::array<std::array<double, 3>, 6> dirs{{{step, 0, 0}, {-step, 0, 0}, {0, step * 10, 0}, {0, -step * 10, 0}, {0, 0, step}, {0, 0, -step}}};
      for (const auto& dir : dirs) {
        const double a = std::clamp(ba + dir[0], 0.0, 0.999), d = std::clamp(bd + dir[1], -12.0, -0.5), b = std::clamp(bb + dir[2], 0.0, 0.95);
        const double c = cost(a, plateau_of(d), b);
        if (c < best) { best = c; ba = a; bd = d; bb = b; moved = true; }
      }
      if (!moved) break;
    }
  r.pole = ba;
  r.zero = zero_for(ba, plateau_of(bd));
  r.lp = bb;
  const auto mag = [&](double c) { return (1.0 - r.pole) * (1.0 - r.lp) / (1.0 + r.zero) * std::sqrt(mag2(r.pole, r.zero, r.lp, c)); };
  double gain = std::exp(-rates[0] / f0) / mag(cw[1]);
  double peak = 0.0;
  for (int k = 0; k <= 128; ++k) peak = std::max(peak, gain * mag(std::cos(kPi * k / 128.0)));
  if (peak > 0.99999) gain *= 0.99999 / peak;
  r.gain = gain;
}

// Fits the dispersion chain: a run of identical first-order allpasses whose coefficient puts
// the loop's partials where the stiff string has them, with the fundamental tuned exactly by
// the delay. The fit is least squares in cents weighted by 1/n over the partials below the
// band's top (24 partials, where the stretch a tuner hears lives); the number of sections is chosen too (1, 2, 4, 8 or 16), because a short treble
// loop has no room for sixteen and a bass string's B is small enough for the chain to follow
// across dozens of partials. The loop needs at least `min_loop` samples of delay for its rails,
// so a chain that would eat the whole period is refused. Returns the weighted residual in cents^2.
inline double fit_dispersion(LoopDesign& r, double f1, double b, double rate, double min_loop = 3.0) {
  r.sections = 0; r.allpass = 0.0;
  const double period = rate / f1;
  // The band: the first 24 partials below 10 kHz, where the stretch a tuner hears lives; a
  // treble string whose few partials reach higher is fitted to the top of the band.
  const double top = std::min(f1 > 800.0 ? 20000.0 : 10000.0, rate * 0.45);
  int count = 0;
  while (count < 24 && partial_hz(f1, b, count + 1) < top) ++count;
  if (count < 2 || 1200.0 * std::log2(std::sqrt((1.0 + b * count * count) / (1.0 + b))) < 1.0) return 0.0;
  const auto error = [&](int sections, double c) {
    LoopDesign t = r; t.sections = sections; t.allpass = c;
    const double base = period - loop_delay(t, kTau * f1 / rate) - 1.0;
    if (base < min_loop) return 1e12;
    double sum = 0.0;
    for (int n = 2; n <= count; ++n) {
      const double fn = partial_hz(f1, b, n);
      const double w = kTau * fn / rate;
      const double cents = 1200.0 * std::log2(kTau * n / (w * (base + 1.0 + loop_delay(t, w))));
      sum += cents * cents / (n * n);
    }
    return sum;
  };
  double best = 1e300;
  for (int sections : {2, 4, 8, 16}) {
    double lo = -0.98, hi = 0.0;
    for (int i = 0; i < 26; ++i) {
      const double a = hi - 0.618 * (hi - lo), c = lo + 0.618 * (hi - lo);
      if (error(sections, a) < error(sections, c)) hi = c; else lo = a;
    }
    const double c = 0.5 * (lo + hi), e = error(sections, c);
    if (e < best) { best = e; r.sections = sections; r.allpass = c; }
  }
  if (best >= 1e12) { r.sections = 0; r.allpass = 0.0; return 0.0; }
  return best;
}

// --- one plane of one string -----------------------------------------------------------------------

// Two rails: `upper` carries waves from the nut (index 0) toward the bridge, `lower` from the
// bridge (index 0) back to the nut. The loop filters sit at the bridge end; the nut (or the
// tangent, or the fret) reflects with its own small loss.
class Plane {
 public:
  void prepare(size_t capacity) {
    upper_.prepare(capacity); lower_.prepare(capacity);
    scratch_.assign(capacity + 8, 0.0f);
    clear();
  }
  void clear() {
    upper_.clear(); lower_.clear();
    loss_state_ = nut_state_ = loss_in_ = lp_state_ = 0.0f;
    allpass_state_.fill(0.0f);
    for (auto& z : split_state_) z.fill(0.0f);
  }
  void design(const LoopDesign& d) {
    loss_b_ = static_cast<float>(d.gain * (1.0 - d.pole) / (1.0 + d.zero) * (1.0 - d.lp));
    loss_a_ = static_cast<float>(d.pole);
    loss_q_ = static_cast<float>(d.zero);
    lp_a_ = static_cast<float>(d.lp);
    allpass_ = static_cast<float>(d.allpass);
    sections_ = d.sections;
    for (int k = 0; k < kSplitSections; ++k) {
      split_a1_[static_cast<size_t>(k)] = static_cast<float>(d.split[static_cast<size_t>(k)][0]);
      split_a2_[static_cast<size_t>(k)] = static_cast<float>(d.split[static_cast<size_t>(k)][1]);
    }
  }
  float split(float x, std::array<std::array<float, 2>, kSplitSections>& state) const {
    for (int k = 0; k < kSplitSections; ++k) {
      const float a1 = split_a1_[static_cast<size_t>(k)], a2 = split_a2_[static_cast<size_t>(k)];
      if (a1 == 0.0f && a2 == 0.0f) continue;
      auto& z = state[static_cast<size_t>(k)];
      const float w = x - a1 * z[0] - a2 * z[1];
      x = a2 * w + a1 * z[0] + z[1];
      z[1] = z[0]; z[0] = w;
    }
    return x;
  }
  // Reads the wave arriving at the bridge (`upper_delay` back) and runs it through the loop's
  // loss and dispersion filters; returns the arriving wave and leaves the filtered one in
  // `filtered` (not yet inverted: the bridge does that, adding its own motion).
  float arrive(double upper_delay, float& filtered) {
    const float arriving = upper_.lagrange(upper_delay);
    loss_state_ = loss_b_ * (arriving + loss_q_ * loss_in_) + loss_a_ * loss_state_;
    loss_in_ = arriving;
    lp_state_ = loss_state_ + lp_a_ * lp_state_;
    float y = lp_state_;
    for (int m = 0; m < sections_; ++m) {
      float& z = allpass_state_[static_cast<size_t>(m)];
      const float out = allpass_ * y + z;
      z = y - allpass_ * out;
      y = out;
    }
    filtered = split(y, split_state_);
    return arriving;
  }
  // Sends the reflected wave back toward the nut, and reflects the wave arriving at the nut
  // (`lower_len` back) with the nut's loss.
  // `nut_frac` is the loop's fractional delay, kept at the nut so the bridge reads a whole cell
  // and the two rails line up cell for cell (lower slot j is the cell `j` from the bridge).
  void finish(float reflected, size_t lower_len, double nut_frac, float nut_gain, float nut_pole) {
    lower_.push(reflected);
    nut_state_ = (1.0f - nut_pole) * lower_.lagrange(static_cast<double>(lower_len) + nut_frac) + nut_pole * nut_state_;
    upper_.push(-nut_gain * nut_state_);
  }
  // A plateau laid up to the bridge stands for a wave that has been arriving there forever:
  // the loop's filters are set to the state a steady input of `step` would have left them in,
  // so what they send back carries on from the plateau without a gap the filters' delay long.
  void prime_bridge_dc(float step) {
    const float shelf = loss_b_ * (1.0f + loss_q_) / (1.0f - loss_a_) * step;
    loss_state_ += shelf;
    loss_in_ += step;
    const float y = shelf / (1.0f - lp_a_);
    lp_state_ += y;
    for (int m = 0; m < sections_; ++m) allpass_state_[static_cast<size_t>(m)] += y * (1.0f - allpass_);
    for (int k = 0; k < kSplitSections; ++k) {
      const float a1 = split_a1_[static_cast<size_t>(k)], a2 = split_a2_[static_cast<size_t>(k)];
      if (a1 == 0.0f && a2 == 0.0f) continue;
      const float w = y / (1.0f + a1 + a2);
      split_state_[static_cast<size_t>(k)][0] += w;
      split_state_[static_cast<size_t>(k)][1] += w;
    }
  }
  // The same for a plateau laid up to the nut: its one-pole has been seeing `step`.
  void prime_nut_dc(float step) { nut_state_ += step; }
  // A constant on every upper cell with its negative on every lower cell is no displacement at
  // all, and the inverting ends keep it forever: a gauge of the two-rail picture, which a force
  // pushing one way for long (a bow) feeds a little of each sample until it swamps the waves in
  // single precision. This takes it out, sharing the string's mean displacement between the
  // rails, and keeps the end filters consistent with the shift.
  void remove_gauge(size_t bridge, size_t lower_len) {
    double up = 0.0, low = 0.0;
    for (size_t i = 0; i <= bridge; ++i) up += upper_.at(i);
    for (size_t i = 0; i <= lower_len; ++i) low += lower_.at(i);
    const float c = static_cast<float>(0.5 * (up / static_cast<double>(bridge + 1) - low / static_cast<double>(lower_len + 1)));
    if (std::abs(c) < 1e-9f) return;
    for (size_t i = 0; i <= bridge; ++i) upper_.slot(i) -= c;
    for (size_t i = 0; i <= lower_len; ++i) lower_.slot(i) += c;
    prime_bridge_dc(-c);
    prime_nut_dc(c);
  }
  // Adds what the bridge would have sent back toward the nut had `arrivals` (oldest first) come
  // in at it over the last `n` samples with a rigid bridge: runs the filters over them from rest
  // and adds the result (inverted) to the rail back and to the filters' own state.
  void preroll(const float* arrivals, size_t n) {
    n = std::min(n, scratch_.size());
    float loss = 0.0f, in1 = 0.0f, lpv = 0.0f;
    std::array<float, kMaxDispersion> state{};
    std::array<std::array<float, 2>, kSplitSections> split_pre{};
    for (size_t i = 0; i < n; ++i) {
      loss = loss_b_ * (arrivals[i] + loss_q_ * in1) + loss_a_ * loss;
      in1 = arrivals[i];
      lpv = loss + lp_a_ * lpv;
      float y = lpv;
      for (int m = 0; m < sections_; ++m) {
        float& z = state[static_cast<size_t>(m)];
        const float out = allpass_ * y + z;
        z = y - allpass_ * out;
        y = out;
      }
      scratch_[i] = -split(y, split_pre);
    }
    for (size_t i = 0; i < n; ++i) lower_.slot(n - 1 - i) += scratch_[i];
    loss_state_ += loss;
    loss_in_ += in1;
    lp_state_ += lpv;
    for (int m = 0; m < sections_; ++m) allpass_state_[static_cast<size_t>(m)] += state[static_cast<size_t>(m)];
    for (int k = 0; k < kSplitSections; ++k)
      for (int j = 0; j < 2; ++j) split_state_[static_cast<size_t>(k)][static_cast<size_t>(j)] += split_pre[static_cast<size_t>(k)][static_cast<size_t>(j)];
  }
  // The displacement at `x` samples of travel from the nut: the wave heading for the bridge
  // that passed the nut x samples ago, plus the wave heading for the nut that will reach it in
  // x samples. Exciters read here and push here.
  // The displacement `x` cells from the nut, the bridge being `bridge` cells from it on both
  // rails (lower slot j sits `j` cells from the bridge).
  float at_point(double x, size_t bridge) const {
    return upper_.linear(x) + lower_.linear(std::max(0.0, static_cast<double>(bridge) - x));
  }
  void inject(double x, size_t bridge, float delta) {
    upper_.add(x, delta);
    lower_.add(std::max(0.0, static_cast<double>(bridge) - x), delta);
  }
  Rail& upper() { return upper_; }
  Rail& lower() { return lower_; }
  const Rail& upper() const { return upper_; }
  const Rail& lower() const { return lower_; }
  void scale(float k, size_t span) {
    for (size_t i = 0; i <= span + 4; ++i) { upper_.slot(i) *= k; lower_.slot(i) *= k; }
    loss_state_ *= k; nut_state_ *= k; loss_in_ *= k; lp_state_ *= k;
    for (auto& z : allpass_state_) z *= k;
    for (auto& z : split_state_) { z[0] *= k; z[1] *= k; }
  }
 private:
  Rail upper_, lower_;
  float loss_b_ = 0.99f, loss_a_ = 0.0f, loss_q_ = 0.0f, loss_in_ = 0.0f, lp_a_ = 0.0f, lp_state_ = 0.0f, allpass_ = 0.0f;
  int sections_ = 0;
  float loss_state_ = 0.0f, nut_state_ = 0.0f;
  std::array<float, kMaxDispersion> allpass_state_{};
  std::vector<float> scratch_;
  std::array<float, kSplitSections> split_a1_{}, split_a2_{};
  std::array<std::array<float, 2>, kSplitSections> split_state_{};
};

// A second-order resonator with unity peak gain: the velocity response of one mode of a
// soundboard, a bar's resonator box or a pipe's body.
class Resonator {
 public:
  void set(double hz, double q, double gain, double rate) {
    const double w = kTau * std::clamp(hz, 10.0, rate * 0.45) / rate;
    const double r = std::exp(-w / (2.0 * std::max(q, 0.5)));
    a1_ = static_cast<float>(2.0 * r * std::cos(w));
    a2_ = static_cast<float>(-r * r);
    b_ = static_cast<float>(gain * (1.0 - r * r) * 0.5);
  }
  float process(float x) {
    const float y = b_ * (x - x2_) + a1_ * y1_ + a2_ * y2_;
    x2_ = x1_; x1_ = x; y2_ = y1_; y1_ = y;
    return y;
  }
  void reset() { x1_ = x2_ = y1_ = y2_ = 0.0f; }
  void scale(float k) { x1_ *= k; x2_ *= k; y1_ *= k; y2_ *= k; }
  // The response at w (rad/sample): b (1 - z^-2) / (1 - a1 z^-1 - a2 z^-2).
  std::complex<double> response(double w) const {
    const std::complex<double> z1 = std::polar(1.0, -w), z2 = z1 * z1;
    return static_cast<double>(b_) * (1.0 - z2) / (1.0 - static_cast<double>(a1_) * z1 - static_cast<double>(a2_) * z2);
  }
 private:
  float a1_ = 0.0f, a2_ = 0.0f, b_ = 0.0f, x1_ = 0.0f, x2_ = 0.0f, y1_ = 0.0f, y2_ = 0.0f;
};

// A bank of resonators laid out so the compiler can run them side by side: a soundboard's
// modes, all driven by one force and summed.
template <int N>
class ModalBank {
 public:
  void clear() { count_ = 0; reset(); }
  void add(double hz, double q, double gain, double rate) {
    if (count_ >= N) return;
    const double w = kTau * std::clamp(hz, 10.0, rate * 0.45) / rate;
    const double r = std::exp(-w / (2.0 * std::max(q, 0.5)));
    a1_[count_] = static_cast<float>(2.0 * r * std::cos(w));
    a2_[count_] = static_cast<float>(-r * r);
    b_[count_] = static_cast<float>(gain * (1.0 - r * r) * 0.5);
    ++count_;
  }
  int count() const { return count_; }
  void reset() { for (int i = 0; i < N; ++i) x1_[i] = x2_[i] = y1_[i] = y2_[i] = 0.0f; }
  float process(float x) {
    float sum = 0.0f;
    for (int i = 0; i < count_; ++i) {
      const float y = b_[i] * (x - x2_[i]) + a1_[i] * y1_[i] + a2_[i] * y2_[i];
      x2_[i] = x1_[i]; x1_[i] = x; y2_[i] = y1_[i]; y1_[i] = y;
      sum += y;
    }
    return sum;
  }
 private:
  int count_ = 0;
  alignas(32) float a1_[N]{}, a2_[N]{}, b_[N]{}, x1_[N]{}, x2_[N]{}, y1_[N]{}, y2_[N]{};
};

// --- the plucked shape -----------------------------------------------------------------------------------

// The shape a plectrum (or a fingertip) leaves a string in at the moment it slips off: a
// triangle peaked `point` samples from the bridge on a string `half` samples long, smoothed by
// a box `width` samples wide over its odd extension past the fixed ends.
class Shape {
 public:
  void prepare(size_t capacity) { shape_.assign(3 * capacity + 64, 0.0f); smooth_.assign(3 * capacity + 64, 0.0f); preroll_.assign(capacity + 8, 0.0f); }
  void build(double half, double point, double width) {
    const int n = std::min(static_cast<int>(half) + 2, static_cast<int>(shape_.size()) / 3 - 2);
    point = std::clamp(point, 1.0, half - 1.0);
    const int w = std::clamp(static_cast<int>(width), 1, std::max(1, n / 3));
    const auto triangle = [&](double x) { return x <= point ? x / point : (half - x) / (half - point); };
    const auto odd = [&](int i) -> float {
      double x = static_cast<double>(i);
      double sign = 1.0;
      while (x < 0.0 || x > half) {
        if (x < 0.0) { x = -x; sign = -sign; }
        if (x > half) { x = 2.0 * half - x; sign = -sign; }
      }
      return static_cast<float>(sign * triangle(x));
    };
    for (int i = -n; i <= 2 * n; ++i) smooth_[static_cast<size_t>(i + n)] = odd(i);
    for (int pass = 0; pass < 2; ++pass) {
      double acc = 0.0;
      for (int i = -n; i <= 2 * n; ++i) {
        acc += smooth_[static_cast<size_t>(i + n)];
        if (i - w >= -n) acc -= smooth_[static_cast<size_t>(i - w + n)];
        const int centre = i - w / 2;
        if (centre >= -n) shape_[static_cast<size_t>(centre + n)] = static_cast<float>(acc / w);
      }
      for (int i = -n; i <= 2 * n - w; ++i) smooth_[static_cast<size_t>(i + n)] = shape_[static_cast<size_t>(i + n)];
    }
    for (int i = 0; i <= n; ++i) shape_[static_cast<size_t>(i)] = i <= static_cast<int>(half) ? smooth_[static_cast<size_t>(i + n)] : 0.0f;
    shape_[0] = 0.0f;
    len_ = half;
  }
  float at(double x) const {
    if (x <= 0.0 || x >= len_) return 0.0f;
    const size_t i = static_cast<size_t>(x);
    const float f = static_cast<float>(x - static_cast<double>(i));
    return shape_[i] + (shape_[i + 1] - shape_[i]) * f;
  }
  // Lays the shape onto a plane's rails at rest (zero velocity): half the displacement in each
  // travelling wave. The loop's filters sit at the bridge and stand for `fd` samples of the
  // string's travel, so the string is `upper_delay + fd / 2` cells long (the shape must have been
  // built over that length, measured from the bridge): the upper rail holds the cells from the
  // nut to fd / 2 short of the bridge, and the lower rail, which leaves the filters fd samples
  // after a wave arrives at them, is filled by running through the filters the arrivals of the
  // last `upper_delay + fd` samples: the shape's odd image beyond the bridge, inverted.
  void lay(Plane& p, double upper_delay, double fd, double amplitude) {
    const float a = static_cast<float>(amplitude * 0.5);
    const double half_fd = 0.5 * std::max(0.0, fd);
    const double length = upper_delay + half_fd;
    const size_t up = static_cast<size_t>(upper_delay);
    for (size_t j = 0; j <= up; ++j) p.upper().slot(j) += a * at(length - static_cast<double>(j));
    const auto odd = [&](double x) { return x >= 0.0 ? at(x) : -at(-x); };
    const size_t n = std::min(static_cast<size_t>(upper_delay + std::max(0.0, fd)) + 1, preroll_.size());
    // Arrival T samples ago (oldest first): what the bridge must have seen for the lower rail to
    // hold the shape now.
    for (size_t i = 0; i < n; ++i) preroll_[i] = -a * odd(static_cast<double>(n - 1 - i) - half_fd);
    p.preroll(preroll_.data(), n);
  }
 private:
  std::vector<float> shape_, smooth_, preroll_;
  double len_ = 0.0;
};

}  // namespace schrodr::dsp::string
