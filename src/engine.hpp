// The instrument: one keyboard (strings, bars or pipes) played from the keys and pedals, heard
// in a room through the air, through the master level and an output limiter. The engine knows
// nothing about CLAP: it takes a snapshot of parameter values per block and note events with
// sample offsets, and it never allocates while rendering. There are no voices in the synthesizer
// sense: a key plays the instrument's own strings, bars or pipes, which know how many of them
// can sound at once.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "dsp/air.hpp"
#include "dsp/bars.hpp"
#include "dsp/core.hpp"
#include "dsp/keyboard.hpp"
#include "dsp/pipes.hpp"
#include "params.hpp"

namespace schrodr {

// A block's worth of parameter values, copied out of the plug-in's atomics once per process
// call so everything in the block sees the same settings.
struct Settings {
  std::array<double, params::kParamCount> v{};
  double operator[](params::ParamId id) const { return v[static_cast<size_t>(id)]; }
  int instrument() const { return std::clamp(static_cast<int>(std::lround(v[params::kInstrument])), 0, params::kInstrumentCount - 1); }
  bool on(params::ParamId id) const { return v[static_cast<size_t>(id)] >= 0.5; }
  static Settings defaults() {
    Settings s;
    for (size_t i = 0; i < params::kParamCount; ++i) s.v[i] = params::kSpecs[i].def;
    return s;
  }
};

// Level trims that put every instrument on the same footing: one C4 at a firm touch with the
// default settings lands near -16 dBFS RMS over its first half second (see docs/QUALITY.md;
// held there by engine_tests). A clavichord is the quietest instrument there is and a grand
// among the loudest; the trims let a preset's Master be a musical choice.
namespace levels {
constexpr std::array<double, params::kInstrumentCount> kInstrumentDb{
      +8.2,   // concert grand
      +7.0,   // upright
     +12.8,   // Viennese fortepiano
    +13.9,   // square piano
    +14.6,   // Cristofori
    +18.3,   // tangent piano
    +19.8,   // Flemish harpsichord
    +23.2,   // Italian harpsichord
    +22.8,   // French harpsichord
    +32.0,   // virginal
    +21.8,   // spinet
    +31.3,   // Lautenwerck
    +22.1,   // fretted clavichord
    +22.7,   // unfretted clavichord
      -3.0,   // hurdy-gurdy
    +11.2,   // Geigenwerk
      +9.9,   // celesta
      +9.9,   // glockenspiel
      +9.0,   // dulcitone
      -3.0,   // electric piano
      -3.0,   // toy piano
     -9.5,   // chamber organ
      +9.3,   // violin
     +14.1,   // viola
     +20.8,   // cello
};
inline double gain(int instrument) { return dsp::db_to_gain(kInstrumentDb[static_cast<size_t>(std::clamp(instrument, 0, params::kInstrumentCount - 1))]); }
}  // namespace levels

// The output limiter: nothing leaves above the ceiling, so a fortissimo chord can be set as loud
// as a clavichord's whisper without clipping. Instant attack on one detector for both channels,
// recovery over about 50 ms: a peak catcher, not a compressor.
class Limiter {
 public:
  static constexpr float kCeiling = 0.794f;  // -2 dBFS
  static constexpr double kClipDb = 9.0;     // reduction beyond which the squash is plainly audible
  void prepare(double rate) { release_ = std::exp(-1.0 / (0.050 * rate)); reset(); }
  void reset() { gain_ = 1.0; held_db_ = 0.0; }
  float gain(float l, float r) {
    const double peak = std::max(std::abs(l), std::abs(r));
    const double wanted = peak > kCeiling ? kCeiling / peak : 1.0;
    if (wanted < gain_) gain_ = wanted;
    else gain_ += (wanted - gain_) * (1.0 - release_);
    if (gain_ < 1.0) held_db_ = std::max(held_db_, -20.0 * std::log10(gain_));
    return static_cast<float>(gain_);
  }
  double held_db() const { return held_db_; }
  void reset_meter() { held_db_ = 0.0; }
 private:
  double release_ = 0.9998, gain_ = 1.0, held_db_ = 0.0;
};

class Engine {
 public:
  void prepare(double rate) {
    rate_ = rate;
    keyboard_.prepare(rate);
    bars_.prepare(rate);
    organ_.prepare(rate);
    room_.prepare(rate);
    limiter_.prepare(rate);
    master_.set_time(10.0, rate);
    master_.snap(dsp::db_to_gain(params::kSpecs[params::kMasterDb].def));
    for (auto& f : lid_) f.set(20000.0, rate);
    reset();
  }
  void reset() {
    keyboard_.reset(); bars_.reset(); organ_.reset();
    room_.reset();
    limiter_.reset();
    for (auto& f : lid_) f.reset();
    bend_.fill(0.0); wheel_.fill(0.0); pressure_.fill(0.0);
    sustain_cc_.fill(false); sostenuto_cc_.fill(false); soft_cc_.fill(false);
    held_keys_.fill(false);
    peak_l_ = peak_r_ = 0.0f;
    clipped_ = false;
    instrument_ = -1;
  }

