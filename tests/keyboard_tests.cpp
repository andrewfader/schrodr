// The string keyboards (src/dsp/keyboard.hpp), proved from their physics and their audio: the
// scaling's strings, every key at its tuned pitch, the tuner's stretch, the partials' stretch,
// the hammer's blow, Weinreich's coupled unisons, the harpsichord's indifference to touch and
// its registers, the clavichord's tangent and bebung, dampers and pedals, sympathetic strings,
// the hurdy-gurdy's wheel and drones, and stability with everything ringing.
// Scenarios: features/keyboard.feature.
#include "../src/dsp/keyboard.hpp"
#include "../src/engine.hpp"
#include "audio_measure.hpp"
#include "spec.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace schrodr::dsp;
using namespace schrodr::dsp::keyboard;
using measure::Span;

namespace {
constexpr double kRate = 48000.0;

Keyboard make(int model, Setup extra = Setup{}) {
  Keyboard k;
  k.prepare(kRate);
  extra.model = model;
  k.configure(extra);
  return k;
}
std::vector<float> run(Keyboard& k, double seconds) {
  std::vector<float> out(static_cast<size_t>(seconds * kRate));
  for (auto& x : out) { float l, r; k.render(l, r); x = 0.5f * (l + r); }
  return out;
}
std::vector<float> play(int model, int key, double velocity, double hold_s, double tail_s, Setup extra = Setup{}) {
  Keyboard k = make(model, extra);
  k.note_on(key, velocity, 7);
  auto out = run(k, hold_s);
  k.note_off(key);
  const auto tail = run(k, tail_s);
  out.insert(out.end(), tail.begin(), tail.end());
  return out;
}
// A tuner's reading: the strongest component within +-span cents of `near` over a window.
double peak_hz(const std::vector<float>& x, double from_s, double length_s, double near, double span = 40.0) {
  const size_t a = static_cast<size_t>(from_s * kRate), n = static_cast<size_t>(length_s * kRate);
  double best = -1.0, best_hz = near;
  for (double c = -span; c <= span; c += 0.25) {
    const double f = near * std::exp2(c / 1200.0), step = kTau * f / kRate;
    double re = 0.0, im = 0.0;
    for (size_t i = 0; i < n && a + i < x.size(); ++i) {
      const double w = 0.5 - 0.5 * std::cos(kTau * static_cast<double>(i) / static_cast<double>(n - 1));
      re += w * x[a + i] * std::cos(step * static_cast<double>(i)); im -= w * x[a + i] * std::sin(step * static_cast<double>(i));
    }
    if (re * re + im * im > best) { best = re * re + im * im; best_hz = f; }
  }
  return best_hz;
}
double cents_off(const std::vector<float>& x, double from_s, double length_s, double want, double span = 40.0) { return 1200.0 * std::log2(peak_hz(x, from_s, length_s, want, span) / want); }
double level_near(const std::vector<float>& x, double from_s, double length_s, double hz) {
  double best = 0.0;
  for (double c = -20.0; c <= 20.0; c += 2.0) best = std::max(best, measure::tone_amplitude(Span(x, static_cast<size_t>(from_s * kRate), static_cast<size_t>(length_s * kRate)), kRate, hz * std::exp2(c / 1200.0)));
  return best;
}
double rms_db(const std::vector<float>& x, double from_s, double length_s) { return measure::db(measure::rms(Span(x, static_cast<size_t>(from_s * kRate), static_cast<size_t>(length_s * kRate)))); }
bool finite(const std::vector<float>& x) { for (float v : x) if (!std::isfinite(v)) return false; return true; }
}  // namespace

