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
| **Diststoch** | working | Free walk with evolving distribution parameters (a secondary walk on the distribution shape) and asymmetric barriers (reflect/wrap/clip/randomize per bound → DC offset & even-harmonic grit). |
| Seggen | planned | A frequency sequence drives macro-pitch; each segment's waveform is a walking control-point set. |
| Chaosgen | planned | The walk's random source is a tunable chaotic map — order→noise via the bifurcation parameter. |

Planned follow-ups: selectable oversampling (anti-aliasing), buffer-backed
arbitrary templates.

## Layout

```
include/                 shared engine
  Distributions.hpp      probability distributions for the step
  BreakpointEngine.hpp   barriers, parametric template, segment interpolation
plugins/<Name>/          <Name>.{hpp,cpp,sc,schelp}
CMakeLists.txt
```

## Build

Requires a SuperCollider **source tree** for the plugin headers (a built install
is not enough), and its plugin `api_version` **must match your installed
scsynth** — otherwise the server rejects the `.scx` with "API version mismatch".
Point `SC_PATH` at a matching source tree:

```sh
cmake -S . -B build -DSC_PATH=/path/to/supercollider -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Copy each `.scx` (plus its `.sc` and `.schelp`) into your SuperCollider
`Extensions` folder (`Platform.userExtensionDir`) and recompile the class library.
