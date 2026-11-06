// Executable scenarios for features/clap_plugin.feature (notes and automation): sound comes out
// when and only when notes say so, through both the CLAP and MIDI dialects, with sample-accurate
// event timing and automation.
#include "../src/params.hpp"
#include "audio_measure.hpp"
#include "clap_harness.hpp"
#include "spec.hpp"

#include <vector>

using namespace harness;
using measure::Span;
namespace P = schrodr::params;

namespace {
const char* g_path = nullptr;
constexpr double kRate = 48000.0;

// Renders `blocks` blocks collecting the mono output.
std::vector<float> collect(Runner& runner, int blocks, Events* events = nullptr) {
  std::vector<float> out;
  for (int b = 0; b < blocks; ++b) {
    runner.run(b == 0 ? events : nullptr);
    for (uint32_t i = 0; i < runner.frames(); ++i) out.push_back(0.5f * (runner.left()[i] + runner.right()[i]));
  }
  return out;
}
}  // namespace

SCENARIO(a_clap_note_on_makes_sound_and_note_off_stops_it) {
  GIVEN("an instance with a 50 ms damper, given a CLAP note on");
  Library lib(g_path);
  const clap_plugin_t* plugin = lib.create();
  set_param(plugin, P::kRelease, 50.0);
  Runner runner(plugin, kRate, 256);
  Events on; on.push(note_event(CLAP_EVENT_NOTE_ON, 0, 60, 7, 0.9));
  const auto held = collect(runner, 40, &on);
  THEN("audio appears");
  CHECK(measure::rms(Span(held, 2000)) > 0.01, "sound while held");
  WHEN("the note is released by id");
  Events off; off.push(note_event(CLAP_EVENT_NOTE_OFF, 0, 60, 7, 0.0));
  const auto tail = collect(runner, 200, &off);
  THEN("it is damped and the plug-in falls silent");
  CHECK(measure::peak(Span(tail, tail.size() - 6000)) < 1e-3, "silent after the damper");
  CHECK(runner.run().peak < 1e-4f, "stays silent");
  runner.destroy();
}

SCENARIO(midi_notes_controllers_and_bend_are_honoured) {
  GIVEN("MIDI note on on the Geigenwerk (a bowed, sustained string), then pitch bend up, then note off");
  Library lib(g_path);
  const clap_plugin_t* plugin = lib.create();
  set_param(plugin, P::kInstrument, P::kGeigenwerk);
  set_param(plugin, P::kRelease, 30.0);
  Runner runner(plugin, kRate, 256);
  Events on; on.push(midi_event(0x90, 60, 100));
  const auto plain = collect(runner, 100, &on);
  Events bend; bend.push(midi_event(0xe0, 0x7f, 0x7f));
  const auto bent = collect(runner, 100, &bend);
  THEN("the pitch is C4, then two semitones up with the wheel fully up");
  // Read as a tuner reads a bowed string: the strongest component near the note (its body's
  // modes and a sawtooth's second harmonic can outweigh the fundamental in the room).
  CHECK_NEAR(measure::cents(measure::peak_hz_near(Span(plain, 12000, 8000), kRate, 261.63, 120.0), 261.63), 0.0, 25.0, "C4");
  CHECK_NEAR(measure::cents(measure::peak_hz_near(Span(bent, 12000, 8000), kRate, 261.63 * std::exp2(200.0 / 1200.0), 120.0), 261.63), 200.0, 30.0, "bent up two semitones");
  WHEN("the note is released while the sustain pedal is down, then the pedal comes up (on the grand)");
  set_param(plugin, P::kInstrument, P::kGrand);
  Events grand; grand.push(midi_event(0x90, 60, 100));
  collect(runner, 10, &grand);
  Events pedal; pedal.push(midi_event(0xb0, 64, 127)); pedal.push(midi_event(0x80, 60, 0));
  const auto sustained = collect(runner, 40, &pedal);
  CHECK(measure::rms(Span(sustained, 6000)) > 1e-3, "sustain pedal holds the note");
  Events up; up.push(midi_event(0xb0, 64, 0));
  const auto released = collect(runner, 200, &up);
  THEN("the note ends only when the pedal is released");
  CHECK(measure::peak(Span(released, released.size() - 6000)) < 1e-3, "released with the pedal");
  WHEN("all notes off (CC 123) arrives during a chord");
  Events chord; chord.push(midi_event(0x90, 60, 100)); chord.push(midi_event(0x90, 64, 100)); chord.push(midi_event(0xb0, 123, 0));
  const auto silenced = collect(runner, 200, &chord);
  THEN("nothing is left sounding");
  CHECK(measure::peak(Span(silenced, silenced.size() - 6000)) < 1e-3, "all notes off");
  runner.destroy();
}

SCENARIO(events_are_applied_at_their_sample_offset) {
  GIVEN("a note on scheduled 200 samples into a 256-sample block, on the toy piano (whose rod speaks at once)");
  Library lib(g_path);
  const clap_plugin_t* plugin = lib.create();
  set_param(plugin, P::kInstrument, P::kToyPiano); set_param(plugin, P::kMechanics, 0.0);
  Runner runner(plugin, kRate, 256);
  Events events;
  auto on = note_event(CLAP_EVENT_NOTE_ON, 0, 72, -1, 1.0);
  on.header.time = 200;
  events.push(on);
  runner.run(&events);
  THEN("the first 200 samples are silent and sound starts right after");
  bool silent_before = true;
  for (uint32_t i = 0; i < 200; ++i) silent_before = silent_before && runner.left()[i] == 0.0f;
  bool sound_after = false;
  for (uint32_t i = 200; i < 256; ++i) sound_after = sound_after || std::abs(runner.left()[i]) > 1e-6f;
  CHECK(silent_before, "silent before the event");
  CHECK(sound_after, "sound after the event");
  runner.destroy();
}

