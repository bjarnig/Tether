# Tether tests

Hands-on listening tests for the UGens. Open a file in the SuperCollider IDE and
evaluate blocks. `Cmd/Ctrl-.` stops everything.

- **`explore.scd`** — random explorer. Evaluate the SETUP block once, then any
  UGen block to hear it with random parameters. Each run **posts the chosen
  values and a copy-paste-playable line**, so when something is interesting or
  strange you can reproduce it or note the range that produced it. Output is
  DC-blocked, limited, and auto-freed after ~5 s.

- **`sweeps.scd`** — MouseX/Y sweeps of each UGen's headline parameter(s), to
  hear a full range in one gesture.

All playback is scaled to 0.2 and passed through `Limiter`, but start at a low
monitoring level. Requires the Tether plugins installed and the class library
recompiled.
