/* three.js extension for EXT_meshopt_compression; requires three.js r118 */
/* Note: this is obsolete since three.js r122 which provides extension support by default when setMeshoptDecoder is called */
/* loader.register(function (parser) { return new EXT_meshopt_compression(parser, MeshoptDecoder); }); */
var EXT_meshopt_compression = (function () {
    function EXT_meshopt_compression(parser, decoder) {
        this.name = "EXT_meshopt_compression";
        this._parser = parser;
        this._decoder = decoder;
    }

    EXT_meshopt_compression.prototype.loadBufferView = function (index) {
    	var bufferView = this._parser.json.bufferViews[index];

        if (bufferView.extensions && bufferView.extensions[this.name]) {
            var extensionDef = bufferView.extensions[this.name];

            var buffer = this._parser.getDependency('buffer', extensionDef.buffer);
            var decoder = this._decoder;

            return Promise.all([buffer, decoder.ready]).then(function (res) {
            	var byteOffset = extensionDef.byteOffset || 0;
            	var byteLength = extensionDef.byteLength || 0;

            	var count = extensionDef.count;
            	var stride = extensionDef.byteStride;

            	var result = new ArrayBuffer(count * stride);
            	var source = new Uint8Array(res[0], byteOffset, byteLength);

            	decoder.decodeGltfBuffer(new Uint8Array(result), count, stride, source, extensionDef.mode, extensionDef.filter);
            	return result;
            });
        } else {
            return null;
        }
    };

    return EXT_meshopt_compression;
}());

/* three.js uses JS modules exclusively since r124 */
export { EXT_meshopt_compression };