SCENARIO(the_strings_follow_the_designers_scaling) {
  GIVEN("the concert grand's strings from its scaling: lengths and tensions at the anchors, music wire to 1.5 mm, wound below");
  Keyboard g = make(kGrand);
  const Spec& s = keyboard::spec(kGrand);
  THEN("middle C is 0.62 m at about 700 N on plain 1.0 mm wire, a single wound string at A0 pulls near 950 N over 2 m, and the break from plain to wound falls in the tenor");
  const Physics& c4 = g.physics(60);
  CHECK_NEAR(c4.length, 0.62, 1e-6, "C4 length");
  CHECK(std::abs(c4.tension - anchor_tension(s, 60)) < 0.05 * c4.tension && !c4.wound, ::spec::fmt("C4 %.0f N, %.2f mm plain", c4.tension, c4.core_diameter * 1e3));
  CHECK(c4.core_diameter > 0.9e-3 && c4.core_diameter < 1.15e-3, ::spec::fmt("C4 wire %.2f mm", c4.core_diameter * 1e3));
  const Physics& a0 = g.physics(21);
  CHECK(a0.wound && a0.length > 1.9 && a0.tension > 850.0 && g.courses_at(21) == 1, ::spec::fmt("A0 %.2f m, %.0f N, %.0f string(s)", a0.length, a0.tension, static_cast<double>(g.courses_at(21))));
  int break_key = -1;
  for (int key = 21; key <= 108; ++key) if (!g.physics(key).wound) { break_key = key; break; }
  CHECK(break_key >= 36 && break_key <= 48, ::spec::fmt("plain wire from key %.0f", static_cast<double>(break_key)));
  CHECK(g.courses_at(60) == 3 && g.courses_at(33) == 2, "triples in the treble, pairs in the bass");
  THEN("the inharmonicity rises from a few 1e-5 in the bass to over 1e-2 at the top, as measured on grands");
  CHECK(g.inharmonicity_at(33) < 2e-4 && g.inharmonicity_at(60) > 2e-4 && g.inharmonicity_at(60) < 7e-4 && g.inharmonicity_at(108) > 1e-2, ::spec::fmt("B: A1 %.1e, C4 %.1e, C8 %.1e", g.inharmonicity_at(33), g.inharmonicity_at(60), g.inharmonicity_at(108)));
  GIVEN("the Flemish harpsichord and the clavichord");
  Keyboard f = make(kFlemish), c = make(kFrettedClavichord);
  THEN("their strings pull a tenth and a thirtieth of the grand's, the harpsichord's 4' strings half the length of its 8'");
  CHECK(f.physics(60).tension < 0.12 * c4.tension && c.physics(60).tension < 0.05 * c4.tension, ::spec::fmt("harpsichord %.0f N, clavichord %.0f N", f.physics(60).tension, c.physics(60).tension));
  CHECK_NEAR(f.physics(60, 2).length, 0.5 * f.physics(60, 0).length, 1e-6, "4' length");
  CHECK(f.physics(36).material.density > 8000.0 && f.physics(72).material.density < 8000.0, "brass in the bass, iron in the treble");
}

SCENARIO(every_key_sounds_at_its_tuned_pitch) {
  GIVEN("the grand, the Flemish harpsichord and the unfretted clavichord, with no stretch, struck or plucked softly at six keys across the compass");
  double worst = 0.0;
  for (int model : {kGrand, kFlemish, kUnfrettedClavichord}) {
    const Spec& s = keyboard::spec(model);
    for (int key : {36, 48, 60, 72, 84, 96}) {
      if (key < s.low || key > s.high) continue;
      Setup extra; extra.stretch = 0.0; extra.unison_cents = 0.0;
      Keyboard k = make(model, extra);
      k.note_on(key, 0.1, 3);
      const auto out = run(k, 1.6);
      const double cents = cents_off(out, 0.3, 1.2, k.pitch_at(key));
      if (std::abs(cents) > 3.0) std::printf("    model %d key %d: %+.1f cents\n", model, key, cents);
      worst = std::max(worst, std::abs(cents));
    }
  }
  THEN("each reads within 3 cents of its tuned pitch on a tuner: the tuner has tuned out the soundboard's pull on the string");
  CHECK(worst <= 3.0, ::spec::fmt("worst %.1f cents", worst));
  THEN("A0, the longest string, reads within 5 cents");
  Setup extra; extra.stretch = 0.0; extra.unison_cents = 0.0;
  Keyboard k = make(kGrand, extra);
  k.note_on(21, 0.1, 3);
  const auto out = run(k, 3.0);
  CHECK(std::abs(cents_off(out, 0.5, 2.4, k.pitch_at(21))) <= 5.0, ::spec::fmt("A0 %+.1f cents", cents_off(out, 0.5, 2.4, k.pitch_at(21))));
}

