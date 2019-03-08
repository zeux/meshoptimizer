// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2019, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
function MeshoptDecoder() {
  var Module = {}

  var wasm = "WASMHERE";

  var page = 65536;
  var memory = new WebAssembly.Memory({
    initial: 1
  });
  var heap = new Uint8Array(memory.buffer);
  var brk = 32768;

  var sbrk = function(size) {
    var old = brk;
    brk += size;
    if (brk > heap.length) {
      memory.grow(Math.ceil((brk - heap.length) / page));
      heap = new Uint8Array(memory.buffer);
    }
    return old;
  };

  var imports = {
    env: {
      memory: memory,
      _emscripten_memcpy_big: function(d, s, n) {
        heap.set(heap.subarray(s, s + n), d);
      },
    }
  };

  var exports = {};

  var source = fetch('data:application/octet-stream;base64,' + wasm);
  var instance = source
    .then(response => response.arrayBuffer())
    .then(bytes => WebAssembly.instantiate(bytes, imports))
    .then(result => exports = result.instance.exports);

  Module.then = function(callback) {
    instance.then(callback);
  };

  Module.decodeVertexBuffer = function(target, count, size, source) {
    var tp = sbrk(count * size);
    var sp = sbrk(source.length);
    heap.set(source, sp);
    var res = exports["_meshopt_decodeVertexBuffer"](tp, count, size, sp, source.length);
    target.set(heap.subarray(tp, tp + count * size), 0);
    sbrk(tp - sbrk(0));
    if (res != 0) throw new Error("Malformed vertex buffer data");
  };

  Module.decodeIndexBuffer = function(target, count, size, source) {
    var tp = sbrk(count * size);
    var sp = sbrk(source.length);
    heap.set(source, sp);
    var res = exports["_meshopt_decodeIndexBuffer"](tp, count, size, sp, source.length);
    target.set(heap.subarray(tp, tp + count * size), 0);
    sbrk(tp - sbrk(0));
    if (res != 0) throw new Error("Malformed index buffer data");
  };

  return Module;
}