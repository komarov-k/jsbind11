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

var numRepeat = 32;

['Int', 'UInt'].forEach(function(dataType) {
    [8, 16, 32, 64].forEach(function(numBits) {
	var testFunctionName = 'test' + dataType + numBits.toString() + 'BypassFunction';
	describe(testFunctionName, function() {
	    for (var n = 0; n < numRepeat; n++) {
		if (dataType.startsWith('U')) {
		    var maxValue = Math.pow(2, numBits) - 1;
		    var minValue = 0;
		} else {
		    var maxValue = Math.pow(2, numBits-1) - 1;
		    var minValue = -Math.pow(2, numBits-1);
		}
		
		var expected = getRandomInt(minValue, maxValue);
		it('should return ' + expected.toString(), function() {
		    assert.equal(expected, test[testFunctionName](expected));
		})
	    }
	})
    })
})

describe('testFloatBypassFunction', function() {
    for (var n = 0; n < numRepeat; n++) {
	var maxValue = Math.pow(10, 32);
	var minValue = -Math.pow(10, 32);
	var expected = getRandomInt(minValue, maxValue);
	it('should return true', function() {
	    assert(almostEqual(expected,
			       test.testFloatBypassFunction(expected),
			       almostEqual.FLT_EPSILON,
			       almostEqual.FLT_EPSILON));
	})	
    }
})

describe('testDoubleBypassFunction', function() {
    for (var n = 0; n < numRepeat; n++) {
	var maxValue = Math.pow(10, 32);
	var minValue = -Math.pow(10, 32);
	var expected = getRandomInt(minValue, maxValue);
	it('should return true', function() {
	    assert(almostEqual(expected,
			       test.testDoubleBypassFunction(expected),
			       almostEqual.DBL_EPSILON,
			       almostEqual.DBL_EPSILON));
	})	
    }
})
