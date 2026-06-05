Probstoch : UGen {
	*ar { |freq = 200, knum = 16, prob = 0.1, ampStep = 1.0, jump = 1.0,
		ampDist = 0, ampDistParam = 0.5, templateType = 0, templateSkew = 0.5,
		interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', freq, knum, prob, ampStep, jump,
			ampDist, ampDistParam, templateType, templateSkew, interp).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
