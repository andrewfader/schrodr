// The per-page artwork above the cards: the keyboard with its sounding keys and the output
// scope, the hammer's felt curve and strike point, the stop jambs, the room. Drawn from
// parameter values plus a LiveState snapshot, so the plug-in and the offscreen preview tool
// render exactly the same pictures.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../dsp/core.hpp"
#include "../dsp/keyboard.hpp"
#include "../params.hpp"
#include "editor.hpp"
#include "layout.hpp"

namespace schrodr::ui {

// How much of the output the plug-in keeps for the scope: every sample, so the scope shows the
// real waveform rather than an aliased decimation of it.
constexpr size_t kScopeSamples = 4096;

// What the artwork needs from the running instrument.
struct LiveState {
  std::array<float, kScopeSamples> scope{};  // recent output samples, oldest first
  double sample_rate = 48000.0;
  float energy = 0.0f;
  std::array<bool, 128> sounding{};
  int voices = 0;
};

// The scope's picture of the output: 20 ms starting at the latest rising zero crossing, so a
// steady note holds still, amplified so its peak fills most of the frame (up to +24 dB).
struct ScopeTrace {
  std::vector<float> points;
  double gain = 1.0;
};

inline ScopeTrace scope_trace(const std::array<float, kScopeSamples>& s, double sample_rate, int count) {
  ScopeTrace out;
  float peak = 0.0f;
  for (float x : s) peak = std::max(peak, std::abs(x));
  out.gain = peak > 1e-4f ? std::clamp(0.9 / peak, 1.0, 16.0) : 1.0;
  const size_t n = s.size();
  const size_t window = std::clamp<size_t>(static_cast<size_t>(sample_rate * 0.02), 64, n / 2);
  size_t start = n - window;
  for (size_t i = n - window; i > n - window - n / 2 && i > 0; --i)
    if (s[i - 1] < 0.0f && s[i] >= 0.0f) { start = i; break; }
  out.points.resize(static_cast<size_t>(std::max(2, count)));
  for (size_t k = 0; k < out.points.size(); ++k) {
    const double pos = static_cast<double>(start) + static_cast<double>(k) * static_cast<double>(window - 1) / static_cast<double>(out.points.size() - 1);
    const size_t i = std::min(static_cast<size_t>(pos), n - 2);
    const double frac = pos - static_cast<double>(i);
    out.points[k] = static_cast<float>(std::clamp((s[i] + (s[i + 1] - s[i]) * frac) * out.gain, -1.0, 1.0));
  }
  return out;
}

// The compass of an instrument: lowest and highest key.
inline std::pair<int, int> compass(int instrument) {
  using namespace params;
  if (is_string_instrument(instrument)) { const auto& s = dsp::keyboard::spec(instrument); return {s.low, s.high}; }
  if (is_bar_instrument(instrument)) {
    static constexpr std::array<std::pair<int, int>, kBarInstrumentCount> kBars{{{60, 108}, {79, 108}, {41, 89}, {28, 100}, {53, 89}}};
    return kBars[static_cast<size_t>(instrument - kStringInstrumentCount)];
  }
  return {36, 96};
}

class Artwork {
 public:
  Artwork(const EditorHost& host, const LiveState& live) : host_(host), live_(live) {}

  void draw(int page, Painter& g, const Rect& r) const {
    switch (page) {
      case 0: draw_keyboard(g, r); break;
      case 1: draw_hammer(g, r); break;
      case 2: draw_stops(g, r); break;
      case 4: draw_room(g, r); break;
      default: break;
    }
  }

 private:
  double value(params::ParamId id) const { return host_.value(static_cast<int>(id)); }
  int instrument() const { return static_cast<int>(value(params::kInstrument)); }

