// Tether — Diststoch
// Free-walking stochastic breakpoint oscillator. Expressive surface: evolving
// distribution parameters (a slow secondary walk on the distribution shape) and
// asymmetric barriers (independent reflect/wrap/clip/randomize per amplitude
// bound — injects DC offset and even-harmonic grit).
#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Diststoch : public Unit {
    int m_knum;
    double* m_ampMem;
    double* m_durMem;
    double m_ampPWalk;
    double m_durPWalk;
    CPState m_main;
    double m_sr;
};

enum {
    MinFreq, MaxFreq, KNum,
    AmpDist, AmpDistP, AmpDistPWalk,
    DurDist, DurDistP, DurDistPWalk,
    AmpStep, DurStep, AmpJump, DurJump,
    AmpLo, AmpHi, BarrierLo, BarrierHi,
    Interp
};

void Diststoch_clear(Diststoch* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Diststoch_next(Diststoch* unit, int inNumSamples) {
    float* o = OUT(0);

    const double minf = ZIN0(MinFreq);
    const double maxf = ZIN0(MaxFreq);
    const int ampDist = (int)ZIN0(AmpDist);
    const double ampDistP = ZIN0(AmpDistP);
    const double ampDistPWalk = ZIN0(AmpDistPWalk);
    const int durDist = (int)ZIN0(DurDist);
    const double durDistP = ZIN0(DurDistP);
    const double durDistPWalk = ZIN0(DurDistPWalk);
    const double ampStep = ZIN0(AmpStep);
    const double durStep = ZIN0(DurStep);
    const double ampJump = t_clamp(ZIN0(AmpJump), 0.0, 1.0);
    const double durJump = t_clamp(ZIN0(DurJump), 0.0, 1.0);
    const double ampLo = ZIN0(AmpLo);
    const double ampHi = ZIN0(AmpHi);
    const int barrierLo = (int)ZIN0(BarrierLo);
    const int barrierHi = (int)ZIN0(BarrierHi);
    const int interp = (int)ZIN0(Interp);

    RGen& rgen = *unit->mParent->mRGen;
    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampMem = unit->m_ampMem;
    double* durMem = unit->m_durMem;
    double ampPWalk = unit->m_ampPWalk;
    double durPWalk = unit->m_durPWalk;
    CPState main = unit->m_main;

    for (int s = 0; s < inNumSamples; ++s) {
        if (main.remain <= 0) {
            main.index = (main.index + 1) % knum;
            const int i = main.index;
            main.curAmp = main.nextAmp;

            ampPWalk = t_mirror(ampPWalk + ampDistPWalk * (rgen.frand() * 2.0 - 1.0), -1.0, 1.0);
            durPWalk = t_mirror(durPWalk + durDistPWalk * (rgen.frand() * 2.0 - 1.0), -1.0, 1.0);
            const double effAmpP = t_mirror(ampDistP + ampPWalk, 0.0, 1.0);
            const double effDurP = t_mirror(durDistP + durPWalk, 0.0, 1.0);

            // ampJump blends the incremental walk toward an absolute redraw
            // across [ampLo,ampHi]: 0 = correlated walk, 1 = memoryless jumps.
            const double r = t_distribution(ampDist, (float)effAmpP, rgen.frand());
            const double walked = ampMem[i] + ampStep * r;
            const double absolute = 0.5 * (ampLo + ampHi) + 0.5 * (ampHi - ampLo) * r;
            double a = walked + ampJump * (absolute - walked);
            a = t_barrier(a, ampLo, ampHi, barrierLo, barrierHi, rgen.frand());
            ampMem[i] = a;
            main.nextAmp = a;

            const double rd = t_distribution(durDist, (float)effDurP, rgen.frand());
            const double dwalked = durMem[i] + durStep * rd;
            const double dabsolute = 0.5 + 0.5 * rd; // spans [0,1] when rd in [-1,1]
            double d = dwalked + durJump * (dabsolute - dwalked);
            d = t_clamp(d, 0.0, 1.0);
            durMem[i] = d;

            main.seglen = t_seglen(sr, minf, maxf, d, knum);
            main.remain = main.seglen;
        }
        o[s] = (float)t_read(main, interp);
        --main.remain;
    }

    unit->m_ampPWalk = ampPWalk;
    unit->m_durPWalk = durPWalk;
    unit->m_main = main;
}

void Diststoch_Ctor(Diststoch* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampMem || !unit->m_durMem) {
        SETCALC(Diststoch_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }

    const double lo = ZIN0(AmpLo), hi = ZIN0(AmpHi);
    for (int i = 0; i < n; ++i) {
        unit->m_ampMem[i] = t_clamp(0.5 * std::sin(2.0 * T_PI * (i + 0.5) / n), lo, hi);
        unit->m_durMem[i] = 0.5;
    }
    unit->m_ampPWalk = 0.0;
    unit->m_durPWalk = 0.0;
    t_cp_reset(unit->m_main);

    SETCALC(Diststoch_next);
    Diststoch_next(unit, 1);
}

void Diststoch_Dtor(Diststoch* unit) {
    if (unit->m_ampMem) RTFree(unit->mWorld, unit->m_ampMem);
    if (unit->m_durMem) RTFree(unit->mWorld, unit->m_durMem);
}

PluginLoad(Diststoch) {
    ft = inTable;
    DefineDtorUnit(Diststoch);
}
