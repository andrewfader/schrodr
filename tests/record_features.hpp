// A stem's features, measured exactly as the song corpus measures the separated stems of real
// records (song-corpus/scripts/features.py, librosa 1.0), so a render of a preset and a stem from a
// record can be compared number for number:
//
//   spectral: the mean power spectrum of the mono sum (STFT n_fft 8192, hop 2048, periodic Hann,
//             centred frames padded with zeros) over frames within 40 dB of the loudest; from
//             20 Hz its centroid, 85% roll-off and share above 3 kHz.
//   crest:    over consecutive 100 ms blocks within 20 dB of the loudest, the median of peak over
//             RMS, in dB.
//   corr:     the zero-lag correlation of left and right, full band.
//   wobble:   pYIN (librosa.pyin: 65 to 1050 Hz, frames of 2048 at 22.05 kHz, hop 256, 100
//             thresholds under a beta(2, 18) prior, Boltzmann trough prior, 10-cent pitch bins,
//             Viterbi through a triangular transition five semitones wide), then in each run of
//             at least 30 voiced frames that never jumps 60 cents, the standard deviation of the
//             pitch around its own 21-frame moving average; the median over runs, in cents.
//
// The one step not reproduced bit for bit is the resampling to 22.05 kHz (librosa uses soxr; a
// linear-phase windowed sinc stands in), checked against features.py on the gated presets'
// renders with tools/record_gate_try.cpp.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <vector>

#include "audio_measure.hpp"

