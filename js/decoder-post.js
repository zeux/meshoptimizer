Module['decodeVertexBuffer'] = function (targetArray, vertexCount, vertexSize, sourceArray)
{
	var tp = Module["_sbrk"](vertexCount * vertexSize);
	var sp = Module["_sbrk"](sourceArray.length);

	Module["HEAPU8"].set(sourceArray, sp);

	var res = Module["_meshopt_decodeVertexBuffer"](tp, vertexCount, vertexSize, sp, sourceArray.length);

	targetArray.set(Module["HEAPU8"].subarray(tp, tp + vertexCount * vertexSize), 0);

	Module["_sbrk"](tp - Module["_sbrk"](0));

	if (res != 0)
		throw new Error("Malformed vertex buffer data");
}

Module['decodeIndexBuffer'] = function(targetArray, indexCount, indexSize, sourceArray)
{
	var tp = Module["_sbrk"](indexCount * indexSize);
	var sp = Module["_sbrk"](sourceArray.length);

	Module["HEAPU8"].set(sourceArray, sp);

	var res = Module["_meshopt_decodeIndexBuffer"](tp, indexCount, indexSize, sp, sourceArray.length);

	targetArray.set(Module["HEAPU8"].subarray(tp, tp + indexCount * indexSize), 0);

	Module["_sbrk"](tp - Module["_sbrk"](0));

	if (res != 0)
		throw new Error("Malformed index buffer data");
}
