#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Chaosgen : public Unit {
    int m_knum;
    double m_ampChaos;
    double m_durChaos;
    CPState m_main;
    double m_sr;
};

enum { MinFreq, MaxFreq, KNum, Chaos, AmpDist, AmpDistP, Interp };

static inline double t_logistic(double x, double r) {
    x = r * x * (1.0 - x);
    return t_clamp(x, 1e-7, 1.0 - 1e-7); // keep off the 0/1 fixed points
}

void Chaosgen_next(Chaosgen* unit, int inNumSamples) {
    float* o = OUT(0);

    const double minf = ZIN0(MinFreq);
    const double maxf = ZIN0(MaxFreq);
    const double chaos = t_clamp(ZIN0(Chaos), 2.5, 3.999); // bifurcation parameter
    const int ampDist = (int)ZIN0(AmpDist);
    const double ampDistP = ZIN0(AmpDistP);
    const int interp = (int)ZIN0(Interp);

    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double ampChaos = unit->m_ampChaos;
    double durChaos = unit->m_durChaos;
    CPState main = unit->m_main;

    for (int s = 0; s < inNumSamples; ++s) {
        int g = 0;
        while (main.phase >= 1.0 && g++ < 64) {
            main.phase -= 1.0;
            main.curAmp = main.nextAmp;

            ampChaos = t_logistic(ampChaos, chaos);
            durChaos = t_logistic(durChaos, chaos);

            // Control-point value is the map output, shaped by the distribution.
            double a = t_distribution(ampDist, (float)ampDistP, (float)ampChaos);
            main.nextAmp = t_clamp(a, -1.0, 1.0);

            main.inc = t_phaseinc(sr, minf, maxf, durChaos, knum);
        }
        o[s] = (float)t_read(main, interp);
        main.phase += main.inc;
    }

    unit->m_ampChaos = ampChaos;
    unit->m_durChaos = durChaos;
    unit->m_main = main;
}

void Chaosgen_Ctor(Chaosgen* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampChaos = 0.31; // seeds off 0.5 to avoid the r=4 -> 1 -> 0 trap
    unit->m_durChaos = 0.53;
    t_cp_reset(unit->m_main);

    SETCALC(Chaosgen_next);
    Chaosgen_next(unit, 1);
}

PluginLoad(Chaosgen) {
    ft = inTable;
    DefineSimpleUnit(Chaosgen);
}
