Diststoch : UGen {
	*ar { |minfreq = 20, maxfreq = 1000, knum = 12,
		ampDist = 0, ampDistParam = 0.5, durDist = 0, durDistParam = 0.5,
		ampKick = 2.0, durKick = 2.0,
		ampLo = -1.0, ampHi = 1.0, barrierLo = 0, barrierHi = 0,
		interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', minfreq, maxfreq, knum,
			ampDist, ampDistParam, durDist, durDistParam,
			ampKick, durKick, ampLo, ampHi, barrierLo, barrierHi,
			interp).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
