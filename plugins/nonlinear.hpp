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

// One RK4 step of a generalized relaxation oscillator, in nondimensional time
// (tau = 2*pi*freq*t) so the natural frequency is 1 and mu / force / z are in
// natural units rather than scaled by w or w^2:
//   x' = v,  v' = mu*damp(x,v)*v - stiff(x) - fb*z + force,  z' = fbRate*(x - z)
// shape morphs the damping from position-based (0: 1-x^2) to velocity-based
// (1: 1-v^2). well morphs the restoring force from a single well (0: x) to a
// double well (1: x^3 - x). z is a slow feedback state: fb > 0 makes the flow 3D,
// the minimum for chaos, since a 2D autonomous flow cannot be chaotic.
static inline void t_relax_step(double& x, double& v, double& z, double mu, double shape,
                                double well, double fb, double fbRate, double force, double dt) {
    auto dv = [&](double xx, double vv, double zz) {
        const double damp = (1.0 - shape) * (1.0 - xx * xx) + shape * (1.0 - vv * vv);
        const double stiff = (1.0 - well) * xx + well * (xx * xx * xx - xx);
        return mu * damp * vv - stiff - fb * zz + force;
    };
    auto dz = [&](double xx, double zz) { return fbRate * (xx - zz); };

    const double k1x = v,                    k1v = dv(x, v, z),                                     k1z = dz(x, z);
    const double k2x = v + 0.5 * dt * k1v,   k2v = dv(x + 0.5*dt*k1x, v + 0.5*dt*k1v, z + 0.5*dt*k1z), k2z = dz(x + 0.5*dt*k1x, z + 0.5*dt*k1z);
    const double k3x = v + 0.5 * dt * k2v,   k3v = dv(x + 0.5*dt*k2x, v + 0.5*dt*k2v, z + 0.5*dt*k2z), k3z = dz(x + 0.5*dt*k2x, z + 0.5*dt*k2z);
    const double k4x = v + dt * k3v,         k4v = dv(x + dt*k3x, v + dt*k3v, z + dt*k3z),             k4z = dz(x + dt*k3x, z + dt*k3z);

    x += (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
    v += (dt / 6.0) * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
    z += (dt / 6.0) * (k1z + 2.0 * k2z + 2.0 * k3z + k4z);
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