SCENARIO(the_tuner_stretches_the_octaves_to_the_strings_inharmonicity) {
  GIVEN("the grand tuned with the stretch, and without");
  Setup pure; pure.stretch = 0.0;
  Keyboard stretched = make(kGrand), flat = make(kGrand, pure);
  THEN("without it every octave is 1200 cents; with it C7 sits 10 to 45 cents above three octaves over C4 (the Railsback curve) and C2 a few cents below two octaves under, while A4 and the octave around it are untouched");
  CHECK_NEAR(1200.0 * std::log2(flat.pitch_at(96) / flat.pitch_at(60)), 3600.0, 0.01, "no stretch");
  const double top = 1200.0 * std::log2(stretched.pitch_at(96) / stretched.pitch_at(60)) - 3600.0;
  const double bottom = 1200.0 * std::log2(stretched.pitch_at(36) / stretched.pitch_at(60)) + 2400.0;
  CHECK(top > 10.0 && top < 45.0, ::spec::fmt("C7 %+.1f cents over three octaves", top));
  CHECK(bottom < -1.0 && bottom > -25.0, ::spec::fmt("C2 %+.1f cents under two octaves", bottom));
  CHECK_NEAR(stretched.pitch_at(69), 440.0, 1e-6, "A4");
  THEN("each octave is tuned 4:2, the lower note's fourth partial on the upper's second: 1200 log2 (sqrt((1 + 16 B_l) / (1 + B_l)) / sqrt((1 + 4 B_u) / (1 + B_u))) cents above the temperament octave");
  const auto partial = [&](int key, int n) { const double b = stretched.inharmonicity_at(key); return std::sqrt((1.0 + b * n * n) / (1.0 + b)); };
  const double want = 1200.0 * std::log2(partial(72, 4) / partial(84, 2));
  CHECK_NEAR(1200.0 * std::log2(stretched.pitch_at(84) / stretched.pitch_at(72)) - 1200.0, want, 0.5, "C5 to C6");
  THEN("a harpsichord, with almost no inharmonicity, stretches less than 2 cents per octave");
  Keyboard h = make(kFlemish);
  CHECK(std::abs(1200.0 * std::log2(h.pitch_at(84) / h.pitch_at(72)) - 1200.0) < 2.0, "harpsichord octave");
}

SCENARIO(partials_are_stretched_by_the_strings_stiffness) {
  GIVEN("the grand's middle C struck softly, with no unison spread");
  Setup extra; extra.unison_cents = 0.0; extra.stretch = 0.0;
  Keyboard k = make(kGrand, extra);
  k.note_on(60, 0.1, 3);
  const auto out = run(k, 1.6);
  const double f1 = k.pitch_at(60), b = k.inharmonicity_at(60);
  THEN("partials 2 to 8 read within 5 cents of n f1 sqrt((1 + B n^2) / (1 + B)), the 8th well over 15 cents sharp of 8 f1");
  double worst = 0.0;
  for (int n = 2; n <= 8; ++n) {
    const double want = string::partial_hz(f1, b, n);
    worst = std::max(worst, std::abs(cents_off(out, 0.2, 1.2, want, 60.0)));
  }
  CHECK(worst <= 5.0, ::spec::fmt("worst partial %.1f cents", worst));
  CHECK(1200.0 * std::log2(peak_hz(out, 0.2, 1.2, string::partial_hz(f1, b, 8), 60.0) / (8.0 * f1)) > 15.0, "the 8th partial sharp of the harmonic");
}

