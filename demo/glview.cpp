#include "objparser.hpp"
#include "../src/meshoptimizer.hpp"

#include <algorithm>

#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>

#pragma comment(lib, "opengl32.lib")

struct Options
{
	bool wireframe;
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

	for (size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		const Vertex& v = mesh.vertices[i];

		centerx += v.px;
		centery += v.py;
		centerz += v.pz;
	}

	centerx /= float(mesh.vertices.size());
	centery /= float(mesh.vertices.size());
	centerz /= float(mesh.vertices.size());

	float extent = 0;

	for (size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		const Vertex& v = mesh.vertices[i];

		extent = std::max(extent, fabsf(v.px - centerx));
		extent = std::max(extent, fabsf(v.py - centery));
		extent = std::max(extent, fabsf(v.pz - centerz));
	}

	extent *= 1.1f;

	float scalex = width > height ? float(height) / float(width) : 1;
	float scaley = height > width ? float(width) / float(height) : 1;

	glBegin(GL_TRIANGLES);

	for (size_t i = 0; i < mesh.indices.size(); ++i)
	{
		const Vertex& v = mesh.vertices[mesh.indices[i]];

		float intensity = -(v.pz - centerz) / extent * 0.5f + 0.5f;

		glColor3f(intensity, intensity, intensity);
		glVertex3f((v.px - centerx) / extent * scalex, (v.py - centery) / extent * scaley, (v.pz - centerz) / extent);
	}

	glEnd();
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

	Mesh mesh = parseObj(argv[1]);
	Options options = {};

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
		}

		RECT rect;
		GetClientRect(hWnd, &rect);

		display(rect.right - rect.left, rect.bottom - rect.top, mesh, options);
		SwapBuffers(hDC);
	}
}