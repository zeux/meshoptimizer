#ifdef _WIN32
#include "objparser.hpp"
#include "../src/meshoptimizer.hpp"

#include <algorithm>
#include <cassert>
#include <ctime>

#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>

#pragma comment(lib, "opengl32.lib")

struct Options
{
	bool wireframe;
	enum { Mode_Default, Mode_Texture, Mode_Normals } mode;
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

struct Vector3
{
	float x, y, z;
};

struct Quadric
{
	// TODO the matrix is symmetric, so we only really need 10 floats here
	float m[4][4];
};

void quadricFromPlane(Quadric& Q, float a, float b, float c, float d)
{
	Q.m[0][0] = a * a;
	Q.m[0][1] = a * b;
	Q.m[0][2] = a * c;
	Q.m[0][3] = a * d;
	Q.m[1][0] = b * a;
	Q.m[1][1] = b * b;
	Q.m[1][2] = b * c;
	Q.m[1][3] = b * d;
	Q.m[2][0] = c * a;
	Q.m[2][1] = c * b;
	Q.m[2][2] = c * c;
	Q.m[2][3] = c * d;
	Q.m[3][0] = d * a;
	Q.m[3][1] = d * b;
	Q.m[3][2] = d * c;
	Q.m[3][3] = d * d;
}

void quadricAdd(Quadric& Q, const Quadric& R)
{
	Q.m[0][0] += R.m[0][0];
	Q.m[0][1] += R.m[0][1];
	Q.m[0][2] += R.m[0][2];
	Q.m[0][3] += R.m[0][3];
	Q.m[1][0] += R.m[1][0];
	Q.m[1][1] += R.m[1][1];
	Q.m[1][2] += R.m[1][2];
	Q.m[1][3] += R.m[1][3];
	Q.m[2][0] += R.m[2][0];
	Q.m[2][1] += R.m[2][1];
	Q.m[2][2] += R.m[2][2];
	Q.m[2][3] += R.m[2][3];
	Q.m[3][0] += R.m[3][0];
	Q.m[3][1] += R.m[3][1];
	Q.m[3][2] += R.m[3][2];
	Q.m[3][3] += R.m[3][3];
}

void quadricMul(Quadric& Q, float s)
{
	Q.m[0][0] *= s;
	Q.m[0][1] *= s;
	Q.m[0][2] *= s;
	Q.m[0][3] *= s;
	Q.m[1][0] *= s;
	Q.m[1][1] *= s;
	Q.m[1][2] *= s;
	Q.m[1][3] *= s;
	Q.m[2][0] *= s;
	Q.m[2][1] *= s;
	Q.m[2][2] *= s;
	Q.m[2][3] *= s;
	Q.m[3][0] *= s;
	Q.m[3][1] *= s;
	Q.m[3][2] *= s;
	Q.m[3][3] *= s;
}

void quadricFromTriangle(Quadric& Q, const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	Vector3 p10 = { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
	Vector3 p20 = { p2.x - p0.x, p2.y - p0.y, p2.z - p0.z };
	Vector3 normal = { p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x };

	float area = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

	if (area > 0)
	{
		normal.x /= area;
		normal.y /= area;
		normal.z /= area;
	}

	float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

	quadricFromPlane(Q, normal.x, normal.y, normal.z, -distance);

	// Three classical weighting methods include weight=1, weight=area and weight=area^2
	// We use weight=area for now
	quadricMul(Q, area);
}

float quadricError(Quadric& Q, const Vector3& v)
{
	float vTQv =
		(Q.m[0][0] * v.x + Q.m[0][1] * v.y + Q.m[0][2] * v.z + Q.m[0][3]) * v.x +
		(Q.m[1][0] * v.x + Q.m[1][1] * v.y + Q.m[1][2] * v.z + Q.m[1][3]) * v.y +
		(Q.m[2][0] * v.x + Q.m[2][1] * v.y + Q.m[2][2] * v.z + Q.m[2][3]) * v.z +
		(Q.m[3][0] * v.x + Q.m[3][1] * v.y + Q.m[3][2] * v.z + Q.m[3][3]);

	return fabsf(vTQv);
}

Mesh optimize(const Mesh& mesh, int lod)
{
	float threshold = powf(0.75f, float(lod));

	std::vector<Vector3> vertex_positions(mesh.vertices.size());

	for (size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		const Vertex& v = mesh.vertices[i];

		vertex_positions[i].x = v.px;
		vertex_positions[i].y = v.py;
		vertex_positions[i].z = v.pz;
	}

	std::vector<Quadric> vertex_quadrics(mesh.vertices.size());

	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		Quadric Q;
		quadricFromTriangle(Q, vertex_positions[mesh.indices[i + 0]], vertex_positions[mesh.indices[i + 1]], vertex_positions[mesh.indices[i + 2]]);

		quadricAdd(vertex_quadrics[mesh.indices[i + 0]], Q);
		quadricAdd(vertex_quadrics[mesh.indices[i + 1]], Q);
		quadricAdd(vertex_quadrics[mesh.indices[i + 2]], Q);
	}

	std::vector<unsigned int> indices = mesh.indices;

	size_t target_index_count = size_t(mesh.indices.size() * threshold);

	while (indices.size() > target_index_count)
	{
		struct Collapse
		{
			size_t v0;
			size_t v1;
			float error;
		};

		std::vector<Collapse> edge_collapses;
		edge_collapses.reserve(indices.size());

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			static const int next[3] = { 1, 2, 0 };

			for (int e = 0; e < 3; ++e)
			{
				unsigned int i0 = indices[i + e];
				unsigned int i1 = indices[i + next[e]];

				float cost = quadricError(vertex_quadrics[i0], vertex_positions[i1]);

				// TODO: do we need uni-directional or bi-directional cost?
				cost += quadricError(vertex_quadrics[i1], vertex_positions[i0]);

				Collapse c = { i0, i1, cost };
				edge_collapses.push_back(c);
			}
		}

		std::sort(edge_collapses.begin(), edge_collapses.end(), [](const Collapse& l, const Collapse& r) { return l.error < r.error; });

		std::vector<unsigned int> vertex_remap(mesh.vertices.size());

		for (size_t i = 0; i < mesh.vertices.size(); ++i)
		{
			vertex_remap[i] = unsigned(i);
		}

		// each collapse removes 2 triangles
		size_t edge_collapse_goal = (indices.size() - target_index_count) / 6;

		// TODO: this is incorrect since edge_collapses have duplicate edges since cost is bidirectional
		float target_error = edge_collapses[edge_collapse_goal].error;

		size_t collapses = 0;
		float worst_error = 0;

		for (size_t i = 0; i < edge_collapses.size(); ++i)
		{
			const Collapse& c = edge_collapses[i];

			if (vertex_remap[c.v0] != c.v0)
				continue;

			if (vertex_remap[c.v1] != c.v1)
				continue;

			quadricAdd(vertex_quadrics[vertex_remap[c.v1]], vertex_quadrics[vertex_remap[c.v0]]);

			vertex_remap[c.v0] = vertex_remap[c.v1];

			collapses++;
			worst_error = c.error;

			if (collapses >= edge_collapse_goal)
				break;
		}

		printf("collapses: %d/%d, worst error: %e, target error: %e\n", int(collapses), int(edge_collapses.size()), worst_error, target_error);

		// no edges can be collapsed any more => bail out
		if (collapses == 0)
			break;

		size_t write = 0;

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			unsigned int v0 = vertex_remap[indices[i + 0]];
			unsigned int v1 = vertex_remap[indices[i + 1]];
			unsigned int v2 = vertex_remap[indices[i + 2]];

			if (v0 != v1 && v0 != v2 && v1 != v2)
			{
				indices[write + 0] = v0;
				indices[write + 1] = v1;
				indices[write + 2] = v2;
				write += 3;
			}
		}

		indices.resize(write);
	}

	Mesh result = mesh;

	for (size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		result.vertices[i].px = vertex_positions[i].x;
		result.vertices[i].py = vertex_positions[i].y;
		result.vertices[i].pz = vertex_positions[i].z;
	}

	result.indices = indices;

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

		switch (options.mode)
		{
		case Options::Mode_Texture:
			glColor3f(v.tx - floorf(v.tx), v.ty - floorf(v.ty), 0.5f);
			break;

		case Options::Mode_Normals:
			glColor3f(v.nx * 0.5f + 0.5f, v.ny * 0.5f + 0.5f, v.nz * 0.5f + 0.5f);
			break;

		default:
			float intensity = -(v.pz - centerz) / extent * 0.5f + 0.5f;
			glColor3f(intensity, intensity, intensity);
		}

		glVertex3f((v.px - centerx) / extent * scalex, (v.py - centery) / extent * scaley, (v.pz - centerz) / extent);
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
