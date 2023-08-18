# 📦 gltfpack

gltfpack is a tool that can automatically optimize glTF files to reduce the download size and improve loading and rendering speed.

## Installation

You can download a pre-built binary for gltfpack on [Releases page](https://github.com/zeux/meshoptimizer/releases), or install [npm package](https://www.npmjs.com/package/gltfpack). Native binaries are recommended over npm since they can work with larger files, run faster, and support texture compression.

## Usage

To convert a glTF file using gltfpack, run the command-line binary like this on an input `.gltf`/`.glb`/`.obj` file (run it without arguments for a list of options):

```
gltfpack -i scene.gltf -o scene.glb
```

gltfpack substantially changes the glTF data by optimizing the meshes for vertex fetch and transform cache, quantizing the geometry to reduce the memory consumption and size, merging meshes to reduce the draw call count, quantizing and resampling animations to reduce animation size and simplify playback, and pruning the node tree by removing or collapsing redundant nodes. It will also simplify the meshes when requested to do so.

By default gltfpack outputs regular `.glb`/`.gltf` files that have been optimized for GPU consumption using various cache optimizers and quantization. These files can be loaded by GLTF loaders that support `KHR_mesh_quantization` extension such as [three.js](https://threejs.org/) (r111+) and [Babylon.js](https://www.babylonjs.com/) (4.1+).

When using `-c` option, gltfpack outputs compressed `.glb`/`.gltf` files that use meshoptimizer codecs to reduce the download size further. Loading these files requires extending GLTF loaders with support for [EXT_meshopt_compression](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Vendor/EXT_meshopt_compression/README.md) extension; three.js supports it in r122+ (requires calling `GLTFLoader.setMeshoptDecoder`), Babylon.js supports it in 5.0+ without further setup.

For better compression, you can use `-cc` option which applies additional compression; additionally make sure that your content delivery method is configured to use deflate (gzip) - meshoptimizer codecs are designed to produce output that can be compressed further with general purpose compressors.

gltfpack can also compress textures using Basis Universal format stored in a KTX2 container (`-tc` flag, requires support for `KHR_texture_basisu`). Textures can also be embedded into `.bin`/`.glb` output using `-te` flag.

When working with glTF files that contain point clouds, gltfpack automatically processes the point cloud data to reduce the download size to the extent possible. In addition to aforementioned compression options (either `-c` or `-cc` are recommended), gltfpack can also prune point clouds to provide a more uniform density when `-si` option is used.

## Decompression

When using compressed files, [js/meshopt_decoder.js](https://github.com/zeux/meshoptimizer/blob/master/js/meshopt_decoder.js) or `js/meshopt_decoder.module.js` needs to be loaded to provide the WebAssembly decoder module like this:

```js
import { MeshoptDecoder } from './meshopt_decoder.module.js';

...

var loader = new GLTFLoader();
loader.setMeshoptDecoder(MeshoptDecoder);
loader.load('pirate.glb', function (gltf) { scene.add(gltf.scene); });
```

When using Three.js, this module can be imported from three.js repository from `examples/jsm/libs/meshopt_decoder.module.js`.

Note that `meshopt_decoder` assumes that WebAssembly is supported. This is the case for all modern browsers; if support for legacy browsers such as Internet Explorer 11 is desired, it's recommended to use `-cf` flag when creating the glTF content. This will create and load fallback uncompressed buffers, but only on browsers that don't support WebAssembly.

## Options

By default gltfpack makes certain assumptions when optimizing the scenes, for example meshes that belong to nodes that aren't animated can be merged together, and has some defaults that represent a tradeoff between precision and size that are picked to fit most use cases. However, in some cases the resulting `.gltf` file needs to retain some way for the application to manipulate individual scene elements, and in other cases precision or size are more important to optimize for. gltfpack has a rich set of command line options to control various aspects of its behavior, with the full list available via `gltfpack -h`.

The following settings are frequently used to reduce the resulting data size:

* `-cc`: produce compressed gltf/glb files (requires `EXT_meshopt_compression`)
* `-tc`: convert all textures to KTX2 with BasisU supercompression (requires `KHR_texture_basisu` and may require `-tp` flag for compatibility with WebGL 1)
* `-mi`: use mesh instancing when serializing references to the same meshes (requires `EXT_mesh_gpu_instancing`)
* `-si R`: simplify meshes targeting triangle/point count ratio R (default: 1; R should be between 0 and 1)

The following settings are frequently used to restrict some optimizations:

* `-kn`: keep named nodes and meshes attached to named nodes so that named nodes can be transformed externally
* `-km`: keep named materials and disable named material merging
* `-ke`: keep extras data
* `-vpf`: use floating-point position quantization instead of the default fixed-point (this results in larger position data, but does not insert new nodes with dequantization transforms; when using this option, `-cc` is recommended as well)

## Extensions

gltfpack supports most Khronos extensions and some multi-vendor extensions in the input scenes, with newer extensions added regularly. The following extensions are fully supported:

- KHR_lights_punctual
- KHR_materials_anisotropy
- KHR_materials_clearcoat
- KHR_materials_emissive_strength
- KHR_materials_ior
- KHR_materials_iridescence
- KHR_materials_pbrSpecularGlossiness
- KHR_materials_sheen
- KHR_materials_specular
- KHR_materials_transmission
- KHR_materials_unlit
- KHR_materials_variants
- KHR_materials_volume
- KHR_mesh_quantization
- KHR_texture_transform

Even if the source file does not use extensions, gltfpack may use some extensions in the output file either by default or when certain options are used:

- KHR_mesh_quantization (used by default unless disabled via `-noq`)
- KHR_texture_transform (used by default when textures are present, unless disabled via `-noq` or `-vtf`)
- KHR_texture_basisu (used when requested via `-tc`)
- EXT_meshopt_compression (used when requested via `-c` or `-cc`)
- EXT_mesh_gpu_instancing (used when requested via `-mi`)

gltfpack does not support vendor-specific extensions or custom extensions, including ones defined in [Khronos glTF repository](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Vendor). Unknown extension nodes are discarded from the output.

## Building

gltfpack can be built from source using CMake or Make. To build a full version of gltfpack that supports texture compression, CMake configuration needs to specify the path to https://github.com/zeux/basis_universal fork (branch gltfpack) via `MESHOPT_BASISU_PATH` variable:

```
git clone -b gltfpack https://github.com/zeux/basis_universal
cmake . -DMESHOPT_BUILD_GLTFPACK=ON -DMESHOPT_BASISU_PATH=basis_universal -DCMAKE_BUILD_TYPE=Release
cmake --build . --target gltfpack --config Release
```

## License

gltfpack is available to anybody free of charge, under the terms of MIT License (see LICENSE.md).
