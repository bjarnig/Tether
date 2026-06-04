// Tether — Gendreve
#include "Gendreve.hpp"
#include <algorithm>

static InterfaceTable* ft;

namespace tether {

Gendreve::Gendreve() {
    m_sr = (double)sampleRate();

    int n = (int)in0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    m_knum = n;

    m_ampMem = (double*)RTAlloc(mWorld, m_knum * sizeof(double));
    m_durMem = (double*)RTAlloc(mWorld, m_knum * sizeof(double));

    if (!m_ampMem || !m_durMem) {
        m_ok = false;
        mCalcFunc = make_calc_function<Gendreve, &Gendreve::clearOutputs>();
        clearOutputs(1);
        return;
    }

    const int templateType = (int)in0(TemplateType);
    const double skew = in0(TemplateSkew);
    const double centerDur = t_clamp(in0(CenterDur), 0.0, 1.0);
    for (int i = 0; i < m_knum; ++i) {
        m_ampMem[i] = t_template(templateType, (i + 0.5) / m_knum, skew);
        m_durMem[i] = centerDur;
    }
    m_main.reset();
    m_ref.reset();

    m_ok = true;
    mCalcFunc = make_calc_function<Gendreve, &Gendreve::next>();
    next(1);
}

Gendreve::~Gendreve() {
    if (m_ampMem) RTFree(mWorld, m_ampMem);
    if (m_durMem) RTFree(mWorld, m_durMem);
}

void Gendreve::clearOutputs(int nSamples) {
    float* o0 = out(0);
    float* o1 = out(1);
    for (int i = 0; i < nSamples; ++i) { o0[i] = 0.f; o1[i] = 0.f; }
}

void Gendreve::next(int nSamples) {
    float* o0 = out(0);
    float* o1 = out(1);

    const double minf = in0(MinFreq);
    const double maxf = in0(MaxFreq);
    const double ampStiff = t_clamp(in0(AmpStiff), 0.0, 1.0);
    const double durStiff = t_clamp(in0(DurStiff), 0.0, 1.0);
    const double ampNoise = in0(AmpNoise);
    const double durNoise = in0(DurNoise);
    const int ampDist = (int)in0(AmpDist);
    const double ampDistP = in0(AmpDistP);
    const int durDist = (int)in0(DurDist);
    const double durDistP = in0(DurDistP);
    const int templateType = (int)in0(TemplateType);
    const double skew = in0(TemplateSkew);
    const double centerDur = t_clamp(in0(CenterDur), 0.0, 1.0);
    const int interp = (int)in0(Interp);

    RGen& rgen = *mParent->mRGen;
    const int knum = m_knum;

    // knum control points per fundamental period.
    auto segLen = [&](double r) -> long {
        double cpRate = (minf + (maxf - minf) * r) * (double)knum;
        if (cpRate < 0.001) cpRate = 0.001;
        long len = (long)(m_sr / cpRate);
        return len < 1 ? 1 : len;
    };

    auto advanceMain = [&]() {
        m_main.index = (m_main.index + 1) % knum;
        const int i = m_main.index;
        m_main.curAmp = m_main.nextAmp;

        const double tAmp = t_template(templateType, (i + 0.5) / knum, skew);
        double a = m_ampMem[i];
        a += -ampStiff * (a - tAmp) + ampNoise * t_distribution(ampDist, ampDistP, rgen.frand());
        a = t_mirror(a, -1.0, 1.0);
        m_ampMem[i] = a;
        m_main.nextAmp = a;

        double d = m_durMem[i];
        d += -durStiff * (d - centerDur) + durNoise * t_distribution(durDist, durDistP, rgen.frand());
        d = t_clamp(d, 0.0, 1.0);
        m_durMem[i] = d;

        m_main.seglen = segLen(d);
        m_main.remain = m_main.seglen;
    };

    auto advanceRef = [&]() {
        m_ref.index = (m_ref.index + 1) % knum;
        const int i = m_ref.index;
        m_ref.curAmp = m_ref.nextAmp;
        m_ref.nextAmp = t_template(templateType, (i + 0.5) / knum, skew);
        m_ref.seglen = segLen(centerDur);
        m_ref.remain = m_ref.seglen;
    };

    for (int s = 0; s < nSamples; ++s) {
        if (m_main.remain <= 0) advanceMain();
        if (m_ref.remain <= 0) advanceRef();
        o0[s] = (float)t_read(m_main, interp);
        o1[s] = (float)t_read(m_ref, interp);
        --m_main.remain;
        --m_ref.remain;
    }
}

} // namespace tether

PluginLoad(GendreveUGens) {
    ft = inTable;
    registerUnit<tether::Gendreve>(ft, "Gendreve", false);
}
