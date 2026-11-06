// Executable scenarios for features/air.feature: the room and its air against the physics
// (the speed of sound, ISO 9613-1 absorption, Sabine's time, diffuse-field level), and the
// air reaching the instrument (a wet top, warm strings).
#include "../src/dsp/air.hpp"
#include "../src/dsp/keyboard.hpp"
#include "audio_measure.hpp"
#include "spec.hpp"

#include <vector>

using namespace schrodr::dsp;
using measure::Span;

namespace {
constexpr double kRate = 48000.0;

std::vector<float> click_through(air::Room& room, size_t length, double hz = 0.0) {
  std::vector<float> out(length);
  for (size_t i = 0; i < length; ++i) {
    float l = hz > 0.0 ? static_cast<float>(0.5 * std::sin(kTau * hz * static_cast<double>(i) / kRate)) : (i == 100 ? 1.0f : 0.0f), r = l;
    room.process(l, r);
    out[i] = l;
  }
  return out;
}
}  // namespace

SCENARIO(sound_reaches_the_microphone_at_the_speed_of_sound) {
  GIVEN("a microphone 5 m from the source in the open, in air at 0 C and at 30 C");
  for (double celsius : {0.0, 30.0}) {
    air::Room room;
    room.prepare(kRate);
    air::Room::Settings s; s.distance_m = 5.0; s.celsius = celsius;
    room.configure(s);
    const auto out = click_through(room, 24000);
    size_t at = 0;
    for (size_t i = 0; i < out.size(); ++i) if (std::abs(out[i]) > 0.3f) { at = i; break; }
    THEN("the click arrives 5 / (331.3 sqrt(1 + T / 273.15)) s later, within a sample");
    CHECK_NEAR(static_cast<double>(at - 100), 5.0 / air::speed_of_sound(celsius) * kRate, 1.0, ::spec::fmt("at %.0f C", celsius));
  }
}

SCENARIO(the_air_takes_the_top_end_off_as_iso_9613_gives) {
  GIVEN("a microphone 10 m away in the open, in air at 20 C and 20% humidity");
  air::Room room;
  room.prepare(kRate);
  air::Room::Settings s; s.distance_m = 10.0; s.celsius = 20.0; s.humidity = 0.2;
  room.configure(s);
  THEN("an 8 kHz sine loses what ISO 9613-1 gives over 10 m within 1 dB, and humid air takes less");
  const auto loss = [&](double hz, double humidity) {
    air::Room r2; r2.prepare(kRate);
    air::Room::Settings s2 = s; s2.humidity = humidity; r2.configure(s2);
    const auto out = click_through(r2, 24000, hz);
    return -measure::db(measure::tone_amplitude(Span(out, 8000, 8192), kRate, hz) / 0.5);
  };
  CHECK_NEAR(loss(8000.0, 0.2), 10.0 * air::absorption_db_per_m(8000.0, 20.0, 0.2), 1.0, "8 kHz over 10 m");
  CHECK(loss(8000.0, 0.7) < loss(8000.0, 0.2), "humid air takes less");
}

SCENARIO(a_room_rings_for_its_sabine_time) {
  GIVEN("a 3000 cubic-metre hall with walls of absorption 0.2, the microphone 3 m away");
  air::Room room;
  room.prepare(kRate);
  air::Room::Settings s; s.volume_m3 = 3000.0; s.absorption = 0.2; s.distance_m = 3.0;
  room.configure(s);
  const auto out = click_through(room, static_cast<size_t>(kRate * 4.0));
  THEN("its impulse response in the 500 Hz octave decays within 25% of Sabine's time");
  const auto level = [&](double from_s) {
    double e = 0.0;
    const Span span(out, static_cast<size_t>(from_s * kRate), static_cast<size_t>(0.3 * kRate));
    for (size_t i = 0; i < span.size(); ++i) e += span[i] * span[i];
    return 10.0 * std::log10(std::max(e, 1e-20));
  };
  const double slope = (level(0.5) - level(1.5)) / 1.0;  // dB per second
  CHECK_NEAR(60.0 / slope, room.rt60_low(), 0.25 * room.rt60_low(), ::spec::fmt("T60 %.2f s against Sabine's %.2f", 60.0 / slope, room.rt60_low()));
}

SCENARIO(humid_air_lowers_a_soundboards_voice_and_warm_air_flattens_its_strings) {
  GIVEN("spruce at 30% and at 80% humidity");
  const air::WoodBody dry = air::wood_body(0.3, 20.0), wet = air::wood_body(0.8, 20.0);
  THEN("the wet top's modes sit lower and broader");
  CHECK(wet.frequency_ratio < dry.frequency_ratio, ::spec::fmt("%.3f against %.3f", wet.frequency_ratio, dry.frequency_ratio));
  CHECK(wet.q_ratio < dry.q_ratio, "damper when wet");
  GIVEN("a concert grand tuned at 20 C and played at 40 C");
  using namespace keyboard;
  const Spec& g = keyboard::spec(kGrand);
  const Physics cool = string_at(g, kGrand, 64, 0, 329.63, 20.0), warm = string_at(g, kGrand, 64, 0, 329.63, 40.0);
  THEN("the E4 string slackens by E A (alpha_steel - alpha_frame) dT, about 0.1 N of its tension per degree in all, and would read flat");
  const double expected = cool.ea * (11.7e-6 - 1.2e-5) * 20.0;
  CHECK_NEAR(cool.tension - warm.tension, expected, 1e-6, "tension lost");
  CHECK(std::abs(expected) < 0.02 * cool.tension, "the iron frame moves almost with the strings: a cast frame holds its tune");
  const Physics gut_cool = string_at(keyboard::spec(kLautenwerck), kLautenwerck, 64, 0, 329.63, 20.0), gut_warm = string_at(keyboard::spec(kLautenwerck), kLautenwerck, 64, 0, 329.63, 40.0);
  CHECK((gut_cool.tension - gut_warm.tension) / gut_cool.tension > (cool.tension - warm.tension) / cool.tension, "gut on wood goes flatter than steel on iron");
}

int main(int argc, char** argv) { return spec::run(argc, argv); }
