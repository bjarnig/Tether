// Tether — parameter sweeps
// ==========================
// Boot the server, then evaluate any block. MouseX (and sometimes MouseY) sweep
// the headline parameter(s) so you can hear the full range in one gesture and
// note where it gets interesting/strange. Cmd/Ctrl-. to stop.

s.boot;

// Gendreve — X: ampStiffness (noisy <-> pitched), Y: durStiffness
{ Gendreve.ar(100, 400, 12, MouseX.kr(0, 1), MouseY.kr(0, 1), 0.3, 0.3)[0] * 0.2 ! 2 }.play;

// Diststoch — X: ampJump (walk <-> memoryless), Y: barrierHi (0..3)
{ Diststoch.ar(120, 480, 14, ampStep: 0.25, ampJump: MouseX.kr(0, 1), barrierHi: MouseY.kr(0, 3).round) * 0.2 ! 2 }.play;

// Cyclegen — X: ampJump, sequenced melody
{ Cyclegen.ar(12, ampStep: 0.1, ampJump: MouseX.kr(0, 1), freqs: [110, 165, 220, 330, 277]) * 0.2 ! 2 }.play;

// Chaosgen — X: chaos (ordered -> noisy)
{ Chaosgen.ar(120, 480, 16, chaos: MouseX.kr(3.2, 3.999)) * 0.2 ! 2 }.play;

// Varperiod — X: knum (sparse -> dense), steady pitch
{ Varperiod.ar(150, 150, 40, MouseX.kr(2, 40).round, ampStep: 0.25, durStep: 0.0) * 0.2 ! 2 }.play;

// Fracflight — X: hurst (rough <-> smooth), Y: levy (gaussian <-> heavy-tailed)
{ Fracflight.ar(110, 330, 16, MouseX.kr(0.1, 0.9), MouseY.kr(0, 1), ampReversion: 0.3, ampStep: 0.3, durReversion: 0.5, durStep: 0.2) * 0.2 ! 2 }.play;

// Grainstoch — X: jitter (pitched pulsar <-> cloud), Y: duty (formant)
{ Grainstoch.ar(160, 16, duty: MouseY.kr(0.1, 1.0), ampStep: 0.2, jitter: MouseX.kr(0, 1)) * 0.3 ! 2 }.play;

// Probstoch — X: prob (tone <-> grains <-> noise), steady pitch
{ Probstoch.ar(200, 16, prob: MouseX.kr(0, 1)) * 0.2 ! 2 }.play;