SCENARIO(parameter_automation_changes_the_sound_and_flush_works_offline) {
  GIVEN("the master level set by a parameter event inside process, then by flush, on a sustained organ note");
  Library lib(g_path);
  const clap_plugin_t* plugin = lib.create();
  set_param(plugin, P::kInstrument, P::kOrgan);
  Runner runner(plugin, kRate, 256);
  Events on; on.push(note_event(CLAP_EVENT_NOTE_ON, 0, 60, -1, 1.0)); on.push(param_event(P::kMasterDb, 0.0));
  const auto loud = collect(runner, 120, &on);
  Events quieter; quieter.push(param_event(P::kMasterDb, -24.0));
  const auto quiet = collect(runner, 120, &quieter);
  THEN("the level drops by about 24 dB");
  const double drop = measure::db(measure::rms(Span(loud, 20000, 8000))) - measure::db(measure::rms(Span(quiet, 20000, 8000)));
  CHECK_NEAR(drop, 24.0, 2.0, "automation applied");
  CHECK(param(plugin, P::kMasterDb) == -24.0, "value readable through the params extension");
  WHEN("a parameter arrives through flush");
  set_param(plugin, P::kMasterDb, -6.0);
  THEN("the plug-in reports it");
  CHECK(param(plugin, P::kMasterDb) == -6.0, "flush applied");
  WHEN("an out-of-range value arrives");
  set_param(plugin, P::kMasterDb, 500.0);
  THEN("it is clamped to the declared range");
  CHECK(param(plugin, P::kMasterDb) == P::spec(P::kMasterDb).max, "clamped");
  runner.destroy();
}

SCENARIO(choosing_a_preset_rewrites_the_other_parameters) {
  GIVEN("the preset parameter set to 'Flemish 8''");
  Library lib(g_path);
  const clap_plugin_t* plugin = lib.create();
  double index = -1.0;
  CHECK(params_of(plugin)->text_to_value(plugin, P::kPreset, "Flemish 8'", &index), "preset found by name");
  set_param(plugin, P::kPreset, index);
  THEN("the instrument becomes the Flemish harpsichord and the preset's room is set");
  CHECK(param(plugin, P::kInstrument) == static_cast<double>(P::kFlemish), "instrument switched");
  CHECK(param(plugin, P::kRoomSize) > 0.0, "preset room applied");
  char text[64]{};
  params_of(plugin)->value_to_text(plugin, P::kPreset, index, text, sizeof(text));
  CHECK(std::string(text) == "Flemish 8'", "preset name formats back");
  WHEN("a parameter is edited afterwards and the same preset index is sent again");
  set_param(plugin, P::kMicDistance, 0.9);
  set_param(plugin, P::kPreset, index);
  THEN("the edit survives (re-sending the current preset is not a reload)");
  CHECK(param(plugin, P::kMicDistance) == 0.9, "edit kept");
  plugin->destroy(plugin);
}

SCENARIO(every_instrument_sounds_through_the_plugin) {
  GIVEN("each instrument selected by parameter, with one note");
  Library lib(g_path);
  bool all = true;
  for (int instrument = 0; instrument < P::kInstrumentCount; ++instrument) {
    const clap_plugin_t* plugin = lib.create();
    set_param(plugin, P::kInstrument, instrument);
    Runner runner(plugin, kRate, 256);
    Events on; on.push(note_event(CLAP_EVENT_NOTE_ON, 0, 60, -1, 0.8));
    const auto out = collect(runner, 100, &on);
    bool finite = true;
    for (float v : out) finite = finite && std::isfinite(v);
    const double rms = measure::rms(Span(out, 1000));
    if (!finite || rms < 1e-4) { all = false; std::printf("    instrument %d (%s): rms %.5f finite %d\n", instrument, P::kInstrumentNames[instrument], rms, finite); }
    runner.destroy();
  }
  THEN("all twenty-two are audible and finite");
  CHECK(all, "every instrument sounds");
}

SCENARIO(tail_reports_longer_in_a_church_with_the_pedal_down) {
  GIVEN("the tail extension on the instrument alone, then in a church with the sustain pedal down");
  Library lib(g_path);
  const clap_plugin_t* plugin = lib.create();
  const auto* tail = static_cast<const clap_plugin_tail_t*>(plugin->get_extension(plugin, CLAP_EXT_TAIL));
  Runner runner(plugin, kRate, 256);
  runner.run();
  const uint32_t dry = tail->get(plugin);
  set_param(plugin, P::kRoomSize, 6); set_param(plugin, P::kMicDistance, 5.0); set_param(plugin, P::kSustain, 1.0);
  runner.run();
  const uint32_t wet = tail->get(plugin);
  THEN("the wet tail is longer and both are sane");
  CHECK(dry > 0 && dry < 48000 * 10, "dry tail is the damper plus a margin");
  CHECK(wet > dry * 2, spec::fmt("wet tail %.1f s exceeds dry %.1f s", wet / kRate, dry / kRate));
  runner.destroy();
}

int main(int argc, char** argv) {
  if (argc < 2) { std::fprintf(stderr, "usage: note_tests <SCHRODR.clap> [filter]\n"); return 2; }
  g_path = argv[1];
  return spec::run(argc, argv, 2);
}
