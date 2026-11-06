// The struck bars and forks: celesta plates, glockenspiel bars, the dulcitone's tuning forks, an
// electric piano's tines and a toy piano's rods. Each is a beam vibrating in its bending modes,
// simulated from the beam's physics: a free-free bar (celesta, glockenspiel) has modes at
// 1 : 2.756 : 5.404 : 8.933 : 13.34 of its fundamental (Euler-Bernoulli, (beta_n L)^2 with
// beta L = 4.730, 7.853, 10.996, 14.137, 17.279), a clamped-free rod (a tine, a toy piano's rod,
// a fork's prong) at 1 : 6.267 : 17.55 : 34.39 : 56.84 (beta L = 1.875, 4.694, 7.855, 10.996,
// 14.137); thick bars have their high modes lowered by shear and rotary inertia (Timoshenko). The
// length that puts the fundamental on the key's pitch follows from the bar's thickness and
// material, f_n = (beta_n L)^2 / (2 pi L^2) sqrt(E I / (rho A)). A hammer meets the bar at its
// strike point through the modes' shapes there, with the same felt law as the piano's hammer;
// the bar answers through its modal masses. A bar is small against the wavelengths it makes,
// so what it radiates directly is its acceleration (the pressure a compact source makes), which
// is where a hard mallet's high modes are heard; a resonator box, tone bar or sounding board is
// driven by its velocity, and a magnetic pickup hears velocity too.
//
// Sources: Fletcher and Rossing, The Physics of Musical Instruments ch. 19 (mallet
// percussion: bar modes, Timoshenko correction, resonators); Rossing, Science of Percussion
// Instruments (celesta, glockenspiel); Chaigne and Doutaut, "Numerical simulations of
// xylophones" (1997) for the mallet contact.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "core.hpp"
#include "filters.hpp"
#include "string.hpp"
#include "temperament.hpp"

