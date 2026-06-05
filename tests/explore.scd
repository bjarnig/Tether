// Tether — interactive random explorer
// =====================================
// 1. Evaluate the SETUP block once (boots the server, defines ~roll).
// 2. Evaluate any UGen block (between the parens) to hear it with random
//    parameters. Each run posts the values and a copy-paste-playable line, so
//    when something is interesting/strange you can reproduce or note the range.
// 3. Cmd/Ctrl-. stops everything.
//
// ~roll picks each parameter from its range, posts name: value, prints
//    { Name.ar(...).asArray[0] * 0.2 ! 2 }.play;
// and plays it safely (DC-blocked, limited, ~5 s, auto-freed).

// ---------- SETUP (evaluate once) ----------
(
s.waitForBoot {
	~fmt = { |v|
		case
		{ v.isArray } { "[" ++ v.collect({ |x| x.asFloat.round(0.1) }).join(", ") ++ "]" }
		{ v.isKindOf(Float) } { v.round(0.001).asString }
		{ v.asString };
	};
	~roll = { |class, spec, dur = 5|
		var vals = spec.collect { |p| p[1].value };
		var argstr = vals.collect(~fmt).join(", ");
		("\n=== % ===".format(class.name)).postln;
		spec.do { |p, i| ("    " ++ p[0].asString.padRight(17) ++ ~fmt.(vals[i])).postln };
		("    { %.ar(%).asArray[0] * 0.2 ! 2 }.play;".format(class.name, argstr)).postln;
		{
			var sig = class.perform(\ar, *vals).asArray[0];
			sig = LeakDC.ar(sig);
			sig = sig * EnvGen.kr(Env.linen(0.05, dur, 0.3), doneAction: 2) * 0.2;
			Limiter.ar(sig.tanh, 0.95) ! 2
		}.play;
	};
	"Tether explorer ready. Evaluate a UGen block below.".postln;
};
)

// ---------- Gendreve — mean-reverting walk (out 0 of 2) ----------
(
~roll.(Gendreve, [
	[\minfreq,      { exprand(40, 400) }],
	[\maxfreq,      { exprand(400, 2000) }],
	[\knum,         { rrand(4, 24) }],
	[\ampStiffness, { rrand(0.0, 1.0) }],
	[\durStiffness, { rrand(0.0, 1.0) }],
	[\ampNoise,     { rrand(0.05, 0.6) }],
	[\durNoise,     { rrand(0.05, 0.6) }],
	[\ampDist,      { rrand(0, 6) }],
	[\ampDistParam, { rrand(0.0, 1.0) }],
	[\durDist,      { rrand(0, 6) }],
	[\durDistParam, { rrand(0.0, 1.0) }],
	[\templateType, { rrand(0, 4) }],
	[\templateSkew, { rrand(0.1, 0.9) }],
	[\centerDur,    { rrand(0.2, 0.8) }],
	[\interp,       { rrand(0, 2) }],
]);
)

// ---------- Diststoch — free walk, barriers, jumps ----------
(
~roll.(Diststoch, [
	[\minfreq,          { exprand(40, 400) }],
	[\maxfreq,          { exprand(400, 2000) }],
	[\knum,             { rrand(6, 24) }],
	[\ampDist,          { rrand(0, 6) }],
	[\ampDistParam,     { rrand(0.0, 1.0) }],
	[\ampDistParamWalk, { rrand(0.0, 0.05) }],
	[\durDist,          { rrand(0, 6) }],
	[\durDistParam,     { rrand(0.0, 1.0) }],
	[\durDistParamWalk, { rrand(0.0, 0.05) }],
	[\ampStep,          { rrand(0.05, 0.4) }],
	[\durStep,          { rrand(0.0, 0.2) }],
	[\ampJump,          { rrand(0.0, 1.0) }],
	[\durJump,          { rrand(0.0, 1.0) }],
	[\ampLo,            { rrand(-1.0, -0.3) }],
	[\ampHi,            { rrand(0.3, 1.0) }],
	[\barrierLo,        { rrand(0, 3) }],
	[\barrierHi,        { rrand(0, 3) }],
	[\interp,           { rrand(0, 2) }],
]);
)

