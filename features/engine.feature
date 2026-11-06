Feature: The engine plays one instrument from the keys, pedals and controllers
  Executable: tests/engine_tests.cpp (CTest engine_tests).

  # a_note_sounds_while_held_and_is_damped_on_release
  Scenario: A note sounds while held and is damped on release
    Given the default grand, one note held for half a second with an 80 ms damper
    Then the note sounds while held, is damped on release, and the engine reports silence within two seconds

  # the_pedals_arrive_as_controllers_and_as_parameters
  Scenario: The pedals arrive as controllers and as parameters
    Given a note released on the sustain controller, then the controller lifted
    Then the note keeps sounding, and releases with it
    Given the same through the Sustain pedal parameter
    Given the soft pedal from the controller
    Then the note is quieter

  # dynamics_scales_the_touch
  Scenario: Dynamics scales the touch
    Given a pianissimo and a fortissimo note with Dynamics at full and at none
    Then at full the two differ by more than 20 dB; at none they are the same mezzo-forte

  # switching_instruments_silences_the_old_one
  Scenario: Switching instruments silences the old one
    Given a grand note ringing on the pedal, then the instrument switched to the harpsichord
    Then the grand's strings are stopped and the harpsichord plays the next note

  # every_instrument_plays_one_note_at_a_comparable_level
  Scenario: Every instrument plays one note at a comparable level
    Given one C4 at a firm touch through every instrument, master at 0 dB
    Then every one holds between -22 and -10 dBFS RMS and peaks under the ceiling

  # every_preset_meets_the_quality_bar
  Scenario: Every factory preset meets the quality bar (docs/QUALITY.md)
    Given each factory preset played the audit phrase
    Then every one is finite, within 2 dB of the target loudness, under the ceiling with the clip light off, audible and free of DC

  # the_output_limiter_holds_the_ceiling_and_leaves_quiet_signals_alone
  Scenario: The output limiter holds the ceiling and leaves quiet signals alone
    Given a fortissimo chord with the master at +12 dB, then at -24 dB
    Then the hot one peaks at the -2 dBFS ceiling with the clip light on; the quiet one, peaking under the ceiling, is not reduced at all

  # the_advertised_tail_covers_the_decay
  Scenario: The advertised tail covers the decay
    Given the grand in a hall with the pedal down, a chord released
    Then after the advertised tail the output is quiet