  // --- events ---
  void note_on(const Settings& s, int16_t channel, int key, int32_t note_id, double velocity) {
    (void)channel; (void)note_id;
    if (key < 0 || key > 127) return;
    switch_instrument(s);
    configure(s);
    held_keys_[static_cast<size_t>(key)] = true;
    // Touch: at full Dynamics the key's velocity reaches the hammer as it is; at none every note
    // is played mezzo-forte, as a harpsichord's plectrum takes it anyway.
    const double touch = dsp::lerp(0.6, std::clamp(velocity, 0.0, 1.0), std::clamp(s[params::kDynamics], 0.0, 1.0));
    const uint32_t seed = static_cast<uint32_t>(++strike_counter_ * 2654435761u);
    if (params::is_string_instrument(instrument_)) keyboard_.note_on(key, touch, seed);
    else if (params::is_bar_instrument(instrument_)) bars_.note_on(key, touch, seed);
    else organ_.note_on(key, touch, seed);
  }
  void note_off(const Settings& s, int16_t channel, int key, int32_t note_id) {
    (void)channel; (void)note_id;
    if (key < 0) { all_notes_off(); return; }
    if (key > 127) return;
    switch_instrument(s);
    held_keys_[static_cast<size_t>(key)] = false;
    if (params::is_string_instrument(instrument_)) keyboard_.note_off(key);
    else if (params::is_bar_instrument(instrument_)) bars_.note_off(key);
    else organ_.note_off(key);
  }
  void all_notes_off() { keyboard_.all_notes_off(); bars_.all_notes_off(); organ_.all_notes_off(); held_keys_.fill(false); }
  void all_sound_off() { keyboard_.all_sound_off(); bars_.all_sound_off(); organ_.all_sound_off(); held_keys_.fill(false); }
  // The pedals from the controller (CC 64, 66, 67), merged with the parameters in render.
  void sustain(int16_t channel, bool down) { sustain_cc_[ch(channel)] = down; }
  void sostenuto(int16_t channel, bool down) { sostenuto_cc_[ch(channel)] = down; }
  void soft(int16_t channel, bool down) { soft_cc_[ch(channel)] = down; }
  void pitch_bend(int16_t channel, double semitones_scaled) { bend_[ch(channel)] = std::clamp(semitones_scaled, -1.0, 1.0); }
  void mod_wheel(int16_t channel, double amount) { wheel_[ch(channel)] = std::clamp(amount, 0.0, 1.0); }
  void aftertouch(int16_t channel, double amount) { pressure_[ch(channel)] = std::clamp(amount, 0.0, 1.0); }
  void set_tempo(double bpm) { if (std::isfinite(bpm) && bpm > 0.0) tempo_ = bpm; }
  double tempo() const { return tempo_; }

