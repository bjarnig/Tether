#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Varperiod : public Unit {
    int m_maxCPs;
    double* m_ampMem;
    double* m_durMem;
    CPState m_main;
    double m_sr;
};

enum {
    MinFreq, MaxFreq, MaxCPs, KNum, AmpStep, DurStep,
    AmpDist, AmpDistP, DurDist, DurDistP, Interp
};

void Varperiod_clear(Varperiod* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Varperiod_next(Varperiod* unit, int inNumSamples) {
    float* o = OUT(0);

    const double minf = ZIN0(MinFreq);
    const double maxf = ZIN0(MaxFreq);
    int knum = (int)ZIN0(KNum);                 // active count — modulatable
    if (knum < 1) knum = 1;
    if (knum > unit->m_maxCPs) knum = unit->m_maxCPs;
    const double ampStep = ZIN0(AmpStep);
    const double durStep = ZIN0(DurStep);
    const int ampDist = (int)ZIN0(AmpDist);
    const double ampDistP = ZIN0(AmpDistP);
    const int durDist = (int)ZIN0(DurDist);
    const double durDistP = ZIN0(DurDistP);
    const int interp = (int)ZIN0(Interp);

    RGen& rgen = *unit->mParent->mRGen;
    const double sr = unit->m_sr;
    double* ampMem = unit->m_ampMem;
    double* durMem = unit->m_durMem;
    CPState main = unit->m_main;

    for (int s = 0; s < inNumSamples; ++s) {
        if (main.remain <= 0) {
            main.index = (main.index + 1) % knum;
            const int i = main.index;
            main.curAmp = main.nextAmp;

            double a = ampMem[i] + ampStep * t_distribution(ampDist, (float)ampDistP, rgen.frand());
            a = t_mirror(a, -1.0, 1.0);
            ampMem[i] = a;
            main.nextAmp = a;

            double d = durMem[i] + durStep * t_distribution(durDist, (float)durDistP, rgen.frand());
            d = t_clamp(d, 0.0, 1.0);
            durMem[i] = d;

            main.seglen = t_seglen(sr, minf, maxf, d, knum);
            main.remain = main.seglen;
        }
        o[s] = (float)t_read(main, interp);
        --main.remain;
    }

    unit->m_main = main;
}

void Varperiod_Ctor(Varperiod* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(MaxCPs);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_maxCPs = n;

    unit->m_ampMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampMem || !unit->m_durMem) {
        SETCALC(Varperiod_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }
    RGen& rgen = *unit->mParent->mRGen;
    for (int i = 0; i < n; ++i) {
        unit->m_ampMem[i] = 2.0 * rgen.frand() - 1.0;
        unit->m_durMem[i] = rgen.frand();
    }
    t_cp_reset(unit->m_main);

    SETCALC(Varperiod_next);
    Varperiod_next(unit, 1);
}

void Varperiod_Dtor(Varperiod* unit) {
    if (unit->m_ampMem) RTFree(unit->mWorld, unit->m_ampMem);
    if (unit->m_durMem) RTFree(unit->mWorld, unit->m_durMem);
}

PluginLoad(Varperiod) {
    ft = inTable;
    DefineDtorUnit(Varperiod);
}
