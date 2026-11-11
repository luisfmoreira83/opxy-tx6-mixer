#include <opxy/extension.hpp>
#include "mapping.h"

namespace opxy { namespace extensions { namespace tx6_mixer {
namespace {
// Set on first use: the module has no static constructors to run initial().
State state;
bool started;
State& current() {
    if (!started) {
        state = initial();
        started = true;
    }
    return state;
}
u32 midi_cc_in(const ControlChange& in, ControlChange* out) { return map(current(), in, out); }
// The settings page edits the faders' tracks, the mode and the choices in place.
Choice listed[choices];
Page grid;
const Page* page() {
    State& s = current();
    for (u32 c = 0; c < choices; ++c) {
        const bool knob = c <= shift_knobs_1, turn = c >= encoder;
        const u32 count = knob ? u32(knob_functions) : turn ? u32(turn_functions) : u32(button_functions);
        listed[c] = Choice{choice_labels[c], knob ? knob_names : turn ? turn_names : button_names, count, &s.choice[c]};
    }
    grid = Page{"tx-6", "tx6.svg", faders, tracks, s.strips, &s.on, listed, choices};
    return &grid;
}
}
extern constexpr Hooks hooks = Hooks().with_midi_cc_in(midi_cc_in).with_page(page);
}}}