SCENARIO(a_hard_blow_is_shorter_brighter_and_louder) {
  GIVEN("middle C on the grand struck pianissimo, mezzo-forte and fortissimo");
  const auto strike = [](double velocity) {
    Keyboard k = make(kGrand);
    k.note_on(60, velocity, 5);
    const auto out = run(k, 0.6);
    return std::tuple{k.last_contact_s() * 1000.0, k.last_peak_force(), measure::spectral_centroid(Span(out, 0, 24000), kRate, 12), rms_db(out, 0.02, 0.3)};
  };
  const auto [pp_contact, pp_force, pp_centroid, pp_level] = strike(0.1);
  const auto [mf_contact, mf_force, mf_centroid, mf_level] = strike(0.5);
  const auto [ff_contact, ff_force, ff_centroid, ff_level] = strike(1.0);
  THEN("the hammer stays on the string 2 to 5 ms at pianissimo and under 2 ms at fortissimo (Askenfelt and Jansson), with a peak force from a few newtons to many tens");
  CHECK(pp_contact > 2.0 && pp_contact < 5.0, ::spec::fmt("pp contact %.2f ms", pp_contact));
  CHECK(ff_contact < 2.0 && ff_contact < pp_contact, ::spec::fmt("ff contact %.2f ms", ff_contact));
  CHECK(pp_force < 15.0 && ff_force > 40.0, ::spec::fmt("peak force %.1f N pp, %.1f N ff", pp_force, ff_force));
  THEN("fortissimo is more than 20 dB louder than pianissimo and its spectrum sits higher: the felt compresses harder and stiffens");
  CHECK(ff_level - pp_level > 20.0, ::spec::fmt("%.1f dB louder", ff_level - pp_level));
  CHECK(ff_centroid > 1.3 * pp_centroid && mf_centroid > pp_centroid, ::spec::fmt("centroid pp %.0f, mf %.0f, ff %.0f Hz", pp_centroid, mf_centroid, ff_centroid));
  THEN("a hard felt (voicing) brightens the same blow");
  Setup hard; hard.hardness = 0.9;
  Keyboard k = make(kGrand, hard);
  k.note_on(60, 0.5, 5);
  const auto out = run(k, 0.6);
  CHECK(measure::spectral_centroid(Span(out, 0, 24000), kRate, 12) > 1.15 * mf_centroid, ::spec::fmt("harder felt: centroid %.0f against %.0f Hz", measure::spectral_centroid(Span(out, 0, 24000), kRate, 12), mf_centroid));
}

SCENARIO(unison_strings_couple_through_the_bridge_and_beat) {
  GIVEN("middle C's three strings on the grand, tuned as a tuner leaves them (a cent apart) and struck mezzo-forte");
  Keyboard k = make(kGrand);
  k.note_on(60, 0.5, 5);
  const auto out = run(k, 3.0);
  const double early = (rms_db(out, 0.05, 0.1) - rms_db(out, 0.35, 0.1)) / 0.3, late = (rms_db(out, 1.0, 0.2) - rms_db(out, 2.0, 0.2)) / 1.0;
  THEN("the note decays in two stages (Weinreich): the first third of a second falls more than twice as fast, in dB per second, as the second second does");
  CHECK(early > 2.0 * late && late > 0.0, ::spec::fmt("%.1f dB/s early, %.1f dB/s late", early, late));
  WHEN("the tuner leaves the unisons 6 cents apart");
  Setup wide; wide.unison_cents = 6.0;
  Keyboard w = make(kGrand, wide);
  w.note_on(60, 0.5, 5);
  const auto beating = run(w, 3.0);
  THEN("the fundamental's level wavers where the true unison decays in a line: it dips over 8 dB half a beat in (0.55 s at 0.9 Hz) and comes back by the full beat");
  const auto contour = [](const std::vector<float>& x) {
    std::vector<double> c;
    for (size_t at = 4800; at + 2400 <= x.size(); at += 480) c.push_back(measure::tone_amplitude(Span(x, at, 2400), kRate, 261.6));
    return c;
  };
  Setup true_unison; true_unison.unison_cents = 0.0;
  Keyboard tu = make(kGrand, true_unison);
  tu.note_on(60, 0.5, 5);
  const auto steady = run(tu, 3.0);
  const auto cw = contour(beating), ct = contour(steady);
  // The level's departure from a steady decay: the residual about a straight line through the
  // contour in dB (a true unison decays in a line; beating strings cross and recross it).
  const auto wobble = [](const std::vector<double>& c) {
    const size_t n = c.size(); double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (size_t i = 0; i < n; ++i) { const double y = measure::db(c[i]); sx += i; sy += y; sxx += double(i) * i; sxy += i * y; }
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx), intercept = (sy - slope * sx) / n;
    double w = 0.0; for (size_t i = 0; i < n; ++i) w += std::abs(measure::db(c[i]) - (intercept + slope * i)); return w / n;
  };
  CHECK(wobble(cw) > 2.0 * wobble(ct), ::spec::fmt("level wavers %.2f dB against %.2f", wobble(cw), wobble(ct)));
  // Half a beat in (0.55 s at 0.9 Hz) the strings have handed their energy to the mode the
  // bridge does not hear: the fundamental dips by over 8 dB and comes back by the full beat.
  const auto at = [&](double s) { return measure::db(cw[static_cast<size_t>((s - 0.1) / 0.01)]); };
  double dip = 1e9; for (double s = 0.4; s <= 0.9; s += 0.01) dip = std::min(dip, at(s));
  double back = -1e9; for (double s = 0.9; s <= 1.4; s += 0.01) back = std::max(back, at(s));
  CHECK(dip < at(0.25) - 8.0 && back > dip + 5.0, ::spec::fmt("level %.1f dB at 0.25 s, dips to %.1f, back to %.1f", at(0.25), dip, back));
}

