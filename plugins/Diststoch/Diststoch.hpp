// Tether — Diststoch
// Free-walking stochastic breakpoint oscillator. Expressive surface: evolving
// distribution parameters (a slow secondary walk on the distribution shape) and
// asymmetric barriers (independent reflect/wrap/clip/randomize per amplitude
// bound — injects DC offset and even-harmonic grit).
#pragma once
#include "SC_PlugIn.hpp"
#include "BreakpointEngine.hpp"

namespace tether {

class Diststoch : public SCUnit {
public:
    Diststoch();
    ~Diststoch();

private:
    void next(int nSamples);
    void clearOutputs(int nSamples);

    enum Inputs {
        MinFreq, MaxFreq, KNum,
        AmpDist, AmpDistP, AmpDistPWalk,
        DurDist, DurDistP, DurDistPWalk,
        AmpStep, DurStep,
        AmpLo, AmpHi, BarrierLo, BarrierHi,
        Interp
    };

    int m_knum = 12;
    double* m_ampMem = nullptr;
    double* m_durMem = nullptr;
    double m_ampPWalk = 0.0;
    double m_durPWalk = 0.0;
    CPState m_main;
    double m_sr = 44100.0;
};

} // namespace tether
