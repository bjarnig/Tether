Gendreve : MultiOutUGen {
	// Returns [signal, lockedReference]; signal - lockedReference = residual.
	*ar { |minfreq = 20, maxfreq = 1000, knum = 12,
		ampStiffness = 0.1, durStiffness = 0.1,
		ampKick = 2.0, durKick = 2.0,
		templateType = 0, templateSkew = 0.5, centerDur = 0.5,
		interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', minfreq, maxfreq, knum,
			ampStiffness, durStiffness, ampKick, durKick,
			templateType, templateSkew, centerDur, interp).madd(mul, add)
	}

	init { arg ... theInputs;
		// knum must be a fixed scalar.
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

	checkInputs { ^this.checkValidInputs }
}
