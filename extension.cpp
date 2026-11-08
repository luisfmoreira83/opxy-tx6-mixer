#include <opxy/extension.hpp>
#include "mapping.h"

namespace opxy { namespace extensions { namespace tx6_mixer {
namespace {
// Set on first use: the module has no static constructors to run initial().
State state;
bool started;
u32 midi_cc_in(const ControlChange& in, ControlChange* out) {
    if (!started) {
        state = initial();
        started = true;
    }
    return map(state, in, out);
}
}
extern constexpr Hooks hooks = Hooks().with_midi_cc_in(midi_cc_in);
}}}
