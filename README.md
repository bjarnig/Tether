![Tether](images/scope-diststoch.png)

# Tether

Experimental SuperCollider UGens for digital synthesis. Work-in-progress.

## UGens

- **Gendreve** : mean-reverting breakpoint walk.
- **Diststoch** : free breakpoint walk; evolving distributions.
- **Cyclegen** : a frequency sequence sets pitch per cycle.
- **Chaosgen** : breakpoint values driven by a logistic map.
- **Varperiod** : live control-point count for timbral density.
- **Fracflight** : mean-reverting walk with fractional motion.
- **Grainstoch** : an emission pitch carrying walking grains.
- **Probstoch** : sparse grains on a fixed-pitch waveform.
- **Limcycle** : generalized relaxation oscillator (Van der Pol / Rayleigh / Duffing) with stochastic forcing.
- **Oscnet** : coupled-oscillator network with selectable topology and nonlinear feedback.
- **Ecostoch** : predator-prey (Lotka-Volterra) population synthesis driving FM/AM.
- **Scangen** : scanned synthesis on a dynamic mass-spring lattice.
- **Lifestoch** : ALife birth-death breakpoint population.

## Build

Needs a SuperCollider source tree whose plugin `api_version` matches your scsynth.

```sh
./build.sh /path/to/supercollider
```

Then recompile the class library and reboot the server. Tagging `vX.Y.Z` builds
multi-platform binaries via GitHub Actions.