  int active_voices() const { return keyboard_.sounding() + bars_.sounding() + organ_.sounding(); }
  bool silent() const { return !keyboard_.awake() && !bars_.awake() && !organ_.awake(); }
  const dsp::keyboard::Keyboard& keyboard() const { return keyboard_; }
  const dsp::bars::Bars& bars() const { return bars_; }
  const dsp::pipes::Organ& organ() const { return organ_; }
  float peak_left() const { return peak_l_; }
  float peak_right() const { return peak_r_; }
  bool clipped() const { return clipped_; }
  double limited_db() const { return limiter_.held_db(); }
  void reset_meters() { peak_l_ = peak_r_ = 0.0f; clipped_ = false; limiter_.reset_meter(); }
  int sounding_keys(bool* out) const {
    int n = 0;
    for (int k = 0; k < 128; ++k) {
      bool on = false;
      if (params::is_string_instrument(instrument_)) on = k >= dsp::keyboard::Keyboard::kFirstKey && k <= dsp::keyboard::Keyboard::kLastKey && keyboard_.note_sounding(k);
      else if (params::is_bar_instrument(instrument_)) on = k >= dsp::bars::Bars::kFirstKey && k <= dsp::bars::Bars::kLastKey && bars_.note(k).awake;
      else on = held_keys_[static_cast<size_t>(k)];
      if (out) out[k] = on;
      n += on;
    }
    return n;
  }

  // The instrument takes the block's settings: the pedals from the parameters and the
  // controller together, the player's pressure and bend.
  void configure(const Settings& s) {
    const bool sustain_down = s[params::kSustain] >= 0.5 || any(sustain_cc_);
    const bool sostenuto_down = s.on(params::kSostenuto) || any(sostenuto_cc_);
    const double soft = std::max(s[params::kUnaCorda], any(soft_cc_) ? 1.0 : 0.0);
    const double pressure = std::max(max_of(pressure_), max_of(wheel_));
    const double bend_semis = max_abs(bend_) * 2.0;
    if (params::is_string_instrument(instrument_)) {
      keyboard_.configure(keyboard_setup(s, sustain_down, sostenuto_down, soft));
      keyboard_.aftertouch(pressure);
      keyboard_.set_bend(std::exp2(bend_semis / 12.0));
    } else if (params::is_bar_instrument(instrument_)) {
      bars_.configure(bars_setup(s, sustain_down, sostenuto_down));
    } else {
      organ_.configure(organ_setup(s));
    }
  }

  // --- rendering ---
  void render(const Settings& s, float* left, float* right, uint32_t frames) {
    switch_instrument(s);
    configure(s);
    master_.target(dsp::db_to_gain(s[params::kMasterDb]));
    const bool strings = params::is_string_instrument(instrument_), bars = params::is_bar_instrument(instrument_);
    const bool violin = params::is_violin_family(instrument_);
    room_.configure(room_settings(s));
    const int lid = std::clamp(static_cast<int>(std::lround(s[params::kLid])), 0, 2);
    // The lid: closed, the top end and a few decibels go; half, less so; open, nothing.
    const double lid_hz = lid == 0 ? 600.0 : lid == 1 ? 3000.0 : 20000.0;
    const float lid_gain = static_cast<float>(lid == 0 ? dsp::db_to_gain(-5.0) : lid == 1 ? dsp::db_to_gain(-2.0) : 1.0);
    for (auto& f : lid_) f.set(lid_hz, rate_);
    const float trim = static_cast<float>(levels::gain(instrument_));
    const double width = std::clamp(s[params::kStereoWidth], 0.0, 1.0);
    const double perspective = std::clamp(s[params::kPerspective], 0.0, 1.0);
    for (uint32_t i = 0; i < frames; ++i) {
      float l = 0.0f, r = 0.0f;
      if (strings) {
        keyboard_.render(l, r);
        // A violin has no lid.
        if (!violin) { l = lid_[0].low(l) * lid_gain; r = lid_[1].low(r) * lid_gain; }
      } else if (bars) bars_.render(l, r);
      else organ_.render(l, r);
      l *= trim; r *= trim;
      // The stereo picture: the instrument's width from the player's bench, or heard from the
      // audience's side, where the bass is on the right.
      const float mid = 0.5f * (l + r), side = 0.5f * (l - r) * static_cast<float>(width);
      l = mid + side; r = mid - side;
      if (perspective > 0.0) { const float a = l; l = static_cast<float>(l * (1.0 - perspective) + r * perspective); r = static_cast<float>(r * (1.0 - perspective) + a * perspective); }
      room_.process(l, r);
      const float gain = static_cast<float>(master_.next());
      l *= gain; r *= gain;
      if (!std::isfinite(l)) l = 0.0f;
      if (!std::isfinite(r)) r = 0.0f;
      const float limit = limiter_.gain(l, r);
      l *= limit; r *= limit;
      if (limiter_.held_db() > Limiter::kClipDb) clipped_ = true;
      peak_l_ = std::max(peak_l_, std::abs(l)); peak_r_ = std::max(peak_r_, std::abs(r));
      left[i] = l; right[i] = r;
    }
  }

