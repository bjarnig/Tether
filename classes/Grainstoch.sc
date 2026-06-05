Grainstoch : UGen {
	*ar { |pitch = 100, knum = 16, duty = 0.5, window = 1,
		ampStep = 0.1, ampJump = 0, jitter = 0, ampDist = 0, ampDistParam = 0.5,
		interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', pitch, knum, duty, window,
			ampStep, ampJump, jitter, ampDist, ampDistParam, interp).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
