/* Babylon.js extension for EXT_meshopt_compression; requires Babylon.js 4.1 */
/* BABYLON.GLTF2.GLTFLoader.RegisterExtension("EXT_meshopt_compression", (loader) => new EXT_meshopt_compression(loader, MeshoptDecoder)); */
var EXT_meshopt_compression = /** @class */ (function () {
    var NAME = "EXT_meshopt_compression";
    /** @hidden */
    function EXT_meshopt_compression(loader, decoder) {
        /** The name of this extension. */
        this.name = NAME;
        /** Defines whether this extension is enabled. */
        this.enabled = true;
        this._loader = loader;
        this._decoder = decoder;
    }
    /** @hidden */
    EXT_meshopt_compression.prototype.dispose = function () {
        delete this._loader;
    };
    /** @hidden */
    EXT_meshopt_compression.prototype.loadBufferViewAsync = function (context, bufferView) {
        if (bufferView.extensions && bufferView.extensions[NAME]) {
            var extensionDef = bufferView.extensions[NAME];
            if (extensionDef._decoded) {
                return extensionDef._decoded;
            }
            var view = this._loader.loadBufferViewAsync(context, extensionDef);
            var decoder = this._decoder;
            extensionDef._decoded = Promise.all([view, decoder.ready]).then(function (res) {
                var source = res[0];
                var count = extensionDef.count;
                var stride = extensionDef.byteStride;
                var result = new Uint8Array(new ArrayBuffer(count * stride));
                decoder.decodeGltfBuffer(result, count, stride, source, extensionDef.mode, extensionDef.filter);
                return Promise.resolve(result);
            });
            return extensionDef._decoded;
        } else {
            return null;
        }
    };
    return EXT_meshopt_compression;
}());
