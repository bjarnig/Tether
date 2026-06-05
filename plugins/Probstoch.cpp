#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Probstoch : public Unit {
    int m_knum;
    double* m_ampMem;
    CPState m_main;
    double m_sr;
};

enum { Freq, KNum, Prob, AmpStep, Jump, AmpDist, AmpDistP, TemplateType, TemplateSkew, Interp };

void Probstoch_clear(Probstoch* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Probstoch_next(Probstoch* unit, int inNumSamples) {
    float* o = OUT(0);

    double freq = ZIN0(Freq);
    if (freq < 0.01) freq = 0.01;
    const double prob = t_clamp(ZIN0(Prob), 0.0, 1.0);
    const double ampStep = ZIN0(AmpStep);
    const double jump = t_clamp(ZIN0(Jump), 0.0, 1.0);
    const int ampDist = (int)ZIN0(AmpDist);
    const double ampDistP = ZIN0(AmpDistP);
    const int interp = (int)ZIN0(Interp);

    RGen& rgen = *unit->mParent->mRGen;
    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampMem = unit->m_ampMem;
    CPState main = unit->m_main;

    // fixed period: knum equal segments -> the fundamental is exactly freq
    long seglen = (long)(sr / (freq * knum));
    if (seglen < 1) seglen = 1;

    for (int s = 0; s < inNumSamples; ++s) {
        if (main.remain <= 0) {
            main.index = (main.index + 1) % knum;
            const int i = main.index;
            main.curAmp = main.nextAmp;

            // most points hold (the stable period -> pitch); prob of a grain
            if (rgen.frand() < prob) {
                const double r = t_distribution(ampDist, (float)ampDistP, rgen.frand());
                const double walked = ampMem[i] + ampStep * r;
                ampMem[i] = t_mirror(walked + jump * (r - walked), -1.0, 1.0);
            }
            main.nextAmp = ampMem[i];

            main.seglen = seglen;
            main.remain = seglen;
        }
        o[s] = (float)t_read(main, interp);
        --main.remain;
    }

    unit->m_main = main;
}

void Probstoch_Ctor(Probstoch* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampMem) {
        SETCALC(Probstoch_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }
    const int templateType = (int)ZIN0(TemplateType);
    const double skew = ZIN0(TemplateSkew);
    for (int i = 0; i < n; ++i)
        unit->m_ampMem[i] = t_template(templateType, (i + 0.5) / n, skew);
    t_cp_reset(unit->m_main);

    SETCALC(Probstoch_next);
    Probstoch_next(unit, 1);
}

void Probstoch_Dtor(Probstoch* unit) {
    if (unit->m_ampMem) RTFree(unit->mWorld, unit->m_ampMem);
}

PluginLoad(Probstoch) {
    ft = inTable;
    DefineDtorUnit(Probstoch);
}
