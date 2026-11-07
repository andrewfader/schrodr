// The plug-in's side of the editor: an EditorHost that reads and writes Plugin parameters, the
// help text and relevance rules, and the per-page artwork. Included inside the plug-in's
// anonymous namespace so it can see Plugin. Layout, widgets and input live in editor.hpp.
#pragma once

#include <array>
#include <cmath>
#include <vector>

#include "canvas.hpp"
#include "editor.hpp"
#include "layout.hpp"
#include "pages.hpp"

// Whether a control does anything for the current instrument and settings. Irrelevant controls
// stay where they are but are dimmed, so the layout never jumps when the instrument changes.
bool gui_param_relevant(const Plugin* p, clap_id id) {
  const auto value = [p](clap_id param) { return p->params[param].load(std::memory_order_relaxed); };
  const int instrument = static_cast<int>(value(kInstrument));
  if (!schrodr::ui::param_relevant_for_instrument(static_cast<int>(id), instrument)) return false;
  switch (id) {
    case kTemperamentRoot: return value(kTemperament) > 0.0;
    case kTremulantRate: return value(kTremulant) > 0.0;
    case kRoomAbsorption: return value(kRoomSize) > 0.0;
    case kMicPattern: case kMicSetup: return value(kRoomSize) > 0.0 || value(kMicDistance) > 0.0;
    default: return true;
  }
}