  // The keyboard across the strip, the instrument's compass in ivory and ebony, keys lit while
  // they sound; the output scope below it.
  void draw_keyboard(Painter& g, const Rect& r) const {
    using namespace theme;
    char name[96]{};
    params::format_value(params::kInstrument, value(params::kInstrument), name, sizeof(name));
    g.text(r.x + 20, r.y + 24, name, amber, 400, TextSize::Normal);
    const auto [low, high] = compass(instrument());
    char range[64]{};
    static const char* const kKeys[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};
    std::snprintf(range, sizeof(range), "%s%d to %s%d  •  %d keys", kKeys[low % 12], low / 12 - 1, kKeys[high % 12], high / 12 - 1, high - low + 1);
    g.text(r.x + 440, r.y + 24, range, muted, 400, TextSize::Small);
    const int first = 21, last = 108;  // the frame always shows a full 88, the compass lit
    int whites = 0;
    for (int k = first; k <= last; ++k) whites += !is_black(k);
    const int kx = r.x + 20, kw = r.w - 40, key_w = kw / whites, ky = r.y + 36, key_h = 72;
    int wi = 0;
    for (int k = first; k <= last; ++k) {
      if (is_black(k)) continue;
      const bool in = k >= low && k <= high, on = live_.sounding[static_cast<size_t>(k)];
      g.rect(kx + wi * key_w + 1, ky, key_w - 2, key_h, on ? amber : (in ? text : faint));
      ++wi;
    }
    wi = 0;
    for (int k = first; k <= last; ++k) {
      if (!is_black(k)) { ++wi; continue; }
      const bool in = k >= low && k <= high, on = live_.sounding[static_cast<size_t>(k)];
      g.rect(kx + wi * key_w - key_w / 3, ky, 2 * key_w / 3, key_h * 6 / 10, on ? orange : (in ? bg : border));
    }
    // The scope under the keys.
    const int top = ky + key_h + 10, mid = top + (r.bottom() - top) / 2, amp = (r.bottom() - top) / 2 - 6;
    if (amp > 6) {
      const ScopeTrace trace = scope_trace(live_.scope, live_.sample_rate, 384);
      std::vector<Point> scope;
      const int last_point = static_cast<int>(trace.points.size()) - 1;
      for (int i = 0; i <= last_point; ++i)
        scope.push_back({kx + i * kw / last_point, mid - static_cast<int>(trace.points[static_cast<size_t>(i)] * static_cast<float>(amp))});
      g.polyline(scope, soft, 2);
      char gain[48]{};
      std::snprintf(gain, sizeof(gain), "scope \xc3\x97%.1f  (+%.0f dB)", trace.gain, 20.0 * std::log10(trace.gain));
      g.text_right(r.right() - 20, r.y + 24, gain, muted, 220, TextSize::Small);
    }
  }
  static bool is_black(int key) { const int pc = key % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }

  // The hammer's felt: force against compression at the chosen hardness, and where on the
  // string it lands.
  void draw_hammer(Painter& g, const Rect& r) const {
    using namespace theme;
    const bool relevant = params::is_hammered(instrument()) || params::is_bar_instrument(instrument());
    g.text(r.x + 20, r.y + 24, "HAMMER FELT  •  force against compression", muted, 420, TextSize::Small);
    const int cx = r.x + 20, cw = 360, base = r.bottom() - 16, h = r.h - 60;
    g.line(cx, base, cx + cw, base, border);
    g.line(cx, base, cx, base - h, border);
    const double hardness = value(params::kHammerHardness);
    std::vector<Point> curve;
    for (int i = 0; i <= 100; ++i) {
      const double u = i / 100.0;
      const double p = 2.5 + 0.3 * (hardness - 0.5), k = std::pow(10.0, 2.0 * (hardness - 0.5));
      const double f = k * std::pow(u, p) / std::pow(10.0, 1.0);  // against the hardest felt at full compression
      curve.push_back({cx + i * cw / 100, base - static_cast<int>(std::clamp(f, 0.0, 1.0) * h)});
    }
    g.polyline(curve, relevant ? orange : faint, 3);
    char line[96]{};
    std::snprintf(line, sizeof(line), "p = %.2f", 2.5 + 0.3 * (hardness - 0.5));
    g.text(cx + cw + 16, base - 4, line, relevant ? soft : faint, 120, TextSize::Small);
    // The string, agraffe to bridge, with the strike point marked.
    const int sx = r.x + 560, sw = r.w - 600, sy = r.y + r.h / 2 + 10;
    g.text(sx, r.y + 24, "STRIKE POINT  •  agraffe to bridge", muted, 320, TextSize::Small);
    g.rect(sx - 6, sy - 20, 6, 40, soft);
    g.rect(sx + sw, sy - 20, 6, 40, soft);
    g.line(sx, sy, sx + sw, sy, relevant ? text : faint, 2);
    const double strike = std::clamp(0.12 * value(params::kStrikePoint), 0.02, 0.5);
    const int hx = sx + static_cast<int>(strike * sw);
    g.circle(hx - 10, sy - 36, 20, relevant ? orange : faint);
    g.rect(hx - 2, sy - 16, 4, 14, relevant ? orange : faint);
    std::snprintf(line, sizeof(line), "1/%.1f of the string", 1.0 / strike);
    g.text(hx + 16, sy - 22, line, relevant ? soft : faint, 220, TextSize::Small);
  }

