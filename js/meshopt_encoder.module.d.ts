// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
export type ExpMode = 'Separate' | 'SharedVector' | 'SharedComponent' | 'Clamped';

export const MeshoptEncoder: {
	supported: boolean;
	ready: Promise<void>;

	reorderMesh: (indices: Uint32Array, triangles: boolean, optsize: boolean) => [Uint32Array, number];
	reorderPoints: (positions: Float32Array, positions_stride: number) => Uint32Array;

	encodeVertexBuffer: (source: Uint8Array, count: number, size: number) => Uint8Array;
	encodeVertexBufferLevel: (source: Uint8Array, count: number, size: number, level: number, version?: number) => Uint8Array;
	encodeIndexBuffer: (source: Uint8Array, count: number, size: number) => Uint8Array;
	encodeIndexSequence: (source: Uint8Array, count: number, size: number) => Uint8Array;

	encodeGltfBuffer: (source: Uint8Array, count: number, size: number, mode: string) => Uint8Array;

	encodeFilterOct: (source: Float32Array, count: number, stride: number, bits: number) => Uint8Array;
	encodeFilterQuat: (source: Float32Array, count: number, stride: number, bits: number) => Uint8Array;
	encodeFilterExp: (source: Float32Array, count: number, stride: number, bits: number, mode?: ExpMode) => Uint8Array;
	encodeFilterColor: (source: Float32Array, count: number, stride: number, bits: number) => Uint8Array;
};
