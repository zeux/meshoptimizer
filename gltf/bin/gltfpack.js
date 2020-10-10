var fs = require('fs');

var WASI_EBADF = 8;
var WASI_EINVAL = 28;
var WASI_ENOSYS = 52;

var instance;

function getHeap() {
	return new DataView(instance.exports.memory.buffer);
}

var fds = {
	0: { fd: 0 },
	1: { fd: 1 },
	2: { fd: 2 },
	3: null // fake fd for directory
};

var args = process.argv.slice(1);

function nextFd() {
	for (var i = 0; ; ++i) {
		if (fds[i] === undefined) {
			return i;
		}
	}
}

var wasi = {
	proc_exit: function(rval) {
		process.exit(rval);
	},

	fd_close: function(fd) {
		if (!fds[fd]) {
			return WASI_EBADF;
		}

		fs.closeSync(fds[fd].fd);
		fds[fd] = undefined;
		return 0;
	},

	fd_fdstat_get: function(fd, stat) {
		if (fds[fd] === undefined) {
			return WASI_EBADF;
		}

		var heap = getHeap();

		heap.setUint8(stat + 0, fds[fd] === null ? 3 : 4); // directory
		heap.setUint16(stat + 2, 0, true);
		heap.setUint32(stat + 8, 0, true);
		heap.setUint32(stat + 12, 0, true);
		heap.setUint32(stat + 16, 0, true);
		heap.setUint32(stat + 20, 0, true);
		return 0;
	},

	path_open32: function(parent_fd, dirflags, path, path_len, oflags, fs_rights_base, fs_rights_inheriting, fdflags, opened_fd) {
		if (fds[parent_fd] !== null) {
			return WASI_EBADF;
		}

		var heap = getHeap();

		var path_name = Buffer.from(heap.buffer, path, path_len).toString('utf-8');
		var flags = (oflags & 1) ? 'w' : 'r';

		try {
			var real_fd = fs.openSync(path_name, flags);
			var stat = fs.fstatSync(real_fd);

			var fd = nextFd();
			fds[fd] = { fd: real_fd, position: 0, size: stat.size };

			heap.setUint32(opened_fd, fd, true);
			return 0;
		} catch (err) {
			return WASI_ENOSYS;
		}
	},

	path_unlink_file: function(parent_fd, path, path_len) {
		return WASI_EINVAL;
	},

	args_sizes_get: function(argc, argv_buf_size) {
		var heap = getHeap();

		var buf_size = 0;
		for (var i = 0; i < args.length; ++i) {
			buf_size += Buffer.from(args[i], 'utf-8').length + 1;
		}

		heap.setUint32(argc, args.length, true);
		heap.setUint32(argv_buf_size, buf_size, true);
		return 0;
	},

	args_get: function(argv, argv_buf) {
		var heap = getHeap();
		var memory = new Uint8Array(heap.buffer);

		var argp = argv_buf;

		for (var i = 0; i < args.length; ++i) {
			var au = Buffer.from(args[i], 'utf-8');

			heap.setUint32(argv + i * 4, argp, true);
			au.copy(memory, argp);
			heap.setUint8(argp + au.length, 0);

			argp += au.length + 1;
		}

		return 0;
	},

	fd_prestat_get: function(fd, buf) {
		var heap = getHeap();

		if (fd == 3) {
			heap.setUint8(buf, 0);
			heap.setUint32(buf + 4, 0, true);
			return 0;
		} else {
			return WASI_EBADF;
		}
	},

	fd_prestat_dir_name: function(fd, path, path_len) {
		return 0;
	},

	path_remove_directory: function(parent_fd, path, path_len) {
		return WASI_EINVAL;
	},

	environ_sizes_get: function(environc, environ_buf_size) {
		var heap = getHeap();
		heap.setUint32(environc, 0, true);
		heap.setUint32(environ_buf_size, 0, true);
		return 0;
	},

	environ_get: function(environ, environ_buf) {
		return 0;
	},

	fd_fdstat_set_flags: function(fd, flags) {
		return WASI_ENOSYS;
	},

	fd_seek32: function(fd, offset, whence, newoffset) {
		if (!fds[fd]) {
			return WASI_EBADF;
		}

		var heap = getHeap();

		switch (whence) {
		case 0:
			fds[fd].position = offset;
			break;

		case 1:
			fds[fd].position += offset;
			break;

		case 2:
			fds[fd].position = fds[fd].size;
			break;

		default:
			return WASI_EINVAL;
		}

		heap.setUint32(newoffset, fds[fd].position, true);
		return 0;
	},

	fd_read: function(fd, iovs, iovs_len, nread) {
		if (!fds[fd]) {
			return WASI_EBADF;
		}

		var heap = getHeap();
		var read = 0;

		for (var i = 0; i < iovs_len; ++i) {
			var buf = heap.getUint32(iovs + 8 * i + 0, true);
			var buf_len = heap.getUint32(iovs + 8 * i + 4, true);

			var readi = fs.readSync(fds[fd].fd, heap, buf, buf_len, fds[fd].position);

			fds[fd].position += readi;
			read += readi;
		}

		heap.setUint32(nread, read, true);
		return 0;
	},

	fd_write: function(fd, iovs, iovs_len, nwritten) {
		var heap = getHeap();
		var written = 0;

		for (var i = 0; i < iovs_len; ++i) {
			var buf = heap.getUint32(iovs + 8 * i + 0, true);
			var buf_len = heap.getUint32(iovs + 8 * i + 4, true);

			var writei = fs.writeSync(fds[fd].fd, heap, buf, buf_len, fds[fd].position);

			fds[fd].position += writei;
			written += writei;
		}

		heap.setUint32(nwritten, written, true);
		return 0;
	},
};

var wasm = fs.readFileSync(__dirname + '/gltfpack.wasm');

WebAssembly.instantiate(wasm, { wasi_snapshot_preview1: wasi })
.then(function (result) {
	instance = result.instance;
	instance.exports._start();
});
