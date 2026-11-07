// Which parameters the editor shows, where, and as what kind of control. Grouped by what a
// player reaches for rather than by parameter ID; the tests check that every parameter is
// reachable from exactly one place.
#pragma once

#include <vector>

#include "../params.hpp"
#include "editor.hpp"

namespace schrodr::ui {

inline std::vector<PageSpec> schrodr_pages() {
  using namespace schrodr::params;
  using W = Widget;
  const auto knob = [](int id) { return ControlSpec{W::Knob, id}; };
  const auto toggle = [](int id) { return ControlSpec{W::Toggle, id}; };
  const auto segments = [](int id, int span) { return ControlSpec{W::Segments, id, span}; };
  const auto menu = [](int id, int span) { return ControlSpec{W::Menu, id, span}; };
  return {
      {"INSTRUMENT", "Choose an instrument and how it is tuned  •  hover any control for help", theme::amber, 3, 190,
       {{"Instrument", 0, {menu(kInstrument, 3), knob(kMasterDb), knob(kDynamics), knob(kTranspose)}},
        {"Tuning", 1, {knob(kPitchA4), menu(kTemperament, 2), menu(kTemperamentRoot, 2), knob(kStretch), knob(kUnisonSpread)}},
        {"Stereo", 2, {knob(kStereoWidth), knob(kPerspective)}}}},
      {"ACTION", "Hammers, plectra, tangents, the wheel and the bow: what strikes, plucks, presses or bows the strings", theme::orange, 3, 168,
       {{"Hammer and plectrum", 0, {knob(kHammerHardness), knob(kStrikePoint), knob(kPlectrum)}},
        {"Keys and dampers", 1, {knob(kMechanics), knob(kRelease), knob(kBebung)}},
        {"Bow and left hand", 1, {knob(kBowForce), knob(kBowSpeed), knob(kBowPosition), knob(kVibrato), knob(kVibratoRate)}},
        {"Wheel", 2, {knob(kWheelSpeed), knob(kWheelPressure), knob(kTrompette), toggle(kDrones)}}}},
      {"STOPS", "A harpsichord's registers and an organ's stops, wind and tremulant", theme::green, 3, 150,
       {{"Registers", 0, {toggle(kBack8), toggle(kFront8), toggle(kFour), toggle(kLute), toggle(kBuff)}},
        {"Stops", 1, {toggle(kDiapason8), toggle(kGedackt8), toggle(kPrincipal4), toggle(kFifteenth2), toggle(kQuint)}},
        {"Wind", 2, {knob(kWind), knob(kTremulant), knob(kTremulantRate), knob(kChiff)}}}},
      {"PEDALS", "The pedals and knee levers, the dampers' lift, the lid", theme::red, 3, 0,
       {{"Pedals", 0, {knob(kSustain), toggle(kSostenuto), knob(kUnaCorda), knob(kModerator)}},
        {"Resonance", 1, {knob(kSympathetic)}},
        {"Lid", 2, {segments(kLid, 3)}}}},
      {"ROOM", "The room the instrument stands in, the air, and the microphones", theme::cyan, 3, 150,
       {{"Room", 0, {menu(kRoomSize, 2), knob(kRoomAbsorption), knob(kMicDistance)}},
        {"Microphones", 1, {menu(kMicPattern, 2), segments(kMicSetup, 2)}},
        {"Air", 2, {knob(kHumidity), knob(kTemperature)}}}},
  };
}

// Names for the knob cells, where the section title already supplies the context. Null means
// the full parameter name fits.
inline const char* schrodr_short_name(int id) {
  using namespace schrodr::params;
  switch (id) {
    case kPitchA4: return "A4";
    case kTemperamentRoot: return "Key";
    case kUnisonSpread: return "Unisons";
    case kHammerHardness: return "Hardness";
    case kStrikePoint: return "Strike point";
    case kWheelSpeed: return "Speed";
    case kWheelPressure: return "Pressure";
    case kDiapason8: return "Diapason 8'";
    case kTremulantRate: return "Trem rate";
    case kSustain: return "Sustain";
    case kRoomAbsorption: return "Absorption";
    case kMicDistance: return "Distance";
    case kMicPattern: return "Pattern";
    case kMicSetup: return "Setup";
    case kStereoWidth: return "Width";
    case kBowForce: return "Force";
    case kBowSpeed: return "Speed";
    case kBowPosition: return "Position";
    case kVibratoRate: return "Vib rate";
    default: return nullptr;
  }
}

// The preset menu in the header is reachable from every page.
inline ControlSpec schrodr_header_control() { return ControlSpec{Widget::Menu, schrodr::params::kPreset}; }

// Parameters edited through page artwork rather than a card control (none at present).
inline std::vector<int> schrodr_strip_params() { return {}; }

// Whether a parameter does anything for an instrument. The rules are the instruments': a
// harpsichord has registers and no hammers, an organ has stops and wind, a clavichord bebung.
inline bool param_relevant_for_instrument(int id, int instrument) {
  using namespace schrodr::params;
  const bool strings = is_string_instrument(instrument), bars = is_bar_instrument(instrument), organ = is_organ(instrument);
  const bool hammered = is_hammered(instrument) || bars, plucked = is_harpsichord(instrument), tangent = is_clavichord(instrument), bowed = is_bowed(instrument);
  const bool wheel = is_wheel(instrument), violin = is_violin_family(instrument);
  switch (id) {
    case kHammerHardness: return hammered;
    case kStrikePoint: return hammered && !bars;
    case kPlectrum: return plucked;
    case kBebung: return tangent;
    case kWheelSpeed: case kWheelPressure: return wheel;
    case kBowForce: case kBowSpeed: case kBowPosition: case kVibrato: case kVibratoRate: return violin;
    case kTrompette: case kDrones: return instrument == kHurdyGurdy;
    case kBack8: case kFront8: case kFour: case kLute: case kBuff: return plucked;
    case kDiapason8: case kGedackt8: case kPrincipal4: case kFifteenth2: case kQuint: case kWind: case kTremulant: case kTremulantRate: case kChiff: return organ;
    case kSustain: return (strings && !bowed) || (bars && instrument != kGlockenspiel && instrument != kToyPiano);
    case kSostenuto: return instrument == kGrand || bars;
    case kUnaCorda: return instrument == kGrand || instrument == kUpright || instrument == kFortepiano || instrument == kCristofori;
    case kModerator: return instrument == kFortepiano || instrument == kSquare || instrument == kTangent;
    case kSympathetic: return strings && !bowed;
    case kLid: return strings && !violin;
    case kStretch: case kUnisonSpread: return strings && !violin;
    case kRelease: return !organ && !wheel;
    case kMechanics: return !organ && !violin;
    default: return true;
  }
}

}  // namespace schrodr::ui