const char* gui_help(clap_id id) {
  switch (id) {
    case kInstrument: return "Which instrument: a grand or upright, a fortepiano, square piano, Cristofori's first piano or a tangent piano; Flemish, Italian and French harpsichords, a virginal, a spinet, a gut-strung Lautenwerck; fretted and unfretted clavichords; a hurdy-gurdy and a Geigenwerk; celesta, glockenspiel, dulcitone, electric piano and toy piano; a chamber organ; a violin, viola and cello, each key bowing its own string.";
    case kMasterDb: return "Overall output level, before the limiter.";
    case kTranspose: return "Shifts every key by whole semitones (the instrument's compass stays where it is).";
    case kPitchA4: return "The pitch of A4: 415 Hz for baroque instruments, 430 for classical ones, 440 or 442 today.";
    case kTemperament: return "How the octave is divided: equal, Pythagorean (pure fifths), quarter-comma meantone (pure thirds, a wolf), Werckmeister III, Kirnberger III or Vallotti (the well temperaments).";
    case kTemperamentRoot: return "The key the temperament is laid out from; A always sits at the reference pitch.";
    case kStretch: return "How far the tuner stretches the octaves to the strings' own inharmonicity (the Railsback curve): none, or as a tuner does by ear.";
    case kUnisonSpread: return "How far apart the tuner leaves the strings of a unison, in cents: a touch of it is a piano's shimmer, a lot is a saloon.";
    case kDynamics: return "How much the key's velocity reaches the hammer: at none every note is mezzo-forte, at full the whole range from a whisper to a blow.";
    case kHammerHardness: return "The felt's voicing: soft and needled (round, quiet) to hard-packed (bright, loud). Mallets on the bars too.";
    case kStrikePoint: return "Where the hammer meets the string, as a fraction of its design point: nearer the agraffe is thinner, further in rounder.";
    case kPlectrum: return "What plucks a harpsichord string: a soft quill that rolls off, or hard delrin that snaps.";
    case kMechanics: return "The sounds of the machine: the key reaching its bed, dampers falling, jacks clicking, the pedal's thump.";
    case kRelease: return "How long the damper (on a violin, viola or cello the finger) takes to stop a released string, 60 dB; the bass is slower.";
    case kBebung: return "A clavichord's bebung: how far the finger's pressure (aftertouch or the mod wheel) bends the pitch.";
    case kWheelSpeed: return "How fast the hurdy-gurdy's crank turns the wheel: the bow speed.";
    case kWheelPressure: return "How hard the strings bear on the wheel: the bow pressure, from a whisper to a growl.";
    case kTrompette: return "How readily the trompette's buzzing bridge (the dog) rattles; a hard key is the coup de poignet that sets it off.";
    case kDrones: return "Whether the bourdons and trompette sound while the wheel turns.";
    case kBack8: return "The back 8' register: jacks plucking well in from the nut, the fuller row.";
    case kFront8: return "The front 8' register: jacks plucking near the nut, the brighter row.";
    case kFour: return "The 4' register: a second set of strings an octave up, over their own bridge.";
    case kLute: return "The lute stop: the front 8' plucked right by the nut, nasal and dry.";
    case kBuff: return "The buff stop: leather pads against the 8' strings by the nut, a dry, lute-like thump.";
    case kDiapason8: return "Open diapason 8': the organ's principal rank, open metal pipes, every harmonic.";
    case kGedackt8: return "Gedackt 8': stopped wooden pipes, half the length, only the odd harmonics: hollow and soft.";
    case kPrincipal4: return "Principal 4': an open rank an octave above the keys.";
    case kFifteenth2: return "Fifteenth 2': an open rank two octaves above the keys.";
    case kQuint: return "Quint 2 2/3': a rank a twelfth above the keys, for the chorus.";
    case kWind: return "Wind pressure against the voicing: more is louder, brighter and a touch sharp; too much overblows.";
    case kTremulant: return "The tremulant shakes the wind: pitch and loudness waver together.";
    case kTremulantRate: return "How fast the tremulant beats.";
    case kChiff: return "The attack's turbulence as the jet forms: the chiff of a pipe's speech.";
    case kSustain: return "The damper pedal (also CC 64): lifts every damper so released strings ring on and the others ring in sympathy; half way, the dampers graze the strings.";
    case kSostenuto: return "The sostenuto pedal (also CC 66): the keys down when it is pressed stay undamped until it comes up.";
    case kUnaCorda: return "The soft pedal (also CC 67): shifts the action so the hammer meets fewer strings on a softer part of its felt.";
    case kModerator: return "A fortepiano's moderator: a strip of cloth between hammers and strings, for a veiled pianissimo.";
    case kSympathetic: return "How much the soundboard carries one string's motion to the others: the strings tuned to a note's partials ring with it.";
    case kLid: return "The lid: closed, half on its short stick, or fully open.";
    case kStereoWidth: return "The width of the picture: the bass on one side and the treble on the other across the soundboard.";
    case kPerspective: return "The player's side (bass on the left) or the audience's (bass on the right).";
    case kRoomSize: return "The room the instrument stands in, from a booth to a church; off is the microphone on the instrument.";
    case kRoomAbsorption: return "How much the walls soak up at each reflection: hard plaster to drapes.";
    case kMicDistance: return "How far the microphones stand from the instrument, through the air.";
    case kHumidity: return "The air's humidity: it takes the top end off over distance, and swells a soundboard so its voice drops.";
    case kTemperature: return "The room's temperature: sound travels faster in warm air, and strings tuned at 20 C go flat as the room warms.";
    case kMicPattern: return "Omnidirectional, cardioid or figure-of-eight microphones; close to the instrument a directional one lifts the bass.";
    case kMicSetup: return "A stereo pair, or one microphone hearing the room in mono.";
    case kPreset: return "Loads a complete starting recipe; later edits remain fully automatable.";
    case kBowForce: return "How hard the bow presses on the string, in newtons: too little for the speed and it will not grip (a whistling surface sound), too much and it grinds. A cellist leans harder than a violinist.";
    case kBowSpeed: return "How fast the bow is drawn: faster is louder. Key velocity and pressure (aftertouch or the mod wheel) draw it faster and press harder together.";
    case kBowPosition: return "Where the bow meets the string, as a fraction of it from the bridge. Nearer the bridge the string swings wider for the same stroke and needs more force to grip; further out less force will do.";
    case kVibrato: return "The left hand's vibrato: how far the finger rolls the pitch either side of the note, in cents. It comes in once the note has spoken.";
    case kVibratoRate: return "How fast the vibrato swings, in cycles a second; every note's own rate varies a little.";
    default: return "Adjusts this part of the current sound; changes are immediately audible and automatable.";
  }
}

