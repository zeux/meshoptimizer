#!/usr/bin/env node
var gltfpack = require('./library.js');

var args = process.argv.slice(2);

gltfpack.pack(args).then(function (result) {
	process.exit(result);
});