namespace schrodr::dsp::bars {

constexpr int kModes = 6;

enum class Beam { FreeFree, ClampedFree };
enum class Body { Bare, ResonatorBox, SoundingBoard, ToneBarPickup, PlasticCase };

struct Spec {
  const char* name;
  int low, high;
  Beam beam;
  double young, density;      // the bar's material
  double thickness_m;         // the dimension that bends (a rod's diameter)
  bool round;                 // a rod (I/A = d^2/16) rather than a plate (h^2/12)
  int heard_mode;             // which mode carries the pitch (a toy piano's rod sounds its second)
  double q0, q_fall;          // Q of the first mode and how Q falls with mode number
  double strike;              // fraction of the length from the fixed (or left) end
  double pickup;              // where it is heard from
  double hammer_mass_kg, hammer_k, hammer_p;
  Body body;
  double body_q, body_level, body_hz;  // a fixed body resonance (0: tuned to the note)
  double radiation_hz;        // a bar radiates poorly below this
  bool dampers;               // released notes are damped
  double damper_s;
};

constexpr int kModelCount = 5;
enum Model : int { kCelesta, kGlockenspiel, kDulcitone, kElectricPiano, kToyPiano };

inline const Spec& spec(int model) {
  static const std::array<Spec, kModelCount> specs{{
      // A celesta (Mustel pattern): steel plates 3 mm thick on felt at their nodes, each over a
      // wooden resonator box tuned to it, struck in the middle by felt hammers, with dampers and
      // a sustain pedal; C4 to C8.
      {"Celesta", 60, 108, Beam::FreeFree, 2.0e11, 7850.0, 3.0e-3, false, 1, 2500.0, 0.25, 0.5, 0.5,
       0.010, 1.0e9, 2.5, Body::ResonatorBox, 18.0, 1.4, 0.0, 250.0, true, 0.08},
      // A keyboard glockenspiel: steel bars 8 mm thick, free on cords, struck in the middle by
      // hard mallets, no resonators, nothing to damp them; G5 to C8.
      {"Glockenspiel", 79, 108, Beam::FreeFree, 2.0e11, 7850.0, 8.0e-3, false, 1, 9000.0, 0.3, 0.5, 0.5,
       0.020, 1.0e10, 2.0, Body::Bare, 1.0, 0.0, 0.0, 400.0, false, 0.5},
      // A dulcitone (Machell, 1880s): steel tuning forks, their prongs clamped-free rods 5 mm
      // across, struck near the tip by felt hammers over a small sounding board; F2 to F6.
      {"Dulcitone", 41, 89, Beam::ClampedFree, 2.0e11, 7850.0, 5.0e-3, true, 1, 6000.0, 0.3, 0.85, 0.9,
       0.008, 5.0e8, 2.5, Body::SoundingBoard, 4.0, 1.0, 380.0, 150.0, true, 0.1},
      // An electric piano (Rhodes pattern): steel tines, 3.2 mm rods clamped in a tone bar that
      // rings with them, struck by neoprene-tipped hammers near their base, heard by a magnetic
      // pickup facing the tip whose field makes a hard blow bark; dampers and a pedal; E1 to E7.
      {"Electric piano", 28, 100, Beam::ClampedFree, 2.0e11, 7850.0, 3.2e-3, true, 1, 3000.0, 0.3, 0.3, 0.92,
       0.005, 2.0e9, 2.2, Body::ToneBarPickup, 120.0, 0.35, 0.0, 60.0, true, 0.06},
      // A toy piano (Schoenhut pattern): steel rods 2.5 mm across clamped in a cast frame, 5 to
      // 20 cm long, struck near the free end by plastic hammers in a plastic case, nothing to damp
      // them; the clank is the rod's modes at 6.27 and 17.5 times the pitch; F3 to F6.
      {"Toy piano", 53, 89, Beam::ClampedFree, 2.0e11, 7850.0, 2.5e-3, true, 1, 800.0, 0.3, 0.65, 0.55,
       0.003, 1.0e10, 2.0, Body::PlasticCase, 2.0, 0.3, 900.0, 300.0, false, 0.5},
  }};
  return specs[static_cast<size_t>(std::clamp(model, 0, kModelCount - 1))];
}

// Euler-Bernoulli eigenvalues beta_n L and the mode shapes they give.
inline double beta_l(Beam beam, int n) {
  static constexpr double kFree[kModes] = {4.7300, 7.8532, 10.9956, 14.1372, 17.2788, 20.4204};
  static constexpr double kClamped[kModes] = {1.8751, 4.6941, 7.8548, 10.9955, 14.1372, 17.2788};
  return beam == Beam::FreeFree ? kFree[n] : kClamped[n];
}
inline double shape(Beam beam, int n, double x) {  // x: fraction of the length
  const double bl = beta_l(beam, n), bx = bl * x;
  if (beam == Beam::FreeFree) {
    const double sigma = (std::cosh(bl) - std::cos(bl)) / (std::sinh(bl) - std::sin(bl));
    return std::cosh(bx) + std::cos(bx) - sigma * (std::sinh(bx) + std::sin(bx));
  }
  const double sigma = (std::sinh(bl) - std::sin(bl)) / (std::cosh(bl) + std::cos(bl));
  return std::cosh(bx) - std::cos(bx) - sigma * (std::sinh(bx) - std::sin(bx));
}

// One bar's modes for a key: frequencies (Hz), shapes at the strike and pickup points
// (normalised to a maximum of 1 along the bar), modal masses and Qs.
struct Design {
  double length_m = 0.1, mass_kg = 0.01;
  std::array<double, kModes> hz{}, phi_strike{}, phi_pickup{}, modal_mass{}, q{};
};

inline Design design(const Spec& s, double pitch_hz) {
  Design d;
  const double area = s.round ? kPi * s.thickness_m * s.thickness_m / 4.0 : s.thickness_m * 0.03;
  const double radius2 = s.round ? s.thickness_m * s.thickness_m / 16.0 : s.thickness_m * s.thickness_m / 12.0;  // I / A
  const double c = std::sqrt(s.young * radius2 / s.density);  // f_n = (beta L)^2 c / (2 pi L^2)
  const int heard = std::clamp(s.heard_mode - 1, 0, kModes - 1);
  // The length that puts the heard mode on the pitch, with the Timoshenko lowering included.
  const auto correction = [&](int n, double length) {
    const double k = beta_l(s.beam, n) * std::sqrt(radius2) / length;
    return 1.0 / std::sqrt(1.0 + 3.0 * k * k);
  };
  double length = std::sqrt(std::pow(beta_l(s.beam, heard), 2.0) * c / (kTau * pitch_hz));
  for (int it = 0; it < 4; ++it) length = std::sqrt(std::pow(beta_l(s.beam, heard), 2.0) * c * correction(heard, length) / (kTau * pitch_hz));
  d.length_m = length;
  d.mass_kg = s.density * area * length;
  for (int n = 0; n < kModes; ++n) {
    d.hz[static_cast<size_t>(n)] = std::pow(beta_l(s.beam, n), 2.0) * c * correction(n, length) / (kTau * length * length);
    double peak = 0.0, integral = 0.0;
    for (int i = 0; i <= 64; ++i) { const double v = shape(s.beam, n, i / 64.0); peak = std::max(peak, std::abs(v)); integral += v * v / 65.0; }
    d.phi_strike[static_cast<size_t>(n)] = shape(s.beam, n, s.strike) / peak;
    d.phi_pickup[static_cast<size_t>(n)] = shape(s.beam, n, s.pickup) / peak;
    d.modal_mass[static_cast<size_t>(n)] = d.mass_kg * integral / (peak * peak);
    d.q[static_cast<size_t>(n)] = s.q0 / (1.0 + s.q_fall * n);
  }
  return d;
}

// The player's settings.
struct Setup {
  int model = kCelesta;
  double hardness = 0.5;    // the mallet, soft to hard
  double release_s = 0.08;
  double sustain = 0.0;
  bool sostenuto = false;
  double mechanics = 0.5;
  double a4 = 440.0;
  int temperament = 0, root = 0;
};

class Bars {
 public:
  static constexpr int kFirstKey = 21, kLastKey = 108, kKeys = kLastKey - kFirstKey + 1;

