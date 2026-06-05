Varperiod : UGen {
	*ar { |minfreq = 20, maxfreq = 1000, maxCPs = 24, knum = 12,
		ampStep = 0.1, durStep = 0.05,
		ampDist = 0, ampDistParam = 0.5, durDist = 0, durDistParam = 0.5,
		interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', minfreq, maxfreq, maxCPs, knum,
			ampStep, durStep, ampDist, ampDistParam, durDist, durDistParam,
			interp).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
