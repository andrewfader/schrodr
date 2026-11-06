Feature: The organ's flue pipes are jets playing bores, simulated
  Executable: tests/pipes_tests.cpp (CTest pipes_tests).

  # every_rank_speaks_on_its_pitch
  Scenario: Every rank speaks on its pitch
    Given each of the five ranks on C3, C4 and C5
    Then each sounds within 8 cents of the key's pitch at its footage, after the attack
    And the 4' speaks an octave above the 8', the 2' two, the quint a twelfth

  # a_stopped_pipe_sounds_the_odd_harmonics
  Scenario: A stopped pipe sounds the odd harmonics
    Given C4 on the gedackt and on the open diapason
    Then the gedackt's third harmonic stands more than 10 dB over its second; the diapason's second harmonic is within 30 dB of its fundamental
    And the stopped pipe's bore is half the open pipe's for the same pitch

  # the_pipe_speaks_with_a_chiff_and_stops_with_the_wind
  Scenario: The pipe speaks with a chiff and stops with the wind
    Given C4 on the diapason with the chiff full and with none
    Then the first 40 ms carry at least 10 dB more noise with the chiff, most of it above 3 kHz
    And the pipe reaches its level within 150 ms and is 40 dB down 150 ms after the key is released

  # the_tremulant_shakes_the_wind_and_the_stops_add_ranks
  Scenario: The tremulant shakes the wind and the stops add ranks
    Given C4 on the gedackt with the tremulant at 5 Hz
    Then its level wavers at 5 Hz
    Given C4 on the diapason alone and with the 4' drawn
    Then the octave above the key rises by more than 6 dB
    Given C4 and C#4 on the diapason, as a stereo pair
    Then C stands on one side of the chest and C# on the other

  # more_wind_is_louder_and_brighter
  Scenario: More wind is louder and brighter
    Given C4 on the diapason at 70% and 130% wind
    Then the harder wind is louder and its spectrum sits higher
