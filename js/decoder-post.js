Module.decodeVertexBuffer = function (target, targetOffset, vertexCount, vertexSize, source)
{
	var tp = Module._malloc(vertexCount * vertexSize);
	var sp = Module._malloc(source.length);

	Module.HEAPU8.set(source, sp);

	var res = Module._meshopt_decodeVertexBuffer(tp, vertexCount, vertexSize, sp, source.length);

	target.set(Module.HEAPU8.subarray(tp, tp + vertexCount * vertexSize), targetOffset);

	Module._free(sp);
	Module._free(tp);

	if (res != 0)
		throw new Error("Malformed vertex buffer data");
}

Module.decodeIndexBuffer = function(target, targetOffset, indexCount, indexSize, source)
{
	var tp = Module._malloc(indexCount * indexSize);
	var sp = Module._malloc(source.length);

	Module.HEAPU8.set(source, sp);

	var res = Module._meshopt_decodeIndexBuffer(tp, indexCount, indexSize, sp, source.length);

	target.set(Module.HEAPU8.subarray(tp, tp + indexCount * indexSize), targetOffset);

	Module._free(sp);
	Module._free(tp);

	if (res != 0)
		throw new Error("Malformed index buffer data");
}
