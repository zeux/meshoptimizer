// Usage:
// 1. import { wasi_trace } from './wasi_trace.js';
// 2. Pass wasi_trace as an import object to WebAssembly.instantiate
// 3. Call wasi_trace.init(instance) after instantiation

var instance;

var wasi_snapshot_preview1 = {
	fd_close: function () {
		return 8;
	},
	fd_seek: function () {
		return 8;
	},
	fd_fdstat_get: function (fd, stat) {
		// needed for isatty() to enable line buffering for stdout
		var heap = new DataView(instance.exports.memory.buffer);
		heap.setUint8(stat, 2);
		for (var i = 1; i < 24; ++i) heap.setUint8(stat + i, 0);
		return 0;
	},
	fd_write: function (fd, iovs, iovs_len, nwritten) {
		var heap = new DataView(instance.exports.memory.buffer);
		var written = 0;
		var str = '';

		for (var i = 0; i < iovs_len; ++i) {
			var buf = heap.getUint32(iovs + 8 * i + 0, true);
			var buf_len = heap.getUint32(iovs + 8 * i + 4, true);

			var buf_data = new Uint8Array(heap.buffer, buf, buf_len);

			for (var j = 0; j < buf_data.length; ++j) {
				str += String.fromCharCode(buf_data[j]);
			}
			written += buf_len;
		}

		console.log(str);

		heap.setUint32(nwritten, written, true);
		return 0;
	},
};

var wasi_trace = {
	wasi_snapshot_preview1,

	init: function (inst) {
		instance = inst;
	},
};

export { wasi_trace };
