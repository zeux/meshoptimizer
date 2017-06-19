#ifdef _WIN32
#include "objparser.hpp"
#include "../src/meshoptimizer.hpp"

#include <algorithm>
#include <ctime>

#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>

#pragma comment(lib, "opengl32.lib")

struct Options
{
	bool wireframe;
	enum { Mode_Default, Mode_Texture, Mode_Normals, Mode_UV } mode;
};

struct Vertex
{
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
};

static Mesh parseObj(const char* path)
{
	ObjFile file;

	if (!objParseFile(file, path) || !objValidate(file))
	{
		printf("Error loading %s\n", path);
		return Mesh();
	}

	objTriangulate(file);

	size_t total_indices = file.f.size() / 3;

	std::vector<Vertex> vertices;
	vertices.reserve(total_indices);

	for (size_t i = 0; i < file.f.size(); i += 3)
	{
		int vi = file.f[i + 0];
		int vti = file.f[i + 1];
		int vni = file.f[i + 2];

		// TODO
		vni = vti = -1;

		Vertex v =
		    {
		        file.v[vi * 3 + 0],
		        file.v[vi * 3 + 1],
		        file.v[vi * 3 + 2],

		        vni >= 0 ? file.vn[vni * 3 + 0] : 0,
		        vni >= 0 ? file.vn[vni * 3 + 1] : 0,
		        vni >= 0 ? file.vn[vni * 3 + 2] : 0,

		        vti >= 0 ? file.vt[vti * 3 + 0] : 0,
		        vti >= 0 ? file.vt[vti * 3 + 1] : 0,
		    };

		vertices.push_back(v);
	}

	Mesh result;

	result.indices.resize(total_indices);

	size_t total_vertices = generateIndexBuffer(&result.indices[0], &vertices[0], total_indices, sizeof(Vertex));

	result.vertices.resize(total_vertices);

	generateVertexBuffer(&result.vertices[0], &result.indices[0], &vertices[0], total_indices, sizeof(Vertex));

	return result;
}

Mesh optimize(const Mesh& mesh, int lod)
{
	float threshold = powf(0.75f, float(lod));
	size_t target_index_count = size_t(mesh.indices.size() * threshold);

	Mesh result = mesh;
	result.indices.resize(simplify(&result.indices[0], mesh.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex), target_index_count));

	return result;
}

void display(int width, int height, const Mesh& mesh, const Options& options)
{
	glViewport(0, 0, width, height);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glPolygonMode(GL_FRONT_AND_BACK, options.wireframe ? GL_LINE : GL_FILL);

	float centerx = 0;
	float centery = 0;
	float centerz = 0;
	float centeru = 0;
	float centerv = 0;

	for (size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		const Vertex& v = mesh.vertices[i];

		centerx += v.px;
		centery += v.py;
		centerz += v.pz;
		centeru += v.tx;
		centerv += v.ty;
	}

	centerx /= float(mesh.vertices.size());
	centery /= float(mesh.vertices.size());
	centerz /= float(mesh.vertices.size());
	centeru /= float(mesh.vertices.size());
	centerv /= float(mesh.vertices.size());

	float extent = 0;
	float extentuv = 0;

	for (size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		const Vertex& v = mesh.vertices[i];

		extent = std::max(extent, fabsf(v.px - centerx));
		extent = std::max(extent, fabsf(v.py - centery));
		extent = std::max(extent, fabsf(v.pz - centerz));
		extentuv = std::max(extentuv, fabsf(v.tx - centeru));
		extentuv = std::max(extentuv, fabsf(v.ty - centerv));
	}

	extent *= 1.1f;
	extentuv *= 1.1f;

	float scalex = width > height ? float(height) / float(width) : 1;
	float scaley = height > width ? float(width) / float(height) : 1;

	glBegin(GL_TRIANGLES);

	for (size_t i = 0; i < mesh.indices.size(); ++i)
	{
		const Vertex& v = mesh.vertices[mesh.indices[i]];

		switch (options.mode)
		{
		case Options::Mode_UV:
			glVertex3f((v.tx - centeru) / extentuv * scalex, (v.ty - centerv) / extentuv * scaley, 0);
			break;

		case Options::Mode_Texture:
			glColor3f(v.tx - floorf(v.tx), v.ty - floorf(v.ty), 0.5f);
			glVertex3f((v.px - centerx) / extent * scalex, (v.py - centery) / extent * scaley, (v.pz - centerz) / extent);
			break;

		case Options::Mode_Normals:
			glColor3f(v.nx * 0.5f + 0.5f, v.ny * 0.5f + 0.5f, v.nz * 0.5f + 0.5f);
			glVertex3f((v.px - centerx) / extent * scalex, (v.py - centery) / extent * scaley, (v.pz - centerz) / extent);
			break;

		default:
			float intensity = -(v.pz - centerz) / extent * 0.5f + 0.5f;
			glColor3f(intensity, intensity, intensity);
			glVertex3f((v.px - centerx) / extent * scalex, (v.py - centery) / extent * scaley, (v.pz - centerz) / extent);
		}
	}

	glEnd();
}

void stats(HWND hWnd, const char* path, const Mesh& mesh, int lod, double time)
{
	char title[256];
	snprintf(title, sizeof(title), "%s: LOD %d - %d triangles (%.1f msec)", path, lod, int(mesh.indices.size() / 3), time * 1000);

	SetWindowTextA(hWnd, title);
}

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		printf("Usage: %s [.obj file]\n", argv[0]);
		return 0;
	}

	HWND hWnd = CreateWindow(L"ListBox", L"Title", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	HDC hDC = GetDC(hWnd);

	PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, 0, 32 };
	pfd.cDepthBits = 24;

	SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd);
	wglMakeCurrent(hDC, wglCreateContext(hDC));

	ShowWindow(hWnd, SW_SHOWNORMAL);

	const char* path = argv[1];
	Mesh basemesh = parseObj(path);
	Options options = {};

	stats(hWnd, path, basemesh, 0, 0);

	Mesh mesh = basemesh;

	MSG msg;

	while (GetMessage(&msg, hWnd, 0, 0) && msg.message)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_KEYDOWN)
		{
			if (msg.wParam == 'W')
			{
				options.wireframe = !options.wireframe;
			}
			else if (msg.wParam == 'T')
			{
				options.mode = options.mode == Options::Mode_Texture ? Options::Mode_Default : Options::Mode_Texture;
			}
			else if (msg.wParam == 'N')
			{
				options.mode = options.mode == Options::Mode_Normals ? Options::Mode_Default : Options::Mode_Normals;
			}
			else if (msg.wParam == 'U')
			{
				options.mode = options.mode == Options::Mode_UV ? Options::Mode_Default : Options::Mode_UV;
			}
			else if (msg.wParam == '0')
			{
				mesh = basemesh;
				stats(hWnd, path, mesh, 0, 0);
			}
			else if (msg.wParam >= '1' && msg.wParam <= '9')
			{
				int lod = int(msg.wParam - '0');

				clock_t start = clock();
				mesh = optimize(basemesh, lod);
				clock_t end = clock();

				stats(hWnd, path, mesh, lod, double(end - start) / CLOCKS_PER_SEC);
			}
		}

		RECT rect;
		GetClientRect(hWnd, &rect);

		display(rect.right - rect.left, rect.bottom - rect.top, mesh, options);
		SwapBuffers(hDC);
	}
}
#endif