  struct Note {
    int key = -1;
    bool awake = false, held = false, damping = false, free = false, sostenuto = false;
    Design design;
    std::array<double, kModes> q{}, q1{}, q2{}, a1{}, a2{}, g{}, decay{};  // modal states and coefficients
    double compliance = 0.0, w1 = 1.0;
    bool hammer_active = false;
    double hammer_y = 0.0, hammer_v = 0.0, hammer_u = 0.0, hammer_f = 0.0;
    double mass = 0.01, k = 1e9, p = 2.5;
    int age = 0, contact = 0;
    float env = 0.0f, velocity_out = 0.0f, prev_pickup = 0.0f;
    string::Resonator body;
    double pan = 0.0;
    uint64_t stamp = 0;
  };

  void prepare(double rate) {
    rate_ = rate;
    for (int i = 0; i < kKeys; ++i) notes_[static_cast<size_t>(i)].key = kFirstKey + i;
    radiation_[0].set(300.0, rate); radiation_[1].set(300.0, rate);
    thump_tone_.set(400.0, rate);
    thump_fall_ = static_cast<float>(std::exp(-1.0 / (0.008 * rate)));
    env_fall_ = static_cast<float>(std::exp(-1.0 / (0.05 * rate)));
    configured_ = false;
    configure(setup_);
    reset();
  }
  void reset() {
    for (auto& n : notes_) quiet(n);
    thump_ = 0.0f;
    for (auto& r : radiation_) r.reset();
  }
  void configure(const Setup& in) {
    const Setup before = setup_;
    setup_ = in;
    setup_.model = std::clamp(setup_.model, 0, kModelCount - 1);
    const bool rebuild = !configured_ || setup_.model != before.model || setup_.a4 != before.a4 || setup_.temperament != before.temperament || setup_.root != before.root;
    if (rebuild) {
      const Spec& s = spec();
      for (int i = 0; i < kKeys; ++i) {
        Note& n = notes_[static_cast<size_t>(i)];
        if (n.key < s.low || n.key > s.high) { quiet(n); continue; }
        n.design = design(s, temperament::hz(setup_.temperament, setup_.root, setup_.a4, n.key));
        n.pan = 2.0 * static_cast<double>(n.key - s.low) / std::max(1, s.high - s.low) - 1.0;
        set_coefficients(n);
        if (!n.awake) quiet(n);
      }
      for (auto& r : radiation_) r.set(s.radiation_hz, rate_);
    }
    if (configured_) {
      const bool down = setup_.sustain >= 0.5, was = before.sustain >= 0.5;
      if (down && !was) for (auto& n : notes_) if (n.awake && !n.held && n.damping) { n.damping = false; n.free = true; set_coefficients(n); }
      if (!down && was) for (auto& n : notes_) if (n.awake && !n.held && !n.sostenuto) damp(n);
      if (setup_.sostenuto && !before.sostenuto) for (auto& n : notes_) if (n.awake && n.held) n.sostenuto = true;
      if (!setup_.sostenuto && before.sostenuto) for (auto& n : notes_) if (n.sostenuto) { n.sostenuto = false; if (n.awake && !n.held && !down) damp(n); }
      if (down != was) thump_ += static_cast<float>(setup_.mechanics * 0.2);
    }
    configured_ = true;
  }
  const Spec& spec() const { return bars::spec(setup_.model); }
  const Setup& setup() const { return setup_; }

