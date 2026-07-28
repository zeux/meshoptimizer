// GPU texture transfer for remeshing demo
//
// Remeshed atlas is rasterized in UV space, the triangle edges are expanded to cover extra gutter space.
import * as THREE from 'three';
import { BVHShaderGLSL, MeshBVHUniformStruct, FloatVertexAttributeTexture, UIntVertexAttributeTexture } from 'three-mesh-bvh';

function expandGeometry(geometry, size, gutter) {
	var position = geometry.attributes.position;
	var normal = geometry.attributes.normal;
	var uv = geometry.attributes.uv;
	var indices = geometry.index.array;

	var count = indices.length;
	var positions = new Float32Array(count * 3);
	var normals = new Float32Array(count * 3);
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

			uvs[v * 2 + 0] = (x0 * w0 + x1 * w1 + x2 * w2) / size;
			uvs[v * 2 + 1] = (y0 * w0 + y1 * w1 + y2 * w2) / size;
		}
	}

	var result = new THREE.BufferGeometry();
	result.setAttribute('position', new THREE.BufferAttribute(positions, 3));
	result.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
	result.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
	return result;
}

var MAP_SIZE = 1024;
var MAP_BUDGET = 1 << 30;
var MAP_SLOTS = ['map', 'metalnessMap', 'roughnessMap'];

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
	var stride = 12;
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

		// layer indices for all maps
		data[i * stride + 4] = getLayer(material, 'map');
		data[i * stride + 5] = getLayer(material, 'metalnessMap');
		data[i * stride + 6] = getLayer(material, 'roughnessMap');

		// material parameters
		data[i * stride + 7] = material.metalness !== undefined ? material.metalness : 0;
		data[i * stride + 8] = material.color ? material.color.r : 1;
		data[i * stride + 9] = material.color ? material.color.g : 1;
		data[i * stride + 10] = material.color ? material.color.b : 1;
		data[i * stride + 11] = material.roughness !== undefined ? material.roughness : 1;
	}

	var texture = new THREE.DataTexture(data, stride / 4, count, THREE.RGBAFormat, THREE.FloatType);
	texture.needsUpdate = true;
	return texture;
}

var bakeMaterial = new THREE.ShaderMaterial({
	uniforms: {
		bvh: { value: null },
		attrNormal: { value: null },
		attrUv: { value: null },
		attrColor: { value: null },
		attrMaterial: { value: null },
		materials: { value: null },
		textures: { value: null },
		thickness: { value: 0 },
	},
	vertexShader: `
varying vec3 vPosition;
varying vec3 vNormal;

void main() {
	vPosition = position;
	vNormal = normal;

	gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
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
uniform usampler2D attrMaterial;
uniform sampler2D materials;
uniform sampler2DArray textures;
uniform float thickness;

vec3 fromsrgb(vec3 c) {
	return pow(c, vec3(2.2));
}

vec3 tosrgb(vec3 c) {
	return pow(c, vec3(1.0 / 2.2));
}

varying vec3 vPosition;
varying vec3 vNormal;

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

	vec2 point = uv * md0.xy + md0.zw;

	vec3 color = md2.rgb;
	if (md1.x >= 0.0) color *= fromsrgb(texture(textures, vec3(point, md1.x)).rgb);

	// vertex colors are linear, and are white when the source mesh or material doesn't use them
	color *= textureSampleBarycoord(attrColor, bary, face.xyz).rgb;

	float metalness = md1.w;
	float roughness = md2.w;
	if (md1.y >= 0.0) metalness *= texture(textures, vec3(point, md1.y)).b;
	if (md1.z >= 0.0) roughness *= texture(textures, vec3(point, md1.z)).g;

	// for now, bake metalness/roughness into color as an approximation
	color *= 1.0 - metalness * (1.0 - roughness * roughness * roughness);

	gl_FragColor = vec4(tosrgb(color), 1.0);
}
`,
	depthTest: false,
	depthWrite: false,
	side: THREE.DoubleSide,
});

export function transferTexture(renderer, geometry, bvh, thickness, materials, size, gutter) {
	var expanded = expandGeometry(geometry, size, gutter);

	var bvhgpu = new MeshBVHUniformStruct();
	bvhgpu.updateFrom(bvh);

	var attributes = bvh.geometry.attributes;
	var attrNormal = new FloatVertexAttributeTexture();
	var attrUv = new FloatVertexAttributeTexture();
	var attrColor = new FloatVertexAttributeTexture();
	var attrMaterial = new UIntVertexAttributeTexture();

	attrNormal.updateFrom(attributes.normal);
	attrUv.updateFrom(attributes.uv);
	attrColor.updateFrom(attributes.color);
	attrMaterial.updateFrom(attributes.material);

	var textures = createTextureArray(materials);
	var materialsgpu = createMaterialTexture(materials, textures.layers);

	bakeMaterial.uniforms.bvh.value = bvhgpu;
	bakeMaterial.uniforms.attrNormal.value = attrNormal;
	bakeMaterial.uniforms.attrUv.value = attrUv;
	bakeMaterial.uniforms.attrColor.value = attrColor;
	bakeMaterial.uniforms.attrMaterial.value = attrMaterial;
	bakeMaterial.uniforms.materials.value = materialsgpu;
	bakeMaterial.uniforms.textures.value = textures.array;
	bakeMaterial.uniforms.thickness.value = thickness;

	var mesh = new THREE.Mesh(expanded, bakeMaterial);
	mesh.frustumCulled = false;

	var target = new THREE.WebGLRenderTarget(size, size, {
		minFilter: THREE.NearestFilter,
		magFilter: THREE.NearestFilter,
		depthBuffer: false,
		generateMipmaps: false,
	});

	renderer.setClearColor(0x000000, 0);
	renderer.setRenderTarget(target);
	renderer.render(mesh, new THREE.OrthographicCamera());

	renderer.setRenderTarget(null);

	// copy render target to DataTexture; this is redundant for display but we need this for GLB export to function
	var data = new Uint8Array(size * size * 4);
	renderer.readRenderTargetPixels(target, 0, 0, size, size, data);

	expanded.dispose();
	target.dispose();
	bvhgpu.dispose();
	attrNormal.dispose();
	attrUv.dispose();
	attrColor.dispose();
	attrMaterial.dispose();
	materialsgpu.dispose();
	textures.array.dispose();

	var texture = new THREE.DataTexture(data, size, size, THREE.RGBAFormat);
	texture.colorSpace = THREE.SRGBColorSpace;
	texture.magFilter = THREE.LinearFilter;
	texture.minFilter = THREE.LinearFilter;
	texture.needsUpdate = true;
	return texture;
}
