// Executable scenarios for features/engine.feature: notes, pedals, controllers, the instrument
// switch, every instrument at a comparable level, every preset against the quality bar, the
// limiter and the tail, through the engine's own interface (no CLAP yet).
#include "../src/engine.hpp"
#include "../src/presets.hpp"
#include "audio_measure.hpp"
#include "audit_phrase.hpp"
#include "spec.hpp"

#include <vector>

using namespace schrodr;
using measure::Span;

namespace {
constexpr double kRate = 48000.0;
constexpr uint32_t kBlock = 256;

struct Rig {
  Engine engine;
  Settings s = Settings::defaults();
  std::vector<float> left, right;
  Rig() { engine.prepare(kRate); }
  void render(uint32_t frames) {
    const size_t at = left.size();
    left.resize(at + frames); right.resize(at + frames);
    for (uint32_t done = 0; done < frames; done += kBlock) {
      const uint32_t n = std::min(kBlock, frames - done);
      engine.render(s, left.data() + at + done, right.data() + at + done, n);
    }
  }
  std::vector<float> mono() const { std::vector<float> m(left.size()); for (size_t i = 0; i < m.size(); ++i) m[i] = 0.5f * (left[i] + right[i]); return m; }
  void on(int key, double vel = 0.7) { engine.note_on(s, 0, key, -1, vel); }
  void off(int key) { engine.note_off(s, 0, key, -1); }
};
bool finite(const std::vector<float>& x) { for (float v : x) if (!std::isfinite(v)) return false; return true; }
}  // namespace

SCENARIO(a_note_sounds_while_held_and_is_damped_on_release) {
  GIVEN("the default grand, one note held for half a second with an 80 ms damper");
  Rig rig;
  rig.on(60);
  rig.render(24000);
  rig.off(60);
  rig.render(96000);
  THEN("the note sounds while held, is damped on release, and the engine reports silence within two seconds");
  const auto m = rig.mono();
  CHECK(finite(m), "finite");
  CHECK(measure::rms(Span(m, 2400, 19200)) > 0.01, "audible while held");
  CHECK(measure::peak(Span(m, 24000 + 72000)) < 1e-3, "quiet after the damper");
  CHECK(rig.engine.silent(), "silent");
}

SCENARIO(the_pedals_arrive_as_controllers_and_as_parameters) {
  GIVEN("a note released on the sustain controller, then the controller lifted");
  Rig rig;
  rig.s.v[params::kRelease] = 50.0;
  rig.engine.sustain(0, true);
  rig.on(60); rig.render(4800); rig.off(60); rig.render(24000);
  THEN("the note keeps sounding");
  CHECK(measure::rms(Span(rig.mono(), rig.left.size() - 4800)) > 1e-3, "held by the pedal");
  rig.engine.sustain(0, false); rig.render(72000);
  THEN("and releases with it");
  CHECK(measure::peak(Span(rig.mono(), rig.left.size() - 4800)) < 1e-3 && rig.engine.silent(), "released");
  GIVEN("the same through the Sustain pedal parameter");
  Rig param;
  param.s.v[params::kSustain] = 1.0;
  param.on(60); param.render(4800); param.off(60); param.render(24000);
  CHECK(measure::rms(Span(param.mono(), param.left.size() - 4800)) > 1e-3, "held by the parameter");
  GIVEN("the soft pedal from the controller");
  Rig soft, plain;
  soft.engine.soft(0, true);
  soft.on(60, 0.5); plain.on(60, 0.5);
  soft.render(24000); plain.render(24000);
  THEN("the note is quieter");
  CHECK(measure::db(measure::rms(Span(soft.mono(), 2400, 14400))) < measure::db(measure::rms(Span(plain.mono(), 2400, 14400))) - 2.0, "una corda from CC 67");
}

SCENARIO(dynamics_scales_the_touch) {
  GIVEN("a pianissimo and a fortissimo note with Dynamics at full and at none");
  const auto level = [](double dynamics, double velocity) {
    Rig rig; rig.s.v[params::kDynamics] = dynamics;
    rig.on(60, velocity); rig.render(24000);
    return measure::db(measure::rms(Span(rig.mono(), 2400, 14400)));
  };
  THEN("at full the two differ by more than 20 dB; at none they are the same mezzo-forte");
  CHECK(level(1.0, 1.0) - level(1.0, 0.1) > 20.0, "full range");
  CHECK(std::abs(level(0.0, 1.0) - level(0.0, 0.1)) < 1.0, "everything mezzo-forte");
}

SCENARIO(switching_instruments_silences_the_old_one) {
  GIVEN("a grand note ringing on the pedal, then the instrument switched to the harpsichord");
  Rig rig;
  rig.s.v[params::kSustain] = 1.0;
  rig.on(60); rig.render(4800); rig.off(60);
  rig.s.v[params::kInstrument] = params::kFlemish;
  rig.render(4800);
  THEN("the grand's strings are stopped and the harpsichord plays the next note");
  CHECK(measure::peak(Span(rig.mono(), rig.left.size() - 2400)) < 1e-3, "old instrument silenced");
  rig.on(60); rig.render(9600);
  CHECK(measure::rms(Span(rig.mono(), rig.left.size() - 4800)) > 1e-3, "new instrument sounds");
}