  void note_on(int key, double velocity, uint32_t seed) {
    const Spec& s = spec();
    key = into_range(key);
    if (key < 0) return;
    Note& n = note_for(key);
    rng_.seed(seed ^ 0x9e3779b9u);
    if (!n.awake) { n.awake = true; n.stamp = ++stamp_; }
    n.held = true; n.damping = false; n.free = false;
    set_coefficients(n);
    // The mallet: its head's mass and felt, hardened or softened by the voicing, thrown at the
    // bar at a speed set by the touch.
    n.mass = s.hammer_mass_kg;
    n.k = s.hammer_k * std::pow(10.0, 2.4 * (std::clamp(setup_.hardness, 0.0, 1.0) - 0.5));
    n.p = s.hammer_p;
    n.hammer_y = displacement_at_strike(n) - 1e-6;
    n.hammer_v = (0.3 + 4.0 * std::pow(std::clamp(velocity, 0.0, 1.0), 1.5)) * (1.0 + 0.03 * rng_.bipolar());
    n.hammer_active = true; n.hammer_u = n.hammer_f = 0.0; n.age = 0; n.contact = 0;
    thump_ += static_cast<float>(setup_.mechanics * velocity * velocity * 0.3);
  }
  void note_off(int key) {
    key = into_range(key);
    if (key < 0) return;
    Note& n = note_for(key);
    if (!n.awake || !n.held) return;
    n.held = false;
    if (!spec().dampers) return;
    if (n.sostenuto || setup_.sustain >= 0.5) { n.free = true; return; }
    damp(n);
    thump_ += static_cast<float>(setup_.mechanics * 0.08);
  }
  void all_notes_off() { for (auto& n : notes_) if (n.awake && n.held) { n.held = false; if (spec().dampers && !n.free) damp(n); } }
  void all_sound_off() { reset(); }
  bool awake() const { for (const auto& n : notes_) if (n.awake) return true; return thump_ > 1e-5f; }
  int sounding() const { int k = 0; for (const auto& n : notes_) k += n.awake; return k; }
  const Note& note(int key) const { return notes_[static_cast<size_t>(std::clamp(key - kFirstKey, 0, kKeys - 1))]; }
  double pitch_at(int key) const { const Note& n = note(key); return n.design.hz[static_cast<size_t>(std::clamp(spec().heard_mode - 1, 0, kModes - 1))]; }
  double last_contact_s() const { return last_contact_ / rate_; }
  double longest_decay_s() const { const Spec& s = spec(); return std::min(30.0, 6.91 * s.q0 / (kPi * temperament::hz(0, 0, 440.0, s.low))); }

