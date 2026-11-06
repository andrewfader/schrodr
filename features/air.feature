Feature: The instrument is played in a room, through air
  Every instrument plays into a room (src/dsp/air.hpp) and is heard by a microphone at a
  distance: through air that absorbs the top end as ISO 9613-1 gives for its temperature and
  humidity, at the speed of sound, with the walls' reflections and a late tail decaying at the
  Sabine time. The air reaches the instrument too: a soundboard takes up its moisture, and the
  strings go flat as the room warms.

  Executable: tests/air_tests.cpp (CTest air_tests).

  # sound_reaches_the_microphone_at_the_speed_of_sound
  Scenario: Sound reaches the microphone at the speed of sound
    Given a microphone 5 m from the source in the open, in air at 0 C and at 30 C
    Then the click arrives 5 / (331.3 sqrt(1 + T / 273.15)) s later, within a sample

  # the_air_takes_the_top_end_off_as_iso_9613_gives
  Scenario: The air takes the top end off as ISO 9613-1 gives
    Given a microphone 10 m away in the open, in air at 20 C and 20% humidity
    Then an 8 kHz sine loses what ISO 9613-1 gives over 10 m within 1 dB, and humid air takes less

  # a_room_rings_for_its_sabine_time
  Scenario: A room rings for its Sabine time
    Given a 3000 cubic-metre hall with walls of absorption 0.2, the microphone 3 m away
    Then its impulse response in the 500 Hz octave decays within 25% of Sabine's time

  # humid_air_lowers_a_soundboards_voice_and_warm_air_flattens_its_strings
  Scenario: Humid air lowers a soundboard's voice and warm air flattens its strings
    Given spruce at 30% and at 80% humidity
    Then the wet top's modes sit lower and broader
    Given a concert grand tuned at 20 C and played at 40 C
    Then the E4 string slackens by E A (alpha_steel - alpha_frame) dT, and gut on wood goes flatter than steel on iron
