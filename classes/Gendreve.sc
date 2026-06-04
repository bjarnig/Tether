Gendreve : MultiOutUGen {
	// Returns [signal, lockedReference]; signal - lockedReference = residual.
	*ar { |minfreq = 20, maxfreq = 1000, knum = 12,
		ampStiffness = 0.1, durStiffness = 0.1,
		ampNoise = 0.1, durNoise = 0.1,
		ampDist = 0, ampDistParam = 0.5, durDist = 0, durDistParam = 0.5,
		templateType = 0, templateSkew = 0.5, centerDur = 0.5,
		interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', minfreq, maxfreq, knum,
			ampStiffness, durStiffness, ampNoise, durNoise,
			ampDist, ampDistParam, durDist, durDistParam,
			templateType, templateSkew, centerDur, interp).madd(mul, add)
	}

	init { arg ... theInputs;
		// knum must be a fixed scalar.
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

	checkInputs { ^this.checkValidInputs }
}
