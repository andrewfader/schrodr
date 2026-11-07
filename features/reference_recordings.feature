Feature: The instruments are held to real recordings
  SCHRODR's instruments are compared, note for note and marking for marking, with recordings of
  real ones: the University of Iowa Musical Instrument Samples (a Steinway at pp, mf and ff,
  orchestra bells, and a violin, viola and cello bowed without vibrato in an anechoic chamber),
  the VSCO-2 upright, VCSL's Flemish, Italian and French harpsichords and Renaissance organ, the
  University of Edinburgh's 1740s clavichord, the Sonatina celesta, the jRhodes3d Rhodes and a
  Michelsonne toy piano. Each recording is measured once by schrodr-reference-extract into
  tests/reference_data.hpp (the audio is never committed); the suite renders the same note dry
  and measures it with the same code. The bar is calibrated on the recordings: SCHRODR's note
  must come as close to the recorded note as that real instrument's own neighbouring notes come
  to each other (docs/QUALITY.md, "The reference recordings").

  A comparison not yet met is listed, with its cause, in tests/reference_gaps.hpp. A listed gap
  that closes fails the suite until it is struck off, so the bar only rises. Every bar has a 2%
  dead band, so that a number landing on its bar cannot pass on one platform and fail on another:
  a check fails only when it misses the bar by more than 2%, and a gap counts as closed only when
  it clears the bar by more than 2%.

  Executable: tests/reference_tests.cpp (CTest reference_tests).

  # the_measurements_read_known_signals_correctly
  Scenario: The measurements read known signals correctly
    Given a synthetic stiff string with B = 4e-4 that falls 20 dB/s, then 4 dB/s
    Then its B reads within 5% and its prompt and aftersound decay rates within 15%
    And an envelope is 0 dB from itself measured twice
    Given a bar at 1000 Hz with its overtone at 2.756 f1, falling 6 dB/s
    Then the overtone reads within 0.5% and the decay within 10%
    Given a bowed tone with 20 cents of vibrato at 5.5 Hz, and the same with ten times the noise power
    Then the vibrato reads 20 cents at 5.5 Hz, the bow noise rises 10 dB, and a clean tone reads under -45 dB
    Given a scale of three bowed notes with silences between them
    Then the note finder picks the middle one

  # the_aiff_reader_reads_the_recordings_format
  Scenario: The AIFF reader reads the recordings' format
    Given a 16-bit and a 24-bit stereo AIFF at 44.1 kHz with an INST chunk before the samples
    Then both read at 44100 Hz, mixed to mono with their signs and scale intact, and a WAVE file is refused

  # the_recordings_were_measured
  Scenario: The recordings were measured
    Given the measured recordings
    Then each holds a measured note within 40 cents of the note its name says
    And every instrument held to recordings has at least four of them

  # the_grand_matches_the_iowa_steinway
  Scenario: The grand matches the Iowa Steinway
    Given the grand at pp, mf and ff on B0, C2, C4, C6 and C7, dry
    Then it sounds its key, its spectral envelope and its brightness meet the bar
    And its rise from the softest marking to ff is within 5 dB of the recording's
    And its inharmonicity at the loudest marking and its prompt and aftersound decays are within a factor 1.5

  # the_glockenspiel_matches_the_iowa_orchestra_bells
  Scenario: The glockenspiel matches the Iowa orchestra bells
    Given the glockenspiel's C6, G6 and C7 at mf and ff
    Then it sounds its key, its brightness is within a factor 1.4 and its dynamics within 5 dB
    And its first overtone is within 5% and its fundamental's decay within a factor 1.5 of the bell's

  # the_violin_matches_the_iowa_violin
  Scenario: The violin matches the Iowa violin
    Given the violin bowed without vibrato on G3, B3, D4, A4, C5 and E5, each on the string the player took it on
    Then it sounds its key, its envelope, brightness and dynamics meet the bar, and its bow noise is within 6 dB

  # the_viola_matches_the_iowa_viola
  Scenario: The viola matches the Iowa viola
    Given the viola bowed without vibrato on C3, E3, G3, D4 and A4
    Then it sounds its key, its envelope, brightness and dynamics meet the bar, and its bow noise is within 6 dB

  # the_cello_matches_the_iowa_cello
  Scenario: The cello matches the Iowa cello
    Given the cello bowed without vibrato on C2, E2, G2, D3 and A3
    Then it sounds its key, its envelope, brightness and dynamics meet the bar, and its bow noise is within 6 dB

  # the_upright_matches_the_vsco_upright
  Scenario: The upright matches the VSCO-2 upright
    Given the upright at pp, mf and f on C1 to C7, dry
    Then it sounds its key, its envelope, brightness and dynamics meet the bar
    And its inharmonicity at the loudest marking and its prompt and aftersound decays are within a factor 1.5

  # the_harpsichords_match_the_vcsl_harpsichords
  Scenario: The harpsichords match VCSL's
    Given the Flemish, Italian and French harpsichords on one 8' choir, from C2 to B5
    Then each sounds its key, its envelope and brightness meet the bar
    And its inharmonicity and its prompt and aftersound decays are within a factor 1.5

  # the_clavichord_matches_the_edinburgh_clavichord
  Scenario: The clavichord matches the Edinburgh clavichord
    Given the unfretted clavichord from C2 to C6, against one of the 1740s pitched at A = 415
    Then it sounds its key, its envelope and brightness meet the bar
    And its inharmonicity and prompt decay are within a factor 1.5 (the recordings were noise-reduced, so not the aftersound)

  # the_struck_bars_match_their_recordings
  Scenario: The celesta, electric piano and toy piano match their recordings
    Given the celesta from C4 to C7, the electric piano from F1 to C7 and the toy piano from F4 to F5
    Then each sounds its key, its brightness meets the bar and its fundamental's decay is within a factor 1.5
    And the toy piano's first overtone is within 5%, and the electric piano's envelope meets the bar

  # the_organ_matches_the_vcsl_renaissance_organ
  Scenario: The organ matches VCSL's Renaissance organ
    Given the chamber organ's Gedackt 8' from F#2 to C6, against the Renaissance organ's stopped 8'
    Then it sounds its key, and its envelope and brightness meet the bar

  # every_listed_gap_is_still_a_gap
  Scenario: Every listed gap is still a gap
    Given the comparisons listed in tests/reference_gaps.hpp as not yet met
    Then none clears the bar by more than its dead band, and each names a comparison that exists
