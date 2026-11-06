// The organ: flue pipes, one to a key for every stop drawn, simulated as an air jet playing a
// bore. The jet from the flue crosses the mouth, is bent by the acoustic flow in the mouth, and
// meets the upper lip a jet's travel time later, where it switches its flow into or out of the
// pipe (a saturating curve: Fletcher's jet drive, Cook's slide flute, Verge's model). The bore is
// a waveguide: an open pipe reflects, inverted, at both ends and sounds every harmonic; a stopped
// pipe reflects without inversion at its stopper, is half the length for the pitch, and sounds
// only the odd harmonics (the hollow Gedackt). The pipe's walls and open end lose energy as the
// pipe's scale sets (narrow pipes keep fewer harmonics). Wind reaches the pipe through the
// pallet and foot over tens of milliseconds, with a little turbulence while the jet forms: the
// chiff. The tremulant shakes the wind, the chest sags as more pipes draw on it, and each pipe
// stands on the C side or the C# side of the chest, as organ builders lay them out.
//
// Sources: Fletcher and Rossing, The Physics of Musical Instruments chs. 16-17 (flue pipes,
// jet drive, end corrections); Verge, "Aeroacoustics of confined jets" (1995); Cook, "A
// meta-wind-instrument physical model" (1992); Toepfer's normal scale for pipe diameters.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "core.hpp"
#include "envelopes.hpp"
#include "temperament.hpp"

namespace schrodr::dsp::pipes {

constexpr int kRanks = 5;
constexpr double kSpeedOfSound = 343.0;

struct Rank {
  const char* name;
  double footage;        // 8, 4, 2, 2 2/3: the pitch relative to the key
  bool stopped;
  double diameter_c2_mm; // Toepfer: diameter at 8' C (C2), halving every `halving` semitones
  double halving;
  double brightness;     // how far the jet is bent by the mouth's flow (the voicing's wind and cut-up)
  double offset;         // where the jet rests against the labium: off-centre makes even harmonics
  double loss;           // the bore's loss per round trip, as a fraction
  double chiff;          // this rank's share of the chiff
};

inline const std::array<Rank, kRanks>& ranks() {
  static const std::array<Rank, kRanks> table{{
      {"Open diapason 8'", 8.0, false, 155.0, 17.0, 0.9, 0.8, 0.2, 1.0},
      {"Gedackt 8'", 8.0, true, 200.0, 18.0, 0.8, 0.35, 0.22, 0.6},
      {"Principal 4'", 4.0, false, 92.0, 17.0, 0.9, 0.8, 0.2, 1.0},
      {"Fifteenth 2'", 2.0, false, 55.0, 17.0, 0.95, 0.8, 0.2, 1.0},
      {"Quint 2 2/3'", 8.0 / 3.0, false, 78.0, 17.0, 0.85, 0.7, 0.2, 0.8},
  }};
  return table;
}

constexpr int kLow = 36, kHigh = 96;  // C2 to C7, 61 keys
constexpr int kKeys = kHigh - kLow + 1;

struct Setup {
  std::array<bool, kRanks> drawn{true, false, false, false, false};
  double wind = 1.0;         // wind pressure against the voicing's
  double tremulant = 0.0, tremulant_hz = 5.0;
  double chiff = 0.5;
  double a4 = 440.0;
  int temperament = 0, root = 0;
};

// One pipe's voicing at prepare: its bore and jet lengths and filters.
struct Pipe {
  double hz = 261.0, bore = 100.0, jet = 30.0, gain = 0.97, lp = 0.0, hp = 0.995, drive = 1.0, offset = 0.5, pan = 0.0;
  double correction = 1.0;  // the bore against the period, found by playing the pipe once
  bool stopped = false;
};

class Organ {
 public:
  struct Voice {
    int key = -1, rank = 0;
    bool awake = false, held = false;
    double pressure = 0.0;       // the wind at the foot, rising and falling
    std::vector<float> bore_buf, jet_buf;
    size_t bore_write = 0, jet_write = 0;
    float bore_out = 0.0f, lp_state = 0.0f, lp_state2 = 0.0f, dc_x = 0.0f, dc_y = 0.0f;
    float env = 0.0f;
    int age = 0;
  };

