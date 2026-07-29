// GPU color/texture transfer for remeshing demo
//
// Remeshed atlas is rasterized in UV space, the triangle edges are expanded to cover extra gutter space.
// The fragment shader traces rays against the original mesh (falling back to closest point on miss).
// The results are shaded using a simplified version of the original material, and written to the output texture.
// When painting vertex colors, we directly rasterize points for target vertex/triangles and trace rays/eval materials as above.
import * as THREE from 'three';
import { BVHShaderGLSL, MeshBVHUniformStruct, FloatVertexAttributeTexture, UIntVertexAttributeTexture } from 'three-mesh-bvh';

function expandGeometry(geometry, size, gutter) {
	var position = geometry.attributes.position;
	var normal = geometry.attributes.normal;
	var tangent = geometry.attributes.tangent;
	var uv = geometry.attributes.uv;
	var indices = geometry.index.array;

	var count = indices.length;
	var positions = new Float32Array(count * 3);
	var normals = new Float32Array(count * 3);
	var tangents = tangent ? new Float32Array(count * 4) : null;
	var uvs = new Float32Array(count * 2);

	for (var i = 0; i < count; i += 3) {
		var a = indices[i],
			b = indices[i + 1],
			c = indices[i + 2];
		var x0 = uv.getX(a) * size,
			y0 = uv.getY(a) * size;
		var x1 = uv.getX(b) * size,
			y1 = uv.getY(b) * size;
		var x2 = uv.getX(c) * size,
			y2 = uv.getY(c) * size;

		var denominator = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
		var scale = Math.abs(denominator) < 1e-7 ? 0 : gutter / Math.abs(denominator);

		var e0 = Math.hypot(x1 - x2, y1 - y2) * scale;
		var e1 = Math.hypot(x2 - x0, y2 - y0) * scale;
		var e2 = Math.hypot(x0 - x1, y0 - y1) * scale;

		for (var j = 0; j < 3; ++j) {
			// expand each triangle in barycentric space by e0/e1/e2 to accommodate extra gutter width
			var w0 = j == 0 ? 1 + e1 + e2 : -e0;
			var w1 = j == 1 ? 1 + e0 + e2 : -e1;
			var w2 = j == 2 ? 1 + e0 + e1 : -e2;

			// interpolate all attributes; we will use UVs to rasterize and positions/normals to trace against the original mesh
			var v = i + j;
			positions[v * 3 + 0] = position.getX(a) * w0 + position.getX(b) * w1 + position.getX(c) * w2;
			positions[v * 3 + 1] = position.getY(a) * w0 + position.getY(b) * w1 + position.getY(c) * w2;
			positions[v * 3 + 2] = position.getZ(a) * w0 + position.getZ(b) * w1 + position.getZ(c) * w2;

			normals[v * 3 + 0] = normal.getX(a) * w0 + normal.getX(b) * w1 + normal.getX(c) * w2;
			normals[v * 3 + 1] = normal.getY(a) * w0 + normal.getY(b) * w1 + normal.getY(c) * w2;
			normals[v * 3 + 2] = normal.getZ(a) * w0 + normal.getZ(b) * w1 + normal.getZ(c) * w2;

			if (tangent) {
				// tangents are only used when normal maps are baked
				tangents[v * 4 + 0] = tangent.getX(a) * w0 + tangent.getX(b) * w1 + tangent.getX(c) * w2;
				tangents[v * 4 + 1] = tangent.getY(a) * w0 + tangent.getY(b) * w1 + tangent.getY(c) * w2;
				tangents[v * 4 + 2] = tangent.getZ(a) * w0 + tangent.getZ(b) * w1 + tangent.getZ(c) * w2;
				tangents[v * 4 + 3] = tangent.getW(a) * w0 + tangent.getW(b) * w1 + tangent.getW(c) * w2;
			}

			uvs[v * 2 + 0] = (x0 * w0 + x1 * w1 + x2 * w2) / size;
			uvs[v * 2 + 1] = (y0 * w0 + y1 * w1 + y2 * w2) / size;
		}
	}

	var result = new THREE.BufferGeometry();
	result.setAttribute('position', new THREE.BufferAttribute(positions, 3));
	result.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
	result.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
	if (tangent) result.setAttribute('tangent', new THREE.BufferAttribute(tangents, 4));
	return result;
}