void gui_request_flush(Plugin* p) {
  if (p->host && p->host->get_extension)
    if (const auto* hp = static_cast<const clap_host_params_t*>(p->host->get_extension(p->host, CLAP_EXT_PARAMS)))
      if (hp->request_flush) hp->request_flush(p->host);
}

void gui_mark_state(Plugin* p) {
  if (p->host && p->host->get_extension)
    if (const auto* hs = static_cast<const clap_host_state_t*>(p->host->get_extension(p->host, CLAP_EXT_STATE)))
      if (hs->mark_dirty) hs->mark_dirty(p->host);
}

class PluginEditorHost final : public schrodr::ui::EditorHost {
 public:
  explicit PluginEditorHost(Plugin* p) : p_(p) {}

  int param_count() const override { return static_cast<int>(kParamCount); }
  schrodr::ui::ParamInfo info(int id) const override {
    const auto& s = kSpecs[static_cast<size_t>(id)];
    return {s.name, s.min, s.max, s.def, s.stepped};
  }
  double value(int id) const override { return p_->params[static_cast<size_t>(id)].load(std::memory_order_relaxed); }
  void begin_edit(int id) override { gui_push(p_, GuiEvent::Begin, static_cast<clap_id>(id), 0); gui_request_flush(p_); }
  void edit(int id, double v) override {
    if (static_cast<clap_id>(id) == kPreset) apply_preset(p_, static_cast<int>(std::lround(v)));
    else set_param(p_, static_cast<clap_id>(id), v);
    snapshot_settings(p_);
    gui_push(p_, GuiEvent::Value, static_cast<clap_id>(id), value(id));
    gui_request_flush(p_);
  }
  void end_edit(int id) override { gui_push(p_, GuiEvent::End, static_cast<clap_id>(id), 0); gui_request_flush(p_); gui_mark_state(p_); }
  void format(int id, double v, char* out, size_t capacity) const override {
    format_value(static_cast<uint32_t>(id), v, out, static_cast<uint32_t>(capacity));
  }
  const char* help(int id) const override { return gui_help(static_cast<clap_id>(id)); }
  const char* short_label(int id) const override {
    if (const char* name = schrodr::ui::schrodr_short_name(id)) return name;
    return label(id);
  }
  bool relevant(int id) const override { return gui_param_relevant(p_, static_cast<clap_id>(id)); }

  int size_percent() const override {
    return static_cast<int>(std::lround(100.0 * schrodr::ui::uniform_scale(static_cast<int>(p_->gui_width), static_cast<int>(p_->gui_height))));
  }
  void request_size(int percent) override {
    p_->gui_request_width = static_cast<uint32_t>(schrodr::ui::width * percent / 100);
    p_->gui_request_height = static_cast<uint32_t>(schrodr::ui::height * percent / 100);
  }

  const char* strip_help(int page) const override {
    if (page == 0) return "The keyboard, its compass in ivory, keys lit while their strings sound; the output scope held still on each cycle and amplified to fill the frame (gain shown).";
    if (page == 1) return "The felt's force curve at the chosen hardness, and where the hammer lands on the string.";
    if (page == 2) return "The stop jambs: registers on the left, the organ's stops on the right, drawn when on.";
    if (page == 4) return "The room from above: the instrument, the microphones at their distance.";
    return nullptr;
  }