SCENARIO(a_harpsichord_does_not_care_how_hard_the_key_is_struck) {
  GIVEN("c' on the Flemish harpsichord plucked at a tenth and at full key speed, and the same on the grand");
  const auto level = [](int model, double velocity) { const auto out = play(model, 60, velocity, 0.6, 0.1); return rms_db(out, 0.05, 0.4); };
  const double h_soft = level(kFlemish, 0.1), h_hard = level(kFlemish, 1.0), p_soft = level(kGrand, 0.1), p_hard = level(kGrand, 1.0);
  THEN("the harpsichord's two notes are within 2.5 dB of each other (the quill lets go at its own deflection), where the grand's differ by more than 20");
  CHECK(std::abs(h_hard - h_soft) < 2.5, ::spec::fmt("harpsichord %.1f dB apart", h_hard - h_soft));
  CHECK(p_hard - p_soft > 20.0, ::spec::fmt("grand %.1f dB apart", p_hard - p_soft));
  THEN("but a slow key plucks later: the sound of the tenth-speed key starts 10 to 30 ms after the full-speed one");
  Keyboard slow = make(kFlemish), fast = make(kFlemish);
  slow.note_on(60, 0.1, 5); fast.note_on(60, 1.0, 5);
  const auto s = run(slow, 0.1), f = run(fast, 0.1);
  const auto onset = [](const std::vector<float>& x) { for (size_t i = 0; i < x.size(); ++i) if (std::abs(x[i]) > 1e-3f) return static_cast<double>(i) / kRate * 1000.0; return 100.0; };
  CHECK(onset(s) - onset(f) > 10.0 && onset(s) - onset(f) < 30.0, ::spec::fmt("slow key %.1f ms later", onset(s) - onset(f)));
}

SCENARIO(the_registers_are_separate_strings_plucked_at_their_own_points) {
  GIVEN("the Flemish harpsichord's c' on the back 8' alone, with the front 8' added, with the 4', with the lute, with the buff");
  const auto with = [](bool back, bool front, bool four, bool lute, bool buff) {
    Setup s; s.back8 = back; s.front8 = front; s.four = four; s.lute = lute; s.buff = buff;
    return play(kFlemish, 60, 0.7, 1.0, 0.5, s);
  };
  const auto back = with(true, false, false, false, false), both = with(true, true, false, false, false), four = with(true, false, true, false, false);
  const auto lute = with(false, false, false, true, false), front = with(false, true, false, false, false), buff = with(true, false, false, false, true);
  THEN("two 8's are 2 to 6 dB louder than one over the attack, where a harpsichord's loudness lives (moving together they drive the board and die faster, Weinreich)");
  const double gain = rms_db(both, 0.0, 0.15) - rms_db(back, 0.0, 0.15);
  CHECK(gain > 2.0 && gain < 6.5, ::spec::fmt("two 8' %.1f dB over one", gain));
  THEN("the 4' alone sounds the octave: its tone at 2 f stands more than 20 dB over anything at f");
  const auto four_alone = with(false, false, true, false, false);
  CHECK(measure::db(level_near(four_alone, 0.1, 0.4, 523.25) / std::max(level_near(four_alone, 0.1, 0.4, 261.63), 1e-9)) > 20.0, ::spec::fmt("octave string: %.1f dB over the 8' pitch", measure::db(level_near(four_alone, 0.1, 0.4, 523.25) / std::max(level_near(four_alone, 0.1, 0.4, 261.63), 1e-9))));
  THEN("the lute stop, plucked by the nut, is nasal: its attack is brighter than the front 8's; the buff's leather shortens the ring");
  CHECK(measure::spectral_centroid(Span(lute, 0, 4096), kRate, 12) > 1.15 * measure::spectral_centroid(Span(front, 0, 4096), kRate, 12), ::spec::fmt("lute attack centroid %.0f Hz against %.0f", measure::spectral_centroid(Span(lute, 0, 4096), kRate, 12), measure::spectral_centroid(Span(front, 0, 4096), kRate, 12)));
  CHECK(rms_db(buff, 0.5, 0.3) < rms_db(back, 0.5, 0.3) - 6.0, ::spec::fmt("buff %.1f dB down after half a second", rms_db(buff, 0.5, 0.3) - rms_db(back, 0.5, 0.3)));
  THEN("the jack's damper stops the string on release: 0.3 s later it is 40 dB down");
  CHECK(rms_db(back, 1.3, 0.1) < rms_db(back, 0.8, 0.1) - 40.0, ::spec::fmt("%.1f dB down", rms_db(back, 1.3, 0.1) - rms_db(back, 0.8, 0.1)));
}

