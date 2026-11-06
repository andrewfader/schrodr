// Renders single notes of an instrument straight from the engine (no room), for listening and
// for comparing against recordings. A review tool, not a test.
//
//   schrodr-note-render <out.wav> instrument=0..21 key=60 velocity=0.7 hold=1.5 tail=2.5
//                       [hardness=0.5 pedal=0 a4=440 temperament=0 stops=1,0,0,0,0 registers=1,0,0,0,0]
//
// Prints the instrument, the key's tuned pitch and the first strings' physics.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "../src/engine.hpp"
#include "../src/wav.hpp"

int main(int argc, char** argv) {
  if (argc < 2) { std::fprintf(stderr, "usage: schrodr-note-render <out.wav> key=value ...\n"); return 2; }
  std::map<std::string, std::string> a{{"instrument", "0"}, {"key", "60"}, {"velocity", "0.7"}, {"hold", "1.5"}, {"tail", "2.5"}, {"hardness", "0.5"},
                                       {"pedal", "0"}, {"a4", "440"}, {"temperament", "0"}, {"stops", "1,0,0,0,0"}, {"registers", "1,0,0,0,0"}, {"rate", "48000"}};
  for (int i = 2; i < argc; ++i) {
    const char* eq = std::strchr(argv[i], '=');
    if (!eq) { std::fprintf(stderr, "bad argument %s\n", argv[i]); return 2; }
    const std::string key(argv[i], static_cast<size_t>(eq - argv[i]));
    if (!a.count(key)) { std::fprintf(stderr, "unknown key %s\n", key.c_str()); return 2; }
    a[key] = eq + 1;
  }
  using namespace schrodr;
  const double rate = std::atof(a["rate"].c_str());
  Engine engine;
  engine.prepare(rate);
  Settings s = Settings::defaults();
  s.v[params::kInstrument] = std::atof(a["instrument"].c_str());
  s.v[params::kHammerHardness] = std::atof(a["hardness"].c_str());
  s.v[params::kSustain] = std::atof(a["pedal"].c_str());
  s.v[params::kPitchA4] = std::atof(a["a4"].c_str());
  s.v[params::kTemperament] = std::atof(a["temperament"].c_str());
  s.v[params::kMasterDb] = 0.0;
  const auto flags = [&](const std::string& list, params::ParamId first) {
    size_t at = 0;
    for (int i = 0; i < 5 && at < list.size(); ++i) {
      s.v[static_cast<size_t>(first) + static_cast<size_t>(i)] = std::atof(list.c_str() + at);
      const size_t comma = list.find(',', at);
      if (comma == std::string::npos) break;
      at = comma + 1;
    }
  };
  flags(a["stops"], params::kDiapason8);
  flags(a["registers"], params::kBack8);
  const int key = std::atoi(a["key"].c_str());
  const double hold = std::atof(a["hold"].c_str()), tail = std::atof(a["tail"].c_str());
  std::vector<float> left(static_cast<size_t>((hold + tail) * rate)), right(left.size());
  engine.note_on(s, 0, key, -1, std::atof(a["velocity"].c_str()));
  const size_t off = static_cast<size_t>(hold * rate);
  for (size_t at = 0; at < left.size(); at += 256) {
    const uint32_t n = static_cast<uint32_t>(std::min<size_t>(256, left.size() - at));
    if (at <= off && off < at + n) engine.note_off(s, 0, key, -1);
    engine.render(s, left.data() + at, right.data() + at, n);
  }
  if (!wav::write_file(argv[1], {left, right}, rate)) { std::fprintf(stderr, "cannot write %s\n", argv[1]); return 1; }
  const int inst = s.instrument();
  std::printf("%s key %d", params::kInstrumentNames[inst], key);
  if (params::is_string_instrument(inst)) {
    const auto& k = engine.keyboard();
    const auto& p = k.physics(key);
    std::printf(": %.3f Hz, %d string(s), L %.3f m, %.2f mm %s, T %.1f N, B %.2e", k.pitch_at(key), k.courses_at(key), p.length, p.core_diameter * 1e3, p.wound ? "wound" : "plain", p.tension, p.inharmonicity);
  } else if (params::is_bar_instrument(inst)) {
    std::printf(": %.3f Hz", engine.bars().pitch_at(key));
  } else {
    std::printf(": %.3f Hz (8')", engine.organ().pitch_at(key, 0));
  }
  std::printf("\n");
  return 0;
}
