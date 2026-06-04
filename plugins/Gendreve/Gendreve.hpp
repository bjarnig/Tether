// Tether — Gendreve
// Breakpoint oscillator whose control points random-walk but are pulled back
// toward a live template. Stiffness sets the pitch<->noise balance. Output 0 is
// the signal, output 1 a template-locked reference (out0 - out1 = residual).
#pragma once
#include "SC_PlugIn.hpp"
#include "BreakpointEngine.hpp"

namespace tether {

class Gendreve : public SCUnit {
public:
    Gendreve();
    ~Gendreve();

private:
    void next(int nSamples);
    void clearOutputs(int nSamples);

    enum Inputs {
        MinFreq, MaxFreq, KNum, AmpStiff, DurStiff, AmpNoise, DurNoise,
        AmpDist, AmpDistP, DurDist, DurDistP, TemplateType, TemplateSkew,
        CenterDur, Interp
    };

    int m_knum = 12;
    double* m_ampMem = nullptr;
    double* m_durMem = nullptr;
    CPState m_main;
    CPState m_ref;
    double m_sr = 44100.0;
    bool m_ok = false;
};

} // namespace tether
