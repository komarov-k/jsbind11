const assert = require('assert');
const almostEqual = require("almost-equal")
const test = require('./jsbind11-test');

function getRandomInt(min, max) {
  min = Math.ceil(min);
  max = Math.floor(max);
  return Math.floor(Math.random() * (max - min)) + min;
}

function getRandom(min, max) {
  return Math.random() * (max - min) + min;
}

var numRepeat = 128;

['Int', 'UInt'].forEach(function(dataType) {
    [8, 16, 32, 64].forEach(function(numBits) {
	var testClassName = 'Test' + dataType + numBits.toString() + 'Adder';
	describe(testClassName, function() {
	    describe('getValue()', function() {
		for (var n = 0; n < numRepeat; n++) {
		    if (dataType.startsWith('U')) {
			var maxValue = Math.pow(2, numBits) - 1;
			var minValue = 0;
		    } else {
			var maxValue = Math.pow(2, numBits-1) - 1;
			var minValue = -Math.pow(2, numBits-1);
		    }

		    var foundValue = false;
		    
		    var x = undefined,
			y = undefined,
			z = undefined;
		    while (!foundValue) {
			x = getRandomInt(minValue / 2, maxValue / 2);
			y = getRandomInt(minValue / 2, maxValue / 2); 
			z = x + y;
			
			if ((minValue < z) && (z < maxValue)) {
			    foundValue = true;
			}
		    }
		    
		    var adder = new test[testClassName](x, y);
		    it('should return ' + z.toString(), function() {
			assert.equal(z, adder.getValue());
		    })
		}
	    })
	})
    })
})

describe('TestFloatAdder', function() {
    describe('getValue()', function() {
	for (var n = 0; n < numRepeat; n++) {
	    var maxValue = Math.pow(10, 32);
	    var minValue = -Math.pow(10, 32);
	    var foundValue = false;
	    
	    var x = undefined,
		y = undefined,
		z = undefined;
	    while (!foundValue) {
		x = getRandomInt(minValue / 2, maxValue / 2);
		y = getRandomInt(minValue / 2, maxValue / 2); 
		z = x + y;
		
		if ((minValue < z) && (z < maxValue)) {
		    foundValue = true;
		}
	    }
	    
	    var adder = new test.TestFloatAdder(x, y);
	    
	    it('should be equal', function() {
		assert(almostEqual(z, adder.getValue(),
				   almostEqual.DBL_EPSILON,
				   almostEqual.FLT_EPSILON),
		       z.toString() + ' != ' + adder.getValue().toString());
	    })	
	}
    })
})

describe('TestDoubleAdder', function() {
    describe('getValue()', function() {
	for (var n = 0; n < numRepeat; n++) {
	    var maxValue = Math.pow(10, 32);
	    var minValue = -Math.pow(10, 32);
	    
	    var foundValue = false;
	    
	    var x = undefined,
		y = undefined,
		z = undefined;
	    while (!foundValue) {
		x = getRandomInt(minValue / 2, maxValue / 2);
		y = getRandomInt(minValue / 2, maxValue / 2); 
		z = x + y;
		
		if ((minValue < z) && (z < maxValue)) {
		    foundValue = true;
		}
	    }
	
	    var adder = new test.TestDoubleAdder(x, y);
	    
	    it('should be equal', function() {
		assert(almostEqual(z, adder.getValue(),
				   almostEqual.DBL_EPSILON,
				   almostEqual.DBL_EPSILON),
		       z.toString() + ' != ' + adder.getValue().toString());
	    })
	}
    })
})
