// Tries a preset with some parameters changed, without rebuilding the preset table: plays a record
// gate phrase, measures it as the gate does and prints the verdict against every group's stems.
// With out=FILE.wav it also writes the render (44.1 kHz), so the same audio can be measured by the
// song corpus's own scripts/features.py. A tuning tool, not a test.
//
//   schrodr-record-gate-try "Preset name" chords|melody [Param=value ...] [out=render.wav]
//                           (parameter names as the editor shows them)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../src/wav.hpp"
#include "record_gate.hpp"

int main(int argc, char** argv) {
  using namespace record_gate;
  namespace P = schrodr::params;
  if (argc < 3) { std::fprintf(stderr, "usage: schrodr-record-gate-try PRESET chords|melody [Param=value ...] [out=FILE.wav]\n"); return 2; }
  const long index = preset_index(argv[1]);
  if (index < 0) { std::fprintf(stderr, "no preset %s\n", argv[1]); return 2; }
  const Phrase phrase = std::strcmp(argv[2], "melody") == 0 ? Phrase::Melody : Phrase::Chords;
  auto s = schrodr::Settings::defaults();
  schrodr::presets::apply(static_cast<size_t>(index), s.v.data());
  std::string out;
  for (int i = 3; i < argc; ++i) {
    const std::string arg = argv[i];
    const size_t eq = arg.find('=');
    if (eq == std::string::npos) { std::fprintf(stderr, "bad argument %s\n", argv[i]); return 2; }
    if (arg.substr(0, eq) == "out") { out = arg.substr(eq + 1); continue; }
    bool found = false;
    for (uint32_t id = 0; id < P::kParamCount; ++id)
      if (arg.substr(0, eq) == P::kSpecs[id].name) { s.v[id] = P::clamp_to_spec(id, std::atof(arg.c_str() + eq + 1)); found = true; }
    if (!found) { std::fprintf(stderr, "unknown parameter in %s\n", argv[i]); return 2; }
  }
  const Stereo audio = play(s, phrase);
  if (!out.empty() && !schrodr::wav::write_file(out.c_str(), {audio.left, audio.right}, kRate)) { std::fprintf(stderr, "cannot write %s\n", out.c_str()); return 1; }
  const record::Features f = record::measure_stem(audio.left, audio.right, kRate);
  std::printf("%s: centroid %.0f Hz, roll-off %.0f Hz, above 3 kHz %.1f dB, crest %.2f dB, corr %.3f, wobble %.2f cents (voiced %.2f)\n", argv[1],
              f.centroid_hz, f.rolloff85_hz, f.hf_share_db, f.crest100_db, f.corr, f.wobble_cents, f.voiced_frac);
  bool any = false;
  for (const auto& r : record::kReferences) {
    const std::string failures = judge(f, r);
    any = any || failures.empty();
    std::printf("  %-9s %-6s %s\n", r.group, r.stem, failures.empty() ? "PASS" : failures.c_str());
  }
  return any ? 0 : 1;
}
