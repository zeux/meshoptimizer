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

var bakeMaterial = new THREE.ShaderMaterial({
	uniforms: {
		bvh: { value: null },
		attrNormal: { value: null },
		attrUv: { value: null },
		attrColor: { value: null },
		attrMaterial: { value: null },
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

${BVHShaderGLSL.common_functions}
${BVHShaderGLSL.bvh_struct_definitions}
${BVHShaderGLSL.bvh_ray_functions}
${BVHShaderGLSL.bvh_distance_functions}

uniform BVH bvh;
uniform sampler2D attrNormal;
uniform sampler2D attrUv;
uniform sampler2D attrColor;
uniform usampler2D attrMaterial;
uniform float thickness;

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

	gl_FragColor = vec4(uv, 0.0, 1.0);
}
`,
	depthTest: false,
	depthWrite: false,
	side: THREE.DoubleSide,
});

export function transferTexture(renderer, geometry, bvh, thickness, size, gutter) {
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

	bakeMaterial.uniforms.bvh.value = bvhgpu;
	bakeMaterial.uniforms.attrNormal.value = attrNormal;
	bakeMaterial.uniforms.attrUv.value = attrUv;
	bakeMaterial.uniforms.attrColor.value = attrColor;
	bakeMaterial.uniforms.attrMaterial.value = attrMaterial;
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

	var texture = new THREE.DataTexture(data, size, size, THREE.RGBAFormat);
	texture.colorSpace = THREE.SRGBColorSpace;
	texture.magFilter = THREE.LinearFilter;
	texture.minFilter = THREE.LinearFilter;
	texture.needsUpdate = true;
	return texture;
}
