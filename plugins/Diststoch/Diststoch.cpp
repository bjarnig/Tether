// Tether — Diststoch
#include "Diststoch.hpp"

static InterfaceTable* ft;

namespace tether {

Diststoch::Diststoch() {
    m_sr = (double)sampleRate();

    int n = (int)in0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    m_knum = n;

    m_ampMem = (double*)RTAlloc(mWorld, m_knum * sizeof(double));
    m_durMem = (double*)RTAlloc(mWorld, m_knum * sizeof(double));
    if (!m_ampMem || !m_durMem) {
        mCalcFunc = make_calc_function<Diststoch, &Diststoch::clearOutputs>();
        clearOutputs(1);
        return;
    }

    const double lo = in0(AmpLo), hi = in0(AmpHi);
    for (int i = 0; i < m_knum; ++i) {
        m_ampMem[i] = t_clamp(0.5 * std::sin(2.0 * T_PI * (i + 0.5) / m_knum), lo, hi);
        m_durMem[i] = 0.5;
    }
    m_ampPWalk = 0.0;
    m_durPWalk = 0.0;
    m_main.reset();

    mCalcFunc = make_calc_function<Diststoch, &Diststoch::next>();
    next(1);
}

Diststoch::~Diststoch() {
    if (m_ampMem) RTFree(mWorld, m_ampMem);
    if (m_durMem) RTFree(mWorld, m_durMem);
}

void Diststoch::clearOutputs(int nSamples) {
    float* o = out(0);
    for (int i = 0; i < nSamples; ++i) o[i] = 0.f;
}

void Diststoch::next(int nSamples) {
    float* o = out(0);

    const double minf = in0(MinFreq);
    const double maxf = in0(MaxFreq);
    const int ampDist = (int)in0(AmpDist);
    const double ampDistP = in0(AmpDistP);
    const double ampDistPWalk = in0(AmpDistPWalk);
    const int durDist = (int)in0(DurDist);
    const double durDistP = in0(DurDistP);
    const double durDistPWalk = in0(DurDistPWalk);
    const double ampStep = in0(AmpStep);
    const double durStep = in0(DurStep);
    const double ampLo = in0(AmpLo);
    const double ampHi = in0(AmpHi);
    const int barrierLo = (int)in0(BarrierLo);
    const int barrierHi = (int)in0(BarrierHi);
    const int interp = (int)in0(Interp);

    RGen& rgen = *mParent->mRGen;
    const int knum = m_knum;

    auto segLen = [&](double r) -> long {
        double cpRate = (minf + (maxf - minf) * r) * (double)knum;
        if (cpRate < 0.001) cpRate = 0.001;
        long len = (long)(m_sr / cpRate);
        return len < 1 ? 1 : len;
    };

    auto advance = [&]() {
        m_main.index = (m_main.index + 1) % knum;
        const int i = m_main.index;
        m_main.curAmp = m_main.nextAmp;

        // Slow walk on the distribution shape, folded into [0,1] around the base.
        m_ampPWalk += ampDistPWalk * (rgen.frand() * 2.0 - 1.0);
        m_ampPWalk = t_mirror(m_ampPWalk, -1.0, 1.0);
        m_durPWalk += durDistPWalk * (rgen.frand() * 2.0 - 1.0);
        m_durPWalk = t_mirror(m_durPWalk, -1.0, 1.0);
        const double effAmpP = t_mirror(ampDistP + m_ampPWalk, 0.0, 1.0);
        const double effDurP = t_mirror(durDistP + m_durPWalk, 0.0, 1.0);

        double a = m_ampMem[i];
        a += ampStep * t_distribution(ampDist, (float)effAmpP, rgen.frand());
        a = t_barrier(a, ampLo, ampHi, barrierLo, barrierHi, rgen.frand());
        m_ampMem[i] = a;
        m_main.nextAmp = a;

        double d = m_durMem[i];
        d += durStep * t_distribution(durDist, (float)effDurP, rgen.frand());
        d = t_clamp(d, 0.0, 1.0);
        m_durMem[i] = d;

        m_main.seglen = segLen(d);
        m_main.remain = m_main.seglen;
    };

    for (int s = 0; s < nSamples; ++s) {
        if (m_main.remain <= 0) advance();
        o[s] = (float)t_read(m_main, interp);
        --m_main.remain;
    }
}

} // namespace tether

PluginLoad(DiststochUGens) {
    ft = inTable;
    registerUnit<tether::Diststoch>(ft, "Diststoch", false);
}
