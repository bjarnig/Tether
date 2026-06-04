Seggen : UGen {
	// freqs is a fixed array stepped one value per cycle.
	*ar { |knum = 8, ampStep = 0.05, ampDist = 0, ampDistParam = 0.5, ampJump = 0,
		interp = 1, freqs = #[110, 165, 220, 330], mul = 1, add = 0|
		var f = freqs.asArray;
		^this.multiNew('audio', knum, ampStep, ampDist, ampDistParam, ampJump,
			interp, f.size, *f).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
