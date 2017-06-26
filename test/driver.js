var Mocha = require('mocha'),
    fs = require('fs'),
    path = require('path');

var mocha = new Mocha();

fs.readdirSync(__dirname).filter(function(file) {
    if (file == 'driver.js') {
	return false;
    } else {
	return file.endsWith('.js');
    }
}).forEach(function(file) {
    mocha.addFile(path.join(__dirname, file));
})

mocha.run()				 