  void render(float& left, float& right) {
    const Spec& s = spec();
    left = right = 0.0f;
    for (auto& n : notes_) {
      if (!n.awake) continue;
      if (n.hammer_active) step_hammer(n);
      float pickup = 0.0f, velocity = 0.0f, radiated = 0.0f;
      double energy = 0.0;
      for (int m = 0; m < kModes; ++m) {
        const size_t i = static_cast<size_t>(m);
        const double next = n.a1[i] * n.q1[i] + n.a2[i] * n.q2[i];
        const double v = (next - n.q1[i]) * rate_;
        // The acceleration, scaled by the fundamental's angular frequency so the fundamental
        // radiates at its velocity's level and each higher mode f_m / f_1 louder.
        const double a = (next - 2.0 * n.q1[i] + n.q2[i]) * rate_ * rate_ / n.w1;
        n.q2[i] = n.q1[i]; n.q1[i] = next; n.q[i] = next;
        pickup += static_cast<float>(n.design.phi_pickup[i] * next);
        velocity += static_cast<float>(n.design.phi_pickup[i] * v);
        radiated += static_cast<float>(n.design.phi_pickup[i] * a);
        energy += next * next;
      }
      float out = radiated;
      switch (s.body) {
        case Body::ResonatorBox: out += n.body.process(velocity) * static_cast<float>(s.body_level); break;
        case Body::SoundingBoard: out = n.body.process(velocity) * static_cast<float>(s.body_level) + 0.3f * radiated; break;
        case Body::PlasticCase: out += n.body.process(velocity) * static_cast<float>(s.body_level); break;
        case Body::ToneBarPickup: {
          // The tone bar rings on with the tine's fundamental; the pickup's field is strongest
          // right under the pole, so a tine swinging past it barks and folds.
          const float bar = n.body.process(velocity) * static_cast<float>(s.body_level);
          const float y = pickup / 1.0e-3f;
          out = (velocity + bar) * (1.0f + 0.8f * y) / (1.0f + y * y);
          break;
        }
        default: break;
      }
      n.env = std::max(static_cast<float>(std::sqrt(energy)), n.env * env_fall_);
      const double angle = (n.pan * 0.5 + 0.5) * kPi * 0.5;
      left += out * static_cast<float>(std::cos(angle));
      right += out * static_cast<float>(std::sin(angle));
      if (!n.hammer_active && !n.held && n.env < 1e-10f) quiet(n);
    }
    if (thump_ > 1e-6f) {
      const float t = thump_tone_.low(rng_.bipolar()) * thump_ * 0.3f;
      left += t; right += t;
      thump_ *= thump_fall_;
    } else thump_ = 0.0f;
    left = radiation_[0].high(left) * static_cast<float>(output_scale_);
    right = radiation_[1].high(right) * static_cast<float>(output_scale_);
  }

 private:
  int into_range(int key) const {
    const Spec& s = spec();
    if (key < 0 || key > 127) return -1;
    while (key < s.low) key += 12;
    while (key > s.high) key -= 12;
    return key < s.low ? -1 : key;
  }
  Note& note_for(int key) { return notes_[static_cast<size_t>(std::clamp(key - kFirstKey, 0, kKeys - 1))]; }

