Feature: One string is simulated from its physics
  A string (src/dsp/string.hpp) is a waveguide whose losses and stiffness are fitted to the wire's
  physics; a force at a point launches the waves a real string would carry.

  Executable: tests/string_tests.cpp (CTest string_tests).

  # a_strings_tension_and_stiffness_follow_from_its_wire_and_pitch
  Scenario: A string's tension and stiffness follow from its wire and pitch
    Given middle C's string on a concert grand: 0.62 m of 1.0 mm music wire at 261.6 Hz
    Then its tension is mu (2 L f)^2 within the stiffness term's percent, about 650 N, and its impedance sqrt(T mu)
    And its inharmonicity B = pi^2 E I / (T L^2) is a few times 1e-4, as measured on grands
    And its 8th partial sits sharp by the stiff string's stretch
    Given a harpsichord's c'' string: 0.36 m of 0.25 mm iron at 523 Hz
    Then it pulls about 55 N, a tenth of the piano's, and its B is a few times 1e-5

  # a_strings_own_losses_rise_with_frequency
  Scenario: A string's own losses rise with frequency
    Given the grand's C4 string (plain) and A1 string (wound) from their physics
    Then the decay rate of the 8th partial exceeds the fundamental's on both, the wound string's by the more
    And a buff stop's leather adds loss in proportion to frequency

  # the_loss_fit_makes_a_loop_decay_at_the_strings_rates
  Scenario: The loss fit makes a loop decay at the string's rates
    Given a 220 Hz loop whose loss filter is fitted to decay rates rising from 1.5 to 12 s^-1
    Then the fundamental's decay rate comes out within 15% and the 8th partial's within 30%
    And no frequency comes round with more than it went in

  # the_dispersion_fit_puts_the_partials_where_the_stiff_string_has_them
  Scenario: The dispersion fit puts the partials where the stiff string has them
    Given loops at 261.6 Hz with B = 4e-4 and at 2093 Hz with B = 7e-3
    Then partials 2 to 6 land within tolerance of n f1 sqrt((1 + B n^2) / (1 + B))
    And the fundamental itself is tuned by the delay, within a cent
    And a string with no stiffness to speak of gets no allpasses

  # a_force_at_a_point_launches_a_plateau_both_ways
  Scenario: A force at a point launches a plateau both ways
    Given a 300-sample string at rest, pushed once at 40 cells from the nut by a displacement of 1, laid to land on the point at the next sample
    When one sample passes
    Then the displacement at the point reads 1, its neighbours 0, and 20 cells toward the bridge 0
    When 20 more samples pass
    Then the plateau has spread 20 cells each way: the point still reads 1, 60 cells from the nut reads 1, 61 reads 0
    Given the same string with a viscous damper of the string's own impedance at 73 cells, plucked
    Then twelve periods on, the damper has taken most of the energy: the model is passive at a point