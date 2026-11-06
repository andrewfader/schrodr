// The parameter table: every automatable control of SCHRODR, with its range, default, unit and
// value names. Shared by the plug-in, the editor and the tests, so the editor's page layout and
// the per-parameter proofs are checked against the one real list.
//
// Parameter IDs are the host's automation handles and index the saved state, so a parameter is
// only ever appended just before kParamCount, never reordered or removed.
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

namespace schrodr::params {

enum ParamId : uint32_t {
  // --- the instrument and its tuning -------------------------------------------------------------
  kInstrument, kMasterDb, kTranspose, kPitchA4, kTemperament, kTemperamentRoot, kStretch,
  kUnisonSpread, kDynamics,
  // --- the action: hammers, plectra, tangents, the wheel -------------------------------------------
  kHammerHardness, kStrikePoint, kPlectrum, kMechanics, kRelease, kBebung,
  kWheelSpeed, kWheelPressure, kTrompette, kDrones,
  // --- harpsichord registers ----------------------------------------------------------------------
  kBack8, kFront8, kFour, kLute, kBuff,
  // --- organ stops and wind ------------------------------------------------------------------------
  kDiapason8, kGedackt8, kPrincipal4, kFifteenth2, kQuint, kWind, kTremulant, kTremulantRate, kChiff,
  // --- pedals, dampers, the lid ---------------------------------------------------------------------
  kSustain, kSostenuto, kUnaCorda, kModerator, kSympathetic, kLid,
  // --- the stereo picture ----------------------------------------------------------------------------
  kStereoWidth, kPerspective,
  // --- the room and the air --------------------------------------------------------------------------
  kRoomSize, kRoomAbsorption, kMicDistance, kHumidity, kTemperature, kMicPattern, kMicSetup,
  // --- presets ----------------------------------------------------------------------------------------
  kPreset,
  kParamCount
};

// How a value is shown to the musician. Stepped parameters with names draw from an option table.
enum class Unit : uint8_t {
  Plain, Ms, Seconds, Percent, Db, Hz, Semitones, Cents, Count, Options, Metres, Celsius, TurnsPerSecond
};

struct ParamSpec {
  const char* name;
  const char* module;
  double min, max, def;
  bool stepped;
  Unit unit;
  const char* const* options;  // Unit::Options: one name per integer value from min to max
};

// --- option tables -------------------------------------------------------------------------------

// The instruments, in the order of dsp::keyboard::Model (the string keyboards), then the bars
// (dsp::bars::Model) and the organ. kStringInstrumentCount string keyboards come first.
constexpr int kInstrumentCount = 22;
constexpr int kStringInstrumentCount = 16;
constexpr int kBarInstrumentCount = 5;
constexpr const char* kInstrumentNames[kInstrumentCount] = {
    "Concert grand", "Upright piano", "Viennese fortepiano", "Square piano", "Cristofori 1720",
    "Tangent piano", "Flemish harpsichord", "Italian harpsichord", "French harpsichord", "Virginal",
    "Spinet", "Lautenwerck", "Fretted clavichord", "Unfretted clavichord", "Hurdy-gurdy", "Geigenwerk",
    "Celesta", "Glockenspiel", "Dulcitone", "Electric piano", "Toy piano",
    "Chamber organ"};
enum Instrument : int {
  kGrand, kUpright, kFortepiano, kSquare, kCristofori, kTangent, kFlemish, kItalian, kFrench, kVirginal,
  kSpinet, kLautenwerck, kFrettedClavichord, kUnfrettedClavichord, kHurdyGurdy, kGeigenwerk,
  kCelesta, kGlockenspiel, kDulcitone, kElectricPiano, kToyPiano,
  kOrgan
};
constexpr bool is_string_instrument(int i) { return i >= 0 && i < kStringInstrumentCount; }
constexpr bool is_bar_instrument(int i) { return i >= kStringInstrumentCount && i < kStringInstrumentCount + kBarInstrumentCount; }
constexpr bool is_organ(int i) { return i == kOrgan; }
constexpr bool is_harpsichord(int i) { return i >= kFlemish && i <= kLautenwerck; }
constexpr bool is_clavichord(int i) { return i == kFrettedClavichord || i == kUnfrettedClavichord; }
constexpr bool is_bowed(int i) { return i == kHurdyGurdy || i == kGeigenwerk; }
constexpr bool is_hammered(int i) { return i >= kGrand && i <= kTangent; }

constexpr int kTemperamentCount = 6;
constexpr const char* kTemperamentNames[kTemperamentCount] = {
    "Equal", "Pythagorean", "Meantone 1/4", "Werckmeister III", "Kirnberger III", "Vallotti"};
constexpr const char* kKeyNames[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};
constexpr const char* kLidNames[] = {"Closed", "Half", "Open"};
// The room (src/dsp/air.hpp): its volume, in cubic metres, for each named size.
constexpr int kRoomSizeCount = 7;
constexpr const char* kRoomSizeNames[kRoomSizeCount] = {"Off", "Booth", "Bedroom", "Studio", "Club", "Hall", "Church"};
constexpr std::array<double, kRoomSizeCount> kRoomVolumes{0.0, 10.0, 35.0, 150.0, 600.0, 3000.0, 12000.0};
constexpr const char* kMicPatternNames[] = {"Omni", "Cardioid", "Figure-8"};
constexpr const char* kMicSetupNames[] = {"Stereo pair", "Mono"};
constexpr int kPresetCount = 48;  // The names live in presets.hpp; the table only needs the size.

constexpr std::array<ParamSpec, kParamCount> kSpecs{{
    // the instrument and its tuning
    {"Instrument", "Instrument", 0, kInstrumentCount - 1, kGrand, true, Unit::Options, kInstrumentNames},
    {"Master", "Instrument", -36, 12, -6, false, Unit::Db, nullptr},
    {"Transpose", "Instrument", -24, 24, 0, true, Unit::Semitones, nullptr},
    {"Pitch A4", "Instrument/Tuning", 392, 466, 440, false, Unit::Hz, nullptr},
    {"Temperament", "Instrument/Tuning", 0, kTemperamentCount - 1, 0, true, Unit::Options, kTemperamentNames},
    {"Temperament key", "Instrument/Tuning", 0, 11, 0, true, Unit::Options, kKeyNames},
    {"Stretch", "Instrument/Tuning", 0, 1, 1, false, Unit::Percent, nullptr},
    {"Unison spread", "Instrument/Tuning", 0, 10, 1, false, Unit::Cents, nullptr},
    {"Dynamics", "Instrument", 0, 1, 1, false, Unit::Percent, nullptr},
    // the action
    {"Hammer hardness", "Action", 0, 1, 0.5, false, Unit::Percent, nullptr},
    {"Strike point", "Action", 0.5, 2, 1, false, Unit::Percent, nullptr},
    {"Plectrum", "Action", 0, 1, 0.4, false, Unit::Percent, nullptr},
    {"Mechanics", "Action", 0, 1, 0.5, false, Unit::Percent, nullptr},
    {"Release", "Action", 10, 3000, 80, false, Unit::Ms, nullptr},
    {"Bebung", "Action", 0, 50, 12, false, Unit::Cents, nullptr},
    {"Wheel speed", "Action/Wheel", 0.3, 3, 1, false, Unit::TurnsPerSecond, nullptr},
    {"Wheel pressure", "Action/Wheel", 0, 1, 0.5, false, Unit::Percent, nullptr},
    {"Trompette", "Action/Wheel", 0, 1, 0.5, false, Unit::Percent, nullptr},
    {"Drones", "Action/Wheel", 0, 1, 1, true, Unit::Plain, nullptr},
    // harpsichord registers
    {"Back 8'", "Registers", 0, 1, 1, true, Unit::Plain, nullptr},
    {"Front 8'", "Registers", 0, 1, 0, true, Unit::Plain, nullptr},
    {"4'", "Registers", 0, 1, 0, true, Unit::Plain, nullptr},
    {"Lute", "Registers", 0, 1, 0, true, Unit::Plain, nullptr},
    {"Buff", "Registers", 0, 1, 0, true, Unit::Plain, nullptr},
    // organ stops and wind
    {"Open diapason 8'", "Stops", 0, 1, 1, true, Unit::Plain, nullptr},
    {"Gedackt 8'", "Stops", 0, 1, 0, true, Unit::Plain, nullptr},
    {"Principal 4'", "Stops", 0, 1, 0, true, Unit::Plain, nullptr},
    {"Fifteenth 2'", "Stops", 0, 1, 0, true, Unit::Plain, nullptr},
    {"Quint 2 2/3'", "Stops", 0, 1, 0, true, Unit::Plain, nullptr},
    {"Wind", "Stops/Wind", 0.5, 1.5, 1, false, Unit::Percent, nullptr},
    {"Tremulant", "Stops/Wind", 0, 1, 0, false, Unit::Percent, nullptr},
    {"Tremulant rate", "Stops/Wind", 3, 8, 5, false, Unit::Hz, nullptr},
    {"Chiff", "Stops/Wind", 0, 1, 0.5, false, Unit::Percent, nullptr},
    // pedals
    {"Sustain pedal", "Pedals", 0, 1, 0, false, Unit::Percent, nullptr},
    {"Sostenuto", "Pedals", 0, 1, 0, true, Unit::Plain, nullptr},
    {"Una corda", "Pedals", 0, 1, 0, false, Unit::Percent, nullptr},
    {"Moderator", "Pedals", 0, 1, 0, false, Unit::Percent, nullptr},
    {"Sympathetic", "Pedals", 0, 1, 1, false, Unit::Percent, nullptr},
    {"Lid", "Pedals", 0, 2, 2, true, Unit::Options, kLidNames},
    // the stereo picture
    {"Stereo width", "Stereo", 0, 1, 0.7, false, Unit::Percent, nullptr},
    {"Perspective", "Stereo", 0, 1, 0, false, Unit::Percent, nullptr},
    // the room and the air
    {"Room", "Room", 0, kRoomSizeCount - 1, 0, true, Unit::Options, kRoomSizeNames},
    {"Absorption", "Room", 0.02, 0.9, 0.25, false, Unit::Percent, nullptr},
    {"Mic distance", "Room", 0, 12, 0, false, Unit::Metres, nullptr},
    {"Humidity", "Room/Air", 0.05, 0.95, 0.45, false, Unit::Percent, nullptr},
    {"Temperature", "Room/Air", -10, 40, 20, false, Unit::Celsius, nullptr},
    {"Mic pattern", "Room", 0, 2, 0, true, Unit::Options, kMicPatternNames},
    {"Mic setup", "Room", 0, 1, 0, true, Unit::Options, kMicSetupNames},
    // presets
    {"Preset", "Presets", 0, kPresetCount - 1, 0, true, Unit::Plain, nullptr},
}};

// The preset parameter's value names come from the preset table, which lives above this header
// in the dependency order; the plug-in installs them here once.
inline const char* const* g_preset_names = nullptr;

inline const ParamSpec& spec(uint32_t id) { return kSpecs[static_cast<size_t>(id)]; }

inline double clamp_to_spec(uint32_t id, double value) {
  const ParamSpec& s = spec(id);
  if (!std::isfinite(value)) return s.def;
  value = std::clamp(value, s.min, s.max);
  return s.stepped ? std::round(value) : value;
}

// The option name for a stepped parameter's value, or null if it has none.
inline const char* option_name(uint32_t id, double value) {
  const ParamSpec& s = spec(id);
  const int index = static_cast<int>(std::lround(value - s.min));
  const int count = static_cast<int>(std::lround(s.max - s.min)) + 1;
  if (index < 0 || index >= count) return nullptr;
  if (id == kPreset) return g_preset_names ? g_preset_names[index] : nullptr;
  if (s.unit != Unit::Options || !s.options) return nullptr;
  return s.options[index];
}

inline bool format_value(uint32_t id, double value, char* out, uint32_t capacity) {
  if (id >= kParamCount || !out || capacity == 0) return false;
  const ParamSpec& s = spec(id);
  if (const char* name = option_name(id, value)) { std::snprintf(out, capacity, "%s", name); return true; }
  switch (s.unit) {
    case Unit::Ms: std::snprintf(out, capacity, value < 100.0 ? "%.1f ms" : "%.0f ms", value); break;
    case Unit::Seconds: std::snprintf(out, capacity, "%.2f s", value); break;
    case Unit::Percent: std::snprintf(out, capacity, "%.0f%%", value * 100.0); break;
    case Unit::Db: std::snprintf(out, capacity, "%+.1f dB", value); break;
    case Unit::Hz: std::snprintf(out, capacity, value < 100.0 ? "%.2f Hz" : "%.1f Hz", value); break;
    case Unit::Semitones: std::snprintf(out, capacity, s.stepped ? "%+.0f st" : "%+.2f st", value); break;
    case Unit::Cents: std::snprintf(out, capacity, s.min < 0 ? "%+.1f ct" : "%.1f ct", value); break;
    case Unit::Count: std::snprintf(out, capacity, "%.0f", value); break;
    case Unit::Metres: std::snprintf(out, capacity, value < 1.0 ? "%.2f m" : "%.1f m", value); break;
    case Unit::Celsius: std::snprintf(out, capacity, "%.0f C", value); break;
    case Unit::TurnsPerSecond: std::snprintf(out, capacity, "%.2f turns/s", value); break;
    case Unit::Plain:
      if (s.stepped && s.min == 0 && s.max == 1) std::snprintf(out, capacity, value >= 0.5 ? "On" : "Off");
      else std::snprintf(out, capacity, "%g", value);
      break;
    case Unit::Options: std::snprintf(out, capacity, "%g", value); break;
  }
  return true;
}

// The reverse of format_value: accepts the formatted text, a bare number, an option name
// (case-insensitively), or "on"/"off" for a toggle.
inline bool parse_value(uint32_t id, const char* text, double* value) {
  if (id >= kParamCount || !text || !value) return false;
  const ParamSpec& s = spec(id);
  std::string_view in(text);
  while (!in.empty() && (in.front() == ' ' || in.front() == '\t')) in.remove_prefix(1);
  while (!in.empty() && (in.back() == ' ' || in.back() == '\t')) in.remove_suffix(1);
  if (in.empty()) return false;
  const auto equals_ci = [](std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
      if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
  };
  const int count = static_cast<int>(std::lround(s.max - s.min)) + 1;
  if (s.stepped) {
    for (int i = 0; i < count; ++i)
      if (const char* name = option_name(id, s.min + i); name && equals_ci(in, name)) { *value = s.min + i; return true; }
    if (s.min == 0 && s.max == 1 && s.unit == Unit::Plain) {
      if (equals_ci(in, "on") || equals_ci(in, "yes")) { *value = 1; return true; }
      if (equals_ci(in, "off") || equals_ci(in, "no")) { *value = 0; return true; }
    }
  }
  char* end = nullptr;
  std::string copy(in);
  const double parsed = std::strtod(copy.c_str(), &end);
  if (end == copy.c_str() || !std::isfinite(parsed)) return false;
  *value = clamp_to_spec(id, s.unit == Unit::Percent ? parsed / 100.0 : parsed);
  // A bare number without the percent sign inside the range is taken literally.
  if (s.unit == Unit::Percent && std::strchr(copy.c_str(), '%') == nullptr && parsed >= s.min && parsed <= s.max)
    *value = clamp_to_spec(id, parsed);
  return true;
}

}  // namespace schrodr::params
