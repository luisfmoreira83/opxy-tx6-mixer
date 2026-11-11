#pragma once
#include <opxy/extension.hpp>

// The TX-6 in controller mode sends on channel 1: faders CC 1-6, knob rows
// top/middle/bottom CC 7-12 / 13-18 / 19-24, track buttons CC 25-30 (127 on
// press), the big encoder CC 31, and buttons I, II and shift CC 33-35 (127 on
// press, 0 on release). Each fader is one OP-XY strip, as the MPC Router's Mesa
// lays them out: OP 1+2, 3+4, 5, 6, 7, 8.
namespace opxy { namespace extensions { namespace tx6_mixer {
const u32 faders = 6, rows = 3, tracks = 8;
const u32 tx6_channel = 0;
// Each fader's OP-XY tracks, bit t for track t + 1 (and MIDI channel t + 1),
// until changed on the extension's settings page: 1+2, 3+4, 5, 6, 7, 8.
const u8 default_strips[faders] = {0x03, 0x0c, 0x10, 0x20, 0x40, 0x80};
const u32 volume_cc = 7, mute_cc = 9, pan_cc = 10;
// Global CCs, on any channel: tempo, previous and next scene.
const u32 tempo_cc = 80, scene_back_cc = 83, scene_next_cc = 84;
const u32 encoder_cc = 31;
const u32 button_i = 33, button_ii = 34, button_shift = 35;

// What a knob row can move, chosen on the settings page. Filter page encoders
// 1-3 are usually cutoff, resonance and envelope amount. Sends FX I and II wait
// for a firmware route: stock has no CC for them.
enum Knob : u8 { cutoff, resonance, env_amount, engine_1, engine_2, pan, lfo_1, knob_off, knob_functions };
const char* const knob_names[knob_functions] = {"cutoff", "resonance", "env amount", "engine 1",
                                                "engine 2", "pan", "lfo 1", "off"};
const u32 knob_cc[knob_functions] = {32, 33, 34, 12, 13, pan_cc, 40, 0};
enum Button : u8 { scene_back, scene_next, button_off, button_functions };
const char* const button_names[button_functions] = {"prev scene", "next scene", "off"};
enum Turn : u8 { volume, tempo, turn_off, turn_functions };
const char* const turn_names[turn_functions] = {"volume", "tempo", "off"};

// The page's choices, in its order, and their defaults.
enum Option : u8 { knobs_1, knobs_2, knobs_3, shift_knobs_1, choice_i, choice_ii, encoder, shift_encoder, choices };
const char* const choice_labels[choices] = {"knobs 1", "knobs 2", "knobs 3", "shift knobs 1",
                                            "button I", "button II", "encoder", "shift encoder"};
const u8 choice_defaults[choices] = {cutoff, resonance, engine_1, pan, scene_back, scene_next, volume, tempo};

// Soft takeover: after a layer switch a knob moves its function only once it
// reaches the value that function last had from here.
const u8 unknown = 0xff;
const u32 pickup = 3;
struct State {
    bool on;                          // The page's mode: off, the TX-6's CCs pass as they come.
    u8 strips[faders];                // The page's grid: each fader's tracks.
    u8 choice[choices];               // The page's choices.
    bool muted[faders];               // Each fader's tracks, muted together.
    u8 fader[faders];                 // Each fader's last position, or unknown.
    u8 master;                        // The big encoder's level over every fader, 0..127.
    u8 tempo;                         // What the tempo turn last sent as CC 80.
    u8 layer;                         // 1 while shift is held.
    u8 knob[rows][faders];            // Each knob's last position, or unknown.
    u8 value[knob_functions][faders]; // What each function last sent, or unknown.
    bool taken[rows][faders];         // The knob drives its row's function in this layer.
};
inline State initial() {
    State s = {};
    s.on = true;
    s.master = 127;
    s.tempo = 64;  // Not read from the OP-XY: the first turn starts here.
    for (u32 f = 0; f < faders; ++f) s.strips[f] = default_strips[f];
    for (u32 c = 0; c < choices; ++c) s.choice[c] = choice_defaults[c];
    for (u8& v : s.fader) v = unknown;
    for (auto& row : s.knob) for (u8& v : row) v = unknown;
    for (auto& f : s.value) for (u8& v : f) v = unknown;
    for (u8& v : s.value[pan]) v = 64;  // Stock's pan starts at the centre.
    return s;
}

// The knob went from `from` to `to`: did it pass `target`, or land near it?
inline bool reaches(u32 from, u32 to, u32 target) {
    const u32 low = from < to ? from : to, high = from < to ? to : from;
    return (low <= target && target <= high) || (to > target ? to - target : target - to) <= pickup;
}

// To the fader's tracks, the first cc_in_limit of them.
inline u32 send(const State& s, u32 strip, u32 cc, u32 value, ControlChange* out) {
    u32 n = 0;
    for (u32 t = 0; t < tracks && n < cc_in_limit; ++t)
        if ((s.strips[strip] >> t) & 1) out[n++] = {t, cc, value};
    return n;
}

inline void set_layer(State& s, u8 layer) {
    if (layer == s.layer) return;
    s.layer = layer;
    for (auto& row : s.taken) for (bool& t : row) t = false;
}

// A knob row's function in the layer: shift changes only the top row.
inline u32 knob_function(const State& s, u32 row) {
    const u32 c = row == 0 && s.layer ? shift_knobs_1 : knobs_1 + row;
    return s.choice[c] < knob_functions ? s.choice[c] : knob_off;
}

inline u8 clamp(i32 v) { return static_cast<u8>(v < 0 ? 0 : v > 127 ? 127 : v); }

// The control changes for one arriving from the TX-6, or pass.
inline u32 map(State& s, const ControlChange& in, ControlChange* out) {
    if (!s.on || in.channel != tx6_channel || in.cc < 1 || in.cc > button_shift) return pass;
    if (in.cc <= 6) {
        s.fader[in.cc - 1] = static_cast<u8>(in.value);
        return send(s, in.cc - 1, volume_cc, in.value * s.master / 127, out);
    }
    if (in.cc <= 24) {
        const u32 row = (in.cc - 7) / faders, strip = (in.cc - 7) % faders;
        const u8 before = s.knob[row][strip];
        s.knob[row][strip] = static_cast<u8>(in.value);
        const u32 f = knob_function(s, row);
        if (f == knob_off) return 0;
        u8& last = s.value[f][strip];
        if (!s.taken[row][strip]) {
            if (last != unknown && !reaches(before == unknown ? in.value : before, in.value, last)) return 0;
            s.taken[row][strip] = true;
        }
        last = static_cast<u8>(in.value);
        return send(s, strip, knob_cc[f], in.value, out);
    }
    if (in.cc == button_shift) {
        set_layer(s, in.value >= 64 ? 1 : 0);
        return 0;
    }
    if (in.cc >= 25 && in.cc <= 30) {
        if (in.value < 64) return 0;  // A release.
        const u32 strip = in.cc - 25;
        s.muted[strip] = !s.muted[strip];
        return send(s, strip, mute_cc, s.muted[strip] ? 127 : 0, out);
    }
    if (in.cc == button_i || in.cc == button_ii) {
        if (in.value < 64) return 0;  // A release.
        const u8 b = s.choice[in.cc == button_i ? choice_i : choice_ii];
        if (b != scene_back && b != scene_next) return 0;
        out[0] = {0, b == scene_back ? scene_back_cc : scene_next_cc, 127};
        return 1;
    }
    if (in.cc == encoder_cc) {
        // Relative: 1..63 up, 65..127 down, a click at a time.
        const i32 clicks = in.value < 64 ? static_cast<i32>(in.value) : static_cast<i32>(in.value) - 128;
        const u8 turn = s.choice[s.layer ? shift_encoder : encoder];
        if (turn == tempo) {
            s.tempo = clamp(s.tempo + clicks);
            out[0] = {0, tempo_cc, s.tempo};
            return 1;
        }
        if (turn != volume) return 0;
        s.master = clamp(s.master + clicks * 4);
        // Every fader that has moved, again at the new level, up to cc_in_limit tracks.
        u32 n = 0;
        for (u32 f = 0; f < faders; ++f)
            if (s.fader[f] != unknown)
                for (u32 t = 0; t < tracks && n < cc_in_limit; ++t)
                    if ((s.strips[f] >> t) & 1) out[n++] = {t, volume_cc, s.fader[f] * s.master / 127u};
        return n;
    }
    return 0;  // CC 32: nothing on the OP-XY.
}
}}}
