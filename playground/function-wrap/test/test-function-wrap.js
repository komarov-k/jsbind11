const assert = require('assert');
const funcWrap = require('../function-wrap.js');

function testAdd()
{
    const a = 3;
    const b = 7;

    assert.strictEqual(funcWrap.add(a, b), a + b);
}

function testMul()
{
    const a = 3;
    const b = 7;

    assert.strictEqual(funcWrap.mul(a, b), a * b);
}

testAdd();
testMul();

console.log("Tests passed- everything looks OK!");
