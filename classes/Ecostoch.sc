Ecostoch : MultiOutUGen {
	// Predator-prey population synthesis. Returns [signal, preyCV, predCV].
	*ar { |rate = 4, growth = 1, decay = 1, interaction = 1,
		carrierFreq = 220, fmIndex = 2, noise = 0, oversample = 4, mul = 1, add = 0|

		^this.multiNew('audio', rate, growth, decay, interaction,
			carrierFreq, fmIndex, noise, oversample).madd(mul, add)
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(3, rate);
	}

	checkInputs { ^this.checkValidInputs }
}
