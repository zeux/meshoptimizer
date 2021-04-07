#!/usr/bin/env node
// This file is part of gltfpack and is distributed under the terms of MIT License.
var gltfpack = require('./library.js');

var fs = require('fs');
var cp = require('child_process');

var args = process.argv.slice(2);

var paths = {
	"basisu": process.env["BASISU_PATH"],
	"toktx": process.env["TOKTX_PATH"],
};

var interface = {
	read: function (path) {
		return fs.readFileSync(path);
	},
	write: function (path, data) {
		fs.writeFileSync(path, data);
	},
	execute: function (command) {
		var arg = command.split(' ');
		var exe = arg.shift();

		// perform substitution of command executable with environment-specific paths
		exe = paths[exe] || exe;

		var ret = cp.spawnSync(exe, arg);
		return ret.status == null ? 256 : ret.status;
	},
	unlink: function (path) {
		fs.unlinkSync(path);
	},
};

gltfpack.pack(args, interface)
	.then(function (log) {
		process.stdout.write(log);
		process.exit(0);
	})
	.catch(function (err) {
		process.stderr.write(err.message);
		process.exit(1);
	});
