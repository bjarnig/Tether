// Tether — shared helpers for the nonlinear / chaotic UGen family
// (Limcycle, Oscnet). Reuses t_clamp / T_PI from shared.hpp.
#pragma once
#include "shared.hpp"
#include <cmath>

// Bounded saturator (~tanh) for distortion and ODE state limiting.
static inline double t_tanh(double x) {
    if (x < -3.0) return -1.0;
    if (x > 3.0) return 1.0;
    const double x2 = x * x;
    return x * (27.0 + x2) / (27.0 + 9.0 * x2);
}

// Reset NaN/Inf to 0 and clamp magnitude — keeps a diverging system server-safe.
static inline double t_blowup_guard(double x, double lim) {
    if (!std::isfinite(x)) return 0.0;
    return t_clamp(x, -lim, lim);
}

// Rising-edge detector: true when the signal crosses thresh upward this sample.
static inline bool t_edge(double cur, double prev, double thresh) {
    return cur > thresh && prev <= thresh;
}

// One RK4 step of a generalized relaxation oscillator:
//   x' = v,  v' = mu * damp(x,v) * v - stiff(x) + force
// shape morphs the damping from position-based (0, Van der Pol: 1-x^2) to
// velocity-based (1, Rayleigh: 1-(v/w)^2). well morphs the restoring force from a
// single well (0: w^2 x) to a double well (1: w^2 (x^3 - x), Duffing).
static inline void t_relax_step(double& x, double& v, double w, double mu,
                                double shape, double well, double force, double dt) {
    const double w2 = w * w;
    const double iw = 1.0 / (w + 1e-9);
    auto dv = [&](double xx, double vv) {
        const double vn = vv * iw;
        const double damp = (1.0 - shape) * (1.0 - xx * xx) + shape * (1.0 - vn * vn);
        const double stiff = w2 * ((1.0 - well) * xx + well * (xx * xx * xx - xx));
        return mu * damp * vv - stiff + force;
    };
    const double k1x = v;
    const double k1v = dv(x, v);
    const double k2x = v + 0.5 * dt * k1v;
    const double k2v = dv(x + 0.5 * dt * k1x, v + 0.5 * dt * k1v);
    const double k3x = v + 0.5 * dt * k2v;
    const double k3v = dv(x + 0.5 * dt * k2x, v + 0.5 * dt * k2v);
    const double k4x = v + dt * k3v;
    const double k4v = dv(x + dt * k3x, v + dt * k3v);
    x += (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
    v += (dt / 6.0) * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
}

// TPT (Zavalishin) state-variable filter. Call set() to retune, then process()
// per sample; read lp / bp / hp afterwards.
struct SVF {
    double ic1eq, ic2eq, a1, a2, a3, k;
    double lp, bp, hp;
    void reset() { ic1eq = ic2eq = lp = bp = hp = 0.0; }
    void set(double cut, double q, double sr) {
        cut = t_clamp(cut, 1.0, sr * 0.45);
        const double g = std::tan(T_PI * cut / sr);
        k = 1.0 / t_clamp(q, 0.05, 100.0);
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void process(double x) {
        const double v3 = x - ic2eq;
        const double v1 = a1 * ic1eq + a2 * v3;
        const double v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }
};