SCENARIO(a_clavichords_tangent_stretches_the_string_and_the_finger_bends_it) {
  GIVEN("c' on the unfretted clavichord pressed gently and firmly");
  Setup pure; pure.stretch = 0.0; pure.unison_cents = 0.0;
  const auto cents = [&](double velocity, double aftertouch) {
    Keyboard k = make(kUnfrettedClavichord, pure);
    k.aftertouch(aftertouch);
    k.note_on(60, velocity, 3);
    const auto out = run(k, 1.0);
    return cents_off(out, 0.15, 0.8, k.pitch_at(60), 80.0);
  };
  const double soft = cents(0.1, 0.0), firm = cents(1.0, 0.0), bebung = cents(0.1, 1.0);
  THEN("the firm stroke lifts the string further onto the tangent and sounds 3 to 20 cents sharper");
  CHECK(firm - soft > 3.0 && firm - soft < 20.0, ::spec::fmt("soft %+.1f, firm %+.1f cents", soft, firm));
  THEN("full pressure after the stroke (bebung, 12 cents) raises the pitch by that much");
  CHECK_NEAR(bebung - soft, 12.0, 3.0, "bebung");
  GIVEN("the fretted clavichord, where C# and D share a pair");
  Keyboard f = make(kFrettedClavichord);
  f.note_on(61, 0.6, 1);
  const auto before = run(f, 0.4);
  f.note_on(62, 0.6, 2);
  const auto after = run(f, 0.6);
  THEN("D's tangent takes the pair: C# is choked, 30 dB down, and D sounds");
  const double cs_before = level_near(before, 0.15, 0.2, f.pitch_at(61)), cs_after = level_near(after, 0.3, 0.2, f.pitch_at(61)), d_after = level_near(after, 0.3, 0.2, f.pitch_at(62));
  CHECK(measure::db(cs_before / std::max(cs_after, 1e-9)) > 30.0, ::spec::fmt("C# fell %.1f dB", measure::db(cs_before / std::max(cs_after, 1e-9))));
  CHECK(d_after > 10.0 * cs_after, "D sounds");
  THEN("a released clavichord string is stopped by its listing cloth almost at once: 40 dB down within 0.1 s, the rest the soundboard's own short ring");
  Keyboard u = make(kUnfrettedClavichord);
  u.note_on(60, 0.6, 1);
  const auto held = run(u, 0.5);
  u.note_off(60);
  const auto tail = run(u, 0.2);
  CHECK(rms_db(tail, 0.1, 0.08) < rms_db(held, 0.3, 0.1) - 40.0, ::spec::fmt("%.1f dB down", rms_db(tail, 0.1, 0.08) - rms_db(held, 0.3, 0.1)));
}

