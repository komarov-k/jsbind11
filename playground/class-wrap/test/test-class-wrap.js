const assert = require('assert');
const classWrap = require('../class-wrap.js');

function testAdder()
{
    const a = 3;
    const b = 7;

    var adder = classWrap.Adder(a, b);
    
    assert.strictEqual(adder.getValue(), a + b);
    assert.strictEqual(adder.getDoubleValue(), a + b);
}

testAdder();

console.log("Tests passed- everything looks OK!");