  // How long after the last note the output can still be sounding, in samples.
  uint32_t tail_samples(const Settings& s) const {
    double seconds = 0.5 + s[params::kRelease] * 0.001;
    const int instrument = s.instrument();
    if (params::is_string_instrument(instrument)) seconds += s[params::kSustain] >= 0.5 ? keyboard_.longest_decay_s() : 2.0;
    // The hurdy-gurdy's player keeps cranking 0.6 s after the last key; the drones then stop.
    if (instrument == params::kHurdyGurdy) seconds += 1.0;
    else if (params::is_bar_instrument(instrument)) seconds += bars_.longest_decay_s();
    else seconds += 0.3;
    if (s[params::kRoomSize] >= 1.0) {
      dsp::air::Room probe;
      probe.prepare(1000.0);
      probe.configure(room_settings(s));
      seconds += probe.rt60_low() + 0.6;
    }
    seconds += s[params::kMicDistance] / 300.0;
    return static_cast<uint32_t>(std::min(seconds * rate_, 2.0e9));
  }

  static dsp::keyboard::Setup keyboard_setup(const Settings& s, bool sustain, bool sostenuto, double soft) {
    dsp::keyboard::Setup k;
    k.model = std::clamp(params::keyboard_model(s.instrument()), 0, dsp::keyboard::kModelCount - 1);
    k.a4 = s[params::kPitchA4];
    k.temperament = static_cast<int>(std::lround(s[params::kTemperament]));
    k.root = static_cast<int>(std::lround(s[params::kTemperamentRoot]));
    k.stretch = s[params::kStretch];
    k.unison_cents = s[params::kUnisonSpread];
    k.hardness = s[params::kHammerHardness];
    k.strike_scale = s[params::kStrikePoint];
    k.plectrum = s[params::kPlectrum];
    k.mechanics = s[params::kMechanics];
    k.release_s = s[params::kRelease] * 0.001;
    k.bebung_cents = s[params::kBebung];
    k.wheel_turns = s[params::kWheelSpeed];
    k.wheel_pressure = s[params::kWheelPressure];
    k.trompette = s[params::kTrompette];
    k.drones = s.on(params::kDrones);
    k.bow_force = s[params::kBowForce];
    k.bow_speed = s[params::kBowSpeed];
    k.bow_position = s[params::kBowPosition];
    k.vibrato_cents = s[params::kVibrato];
    k.vibrato_hz = s[params::kVibratoRate];
    k.back8 = s.on(params::kBack8); k.front8 = s.on(params::kFront8); k.four = s.on(params::kFour); k.lute = s.on(params::kLute); k.buff = s.on(params::kBuff);
    k.sustain = sustain ? std::max(0.5, s[params::kSustain]) : 0.0;
    k.sostenuto = sostenuto;
    k.una_corda = soft;
    k.moderator = s[params::kModerator];
    k.sympathetic = s[params::kSympathetic];
    k.humidity = s[params::kHumidity];
    k.celsius = s[params::kTemperature];
    return k;
  }
  static dsp::bars::Setup bars_setup(const Settings& s, bool sustain, bool sostenuto) {
    dsp::bars::Setup b;
    b.model = std::clamp(s.instrument() - params::kStringInstrumentCount, 0, dsp::bars::kModelCount - 1);
    b.hardness = s[params::kHammerHardness];
    b.release_s = s[params::kRelease] * 0.001;
    b.sustain = sustain ? 1.0 : 0.0;
    b.sostenuto = sostenuto;
    b.mechanics = s[params::kMechanics];
    b.a4 = s[params::kPitchA4];
    b.temperament = static_cast<int>(std::lround(s[params::kTemperament]));
    b.root = static_cast<int>(std::lround(s[params::kTemperamentRoot]));
    return b;
  }
  static dsp::pipes::Setup organ_setup(const Settings& s) {
    dsp::pipes::Setup o;
    o.drawn = {s.on(params::kDiapason8), s.on(params::kGedackt8), s.on(params::kPrincipal4), s.on(params::kFifteenth2), s.on(params::kQuint)};
    o.wind = s[params::kWind];
    o.tremulant = s[params::kTremulant];
    o.tremulant_hz = s[params::kTremulantRate];
    o.chiff = s[params::kChiff];
    o.a4 = s[params::kPitchA4];
    o.temperament = static_cast<int>(std::lround(s[params::kTemperament]));
    o.root = static_cast<int>(std::lround(s[params::kTemperamentRoot]));
    return o;
  }
  static dsp::air::Room::Settings room_settings(const Settings& s) {
    dsp::air::Room::Settings r;
    r.volume_m3 = params::kRoomVolumes[static_cast<size_t>(std::clamp(static_cast<int>(std::lround(s[params::kRoomSize])), 0, params::kRoomSizeCount - 1))];
    r.absorption = s[params::kRoomAbsorption];
    r.distance_m = s[params::kMicDistance];
    r.humidity = s[params::kHumidity];
    r.celsius = s[params::kTemperature];
    r.pattern = static_cast<int>(std::lround(s[params::kMicPattern]));
    r.mono = s[params::kMicSetup] >= 0.5;
    return r;
  }

