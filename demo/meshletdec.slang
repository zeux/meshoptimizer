/**
 * meshletdec.slang - an example GPU decoder for meshlet data encoded using meshopt_encodeMeshlet
 * This is intended to be used as a starting point for applications that want to decode meshlet data on the GPU.
 *
 * The shader exposes an entrypoint, decodeMeshlets, that decodes a set of meshlets; each meshlet is decoded independently,
 * and the output vertex/triangle data is written as uint32 per element (triangle data is written as 0xccbbaa).
 * This matches the output format for meshopt_decodeMeshlet with vertex_size=4 triangle_size=4. If alternative formats are
 * needed, the code should be changed to output them; note that for triangle data, it may make sense to output data to shared
 * memory to be able to use larger aligned 32-bit writes to global memory after that.
 *
 * Copyright (C) 2016-2026, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * This code is distributed under the MIT License. See notice at the end of this file.
 */

struct MeshletDesc
{
	uint stream_offset;
	uint output_offset;
	uint16_t encoded_size;
	uint8_t vertex_count;
	uint8_t triangle_count;
};

[[vk::binding(0)]]
StructuredBuffer<uint8_t> gStream : register(t0);
[[vk::binding(1)]]
StructuredBuffer<MeshletDesc> gMeshlets : register(t1);
[[vk::binding(2)]]
RWStructuredBuffer<uint> gOutput : register(u2);
[[vk::binding(3)]]
cbuffer MeshletConfigCB : register(b3) { uint gMeshletCount; }

uint decodeVertices(uint out_vertices, uint ctrl, uint data, uint bound, uint vertex_count)
{
	uint last = ~0u;

	for (uint i = 0; i < vertex_count; i += 4)
	{
		if (data > bound)
			return ~0u;

		uint code4 = uint(gStream[ctrl + i / 4]);

		for (int k = 0; k < 4; ++k)
		{
			int code = ((code4 >> k) & 1) | ((code4 >> (k + 3)) & 2);
			int length = code4 == 0xff ? 4 : code;

			// branchlessly read up to 4 bytes
			uint mask = (length == 4) ? ~0u : (1 << (8 * length)) - 1;
			uint v = (uint(gStream[data + 0]) | (uint(gStream[data + 1]) << 8) | (uint(gStream[data + 2]) << 16) | (uint(gStream[data + 3]) << 24)) & mask;

			// unzigzag + 1
			uint d = (v >> 1) ^ -int(v & 1);
			uint r = last + d + 1;

			if (i + k < vertex_count)
				gOutput[out_vertices + i + k] = r;

			data += length;
			last = r;
		}
	}

	return data;
}

uint decodeTriangle(uint code, uint extra0, uint extra1, uint extra2, inout uint fifo0, inout uint fifo1, inout uint fifo2, inout uint next, inout uint extra)
{
	// reuse: 0-1 extra vertices
	uint fifo = code < 4 ? fifo0 : (code < 8 ? fifo1 : fifo2);
	uint edge = fifo >> ((code << 3) & 16); // shift by 16 if bit 1 is set (odd edge for each triangle)
	uint c_reuse = (code & 1) == 1 ? extra0 : next;

	// restart: 0-3 extra vertices
	uint extran = code & 3;
	uint a = extran > 0 ? extra0 : next;
	uint b = extran > 1 ? extra1 : next + (1 - extran);
	uint c = extran > 2 ? extra2 : next + (2 - extran);

	// select between reuse and restart and repack triangle into edge format (0xcbac)
	a = code >= 12 ? a : (edge >> 8) & 0xff;
	b = code >= 12 ? b : edge & 0xff;
	c = code >= 12 ? c : c_reuse;

	uint tri = c | (a << 8) | (b << 16) | (c << 24);

	// advance next/extra; reuse codes use 1 lsb for extra count, restart codes use 2 lsbs
	uint extrab = code < 12 ? 1 : 3;
	next += extrab - code & extrab;
	extra += code & extrab;

	// rotate fifo
	fifo2 = fifo1;
	fifo1 = fifo0;
	fifo0 = tri;

	// output triangle is stored without extra edge vertex (0xcbac => 0xcba)
	return tri >> 8;
}

uint decodeTriangles(uint out_triangles, uint codes, uint extra, uint bound, uint triangle_count)
{
	uint next = 0;
	uint fifo0 = 0, fifo1 = 0, fifo2 = 0; // two edge fifo entries in one uint: 0xcbac

	for (uint i = 0; i < triangle_count; i += 2)
	{
		if (extra > bound)
			return ~0u;

		uint codeg = uint(gStream[codes + i / 2]);

		// first triangle
		uint extra0 = uint(gStream[extra + 0]);
		uint extra1 = uint(gStream[extra + 1]);
		uint extra2 = uint(gStream[extra + 2]);
		uint tri = decodeTriangle(codeg & 15, extra0, extra1, extra2, fifo0, fifo1, fifo2, next, extra);

		gOutput[out_triangles + i] = tri;

		// second triangle, if any
		extra0 = uint(gStream[extra + 0]);
		extra1 = uint(gStream[extra + 1]);
		extra2 = uint(gStream[extra + 2]);
		tri = decodeTriangle(codeg >> 4, extra0, extra1, extra2, fifo0, fifo1, fifo2, next, extra);

		if (i + 1 < triangle_count)
			gOutput[out_triangles + i + 1] = tri;
	}

	return extra;
}

int decodeMeshlet(uint out_vertices, uint vertex_count, uint out_triangles, uint triangle_count, uint buffer, uint buffer_size)
{
	uint codes_size = (triangle_count + 1) / 2;
	uint ctrl_size = (vertex_count + 3) / 4;
	uint gap_size = (codes_size + ctrl_size < 16) ? 16 - (codes_size + ctrl_size) : 0;

	if (buffer_size < codes_size + ctrl_size + gap_size)
		return -2;

	uint end = buffer + buffer_size;
	uint codes = end - codes_size;
	uint ctrl = codes - ctrl_size;
	uint data = buffer;

	// gap ensures we have at least 16 bytes available after bound; this allows decoder to over-read safely
	uint bound = ctrl - gap_size;

	data = decodeVertices(out_vertices, ctrl, data, bound, vertex_count);
	if (data == ~0u)
		return -2;

	data = decodeTriangles(out_triangles, codes, data, bound, triangle_count);
	if (data == ~0u)
		return -2;

	return (data == bound) ? 0 : -3;
}

[shader("compute")]
[numthreads(32, 1, 1)]
void decodeMeshlets(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
	uint meshlet_count = gMeshletCount;

	uint tid = dispatch_thread_id.x;
	if (tid >= meshlet_count)
		return;

	MeshletDesc desc = gMeshlets[tid];
	uint out_vertices = desc.output_offset;
	uint out_triangles = desc.output_offset + uint(desc.vertex_count);

	int rc = decodeMeshlet(out_vertices, uint(desc.vertex_count), out_triangles, uint(desc.triangle_count), desc.stream_offset, uint(desc.encoded_size));

	// if decoding failed, we write 0xff.. to the first word of the output data
	// this can be adjusted arbitrarily; for example, a separate buffer with a single status for the entire stream could be used
	// note that decoding fails only if the input data is corrupt; so this may not be required at all depending on the requirements
	if (rc < 0)
		gOutput[desc.output_offset] = ~0u;
}

/**
 * Copyright (c) 2016-2026 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
