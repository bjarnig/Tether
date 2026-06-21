Oscnet : MultiOutUGen {
	// Coupled-oscillator network. Returns [shaped, clean]. voices is fixed at build time.
	*ar { |freq = 110, voices = 4, spread = 1.01, topo = 0, couple = 0.2,
		modFreq = 0.3, modDepth = 0.05, fold = 0, drive = 2, feedback = 0.2, drift = 0,
		mul = 1, add = 0|

		^this.multiNew('audio', freq, voices, spread, topo, couple,
			modFreq, modDepth, fold, drive, feedback, drift).madd(mul, add)
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

	checkInputs { ^this.checkValidInputs }
}
