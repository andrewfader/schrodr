Feature: The string keyboards are real instruments, simulated
  The string keyboards (src/dsp/keyboard.hpp): pianos, harpsichords, clavichords, the
  wheel-bowed instruments and the violin family, each a scaling of real strings over a bridge
  on a soundboard, played by hammers, plectra, tangents, a rosined wheel or a bow, with
  dampers, pedals and sympathetic strings. Each scenario is proved from the model's numbers and its audio.

  Executable: tests/keyboard_tests.cpp (CTest keyboard_tests).

  # the_strings_follow_the_designers_scaling
  Scenario: The strings follow the designer's scaling
    Given the concert grand's strings from its scaling
    Then middle C is 0.62 m at about 700 N on plain 1.0 mm wire, A0 a single wound string near 950 N over 2 m, and the break from plain to wound falls in the tenor
    And the inharmonicity rises from a few 1e-5 in the bass to over 1e-2 at the top
    Given the Flemish harpsichord and the clavichord
    Then their strings pull a tenth and a thirtieth of the grand's, the harpsichord's 4' strings half the length of its 8'

  # every_key_sounds_at_its_tuned_pitch
  Scenario: Every key sounds at its tuned pitch
    Given the grand, the Flemish harpsichord and the unfretted clavichord struck or plucked softly at six keys across the compass
    Then each reads within 3 cents of its tuned pitch on a tuner
    And A0, the longest string, reads within 5 cents

  # the_tuner_stretches_the_octaves_to_the_strings_inharmonicity
  Scenario: The tuner stretches the octaves to the strings' inharmonicity
    Given the grand tuned with the stretch, and without
    Then without it every octave is 1200 cents; with it C7 sits 10 to 45 cents above three octaves over C4 and C2 a few cents below two octaves under, while A4 is untouched
    And each octave is tuned 4:2, the lower note's fourth partial on the upper's second
    And a harpsichord stretches less than 2 cents per octave

  # partials_are_stretched_by_the_strings_stiffness
  Scenario: Partials are stretched by the string's stiffness
    Given the grand's middle C struck softly
    Then partials 2 to 8 read within 5 cents of n f1 sqrt((1 + B n^2) / (1 + B)), the 8th well over 15 cents sharp of 8 f1

  # a_hard_blow_is_shorter_brighter_and_louder
  Scenario: A hard blow is shorter, brighter and louder
    Given middle C on the grand struck pianissimo, mezzo-forte and fortissimo
    Then the hammer stays on the string 2 to 5 ms at pianissimo and under 2 ms at fortissimo, with a peak force from a few newtons to many tens
    And fortissimo is more than 20 dB louder than pianissimo and its spectrum sits higher
    And a hard felt (voicing) brightens the same blow

  # unison_strings_couple_through_the_bridge_and_beat
  Scenario: Unison strings couple through the bridge and beat
    Given middle C's three strings on the grand, tuned as a tuner leaves them (a cent apart) and struck mezzo-forte
    Then the note decays in two stages (Weinreich): the first third of a second falls more than twice as fast as the second second
    When the tuner leaves the unisons 6 cents apart
    Then the fundamental's level wavers where the true unison decays in a line, dipping over 8 dB half a beat in and coming back by the full beat

  # a_harpsichord_does_not_care_how_hard_the_key_is_struck
  Scenario: A harpsichord does not care how hard the key is struck
    Given c' on the Flemish harpsichord plucked at a tenth and at full key speed, and the same on the grand
    Then the harpsichord's two notes are within 2.5 dB of each other, where the grand's differ by more than 20
    And a slow key plucks 10 to 30 ms later

  # the_registers_are_separate_strings_plucked_at_their_own_points
  Scenario: The registers are separate strings plucked at their own points
    Given the Flemish harpsichord's c' on the back 8' alone, with the front 8' added, with the 4', with the lute, with the buff
    Then two 8's are 2 to 6 dB louder than one over the attack, where a harpsichord's loudness lives
    And the 4' alone sounds the octave: its tone at 2 f stands more than 20 dB over anything at f
    And the lute stop is brighter than the front 8'; the buff's leather shortens the ring
    And the jack's damper stops the string on release

  # a_clavichords_tangent_stretches_the_string_and_the_finger_bends_it
  Scenario: A clavichord's tangent stretches the string and the finger bends it
    Given c' on the unfretted clavichord pressed gently and firmly
    Then the firm stroke sounds 3 to 20 cents sharper
    And full pressure after the stroke (bebung, 12 cents) raises the pitch by that much
    Given the fretted clavichord, where C# and D share a pair
    Then D's tangent takes the pair: C# is choked and D sounds
    And a released clavichord string is stopped by its listing cloth almost at once

  # dampers_pedals_and_sympathetic_strings
  Scenario: Dampers, pedals and sympathetic strings
    Given middle C on the grand, held half a second and released with the dampers down
    Then the damper stops it: half a second later it is 40 dB down
    When the sustain pedal is down
    Then the string rings on, and the strings tuned to its partials were woken to ring in sympathy
    When the sostenuto pedal goes down with C4 held, then E4 is played and both are released
    Then C4 rings on and E4 is damped
    When the una corda shifts the action
    Then the hammer meets two strings on softer felt: 2 to 8 dB quieter
    When the fortepiano's moderator is on
    Then the cloth veils the blow: darker and quieter

  # the_wheel_bows_the_strings
  Scenario: The wheel bows the strings
    Given the hurdy-gurdy with a key held for a second and then let go
    Then the chanterelles sound the key and the drones their C's underneath, all sustained while the wheel turns
    And with the key up the chanterelle returns to open g', and everything stops within a second of the last key
    When the trompette is set to buzz and the key is struck hard
    Then the dog rattles: at least 6 dB more energy above 3 kHz than with the dog quiet
    Given the Geigenwerk
    Then a key bows its string for as long as it is down, in tune, and the string stops when it lifts off the wheel

  # the_bow_plays_the_violin_family
  Scenario: The bow plays the violin, viola and cello
    Given the violin, viola and cello
    Then each key is stopped on the open string a player takes it on, to the length that sounds it, at that string's tension
    And their bodies' air modes sit where the makers' do: a violin's A0 near 275 Hz, a viola's a sixth of the way lower, a cello's near 105 Hz
    When every fifth key of each is bowed for a second without vibrato
    Then each sounds its key within 10 cents, holds its level within 2 dB while bowed, and stops once the bow and finger lift
    When the violin's G3 is bowed a tenth and then a fifth of the string from the bridge
    Then the string is in Helmholtz motion: the bow slips for its own share of every period, a tenth and then a fifth of it
    When the player leans into the bow (full aftertouch), and when the key is struck hard or softly
    Then leaning draws the bow faster and harder together: at least 4 dB louder
    And a hard key sets the string going sooner than a soft one

  # the_whole_instrument_is_stable_with_everything_ringing
  Scenario: The whole instrument is stable with everything ringing
    Given every string keyboard with the pedal down and every fourth key struck fortissimo
    Then everything stays finite and dies away once the dampers fall

  # the_engine_plays_the_keyboard_from_the_keys_and_pedals
  Scenario: The engine plays the keyboard from the keys and pedals
    Given the engine on the grand, a note held, released on the sustain CC, then the pedal lifted
    Then the note rings on the pedal and the engine is awake
    And with the pedal up it is damped and the engine falls silent within two seconds
