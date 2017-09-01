#ifdef _WIN32
#include <assert.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>

#include <cassert>
#include <cmath>

#include <algorithm>
#include <vector>

#include "../src/meshoptimizer.h"
#include "objparser.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

void stripGen(std::vector<unsigned int>& indices, int x0, int x1, int y0, int y1, int width, bool prefetch)
{
	if (prefetch)
	{
		for (int x = x0; x < x1; x++)
		{
			indices.push_back(x + 0);
			indices.push_back(x + 0);
			indices.push_back(x + 1);
		}
	}

	for (int y = y0; y < y1; y++)
	{
		for (int x = x0; x < x1; x++)
		{
			indices.push_back((width + 1) * (y + 0) + (x + 0));
			indices.push_back((width + 1) * (y + 1) + (x + 0));
			indices.push_back((width + 1) * (y + 0) + (x + 1));

			indices.push_back((width + 1) * (y + 0) + (x + 1));
			indices.push_back((width + 1) * (y + 1) + (x + 0));
			indices.push_back((width + 1) * (y + 1) + (x + 1));
		}
	}
}

void gridGen(std::vector<unsigned int>& indices, int x0, int x1, int y0, int y1, int width, int cacheSize, bool prefetch)
{
	if (x1 - x0 + 1 < cacheSize)
	{
		bool prefetchStrip = 2 * (x1 - x0) + 1 > cacheSize && prefetch;

		stripGen(indices, x0, x1, y0, y1, width, prefetchStrip);
	}
	else
	{
		int xm = x0 + cacheSize - 2;
		gridGen(indices, x0, xm, y0, y1, width, cacheSize, prefetch);
		gridGen(indices, xm, x1, y0, y1, width, cacheSize, prefetch);
	}
}

unsigned int queryVSInvocations(ID3D11Device* device, ID3D11DeviceContext* context, const unsigned int* indices, size_t index_count)
{
	if (index_count == 0)
		return 0;

	ID3D11Buffer* ib = 0;

	{
		D3D11_BUFFER_DESC bd = {};

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = index_count * 4;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		device->CreateBuffer(&bd, 0, &ib);

		D3D11_MAPPED_SUBRESOURCE ms;
		context->Map(ib, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, indices, index_count * 4);
		context->Unmap(ib, 0);
	}

	context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

	D3D11_QUERY_DESC qdesc = {D3D11_QUERY_PIPELINE_STATISTICS};
	ID3D11Query* query = 0;
	device->CreateQuery(&qdesc, &query);

	context->Begin(query);
	context->DrawIndexed(index_count, 0, 0);
	context->End(query);

	D3D11_QUERY_DATA_PIPELINE_STATISTICS stats = {};
	while (S_FALSE == context->GetData(query, &stats, sizeof(stats), 0))
		;

	query->Release();
	ib->Release();

	assert(stats.IAVertices == index_count);

	return stats.VSInvocations;
}

unsigned int estimateVSInvocationsFIFO(const unsigned int* indices, size_t index_count, size_t cache_size)
{
	std::vector<unsigned int> cache;

	unsigned int invocations = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];

		bool found = false;

		for (size_t j = 0; j < cache.size(); ++j)
		{
			if (cache[j] == index)
			{
				found = true;
				break;
			}
		}

		if (!found)
			invocations++;

		if (!found)
			cache.push_back(index);

		if (cache.size() > cache_size)
			cache.erase(cache.begin(), cache.end() - cache_size);
	}

	return invocations;
}

unsigned int estimateVSInvocationsLRU(const unsigned int* indices, size_t index_count, size_t cache_size)
{
	std::vector<unsigned int> cache;

	unsigned int invocations = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];

		bool found = false;

		for (size_t j = 0; j < cache.size(); ++j)
		{
			if (cache[j] == index)
			{
				found = true;
				cache.erase(cache.begin() + j);
				break;
			}
		}

		if (!found)
			invocations++;

		cache.push_back(index);

		if (cache.size() > cache_size)
			cache.erase(cache.begin(), cache.end() - cache_size);
	}

	return invocations;
}

unsigned int estimateVSInvocationsReuse(const unsigned int* indices, size_t index_count, size_t warp_size, size_t fifo_size, size_t triangle_limit)
{
	std::vector<unsigned int> warp;

	unsigned int invocations = 0;
	unsigned int triangles = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		bool found[3] = {};

		for (size_t k = 0; k < 3; ++k)
		{
			for (size_t j = 0; j < warp.size(); ++j)
			{
				if (warp.size() - j > fifo_size)
					continue;

				if (warp[j] == indices[i + k])
				{
					found[k] = true;
					break;
				}
			}
		}

		// triangle doesn't fit into warp we're currently building - flush
		if (warp.size() + !found[0] + !found[1] + !found[2] > warp_size || triangles >= triangle_limit)
		{
			invocations += warp.size();
			warp.clear();

			// restart warp from scratch
			warp.push_back(indices[i + 0]);
			warp.push_back(indices[i + 1]);
			warp.push_back(indices[i + 2]);
			triangles = 1;
		}
		else
		{
			if (!found[0])
				warp.push_back(indices[i + 0]);
			if (!found[1])
				warp.push_back(indices[i + 1]);
			if (!found[2])
				warp.push_back(indices[i + 2]);
			triangles++;
		}
	}

	// last warp
	invocations += warp.size();

	return invocations;
}