  void draw_meters(schrodr::ui::Painter& g, const schrodr::ui::Rect& r) override {
    using namespace schrodr::ui::theme;
    const float peak_l = p_->peak_l.load(std::memory_order_relaxed), peak_r = p_->peak_r.load(std::memory_order_relaxed);
    const bool clipped = p_->clipped.load(std::memory_order_relaxed);
    g.text(r.x, r.y + 24, "OUT", muted, 50, schrodr::ui::TextSize::Small);
    const int bar_x = r.x + 52, bar_w = 230;
    g.rect(bar_x, r.y + 6, bar_w, 9, border);
    g.rect(bar_x, r.y + 20, bar_w, 9, border);
    const uint32_t level = clipped ? red : green;
    g.rect(bar_x, r.y + 6, static_cast<int>(bar_w * std::clamp(peak_l, 0.0f, 1.0f)), 9, level);
    g.rect(bar_x, r.y + 20, static_cast<int>(bar_w * std::clamp(peak_r, 0.0f, 1.0f)), 9, level);
    char voices[32]{};
    std::snprintf(voices, sizeof(voices), clipped ? "CLIP" : "%d SOUNDING", p_->active_voices.load(std::memory_order_relaxed));
    g.text(bar_x + bar_w + 18, r.y + 24, voices, clipped ? red : muted, r.right() - bar_x - bar_w - 18, schrodr::ui::TextSize::Small);
  }

  void draw_strip(int page, schrodr::ui::Painter& g, const schrodr::ui::Rect& r) override {
    schrodr::ui::LiveState live;
    const uint32_t write = p_->scope_write.load(std::memory_order_acquire);
    float energy = 0.0f;
    for (size_t i = 0; i < live.scope.size(); ++i) {
      live.scope[i] = p_->scope[(write + static_cast<uint32_t>(i)) & (Plugin::kScopeSize - 1)].load(std::memory_order_relaxed);
      energy += live.scope[i] * live.scope[i];
    }
    live.energy = std::sqrt(energy / static_cast<float>(live.scope.size()));
    live.sample_rate = p_->sample_rate;
    for (int k = 0; k < 128; ++k) live.sounding[static_cast<size_t>(k)] = p_->sounding[static_cast<size_t>(k)].load(std::memory_order_relaxed);
    live.voices = p_->active_voices.load(std::memory_order_relaxed);
    schrodr::ui::Artwork(*this, live).draw(page, g, r);
  }

  bool strip_pointer(int page, int button, int x, int y, const schrodr::ui::Rect& r) override {
    (void)page; (void)button; (void)x; (void)y; (void)r;
    return false;
  }

 private:
  Plugin* p_;
};
static_assert(Plugin::kScopeSize == schrodr::ui::kScopeSamples, "the scope ring and the artwork agree on its length");

schrodr::ui::Editor& gui_editor(Plugin* p) {
  if (!p->editor) {
    p->editor_host = std::make_unique<PluginEditorHost>(p);
    p->editor = std::make_unique<schrodr::ui::Editor>(*p->editor_host, schrodr::ui::schrodr_pages(), schrodr::ui::schrodr_header_control());
  }
  return *p->editor;
}

void gui_draw(Plugin* p, schrodr::ui::Canvas& canvas) { gui_editor(p).draw(canvas); }

using GuiPointer = schrodr::ui::PointerAction;

bool gui_set_size(const clap_plugin_t* plugin, uint32_t w, uint32_t h);

// Asks the host to resize its editor frame; only once it agrees does the editor window follow.
void gui_apply_requested_size(Plugin* p) {
  const uint32_t w = p->gui_request_width, h = p->gui_request_height;
  p->gui_request_width = p->gui_request_height = 0;
  if (!w || !h || (w == p->gui_width && h == p->gui_height) || !p->host || !p->host->get_extension) return;
  const auto* host_gui = static_cast<const clap_host_gui_t*>(p->host->get_extension(p->host, CLAP_EXT_GUI));
  if (!host_gui || !host_gui->request_resize || !host_gui->request_resize(p->host, w, h)) return;
  gui_set_size(&p->api, w, h);
  gui_mark_state(p);
}

void gui_input(Plugin* p, GuiPointer action, int button, int x, int y, unsigned modifiers = 0) {
  gui_editor(p).pointer(action, button, x, y, modifiers, schrodr::ui::now_ms());
  gui_apply_requested_size(p);
}

// True when the editor needs a repaint for its own animations.
bool gui_tick(Plugin* p) { return gui_editor(p).tick(schrodr::ui::now_ms()); }

[[maybe_unused]] int gui_size_percent_for_test(Plugin* p) { gui_editor(p); return p->editor_host->size_percent(); }
