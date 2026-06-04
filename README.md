# Tether

Experimental SuperCollider UGens for non-standard synthesis — oscillators built
from stochastic, segment-based, and chaotic processes rather than fixed
waveforms.

## Idea

In breakpoint-walk synthesis, pitch and noise come from the same mechanism at
two timescales: pitch is the periodic re-traversal of a set of control points;
noise is the random walk perturbing them. Tether's core move is to **tether the
walk to a live template with a restoring force**. A *stiffness* parameter sets
how tightly each control point is pulled back, so a stable pitch and a
controllable noise band can coexist — continuously morphable from tone to noise.

## UGens

| UGen | Status | Idea |
|------|--------|------|
| **Gendreve** | working | Mean-reverting walk. Stiffness = pitch↔noise dial. Dual output: signal + template-locked reference, so `out0 - out1` is the stochastic residual. |
| **Diststoch** | working | Free walk with evolving distribution parameters, asymmetric barriers (reflect/wrap/clip/randomize per bound → DC offset & even-harmonic grit), and amp/dur `jump` controls to break the walk correlation into memoryless leaps. |
| **Seggen** | working | A frequency sequence sets exact pitch one cycle at a time; each cycle's waveform is a walking control-point set. |
| **Chaosgen** | working | The walk's random source is a logistic map — `chaos` (bifurcation) morphs order→noise. |

Planned follow-ups: selectable oversampling (anti-aliasing), buffer-backed
arbitrary templates.

## Layout

```
plugins/<Name>.cpp       one .cpp per UGen (plain-C server-plugin API)
plugins/shared.hpp       distributions, barriers, template, interpolation
classes/<Name>.sc        language-side class
HelpSource/Classes/      <Name>.schelp
cmake_modules/           vendored from <SC_PATH>/tools/cmake_gen/
CMakeLists.txt           foreach over UGen names → sc_add_server_plugin
build.sh                 configure + build + install to Extensions
```

Adding a UGen = a `.cpp` + `.sc` + `.schelp` and one name in the `CMakeLists.txt`
`foreach` list.

## Build

Requires a SuperCollider **source tree** for the plugin headers, whose plugin
`api_version` **must match your installed scsynth** — otherwise the server
rejects the plugin with "API version mismatch" (3.12–3.14.x = v3,
develop/3.15-dev = v6). If your dev checkout doesn't match, add a worktree of the
right tag:

```sh
git -C /path/to/supercollider worktree add --detach ../supercollider-3.14.1 Version-3.14.1
```

Then build and install into the user Extensions folder:

```sh
./build.sh /path/to/supercollider-3.14.1
```

Afterwards, recompile the class library (Cmd/Ctrl-Shift-L) and reboot the server.
Multi-platform binaries are produced by tagging `vX.Y.Z` (see
`.github/workflows/release.yml`).
