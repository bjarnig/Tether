Chaosgen : UGen {
	*ar { |minfreq = 20, maxfreq = 1000, knum = 12, chaos = 3.7,
		ampDist = 0, ampDistParam = 0.5, interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', minfreq, maxfreq, knum, chaos,
			ampDist, ampDistParam, interp).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