// ---------- Cyclegen — sequenced pitch, walking waveform ----------
(
~roll.(Cyclegen, [
	[\knum,         { rrand(4, 16) }],
	[\ampStep,      { rrand(0.0, 0.2) }],
	[\ampDist,      { rrand(0, 6) }],
	[\ampDistParam, { rrand(0.0, 1.0) }],
	[\ampJump,      { rrand(0.0, 1.0) }],
	[\stiffness,    { rrand(0.0, 1.0) }],
	[\templateType, { rrand(0, 4) }],
	[\templateSkew, { rrand(0.1, 0.9) }],
	[\freqMul,      { exprand(0.5, 2.0) }],
	[\repeats,      { rrand(1, 4) }],
	[\seqMode,      { rrand(0, 3) }],
	[\interp,       { rrand(0, 2) }],
	[\freqs,        { Array.fill(rrand(2, 6), { exprand(80, 600).round(1) }) }],
]);
)

// ---------- Chaosgen — logistic-map values ----------
(
~roll.(Chaosgen, [
	[\minfreq,      { exprand(40, 400) }],
	[\maxfreq,      { exprand(400, 1500) }],
	[\knum,         { rrand(6, 24) }],
	[\chaos,        { rrand(3.2, 3.999) }],
	[\ampDist,      { rrand(0, 6) }],
	[\ampDistParam, { rrand(0.0, 1.0) }],
	[\interp,       { rrand(0, 2) }],
]);
)

// ---------- Varperiod — variable control-point count ----------
(
~roll.(Varperiod, [
	[\minfreq,      { exprand(40, 400) }],
	[\maxfreq,      { exprand(400, 1500) }],
	[\maxCPs,       { rrand(24, 48) }],
	[\knum,         { rrand(2, 24) }],          // <= maxCPs by construction
	[\ampStep,      { rrand(0.05, 0.4) }],
	[\durStep,      { rrand(0.0, 0.15) }],
	[\ampDist,      { rrand(0, 6) }],
	[\ampDistParam, { rrand(0.0, 1.0) }],
	[\durDist,      { rrand(0, 6) }],
	[\durDistParam, { rrand(0.0, 1.0) }],
	[\interp,       { rrand(0, 2) }],
]);
)

// ---------- Fracflight — OU + fractional (Hurst) + Lévy ----------
(
~roll.(Fracflight, [
	[\minfreq,       { exprand(60, 300) }],
	[\maxfreq,       { exprand(300, 1200) }],
	[\knum,          { rrand(6, 20) }],
	[\hurst,         { rrand(0.1, 0.9) }],
	[\levy,          { rrand(0.0, 1.0) }],
	[\ampReversion,  { rrand(0.0, 0.6) }],
	[\ampStep,       { rrand(0.05, 0.4) }],
	[\durReversion,  { rrand(0.0, 0.7) }],
	[\durStep,       { rrand(0.0, 0.3) }],
	[\centerDur,     { rrand(0.2, 0.8) }],
	[\templateType,  { rrand(0, 4) }],
	[\templateSkew,  { rrand(0.1, 0.9) }],
	[\interp,        { rrand(0, 2) }],
]);
)

// ---------- Grainstoch — stochastic pulsar ----------
(
~roll.(Grainstoch, [
	[\pitch,        { exprand(50, 800) }],
	[\knum,         { rrand(8, 32) }],
	[\duty,         { rrand(0.1, 1.0) }],
	[\window,       { rrand(0, 3) }],
	[\ampStep,      { rrand(0.0, 0.4) }],
	[\ampJump,      { rrand(0.0, 1.0) }],
	[\jitter,       { rrand(0.0, 1.0) }],
	[\ampDist,      { rrand(0, 6) }],
	[\ampDistParam, { rrand(0.0, 1.0) }],
	[\interp,       { rrand(0, 2) }],
]);
)

// ---------- Probstoch — sparse grains on a fixed pitch ----------
(
~roll.(Probstoch, [
	[\freq,         { exprand(80, 1000) }],
	[\knum,         { rrand(8, 32) }],
	[\prob,         { rrand(0.0, 1.0) }],
	[\ampStep,      { rrand(0.2, 1.0) }],
	[\jump,         { rrand(0.0, 1.0) }],
	[\ampDist,      { rrand(0, 6) }],
	[\ampDistParam, { rrand(0.0, 1.0) }],
	[\templateType, { rrand(0, 4) }],
	[\templateSkew, { rrand(0.1, 0.9) }],
	[\interp,       { rrand(0, 2) }],
]);
)