  // Stop knobs on the jamb: the registers on the left, the organ's stops on the right, each
  // drawn out when on.
  void draw_stops(Painter& g, const Rect& r) const {
    using namespace theme;
    const bool harpsichord = params::is_harpsichord(instrument()), organ = params::is_organ(instrument());
    static constexpr params::ParamId kRegisters[5] = {params::kBack8, params::kFront8, params::kFour, params::kLute, params::kBuff};
    static constexpr params::ParamId kStops[5] = {params::kDiapason8, params::kGedackt8, params::kPrincipal4, params::kFifteenth2, params::kQuint};
    g.text(r.x + 20, r.y + 24, "REGISTERS", muted, 200, TextSize::Small);
    g.text(r.x + r.w / 2 + 20, r.y + 24, "STOPS", muted, 200, TextSize::Small);
    for (int side = 0; side < 2; ++side) {
      const bool relevant = side == 0 ? harpsichord : organ;
      for (int i = 0; i < 5; ++i) {
        const params::ParamId id = side == 0 ? kRegisters[i] : kStops[i];
        const bool on = value(id) >= 0.5;
        const int x = r.x + 30 + side * (r.w / 2) + i * 140, y = r.y + 44;
        g.round_rect(Rect{x, y, 120, r.h - 60}, 8, panel_hi);
        g.circle(x + 36, y + (on ? 10 : 22), 48, relevant ? (on ? green : border) : faint);
        g.text_centered(x + 60, y + r.h - 70, params::kSpecs[id].name, relevant ? (on ? text : muted) : faint, 116, TextSize::Small);
      }
    }
  }

  // The room from above: its box, the instrument, the microphones at their distance.
  void draw_room(Painter& g, const Rect& r) const {
    using namespace theme;
    const int size = std::clamp(static_cast<int>(value(params::kRoomSize)), 0, params::kRoomSizeCount - 1);
    const double volume = params::kRoomVolumes[static_cast<size_t>(size)];
    g.text(r.x + 20, r.y + 24, "THE ROOM FROM ABOVE", muted, 300, TextSize::Small);
    const double a = volume > 0.0 ? std::cbrt(volume / (1.0 * 1.4 * 1.9)) : 0.0;
    const double length = 1.9 * a, width = 1.4 * a;
    const double scale = volume > 0.0 ? std::min((r.w - 60) / length, (r.h - 60) / width) : 0.0;
    const int bx = r.x + 30, by = r.y + 40;
    const int bw = volume > 0.0 ? static_cast<int>(length * scale) : r.w - 60, bh = volume > 0.0 ? static_cast<int>(width * scale) : r.h - 60;
    g.round_rect(Rect{bx, by, bw, bh}, 6, volume > 0.0 ? panel_hi : panel);
    // The instrument a quarter of the way along; the microphones down the room.
    const int ix = bx + bw / 4, iy = by + bh * 43 / 100;
    g.circle(ix - 8, iy - 8, 16, amber);
    const double distance = value(params::kMicDistance);
    const double reach = volume > 0.0 ? std::min(distance, 0.9 * (length - length / 4) - 0.3) : distance;
    const int mx = ix + static_cast<int>((volume > 0.0 ? reach * scale : std::min(reach / 12.0, 1.0) * (bw * 0.7)));
    const bool mono = value(params::kMicSetup) >= 0.5;
    if (mono) g.circle(mx - 6, iy - 6, 12, cyan);
    else { g.circle(mx - 6, iy - 16, 12, cyan); g.circle(mx - 6, iy + 4, 12, cyan); }
    g.line(ix, iy, mx, iy, border);
    char line[160]{};
    std::snprintf(line, sizeof(line), "%s  •  %.0f m\xc2\xb3  •  microphones %.2f m away  •  %.0f%% humidity, %.0f C", params::kRoomSizeNames[size], volume, distance,
                  value(params::kHumidity) * 100.0, value(params::kTemperature));
    g.text(r.x + 330, r.y + 24, line, soft, r.w - 360, TextSize::Small);
  }

  const EditorHost& host_;
  const LiveState& live_;
};

}  // namespace schrodr::ui