void setupShaders(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// load and compile the two shaders
	const char* shaders =
	    "#define ATTRIBUTES 5\n"
	    "struct Foo { float4 v[ATTRIBUTES]; };"
	    "float4 VS(uint index: SV_VertexId, out Foo foo: FOO): SV_Position { uint i = index % 3; [unroll] for (int j = 0; j < ATTRIBUTES; j++) foo.v[j] = j; return float4(i != 0, i != 2, 0, 1); }"
	    "float4 PS(Foo foo: FOO): SV_Target { float4 result = 0; [unroll] for (int j = 0; j < ATTRIBUTES; j++) result += foo.v[j]; return result; }";

	ID3DBlob* vsblob = 0;
	ID3DBlob* psblob = 0;
	D3DCompile(shaders, strlen(shaders), 0, 0, 0, "VS", "vs_5_0", 0, 0, &vsblob, 0);
	D3DCompile(shaders, strlen(shaders), 0, 0, 0, "PS", "ps_5_0", 0, 0, &psblob, 0);

	ID3D11VertexShader* vs = 0;
	ID3D11PixelShader* ps = 0;
	device->CreateVertexShader(vsblob->GetBufferPointer(), vsblob->GetBufferSize(), 0, &vs);
	device->CreatePixelShader(psblob->GetBufferPointer(), psblob->GetBufferSize(), 0, &ps);

	context->VSSetShader(vs, 0, 0);
	context->PSSetShader(ps, 0, 0);
}

template <typename Cache>
void inspectCache(Cache cache)
{
	unsigned int max_cache_size = 200;
	unsigned int grid_size = 100;

	for (unsigned int cache_size = 3; cache_size <= max_cache_size; cache_size += 1)
	{
		std::vector<unsigned int> grid1;
		gridGen(grid1, 0, grid_size, 0, grid_size, grid_size, cache_size, true);

		std::vector<unsigned int> grid2;
		gridGen(grid2, 0, grid_size, 0, grid_size, grid_size, cache_size, false);

		std::vector<unsigned int> grid3;
		gridGen(grid3, 0, grid_size, 0, grid_size, grid_size, grid_size * 4, false); // this generates a simple indexed grid without striping/degenerate triangles
		meshopt::optimizeVertexCache(&grid3[0], &grid3[0], grid3.size(), (grid_size + 1) * (grid_size + 1), cache_size);

		std::vector<unsigned int> grid4;
		gridGen(grid4, 0, grid_size, 0, grid_size, grid_size, grid_size * 4, false); // this generates a simple indexed grid without striping/degenerate triangles
		meshopt::optimizeVertexCache(&grid4[0], &grid4[0], grid4.size(), (grid_size + 1) * (grid_size + 1), 0);

		unsigned int invocations1 = cache(&grid1[0], grid1.size());
		unsigned int invocations2 = cache(&grid2[0], grid2.size());
		unsigned int invocations3 = cache(&grid3[0], grid3.size());
		unsigned int invocations4 = cache(&grid4[0], grid4.size());

		unsigned int ideal_invocations = (grid_size + 1) * (grid_size + 1);

		printf("%d, %f, %f, %f, %f\n", cache_size,
		       double(invocations1) / double(ideal_invocations),
		       double(invocations2) / double(ideal_invocations),
		       double(invocations3) / double(ideal_invocations),
		       double(invocations4) / double(ideal_invocations));
	}
}

void testCache(IDXGIAdapter* adapter)
{
	ID3D11Device* device = 0;
	ID3D11DeviceContext* context = 0;
	D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, 0, 0, D3D11_SDK_VERSION, &device, 0, &context);

	setupShaders(device, context);

	inspectCache([&](const unsigned int* indices, size_t index_count) { return queryVSInvocations(device, context, indices, index_count); });

	unsigned int dib[] = {0, 1, 1, 2, 3, 4, 5, 5, 5};

	printf("// Degenerate IB invocations: %d\n", queryVSInvocations(device, context, dib, sizeof(dib) / sizeof(dib[0])));

	printf("// Reuse boundaries: ");

	unsigned int last = 0;

	for (unsigned int count = 3; count < 1000; count += 3)
	{
		std::vector<unsigned int> ib;

		for (unsigned int i = 0; i < count; i += 3)
		{
			ib.push_back(0);
			ib.push_back(1);
			ib.push_back(2);
		}

		unsigned int invocations = queryVSInvocations(device, context, ib.data(), ib.size());

		if (invocations != last)
		{
			printf("%d, ", count);
			last = invocations;
		}
	}

	printf("\n");
}

