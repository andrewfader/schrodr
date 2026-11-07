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
      // --- appended presets ------------------------------------------------------------------------
      {"Antique Steinway", {{kInstrument, kGrand}, {kPitchA4, 435}, {kUnisonSpread, 3.5}, {kHammerHardness, 0.4}, {kMechanics, 0.7}, {kLid, 1}, {kHumidity, 0.35}, {kRoomSize, 3}, {kRoomAbsorption, 0.35}, {kMicDistance, 1.8}, {kMasterDb, -6.1}}},
      {"Violin", {{kInstrument, kViolin}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -4.7}}},
      {"Violin, wide vibrato", {{kInstrument, kViolin}, {kVibrato, 22}, {kVibratoRate, 6.2}, {kBowForce, 0.8}, {kBowSpeed, 0.35}, {kRoomSize, 4}, {kMicDistance, 2}, {kMasterDb, -6}}},
      {"Viola", {{kInstrument, kViola}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -15.5}}},
      {"Cello", {{kInstrument, kCello}, {kBowForce, 1.5}, {kRoomSize, 3}, {kMicDistance, 1.5}, {kMasterDb, -17.6}}},
      {"Cello in a hall", {{kInstrument, kCello}, {kBowForce, 1.5}, {kVibrato, 16}, {kVibratoRate, 5}, {kRoomSize, 5}, {kRoomAbsorption, 0.2}, {kMicDistance, 4}, {kMasterDb, -14.7}}},
      // --- studio sounds, each at an A4 measured from a period record (tape ran where it ran) ----
      {"Flemish, close studio", {{kInstrument, kFlemish}, {kPitchA4, 433.9}, {kFront8, 1}, {kRoomSize, 3}, {kRoomAbsorption, 0.4}, {kMicDistance, 0.8}, {kMasterDb, -8.3}}},
      {"French, crisp studio", {{kInstrument, kFrench}, {kPitchA4, 437.3}, {kFront8, 1}, {kPlectrum, 0.7}, {kRoomSize, 3}, {kRoomAbsorption, 0.4}, {kMicDistance, 0.8}, {kMasterDb, -14.2}}},
      {"Italian, low tape pitch", {{kInstrument, kItalian}, {kPitchA4, 431.2}, {kFront8, 1}, {kRoomSize, 3}, {kMicDistance, 1.0}, {kMasterDb, -13.4}}},
      {"Violins, dry and close", {{kInstrument, kViolin}, {kPitchA4, 442.0}, {kVibrato, 3}, {kBowForce, 0.9}, {kBowSpeed, 0.4}, {kRoomSize, 1}, {kMicDistance, 0.3}, {kMasterDb, -4.3}}},
      {"Cellos, dry and close", {{kInstrument, kCello}, {kPitchA4, 442.0}, {kVibrato, 3}, {kBowForce, 2.0}, {kBowSpeed, 0.4}, {kRoomSize, 1}, {kMicDistance, 0.3}, {kMasterDb, -17.9}}},
      {"Violins, warm studio", {{kInstrument, kViolin}, {kPitchA4, 435.9}, {kVibrato, 14}, {kVibratoRate, 5.8}, {kRoomSize, 3}, {kRoomAbsorption, 0.3}, {kMicDistance, 2}, {kMasterDb, -7.8}}},
      {"Cellos, hard sawing", {{kInstrument, kCello}, {kPitchA4, 438.7}, {kBowForce, 2.2}, {kBowSpeed, 0.45}, {kVibrato, 6}, {kRoomSize, 3}, {kMicDistance, 0.6}, {kMasterDb, -17.7}}},
      {"Celesta, sixties studio", {{kInstrument, kCelesta}, {kPitchA4, 434.3}, {kRoomSize, 3}, {kMicDistance, 1.0}, {kMasterDb, -11.5}}},
      {"Upright, bright and close", {{kInstrument, kUpright}, {kPitchA4, 434.2}, {kHammerHardness, 0.75}, {kUnisonSpread, 3}, {kRoomSize, 3}, {kRoomAbsorption, 0.4}, {kMicDistance, 0.5}, {kMasterDb, -11.4}}},
      {"Grand, studio ballad", {{kInstrument, kGrand}, {kPitchA4, 440.6}, {kRoomSize, 3}, {kRoomAbsorption, 0.35}, {kMicDistance, 1.2}, {kMasterDb, -9}}},
      {"Electric piano, soft and dry", {{kInstrument, kElectricPiano}, {kPitchA4, 438.9}, {kHammerHardness, 0.4}, {kRoomSize, 2}, {kMicDistance, 0.5}, {kMasterDb, -13.2}}},
      {"Violins, airy room", {{kInstrument, kViolin}, {kPitchA4, 439.8}, {kVibrato, 12}, {kRoomSize, 4}, {kMicDistance, 3}, {kMasterDb, -6.5}}},
      {"Violins, big hall drive", {{kInstrument, kViolin}, {kPitchA4, 439.9}, {kBowForce, 1.0}, {kBowSpeed, 0.5}, {kVibrato, 16}, {kVibratoRate, 6}, {kRoomSize, 5}, {kRoomAbsorption, 0.2}, {kMicDistance, 5}, {kMasterDb, -12.7}}},
      {"Cello, intimate", {{kInstrument, kCello}, {kPitchA4, 437.3}, {kBowForce, 1.3}, {kBowSpeed, 0.25}, {kVibrato, 10}, {kRoomSize, 2}, {kMicDistance, 0.8}, {kMasterDb, -16.5}}},
      // --- famous instruments and ways of playing: a setup and a player's habits, on the family's
      //     one model (the bodies here are not measurements of those particular instruments) ------
      {"Stradivarius, concert", {{kInstrument, kViolin}, {kPitchA4, 442}, {kBowForce, 0.8}, {kBowSpeed, 0.35}, {kVibrato, 15}, {kVibratoRate, 6}, {kRoomSize, 5}, {kRoomAbsorption, 0.25}, {kMicDistance, 6}, {kMasterDb, -8.9}}},
      {"Stradivarius, baroque", {{kInstrument, kViolin}, {kPitchA4, 415}, {kTemperament, 5}, {kBowForce, 0.5}, {kBowSpeed, 0.25}, {kBowPosition, 0.13}, {kVibrato, 3}, {kRoomSize, 6}, {kRoomAbsorption, 0.15}, {kMicDistance, 5}, {kMasterDb, 2.7}}},
      {"Il Cannone, Paganini", {{kInstrument, kViolin}, {kPitchA4, 442}, {kBowForce, 1.0}, {kBowSpeed, 0.45}, {kBowPosition, 0.08}, {kVibrato, 18}, {kVibratoRate, 6.5}, {kRoomSize, 5}, {kMicDistance, 4}, {kMasterDb, -8.3}}},
      {"Davidov cello, du Pre", {{kInstrument, kCello}, {kPitchA4, 440}, {kBowForce, 1.8}, {kBowSpeed, 0.4}, {kVibrato, 22}, {kVibratoRate, 6}, {kRoomSize, 5}, {kRoomAbsorption, 0.22}, {kMicDistance, 4}, {kMasterDb, -18.1}}},
      {"Fiddle, front porch", {{kInstrument, kViolin}, {kBowForce, 0.8}, {kBowSpeed, 0.45}, {kVibrato, 0}, {kRoomSize, 2}, {kRoomAbsorption, 0.15}, {kMicDistance, 1.0}, {kMasterDb, -9.2}}},
      {"Viola, baroque", {{kInstrument, kViola}, {kPitchA4, 415}, {kTemperament, 5}, {kBowForce, 0.5}, {kVibrato, 3}, {kRoomSize, 4}, {kMicDistance, 2.5}, {kMasterDb, -8.0}}},
      {"Cello, baroque", {{kInstrument, kCello}, {kPitchA4, 415}, {kTemperament, 5}, {kBowForce, 1.2}, {kBowSpeed, 0.25}, {kVibrato, 3}, {kRoomSize, 4}, {kMicDistance, 2.5}, {kMasterDb, -17.6}}},
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