function sampleGeometry(geometry, size) {
	var position = geometry.attributes.position;
	var normal = geometry.attributes.normal;
	var indices = geometry.index.array;

	var vertices = position.count;
	var triangles = indices.length / 3;
	var samples = vertices + triangles; // one sample per vertex plus one sample per triangle centroid

	var positions = new Float32Array(samples * 3);
	var normals = new Float32Array(samples * 3);
	var uvs = new Float32Array(samples * 2);

	for (var i = 0; i < vertices; ++i) {
		for (var k = 0; k < 3; ++k) {
			positions[i * 3 + k] = position.getComponent(i, k);
			normals[i * 3 + k] = normal.getComponent(i, k);
		}
	}

	for (var i = 0; i < triangles; ++i) {
		var a = indices[i * 3 + 0],
			b = indices[i * 3 + 1],
			c = indices[i * 3 + 2];

		// normals will be normalized in the shader
		for (var k = 0; k < 3; ++k) {
			positions[(vertices + i) * 3 + k] = (position.getComponent(a, k) + position.getComponent(b, k) + position.getComponent(c, k)) / 3;
			normals[(vertices + i) * 3 + k] = normal.getComponent(a, k) + normal.getComponent(b, k) + normal.getComponent(c, k);
		}
	}

	for (var i = 0; i < samples; ++i) {
		uvs[i * 2 + 0] = ((i % size) + 0.5) / size;
		uvs[i * 2 + 1] = (Math.floor(i / size) + 0.5) / size;
	}

	var result = new THREE.BufferGeometry();
	result.setAttribute('position', new THREE.BufferAttribute(positions, 3));
	result.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
	result.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
	return result;
}

function extractColors(data, colors, indices) {
	var vertices = colors.length / 4;
	var triangles = indices.length / 3;

	for (var i = 0; i < vertices; ++i) {
		if (data[i * 4 + 3] == 0) continue;

		for (var k = 0; k < 3; ++k) colors[i * 4 + k] = Math.pow(data[i * 4 + k] / 255, 2.2);
		colors[i * 4 + 3] = 1;
	}

	for (var i = 0; i < triangles; ++i) {
		var offset = (vertices + i) * 4;
		if (data[offset + 3] == 0) continue;

		for (var j = 0; j < 3; ++j) {
			var v = indices[i * 3 + j];

			for (var k = 0; k < 3; ++k) colors[v * 4 + k] += Math.pow(data[offset + k] / 255, 2.2);
			colors[v * 4 + 3]++;
		}
	}

	for (var i = 0; i < colors.length; i += 4) {
		var weight = colors[i + 3];
		if (weight == 0) continue;

		for (var k = 0; k < 3; ++k) colors[i + k] /= weight;
		colors[i + 3] = 1;
	}
}

var MAP_SIZE = 1024;
var MAP_BUDGET = 1 << 30;
var MAP_SLOTS = ['map', 'aoMap', 'emissiveMap', 'roughnessMap', 'metalnessMap', 'normalMap'];

function createTextureArray(materials) {
	var textures = [];
	var layers = new Map();

	for (var i = 0; i < materials.length; ++i) {
		for (var j = 0; j < MAP_SLOTS.length; ++j) {
			var texture = materials[i][MAP_SLOTS[j]];

			// deduplicate images by source UUID to avoid wasting layers for reused images
			if (texture && texture.image && !layers.has(texture.source)) {
				layers.set(texture.source, textures.length);
				textures.push(texture);
			}
		}
	}

	var depth = Math.max(textures.length, 1);
	var size = MAP_SIZE;

	// prevent accidental catastrophic memory use by downscaling texture array until it fits
	while (size > 4 && depth * size * size * 4 > MAP_BUDGET) size /= 2;

	// all images need to be resized to the same size because we need all of them in one texture array
	var canvas = document.createElement('canvas');
	canvas.width = canvas.height = size;
	var context = canvas.getContext('2d', { willReadFrequently: true });

	var data = new Uint8Array(size * size * 4 * depth);

	for (var i = 0; i < textures.length; ++i) {
		context.clearRect(0, 0, size, size);
		context.drawImage(textures[i].image, 0, 0, size, size);
		data.set(context.getImageData(0, 0, size, size).data, i * size * size * 4);
	}

	var result = new THREE.DataArrayTexture(data, size, size, depth);
	result.wrapS = THREE.RepeatWrapping;
	result.wrapT = THREE.RepeatWrapping;
	result.minFilter = THREE.LinearFilter;
	result.magFilter = THREE.LinearFilter;
	result.needsUpdate = true;

	return { array: result, layers: layers };
}

