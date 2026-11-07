// The measurements that hold SCHRODR's notes to real recordings (docs/QUALITY.md, "The reference
// recordings"). One function measures a note, whether it came from a recording or from the
// engine, so the two are always compared the same way. The recordings' numbers are taken once by
// schrodr-reference-extract into tests/reference_data.hpp; reference_tests renders the same
// notes and measures them here.
//
// What is measured:
//  - the spectral envelope (the partials' levels, sampled at third-octave centres from the note
//    up to 10 kHz) as a shape, its mean removed when two are compared: the instrument's colour.
//    Not measured for bars, whose few inharmonic partials are not an envelope;
//  - the spectral centroid in harmonics of the fundamental: its brightness;
//  - the level, compared only between dynamics of one source (the Iowa sessions never moved the
//    input gain), so pp to ff is a number in dB of the same instrument;
//  - for a struck string the inharmonicity B fitted to its partials, and its decay early (the
//    prompt sound) and late (the aftersound, from its energy decay curve, which beating unisons
//    cannot fool), in dB/s;
//  - for a bar its first overtone over its fundamental and the fundamental's decay in dB/s;
//  - for a bowed note the attack (onset to within 3 dB of the steady level), the vibrato's
//    depth in cents and rate in Hz, and the bow noise: the power between the harmonics relative
//    to the power on them.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numeric>
#include <vector>

#include "audio_measure.hpp"