  void prepare(double rate) {
    rate_ = rate;
    const size_t longest = static_cast<size_t>(rate / 60.0) + 8;
    for (int k = 0; k < kKeys; ++k)
      for (int r = 0; r < kRanks; ++r) {
        Voice& v = voices_[static_cast<size_t>(k)][static_cast<size_t>(r)];
        v.key = kLow + k; v.rank = r;
        v.bore_buf.assign(longest, 0.0f);
        v.jet_buf.assign(static_cast<size_t>(rate / 50.0) + 8, 0.0f);
      }
    tremulant_.prepare(rate);
    configured_ = false;
    voice_all(true);
    configured_ = true;
    reset();
  }
  void reset() {
    for (auto& row : voices_) for (auto& v : row) silence(v);
    sag_ = 1.0;
    held_keys_.fill(false);
  }
  void configure(const Setup& in) {
    const Setup before = setup_;
    setup_ = in;
    if (!configured_ || setup_.a4 != before.a4 || setup_.temperament != before.temperament || setup_.root != before.root) voice_all(!configured_);
    configured_ = true;
    // A stop drawn while keys are down speaks at once; one pushed in stops its pipes.
    for (int k = 0; k < kKeys; ++k)
      if (held_keys_[static_cast<size_t>(k)])
        for (int r = 0; r < kRanks; ++r) {
          Voice& v = voices_[static_cast<size_t>(k)][static_cast<size_t>(r)];
          if (setup_.drawn[static_cast<size_t>(r)] && !v.held) speak(v);
          if (!setup_.drawn[static_cast<size_t>(r)] && v.held) v.held = false;
        }
  }
  const Setup& setup() const { return setup_; }

  void note_on(int key, double velocity, uint32_t seed) {
    (void)velocity; (void)seed;
    key = into_range(key);
    if (key < 0) return;
    held_keys_[static_cast<size_t>(key - kLow)] = true;
    for (int r = 0; r < kRanks; ++r) if (setup_.drawn[static_cast<size_t>(r)]) speak(voices_[static_cast<size_t>(key - kLow)][static_cast<size_t>(r)]);
  }
  void note_off(int key) {
    key = into_range(key);
    if (key < 0) return;
    held_keys_[static_cast<size_t>(key - kLow)] = false;
    for (auto& v : voices_[static_cast<size_t>(key - kLow)]) v.held = false;
  }
  void all_notes_off() { for (auto& row : voices_) for (auto& v : row) v.held = false; held_keys_.fill(false); }
  void all_sound_off() { reset(); }
  bool awake() const { for (const auto& row : voices_) for (const auto& v : row) if (v.awake) return true; return false; }
  int sounding() const { int n = 0; for (const auto& row : voices_) for (const auto& v : row) n += v.awake; return n; }
  const Pipe& pipe(int key, int rank) const { return pipes_[static_cast<size_t>(std::clamp(key - kLow, 0, kKeys - 1))][static_cast<size_t>(std::clamp(rank, 0, kRanks - 1))]; }
  double pitch_at(int key, int rank = 0) const { return pipe(key, rank).hz; }

  void render(float& left, float& right) {
    left = right = 0.0f;
    // The chest: the tremulant's shake and the sag under the pipes drawing on it.
    const double trem = 1.0 + 0.2 * std::clamp(setup_.tremulant, 0.0, 1.0) * tremulant_.next(setup_.tremulant_hz);
    int speaking = 0;
    for (const auto& row : voices_) for (const auto& v : row) speaking += v.held;
    const double sag_target = 1.0 - 0.012 * std::max(0, speaking - 1);
    sag_ += (sag_target - sag_) * (1.0 - std::exp(-1.0 / (0.05 * rate_)));
    const double wind = std::clamp(setup_.wind, 0.3, 2.0) * trem * sag_;
    for (auto& row : voices_)
      for (auto& v : row) {
        if (!v.awake) continue;
        const float out = tick(v, wind);
        const Pipe& p = pipes_[static_cast<size_t>(v.key - kLow)][static_cast<size_t>(v.rank)];
        const double angle = (p.pan * 0.5 + 0.5) * kPi * 0.5;
        left += out * static_cast<float>(std::cos(angle));
        right += out * static_cast<float>(std::sin(angle));
      }
    left *= static_cast<float>(output_scale_);
    right *= static_cast<float>(output_scale_);
  }

 private:
  int into_range(int key) const {
    if (key < 0 || key > 127) return -1;
    while (key < kLow) key += 12;
    while (key > kHigh) key -= 12;
    return key < kLow ? -1 : key;
  }