function createMaterialTexture(materials, layers) {
	var stride = 24;
	var count = Math.max(materials.length, 1);
	var data = new Float32Array(count * stride);

	function getLayer(material, slot) {
		var texture = material[slot];
		return texture && layers.has(texture.source) ? layers.get(texture.source) : -1;
	}

	for (var i = 0; i < materials.length; ++i) {
		var material = materials[i];
		var map = material.map;

		// simplify UV transform by only taking repeat/offset from main texture
		data[i * stride + 0] = map ? map.repeat.x : 1;
		data[i * stride + 1] = map ? map.repeat.y : 1;
		data[i * stride + 2] = map ? map.offset.x : 0;
		data[i * stride + 3] = map ? map.offset.y : 0;

		// layer indices
		for (var j = 0; j < MAP_SLOTS.length; ++j) {
			data[i * stride + 4 + j] = getLayer(material, MAP_SLOTS[j]);
		}

		// material parameters
		data[i * stride + 12] = material.color ? material.color.r : 1;
		data[i * stride + 13] = material.color ? material.color.g : 1;
		data[i * stride + 14] = material.color ? material.color.b : 1;
		data[i * stride + 15] = material.aoMapIntensity !== undefined ? material.aoMapIntensity : 1;

		data[i * stride + 16] = material.emissive ? material.emissive.r * material.emissiveIntensity : 0;
		data[i * stride + 17] = material.emissive ? material.emissive.g * material.emissiveIntensity : 0;
		data[i * stride + 18] = material.emissive ? material.emissive.b * material.emissiveIntensity : 0;
		data[i * stride + 19] = material.roughness !== undefined ? material.roughness : 1;

		data[i * stride + 20] = material.metalness !== undefined ? material.metalness : 0;
	}

	var texture = new THREE.DataTexture(data, stride / 4, count, THREE.RGBAFormat, THREE.FloatType);
	texture.needsUpdate = true;
	return texture;
}

