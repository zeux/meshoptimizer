# meshoptimizer.js

This folder contains JavaScript/WebAssembly modules that can be used to access parts of functionality of meshoptimizer library. While normally these would be used internally by glTF loaders, processors and other Web optimization tools, they can also be used directly if needed. The modules are available as an [NPM package](https://www.npmjs.com/package/meshoptimizer) but can also be redistributed individually on a file-by-file basis.

## Structure

Each component comes in two variants:

- `meshopt_component.js` uses a UMD-style module declaration and can be used by a wide variety of JavaScript module loaders, including node.js require(), AMD, Common.JS, and can also be loaded into the web page directly via a `<script>` tag which exposes the module as a global variable
- `meshopt_component.module.js` uses ES6 module exports and can be imported from another ES6 module

In either case the export name is MeshoptComponent and is an object that has two fields:

- `supported` is a boolean that can be checked to see if the component is supported by the current execution environment; it will generally be `false` when WebAssembly is not supported or enabled. To use these components on browsers without WebAssembly a polyfill library is recommended.
- `ready` is a Promise that is resolved when WebAssembly compilation and initialization finishes; any functions are unsafe to call before that happens.

In addition to that, each component exposes a set of specific functions documented below.

## Decoder

`MeshoptDecoder` (`meshopt_decoder.js`) implements high performance decompression of attribute and index buffers encoded using meshopt compression. This can be used to decompress glTF buffers encoded with `EXT_meshopt_compression` extension or for custom geometry compression pipelines. The module contains two implementations, scalar and SIMD, with the best performing implementation selected automatically. When SIMD is available, the decoders run at 1-3 GB/s on modern desktop computers.

To decode a buffer, one of the decoding functions should be called:

```ts
decodeVertexBuffer: (target: Uint8Array, count: number, size: number, source: Uint8Array, filter?: string) => void;
decodeIndexBuffer: (target: Uint8Array, count: number, size: number, source: Uint8Array) => void;
decodeIndexSequence: (target: Uint8Array, count: number, size: number, source: Uint8Array) => void;
```

The `source` should contain the data encoded using meshopt codecs; `count` represents the number of elements (attributes or indices); `size` represents the size of each element and should be divisible by 4 for `decodeVertexBuffer` and equal to 2 or 4 for the index decoders. `target` must be `count * size` bytes.

Given a valid encoded buffer and the correct input parameters, these functions always succeed; they fail if the input data is malformed.

When decoding attribute (vertex) data, additionally one of the decoding filters can be applied to further post-process the decoded data. `filter` must be equal to `"OCTAHEDRAL"`, `"QUATERNION"` or `"EXPONENTIAL"` to activate this extra step. The description of filters can be found in [the specification for EXT_meshopt_compression](https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Vendor/EXT_meshopt_compression/README.md).

To simplify the decoding further, a wrapper function is provided that automatically calls the correct version of the decoding based on `mode` - which should be `"ATTRIBUTES"`, `"TRIANGLES"` or `"INDICES"`. The difference in terminology is due to the fact that the JavaScript API uses the terms established in the glTF extension, whereas the function names match that of the meshoptimizer C++ API.

```ts
decodeGltfBuffer: (target: Uint8Array, count: number, size: number, source: Uint8Array, mode: string, filter?: string) => void;
```

## Encoder

`MeshoptEncoder` (`meshopt_encoder.js`) implements data preprocessing and compression of attribute and index buffers. It can be used to compress data that can be decompressed using the decoder module - note that the encoding process is more complicated and nuanced. It is typically split into three steps:

1. Pre-process the mesh to improve index and vertex locality which increases compression ratio
2. Quantize the data, either manually using integer or normalized integer format as a target, or using filter encoders
3. Encode the data

Step 1 is optional but highly recommended for triangle meshes; it can be omitted when compressing data with a predefined order such as animation keyframes.
Step 2 is the only lossy step in this process; without step 2, encoding will retain all semantics of the input exactly which can result in compressed data that is too large.

To reverse the process, decoder is used to reverse step 3 and (optionally) 2; the resulting data can typically be fed directly to the GPU. Note that the output of step 3 can also be further compressed in transport using a general-purpose compression algorithm such as Deflate.

To pre-process the mesh, the following function should be called with the input index buffer:

```ts
reorderMesh: (indices: Uint32Array, triangles: boolean, optsize: boolean) => [Uint32Array, number];
```

The function optimizes the input array for locality of reference (make sure to pass `triangles=true` for triangle lists, and `false` otherwise). `optsize` can choose whether the order should be optimal for transmission size (recommended for Web) or for GPU rendering performance. The function changes the `indices` array in place and returns an additional remap array and the total number of unique vertices.

After this function returns, to maintain correct rendering the application should reorder all vertex streams - including morph targets if applicable - according to the remap array. For each original index, remap array contains the new location for that index, so the remapping pseudocode looks like this:

```ts
let newvertices = new VertexArray(unique); // unique is returned by reorderMesh
for (let i = 0; i < oldvertices.length; ++i)
	newvertices[remap[i]] = oldvertices[i];
```

To quantize the attribute data (whether it represents a mesh component or something else like a rotation quaternion for a bone), typically some data-specific analysis should be performed to determine the optimal quantization strategy. For linear data such as positions or texture coordinates remapping the input range to 0..1 and quantizing the resulting integer using fixed-point encoding with a given number of bits stored in a 16-bit or 8-bit integer is recommended; however, this is not always best for compression ratio for data with complex cross-component dependencies.

To that end, three filter encoders are provided: octahedral (optimal for normal or tangent data), quaternion (optimal for unit-length quaternions) and exponential (optimal for compressing floating-point vectors). The last two are recommended for use for animation data, and exponential filter can additionally be used to quantize any floating-point vertex attribute for which integer quantization is not sufficiently precise.

```ts
encodeFilterOct: (source: Float32Array, count: number, stride: number, bits: number) => Uint8Array;
encodeFilterQuat: (source: Float32Array, count: number, stride: number, bits: number) => Uint8Array;
encodeFilterExp: (source: Float32Array, count: number, stride: number, bits: number) => Uint8Array;
```

All these functions take a source floating point buffer as an input, and perform a complex transformation that, when reversed by a decoder, results in an optimally quantized decompressed output. Because of this these functions assume specific configuration of input and output data:

- `encodeFilterOct` takes each 4 floats from the source array (for a total of `count` 4-vectors), treats them as a unit vector (XYZ) and fourth component from -1..1 (W), and encodes them into `stride` bytes in a way that, when decoded, the result is stored as a normalized signed 4-vector. `stride` must be 4 (in which case the round-trip result is 4 8-bit normalized values) or 8 (in which case the round-trip result is 4 16-bit normalized values). This encoding is recommended for normals (with stride=4 for medium quality and 8 for high quality output) and tangents (with stride=4 providing enough quality in all cases; note that 4-th component is preserved in case it stores coordinate spaced winding). `bits` represents the desired precision of each component and must be in `[1..8]` range if `stride=4` and `[1..16]` range if `stride=8`.

- `encodeFilterQuat` takes each 4 floats from the source array (for a total of `count` 4-vectrors), treats them as a unit quaternion, and encodes them into `stride` bytes in a way that, when decoded, the result is stored as a normalized signed 4-vector representing the same rotation as the source quaternion. `stride` must be 8 (the round-trip result is 4 16-bit normalized values). `bits` represents the desired precision of each component and must be in `[4..16]` range, although using less than 9-10 bits is likely going to lead to significant deviation in rotations.

- `encodeFilterExp` takes each K floats from the source array (where `K=stride/4`, for a total of `count` K-vectors), and encodes them into `stride` bytes in a way that, when decoded, the result is stored as K single-precision floating point values. This may seem redundant but it allows to trade some precision for a higher compression ratio due to reduced precision of stored components, controlled by `bits` which must be in `[1..24]` range, and a shared exponent encoding used by the function.

Note that in all cases using the highest `bits` value allowed by the output `stride` won't change the size of the output array (which is always going to be `count * stride` bytes), but it *will* reduce compression efficiency, as such the lowest acceptable `bits` value is recommended to use. When multiple parts of the data require different levels of precision, encode filters can be called multiple times and the output of the same filter called with the same `stride` can be concatenated even if `bits` are different.

After data is quantized using filter encoding or manual quantization, the result should be compressed using one of the following functions that mirror the interface of the decoding functions described above:

```ts
encodeVertexBuffer: (source: Uint8Array, count: number, size: number) => Uint8Array;
encodeIndexBuffer: (source: Uint8Array, count: number, size: number) => Uint8Array;
encodeIndexSequence: (source: Uint8Array, count: number, size: number) => Uint8Array;

encodeGltfBuffer: (source: Uint8Array, count: number, size: number, mode: string) => Uint8Array;
```

`size` is the size of each component in bytes; it must be divisible by 4 for attribute/vertex encoding and must be equal to 2 or 4 for index encoding; additionally, index buffer encoding assumes triangle lists as an input and as such count must be divisible by 3.

Note that the source is specified as byte arrays; for example, to quantize a position stream encoded using 16-bit integers with 5 vertices, `source` must have length of `5 * 8 = 40` bytes (8 bytes for each position - 3\*2 bytes of data and 2 bytes of padding to conform to alignment requirements), `count` must be 5 and `size` must be 8. When padding data to the alignment boundary make sure to use 0 as padding bytes for optimal compression.

When interleaved vertex data is compressed, `encodeVertexBuffer` can be called with the full size of a single interleaved vertex; however, when compressing deinterleaved data, note that `encodeVertexBuffer` should be called on each component individually if the strides of different streams are different.

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE.md).
