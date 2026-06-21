Lifestoch : UGen {
	// ALife birth-death breakpoint population. maxCPs is fixed at build time.
	*ar { |freq = 110, maxCPs = 64, growth = 0.2, crowding = 0.01,
		birthThresh = 1.0, mutate = 0.1, interp = 1, mul = 1, add = 0|

		^this.multiNew('audio', freq, maxCPs, growth, crowding,
			birthThresh, mutate, interp).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
