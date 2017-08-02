#ifdef _WIN32
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <assert.h>

#include <cassert>
#include <cmath>
#include <vector>

#include "../src/meshoptimizer.hpp"
#include "objparser.hpp"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "dxgi.lib")

void gridGen(std::vector<unsigned int>& indices, int x0, int x1, int y0, int y1, int width, int cacheSize, bool prefetch)
{
    if (x1 - x0 + 1 < cacheSize)
    {
        if (2 * (x1 - x0) + 1 > cacheSize && prefetch)
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
    else
    {
        int xm = x0 + cacheSize - 2;
        gridGen(indices, x0, xm, y0, y1, width, cacheSize, prefetch);
        gridGen(indices, xm, x1, y0, y1, width, cacheSize, prefetch);
    }
}

unsigned int queryVSInvocations(ID3D11Device* device, ID3D11DeviceContext* context, const unsigned int* indices, size_t index_count)
{
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

	D3D11_QUERY_DESC qdesc = { D3D11_QUERY_PIPELINE_STATISTICS };
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

	unsigned int dib[] = { 0, 1, 1, 2, 3, 4, 5, 5, 5 };

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

void testCacheMeshes(IDXGIAdapter* adapter, int argc, char** argv)
{
	ID3D11Device* device = 0;
	ID3D11DeviceContext* context = 0;
    D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, 0, 0, D3D11_SDK_VERSION, &device, 0, &context);

	setupShaders(device, context);

	for (int i = 1; i < argc; ++i)
	{
		const char* path = argv[i];

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

		printf("// GPU %d: %S\n", index, ad.Description);

		if (argc == 1)
		{
			testCache(adapter);
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

		printf("// Model LRU 16\n");
		modelCacheLRU(16);

		printf("// Model Reuse 32\n");
		modelCacheReuse(32, 32, 32);

		printf("// Model Reuse 32 w/16-entry FIFO\n");
		modelCacheReuse(32, 16, 32);
	}
}
#endif
