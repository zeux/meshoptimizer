#include "../demo/objparser.h"
#include "../src/meshoptimizer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#ifdef _WIN32
#pragma comment(lib, "opengl32.lib")
#endif

extern unsigned char* meshopt_simplifyDebugKind;

struct Options
{
	bool wireframe;
	enum
	{
		Mode_Default,
		Mode_Texture,
		Mode_Normals,
		Mode_UV,
		Mode_Kind,
	} mode;
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
	std::vector<unsigned char> kinds;
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

	std::vector<unsigned int> remap(total_indices);
	size_t total_vertices = meshopt_generateVertexRemap(&remap[0], NULL, total_indices, &vertices[0], total_indices, sizeof(Vertex));

	result.indices.resize(total_indices);
	meshopt_remapIndexBuffer(&result.indices[0], NULL, total_indices, &remap[0]);

	result.vertices.resize(total_vertices);
	meshopt_remapVertexBuffer(&result.vertices[0], &vertices[0], total_indices, sizeof(Vertex), &remap[0]);

	return result;
}

Mesh optimize(const Mesh& mesh, int lod)
{
	float threshold = powf(0.7f, float(lod));
	size_t target_index_count = size_t(mesh.indices.size() * threshold);

	Mesh result = mesh;
	result.kinds.resize(result.vertices.size());
	meshopt_simplifyDebugKind = &result.kinds[0];
	result.indices.resize(meshopt_simplify(&result.indices[0], &result.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), target_index_count));

	return result;
}

void display(int x, int y, int width, int height, const Mesh& mesh, const Options& options)
{
	glViewport(x, y, width, height);
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

		float intensity = -(v.pz - centerz) / extent * 0.5f + 0.5f;

		switch (options.mode)
		{
		case Options::Mode_UV:
			glColor3f(intensity, intensity, intensity);
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
			glColor3f(intensity, intensity, intensity);
			glVertex3f((v.px - centerx) / extent * scalex, (v.py - centery) / extent * scaley, (v.pz - centerz) / extent);
		}
	}

	glEnd();

	if (options.mode == Options::Mode_Kind && !mesh.kinds.empty())
	{
		glPointSize(2);

		glBegin(GL_POINTS);

		for (size_t i = 0; i < mesh.indices.size(); ++i)
		{
			const Vertex& v = mesh.vertices[mesh.indices[i]];
			unsigned char kind = mesh.kinds[mesh.indices[i]];

			if (kind != 0)
			{
				glColor3f(kind == 0 || kind == 3, kind == 0 || kind == 2, kind == 0 || kind == 1);
				glVertex3f((v.px - centerx) / extent * scalex, (v.py - centery) / extent * scaley, (v.pz - centerz) / extent);
			}
		}

		glEnd();
	}
}

void stats(GLFWwindow* window, const char* path, unsigned int triangles, int lod, double time)
{
	char title[256];
	snprintf(title, sizeof(title), "%s: LOD %d - %d triangles (%.1f msec)", path, lod, triangles, time * 1000);

	glfwSetWindowTitle(window, title);
}

struct File
{
	Mesh basemesh;
	Mesh lodmesh;
	const char* path;
};

std::vector<File> files;
Options options;

void keyhandler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_W)
		{
			options.wireframe = !options.wireframe;
		}
		else if (key == GLFW_KEY_T)
		{
			options.mode = options.mode == Options::Mode_Texture ? Options::Mode_Default : Options::Mode_Texture;
		}
		else if (key == GLFW_KEY_N)
		{
			options.mode = options.mode == Options::Mode_Normals ? Options::Mode_Default : Options::Mode_Normals;
		}
		else if (key == GLFW_KEY_U)
		{
			options.mode = options.mode == Options::Mode_UV ? Options::Mode_Default : Options::Mode_UV;
		}
		else if (key == GLFW_KEY_K)
		{
			options.mode = options.mode == Options::Mode_Kind ? Options::Mode_Default : Options::Mode_Kind;
		}
		else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
		{
			int lod = int(key - GLFW_KEY_0);

			unsigned int triangles = 0;

			clock_t start = clock();
			for (auto& f : files)
			{
				f.lodmesh = optimize(f.basemesh, lod);
				triangles += unsigned(f.lodmesh.indices.size() / 3);
			}
			clock_t end = clock();

			stats(window, files[0].path, triangles, lod, double(end - start) / CLOCKS_PER_SEC);
		}
	}
}

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		printf("Usage: %s [.obj files]\n", argv[0]);
		return 0;
	}

	unsigned int basetriangles = 0;

	for (int i = 1; i < argc; ++i)
	{
		files.emplace_back();
		File& f = files.back();

		f.path = argv[i];
		f.basemesh = parseObj(f.path);
		f.lodmesh = optimize(f.basemesh, 0);

		basetriangles += unsigned(f.basemesh.indices.size() / 3);
	}

	glfwInit();

	GLFWwindow* window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
	glfwMakeContextCurrent(window);

	stats(window, files[0].path, basetriangles, 0, 0);

	glfwSetKeyCallback(window, keyhandler);

	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		glClearDepth(1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		int cols = int(ceil(sqrt(double(files.size()))));
		int rows = int(ceil(double(files.size()) / cols));

		int tilew = width / cols;
		int tileh = height / rows;

		for (size_t i = 0; i < files.size(); ++i)
		{
			File& f = files[i];
			int x = int(i) % cols;
			int y = int(i) / cols;

			display(x * tilew, y * tileh, tilew, tileh, f.lodmesh, options);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}