namespace record {

struct Features {
  double centroid_hz = 0.0, rolloff85_hz = 0.0, hf_share_db = 0.0, crest100_db = 0.0, corr = 1.0;
  double wobble_cents = 0.0, voiced_frac = 0.0;
  bool has_wobble = false;
};

namespace detail {

inline double median(std::vector<double> v) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  const size_t n = v.size();
  return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

inline double db10(double x) { return 10.0 * std::log10(std::max(x, 1e-20)); }

inline void spectral(const std::vector<float>& y, double rate, Features& f) {
  constexpr int n_fft = 8192, hop = 2048, pad = n_fft / 2;
  const long n = static_cast<long>(y.size());
  const long frames = 1 + n / hop;
  std::vector<double> window(n_fft);
  for (int i = 0; i < n_fft; ++i) window[static_cast<size_t>(i)] = 0.5 - 0.5 * std::cos(2.0 * measure::kPi * i / n_fft);
  std::vector<std::vector<double>> S;
  S.reserve(static_cast<size_t>(frames));
  std::vector<std::complex<double>> buffer(n_fft);
  for (long t = 0; t < frames; ++t) {
    const long start = t * hop - pad;
    for (int i = 0; i < n_fft; ++i) {
      const long j = start + i;
      buffer[static_cast<size_t>(i)] = (j >= 0 && j < n ? y[static_cast<size_t>(j)] : 0.0) * window[static_cast<size_t>(i)];
    }
    measure::fft(buffer);
    std::vector<double> p(n_fft / 2 + 1);
    for (size_t k = 0; k < p.size(); ++k) p[k] = std::norm(buffer[k]);
    S.push_back(std::move(p));
  }
  std::vector<double> e(S.size(), 0.0);
  double emax = 0.0;
  for (size_t t = 0; t < S.size(); ++t) { for (double v : S[t]) e[t] += v; emax = std::max(emax, e[t]); }
  std::vector<double> P(n_fft / 2 + 1, 0.0);
  size_t active = 0;
  for (size_t t = 0; t < S.size(); ++t)
    if (e[t] > emax * 1e-4) { ++active; for (size_t k = 0; k < P.size(); ++k) P[k] += S[t][k]; }
  for (double& v : P) v /= static_cast<double>(std::max<size_t>(active, 1));
  const auto hz = [&](size_t k) { return static_cast<double>(k) * rate / n_fft; };
  double sum = 0.0, weighted = 0.0, high = 0.0;
  for (size_t k = 0; k < P.size(); ++k)
    if (hz(k) >= 20.0) { sum += P[k]; weighted += P[k] * hz(k); if (hz(k) >= 3000.0) high += P[k]; }
  f.centroid_hz = weighted / std::max(sum, 1e-30);
  double running = 0.0;
  for (size_t k = 0; k < P.size(); ++k)
    if (hz(k) >= 20.0) { running += P[k]; if (running / sum >= 0.85) { f.rolloff85_hz = hz(k); break; } }
  f.hf_share_db = db10(high / std::max(sum, 1e-30));
}

inline double crest100(const std::vector<float>& y, double rate) {
  const size_t hop = static_cast<size_t>(0.1 * rate), blocks = y.size() / hop;
  std::vector<double> rms(blocks), peak(blocks);
  double top = 0.0;
  for (size_t b = 0; b < blocks; ++b) {
    double sq = 0.0, pk = 0.0;
    for (size_t i = b * hop; i < (b + 1) * hop; ++i) { sq += static_cast<double>(y[i]) * y[i]; pk = std::max(pk, static_cast<double>(std::abs(y[i]))); }
    rms[b] = std::sqrt(sq / static_cast<double>(hop)) + 1e-12;
    peak[b] = pk;
    top = std::max(top, rms[b]);
  }
  std::vector<double> crest;
  for (size_t b = 0; b < blocks; ++b) if (rms[b] > top * 0.1) crest.push_back(20.0 * std::log10(peak[b] / rms[b]));
  return median(crest);
}

// 44.1 to 22.05 kHz: a 255-tap Kaiser-windowed sinc (beta 9) cut at 10.3 kHz, centred so the
// output lines up with the input as soxr's does.
inline std::vector<float> halve_rate(const std::vector<float>& x) {
  constexpr int taps = 255, half = taps / 2;
  constexpr double cutoff = 10300.0 / 44100.0, beta = 9.0;
  const auto bessel0 = [](double v) { double sum = 1.0, term = 1.0; for (int k = 1; k < 40; ++k) { term *= (v / (2.0 * k)) * (v / (2.0 * k)); sum += term; } return sum; };
  std::array<double, taps> h{};
  double total = 0.0;
  for (int i = 0; i < taps; ++i) {
    const double m = i - half, r = m / half;
    const double sinc = m == 0 ? 2.0 * cutoff : std::sin(2.0 * measure::kPi * cutoff * m) / (measure::kPi * m);
    h[static_cast<size_t>(i)] = sinc * bessel0(beta * std::sqrt(std::max(0.0, 1.0 - r * r))) / bessel0(beta);
    total += h[static_cast<size_t>(i)];
  }
  for (double& v : h) v /= total;
  const long n = static_cast<long>(x.size());
  std::vector<float> out(static_cast<size_t>((n + 1) / 2));
  for (long o = 0; o < static_cast<long>(out.size()); ++o) {
    double acc = 0.0;
    for (int i = 0; i < taps; ++i) {
      const long j = 2 * o + i - half;
      if (j >= 0 && j < n) acc += h[static_cast<size_t>(i)] * x[static_cast<size_t>(j)];
    }
    out[static_cast<size_t>(o)] = static_cast<float>(acc);
  }
  return out;
}

// librosa.pyin's f0 track: the pitch in Hz per frame, or 0 where it decodes unvoiced.
inline std::vector<double> pyin(const std::vector<float>& y, double sr = 22050.0, double fmin = 65.0, double fmax = 1050.0) {
  constexpr int frame = 2048, hop = 256, n_thresholds = 100, per_semitone = 10;
  const int min_period = static_cast<int>(std::floor(sr / fmax));
  const int max_period = std::min(static_cast<int>(std::ceil(sr / fmin)), frame - 1);
  const int periods = max_period - min_period + 1;
  const int bins = static_cast<int>(std::floor(12 * per_semitone * std::log2(fmax / fmin))) + 1;
  const long n = static_cast<long>(y.size());
  const long frames = 1 + n / hop;

  // The threshold prior: beta(2, 18)'s probability between successive thresholds.
  const auto beta_cdf = [](double x) {
    // I_x(2, 18) = sum_{j=2}^{19} C(19, j) x^j (1 - x)^(19 - j)
    double sum = 0.0, c = 1.0;
    for (int j = 0; j <= 19; ++j) {
      if (j > 0) c = c * (19 - j + 1) / j;
      if (j >= 2) sum += c * std::pow(x, j) * std::pow(1.0 - x, 19 - j);
    }
    return sum;
  };
  std::array<double, n_thresholds + 1> thresholds{};
  std::array<double, n_thresholds> beta_probs{};
  for (int j = 0; j <= n_thresholds; ++j) thresholds[static_cast<size_t>(j)] = static_cast<double>(j) / n_thresholds;
  for (int j = 0; j < n_thresholds; ++j)
    beta_probs[static_cast<size_t>(j)] = beta_cdf(thresholds[static_cast<size_t>(j + 1)]) - beta_cdf(thresholds[static_cast<size_t>(j)]);
  const double lambda = 2.0, no_trough_prob = 0.01;
  const auto boltzmann = [&](int k, int count) { return (1.0 - std::exp(-lambda)) * std::exp(-lambda * k) / (1.0 - std::exp(-lambda * count)); };

  const int states = 2 * bins;
  const double tiny = std::numeric_limits<double>::min();
  std::vector<float> log_obs(static_cast<size_t>(frames) * static_cast<size_t>(states));
  std::vector<std::complex<double>> fft(4096);
  std::vector<double> d(static_cast<size_t>(max_period) + 1), cmnd(static_cast<size_t>(periods)), shift(static_cast<size_t>(periods));
  std::vector<double> energy(frame), obs(static_cast<size_t>(states));
  std::vector<double> frame_y(frame);
  for (long t = 0; t < frames; ++t) {
    const long start = t * hop - frame / 2;
    for (int i = 0; i < frame; ++i) { const long j = start + i; frame_y[static_cast<size_t>(i)] = j >= 0 && j < n ? y[static_cast<size_t>(j)] : 0.0; }
    for (size_t i = 0; i < fft.size(); ++i) fft[i] = i < static_cast<size_t>(frame) ? frame_y[i] : 0.0;
    measure::fft(fft);
    for (auto& v : fft) v = std::norm(v);
    for (auto& v : fft) v = std::conj(v);
    measure::fft(fft);
    const double scale = 1.0 / static_cast<double>(fft.size());
    double running = 0.0;
    for (int i = 0; i < frame; ++i) { running += frame_y[static_cast<size_t>(i)] * frame_y[static_cast<size_t>(i)]; energy[static_cast<size_t>(i)] = running; }
    const double acf0 = fft[0].real() * scale;
    d[0] = 0.0;
    for (int k = 1; k <= max_period; ++k) d[static_cast<size_t>(k)] = 2.0 * (acf0 - fft[static_cast<size_t>(k)].real() * scale) - energy[static_cast<size_t>(k - 1)];
    double cumulative = 0.0;
    for (int k = 1; k <= max_period; ++k) {
      cumulative += d[static_cast<size_t>(k)];
      if (k >= min_period) cmnd[static_cast<size_t>(k - min_period)] = d[static_cast<size_t>(k)] / (cumulative / k + tiny);
    }
    for (int i = 0; i < periods; ++i) {
      shift[static_cast<size_t>(i)] = 0.0;
      if (i == 0 || i == periods - 1) continue;
      const double a = cmnd[static_cast<size_t>(i + 1)] + cmnd[static_cast<size_t>(i - 1)] - 2.0 * cmnd[static_cast<size_t>(i)];
      const double b = 0.5 * (cmnd[static_cast<size_t>(i + 1)] - cmnd[static_cast<size_t>(i - 1)]);
      if (std::abs(b) < std::abs(a)) shift[static_cast<size_t>(i)] = -b / a;
    }
    std::vector<int> troughs;
    for (int i = 0; i < periods; ++i) {
      const double x = cmnd[static_cast<size_t>(i)];
      bool is = false;
      if (i == 0) is = x < cmnd[1];
      else if (i == periods - 1) is = x < cmnd[static_cast<size_t>(i - 1)];
      else is = x < cmnd[static_cast<size_t>(i - 1)] && x <= cmnd[static_cast<size_t>(i + 1)];
      if (is) troughs.push_back(i);
    }
    std::fill(obs.begin(), obs.end(), 0.0);
    if (!troughs.empty()) {
      std::vector<double> probs(troughs.size(), 0.0);
      for (int j = 0; j < n_thresholds; ++j) {
        const double thr = thresholds[static_cast<size_t>(j + 1)];
        int count = 0;
        for (int i : troughs) count += cmnd[static_cast<size_t>(i)] < thr;
        int position = 0;
        for (size_t q = 0; q < troughs.size(); ++q)
          if (cmnd[static_cast<size_t>(troughs[q])] < thr) probs[q] += boltzmann(position++, count) * beta_probs[static_cast<size_t>(j)];
      }
      size_t global = 0;
      for (size_t q = 1; q < troughs.size(); ++q) if (cmnd[static_cast<size_t>(troughs[q])] < cmnd[static_cast<size_t>(troughs[global])]) global = q;
      double below = 0.0;
      for (int j = 0; j < n_thresholds; ++j) if (!(cmnd[static_cast<size_t>(troughs[global])] < thresholds[static_cast<size_t>(j + 1)])) below += beta_probs[static_cast<size_t>(j)];
      probs[global] += no_trough_prob * below;
      for (size_t q = 0; q < troughs.size(); ++q) {
        if (probs[q] == 0.0) continue;
        const int i = troughs[q];
        const double period = min_period + i + shift[static_cast<size_t>(i)];
        const double bin = std::nearbyint(12.0 * per_semitone * std::log2(sr / period / fmin));
        const int index = static_cast<int>(std::clamp(bin, 0.0, static_cast<double>(bins)));
        if (index < bins) obs[static_cast<size_t>(index)] = probs[q];
      }
    }
    double voiced = 0.0;
    for (int b = 0; b < bins; ++b) voiced += obs[static_cast<size_t>(b)];
    voiced = std::clamp(voiced, 0.0, 1.0);
    for (int b = bins; b < states; ++b) obs[static_cast<size_t>(b)] = (1.0 - voiced) / bins;
    for (int s = 0; s < states; ++s) log_obs[static_cast<size_t>(t) * static_cast<size_t>(states) + static_cast<size_t>(s)] = static_cast<float>(std::log(obs[static_cast<size_t>(s)] + tiny));
  }

  // The transition: a triangle five semitones either way inside voiced or unvoiced, kept 0.99,
  // switched 0.01; only transitions of at least 1e-4 are searched, as librosa does.
  const int max_semitones = static_cast<int>(std::lround(35.92 * 12.0 * hop / sr));
  const int width = max_semitones * per_semitone + 1, reach = width / 2;
  std::vector<double> tri(static_cast<size_t>(width));
  for (int i = 0; i < width; ++i) tri[static_cast<size_t>(i)] = 1.0 - std::abs(i - (width - 1) / 2.0) / ((width + 1) / 2.0);
  std::vector<std::vector<double>> row(static_cast<size_t>(bins), std::vector<double>(static_cast<size_t>(width), 0.0));
  for (int i = 0; i < bins; ++i) {
    double sum = 0.0;
    for (int o = -reach; o <= reach; ++o) if (i + o >= 0 && i + o < bins) sum += tri[static_cast<size_t>(o + reach)];
    for (int o = -reach; o <= reach; ++o) if (i + o >= 0 && i + o < bins) row[static_cast<size_t>(i)][static_cast<size_t>(o + reach)] = tri[static_cast<size_t>(o + reach)] / sum;
  }
  const double log_floor = std::log(1e-4 + tiny);
  struct Pred { int from; double log_p; };
  std::vector<std::vector<Pred>> preds(static_cast<size_t>(states));
  for (int j = 0; j < states; ++j) {
    const int jb = j % bins, jv = j / bins;
    for (int kv = 0; kv < 2; ++kv)
      for (int kb = std::max(0, jb - reach); kb <= std::min(bins - 1, jb + reach); ++kb) {
        const double p = row[static_cast<size_t>(kb)][static_cast<size_t>(jb - kb + reach)] * (kv == jv ? 0.99 : 0.01);
        const double lp = std::log(p + tiny);
        if (lp >= log_floor) preds[static_cast<size_t>(j)].push_back({kv * bins + kb, lp});
      }
    std::sort(preds[static_cast<size_t>(j)].begin(), preds[static_cast<size_t>(j)].end(), [](const Pred& a, const Pred& b) { return a.from < b.from; });
  }
  std::vector<double> value(static_cast<size_t>(states)), next(static_cast<size_t>(states));
  std::vector<uint16_t> ptr(static_cast<size_t>(frames) * static_cast<size_t>(states), 0);
  const double log_init = std::log(1.0 / states + tiny);
  for (int s = 0; s < states; ++s) value[static_cast<size_t>(s)] = log_obs[static_cast<size_t>(s)] + log_init;
  for (long t = 1; t < frames; ++t) {
    for (int j = 0; j < states; ++j) {
      double best = -std::numeric_limits<double>::infinity();
      int arg = 0;
      for (const Pred& p : preds[static_cast<size_t>(j)]) {
        const double cost = value[static_cast<size_t>(p.from)] + p.log_p;
        if (cost > best) { best = cost; arg = p.from; }
      }
      ptr[static_cast<size_t>(t) * static_cast<size_t>(states) + static_cast<size_t>(j)] = static_cast<uint16_t>(arg);
      next[static_cast<size_t>(j)] = log_obs[static_cast<size_t>(t) * static_cast<size_t>(states) + static_cast<size_t>(j)] + best;
    }
    value.swap(next);
  }
  std::vector<int> path(static_cast<size_t>(frames));
  path.back() = static_cast<int>(std::max_element(value.begin(), value.end()) - value.begin());
  for (long t = frames - 2; t >= 0; --t)
    path[static_cast<size_t>(t)] = ptr[static_cast<size_t>(t + 1) * static_cast<size_t>(states) + static_cast<size_t>(path[static_cast<size_t>(t + 1)])];
  std::vector<double> f0(static_cast<size_t>(frames), 0.0);
  for (long t = 0; t < frames; ++t)
    if (path[static_cast<size_t>(t)] < bins) f0[static_cast<size_t>(t)] = fmin * std::exp2(path[static_cast<size_t>(t)] / (12.0 * per_semitone));
  return f0;
}

inline void wobble(const std::vector<double>& f0, Features& f) {
  size_t voiced = 0;
  for (double v : f0) voiced += v > 0.0;
  f.voiced_frac = f0.empty() ? 0.0 : static_cast<double>(voiced) / static_cast<double>(f0.size());
  if (voiced < 40) return;
  std::vector<double> wob, run;
  const auto close = [&] {
    if (run.size() >= 30) {
      constexpr size_t k = 21;
      const size_t m = run.size() - k + 1;
      double sum = 0.0, sq = 0.0;
      std::vector<double> residual(m);
      for (size_t i = 0; i < m; ++i) {
        double avg = 0.0;
        for (size_t q = 0; q < k; ++q) avg += run[i + q];
        residual[i] = run[i + k / 2] - avg / k;
        sum += residual[i];
      }
      const double mean = sum / static_cast<double>(m);
      for (double r : residual) sq += (r - mean) * (r - mean);
      wob.push_back(std::sqrt(sq / static_cast<double>(m)));
    }
  };
  for (double hz : f0) {
    const bool finite = hz > 0.0;
    const double cents = finite ? 1200.0 * std::log2(hz / 440.0) : 0.0;
    if (finite && (run.empty() || std::abs(cents - run.back()) < 60.0)) run.push_back(cents);
    else { close(); run.clear(); if (finite) run.push_back(cents); }
  }
  if (!wob.empty()) { f.wobble_cents = median(wob); f.has_wobble = true; }
}

}  // namespace detail

// Measures a stereo render at 44.1 kHz.
inline Features measure_stem(const std::vector<float>& left, const std::vector<float>& right, double rate = 44100.0) {
  Features f;
  std::vector<float> y(left.size());
  for (size_t i = 0; i < y.size(); ++i) y[i] = 0.5f * (left[i] + right[i]);
  detail::spectral(y, rate, f);
  f.crest100_db = detail::crest100(y, rate);
  double ll = 0.0, rr = 0.0, lr = 0.0;
  for (size_t i = 0; i < left.size(); ++i) { ll += static_cast<double>(left[i]) * left[i]; rr += static_cast<double>(right[i]) * right[i]; lr += static_cast<double>(left[i]) * right[i]; }
  f.corr = lr / (std::sqrt(ll * rr) + 1e-20);
  detail::wobble(detail::pyin(detail::halve_rate(y)), f);
  return f;
}

}  // namespace record
