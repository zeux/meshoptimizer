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
	log: function (channel, data) {
		(channel == 2 ? process.stderr : process.stdout).write(data);
	},
	execute: function (command) {
		// perform substitution of command executable with environment-specific paths
		var pk = Object.keys(paths);
		for (var pi = 0; pi < pk.length; ++pi) {
			if (command.startsWith(pk[pi] + " ")) {
				command = paths[pk[pi]] + command.substr(pk[pi].length);
				break;
			}
		}

		var ret = cp.spawnSync(command, [], {shell:true});
		return ret.status == null ? 256 : ret.status;
	},
	unlink: function (path) {
		fs.unlinkSync(path);
	},
};

gltfpack.init(fs.readFileSync(__dirname + '/library.wasm'));
gltfpack.pack(args, interface)
	.then(function () {
		process.exit(0);
	})
	.catch(function (err) {
		process.exit(1);
	});
