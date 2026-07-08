#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Diststoch : public Unit {
    int m_knum;
    double* m_ampP;
    double* m_ampTh;
    double* m_durP;
    double* m_durTh;
    CPState m_main;
    double m_sr;
};

enum {
    MinFreq, MaxFreq, KNum,
    AmpDist, AmpDistP, DurDist, DurDistP,
    AmpKick, DurKick,
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
    const int durDist = (int)ZIN0(DurDist);
    const double durDistP = ZIN0(DurDistP);
    const double ampKick = ZIN0(AmpKick);
    const double durKick = ZIN0(DurKick);
    const double ampLo = ZIN0(AmpLo);
    const double ampHi = ZIN0(AmpHi);
    const int barrierLo = (int)ZIN0(BarrierLo);
    const int barrierHi = (int)ZIN0(BarrierHi);
    const int interp = (int)ZIN0(Interp);

    RGen& rgen = *unit->mParent->mRGen;
    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampP = unit->m_ampP;
    double* ampTh = unit->m_ampTh;
    double* durP = unit->m_durP;
    double* durTh = unit->m_durTh;
    CPState main = unit->m_main;

    // Irrational angle scale: keeps the barrier fold off the low-order resonances
    // that otherwise open period-2 windows at ordinary kick values.
    const double ampRange = ampHi - ampLo;
    const double ampScale = (ampRange > 1e-9) ? (2.0 * T_PI * T_PHI / ampRange) : 0.0;
    const double durScale = 2.0 * T_PI * T_PHI;

    for (int s = 0; s < inNumSamples; ++s) {
        int g = 0;
        while (main.phase >= 1.0 && g++ < 64) {
            main.phase -= 1.0;
            main.index = (main.index + 1) % knum;
            const int i = main.index;
            main.curAmp = main.nextAmp;

            // dist 0 (linear) makes the kick exactly ampKick*sin(theta): the
            // Chirikov map, chaotic above ~0.97. Other laws reshape the kick.
            const double au = 0.5 + 0.5 * std::sin(ampTh[i]);
            const double akick = ampKick * t_distribution(ampDist, (float)ampDistP, (float)au);
            t_stdmap(ampP[i], ampTh[i], akick, 1.0, ampScale);
            ampP[i] = t_barrier(ampP[i], ampLo, ampHi, barrierLo, barrierHi, rgen.frand());
            main.nextAmp = ampP[i];

            const double du = 0.5 + 0.5 * std::sin(durTh[i]);
            const double dkick = durKick * t_distribution(durDist, (float)durDistP, (float)du);
            t_stdmap(durP[i], durTh[i], dkick, 1.0, durScale);
            durP[i] = t_mirror(durP[i], 0.0, 1.0);

            main.inc = t_phaseinc(sr, minf, maxf, durP[i], knum);
        }
        o[s] = (float)t_read(main, interp);
        main.phase += main.inc;
    }

    unit->m_main = main;
}

void Diststoch_Ctor(Diststoch* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampP = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_ampTh = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durP = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durTh = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampP || !unit->m_ampTh || !unit->m_durP || !unit->m_durTh) {
        SETCALC(Diststoch_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }

    const double lo = ZIN0(AmpLo), hi = ZIN0(AmpHi);
    // Golden-ratio angles: a low-discrepancy spread so the rotors start decorrelated.
    for (int i = 0; i < n; ++i) {
        unit->m_ampP[i] = t_clamp(0.5 * std::sin(2.0 * T_PI * (i + 0.5) / n), lo, hi);
        unit->m_ampTh[i] = 2.0 * T_PI * std::fmod((i + 1) * T_PHI, 1.0);
        unit->m_durP[i] = 0.5;
        unit->m_durTh[i] = 2.0 * T_PI * std::fmod((i + 1) * T_PHI * 0.5 + 0.37, 1.0);
    }
    t_cp_reset(unit->m_main);

    SETCALC(Diststoch_next);
    Diststoch_next(unit, 1);
}

void Diststoch_Dtor(Diststoch* unit) {
    if (unit->m_ampP) RTFree(unit->mWorld, unit->m_ampP);
    if (unit->m_ampTh) RTFree(unit->mWorld, unit->m_ampTh);
    if (unit->m_durP) RTFree(unit->mWorld, unit->m_durP);
    if (unit->m_durTh) RTFree(unit->mWorld, unit->m_durTh);
}

PluginLoad(Diststoch) {
    ft = inTable;
    DefineDtorUnit(Diststoch);
}
