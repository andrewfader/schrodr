// Measures the reference recordings (tests/reference/manifest.tsv, fetched by
// tools/fetch_references.py) with the same code that measures SCHRODR's notes, prints the two
// side by side, and writes the recordings' numbers as a C++ table for reference_tests. A review
// tool, not a test: the suite reads the table and never the recordings.
//
//   schrodr-reference-extract MANIFEST CACHE_DIR [OUT_HEADER] [ID_FILTER]
//
// It ends with the calibration behind the bar in tests/reference_compare.hpp: for each
// instrument, how far apart its recorded notes up to an octave apart (same marking) measure.
//
// A recording whose note cannot be found or measured is an error, never skipped.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../src/wav.hpp"
#include "../tests/reference_measure.hpp"
#include "../tests/reference_render.hpp"

namespace {

struct Row { std::string id, instrument, dynamic, path; int key; double rate = 0.0; };

std::vector<Row> read_manifest(const char* path) {
  std::vector<Row> rows;
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream cols(line);
    Row r; std::string key, sha;
    std::getline(cols, r.id, '\t'); std::getline(cols, r.instrument, '\t'); std::getline(cols, key, '\t');
    std::string rate;
    std::getline(cols, r.dynamic, '\t'); std::getline(cols, r.path, '\t'); std::getline(cols, sha, '\t'); std::getline(cols, rate, '\t');
    r.key = std::stoi(key);
    if (!rate.empty() && rate != "-") r.rate = std::stod(rate);
    rows.push_back(r);
  }
  return rows;
}

// Where tools/fetch_references.py put a manifest path SOURCE:PATH (a compressed file decoded
// to PATH.wav beside it).
std::string cached(const std::string& cache, const std::string& spec) {
  const size_t colon = spec.find(':');
  std::string path = cache + "/" + spec.substr(0, colon) + "/" + spec.substr(colon + 1);
  for (const char* coded : {".flac", ".ogg", ".mp3"}) {
    const size_t n = std::strlen(coded);
    if (path.size() > n && path.compare(path.size() - n, n, coded) == 0) path += ".wav";
  }
  return path;
}

void print(const char* who, const reference::Features& f) {
  std::printf("  %-6s f0 %8.2f  level %6.1f  centroid %5.2f  B %.2e  decay %5.1f/%5.1f  overtone %5.3f  attack %5.0f  vib %4.1fc %4.1fHz  noise %6.1f\n",
              who, f.f0, f.level_db, f.centroid, f.inharmonicity, f.decay_early, f.decay_late, f.overtone_ratio,
              f.attack_ms, f.vibrato_cents, f.vibrato_hz, f.noise_db);
}

double median(std::vector<double> v) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  return v[v.size() / 2];
}

struct Measured { Row row; reference::Features f; };

