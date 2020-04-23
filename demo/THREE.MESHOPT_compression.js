/* three.js extension for MESHOPT_compression; requires three.js v?? (PR19144) */
/* loader.register(function (parser) { return new MESHOPT_compression(parser, MeshoptDecoder); }); */
var MESHOPT_compression = (function () {
	var NAME = "MESHOPT_compression";

    function MESHOPT_compression(parser, decoder) {
        this.name = NAME;
        this._parser = parser;
        this._decoder = decoder;
    }

    MESHOPT_compression.prototype.loadBufferView = function (index) {
    	var bufferView = this._parser.json.bufferViews[index];

        if (bufferView.extensions && bufferView.extensions[NAME]) {
            var extensionDef = bufferView.extensions[NAME];

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

    return MESHOPT_compression;
}());
