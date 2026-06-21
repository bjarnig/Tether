Scangen : UGen {
	// Scanned synthesis: a mass-spring ring read at audio rate. size is fixed at build time.
	*ar { |freq = 110, size = 64, rate = 0.05, tension = 0.2, damping = 0.005,
		center = 0.001, nonlin = 0, excite = 0.01, mul = 1, add = 0|

		^this.multiNew('audio', freq, size, rate, tension, damping,
			center, nonlin, excite).madd(mul, add)
	}

	checkInputs { ^this.checkValidInputs }
}