SCENARIO(every_instrument_plays_one_note_at_a_comparable_level) {
  GIVEN("one C4 at a firm touch through every instrument, master at 0 dB, a second of it");
  bool all_ok = true;
  for (int inst = 0; inst < params::kInstrumentCount; ++inst) {
    Rig rig;
    rig.s.v[params::kInstrument] = inst; rig.s.v[params::kMasterDb] = 0;
    rig.on(60, 0.8); rig.render(48000);
    const auto m = rig.mono();
    // The level the trims aim at: -16 dBFS RMS over the first half second, with the peak under
    // the ceiling; the slow-speaking organ and the bowed instruments are measured later in.
    const bool slow = params::is_organ(inst) || params::is_bowed(inst);
    const double held = measure::db(measure::rms(Span(m, slow ? 14400 : 2400, 24000)));
    const double peak = measure::db(measure::peak(Span(m)));
    const bool ok = finite(m) && held >= -22.0 && held <= -10.0 && peak <= -2.0 + 0.1;
    if (!ok) { all_ok = false; std::printf("    %s: held %.1f dBFS, peak %.1f dBFS\n", params::kInstrumentNames[inst], held, peak); }
  }
  THEN("every one holds between -22 and -10 dBFS RMS and peaks under the ceiling");
  CHECK(all_ok, "instruments sit at one level");
}

SCENARIO(every_preset_meets_the_quality_bar) {
  GIVEN("each factory preset played the audit phrase: a four-note chord at playing velocities, then three melody notes");
  bool all_ok = true;
  double loudest = -120.0, quietest = 0.0;
  for (size_t p = 0; p < presets::count(); ++p) {
    Engine engine;
    engine.prepare(kRate);
    Settings s = Settings::defaults();
    presets::apply(p, s.v.data());
    const audit::Rendered r = audit::render(s, engine, 2.0);
    const double peak = std::max(measure::peak(Span(r.left)), measure::peak(Span(r.right)));
    const double loud = measure::momentary_loudness_max(r.mono, kRate);
    const Span chord(r.mono, r.first_note + 9600, 57600);
    const double dc = std::abs(measure::mean(chord));
    const bool ok = finite(r.mono) && std::abs(loud - audit::kTargetLufs) <= audit::kLoudnessSlackDb && peak <= Limiter::kCeiling + 1e-4 && !engine.clipped() && measure::rms(chord) > 1e-3 && dc < 0.01;
    loudest = std::max(loudest, loud); quietest = std::min(quietest, loud);
    if (!ok) { all_ok = false; std::printf("    preset %zu '%s': loud %.1f LUFS limit %.1f dB peak %.1f dBFS dc %.1f dB rms %.4f finite %d clipped %d\n", p, presets::all()[p].name, loud, engine.limited_db(), measure::db(peak), measure::db(dc), measure::rms(chord), finite(r.mono), engine.clipped()); }
  }
  THEN("every one is finite, within 2 dB of the target loudness, under the ceiling with the clip light off, audible and free of DC");
  CHECK(all_ok, "all presets meet the bar");
  CHECK(presets::count() == params::kPresetCount, spec::fmt("preset table has %.0f entries, the parameter promises %.0f", static_cast<double>(presets::count()), params::kPresetCount));
}

SCENARIO(the_output_limiter_holds_the_ceiling_and_leaves_quiet_signals_alone) {
  GIVEN("a fortissimo chord with the master at +12 dB, then at -24 dB");
  const auto chord = [](double master) {
    Rig rig; rig.s.v[params::kMasterDb] = master;
    for (int key : {48, 55, 60, 64, 67}) rig.on(key, 1.0);
    rig.render(48000);
    return std::tuple{rig.engine.clipped(), measure::db(std::max(measure::peak(Span(rig.left)), measure::peak(Span(rig.right)))), rig.engine.limited_db()};
  };
  const auto [hot_clipped, hot_peak, hot_limited] = chord(12.0);
  const auto [quiet_clipped, quiet_peak, quiet_limited] = chord(-24.0);
  THEN("the hot one peaks at the -2 dBFS ceiling with the clip light on; the quiet one, peaking under the ceiling, is not reduced at all");
  CHECK_NEAR(hot_peak, -2.0, 0.1, "ceiling");
  CHECK(hot_clipped && hot_limited > 9.0, ::spec::fmt("clip light, %.1f dB of reduction", hot_limited));
  CHECK(!quiet_clipped && quiet_limited == 0.0 && quiet_peak < -2.5, ::spec::fmt("quiet signal passes: peak %.1f dBFS, %.2f dB of reduction", quiet_peak, quiet_limited));
}

SCENARIO(the_advertised_tail_covers_the_decay) {
  GIVEN("the grand in a hall with the pedal down, a chord released");
  Rig rig;
  presets::apply(0, rig.s.v.data());
  rig.s.v[params::kSustain] = 1.0;
  rig.on(48); rig.on(60); rig.render(9600); rig.off(48); rig.off(60);
  const uint32_t tail = rig.engine.tail_samples(rig.s);
  rig.render(tail);
  THEN("after the advertised tail the output is quiet");
  CHECK(tail > 48000, spec::fmt("tail of %.1f s advertised", tail / kRate));
  CHECK(measure::peak(Span(rig.mono(), rig.left.size() - 4800)) < 0.02, "quiet after the tail");
}

int main(int argc, char** argv) { return spec::run(argc, argv); }
