# 📦 gltfpack

gltfpack is a tool that can automatically optimize glTF files to reduce the download size and improve loading and rendering speed.

## Installation

You can download a pre-built binary for gltfpack on [Releases page](https://github.com/zeux/meshoptimizer/releases), or install [npm package](https://www.npmjs.com/package/gltfpack) as follows:

```
npm install -g gltfpack
```

## Usage

To convert a glTF file using gltfpack, run the command-line binary like this on an input `.gltf`/`.glb`/`.obj` file (run it without arguments for a list of options):

```
gltfpack -i scene.gltf -o scene.glb
```

gltfpack substantially changes the glTF data by optimizing the meshes for vertex fetch and transform cache, quantizing the geometry to reduce the memory consumption and size, merging meshes to reduce the draw call count, quantizing and resampling animations to reduce animation size and simplify playback, and pruning the node tree by removing or collapsing redundant nodes. It will also simplify the meshes when requested to do so.

By default gltfpack outputs regular `.glb`/`.gltf` files that have been optimized for GPU consumption using various cache optimizers and quantization. These files can be loaded by GLTF loaders that support `KHR_mesh_quantization` extension such as [three.js](https://threejs.org/) (r111+) and [Babylon.js](https://www.babylonjs.com/) (4.1+).

When using `-c` option, gltfpack outputs compressed `.glb`/`.gltf` files that use meshoptimizer codecs to reduce the download size further. Loading these files requires extending GLTF loaders with support for `EXT_meshopt_compression` extension; [demo/GLTFLoader.js](https://github.com/zeux/meshoptimizer/blob/master/demo/GLTFLoader.js) contains a custom version of three.js loader that can be used to load them.

For better compression, you can use `-cc` option which applies additional compression; additionally make sure that your content delivery method is configured to use deflate (gzip) - meshoptimizer codecs are designed to produce output that can be compressed further with general purpose compressors.

gltfpack can also compress textures using Basis Universal format, either storing .basis images directly (`-tb` flag, supported by three.js) or using KTX2 container (`-tc` flag, requires support for `KHR_image_ktx2`). Compression is performed using `basisu` executable and is thus only available in native builds.

When using compressed files, [js/meshopt_decoder.js](https://github.com/zeux/meshoptimizer/blob/master/js/meshopt_decoder.js) needs to be loaded to provide the WebAssembly decoder module like this:

```js
import './meshopt_decoder.js'; // imports MeshoptDecoder using ES6

...

var loader = new THREE.GLTFLoader();
loader.setMeshoptDecoder(MeshoptDecoder);
loader.load('pirate.glb', function (gltf) { scene.add(gltf.scene); });
```