void testCacheSequence(IDXGIAdapter* adapter, int argc, char** argv)
{
	ID3D11Device* device = 0;
	ID3D11DeviceContext* context = 0;
	D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, 0, 0, D3D11_SDK_VERSION, &device, 0, &context);

	setupShaders(device, context);

	std::vector<unsigned int> ib;

	for (int i = 2; i < argc; ++i)
	{
		char* end;
		int i0 = strtol(argv[i], &end, 10);

		if (end[0] == '-')
		{
			int i1 = strtol(end + 1, &end, 10);

			if (end[0] != 0)
			{
				printf("Unrecognized index range: %s\n", argv[i]);
				return;
			}

			if (i0 < i1)
			{
				for (int ii = i0; ii <= i1; ++ii)
					ib.push_back(ii);
			}
			else
			{
				for (int ii = i0; ii >= i1; --ii)
					ib.push_back(ii);
			}
		}
		else if (end[0] == '*')
		{
			int i1 = strtol(end + 1, &end, 10);

			if (end[0] != 0 || i1 == 0)
			{
				printf("Unrecognized index range: %s\n", argv[i]);
				return;
			}

			for (int ii = 0; ii < i1; ++ii)
				ib.push_back(i0);
		}
		else if (end[0] == 'x')
		{
			int i1 = strtol(end + 1, &end, 10);

			if (end[0] != 0)
			{
				printf("Unrecognized index range: %s\n", argv[i]);
				return;
			}

			stripGen(ib, 0, i0, 0, i1, i0, true);
		}
		else if (end[0] == 0)
		{
			ib.push_back(i0);
		}
		else
		{
			printf("Unrecognized index range: %s\n", argv[i]);
			return;
		}
	}

	if (ib.size() % 3)
		ib.resize(ib.size() - ib.size() % 3);

	std::vector<bool> xformed(ib.size());

	for (size_t i = 0; i < ib.size(); i += 3)
	{
		unsigned int inv0 = i == 0 ? 0 : queryVSInvocations(device, context, ib.data(), i);
		unsigned int inv1 = queryVSInvocations(device, context, ib.data(), i + 3);

		unsigned int a = ib[i + 0];
		unsigned int b = ib[i + 1];
		unsigned int c = ib[i + 2];

		ib[i + 0] = ib[i + 1] = ib[i + 2] = a;
		unsigned int inva = queryVSInvocations(device, context, ib.data(), i + 3);

		ib[i + 1] = ib[i + 2] = b;
		unsigned int invb = queryVSInvocations(device, context, ib.data(), i + 3);

		ib[i + 2] = c;
		unsigned int invc = queryVSInvocations(device, context, ib.data(), i + 3);

		if (inva < inv0 || invb < inva || invc < invb || inv1 < invc)
		{
			printf("desync at %d\n", int(i));
			break;
		}

		xformed[i + 0] = inva == inv0 + 1;
		xformed[i + 1] = invb == inva + 1;
		xformed[i + 2] = invc == invb + 1;
	}

	printf("// Sequence:");

	for (size_t i = 0; i < ib.size(); ++i)
	{
		if (xformed[i])
			printf(" +%d", ib[i]);
		else
			printf(" %d", ib[i]);
	}

	printf("\n");

	std::vector<unsigned int> cached;

	for (size_t i = 0; i < ib.size(); ++i)
	{
		unsigned int index = ib[i];
		unsigned int inv0 = queryVSInvocations(device, context, ib.data(), ib.size());

		ib.push_back(index);
		ib.push_back(index);
		ib.push_back(index);

		unsigned int inv1 = queryVSInvocations(device, context, ib.data(), ib.size());

		ib.resize(ib.size() - 3);

		if (inv1 == inv0)
			cached.push_back(index);
	}

	std::sort(cached.begin(), cached.end());
	cached.erase(std::unique(cached.begin(), cached.end()), cached.end());

	printf("// Cached  :");

	for (size_t i = 0; i < cached.size(); ++i)
		printf(" %d", cached[i]);

	printf(" (%d)\n", int(cached.size()));

	printf("// Invocations: %d\n", queryVSInvocations(device, context, ib.data(), ib.size()));
}
void testCacheMeshes(IDXGIAdapter* adapter, int argc, char** argv)
{
	ID3D11Device* device = 0;
	ID3D11DeviceContext* context = 0;
	D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, 0, 0, D3D11_SDK_VERSION, &device, 0, &context);

	setupShaders(device, context);

	bool stat = false;

	double atvr_sum = 0;
	double atvr_count = 0;

	unsigned int total_invocations = 0;
	unsigned int total_vertices = 0;

	for (int i = 1; i < argc; ++i)
	{
		const char* path = argv[i];

		if (strcmp(path, "--stat") == 0)
		{
			stat = true;
			continue;
		}

		ObjFile file;

		if (!objParseFile(file, path))
		{
			printf("Error loading %s: file not found\n", path);
			continue;
		}

		if (!objValidate(file))
		{
			printf("Error loading %s: invalid file data\n", path);
			continue;
		}

		objTriangulate(file);

		std::vector<unsigned int> ib1;

		for (size_t i = 0; i < file.f.size(); i += 3)
			ib1.push_back(file.f[i]);

		unsigned int vertex_count = file.v.size() / 3;
		unsigned int index_count = ib1.size();

		unsigned int invocations1 = queryVSInvocations(device, context, ib1.data(), index_count);

		if (stat)
		{
			std::vector<unsigned int> ib2(ib1.size());
			meshopt::optimizeVertexCache(&ib2[0], &ib1[0], ib1.size(), vertex_count, 0);

			unsigned int invocations = queryVSInvocations(device, context, ib2.data(), index_count);

			atvr_sum += double(invocations) / double(vertex_count);
			atvr_count += 1;

			total_invocations += invocations;
			total_vertices += vertex_count;
		}
		else
		{
			printf("%s: baseline %f\n", path, double(invocations1) / double(vertex_count));

			for (unsigned int cache_size = 14; cache_size <= 24; ++cache_size)
			{
				std::vector<unsigned int> ib2(ib1.size());
				meshopt::optimizeVertexCache(&ib2[0], &ib1[0], ib1.size(), vertex_count, cache_size);

				std::vector<unsigned int> ib3(ib1.size());
				meshopt::optimizeVertexCache(&ib3[0], &ib1[0], ib1.size(), vertex_count, 0);

				unsigned int invocations2 = queryVSInvocations(device, context, ib2.data(), index_count);
				unsigned int invocations3 = queryVSInvocations(device, context, ib3.data(), index_count);

				printf("%s: %d tipsify %f tomf %f\n", path, cache_size, double(invocations2) / double(vertex_count), double(invocations3) / double(vertex_count));
			}
		}
	}

	if (stat)
	{
		printf("ATVR: average %f cumulative %f; %d vertices\n", atvr_sum / atvr_count, double(total_invocations) / double(total_vertices), total_vertices);
	}
}