  // The pipe's voicing from its rank and key: bore length from the pitch (its end correction
  // found by playing it, once, at prepare), the jet's travel time most of a period (the
  // fundamental's regime), the bore's losses from the pipe's diameter against its length.
  void voice_all(bool calibrate) {
    for (int k = 0; k < kKeys; ++k)
      for (int r = 0; r < kRanks; ++r) {
        const Rank& rank = ranks()[static_cast<size_t>(r)];
        Pipe& p = pipes_[static_cast<size_t>(k)][static_cast<size_t>(r)];
        const int key = kLow + k;
        p.hz = temperament::hz(setup_.temperament, setup_.root, setup_.a4, key) * 8.0 / rank.footage;
        p.stopped = rank.stopped;
        const double period = rate_ / p.hz;
        const double diameter = rank.diameter_c2_mm * 1e-3 * std::exp2(-(key - 36 + 12.0 * std::log2(8.0 / rank.footage)) / rank.halving);
        const double length = kSpeedOfSound / (p.stopped ? 4.0 : 2.0) / p.hz;
        // Wall losses grow with the pipe's length against its diameter; the open end and the
        // mouth stop reflecting (and radiate instead) above c / (2 pi a), where a is the pipe's
        // radius: a wide bass pipe keeps few harmonics, a narrow treble one many.
        // A pipe's resonance has a Q of a few tens: the open end radiates and the walls rub.
        const double slenderness = length / std::max(diameter, 1e-3);
        p.gain = 1.0 - rank.loss * std::clamp(slenderness / 16.0, 0.85, 1.15);
        // No pipe resonates below its fundamental, and an open end passes steady flow straight
        // out: the loop is blocked below a quarter of the pitch.
        p.hp = std::exp(-kTau * (p.hz * 0.25) / rate_);
        const double corner = std::clamp(0.8 * kSpeedOfSound / (kPi * diameter), 400.0, 12000.0);
        p.lp = std::exp(-kTau * corner / rate_);
        // The jet's travel across the mouth takes most of a period in the fundamental's regime
        // (Fletcher); faster wind shortens it toward the octave's.
        p.jet = std::clamp(period * 0.93, 2.0, static_cast<double>(rate_ / 50.0) - 2.0);
        // Bass pipes are voiced with more wind for their size, or they would be slow to speak.
        p.drive = rank.brightness * (key < 48 ? 1.3 : key < 60 ? 1.1 : 1.0);
        p.offset = rank.offset;
        const double nominal = (p.stopped ? 0.5 : 1.0) * period - 1.0;
        // Organ builders stand the pipes in two rows, C and C#, and the pair alternate sides.
        p.pan = (key % 2 == 0 ? -1.0 : 1.0) * (0.3 + 0.4 * static_cast<double>(k) / kKeys);
        // The loop's filters and the jet add phase: play the pipe for a moment, read its pitch and
        // shorten or lengthen the bore until it speaks on the key's note. The correction holds
        // when the tuning moves a little, so it is found once.
        if (calibrate) {
          p.bore = nominal;
          for (int trial = 0; trial < 4; ++trial) {
            const double sounded = trial_pitch(p);
            if (sounded > 0.7 * p.hz && sounded < 1.4 * p.hz) p.bore = std::max(2.0, p.bore * sounded / p.hz);
            if (std::abs(sounded / p.hz - 1.0) < 0.002) break;
          }
          p.correction = p.bore / nominal;
        }
        p.bore = std::max(2.0, nominal * p.correction);
      }
  }

  // Runs a pipe alone for half a second and reads its period from the rising zero crossings
  // of its last third, once the pipe has settled.
  double trial_pitch(const Pipe& p) {
    Voice v;
    v.bore_buf.assign(static_cast<size_t>(rate_ / 60.0) + 8, 0.0f);
    v.jet_buf.assign(static_cast<size_t>(rate_ / 50.0) + 8, 0.0f);
    v.key = kLow; v.rank = 0; v.awake = true; v.held = true; v.pressure = 1.0;
    const int n = static_cast<int>(rate_ * 0.5);
    int crossings = 0, first = -1, last = -1;
    float prev = 0.0f, peak = 0.0f;
    for (int i = 0; i < n; ++i) {
      const float x = tick_pipe(v, p, 1.0, false);
      if (i > 2 * n / 3) {
        peak = std::max(peak, std::abs(x));
        if (prev <= 0.0f && x > 0.0f) { if (first < 0) first = i; last = i; ++crossings; }
      }
      prev = x;
    }
    if (crossings < 6 || last <= first || peak < 0.05f) return 0.0;
    return rate_ * (crossings - 1) / static_cast<double>(last - first);
  }

  void speak(Voice& v) {
    if (!v.awake) { silence(v); v.awake = true; v.age = 0; }
    v.held = true;
  }
  void silence(Voice& v) {
    v.awake = v.held = false;
    v.pressure = 0.0;
    std::fill(v.bore_buf.begin(), v.bore_buf.end(), 0.0f);
    std::fill(v.jet_buf.begin(), v.jet_buf.end(), 0.0f);
    v.bore_out = v.lp_state = v.lp_state2 = v.dc_x = v.dc_y = 0.0f; v.env = 0.0f;
  }

