Limcycle : MultiOutUGen {
	// Generalized relaxation oscillator. Returns [x, velocity].
	*ar { |freq = 110, mu = 1, shape = 0, well = 0, force = 0, forceFreq = 55,
		noise = 0, tail = 2, oversample = 4, mul = 1, add = 0|

		^this.multiNew('audio', freq, mu, shape, well, force, forceFreq,
			noise, tail, oversample).madd(mul, add)
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

	checkInputs { ^this.checkValidInputs }
}
