Fracflight : UGen {
	*ar { |minfreq = 20, maxfreq = 1000, knum = 12, hurst = 0.5, levy = 0.0,
		ampReversion = 0.1, ampStep = 0.1, durReversion = 0.1, durStep = 0.05,
		centerDur = 0.5, templateType = 0, templateSkew = 0.5,
		interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', minfreq, maxfreq, knum, hurst, levy,
			ampReversion, ampStep, durReversion, durStep, centerDur,
			templateType, templateSkew, interp).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
