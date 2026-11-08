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
// The OP-XY tracks (0-based, so their MIDI channels) of each fader; 0xff: none.
const u8 strip_tracks[faders][2] = {{0, 1}, {2, 3}, {4, 0xff}, {5, 0xff}, {6, 0xff}, {7, 0xff}};
const u32 volume_cc = 7, mute_cc = 9, pan_cc = 10;
const u32 button_i = 33, button_ii = 34, button_shift = 35;
// Each OP-XY track's own mute: the track buttons take each strip's first
// track, I and II the second tracks of strips 1 and 2.
const u8 button_track[8] = {0, 2, 4, 5, 6, 7, 1, 3};  // CC 25-30, then I, II

// What the knobs move: a function per row, normal and while shift is held;
// the faders never change. Filter page encoders 1 and 2 are usually cutoff
// and resonance. Sends FX I and II wait for a firmware route: stock has no CC
// for them.
enum Function : u8 { none, cutoff, resonance, engine, pan, functions };
const u32 function_cc[functions] = {0, 32, 33, 12, pan_cc};
const Function layout[2][rows] = {
    {cutoff, resonance, engine},
    {pan, resonance, engine},
};

// Soft takeover: after a layer switch a knob moves its function only once it
// reaches the value that function last had from here.
const u8 unknown = 0xff;
const u32 pickup = 3;
struct State {
    bool muted[tracks];
    u8 layer;                      // 1 while shift is held.
    u8 knob[rows][faders];         // Each knob's last position, or unknown.
    u8 value[functions][faders];   // What each function last sent, or unknown.
    bool taken[rows][faders];      // The knob drives its row's function in this layer.
};
inline State initial() {
    State s = {};
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

inline u32 send(u32 strip, u32 cc, u32 value, ControlChange* out) {
    u32 n = 0;
    for (u32 t : strip_tracks[strip])
        if (t != 0xff) out[n++] = {t, cc, value};
    return n;
}

inline void set_layer(State& s, u8 layer) {
    if (layer == s.layer) return;
    s.layer = layer;
    for (auto& row : s.taken) for (bool& t : row) t = false;
}

// The control changes for one arriving from the TX-6, or pass.
inline u32 map(State& s, const ControlChange& in, ControlChange* out) {
    if (in.channel != tx6_channel || in.cc < 1 || in.cc > button_shift) return pass;
    if (in.cc <= 6) return send(in.cc - 1, volume_cc, in.value, out);
    if (in.cc <= 24) {
        const u32 row = (in.cc - 7) / faders, strip = (in.cc - 7) % faders;
        const u8 before = s.knob[row][strip];
        s.knob[row][strip] = static_cast<u8>(in.value);
        const Function f = layout[s.layer][row];
        if (f == none) return 0;
        u8& last = s.value[f][strip];
        if (!s.taken[row][strip]) {
            if (last != unknown && !reaches(before == unknown ? in.value : before, in.value, last)) return 0;
            s.taken[row][strip] = true;
        }
        last = static_cast<u8>(in.value);
        return send(strip, function_cc[f], in.value, out);
    }
    if (in.cc == button_shift) {
        set_layer(s, in.value >= 64 ? 1 : 0);
        return 0;
    }
    if ((in.cc >= 25 && in.cc <= 30) || in.cc == button_i || in.cc == button_ii) {
        if (in.value < 64) return 0;  // A release.
        const u32 t = button_track[in.cc <= 30 ? in.cc - 25 : in.cc - button_i + 6];
        s.muted[t] = !s.muted[t];
        out[0] = {t, mute_cc, s.muted[t] ? 127u : 0u};
        return 1;
    }
    return 0;  // The encoder and CC 32: nothing on the OP-XY yet.
}
}}}