SCENARIO(dampers_pedals_and_sympathetic_strings) {
  GIVEN("middle C on the grand, held half a second and released with the dampers down");
  Keyboard k = make(kGrand);
  k.note_on(60, 0.7, 1);
  const auto held = run(k, 0.5);
  k.note_off(60);
  const auto tail = run(k, 1.0);
  THEN("the damper stops it: half a second later it is 40 dB down");
  CHECK(rms_db(tail, 0.5, 0.2) < rms_db(held, 0.3, 0.2) - 40.0, ::spec::fmt("%.1f dB down", rms_db(tail, 0.5, 0.2) - rms_db(held, 0.3, 0.2)));
  WHEN("the sustain pedal is down");
  Setup pedal; pedal.sustain = 1.0;
  Keyboard p = make(kGrand, pedal);
  p.note_on(60, 0.7, 1);
  run(p, 0.5);
  p.note_off(60);
  const auto ringing = run(p, 1.0);
  THEN("the string rings on: less than 20 dB down after the same time, the prompt sound's own decay (Weinreich: about 20 dB in the first second at middle C)");
  CHECK(rms_db(ringing, 0.5, 0.2) > rms_db(held, 0.3, 0.2) - 20.0, ::spec::fmt("rings on the pedal: %.1f dB down", rms_db(held, 0.3, 0.2) - rms_db(ringing, 0.5, 0.2)));
  THEN("and the strings tuned to its partials were woken to ring in sympathy: C5 and G5 sound, C#4 does not");
  CHECK(p.note_sympathetic(72) && p.note_sympathetic(79), "octave and twelfth woken");
  CHECK(!p.note_sounding(61), "the semitone left alone");
  WHEN("the sostenuto pedal goes down with C4 held, then E4 is played and both are released");
  Keyboard s = make(kGrand);
  s.note_on(60, 0.7, 1);
  run(s, 0.2);
  Setup sost; sost.sostenuto = true; s.configure(sost);
  s.note_on(64, 0.7, 2);
  run(s, 0.3);
  s.note_off(60); s.note_off(64);
  const auto after = run(s, 0.8);
  THEN("C4 rings on and E4 is damped");
  CHECK(level_near(after, 0.5, 0.2, s.pitch_at(60)) > 30.0 * level_near(after, 0.5, 0.2, s.pitch_at(64)), "sostenuto holds only the note under it");
  WHEN("the una corda shifts the action");
  Setup soft; soft.una_corda = 1.0;
  const auto shifted = play(kGrand, 60, 0.5, 0.5, 0.1, soft), plain = play(kGrand, 60, 0.5, 0.5, 0.1);
  THEN("the hammer meets two strings on softer felt: 2 to 8 dB quieter");
  const double drop = rms_db(plain, 0.05, 0.3) - rms_db(shifted, 0.05, 0.3);
  CHECK(drop > 2.0 && drop < 8.0, ::spec::fmt("una corda %.1f dB down", drop));
  WHEN("the fortepiano's moderator is on");
  Setup moderator; moderator.moderator = 1.0;
  const auto veiled = play(kFortepiano, 60, 0.5, 0.5, 0.1, moderator), bare = play(kFortepiano, 60, 0.5, 0.5, 0.1);
  THEN("the cloth veils the blow: darker and quieter");
  CHECK(measure::spectral_centroid(Span(veiled, 0, 16384), kRate, 12) < 0.8 * measure::spectral_centroid(Span(bare, 0, 16384), kRate, 12), "darker");
  CHECK(rms_db(veiled, 0.05, 0.3) < rms_db(bare, 0.05, 0.3) - 3.0, "quieter");
}