void print_calibration(const std::vector<Measured>& all) {
  std::printf("calibration: recorded notes up to an octave apart, same instrument and marking (medians)\n");
  for (const reference::Instrument& inst : reference::instruments()) {
    std::vector<double> colour, bright, noise;
    for (const Measured& a : all)
      for (const Measured& b : all) {
        if (a.row.instrument != inst.name || b.row.instrument != inst.name || a.row.dynamic != b.row.dynamic) continue;
        if (b.row.key - a.row.key <= 0 || b.row.key - a.row.key > 12) continue;
        const double d = reference::envelope_distance(a.f.bands, b.f.bands);
        if (d >= 0.0) colour.push_back(d);
        bright.push_back(std::exp(std::abs(std::log(a.f.centroid / b.f.centroid))));
        if (inst.kind == reference::Kind::Bowed) noise.push_back(std::abs(a.f.noise_db - b.f.noise_db));
      }
    std::printf("  %-13s %3zu pairs  colour %4.1f dB  brightness x%.2f  bow noise %4.1f dB\n", inst.name, bright.size(), median(colour), median(bright), median(noise));
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) { std::fprintf(stderr, "usage: schrodr-reference-extract MANIFEST CACHE_DIR [OUT_HEADER] [ID_FILTER]\n"); return 2; }
  const std::string cache = argv[2], out_path = argc >= 4 ? argv[3] : "", filter = argc >= 5 ? argv[4] : "";
  std::map<std::string, schrodr::wav::Audio> loaded;
  std::ostringstream table;
  int errors = 0;
  std::vector<Measured> measured;
  for (const Row& r : read_manifest(argv[1])) {
    if (!filter.empty() && r.id.find(filter) == std::string::npos) continue;
    const reference::Instrument* inst = reference::instrument(r.instrument);
    if (!inst) { std::fprintf(stderr, "%s: unknown instrument %s\n", r.id.c_str(), r.instrument.c_str()); ++errors; continue; }
    if (!loaded.count(r.path)) {
      std::string error;
      if (!schrodr::wav::read_file(cached(cache, r.path).c_str(), loaded[r.path], &error)) {
        std::fprintf(stderr, "%s: %s: %s\n", r.id.c_str(), r.path.c_str(), error.c_str()); ++errors; continue;
      }
      if (r.rate > 0.0) loaded[r.path].rate = r.rate;  // the manifest corrects a wrong header
    }
    const schrodr::wav::Audio& a = loaded[r.path];
    std::vector<float> note = a.mono;
    if (inst->kind != reference::Kind::Struck && reference::segments(a.mono, a.rate).size() > 1) {
      // A scale: the one note, from 0.1 s before its onset.
      reference::Segment s{};
      if (!reference::find_note(a.mono, a.rate, r.key, s, inst->kind != reference::Kind::Bar)) { std::fprintf(stderr, "%s: key %d not found in %s\n", r.id.c_str(), r.key, r.path.c_str()); ++errors; continue; }
      const size_t pre = std::min(s.begin, static_cast<size_t>(0.1 * a.rate));
      note.assign(a.mono.begin() + static_cast<long>(s.begin - pre), a.mono.begin() + static_cast<long>(s.end));
    }
    const reference::Features rec = reference::measure_note(note, a.rate, inst->kind, r.key, reference::floor_db(a.mono, a.rate));
    if (!rec.ok) { std::fprintf(stderr, "%s: could not be measured (under 25 dB over the floor?)\n", r.id.c_str()); ++errors; continue; }
    measured.push_back({r, rec});
    const reference::Features ours = reference::measure_render(*inst, r.key, reference::dynamic_from(r.dynamic));
    std::printf("%s (%.0f Hz, %.1f s)\n", r.id.c_str(), a.rate, static_cast<double>(note.size()) / a.rate);
    print("rec", rec);
    print("ours", ours);
    std::printf("  envelope distance %.1f dB\n", reference::envelope_distance(rec.bands, ours.bands));
    char line[1024];
    std::snprintf(line, sizeof(line), "    {\"%s\", \"%s\", %d, %d, {%.3f, %.2f, %.3f, {", r.id.c_str(), r.instrument.c_str(), r.key,
                  reference::dynamic_from(r.dynamic), rec.f0, rec.level_db, rec.centroid);
    table << line;
    for (size_t b = 0; b < rec.bands.size(); ++b) table << (b ? ", " : "") << (rec.bands[b] <= -100.0 ? -100.0 : std::round(rec.bands[b] * 10.0) / 10.0);
    std::snprintf(line, sizeof(line), "}, %.4e, %.2f, %.2f, %.4f, %.1f, %.1f, %.2f, %.1f, true}},\n", rec.inharmonicity, rec.decay_early,
                  rec.decay_late, rec.overtone_ratio, rec.attack_ms, rec.vibrato_cents, rec.vibrato_hz, rec.noise_db);
    table << line;
  }
  print_calibration(measured);
  if (errors) { std::fprintf(stderr, "%d error(s); nothing written\n", errors); return 1; }
  if (!out_path.empty()) {
    std::ofstream out(out_path);
    out << "// Generated by schrodr-reference-extract from the recordings in tests/reference/manifest.tsv\n"
           "// (sources in tools/fetch_references.py). Do not edit: re-run the tool.\n"
           "#pragma once\n\n#include <vector>\n\n#include \"reference_measure.hpp\"\n\nnamespace reference {\n\n"
           "struct Recorded { const char* id; const char* instrument; int key; int dynamic; Features features; };\n\n"
           "inline const std::vector<Recorded>& recorded() {\n  static const std::vector<Recorded> all{\n"
        << table.str() << "  };\n  return all;\n}\n\n}  // namespace reference\n";
    std::printf("wrote %s\n", out_path.c_str());
  }
  return 0;
}
