# gltfpack

This is an npm package for [gltfpack](https://github.com/zeux/meshoptimizer/#gltfpack) compiled from C++ to JS/WebAssembly.

## Installation

```
npm install -g gltfpack
```

## Usage

To convert a glTF file using gltfpack, run the command-line binary like this (run it without arguments for a list of options):

```
gltfpack -i scene.gltf -o scene.glb
```

gltfpack substantially changes the glTF data by optimizing the meshes for vertex fetch and transform cache, quantizing the geometry to reduce the memory consumption and size, merging meshes to reduce the draw call count, quantizing and resampling animations to reduce animation size and simplify playback, and pruning the node tree by removing or collapsing redundant nodes. It will also simplify the meshes when requested to do so.

By default gltfpack outputs regular `.glb`/`.gltf` files that have been optimized for GPU consumption using various cache optimizers and quantization. These files can be loaded by GLTF loaders that support `KHR_mesh_quantization` extension such as [three.js](https://threejs.org/) (r111+) and [Babylon.js](https://www.babylonjs.com/) (4.1+).

When using `-c` option, gltfpack outputs compressed `.glb`/`.gltf` files that use meshoptimizer codecs to reduce the download size further. Loading these files requires extending GLTF loaders with support for `MESHOPT_compression` extension; [demo/GLTFLoader.js](https://github.com/zeux/meshoptimizer/blob/master/demo/GLTFLoader.js) contains a custom version of three.js loader that can be used to load them.

When using compressed files, [js/meshopt_decoder.js](https://github.com/zeux/meshoptimizer/blob/master/js/meshopt_decoder.js) needs to be loaded to provide the WebAssembly decoder module like this:

```js
<script src="js/meshopt_decoder.js"></script>

...

var loader = new THREE.GLTFLoader();
loader.setMeshoptDecoder(MeshoptDecoder);
loader.load('pirate.glb', function (gltf) { scene.add(gltf.scene); });
```
