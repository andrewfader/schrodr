// Factory presets: named starting points, one instrument each, in the rooms and at the pitches
// their music was made in. A preset is the parameters it changes from their defaults, so it
// stays valid when a parameter is appended to the table. Every preset is measured against
// docs/QUALITY.md by `schrodr-preset-audit` and held to it by engine_tests. The order is
// append-only, because the preset index is an automatable parameter value.
#pragma once

#include <cstddef>
#include <vector>

#include "params.hpp"

namespace schrodr::presets {

struct Setting { params::ParamId id; double value; };
struct Preset { const char* name; std::vector<Setting> settings; };

using namespace params;

inline const std::vector<Preset>& all() {
  static const std::vector<Preset> table{
      // --- pianos ---------------------------------------------------------------------------------
      {"Concert grand", {{kInstrument, kGrand}, {kRoomSize, 5}, {kRoomAbsorption, 0.3}, {kMicDistance, 2.5}, {kMasterDb, -8.9}}},
      {"Grand, close pair", {{kInstrument, kGrand}, {kMicDistance, 0.6}, {kRoomSize, 3}, {kMicPattern, 1}, {kMasterDb, -7.1}}},
      {"Grand, lid down", {{kInstrument, kGrand}, {kLid, 0}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -2.9}}},
      {"Soft felt grand", {{kInstrument, kGrand}, {kHammerHardness, 0.2}, {kDynamics, 0.7}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -9.2}}},
      {"Bright grand", {{kInstrument, kGrand}, {kHammerHardness, 0.8}, {kStrikePoint, 0.85}, {kRoomSize, 3}, {kMicDistance, 1.2}, {kMasterDb, -11.4}}},
      {"Upright, bedroom", {{kInstrument, kUpright}, {kRoomSize, 2}, {kMicDistance, 1.0}, {kLid, 1}, {kMasterDb, -10.5}}},
      {"Honky upright", {{kInstrument, kUpright}, {kUnisonSpread, 8}, {kHammerHardness, 0.85}, {kRoomSize, 3}, {kRoomAbsorption, 0.15}, {kMicDistance, 1.5}, {kMasterDb, -14.3}}},
      {"Upright, soft pedal", {{kInstrument, kUpright}, {kUnaCorda, 1}, {kHammerHardness, 0.35}, {kRoomSize, 2}, {kMicDistance, 0.8}, {kMasterDb, -7.4}}},
      {"Grand, audience", {{kInstrument, kGrand}, {kPerspective, 1}, {kRoomSize, 5}, {kMicDistance, 6}, {kMicPattern, 1}, {kMasterDb, -9.6}}},
      {"Grand, Werckmeister", {{kInstrument, kGrand}, {kTemperament, 3}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -10.2}}},
      // --- early pianos ---------------------------------------------------------------------------
      {"Fortepiano, Walter", {{kInstrument, kFortepiano}, {kPitchA4, 430}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -15.1}}},
      {"Fortepiano, moderator", {{kInstrument, kFortepiano}, {kPitchA4, 430}, {kModerator, 1}, {kRoomSize, 3}, {kMicDistance, 1.2}, {kMasterDb, -5.6}}},
      {"Fortepiano, Kirnberger", {{kInstrument, kFortepiano}, {kPitchA4, 430}, {kTemperament, 4}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -11.6}}},
      {"Square piano, parlour", {{kInstrument, kSquare}, {kPitchA4, 430}, {kRoomSize, 2}, {kMicDistance, 1.2}, {kMasterDb, -13.6}}},
      {"Cristofori 1720", {{kInstrument, kCristofori}, {kPitchA4, 415}, {kTemperament, 2}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -14.2}}},
      {"Tangent piano", {{kInstrument, kTangent}, {kPitchA4, 430}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -15.8}}},
      {"Tangent piano, moderator", {{kInstrument, kTangent}, {kPitchA4, 430}, {kModerator, 1}, {kRoomSize, 2}, {kMicDistance, 1.0}, {kMasterDb, -10}}},
      // --- harpsichords ---------------------------------------------------------------------------
      {"Flemish 8'", {{kInstrument, kFlemish}, {kPitchA4, 415}, {kTemperament, 2}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -12.9}}},
      {"Flemish, two 8'", {{kInstrument, kFlemish}, {kPitchA4, 415}, {kTemperament, 2}, {kFront8, 1}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -15.2}}},
      {"Flemish, full", {{kInstrument, kFlemish}, {kPitchA4, 415}, {kTemperament, 2}, {kFront8, 1}, {kFour, 1}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -16.4}}},
      {"Flemish, lute stop", {{kInstrument, kFlemish}, {kPitchA4, 415}, {kTemperament, 2}, {kBack8, 0}, {kLute, 1}, {kRoomSize, 3}, {kMicDistance, 1.2}, {kMasterDb, -15.2}}},
      {"Italian harpsichord", {{kInstrument, kItalian}, {kPitchA4, 415}, {kTemperament, 2}, {kFront8, 1}, {kRoomSize, 3}, {kRoomAbsorption, 0.2}, {kMicDistance, 1.5}, {kMasterDb, -15.2}}},
      {"French double, Vallotti", {{kInstrument, kFrench}, {kPitchA4, 415}, {kTemperament, 5}, {kFront8, 1}, {kFour, 1}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -20.9}}},
      {"French, buff stop", {{kInstrument, kFrench}, {kPitchA4, 415}, {kTemperament, 5}, {kBuff, 1}, {kRoomSize, 3}, {kMicDistance, 1.2}, {kMasterDb, -6.7}}},
      {"Harpsichord in a church", {{kInstrument, kFrench}, {kPitchA4, 415}, {kTemperament, 3}, {kFront8, 1}, {kRoomSize, 6}, {kRoomAbsorption, 0.12}, {kMicDistance, 5}, {kMasterDb, -13.9}}},
      {"Virginal", {{kInstrument, kVirginal}, {kPitchA4, 415}, {kTemperament, 2}, {kRoomSize, 2}, {kMicDistance, 1.0}, {kMasterDb, -24.8}}},
      {"Spinet", {{kInstrument, kSpinet}, {kPitchA4, 415}, {kTemperament, 2}, {kRoomSize, 2}, {kMicDistance, 1.0}, {kMasterDb, -17.9}}},
      {"Lautenwerck", {{kInstrument, kLautenwerck}, {kPitchA4, 415}, {kTemperament, 3}, {kFront8, 1}, {kRoomSize, 3}, {kMicDistance, 1.2}, {kMasterDb, -19.1}}},
      // --- clavichords ----------------------------------------------------------------------------
      {"Fretted clavichord", {{kInstrument, kFrettedClavichord}, {kPitchA4, 415}, {kTemperament, 2}, {kRoomSize, 2}, {kMicDistance, 0.5}, {kMasterDb, -18.6}}},
      {"Clavichord, Vallotti", {{kInstrument, kUnfrettedClavichord}, {kPitchA4, 415}, {kTemperament, 5}, {kRoomSize, 2}, {kMicDistance, 0.6}, {kMasterDb, -17.6}}},
      {"Clavichord, bebung", {{kInstrument, kUnfrettedClavichord}, {kPitchA4, 415}, {kTemperament, 3}, {kBebung, 30}, {kRoomSize, 2}, {kMicDistance, 0.4}, {kMicPattern, 1}, {kMasterDb, -16.7}}},
      // --- wheels ---------------------------------------------------------------------------------
      {"Hurdy-gurdy", {{kInstrument, kHurdyGurdy}, {kRoomSize, 3}, {kMicDistance, 1.2}, {kMasterDb, -7.1}}},
      {"Hurdy-gurdy, trompette", {{kInstrument, kHurdyGurdy}, {kTrompette, 0.9}, {kWheelSpeed, 1.6}, {kRoomSize, 4}, {kRoomAbsorption, 0.2}, {kMicDistance, 2}, {kMasterDb, -13.8}}},
      {"Hurdy-gurdy, no drones", {{kInstrument, kHurdyGurdy}, {kDrones, 0}, {kRoomSize, 2}, {kMicDistance, 0.8}, {kMasterDb, -4.3}}},
      {"Geigenwerk", {{kInstrument, kGeigenwerk}, {kPitchA4, 415}, {kTemperament, 2}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -11.2}}},
      // --- bars -----------------------------------------------------------------------------------
      {"Celesta", {{kInstrument, kCelesta}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -10.8}}},
      {"Celesta, pedal", {{kInstrument, kCelesta}, {kSustain, 1}, {kRoomSize, 5}, {kMicDistance, 3}, {kMasterDb, -16.9}}},
      {"Glockenspiel", {{kInstrument, kGlockenspiel}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -13.5}}},
      {"Dulcitone", {{kInstrument, kDulcitone}, {kRoomSize, 2}, {kMicDistance, 1.0}, {kMasterDb, -14}}},
      {"Electric piano", {{kInstrument, kElectricPiano}, {kRoomSize, 0}, {kMasterDb, -10.7}}},
      {"Electric piano, bark", {{kInstrument, kElectricPiano}, {kHammerHardness, 0.8}, {kDynamics, 1}, {kRoomSize, 1}, {kMicDistance, 0.3}, {kMasterDb, -13.4}}},
      {"Toy piano", {{kInstrument, kToyPiano}, {kRoomSize, 2}, {kMicDistance, 0.8}, {kMasterDb, -17.6}}},
      // --- the organ --------------------------------------------------------------------------------
      {"Chamber organ, diapason", {{kInstrument, kOrgan}, {kRoomSize, 5}, {kRoomAbsorption, 0.15}, {kMicDistance, 4}, {kMasterDb, -13.1}}},
      {"Organ, gedackt", {{kInstrument, kOrgan}, {kDiapason8, 0}, {kGedackt8, 1}, {kRoomSize, 4}, {kMicDistance, 3}, {kMasterDb, -9.3}}},
      {"Organ, principal chorus", {{kInstrument, kOrgan}, {kPrincipal4, 1}, {kFifteenth2, 1}, {kRoomSize, 6}, {kRoomAbsorption, 0.12}, {kMicDistance, 6}, {kMasterDb, -14.6}}},
      {"Organ, with quint", {{kInstrument, kOrgan}, {kGedackt8, 1}, {kQuint, 1}, {kRoomSize, 5}, {kMicDistance, 4}, {kMasterDb, -11.1}}},
      {"Organ, tremulant", {{kInstrument, kOrgan}, {kDiapason8, 0}, {kGedackt8, 1}, {kTremulant, 0.8}, {kTremulantRate, 5.5}, {kRoomSize, 4}, {kMicDistance, 3}, {kMasterDb, -10.1}}},
      {"Organ, meantone", {{kInstrument, kOrgan}, {kTemperament, 2}, {kPitchA4, 415}, {kPrincipal4, 1}, {kRoomSize, 6}, {kMicDistance, 5}, {kMasterDb, -10.9}}},
  };
  return table;
}

inline size_t count() { return all().size(); }

inline void apply(size_t index, double* values) {
  for (size_t i = 0; i < params::kParamCount; ++i) values[i] = params::kSpecs[i].def;
  if (index >= count()) return;
  for (const Setting& s : all()[index].settings) values[s.id] = s.value;
  values[params::kPreset] = static_cast<double>(index);
}

inline const char* const* names() {
  static std::vector<const char*> list = [] {
    std::vector<const char*> n;
    for (const Preset& p : all()) n.push_back(p.name);
    return n;
  }();
  return list.data();
}

}  // namespace schrodr::presets