var bakeMaterial = new THREE.ShaderMaterial({
	glslVersion: THREE.GLSL3,
	uniforms: {
		bvh: { value: null },
		attrNormal: { value: null },
		attrUv: { value: null },
		attrColor: { value: null },
		attrTangent: { value: null },
		attrMaterial: { value: null },
		materials: { value: null },
		textures: { value: null },
		thickness: { value: 0 },
		bakeNormal: { value: 0 },
	},
	vertexShader: `
attribute vec4 tangent;

varying vec3 vPosition;
varying vec3 vNormal;
varying vec4 vTangent;

void main() {
	vPosition = position;
	vNormal = normal;
	vTangent = tangent;

	gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
	gl_PointSize = 1.0; // for vertex colors
}
`,
	fragmentShader: `
precision highp isampler2D;
precision highp usampler2D;
precision highp sampler2DArray;

${BVHShaderGLSL.common_functions}
${BVHShaderGLSL.bvh_struct_definitions}
${BVHShaderGLSL.bvh_ray_functions}
${BVHShaderGLSL.bvh_distance_functions}

uniform BVH bvh;
uniform sampler2D attrNormal;
uniform sampler2D attrUv;
uniform sampler2D attrColor;
uniform sampler2D attrTangent;
uniform usampler2D attrMaterial;
uniform sampler2D materials;
uniform sampler2DArray textures;
uniform float thickness;
uniform float bakeNormal;

vec3 fromsrgb(vec3 c) {
	return pow(c, vec3(2.2));
}

vec3 tosrgb(vec3 c) {
	return pow(c, vec3(1.0 / 2.2));
}

varying vec3 vPosition;
varying vec3 vNormal;
varying vec4 vTangent;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

void main() {
	vec3 normal = normalize(vNormal);

	uvec4 face = uvec4(0u);
	vec3 faceNormal = vec3(0.0), bary = vec3(0.0), closest = vec3(0.0);
	float side = 1.0, dist = 0.0;
	float maxClosest = thickness * 10.0;

	if (bvhIntersectFirstHit(bvh, vPosition + normal * thickness, -normal, face, faceNormal, bary, side, dist) && dist < thickness * 2.0) {
		// ray hit
	} else if (bvhClosestPointToPoint(bvh, vPosition, maxClosest, face, faceNormal, bary, side, closest) < maxClosest) {
		// closest point found; note, bary is ordered incorrectly in three-mesh-bvh for closest point so we need to swizzle it
		bary = bary.zxy;
	} else {
		discard;
	}

	vec2 uv = textureSampleBarycoord(attrUv, bary, face.xyz).xy;
	int mati = int(uTexelFetch1D(attrMaterial, face.x).r);

	vec4 md0 = texelFetch(materials, ivec2(0, mati), 0);
	vec4 md1 = texelFetch(materials, ivec2(1, mati), 0);
	vec4 md2 = texelFetch(materials, ivec2(2, mati), 0);
	vec4 md3 = texelFetch(materials, ivec2(3, mati), 0);
	vec4 md4 = texelFetch(materials, ivec2(4, mati), 0);
	vec4 md5 = texelFetch(materials, ivec2(5, mati), 0);

	// md0: uv transform, md1.xyzw + md2.xy: layer indices in MAP_SLOTS order,
	// md3: base color + ao intensity, md4: emissive color + roughness, md5.x: metalness
	vec2 point = uv * md0.xy + md0.zw;

	vec3 color = md3.rgb;
	if (md1.x >= 0.0) color *= fromsrgb(texture(textures, vec3(point, md1.x)).rgb);

	// vertex colors are linear, and are white when the source mesh or material doesn't use them
	color *= textureSampleBarycoord(attrColor, bary, face.xyz).rgb;

	if (md1.y >= 0.0) {
		float occlusion = texture(textures, vec3(point, md1.y)).r;
		color *= 1.0 + md3.w * (occlusion - 1.0);
	}

	float roughness = md4.w;
	float metalness = md5.x;
	if (md1.w >= 0.0) roughness *= texture(textures, vec3(point, md1.w)).g;
	if (md2.x >= 0.0) metalness *= texture(textures, vec3(point, md2.x)).b;

	// for now, bake metalness/roughness into color as an approximation
	color *= 1.0 - metalness * (1.0 - roughness * roughness * roughness);

	vec3 emissive = md4.rgb;
	if (md1.z >= 0.0) emissive *= fromsrgb(texture(textures, vec3(point, md1.z)).rgb);
	color += emissive;

	outColor = vec4(tosrgb(color), 1.0);
	outNormal = vec4(0.0);

	if (bakeNormal > 0.5) {
		vec3 snormal = textureSampleBarycoord(attrNormal, bary, face.xyz).xyz;

		if (md2.y >= 0.0) {
			vec4 stangent = textureSampleBarycoord(attrTangent, bary, face.xyz);
			vec3 bitangent = cross(snormal, stangent.xyz) * (stangent.w < 0.0 ? -1.0 : 1.0);
			vec3 nmap = texture(textures, vec3(point, md2.y)).xyz * 2.0 - 1.0;

			// replace source (smooth) normal with perturbed normal
			snormal = stangent.xyz * nmap.x + bitangent * nmap.y + snormal * nmap.z;
		}

		vec3 axisN = normal;
		vec3 axisT = vTangent.xyz - axisN * dot(axisN, vTangent.xyz);
		vec3 axisB = cross(axisN, axisT) * (vTangent.w < 0.0 ? -1.0 : 1.0);

		vec3 tbn = vec3(dot(snormal, axisT), dot(snormal, axisB), dot(snormal, axisN));
		outNormal = vec4(normalize(tbn) * 0.5 + 0.5, 1.0);
	}
}
`,
	depthTest: false,
	depthWrite: false,
	side: THREE.DoubleSide,
});

