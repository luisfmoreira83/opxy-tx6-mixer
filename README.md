# TX-6 mixer (Open-XY extension)

A Teenage Engineering TX-6 in controller mode mixes the OP-XY directly, with no
computer in between, and the OP-XY gets a **tx-6** page in its settings to set
it up.

Unofficial, for OP-XY OS 1.2.1 through [Open-XY](https://github.com/etcetc000/open-xy).
Modified firmware can cause data loss or an unusable device: keep your stock
`os.ldr`.

## Connect

Connect the TX-6 to the OP-XY with a USB-C cable (the TX-6 hosts) or over
Bluetooth MIDI, put the TX-6 in controller mode, and set MIDI settings → other
to **in** or **both** on the OP-XY.

## The TX-6

| TX-6 (channel 1) | OP-XY |
|---|---|
| Faders 1–6 (CC 1–6) | Volume (CC 7) of the fader's tracks |
| Track buttons 1–6 (CC 25–30) | Mute (CC 9) of the fader's tracks, together |
| Knob rows (CC 7–24) | Each row's function, chosen on the page; shift changes the top row |
| Buttons I and II (CC 33, 34) | Previous or next scene (CC 83 / 84), chosen on the page |
| Big encoder (CC 31) | Volume over every fader, or tempo (CC 80) |
| Shift + big encoder | Tempo, or volume |

Knob functions: cutoff, resonance, env amount (filter page encoders 1–3),
engine 1 and 2 (CC 12, 13), pan, lfo 1 (CC 40) or off.

Soft takeover: after shift, a knob moves its function only once it passes the
value that function last had from the TX-6 (pan starts at the centre). Values
changed on the OP-XY itself are not seen. Volume scales only the faders moved
since power-on, and tempo starts from the middle of its range, since neither is
read from the OP-XY.

## The tx-6 page

COM → settings, below **legal**. In the style of stock's USB routing page:

- **Encoder 2:** off, or the preset in use (user1–user4). Off, the TX-6's CCs
  reach the OP-XY as they come.
- **Encoder 3:** the row. The grid's rows are the faders; below them, scrolling,
  the choices (knobs 1–3, shift knobs 1, button I, button II, encoder, shift
  encoder).
- **Encoder 4:** on the grid, the track (1–8); on a choice, its value.
- **Press 2, 3 or 4:** on the grid, that track on or off for that fader.

Each preset has its own grid (by default 1+2, 3+4, 5, 6, 7, 8); the choices are
shared. The OP-XY saves them with its own settings.

## Build

The extension needs two SDK additions that are not in Open-XY yet, on the
`tx6-mixer-staging` branch of
[luisfmoreira83/open-xy](https://github.com/luisfmoreira83/open-xy/tree/tx6-mixer-staging):

- `midi_cc_in`: an extension rewrites incoming MIDI CCs before stock acts on
  them (a hook at `0x8d58ec90`);
- `page`: an extension's own page in the settings menu, as its ninth row (a hook
  at `0x8c088240`; see `docs/prefs-pages.md` there).

```sh
git clone -b tx6-mixer-staging https://github.com/luisfmoreira83/open-xy.git
git clone https://github.com/luisfmoreira83/opxy-tx6-mixer.git tx6-mixer
cd open-xy
python sdk.py toolchain fetch
python sdk.py test --extension ../tx6-mixer
python sdk.py kit --personal --stock-os <your os.ldr> --out ../kit --extension ../tx6-mixer
```

Then open `../kit/opxy-builder.html`, tick TX-6 mixer and build your firmware.
The page's artwork, `tx6.svg`, goes to `content/display/` with the firmware.

## Files

- `mapping.h`: what each TX-6 control sends, and the page's choices;
- `extension.cpp`: the hooks and the page description;
- `tx6.svg`: the page's artwork;
- `tests/mapping.cpp`: host tests.

MIT license, as Open-XY.
