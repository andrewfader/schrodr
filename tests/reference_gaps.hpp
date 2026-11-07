// The comparisons with the reference recordings that SCHRODR does not yet meet, each with what
// is wrong. reference_tests reports these instead of failing on them, and fails as soon as one
// of them passes, so the list only ever shrinks. Fix the model, then strike the line.
#pragma once

#include <string>
#include <vector>

namespace reference {

struct Gap { const char* id; const char* why; };

// Why, by cause. The numbers are the suite's at the time the gap was listed.
constexpr const char* kGrandLowPartials =
    "the bass and middle lack their low partials: around 250-320 Hz the envelope sits 15-25 dB under the Steinway's "
    "(C2 at its 4th and 5th partials, C4 at its fundamental), so C4 is twice as bright and B0 1.6 times";
constexpr const char* kGrandPromptDecay =
    "the prompt sound falls at the wrong rate: the bass about twice as fast as the Steinway's, middle C half as fast, "
    "C6 and C7 two to nine times as fast";
constexpr const char* kGrandAftersound =
    "the aftersound: B0 and C2 ring out two to four times too fast, C6 holds twice as long as the Steinway's";
constexpr const char* kGrandBassStiffness =
    "the wound bass strings are too flexible: B 7.5e-5 at B0 and 2.3e-5 at C2 against the Steinway's 2.6e-4 and 1.4e-4";
constexpr const char* kGrandTouchRange =
    "the touch's range is too narrow: C4 rises 17.5 dB pp to ff against 26.4, C7 14.7 dB mf to ff against 19.9";
constexpr const char* kGlockOvertone =
    "the first overtone is not where the Iowa bars have it: 2.41 f1 against 3.13 at C6, 2.82 against 3.01 at G6, "
    "3.14 against 2.92 at C7";
constexpr const char* kGlockBright = "C6 at ff is three times as bright as the Iowa bell (centroid 3.6 against 1.1 harmonics)";
constexpr const char* kGlockRing = "the fundamental's ring: C6 rings 2.4 times as long as the Iowa bell, G6 at mf half as long";
constexpr const char* kBowNoise =
    "too much bow noise: the power between the harmonics sits 10 to 27 dB under them where the Iowa strings' sits 27 to 43 dB under";
constexpr const char* kBowTouch =
    "the touch hardly changes a bowed note's level: pp to ff rises 0 to 4 dB (mf to ff 1 to 2) where the Iowa players' rises 9 to 17.5";
constexpr const char* kViolinBrightness =
    "brightness runs the wrong way across the strings: D4 (and A4 at pp) two to three times too bright, A4 to E5 at mf and ff "
    "half as bright as the Iowa violin";
constexpr const char* kViolinColour = "too much above 5 kHz (10 to 18 dB) and too little at 300 to 500 Hz";
constexpr const char* kViolaColour = "too much above 4 kHz (up to 20 dB) and dips of 10 to 20 dB between 200 and 800 Hz";
constexpr const char* kViolaBrightness = "D4 is 1.5 times as bright as the Iowa viola's";
constexpr const char* kCelloLowPartials =
    "the C and G strings lack their fundamental and low partials (100 to 200 Hz 10 to 35 dB under the Iowa cello's) and have "
    "too much above 2 kHz, so they are two to four times too bright";
constexpr const char* kCelloSquealE2 =
    "E2 bowed ff leaves Helmholtz motion after 0.9 s and squeals on its 8th harmonic (665 Hz); pp and mf hold";
constexpr const char* kGrandB0Touch =
    "B0 rises 11.2 dB from pp to mf where the Iowa B0 takes rise 2.6 (the two takes are nearly one level)";
constexpr const char* kUprightBright =
    "the bass and tenor are too bright: C1 to C3 at 1.5 to 2.4 times the VSCO upright's centroid (C2 mf 8.7 harmonics against 3.6)";
constexpr const char* kUprightColour =
    "the envelope sits 9 to 14 dB RMS from the VSCO upright's through most of the compass (bar 8.5)";
constexpr const char* kUprightTouch =
    "the touch's range is too narrow: pp to f rises 12 to 23 dB against the upright's 27 to 33, mf to f at C3 1.6 against 8.3";
constexpr const char* kUprightPromptDecay =
    "the prompt sound: C1 and C2 fall at two thirds of the upright's rate, C6 at three times it (88 dB/s against 23 to 32), C7 at half";
constexpr const char* kUprightAftersound =
    "the aftersound: C2 and C5 ring 1.5 to 2.5 times too long, C4 half as long";
constexpr const char* kUprightStiffness =
    "B: C3 5.6 times, C4 1.6 times and C7 4.2 times the upright's";
constexpr const char* kHarpsichordStiffness =
    "the strings are too flexible: B a half to an eighth of the recordings' below C5 (Italian C#3 3.3e-6 against 2.6e-5); the Italian B5 a ninth, the Flemish A#5 1.7 times too stiff";
constexpr const char* kHarpsichordPromptDecay =
    "the prompt sound falls 1.5 to 4 times too fast through the bass and middle (Flemish C4 35 dB/s against 10.5, French F#3 20 against 5); the Italian F#4 too slowly (21.5 against 35)";
constexpr const char* kHarpsichordAftersound =
    "the aftersound: the bass rings out about twice too fast (French C2 7.7 dB/s against 3.0), the treble twice too long (Italian F#5 6.3 against 14.3, Flemish A#5 7.9 against 18.4)";
constexpr const char* kHarpsichordColour =
    "the envelope sits 10 to 16 dB RMS from the VCSL harpsichords' across the compass";
constexpr const char* kHarpsichordBrightness =
    "brightness misses both ways: the bass too bright (French F#2 5.3 harmonics against 2.9, Flemish C2 5.9 against 4.1), the middle too dull (Flemish C4 1.6 against 2.5, French C5 1.4 against 3.8), the Italian F#4 2.7 times too bright";
constexpr const char* kClavichordBright =
    "the bass and middle are 1.5 to 6 times too bright (D#3 16.8 harmonics against 2.7, C4 7.4 against 2.2) and their envelopes sit 7 to 14 dB RMS from the Edinburgh clavichord's; B3 is too dull";
constexpr const char* kClavichordStiffness =
    "the strings are too flexible: B a half to a twenty-fifth of the Edinburgh clavichord's";
constexpr const char* kClavichordPromptDecay =
    "the prompt sound falls at half the clavichord's rate from C2 to D5 (C3 20 dB/s against 42), B3 1.5 times too fast";
constexpr const char* kClavichordC6Sharp =
    "C6 sounds 29 cents sharp of its key (1064 Hz)";
constexpr const char* kElectricPianoColour =
    "the envelope sits 8 to 20 dB RMS from the jRhodes3d's (recorded through EQ with a treble boost and a low-mid scoop, so part of this is the recording)";
constexpr const char* kElectricPianoBrightness =
    "brightness runs the wrong way with the touch: the Rhodes' bark is in its soft layers and gone at ff (E2 6.1 harmonics at pp, 1.05 at ff), SCHRODR's grows with the blow (5.6 to 6.7)";
constexpr const char* kElectricPianoRing =
    "the tine rings two to four times as long as the Rhodes' (B3 ff 0.8 dB/s against 3.4); the bass even swells after the blow";
constexpr const char* kCelestaRing =
    "the plates below C6 ring two to five times as long as the Sonatina celesta's (E4 3.5 dB/s against 14 to 17); C6 at mf 1.5 times too short";
constexpr const char* kCelestaC4Bright =
    "C4 at ff is twice as bright as the recording (2.06 harmonics against 1.01)";
constexpr const char* kToyOvertone =
    "the rods' first overtone sits at 4.25 f1 below D#5 and 3.00 from it up, where the Michelsonne's is about 4.03 (3.56 at A#4, 2.23 at D#5)";
constexpr const char* kToyRing =
    "the lower rods ring two to five times as long as the Michelsonne's (C5 17 dB/s against 70)";
constexpr const char* kToyF4Bright =
    "F4 is half as bright as the Michelsonne's (5.5 harmonics against 11.2)";
constexpr const char* kOrganF2 =
    "F#2 has too little above its fundamental: centroid 1.02 harmonics against the Renaissance organ's 1.90";

inline const std::vector<Gap>& known_gaps() {
  static const std::vector<Gap> all{
      {"grand_B0_pp:colour", kGrandLowPartials},
      {"grand_B0_pp:prompt decay", kGrandPromptDecay},
      {"grand_B0_pp:aftersound decay", kGrandAftersound},
      {"grand_B0_mf:colour", kGrandLowPartials},
      {"grand_B0_mf:brightness", kGrandLowPartials},
      {"grand_B0_mf:inharmonicity", kGrandBassStiffness},
      {"grand_B0_mf:aftersound decay", kGrandAftersound},
      {"grand_C2_pp:colour", kGrandLowPartials},
      {"grand_C2_pp:prompt decay", kGrandPromptDecay},
      {"grand_C2_pp:aftersound decay", kGrandAftersound},
      {"grand_C2_mf:colour", kGrandLowPartials},
      {"grand_C2_mf:prompt decay", kGrandPromptDecay},
      {"grand_C2_mf:aftersound decay", kGrandAftersound},
      {"grand_C2_ff:colour", kGrandLowPartials},
      {"grand_C2_ff:inharmonicity", kGrandBassStiffness},
      {"grand_C2_ff:prompt decay", kGrandPromptDecay},
      {"grand_C2_ff:aftersound decay", kGrandAftersound},
      {"grand_C4_pp:colour", kGrandLowPartials},
      {"grand_C4_pp:brightness", kGrandLowPartials},
      {"grand_C4_pp:prompt decay", kGrandPromptDecay},
      {"grand_C4_mf:colour", kGrandLowPartials},
      {"grand_C4_mf:brightness", kGrandLowPartials},
      {"grand_C4_mf:prompt decay", kGrandPromptDecay},
      {"grand_C4_ff:colour", kGrandLowPartials},
      {"grand_C4_ff:brightness", kGrandLowPartials},
      {"grand_C4_ff:dynamics", kGrandTouchRange},
      {"grand_C4_ff:prompt decay", kGrandPromptDecay},
      {"grand_C6_pp:prompt decay", kGrandPromptDecay},
      {"grand_C6_mf:prompt decay", kGrandPromptDecay},
      {"grand_C6_ff:prompt decay", kGrandPromptDecay},
      {"grand_C6_ff:aftersound decay", kGrandAftersound},
      {"grand_C7_mf:prompt decay", kGrandPromptDecay},
      {"grand_C7_ff:dynamics", kGrandTouchRange},
      {"glockenspiel_C6_ff:brightness", kGlockBright},
      {"glockenspiel_C6_ff:overtone", kGlockOvertone},
      {"glockenspiel_C6_ff:decay", kGlockRing},
      {"glockenspiel_G6_mf:overtone", kGlockOvertone},
      {"glockenspiel_G6_mf:decay", kGlockRing},
      {"glockenspiel_G6_ff:overtone", kGlockOvertone},
      {"glockenspiel_C7_ff:overtone", kGlockOvertone},
      {"violin_G3_ff:colour", kViolinColour},
      {"violin_G3_ff:bow noise", kBowNoise},
      {"violin_B3_mf:colour", kViolinColour},
      {"violin_B3_ff:colour", kViolinColour},
      {"violin_B3_ff:dynamics", kBowTouch},
      {"violin_D4_pp:colour", kViolinColour},
      {"violin_D4_pp:brightness", kViolinBrightness},
      {"violin_D4_pp:bow noise", kBowNoise},
      {"violin_D4_mf:colour", kViolinColour},
      {"violin_D4_mf:brightness", kViolinBrightness},
      {"violin_D4_mf:bow noise", kBowNoise},
      {"violin_D4_ff:colour", kViolinColour},
      {"violin_D4_ff:brightness", kViolinBrightness},
      {"violin_D4_ff:dynamics", kBowTouch},
      {"violin_D4_ff:bow noise", kBowNoise},
      {"violin_A4_pp:brightness", kViolinBrightness},
      {"violin_A4_pp:bow noise", kBowNoise},
      {"violin_A4_mf:brightness", kViolinBrightness},
      {"violin_A4_mf:bow noise", kBowNoise},
      {"violin_A4_ff:colour", kViolinColour},
      {"violin_A4_ff:brightness", kViolinBrightness},
      {"violin_A4_ff:dynamics", kBowTouch},
      {"violin_A4_ff:bow noise", kBowNoise},
      {"violin_C5_pp:bow noise", kBowNoise},
      {"violin_C5_mf:colour", kViolinColour},
      {"violin_C5_mf:brightness", kViolinBrightness},
      {"violin_C5_ff:colour", kViolinColour},
      {"violin_C5_ff:brightness", kViolinBrightness},
      {"violin_C5_ff:dynamics", kBowTouch},
      {"violin_C5_ff:bow noise", kBowNoise},
      {"violin_E5_mf:brightness", kViolinBrightness},
      {"violin_E5_mf:bow noise", kBowNoise},
      {"violin_E5_ff:brightness", kViolinBrightness},
      {"violin_E5_ff:dynamics", kBowTouch},
      {"violin_E5_ff:bow noise", kBowNoise},
      {"viola_C3_mf:colour", kViolaColour},
      {"viola_C3_mf:bow noise", kBowNoise},
      {"viola_C3_ff:colour", kViolaColour},
      {"viola_C3_ff:bow noise", kBowNoise},
      {"viola_E3_mf:colour", kViolaColour},
      {"viola_E3_mf:bow noise", kBowNoise},
      {"viola_E3_ff:colour", kViolaColour},
      {"viola_G3_mf:colour", kViolaColour},
      {"viola_G3_mf:bow noise", kBowNoise},
      {"viola_G3_ff:bow noise", kBowNoise},
      {"viola_D4_pp:brightness", kViolaBrightness},
      {"viola_D4_pp:bow noise", kBowNoise},
      {"viola_D4_mf:colour", kViolaColour},
      {"viola_D4_mf:brightness", kViolaBrightness},
      {"viola_D4_mf:bow noise", kBowNoise},
      {"viola_D4_ff:colour", kViolaColour},
      {"viola_D4_ff:brightness", kViolaBrightness},
      {"viola_D4_ff:dynamics", kBowTouch},
      {"viola_D4_ff:bow noise", kBowNoise},
      {"viola_A4_mf:bow noise", kBowNoise},
      {"viola_A4_ff:colour", kViolaColour},
      {"viola_A4_ff:bow noise", kBowNoise},
      {"cello_C2_pp:colour", kCelloLowPartials},
      {"cello_C2_pp:brightness", kCelloLowPartials},
      {"cello_C2_pp:bow noise", kBowNoise},
      {"cello_C2_mf:colour", kCelloLowPartials},
      {"cello_C2_mf:brightness", kCelloLowPartials},
      {"cello_C2_mf:bow noise", kBowNoise},
      {"cello_C2_ff:colour", kCelloLowPartials},
      {"cello_C2_ff:brightness", kCelloLowPartials},
      {"cello_C2_ff:dynamics", kBowTouch},
      {"cello_C2_ff:bow noise", kBowNoise},
      {"cello_E2_pp:colour", kCelloLowPartials},
      {"cello_E2_pp:brightness", kCelloLowPartials},
      {"cello_E2_mf:colour", kCelloLowPartials},
      {"cello_E2_mf:bow noise", kBowNoise},
      {"cello_E2_ff:pitch", kCelloSquealE2},
      {"cello_E2_ff:colour", kCelloLowPartials},
      {"cello_E2_ff:dynamics", kBowTouch},
      {"cello_E2_ff:bow noise", kBowNoise},
      {"cello_G2_pp:colour", kCelloLowPartials},
      {"cello_G2_pp:brightness", kCelloLowPartials},
      {"cello_G2_pp:bow noise", kBowNoise},
      {"cello_G2_mf:colour", kCelloLowPartials},
      {"cello_G2_mf:brightness", kCelloLowPartials},
      {"cello_G2_mf:bow noise", kBowNoise},
      {"cello_G2_ff:colour", kCelloLowPartials},
      {"cello_G2_ff:brightness", kCelloLowPartials},
      {"cello_G2_ff:dynamics", kBowTouch},
      {"cello_G2_ff:bow noise", kBowNoise},
      {"cello_D3_pp:colour", kCelloLowPartials},
      {"cello_D3_pp:brightness", kCelloLowPartials},
      {"cello_D3_pp:bow noise", kBowNoise},
      {"cello_D3_mf:bow noise", kBowNoise},
      {"cello_D3_ff:colour", kCelloLowPartials},
      {"cello_D3_ff:brightness", kCelloLowPartials},
      {"cello_D3_ff:dynamics", kBowTouch},
      {"cello_A3_pp:bow noise", kBowNoise},
      {"cello_A3_mf:colour", kCelloLowPartials},
      {"cello_A3_mf:bow noise", kBowNoise},
      {"cello_A3_ff:colour", kCelloLowPartials},
      {"cello_A3_ff:dynamics", kBowTouch},
      {"grand_B0_mf:dynamics", kGrandB0Touch},
      {"upright_C1_f:brightness", kUprightBright},
      {"upright_C1_f:colour", kUprightColour},
      {"upright_C1_f:dynamics", kUprightTouch},
      {"upright_C1_f:prompt decay", kUprightPromptDecay},
      {"upright_C1_mf:brightness", kUprightBright},
      {"upright_C1_mf:colour", kUprightColour},
      {"upright_C1_pp:brightness", kUprightBright},
      {"upright_C1_pp:colour", kUprightColour},
      {"upright_C2_f:aftersound decay", kUprightAftersound},
      {"upright_C2_f:brightness", kUprightBright},
      {"upright_C2_f:colour", kUprightColour},
      {"upright_C2_f:dynamics", kUprightTouch},
      {"upright_C2_f:prompt decay", kUprightPromptDecay},
      {"upright_C2_mf:brightness", kUprightBright},
      {"upright_C2_mf:colour", kUprightColour},
      {"upright_C2_pp:aftersound decay", kUprightAftersound},
      {"upright_C2_pp:brightness", kUprightBright},
      {"upright_C2_pp:colour", kUprightColour},
      {"upright_C2_pp:prompt decay", kUprightPromptDecay},
      {"upright_C3_f:brightness", kUprightBright},
      {"upright_C3_f:colour", kUprightColour},
      {"upright_C3_f:dynamics", kUprightTouch},
      {"upright_C3_f:inharmonicity", kUprightStiffness},
      {"upright_C3_mf:brightness", kUprightBright},
      {"upright_C3_mf:colour", kUprightColour},
      {"upright_C4_f:aftersound decay", kUprightAftersound},
      {"upright_C4_f:colour", kUprightColour},
      {"upright_C4_f:dynamics", kUprightTouch},
      {"upright_C4_f:inharmonicity", kUprightStiffness},
      {"upright_C4_mf:aftersound decay", kUprightAftersound},
      {"upright_C5_f:aftersound decay", kUprightAftersound},
      {"upright_C5_f:colour", kUprightColour},
      {"upright_C5_f:dynamics", kUprightTouch},
      {"upright_C6_f:colour", kUprightColour},
      {"upright_C6_f:dynamics", kUprightTouch},
      {"upright_C6_f:prompt decay", kUprightPromptDecay},
      {"upright_C6_mf:colour", kUprightColour},
      {"upright_C6_mf:prompt decay", kUprightPromptDecay},
      {"upright_C6_pp:prompt decay", kUprightPromptDecay},
      {"upright_C7_f:colour", kUprightColour},
      {"upright_C7_f:inharmonicity", kUprightStiffness},
      {"upright_C7_f:prompt decay", kUprightPromptDecay},
      {"upright_C7_mf:colour", kUprightColour},
      {"flemish_As5:aftersound decay", kHarpsichordAftersound},
      {"flemish_As5:colour", kHarpsichordColour},
      {"flemish_As5:inharmonicity", kHarpsichordStiffness},
      {"flemish_C2:brightness", kHarpsichordBrightness},
      {"flemish_C2:inharmonicity", kHarpsichordStiffness},
      {"flemish_C2:prompt decay", kHarpsichordPromptDecay},
      {"flemish_C3:colour", kHarpsichordColour},
      {"flemish_C3:inharmonicity", kHarpsichordStiffness},
      {"flemish_C3:prompt decay", kHarpsichordPromptDecay},
      {"flemish_C4:brightness", kHarpsichordBrightness},
      {"flemish_C4:colour", kHarpsichordColour},
      {"flemish_C4:inharmonicity", kHarpsichordStiffness},
      {"flemish_C4:prompt decay", kHarpsichordPromptDecay},
      {"flemish_C5:brightness", kHarpsichordBrightness},
      {"flemish_C5:colour", kHarpsichordColour},
      {"flemish_C5:prompt decay", kHarpsichordPromptDecay},
      {"flemish_Fs2:aftersound decay", kHarpsichordAftersound},
      {"flemish_Fs2:colour", kHarpsichordColour},
      {"flemish_Fs2:inharmonicity", kHarpsichordStiffness},
      {"flemish_Fs3:aftersound decay", kHarpsichordAftersound},
      {"flemish_Fs3:brightness", kHarpsichordBrightness},
      {"flemish_Fs3:colour", kHarpsichordColour},
      {"flemish_Fs3:inharmonicity", kHarpsichordStiffness},
      {"flemish_Fs4:brightness", kHarpsichordBrightness},
      {"flemish_Fs4:colour", kHarpsichordColour},
      {"flemish_Fs4:prompt decay", kHarpsichordPromptDecay},
      {"italian_B5:colour", kHarpsichordColour},
      {"italian_B5:inharmonicity", kHarpsichordStiffness},
      {"italian_B5:prompt decay", kHarpsichordPromptDecay},
      {"italian_Cs2:aftersound decay", kHarpsichordAftersound},
      {"italian_Cs2:colour", kHarpsichordColour},
      {"italian_Cs2:inharmonicity", kHarpsichordStiffness},
      {"italian_Cs2:prompt decay", kHarpsichordPromptDecay},
      {"italian_Cs3:aftersound decay", kHarpsichordAftersound},
      {"italian_Cs3:colour", kHarpsichordColour},
      {"italian_Cs3:inharmonicity", kHarpsichordStiffness},
      {"italian_Cs4:colour", kHarpsichordColour},
      {"italian_Cs4:inharmonicity", kHarpsichordStiffness},
      {"italian_Cs5:aftersound decay", kHarpsichordAftersound},
      {"italian_Cs5:colour", kHarpsichordColour},
      {"italian_Cs5:inharmonicity", kHarpsichordStiffness},
      {"italian_Cs5:prompt decay", kHarpsichordPromptDecay},
      {"italian_Fs2:colour", kHarpsichordColour},
      {"italian_Fs2:inharmonicity", kHarpsichordStiffness},
      {"italian_Fs2:prompt decay", kHarpsichordPromptDecay},
      {"italian_Fs3:aftersound decay", kHarpsichordAftersound},
      {"italian_Fs3:colour", kHarpsichordColour},
      {"italian_Fs3:inharmonicity", kHarpsichordStiffness},
      {"italian_Fs3:prompt decay", kHarpsichordPromptDecay},
      {"italian_Fs4:aftersound decay", kHarpsichordAftersound},
      {"italian_Fs4:brightness", kHarpsichordBrightness},
      {"italian_Fs4:colour", kHarpsichordColour},
      {"italian_Fs4:inharmonicity", kHarpsichordStiffness},
      {"italian_Fs4:prompt decay", kHarpsichordPromptDecay},
      {"italian_Fs5:aftersound decay", kHarpsichordAftersound},
      {"italian_Fs5:colour", kHarpsichordColour},
      {"italian_Fs5:inharmonicity", kHarpsichordStiffness},
      {"french_C2:aftersound decay", kHarpsichordAftersound},
      {"french_C2:colour", kHarpsichordColour},
      {"french_C2:inharmonicity", kHarpsichordStiffness},
      {"french_C2:prompt decay", kHarpsichordPromptDecay},
      {"french_C3:colour", kHarpsichordColour},
      {"french_C3:inharmonicity", kHarpsichordStiffness},
      {"french_C3:prompt decay", kHarpsichordPromptDecay},
      {"french_C4:colour", kHarpsichordColour},
      {"french_C4:inharmonicity", kHarpsichordStiffness},
      {"french_C4:prompt decay", kHarpsichordPromptDecay},
      {"french_C5:brightness", kHarpsichordBrightness},
      {"french_C5:colour", kHarpsichordColour},
      {"french_C5:inharmonicity", kHarpsichordStiffness},
      {"french_Fs2:aftersound decay", kHarpsichordAftersound},
      {"french_Fs2:brightness", kHarpsichordBrightness},
      {"french_Fs2:colour", kHarpsichordColour},
      {"french_Fs2:inharmonicity", kHarpsichordStiffness},
      {"french_Fs3:brightness", kHarpsichordBrightness},
      {"french_Fs3:colour", kHarpsichordColour},
      {"french_Fs3:inharmonicity", kHarpsichordStiffness},
      {"french_Fs3:prompt decay", kHarpsichordPromptDecay},
      {"french_Fs4:aftersound decay", kHarpsichordAftersound},
      {"french_Fs4:colour", kHarpsichordColour},
      {"french_Fs4:inharmonicity", kHarpsichordStiffness},
      {"french_Fs4:prompt decay", kHarpsichordPromptDecay},
      {"french_Fs5:colour", kHarpsichordColour},
      {"clavichord_As4:brightness", kClavichordBright},
      {"clavichord_As4:colour", kClavichordBright},
      {"clavichord_As4:inharmonicity", kClavichordStiffness},
      {"clavichord_B3:brightness", kClavichordBright},
      {"clavichord_B3:inharmonicity", kClavichordStiffness},
      {"clavichord_B3:prompt decay", kClavichordPromptDecay},
      {"clavichord_C2:brightness", kClavichordBright},
      {"clavichord_C2:colour", kClavichordBright},
      {"clavichord_C2:inharmonicity", kClavichordStiffness},
      {"clavichord_C2:prompt decay", kClavichordPromptDecay},
      {"clavichord_C3:brightness", kClavichordBright},
      {"clavichord_C3:colour", kClavichordBright},
      {"clavichord_C3:inharmonicity", kClavichordStiffness},
      {"clavichord_C3:prompt decay", kClavichordPromptDecay},
      {"clavichord_C4:brightness", kClavichordBright},
      {"clavichord_C4:colour", kClavichordBright},
      {"clavichord_C4:inharmonicity", kClavichordStiffness},
      {"clavichord_C6:inharmonicity", kClavichordStiffness},
      {"clavichord_C6:pitch", kClavichordC6Sharp},
      {"clavichord_Cs4:brightness", kClavichordBright},
      {"clavichord_Cs4:colour", kClavichordBright},
      {"clavichord_Cs4:prompt decay", kClavichordPromptDecay},
      {"clavichord_D5:brightness", kClavichordBright},
      {"clavichord_D5:colour", kClavichordBright},
      {"clavichord_D5:inharmonicity", kClavichordStiffness},
      {"clavichord_D5:prompt decay", kClavichordPromptDecay},
      {"clavichord_Ds3:brightness", kClavichordBright},
      {"clavichord_Ds3:colour", kClavichordBright},
      {"clavichord_Ds3:inharmonicity", kClavichordStiffness},
      {"clavichord_Ds5:brightness", kClavichordBright},
      {"clavichord_Ds5:colour", kClavichordBright},
      {"clavichord_Ds5:inharmonicity", kClavichordStiffness},
      {"clavichord_G2:brightness", kClavichordBright},
      {"clavichord_G2:colour", kClavichordBright},
      {"clavichord_G2:inharmonicity", kClavichordStiffness},
      {"celesta_C4_ff:brightness", kCelestaC4Bright},
      {"celesta_C4_ff:decay", kCelestaRing},
      {"celesta_C4_mf:decay", kCelestaRing},
      {"celesta_C5_ff:decay", kCelestaRing},
      {"celesta_C5_mf:decay", kCelestaRing},
      {"celesta_C6_mf:decay", kCelestaRing},
      {"celesta_E4_ff:decay", kCelestaRing},
      {"celesta_E4_mf:decay", kCelestaRing},
      {"celesta_Gs4_ff:decay", kCelestaRing},
      {"celesta_Gs4_mf:decay", kCelestaRing},
      {"epiano_B3_ff:brightness", kElectricPianoBrightness},
      {"epiano_B3_ff:colour", kElectricPianoColour},
      {"epiano_B3_ff:decay", kElectricPianoRing},
      {"epiano_B3_mf:brightness", kElectricPianoBrightness},
      {"epiano_B3_mf:decay", kElectricPianoRing},
      {"epiano_B3_pp:brightness", kElectricPianoBrightness},
      {"epiano_B3_pp:colour", kElectricPianoColour},
      {"epiano_B3_pp:decay", kElectricPianoRing},
      {"epiano_C7_ff:colour", kElectricPianoColour},
      {"epiano_C7_ff:decay", kElectricPianoRing},
      {"epiano_D3_ff:brightness", kElectricPianoBrightness},
      {"epiano_D3_ff:colour", kElectricPianoColour},
      {"epiano_D3_ff:decay", kElectricPianoRing},
      {"epiano_D3_mf:brightness", kElectricPianoBrightness},
      {"epiano_D3_mf:colour", kElectricPianoColour},
      {"epiano_D3_mf:decay", kElectricPianoRing},
      {"epiano_D3_pp:colour", kElectricPianoColour},
      {"epiano_D6_ff:colour", kElectricPianoColour},
      {"epiano_E2_ff:brightness", kElectricPianoBrightness},
      {"epiano_E2_ff:colour", kElectricPianoColour},
      {"epiano_E2_ff:decay", kElectricPianoRing},
      {"epiano_E2_mf:brightness", kElectricPianoBrightness},
      {"epiano_E2_mf:colour", kElectricPianoColour},
      {"epiano_E2_pp:colour", kElectricPianoColour},
      {"epiano_E5_ff:colour", kElectricPianoColour},
      {"epiano_E5_ff:decay", kElectricPianoRing},
      {"epiano_E5_pp:colour", kElectricPianoColour},
      {"epiano_E5_pp:decay", kElectricPianoRing},
      {"epiano_F1_ff:brightness", kElectricPianoBrightness},
      {"epiano_F1_ff:colour", kElectricPianoColour},
      {"epiano_F1_ff:decay", kElectricPianoRing},
      {"epiano_F1_mf:brightness", kElectricPianoBrightness},
      {"epiano_F1_mf:colour", kElectricPianoColour},
      {"epiano_F1_pp:brightness", kElectricPianoBrightness},
      {"epiano_F1_pp:colour", kElectricPianoColour},
      {"epiano_F4_ff:brightness", kElectricPianoBrightness},
      {"epiano_F4_ff:colour", kElectricPianoColour},
      {"epiano_F4_ff:decay", kElectricPianoRing},
      {"epiano_F4_mf:decay", kElectricPianoRing},
      {"epiano_F4_pp:brightness", kElectricPianoBrightness},
      {"epiano_F4_pp:colour", kElectricPianoColour},
      {"epiano_F4_pp:decay", kElectricPianoRing},
      {"toy_A4:decay", kToyRing},
      {"toy_A4:overtone", kToyOvertone},
      {"toy_As4:overtone", kToyOvertone},
      {"toy_C5:decay", kToyRing},
      {"toy_C5:overtone", kToyOvertone},
      {"toy_D5:overtone", kToyOvertone},
      {"toy_Ds5:overtone", kToyOvertone},
      {"toy_E5:overtone", kToyOvertone},
      {"toy_F4:brightness", kToyF4Bright},
      {"toy_F4:decay", kToyRing},
      {"toy_F5:overtone", kToyOvertone},
      {"toy_G4:decay", kToyRing},
      {"toy_Gs4:overtone", kToyOvertone},
      {"organ_Fs2:brightness", kOrganF2},
  };
  return all;
}

inline bool is_known_gap(const std::string& id) {
  for (const Gap& g : known_gaps()) if (id == g.id) return true;
  return false;
}

}  // namespace reference