namespace reference {

using measure::Span;

enum class Kind { Struck, Bar, Bowed, Pipe };

constexpr int kBands = 24;            // third octaves from 63 Hz to 12.5 kHz
constexpr double kFirstBandHz = 63.0;
constexpr double kTopHz = 10000.0;    // nothing above is compared: the microphones differ there
inline double band_centre(int b) { return kFirstBandHz * std::exp2(b / 3.0); }

struct Features {
  double f0 = 0.0;                        // Hz, as played (the tuning is not compared)
  double level_db = 0.0;                  // LUFS of the loudest 400 ms (bowed: of the steady span)
  double centroid = 0.0;                  // spectral centroid, harmonics of f0
  // The envelope at third-octave centres, dB, the loudest 0; -100 where not measured.
  std::array<double, kBands> bands = [] { std::array<double, kBands> a; a.fill(-100.0); return a; }();
  double inharmonicity = 0.0;             // struck: B
  double decay_early = 0.0;               // struck: dB/s over 30 to 400 ms; bar: the fundamental's
  double decay_late = 0.0;                // struck: dB/s of the energy still to come, over 1 to 3 s
  double overtone_ratio = 0.0;            // bar: first overtone / fundamental
  double attack_ms = 0.0;                 // bowed
  double vibrato_cents = 0.0;             // bowed: peak deviation
  double vibrato_hz = 0.0;                // bowed: 0 if none
  double noise_db = 0.0;                  // bowed: power between the harmonics over power on them
  bool ok = false;
};

inline double nominal_hz(int key) { return 440.0 * std::exp2((key - 69) / 12.0); }

// Magnitude spectrum of a Hann-windowed span, zero-padded to 2^order points.
inline std::vector<double> magnitude(const Span& s, int order) {
  const size_t n = size_t{1} << order, len = std::min(s.size(), n);
  std::vector<std::complex<double>> a(n);
  for (size_t i = 0; i < len; ++i)
    a[i] = s[i] * (0.5 - 0.5 * std::cos(2.0 * measure::kPi * static_cast<double>(i) / static_cast<double>(len - 1)));
  measure::fft(a);
  std::vector<double> m(n / 2);
  for (size_t k = 0; k < n / 2; ++k) m[k] = std::abs(a[k]);
  return m;
}

// The strongest bin of `m` within [lo, hi) Hz, refined by a parabola on the log magnitudes.
inline double peak_in(const std::vector<double>& m, double bin_hz, double lo, double hi) {
  const size_t a = std::max<size_t>(1, static_cast<size_t>(lo / bin_hz));
  const size_t b = std::min(m.size() - 2, static_cast<size_t>(hi / bin_hz) + 1);
  size_t best = a;
  for (size_t k = a; k <= b; ++k) if (m[k] > m[best]) best = k;
  const double y0 = std::log(m[best - 1] + 1e-20), y1 = std::log(m[best] + 1e-20), y2 = std::log(m[best + 1] + 1e-20);
  const double d = y0 - 2.0 * y1 + y2;
  const double shift = std::abs(d) > 1e-12 ? std::clamp(0.5 * (y0 - y2) / d, -0.5, 0.5) : 0.0;
  return (static_cast<double>(best) + shift) * bin_hz;
}

// The spectral envelope of a harmonic (or nearly harmonic) note: each partial's level, read at
// its peak near n f0 sqrt(1 + B n^2), joined in log frequency and sampled at third-octave
// centres from the note up to kTopHz, the loudest at 0 dB and nothing under -60. Bands are not
// summed: a third octave in the bass is narrower than the spacing of the harmonics, so a band
// sum swings with where the harmonics fall, not with the instrument's colour. Bands below the
// note or above kTopHz are marked -100.
inline std::array<double, kBands> harmonic_envelope(const Span& s, double rate, double f0, double b = 0.0) {
  constexpr int order = 16;
  const auto m = magnitude(s, order);
  const double bin = rate / static_cast<double>(size_t{1} << order);
  std::vector<double> hz, level;
  for (int n = 1; n <= 80; ++n) {
    const double f = n * f0 * std::sqrt(1.0 + b * n * n);
    if (f > std::min(kTopHz * 1.2, 0.45 * rate)) break;
    const double half = std::max(0.01 * f, 3.0 * bin);
    const size_t lo = static_cast<size_t>((f - half) / bin), hi = std::min(m.size() - 1, static_cast<size_t>((f + half) / bin) + 1);
    double peak = 0.0;
    for (size_t k = lo; k <= hi; ++k) peak = std::max(peak, m[k]);
    hz.push_back(f); level.push_back(20.0 * std::log10(peak + 1e-20));
  }
  std::array<double, kBands> out{};
  out.fill(-100.0);
  if (hz.empty()) return out;
  const double loudest = *std::max_element(level.begin(), level.end());
  for (int band = 0; band < kBands; ++band) {
    const double fc = band_centre(band);
    if (fc < 0.9 * f0 || fc > kTopHz || fc > hz.back()) continue;
    double v = level.front();
    for (size_t i = 1; i < hz.size(); ++i)
      if (hz[i] >= fc) {
        const double t = std::log(fc / hz[i - 1]) / std::log(hz[i] / hz[i - 1]);
        v = level[i - 1] + std::clamp(t, 0.0, 1.0) * (level[i] - level[i - 1]);
        break;
      }
    out[static_cast<size_t>(band)] = std::max(v - loudest, -60.0);
  }
  return out;
}

// Spectral centroid between 0.8 f0 and kTopHz, in harmonics of f0 (rumble and hiss excluded).
inline double centroid_harmonics(const Span& s, double rate, double f0) {
  const auto p = measure::power_spectrum(s, 14);
  const double bin = rate / static_cast<double>(p.size() * 2);
  double num = 0.0, den = 0.0;
  for (size_t k = 1; k < p.size(); ++k) {
    const double hz = static_cast<double>(k) * bin;
    if (hz < 0.8 * f0 || hz > kTopHz) continue;
    num += hz * p[k]; den += p[k];
  }
  return den > 0.0 ? num / den / f0 : 0.0;
}

// Least-squares slope of y over x.
inline double slope(const std::vector<double>& x, const std::vector<double>& y) {
  const size_t n = std::min(x.size(), y.size());
  if (n < 3) return 0.0;
  const double mx = std::accumulate(x.begin(), x.begin() + static_cast<long>(n), 0.0) / static_cast<double>(n);
  const double my = std::accumulate(y.begin(), y.begin() + static_cast<long>(n), 0.0) / static_cast<double>(n);
  double num = 0.0, den = 0.0;
  for (size_t i = 0; i < n; ++i) { num += (x[i] - mx) * (y[i] - my); den += (x[i] - mx) * (x[i] - mx); }
  return den > 0.0 ? num / den : 0.0;
}

// Decay in dB/s of a contour (one value per `hop_s`) between `from_s` and `to_s`, stopping where
// it comes within 10 dB of `floor_db`: past that the noise, not the note, is measured.
inline double decay_rate(const std::vector<double>& c, double hop_s, double from_s, double to_s, double floor_db) {
  std::vector<double> x, y;
  for (size_t i = static_cast<size_t>(from_s / hop_s); i < c.size() && static_cast<double>(i) * hop_s <= to_s; ++i) {
    if (c[i] < floor_db + 10.0) break;
    x.push_back(static_cast<double>(i) * hop_s); y.push_back(c[i]);
  }
  return x.size() >= 5 ? -slope(x, y) : 0.0;
}

// The energy decay curve of a note (Schroeder's backward integral, as for a room's reverberation
// time): at each 10 ms step the energy still to come, in dB, with the noise floor's power taken
// off every sample and the integral stopped where the note sinks to 10 dB over the floor (or at
// `end`, the key's release), the energy past that point added as the note's own last second
// predicts. It falls monotonically, so the beating of a piano's unisons does not pass for decay.
// Index 0 is `from`; empty if the note lasts under 1.5 s past it.
inline std::vector<double> energy_decay(const std::vector<float>& x, double rate, size_t from, size_t end, double noise_floor_db) {
  const double noise = std::pow(10.0, noise_floor_db / 10.0);
  const auto c = measure::contour_db(x, rate, 0.05, 0.05);
  size_t stop = std::min(end, x.size());
  for (size_t i = static_cast<size_t>(static_cast<double>(from) / rate / 0.05) + 2; i < c.size(); ++i)
    if (c[i] < noise_floor_db + 10.0) { stop = std::min(stop, static_cast<size_t>(static_cast<double>(i) * 0.05 * rate)); break; }
  if (stop <= from + static_cast<size_t>(1.5 * rate)) return {};
  // The energy past the cut-off, had the note gone on at the rate of its last second (Lundeby's
  // compensation): without it the curve steepens toward the cut-off.
  double sum = 0.0;
  {
    const size_t span = static_cast<size_t>(0.2 * rate);
    double p_end = 0.0;
    for (size_t i = stop - span; i < stop; ++i) p_end += std::max(0.0, static_cast<double>(x[i]) * x[i] - noise);
    p_end /= static_cast<double>(span);
    const size_t a = static_cast<size_t>(static_cast<double>(stop) / rate / 0.05) - 24, b = a + 20;
    std::vector<double> t, y;
    for (size_t i = a; i < b && i < c.size(); ++i) { t.push_back(static_cast<double>(i) * 0.05); y.push_back(c[i]); }
    const double db_per_s = -slope(t, y);
    if (db_per_s > 0.1) sum = p_end * rate / (db_per_s * std::log(10.0) / 10.0);
  }
  const size_t hop = static_cast<size_t>(0.01 * rate);
  std::vector<double> tail((stop - from) / hop + 1, 0.0);
  for (size_t i = stop; i-- > from;) {
    sum += std::max(0.0, static_cast<double>(x[i]) * x[i] - noise);
    if ((i - from) % hop == 0) tail[(i - from) / hop] = sum;
  }
  std::vector<double> out;
  for (double v : tail) out.push_back(10.0 * std::log10(v + 1e-30));
  return out;
}

// The onset of the loudest event in a span: the first point at most 30 dB under its peak.
inline size_t onset(const std::vector<float>& x, double rate) {
  const double hop_s = 0.002;
  const auto c = measure::contour_db(x, rate, hop_s, 0.004);
  if (c.empty()) return 0;
  const double top = *std::max_element(c.begin(), c.end());
  const long at = measure::first_at_or_above(c, top - 30.0);
  return at < 0 ? 0 : static_cast<size_t>(static_cast<double>(at) * hop_s * rate);
}

// The noise floor of a recording: the level its quietest 2% of 50 ms steps reach, digital
// silence (an edited-out stretch) aside.
inline double floor_db(const std::vector<float>& x, double rate) {
  std::vector<double> c;
  for (double v : measure::contour_db(x, rate, 0.05, 0.05)) if (v > -150.0) c.push_back(v);
  if (c.empty()) return -150.0;
  std::nth_element(c.begin(), c.begin() + static_cast<long>(c.size() / 50), c.end());
  return c[c.size() / 50];
}

// The inharmonicity of a struck string: its partials found one at a time, each near where the
// fit so far predicts it (stiffness only ever sharpens a partial, so the window opens upward),
// and f_n = n F sqrt(1 + B n^2) fitted as (f_n / n)^2 = F^2 + F^2 B n^2. The fundamental is
// used only above 200 Hz: a bass string radiates almost none of it.
struct StringFit { double b = 0.0, f = 0.0; };
inline StringFit fit_inharmonicity(const Span& s, double rate, double f1) {
  constexpr int order = 18;
  const auto m = magnitude(s, order);
  const double bin = rate / static_cast<double>(size_t{1} << order);
  const int top = std::min(16, static_cast<int>(std::min(kTopHz, 0.45 * rate) / f1));
  if (top < 3) return {0.0, f1};
  std::vector<double> n2, y;
  if (f1 > 200.0) { n2.push_back(1.0); y.push_back(f1 * f1); }
  double b = 0.0, f = f1;
  const auto fit = [&] {
    const double k = slope(n2, y), mean_n2 = std::accumulate(n2.begin(), n2.end(), 0.0) / static_cast<double>(n2.size());
    const double intercept = std::accumulate(y.begin(), y.end(), 0.0) / static_cast<double>(y.size()) - k * mean_n2;
    if (intercept > 0.0) { b = std::max(0.0, k / intercept); f = std::sqrt(intercept); }
  };
  for (int n = 2; n <= top; ++n) {
    const double predicted = n * f * std::sqrt(1.0 + b * n * n);
    const bool guessing = n2.size() < 2;
    const double hz = peak_in(m, bin, predicted * (guessing ? 0.98 : 0.99), predicted * (guessing ? 1.08 : 1.012));
    n2.push_back(static_cast<double>(n * n)); y.push_back((hz / n) * (hz / n));
    if (n2.size() >= 2) {
      if (n2.size() == 2) {  // two points determine the line
        const double k = (y[1] - y[0]) / (n2[1] - n2[0]), intercept = y[0] - k * n2[0];
        if (intercept > 0.0) { b = std::max(0.0, k / intercept); f = std::sqrt(intercept); }
      } else {
        fit();
      }
    }
  }
  return {b, f};
}

// Measures one note. `x` holds only it; `key` names the note. `noise_floor_db` is the floor of
// the recording it was cut from (a note cut from a scale may hold no silence of its own); by
// default the floor is read from `x`. `end` is where the key was let go, if known.
inline Features measure_note(const std::vector<float>& x, double rate, Kind kind, int key, double noise_floor_db = 1.0,
                             size_t end = static_cast<size_t>(-1)) {
  Features out;
  const size_t start = onset(x, rate);
  const double floor = noise_floor_db <= 0.0 ? noise_floor_db : floor_db(x, rate);
  const auto at = [&](double s) { return start + static_cast<size_t>(s * rate); };
  if (at(0.5) >= x.size()) return out;
  // A note less than 25 dB over the recording's floor is measuring the floor.
  {
    double loudest = 0.0;
    for (size_t p = start; p + static_cast<size_t>(0.3 * rate) <= x.size() && p < at(1.0); p += static_cast<size_t>(0.05 * rate))
      loudest = std::max(loudest, measure::rms(Span(x, p, static_cast<size_t>(0.3 * rate))));
    if (measure::db(loudest) < floor + 25.0) return out;
  }

  if (kind == Kind::Bowed || kind == Kind::Pipe) {
    // The steady part: from 0.4 s in to 0.2 s before the bow leaves, at most 2 s of it.
    const auto c = measure::contour_db(x, rate, 0.01, 0.05);
    const long last = measure::last_at_or_above(c, *std::max_element(c.begin(), c.end()) - 20.0);
    const size_t bow_off = std::min(x.size(), static_cast<size_t>(static_cast<double>(std::max(0L, last)) * 0.01 * rate));
    if (bow_off < at(0.9)) return out;
    const size_t b = at(0.4), len = std::min(bow_off - static_cast<size_t>(0.2 * rate) - b, static_cast<size_t>(2.0 * rate));
    const Span steady(x, b, len);
    const auto track = measure::pitch_track(std::vector<float>(x.begin() + static_cast<long>(b), x.begin() + static_cast<long>(b + len)),
                                            rate, 0.01, 0.04, 0.7 * nominal_hz(key), 1.5 * nominal_hz(key));
    std::vector<double> voiced;
    for (double v : track) if (v > 0.0) voiced.push_back(v);
    if (voiced.size() < 20) return out;
    std::vector<double> sorted = voiced;
    std::nth_element(sorted.begin(), sorted.begin() + static_cast<long>(sorted.size() / 2), sorted.end());
    out.f0 = sorted[sorted.size() / 2];
    double sq = 0.0;
    for (double v : voiced) sq += std::pow(measure::cents(v, out.f0), 2.0);
    out.vibrato_cents = std::sqrt(2.0 * sq / static_cast<double>(voiced.size()));
    out.vibrato_hz = out.vibrato_cents > 3.0 ? measure::modulation_hz(track, 0.01, 2.5, 10.0) : 0.0;
    std::vector<float> steady_x(x.begin() + static_cast<long>(b), x.begin() + static_cast<long>(b + len));
    out.level_db = measure::loudness_lufs(Span(measure::k_weighted(steady_x, rate)));
    // Attack: onset to the first 10 ms within 3 dB of the steady level.
    const double steady_db = measure::db(measure::rms(steady));
    const auto fine = measure::contour_db(x, rate, 0.005, 0.01);
    const long reached = measure::first_at_or_above(fine, steady_db - 3.0, static_cast<size_t>(static_cast<double>(start) / rate / 0.005));
    out.attack_ms = reached < 0 ? 0.0 : (static_cast<double>(reached) * 0.005 - static_cast<double>(start) / rate) * 1000.0;
    // Bow noise: power off the harmonics over power on them, 0.8 f0 to kTopHz.
    const auto p = measure::power_spectrum(steady, 14);
    const double bin = rate / static_cast<double>(p.size() * 2);
    double on = 0.0, off = 0.0;
    for (size_t k = 1; k < p.size(); ++k) {
      const double hz = static_cast<double>(k) * bin;
      if (hz < 0.8 * out.f0 || hz > kTopHz) continue;
      // A harmonic's line, a little widened by the bow's jitter: 1.5% of its frequency, at
      // least four bins, at most a quarter of the spacing.
      const double n = std::round(hz / out.f0), half = std::min(0.25 * out.f0, std::max(0.015 * n * out.f0, 4.0 * bin));
      (std::abs(hz - n * out.f0) <= half ? on : off) += p[k];
    }
    out.noise_db = 10.0 * std::log10((off + 1e-30) / (on + 1e-30));
    out.bands = harmonic_envelope(steady, rate, out.f0);
    out.centroid = centroid_harmonics(steady, rate, out.f0);
    out.ok = true;
    return out;
  }

  // Struck strings and bars: the colour over the first 300 ms, the level over the first 1.5 s.
  const Span early(x, at(0.01), static_cast<size_t>(0.3 * rate));
  out.f0 = measure::peak_hz_near(Span(x, at(0.05), static_cast<size_t>(0.25 * rate)), rate, nominal_hz(key), 70.0);
  std::vector<float> head(x.begin() + static_cast<long>(start), x.begin() + static_cast<long>(std::min(x.size(), at(1.5))));
  out.level_db = measure::momentary_loudness_max(head, rate);
  out.centroid = centroid_harmonics(early, rate, out.f0);

  if (kind == Kind::Struck) {
    // The prompt sound: the level's slope over 30 to 400 ms after its peak. The aftersound: the
    // slope of the energy decay curve integrated from 1 s on, over 1 to 3 s; integrating from
    // the onset would let the long aftersound's energy flatten the prompt sound's slope.
    const auto c = measure::contour_db(std::vector<float>(x.begin() + static_cast<long>(start), x.end()), rate, 0.01, 0.05);
    const size_t peak = static_cast<size_t>(std::max_element(c.begin(), c.begin() + std::min<long>(static_cast<long>(c.size()), 50)) - c.begin());
    const double peak_s = static_cast<double>(peak) * 0.01;
    out.decay_early = decay_rate(c, 0.01, peak_s + 0.03, peak_s + 0.4, floor);
    const auto edc = energy_decay(x, rate, at(1.0), end, floor);
    out.decay_late = decay_rate(edc, 0.01, 0.0, 2.0, -1e9);
    // Below 200 Hz the search starts from the key's pitch, not from the faint fundamental.
    const StringFit fit = fit_inharmonicity(Span(x, at(0.05), static_cast<size_t>(1.0 * rate)), rate, out.f0 < 200.0 ? nominal_hz(key) : out.f0);
    out.inharmonicity = fit.b;
    // A bass string radiates too little of its fundamental to read: its pitch is the fit's.
    if (out.f0 < 200.0) out.f0 = fit.f * std::sqrt(1.0 + fit.b);
    out.bands = harmonic_envelope(early, rate, fit.f, fit.b);
  } else {
    // The first overtone: the strongest component between 2.2 and 4.5 times the fundamental that
    // the blow added, in the 100 ms after it over the 100 ms before (a scale's earlier bars are
    // still ringing), before it has rung away.
    auto m = magnitude(Span(x, at(0.005), static_cast<size_t>(0.1 * rate)), 16);
    if (start > static_cast<size_t>(0.1 * rate)) {
      const auto before = magnitude(Span(x, start - static_cast<size_t>(0.1 * rate), static_cast<size_t>(0.1 * rate)), 16);
      for (size_t k = 0; k < m.size(); ++k) m[k] = std::sqrt(std::max(0.0, m[k] * m[k] - before[k] * before[k]));
    }
    const double bin = rate / 65536.0;
    out.overtone_ratio = peak_in(m, bin, 2.2 * out.f0, std::min(4.5 * out.f0, 0.45 * rate)) / out.f0;
    // The fundamental's own decay: its amplitude in 50 ms windows over the first 1.5 s.
    std::vector<double> c;
    const double floor_tone = floor - 3.0;
    for (double t = 0.0; t < 1.5 && at(t + 0.05) < x.size(); t += 0.05)
      c.push_back(measure::db(measure::tone_amplitude(Span(x, at(t), static_cast<size_t>(0.05 * rate)), rate, out.f0)));
    out.decay_early = decay_rate(c, 0.05, 0.05, 1.5, floor_tone);
    // An envelope as if harmonic: meaningful for an electric piano, whose pickup makes harmonics
    // of the tine; for a bar's inharmonic partials it is not judged.
    out.bands = harmonic_envelope(early, rate, out.f0);
  }
  out.ok = true;
  return out;
}

// The notes of a recorded scale, one at a time. The Iowa scales leave a silence between notes,
// so a note is a stretch at least 15 dB over the recording's floor (the level its quietest 2% of
// 50 ms steps reach, digital silence aside), gaps under 150 ms bridged, at least 0.4 s long.
struct Segment { size_t begin, end; };
inline std::vector<Segment> segments(const std::vector<float>& x, double rate) {
  const double hop_s = 0.05;
  const auto c = measure::contour_db(x, rate, hop_s, hop_s);
  if (c.empty()) return {};
  const double threshold = floor_db(x, rate) + 15.0;
  std::vector<Segment> out;
  size_t i = 0;
  while (i < c.size()) {
    if (c[i] < threshold) { ++i; continue; }
    size_t j = i, quiet = 0;
    while (j < c.size() && quiet < 3) { quiet = c[j] < threshold ? quiet + 1 : 0; ++j; }
    const size_t last = j - quiet;
    if (static_cast<double>(last - i) * hop_s >= 0.4)
      out.push_back({static_cast<size_t>(static_cast<double>(i) * hop_s * rate), std::min(x.size(), static_cast<size_t>(static_cast<double>(last) * hop_s * rate))});
    i = j;
  }
  return out;
}

// How strongly a span holds a harmonic series on `hz` (searched within 40 cents of it): the mean
// log magnitude on its first eight harmonics over the mean halfway between them. A series an
// octave down scores low (half its "harmonics" are empty), one an octave up too (the halfway
// points hold the true harmonics).
inline double salience(const std::vector<double>& m, double bin_hz, double hz) {
  double best = -1e9;
  for (double c = -40.0; c <= 40.0; c += 5.0) {
    const double f = hz * std::exp2(c / 1200.0);
    double on = 0.0, off = 0.0;
    int n = 1;
    for (; n <= 8 && (n + 0.5) * f < 0.45 * bin_hz * static_cast<double>(m.size()) * 2.0; ++n) {
      const auto at = [&](double q) {
        const size_t k = static_cast<size_t>(q / bin_hz + 0.5);
        double v = 0.0;
        for (size_t j = k > 2 ? k - 2 : 0; j <= k + 2 && j < m.size(); ++j) v = std::max(v, m[j]);
        return std::log(v + 1e-12);
      };
      on += at(n * f); off += at((n - 0.5) * f);
    }
    if (n > 1) best = std::max(best, (on - off) / (n - 1));
  }
  return best;
}

// The strongest component within 40 cents of `hz`, in log magnitude: a bar's fundamental.
inline double line_level(const std::vector<double>& m, double bin_hz, double hz) {
  const size_t lo = static_cast<size_t>(hz * std::exp2(-40.0 / 1200.0) / bin_hz), hi = static_cast<size_t>(hz * std::exp2(40.0 / 1200.0) / bin_hz) + 1;
  double v = 0.0;
  for (size_t k = lo; k <= hi && k < m.size(); ++k) v = std::max(v, m[k]);
  return std::log(v + 1e-12);
}

// The segment of a scale that plays `key`. Segments 20 dB under the loudest are the ring of a
// note before them and are passed over. Of the rest, the one where the key's harmonic series
// (or, for a bar, its fundamental) stands most clearly above the series a semitone or two
// either side. Returns false if none does.
inline bool find_note(const std::vector<float>& x, double rate, int key, Segment& found, bool harmonic = true) {
  const size_t win = static_cast<size_t>(0.3 * rate), hop = static_cast<size_t>(0.05 * rate);
  struct Candidate { Segment s; size_t loudest; double level; };
  std::vector<Candidate> candidates;
  double top_level = 0.0;
  for (const Segment& s : segments(x, rate)) {
    Candidate c{s, s.begin, 0.0};
    for (size_t at = s.begin; at + win <= s.end; at += hop) {
      const double r = measure::rms(Span(x, at, win));
      if (r > c.level) { c.level = r; c.loudest = at; }
    }
    top_level = std::max(top_level, c.level);
    candidates.push_back(c);
  }
  double best = 0.0;
  bool any = false;
  for (const Candidate& c : candidates) {
    if (c.level < top_level * 0.1) continue;
    const auto m = magnitude(Span(x, c.loudest, win), 15);
    const double bin = rate / 32768.0;
    const auto score = [&](int k) { return harmonic ? salience(m, bin, nominal_hz(k)) : line_level(m, bin, nominal_hz(k)); };
    const double mine = score(key);
    double others = -1e9;
    for (int d : {-2, -1, 1, 2}) others = std::max(others, score(key + d));
    if (mine - others > best) { best = mine - others; found = c.s; any = true; }
  }
  return any;
}

// How far apart two spectral envelopes are: the RMS difference in dB over the bands both
// measured, after removing the mean difference (a level, not a colour). Negative when fewer than
// three bands are measured in both.
inline double envelope_distance(const std::array<double, kBands>& a, const std::array<double, kBands>& b) {
  std::vector<double> d;
  for (int i = 0; i < kBands; ++i) {
    const double x = a[static_cast<size_t>(i)], y = b[static_cast<size_t>(i)];
    if (x <= -100.0 || y <= -100.0) continue;
    d.push_back(x - y);
  }
  if (d.size() < 3) return -1.0;
  const double m = std::accumulate(d.begin(), d.end(), 0.0) / static_cast<double>(d.size());
  double sq = 0.0;
  for (double v : d) sq += (v - m) * (v - m);
  return std::sqrt(sq / static_cast<double>(d.size()));
}

}  // namespace reference