void modelCacheFIFO(size_t model_cache_size)
{
	inspectCache([&](const unsigned int* indices, size_t index_count) { return estimateVSInvocationsFIFO(indices, index_count, model_cache_size); });
}

void modelCacheLRU(size_t model_cache_size)
{
	inspectCache([&](const unsigned int* indices, size_t index_count) { return estimateVSInvocationsLRU(indices, index_count, model_cache_size); });
}

void modelCacheReuse(size_t model_warp_size, size_t model_fifo_size, size_t model_triangle_limit)
{
	inspectCache([&](const unsigned int* indices, size_t index_count) { return estimateVSInvocationsReuse(indices, index_count, model_warp_size, model_fifo_size, model_triangle_limit); });
}

int main(int argc, char** argv)
{
	IDXGIFactory1* factory = 0;
	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);

	IDXGIAdapter* adapter = NULL;
	for (unsigned int index = 0; SUCCEEDED(factory->EnumAdapters(index, &adapter)); ++index)
	{
		DXGI_ADAPTER_DESC ad = {};
		adapter->GetDesc(&ad);

		if (ad.VendorId == 0x1414 && ad.DeviceId == 0x8c)
			continue; // Skip Microsoft Basic Render Driver

		printf("// GPU %d: %S (Vendor %04x Device %04x)\n", index, ad.Description, ad.VendorId, ad.DeviceId);

		if (argc == 1)
		{
			testCache(adapter);
		}
		else if (strcmp(argv[1], "--") == 0)
		{
			testCacheSequence(adapter, argc, argv);
		}
		else
		{
			testCacheMeshes(adapter, argc, argv);
		}
	}

	if (argc == 1)
	{
		printf("// Model FIFO 128\n");
		modelCacheFIFO(128);

		printf("// Model FIFO 16\n");
		modelCacheFIFO(16);

		printf("// Model LRU 16\n");
		modelCacheLRU(16);

		printf("// Model Reuse 32\n");
		modelCacheReuse(32, 32, 32);

		printf("// Model Reuse 32 w/16-entry FIFO\n");
		modelCacheReuse(32, 16, 32);
	}
}
#endif
