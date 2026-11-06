Feature: The struck bars are beams, simulated
  Celesta plates, glockenspiel bars, dulcitone forks, electric piano tines and toy piano rods
  (src/dsp/bars.hpp): beams vibrating in their bending modes, struck by a mallet through their
  mode shapes, heard through a box, a tone bar's pickup or a case.

  Executable: tests/bars_tests.cpp (CTest bars_tests).

  # a_beams_modes_fall_where_euler_and_timoshenko_put_them
  Scenario: A beam's modes fall where Euler and Timoshenko put them
    Given a celesta plate and an electric piano tine designed for A4
    Then the free-free plate's modes sit at 1 : 2.76 : 5.40 of its pitch (a little under, for a thick plate), the clamped-free tine's at 1 : 6.27 : 17.5
    And the lengths the pitch asks for are a celesta plate's and a tine's
    And struck in the middle, a free-free plate's second mode (a node there) is not excited

  # every_bar_sounds_at_its_pitch
  Scenario: Every bar sounds at its pitch
    Given each of the five bar instruments struck softly at three keys of its compass
    Then each reads within 3 cents of its pitch

  # a_glockenspiel_rings_and_a_celesta_is_damped
  Scenario: A glockenspiel rings and a celesta is damped
    Given C7 on the glockenspiel and C6 on the celesta, each released after half a second
    Then the glockenspiel, with nothing to damp it, is still within 20 dB two seconds later
    And the celesta's damper stops its plate, unless the pedal holds the dampers up
    And the celesta's resonator box carries the fundamental over the plate's third mode

  # the_tine_barks_and_the_toy_piano_clanks
  Scenario: The tine barks and the toy piano clanks
    Given C4 on the electric piano struck softly and as hard as it goes
    Then the hard note's second harmonic stands at least 6 dB higher against its fundamental
    Given A4 on the toy piano
    Then its rod's second mode rings at 6.27 times the pitch, within 25 dB of it

  # a_harder_mallet_is_brighter_and_the_contact_shorter
  Scenario: A harder mallet is brighter and the contact shorter
    Given C5 on the celesta with a soft and a hard mallet
    Then the hard mallet stays on the plate for less time and its attack carries the plate's third mode (the tink) at least 10 dB stronger