SCENARIO(the_wheel_bows_the_strings) {
  GIVEN("the hurdy-gurdy with a key held for a second and then let go");
  Keyboard h = make(kHurdyGurdy);
  h.note_on(74, 0.5, 1);
  const auto held = run(h, 1.2);
  h.note_off(74);
  const auto after = run(h, 1.5);
  THEN("the chanterelles sound the key (d'', 587 Hz) and the drones their C's underneath, all sustained while the wheel turns");
  CHECK(level_near(held, 0.5, 0.6, h.pitch_at(74)) > 1e-3, ::spec::fmt("chanterelle at %.0f Hz", h.pitch_at(74)));
  CHECK(level_near(held, 0.5, 0.6, 65.4) > 1e-3 && level_near(held, 0.5, 0.6, 261.6) > 1e-3, "drones");
  CHECK(std::abs(rms_db(held, 0.4, 0.3) - rms_db(held, 0.8, 0.3)) < 3.0, "sustained");
  THEN("with the key up the chanterelle returns to open g' while the player keeps cranking, and everything stops within a second of the last key");
  CHECK(level_near(after, 0.1, 0.3, h.pitch_at(67)) > 0.3 * level_near(held, 0.5, 0.6, h.pitch_at(74)), "open string");
  CHECK(rms_db(after, 1.3, 0.2) < rms_db(held, 0.5, 0.5) - 30.0, ::spec::fmt("%.1f dB down", rms_db(after, 1.3, 0.2) - rms_db(held, 0.5, 0.5)));
  WHEN("the trompette is set to buzz and the key is struck hard");
  Setup buzz; buzz.trompette = 1.0; buzz.wheel_turns = 1.8;
  Keyboard t = make(kHurdyGurdy, buzz);
  t.note_on(74, 1.0, 1);
  const auto rattling = run(t, 0.8);
  Setup quiet; quiet.trompette = 0.0;
  Keyboard q = make(kHurdyGurdy, quiet);
  q.note_on(74, 1.0, 1);
  const auto smooth = run(q, 0.8);
  THEN("the dog rattles: at least 6 dB more energy above 3 kHz than with the dog quiet");
  const auto high_db = [](const std::vector<float>& x) { const Span s(x, 4800, 32768); return measure::db(measure::rms(s)) + measure::db(std::sqrt(measure::band_fraction(s, kRate, 3000.0, 20000.0))); };
  CHECK(high_db(rattling) > high_db(smooth) + 6.0, ::spec::fmt("buzz: %.1f dB above 3 kHz against %.1f", high_db(rattling), high_db(smooth)));
  GIVEN("the Geigenwerk");
  Keyboard g = make(kGeigenwerk);
  g.note_on(60, 0.6, 1);
  const auto bowed = run(g, 1.0);
  g.note_off(60);
  const auto stopped = run(g, 0.6);
  THEN("a key bows its string for as long as it is down, in tune, and the string stops when it lifts off the wheel");
  CHECK(std::abs(cents_off(bowed, 0.4, 0.5, g.pitch_at(60), 60.0)) < 15.0, ::spec::fmt("%+.1f cents", cents_off(bowed, 0.4, 0.5, g.pitch_at(60), 60.0)));
  CHECK(rms_db(stopped, 0.4, 0.2) < rms_db(bowed, 0.5, 0.3) - 30.0, "stops");
}

SCENARIO(the_whole_instrument_is_stable_with_everything_ringing) {
  GIVEN("every string keyboard with the pedal down and every fourth key struck fortissimo");
  bool all_finite = true, all_decay = true;
  for (int model = 0; model < kModelCount; ++model) {
    Setup s; s.sustain = 1.0;
    Keyboard k = make(model, s);
    const Spec& g = keyboard::spec(model);
    for (int key = g.low; key <= g.high; key += 4) k.note_on(key, 1.0, static_cast<uint32_t>(key));
    const auto loud = run(k, 1.0);
    for (int key = g.low; key <= g.high; key += 4) k.note_off(key);
    Setup up; up.sustain = 0.0; up.model = model; k.configure(up);
    const auto quiet = run(k, 4.0);
    all_finite = all_finite && finite(loud) && finite(quiet);
    const double early = measure::rms(Span(loud, 0, 24000)), late = measure::rms(Span(quiet, quiet.size() - 24000, 24000));
    if (!(late < early * 0.1)) std::printf("    model %d: %.5f -> %.5f\n", model, early, late);
    all_decay = all_decay && late < early * 0.1;
    CHECK(k.sounding() <= Keyboard::kMaxSounding, ::spec::fmt("model %.0f within the budget", static_cast<double>(model)));
  }
  THEN("everything stays finite and dies away once the dampers fall");
  CHECK(all_finite, "finite");
  CHECK(all_decay, "decays");
}

SCENARIO(the_engine_plays_the_keyboard_from_the_keys_and_pedals) {
  GIVEN("the engine on the grand, a note held, released on the sustain CC, then the pedal lifted");
  schrodr::Engine engine;
  engine.prepare(kRate);
  auto s = schrodr::Settings::defaults();
  s.v[schrodr::params::kInstrument] = schrodr::params::kGrand;
  std::vector<float> l(48000), r(48000);
  engine.note_on(s, 0, 60, -1, 0.7);
  engine.render(s, l.data(), r.data(), 24000);
  engine.sustain(0, true);
  engine.note_off(s, 0, 60, -1);
  engine.render(s, l.data(), r.data(), 24000);
  THEN("the note rings on the pedal and the engine is awake");
  CHECK(!engine.silent() && measure::rms(Span(l, 0, 24000)) > 1e-4, "ringing");
  engine.sustain(0, false);
  std::vector<float> l2(96000), r2(96000);
  engine.render(s, l2.data(), r2.data(), 96000);
  THEN("with the pedal up it is damped and the engine falls silent within two seconds");
  CHECK(engine.silent(), "silent");
}

int main(int argc, char** argv) { return spec::run(argc, argv); }