  // Each mode as a damped resonator: q[n] = a1 q[n-1] + a2 q[n-2] + g F, with g the displacement
  // an impulse of force leaves after one sample (impulse-invariant), and the damping from its Q
  // or from the damper's felt.
  void set_coefficients(Note& n) {
    const Spec& s = spec();
    n.compliance = 0.0;
    n.w1 = kTau * std::max(20.0, n.design.hz[0]);
    for (int m = 0; m < kModes; ++m) {
      const size_t i = static_cast<size_t>(m);
      const double hz = std::min(n.design.hz[i], rate_ * 0.45);
      const double w = kTau * hz / rate_;
      double q = n.design.q[i];
      if (n.damping) q = std::min(q, kPi * hz * std::max(0.005, setup_.release_s * s.damper_s / 0.08) / 6.91);
      const double sigma = w / (2.0 * q);
      const double r = std::exp(-sigma);
      n.a1[i] = 2.0 * r * std::cos(w);
      n.a2[i] = -r * r;
      // Response of the mode (mass m, damped frequency w) to a unit force held for one sample.
      n.g[i] = std::sin(w) * r / (n.design.modal_mass[i] * w * rate_ * rate_);
      n.decay[i] = sigma;
      n.compliance += n.design.phi_strike[i] * n.design.phi_strike[i] * n.g[i];
    }
    switch (s.body) {
      case Body::ResonatorBox: n.body.set(n.design.hz[0], s.body_q, 1.0, rate_); break;
      case Body::ToneBarPickup: n.body.set(n.design.hz[0] * (1.0 + 0.0008 * rng_.bipolar()), s.body_q, 1.0, rate_); break;
      default: n.body.set(s.body_hz, s.body_q, 1.0, rate_); break;
    }
  }
  double displacement_at_strike(const Note& n) const {
    double y = 0.0;
    for (int m = 0; m < kModes; ++m) y += n.design.phi_strike[static_cast<size_t>(m)] * n.q[static_cast<size_t>(m)];
    return y;
  }
  void step_hammer(Note& n) {
    const double dt = 1.0 / rate_;
    const double y_in = displacement_at_strike(n);
    const double u0 = n.hammer_y - y_in;
    double f = 0.0;
    if (u0 > 0.0) {
      const double udot = (u0 - n.hammer_u) * rate_;
      const double hyst = std::clamp(1.0 + 0.1 * udot, 0.3, 3.0);
      double lo = 0.0, hi = n.compliance > 0.0 ? u0 / n.compliance : 1e6;
      f = std::clamp(n.hammer_f > 0.0 ? n.hammer_f : n.k * std::pow(u0, n.p) * hyst, lo, hi);
      // Newton from the last sample's force, bisection whenever a step leaves the bracket. The
      // residual is the test: an iterate that lands on the root exactly must not be mistaken for
      // one outside the bracket and bisected away from.
      for (int it = 0; it < 24; ++it) {
        const double u = u0 - n.compliance * f;
        const double gval = f - n.k * std::pow(std::max(u, 0.0), n.p) * hyst;
        if (std::abs(gval) <= 1e-7 * std::max(1.0, f)) break;
        if (gval > 0.0) hi = f; else lo = f;
        const double dg = 1.0 + n.k * n.p * std::pow(std::max(u, 1e-12), n.p - 1.0) * hyst * n.compliance;
        double next = f - gval / dg;
        if (!(next >= lo && next <= hi)) next = 0.5 * (lo + hi);
        if (std::abs(next - f) < 1e-9 * std::max(1.0, f)) { f = next; break; }
        f = next;
      }
      n.hammer_u = u0 - n.compliance * f;
      for (int m = 0; m < kModes; ++m) { const size_t i = static_cast<size_t>(m); n.q1[i] += n.g[i] * n.design.phi_strike[i] * f; }
      ++n.contact;
    } else n.hammer_u = 0.0;
    n.hammer_f = f;
    n.hammer_v -= f / n.mass * dt;
    n.hammer_y += n.hammer_v * dt;
    ++n.age;
    if ((f <= 0.0 && n.hammer_v < 0.0 && n.hammer_y < y_in - 2e-5) || n.age > static_cast<int>(rate_ * 0.05)) {
      n.hammer_active = false;
      last_contact_ = n.contact;
    }
  }
  void damp(Note& n) { n.damping = true; n.free = false; set_coefficients(n); }
  void quiet(Note& n) {
    n.awake = n.held = n.damping = n.free = n.sostenuto = n.hammer_active = false;
    n.q.fill(0.0); n.q1.fill(0.0); n.q2.fill(0.0);
    n.env = 0.0f; n.hammer_f = n.hammer_u = 0.0;
    n.body.reset();
  }

  double rate_ = 48000.0, output_scale_ = 0.3;
  Setup setup_{};
  bool configured_ = false;
  std::array<Note, kKeys> notes_{};
  std::array<OnePole, 2> radiation_{};
  OnePole thump_tone_;
  float thump_ = 0.0f, thump_fall_ = 0.99f, env_fall_ = 0.9995f;
  int last_contact_ = 0;
  uint64_t stamp_ = 0;
  Rng rng_{0x2545f491u};
};

}  // namespace schrodr::dsp::bars