  float tick(Voice& v, double wind) {
    const Pipe& p = pipes_[static_cast<size_t>(v.key - kLow)][static_cast<size_t>(v.rank)];
    // The pallet opens: wind fills the foot over about 20 ms with a little overshoot; closed, it
    // drains over 30 ms.
    const double target = v.held ? wind * (1.0 + 0.15 * std::exp(-v.age / (0.03 * rate_))) : 0.0;
    v.pressure += (target - v.pressure) * (v.held ? 1.0 - std::exp(-1.0 / (0.012 * rate_)) : 1.0 - std::exp(-1.0 / (0.03 * rate_)));
    ++v.age;
    // What the mouth radiates grows with the jet's speed: a flue pipe's sound pressure goes
    // about as the wind pressure to the three-quarters (its power as the pressure to the 3/2),
    // over and above the loop's own level, which the jet's saturation holds nearly constant.
    const float out = tick_pipe(v, p, v.pressure, true) * static_cast<float>(std::pow(std::max(v.pressure, 0.0), 0.75));
    v.env = std::max(std::abs(out), v.env * 0.9995f);
    if (!v.held && v.pressure < 0.01 && v.env < 1e-5f) silence(v);
    return out;
  }

  // One sample of the jet and bore: the wave back from the far end (the bore's losses and the
  // end's radiation taken off, inverted at a stopper), the jet it bends on its way to the labium
  // (a travel time shortened by harder wind), the flow the jet switches into the pipe there (a
  // saturating curve, off-centre so the even harmonics come), and the mouth's own turbulence.
  float tick_pipe(Voice& v, const Pipe& p, double pressure, bool noisy) {
    const size_t bn = v.bore_buf.size(), jn = v.jet_buf.size();
    const auto read = [](const std::vector<float>& buf, size_t write, double delay) {
      const size_t n = buf.size();
      const double d = std::clamp(delay, 1.0, static_cast<double>(n - 2));
      const size_t i = static_cast<size_t>(d);
      const float f = static_cast<float>(d - static_cast<double>(i));
      const float a = buf[(write + n - i) % n], b = buf[(write + n - i - 1) % n];
      return a + (b - a) * f;
    };
    const float bore = read(v.bore_buf, v.bore_write, p.bore);
    // Two poles at the end's radiation corner: a wide bass pipe keeps little above it.
    v.lp_state = static_cast<float>(p.lp) * v.lp_state + (1.0f - static_cast<float>(p.lp)) * bore;
    v.lp_state2 = static_cast<float>(p.lp) * v.lp_state2 + (1.0f - static_cast<float>(p.lp)) * v.lp_state;
    float returned = v.lp_state2 * static_cast<float>(p.gain) * (p.stopped ? -1.0f : 1.0f);
    const float y = returned - v.dc_x + static_cast<float>(p.hp) * v.dc_y;
    v.dc_x = returned; v.dc_y = y; returned = y;
    // The mouth's turbulence: a little always, more while the jet is still forming (the chiff),
    // some of it radiating straight out of the mouth.
    const double establishing = std::clamp(1.2 - pressure, 0.0, 1.0);
    const float noise = noisy ? rng_.bipolar() * static_cast<float>(0.004 + 0.3 * setup_.chiff * establishing) : rng_.bipolar() * 0.002f;
    v.jet_buf[v.jet_write] = returned + noise;
    v.jet_write = (v.jet_write + 1) % jn;
    const double wind = std::max(0.05, pressure);
    // The jet crosses the mouth faster under harder wind (its speed goes as the root of the
    // pressure), though the voicer's cut-up and the jet's spread keep the pipe on its fundamental
    // over the range a wind trunk sags and swells through: taken as the fourth root here.
    const double travel = p.jet / std::pow(wind, 0.25);
    const float bend = static_cast<float>(p.drive * wind) * read(v.jet_buf, v.jet_write, travel);
    // The jet switches almost wholly in or out of the pipe: a hard curve, off-centre.
    const float offset = static_cast<float>(p.offset);
    const float flow = std::tanh(bend + offset) - std::tanh(offset);
    // The jet's flow goes as its speed, the square root of the pressure: harder wind is louder.
    v.bore_buf[v.bore_write] = returned + 1.6f * static_cast<float>(std::sqrt(std::min(1.0, pressure * 1.5) * std::max(1.0, pressure))) * flow + 0.5f * noise;
    v.bore_write = (v.bore_write + 1) % bn;
    v.bore_out = returned + 1.5f * noise;
    return v.bore_out;
  }

  double rate_ = 48000.0, output_scale_ = 0.15, sag_ = 1.0;
  Setup setup_{};
  bool configured_ = false;
  std::array<std::array<Voice, kRanks>, kKeys> voices_{};
  std::array<std::array<Pipe, kRanks>, kKeys> pipes_{};
  std::array<bool, kKeys> held_keys_{};
  Lfo tremulant_;
  Rng rng_{0x7a3e1d5bu};
};

}  // namespace schrodr::dsp::pipes
