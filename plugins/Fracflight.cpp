#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Fracflight : public Unit {
    int m_knum;
    double* m_ampMem;
    double* m_ampVel;   // fBm-correlated increment, per control point
    double* m_durMem;
    double* m_durVel;
    CPState m_main;
    double m_sr;
};

enum {
    MinFreq, MaxFreq, KNum, Hurst, Levy,
    AmpReversion, AmpStep, DurReversion, DurStep, CenterDur,
    TemplateType, TemplateSkew, Interp
};

void Fracflight_clear(Fracflight* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Fracflight_next(Fracflight* unit, int inNumSamples) {
    float* o = OUT(0);

    const double minf = ZIN0(MinFreq);
    const double maxf = ZIN0(MaxFreq);
    const double hurst = t_clamp(ZIN0(Hurst), 0.05, 0.95);
    const double levy = t_clamp(ZIN0(Levy), 0.0, 1.0);
    const double ampRev = t_clamp(ZIN0(AmpReversion), 0.0, 1.0);
    const double ampStep = ZIN0(AmpStep);
    const double durRev = t_clamp(ZIN0(DurReversion), 0.0, 1.0);
    const double durStep = ZIN0(DurStep);
    const double centerDur = t_clamp(ZIN0(CenterDur), 0.0, 1.0);
    const int templateType = (int)ZIN0(TemplateType);
    const double skew = ZIN0(TemplateSkew);
    const int interp = (int)ZIN0(Interp);

    // fBm: lag-1 correlation of fractional Gaussian noise; >0 persistent, <0 reverting.
    const double rho = std::pow(2.0, 2.0 * hurst - 1.0) - 1.0;
    const double innov = std::sqrt(1.0 - rho * rho);
    const double alpha = 2.0 - levy; // Lévy stability: 2 Gaussian .. 1 Cauchy

    RGen& rgen = *unit->mParent->mRGen;
    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampMem = unit->m_ampMem;
    double* ampVel = unit->m_ampVel;
    double* durMem = unit->m_durMem;
    double* durVel = unit->m_durVel;
    CPState main = unit->m_main;

    for (int s = 0; s < inNumSamples; ++s) {
        if (main.remain <= 0) {
            main.index = (main.index + 1) % knum;
            const int i = main.index;
            main.curAmp = main.nextAmp;

            // amplitude: OU reversion toward the template, fBm-correlated Lévy step
            ampVel[i] = rho * ampVel[i] + innov * t_stable(alpha, rgen.frand(), rgen.frand());
            const double tAmp = t_template(templateType, (i + 0.5) / knum, skew);
            double a = ampMem[i] - ampRev * (ampMem[i] - tAmp) + ampStep * ampVel[i];
            a = t_mirror(a, -1.0, 1.0);
            ampMem[i] = a;
            main.nextAmp = a;

            // duration: OU reversion toward a home pitch (centerDur)
            durVel[i] = rho * durVel[i] + innov * t_stable(alpha, rgen.frand(), rgen.frand());
            double d = durMem[i] - durRev * (durMem[i] - centerDur) + durStep * durVel[i];
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

void Fracflight_Ctor(Fracflight* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_ampVel = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durVel = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampMem || !unit->m_ampVel || !unit->m_durMem || !unit->m_durVel) {
        SETCALC(Fracflight_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }
    const int templateType = (int)ZIN0(TemplateType);
    const double skew = ZIN0(TemplateSkew);
    const double centerDur = t_clamp(ZIN0(CenterDur), 0.0, 1.0);
    for (int i = 0; i < n; ++i) {
        unit->m_ampMem[i] = t_template(templateType, (i + 0.5) / n, skew);
        unit->m_ampVel[i] = 0.0;
        unit->m_durMem[i] = centerDur;
        unit->m_durVel[i] = 0.0;
    }
    t_cp_reset(unit->m_main);

    SETCALC(Fracflight_next);
    Fracflight_next(unit, 1);
}

void Fracflight_Dtor(Fracflight* unit) {
    if (unit->m_ampMem) RTFree(unit->mWorld, unit->m_ampMem);
    if (unit->m_ampVel) RTFree(unit->mWorld, unit->m_ampVel);
    if (unit->m_durMem) RTFree(unit->mWorld, unit->m_durMem);
    if (unit->m_durVel) RTFree(unit->mWorld, unit->m_durVel);
}

PluginLoad(Fracflight) {
    ft = inTable;
    DefineDtorUnit(Fracflight);
}