 private:
  static size_t ch(int16_t channel) { return static_cast<size_t>(std::clamp<int>(channel, 0, 15)); }
  static bool any(const std::array<bool, 16>& a) { for (bool b : a) if (b) return true; return false; }
  static double max_of(const std::array<double, 16>& a) { double m = 0.0; for (double v : a) m = std::max(m, v); return m; }
  static double max_abs(const std::array<double, 16>& a) { double m = 0.0; for (double v : a) if (std::abs(v) > std::abs(m)) m = v; return m; }

  // A new instrument silences the old one: the player has moved to another bench.
  void switch_instrument(const Settings& s) {
    const int wanted = s.instrument();
    if (wanted == instrument_) return;
    if (instrument_ >= 0) { keyboard_.all_sound_off(); bars_.all_sound_off(); organ_.all_sound_off(); }
    instrument_ = wanted;
  }

  double rate_ = 48000.0, tempo_ = 120.0;
  int instrument_ = -1;
  dsp::keyboard::Keyboard keyboard_;
  dsp::bars::Bars bars_;
  dsp::pipes::Organ organ_;
  dsp::air::Room room_;
  Limiter limiter_;
  dsp::Smoother master_;
  std::array<dsp::OnePole, 2> lid_{};
  std::array<double, 16> bend_{}, wheel_{}, pressure_{};
  std::array<bool, 16> sustain_cc_{}, sostenuto_cc_{}, soft_cc_{};
  std::array<bool, 128> held_keys_{};
  uint64_t strike_counter_ = 0;
  float peak_l_ = 0.0f, peak_r_ = 0.0f;
  bool clipped_ = false;
};

}  // namespace schrodr
