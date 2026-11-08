# TX-6 mixer (Open-XY extension)

A Teenage Engineering TX-6 in controller mode mixes the OP-XY directly, with no
computer or router in between. Connect them with a USB-C cable (the TX-6 hosts)
or over Bluetooth MIDI, and set MIDI settings → other to **in** or **both** on
the OP-XY.

| TX-6 (channel 1) | OP-XY |
|---|---|
| Faders 1–6 (CC 1–6) | Track volume (CC 7) of the fader's tracks, in every layer |
| Track buttons 1–6 (CC 25–30) | Mute toggle (CC 9) of tracks 1, 3, 5, 6, 7, 8 |
| Buttons I and II (CC 33, 34) | Mute toggle of tracks 2 and 4 |
| Encoder (CC 31) | Nothing yet |

The knobs have two layers: normal and **while shift is held** (CC 35).

| Knob row | Normal | Holding shift |
|---|---|---|
| Top (CC 7–12) | Filter encoder 1 (CC 32, usually cutoff) | Pan (CC 10) |
| Middle (CC 13–18) | Filter encoder 2 (CC 33, usually resonance) | Same as normal |
| Bottom (CC 19–24) | Engine control 1 (CC 12) | Same as normal |

Sends FX I and II have no CC in stock, so they wait for a firmware route.

Soft takeover: after a layer change, a knob moves its function only once it
passes the value that function last had from the TX-6 (pan starts at the
centre). Values changed on the OP-XY itself are not seen, so a knob can wait
for an old value.

Each fader is one strip of OP-XY tracks, as the MPC Router's Mesa lays them
out: 1+2, 3+4, 5, 6, 7, 8 (`strip_tracks` in `mapping.h`). Every other CC, and
every other channel, reaches the OP-XY unchanged.

It needs the SDK's `midi_cc_in` event (an incoming-CC hook at `0x8d58ec90`,
OP-XY 1.2.1), which is not in Open-XY yet: build with the local SDK branch
`tx6-mixer-staging` (`../open-xy-tx6s`). While the extension is in your firmware, CCs 1–31 on channel 1
belong to the TX-6 (CCs 1–35). Mutes are remembered by the extension, so a track muted on
the OP-XY itself takes one extra press on the TX-6.

Test: `python ../open-xy-tx6s/sdk.py test --extension .`
