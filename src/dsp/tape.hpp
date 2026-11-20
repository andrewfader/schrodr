// The tape machine after the microphones: what a recorder's transport and heads do to the signal
// it is sent. The room and the microphones are the air; this is the machine.
//
// - Wow and flutter. The tape passes the heads at a speed that wavers with the transport's turning
//   parts, so what was recorded plays back with its pitch moving. Each part pulls once a turn: on a
//   cassette transport at 4.76 cm/s, the reels mid-spool (0.5 Hz), a 12 mm pinch roller (1.26 Hz)
//   and a 2.4 mm capstan (6.3 Hz). The deviation is their sum, with 40%, 30% and 30% of its power,
//   and the parameter is its RMS in cents. A playback speed of 1 + k c(t) (k = ln 2 / 1200, c in
//   cents) is a delay whose rate of change is -k c(t): each part's share is a cosine of amplitude
//   k A / w on the delay, so the delay never drifts.
// - Bandwidth. The heads' gap and slow tape lose the top end: a fourth-order Butterworth low-pass,
//   3 dB down at the stated frequency.
//
// At no wow and flutter the delay is out of the chain, and at 20 kHz the low-pass is, so the
// machine at its defaults passes the microphones' signal unchanged.
//
// Not modelled: tape saturation and hiss, the head bump, print-through, dropouts.
#pragma once

#include <array>
#include <cmath>

#include "core.hpp"
#include "filters.hpp"

namespace schrodr::dsp::tape {

struct Part { double hz, share, phase; };
inline constexpr std::array<Part, 3> kParts{{{0.5, 0.4, 0.3}, {1.26, 0.3, 2.1}, {6.3, 0.3, 4.4}}};
inline constexpr double kMaxCents = 30.0;
inline constexpr double kOpenHz = 20000.0;

class Machine {
 public:
  void prepare(double rate) {
    rate_ = rate;
    // The delay's centre: the largest swing the parts can make at the most wow and flutter.
    double swing = 0.0;
    for (const Part& p : kParts) swing += kK * kMaxCents * std::sqrt(2.0 * p.share) / (kTau * p.hz);
    centre_ = swing * rate + 2.0;
    for (auto& d : delay_) d.prepare(static_cast<size_t>(2.0 * centre_) + 4);
    reset();
  }
  void reset() {
    for (auto& d : delay_) d.clear();
    for (auto& f : low_) f.reset();
    t_ = 0;
  }
  void configure(double wow_cents, double bandwidth_hz) {
    cents_ = std::clamp(wow_cents, 0.0, kMaxCents);
    const double hz = std::clamp(bandwidth_hz, 100.0, kOpenHz);
    open_ = hz >= kOpenHz;
    if (!open_ && hz != hz_) {
      for (int c = 0; c < 2; ++c) {
        low_[static_cast<size_t>(2 * c)].set(Biquad::Type::LowPass, hz, 0.5412, 0.0, rate_);
        low_[static_cast<size_t>(2 * c + 1)].set(Biquad::Type::LowPass, hz, 1.3066, 0.0, rate_);
      }
      hz_ = hz;
    }
  }
  void process(float& l, float& r) {
    if (cents_ > 0.0) {
      delay_[0].push(l); delay_[1].push(r);
      const double t = static_cast<double>(t_++) / rate_;
      double offset = 0.0;
      for (const Part& p : kParts) {
        const double w = kTau * p.hz;
        offset += kK * cents_ * std::sqrt(2.0 * p.share) / w * std::cos(w * t + p.phase);
      }
      const double at = centre_ + offset * rate_;
      l = delay_[0].read(at); r = delay_[1].read(at);
    }
    if (!open_) {
      l = low_[1].process(low_[0].process(l));
      r = low_[3].process(low_[2].process(r));
    }
  }

 private:
  static constexpr double kK = 0.000577622650466621;  // ln 2 / 1200
  double rate_ = 48000.0, centre_ = 0.0, cents_ = 0.0, hz_ = -1.0;
  bool open_ = true;
  uint64_t t_ = 0;
  std::array<DelayLine, 2> delay_;
  std::array<Biquad, 4> low_{};
};

}  // namespace schrodr::dsp::tape
