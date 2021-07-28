// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2021, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
export const MeshoptEncoder: {
    supported: boolean;
    ready: Promise<void>;
    
    encodeVertexBuffer: (source: Uint8Array, count: number, size: number) => Uint8Array;
    encodeIndexBuffer: (source: Uint32Array, count: number) => Uint8Array;
    encodeIndexSequence: (source: Uint32Array, count: number) => Uint8Array;
};
