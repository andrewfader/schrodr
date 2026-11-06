Feature: The building blocks measure as the physics and the treatises say
  Executable: tests/dsp_tests.cpp (CTest dsp_tests).

  # temperaments_lay_out_the_octave_as_the_treatises_give
  Scenario: Temperaments lay out the octave as the treatises give
    Given the six temperaments laid out from C with A at 440 Hz
    Then equal temperament puts every note on its equal-tempered pitch
    And Pythagorean fifths are pure (702 cents) and its major third wide (408)
    And quarter-comma meantone's major third C-E is pure (386 cents) and its wolf G#-Eb howls (737)
    And Werckmeister III keeps C-E at 390 cents and no fifth narrower than 696
    And in every temperament and key, A4 stays on the fork
    When meantone is laid out from D instead of C
    Then D-F# is the pure third and the wolf moves

  # the_limiter_holds_the_ceiling_and_recovers
  Scenario: The limiter holds the ceiling and recovers
    Given the output limiter fed a sine 6 dB over the ceiling, then one 6 dB under
    Then nothing leaves above the ceiling, the meter reads the 6 dB it took, and the quiet signal passes untouched once the gain has recovered

  # a_modal_bank_is_its_resonators_summed
  Scenario: A modal bank is its resonators summed
    Given three resonators and a bank of the same three
    When both are struck with an impulse
    Then the bank's output is the sum of the three, and the 250 Hz mode rings with its Q

  # the_plucked_shape_is_a_rounded_triangle_pinned_at_both_ends
  Scenario: The plucked shape is a rounded triangle pinned at both ends
    Given a triangle peaked a fifth of the way along a 300-sample string, rounded over 12 samples
    Then it is zero at both ends, peaks near the pluck point, and has no corner sharper than the rounding allows
