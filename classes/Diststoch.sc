Diststoch : UGen {
	*ar { |minfreq = 20, maxfreq = 1000, knum = 12,
		ampDist = 0, ampDistParam = 0.5, ampDistParamWalk = 0.0,
		durDist = 0, durDistParam = 0.5, durDistParamWalk = 0.0,
		ampStep = 0.1, durStep = 0.05, ampJump = 0.0, durJump = 0.0,
		ampLo = -1.0, ampHi = 1.0, barrierLo = 0, barrierHi = 0,
		interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', minfreq, maxfreq, knum,
			ampDist, ampDistParam, ampDistParamWalk,
			durDist, durDistParam, durDistParamWalk,
			ampStep, durStep, ampJump, durJump, ampLo, ampHi, barrierLo, barrierHi,
			interp).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