function bindCache(cache, thickness, normals) {
	bakeMaterial.uniforms.bvh.value = cache.bvh;
	bakeMaterial.uniforms.attrNormal.value = cache.attributes.normal;
	bakeMaterial.uniforms.attrUv.value = cache.attributes.uv;
	bakeMaterial.uniforms.attrColor.value = cache.attributes.color;
	bakeMaterial.uniforms.attrTangent.value = cache.attributes.tangent;
	bakeMaterial.uniforms.attrMaterial.value = cache.attributes.material;
	bakeMaterial.uniforms.materials.value = cache.materials;
	bakeMaterial.uniforms.textures.value = cache.textures;
	bakeMaterial.uniforms.thickness.value = thickness;
	bakeMaterial.uniforms.bakeNormal.value = normals ? 1 : 0;
}

function renderSamples(renderer, object, size, count) {
	var target = new THREE.WebGLRenderTarget(size, size, {
		count: count,
		minFilter: THREE.NearestFilter,
		magFilter: THREE.NearestFilter,
		depthBuffer: false,
		generateMipmaps: false,
	});

	renderer.setClearColor(0x000000, 0);
	renderer.setRenderTarget(target);
	renderer.render(object, new THREE.OrthographicCamera());

	renderer.setRenderTarget(null);

	var data = [];
	for (var i = 0; i < count; ++i) {
		data.push(new Uint8Array(size * size * 4));
		renderer.readRenderTargetPixels(target, 0, 0, size, size, data[i], undefined, i);
	}

	target.dispose();
	return data;
}

// data comes from render texture, but we need it as DataTexture to be compatible with canvas/GLB export
function createTexture(data, size, srgb) {
	var texture = new THREE.DataTexture(data, size, size, THREE.RGBAFormat);
	if (srgb) texture.colorSpace = THREE.SRGBColorSpace;
	texture.magFilter = THREE.LinearFilter;
	texture.minFilter = THREE.LinearFilter;
	texture.needsUpdate = true;
	return texture;
}

export function buildCache(bvh, materials) {
	var bvhgpu = new MeshBVHUniformStruct();
	var attrgpu = {
		normal: new FloatVertexAttributeTexture(),
		uv: new FloatVertexAttributeTexture(),
		color: new FloatVertexAttributeTexture(),
		tangent: new FloatVertexAttributeTexture(),
		material: new UIntVertexAttributeTexture(),
	};

	bvhgpu.updateFrom(bvh);
	attrgpu.normal.updateFrom(bvh.geometry.attributes.normal);
	attrgpu.uv.updateFrom(bvh.geometry.attributes.uv);
	attrgpu.color.updateFrom(bvh.geometry.attributes.color);
	attrgpu.tangent.updateFrom(bvh.geometry.attributes.tangent);
	attrgpu.material.updateFrom(bvh.geometry.attributes.material);

	var textures = createTextureArray(materials);
	var matgpu = createMaterialTexture(materials, textures.layers);

	return {
		bvh: bvhgpu,
		attributes: attrgpu,
		materials: matgpu,
		textures: textures.array,

		dispose: function () {
			this.bvh.dispose();
			this.materials.dispose();
			this.textures.dispose();

			for (var k in this.attributes) this.attributes[k].dispose();
		},
	};
}

export function transferTexture(renderer, geometry, cache, thickness, size, gutter, normals) {
	var expanded = expandGeometry(geometry, size, gutter);
	var mesh = new THREE.Mesh(expanded, bakeMaterial);
	mesh.frustumCulled = false;

	bindCache(cache, thickness, normals);

	var data = renderSamples(renderer, mesh, size, normals ? 2 : 1);

	expanded.dispose();

	return {
		map: createTexture(data[0], size, true),
		normalMap: normals ? createTexture(data[1], size, false) : null,
	};
}

export function transferColors(renderer, geometry, cache, thickness) {
	var vertices = geometry.attributes.position.count;
	var triangles = geometry.index.array.length / 3;
	var size = Math.ceil(Math.sqrt(vertices + triangles));

	var samples = sampleGeometry(geometry, size);
	var points = new THREE.Points(samples, bakeMaterial);
	points.frustumCulled = false;

	bindCache(cache, thickness, false);

	var data = renderSamples(renderer, points, size, 1)[0];

	samples.dispose();

	var colors = new Float32Array(vertices * 4);
	extractColors(data, colors, geometry.index.array);

	geometry.setAttribute('color', new THREE.BufferAttribute(colors, 4));
}
