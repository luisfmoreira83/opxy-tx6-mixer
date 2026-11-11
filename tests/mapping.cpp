#include "mapping.h"

using namespace opxy;
using namespace opxy::extensions;
using namespace opxy::extensions::tx6_mixer;

namespace {
int failures;
void check(bool ok) { failures += !ok; }
ControlChange out[cc_in_limit];
u32 run(State& s, u32 cc, u32 value, u32 channel = 0) { return map(s, {channel, cc, value}, out); }
}

int main() {
    State s = initial();
    // Fader 1: the volume of tracks 1 and 2; fader 3: track 5 alone.
    check(run(s, 1, 100) == 2 && out[0].channel == 0 && out[0].cc == 7 && out[0].value == 100 &&
          out[1].channel == 1 && out[1].cc == 7);
    check(run(s, 3, 5) == 1 && out[0].channel == 4 && out[0].cc == 7 && out[0].value == 5);
    // Knobs: top row 1 is fader 1's cutoff, middle row 2 fader 2's resonance, bottom row 6 fader 6's engine.
    check(run(s, 7, 64) == 2 && out[0].cc == 32 && out[1].channel == 1);
    check(run(s, 14, 9) == 2 && out[0].channel == 2 && out[0].cc == 33 && out[1].channel == 3);
    check(run(s, 24, 1) == 1 && out[0].channel == 7 && out[0].cc == 12 && out[0].value == 1);

    // Track buttons mute their fader's tracks together: button 1 tracks 1 and 2.
    check(run(s, 25, 127) == 2 && out[0].channel == 0 && out[1].channel == 1 && out[0].cc == 9 && out[0].value == 127);
    check(run(s, 25, 0) == 0);
    check(run(s, 25, 127) == 2 && out[1].value == 0);
    check(run(s, 27, 127) == 1 && out[0].channel == 4 && out[0].value == 127);
    // I and II: previous and next scene on presses.
    check(run(s, 33, 127) == 1 && out[0].cc == 83 && out[0].value == 127);
    check(run(s, 34, 127) == 1 && out[0].cc == 84);
    check(run(s, 34, 0) == 0);
    // The big encoder scales every moved fader: fader 1 at 100 (tracks 1, 2), fader 3 at 5 (track 5).
    check(run(s, 31, 127) == 3 && out[0].cc == 7 && out[0].value == 100 * 123 / 127 && out[2].channel == 4);
    check(run(s, 1, 100) == 2 && out[0].value == 100 * 123 / 127);
    check(run(s, 31, 1) == 3 && out[0].value == 100);

    // Holding shift: the top row is pan, which waits for the knob to reach the centre.
    check(run(s, 7, 100) == 2 && out[0].cc == 32);   // cutoff at 100
    check(run(s, 35, 127) == 0 && s.layer == 1);
    check(run(s, 7, 90) == 0);
    check(run(s, 7, 66) == 2 && out[0].cc == 10 && out[0].value == 66);  // near the centre: taken
    check(run(s, 7, 20) == 2 && out[0].cc == 10 && out[0].value == 20);  // then it follows
    check(run(s, 14, 9) == 2 && out[0].cc == 33);    // the middle row is still resonance, at 9
    check(run(s, 24, 2) == 1 && out[0].cc == 12);    // and the bottom row the engine
    // Shift + the big encoder: tempo, CC 80, from 64.
    check(run(s, 31, 2) == 1 && out[0].cc == 80 && out[0].value == 66);
    check(run(s, 31, 127) == 1 && out[0].value == 65);
    // Releasing shift: cutoff waits for the knob to pass 100 again.
    check(run(s, 35, 0) == 0 && s.layer == 0);
    check(run(s, 7, 30) == 0);
    check(run(s, 7, 110) == 2 && out[0].cc == 32 && out[0].value == 110);
    // CC 32 never reaches the OP-XY's track 1; anything else passes.
    check(run(s, 32, 127) == 0);
    check(run(s, 1, 100, 1) == pass);
    check(run(s, 64, 127) == pass);
    check(run(s, 0, 1) == pass);
    // The page's grid decides each fader's tracks: fader 3 on tracks 5 and 8.
    s.strips[2] = 0x90;
    check(run(s, 3, 40) == 2 && out[0].channel == 4 && out[1].channel == 7 && out[1].cc == 7);
    s.strips[2] = 0;
    check(run(s, 3, 40) == 0);
    // The page's choices: the top knobs on lfo 1, button I on next scene, the encoder off.
    State c = initial();
    c.choice[knobs_1] = lfo_1;
    c.choice[choice_i] = scene_next;
    c.choice[encoder] = turn_off;
    check(run(c, 7, 30) == 2 && out[0].cc == 40 && out[0].value == 30);
    check(run(c, 33, 127) == 1 && out[0].cc == 84);
    check(run(c, 31, 1) == 0);
    c.choice[knobs_2] = knob_off;
    check(run(c, 13, 50) == 0);
    // Switched off on the page, everything passes as it came.
    s.on = false;
    check(run(s, 1, 100) == pass && run(s, 25, 127) == pass);
    return failures != 0;
}
