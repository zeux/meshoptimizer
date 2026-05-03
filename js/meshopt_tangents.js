// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2026, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptTangents = (function () {
	// Built with clang version 22.1.0-wasi-sdk
	// Built from meshoptimizer 1.1
	var wasm =
		'b9H79Tebbbegv9Geueu9Geub9Gbb9Gkuuuuuuuuuuub9Giuuueuirodiblbelve9Weiiviebeoweuecj:Gdkrnlo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbK9TW79O9V9Wt9F9NW9UWV9HtW9u9H9U9NW9Ut7bel79IV9RbdDwebcekdlvq;m8Lodbk:wgvOuw99iuv99vu8Jjjjjbc;Wb9Rgk8Kjjjjbakcxfcbc;Kbz:djjjb8AakcualcdtgxalcFFFFi0Ecbyd:q:G:cjbHjjjjbbgmBdxakceBd2adci9UhPalcd4alfhscehzinazgHcethzaHas6mbkakcuaHcdtgzaHcFFFFi0Ecbyd:q:G:cjbHjjjjbbgsBdzakcdBd2ascFeazz:djjjbhOaDcd4hAarcd4hCavcd4hXdnalTmbaHcufhrcbhvindndnaOcbaiavaX2cdtfgHydlgDaDcjjjj94SEgscH4cbaoavaC2cdtfgzydlgQcs4aQcjjjj94SE7as7c:F:b:DD2cbaHydbgLaLcjjjj94SEgscH4cbazydbgKcs4aKcjjjj94SE7as7c;D;O:B8J27cbaHydwgYaYcjjjj94SEgHcH4cbazydwg8Acs4a8Acjjjj94SE7aH7c:3F;N8N27awavaA2cdtfgHydlgEaHydbg37cFFFFrGgHcm4aH7c:fjjK27arGgscdtfgHydbgzcuSmbaE::h5a3::h8Ea8A::h8FaQ::haaK::hhaY::hgaD::h8JaL::h8KcehDinaDhHdnaiazaX2cdtfgDIdba8K9CmbaDIdla8J9CmbaDIdwag9CmbaoazaC2cdtfgDIdbah9CmbaDIdlaa9CmbaDIdwa8F9CmbawazaA2cdtfgDIdba8E9CmbaDIdla59BmikaHcefhDaOasaHfarGgscdtfgHydbgzcu9hmbkkaHavBdbavhzkamavcdtfazBdbavcefgval9hmbkkaOcbydN:G:cjbH:bjjjbbakceBd2akcualcefgHcdtaHcFFFFi0Ecbyd:q:G:cjbHjjjjbbg8LBdzakcdBd2akcuadcdtadcFFFFi0EgYcbyd:q:G:cjbHjjjjbbg8MBdCakciBd2a8Lclfcbaxz:djjjbhzdnadTmbdnaeTmbaehHadhsinazamaHydbcdtfydbcdtfgDaDydbcefBdbaHclfhHascufgsmbxdkkamhHadhsinazaHydbcdtfgDaDydbcefBdbaHclfhHascufgsmbkkdnalTmbcbhsazhHalhDinaHydbhOaHasBdbaHclfhHaOasfhsaDcufgDmbkkdnadci6gQmbcbhscdhHaPhOindndnaeTmbamaeasfgvydbcdtfhDamavcwfydbcdtfhramavclfydbcdtfhvxekamasfgDcwfhraDclfhvkarydbhravydbhvazaDydbcdtfgDaDydbgDcefBdba8MaDcdtfaHc9:fBdbazavcdtfgDaDydbgDcefBdba8MaDcdtfaHcufBdbazarcdtfgDaDydbgDcefBdba8MaDcdtfaHBdbascxfhsaHclfhHaOcufgOmbkkcbhsa8LcbBdbakcuaPcltadcFFFFd0Ecbyd:q:G:cjbHjjjjbbg8NBdKakclBd2dnaQmbaehza8NhHaPhvindndnaeTmbazcwfydbhDazclfydbhOazydbhrxekascdfhDascefhOashrkJbbbbh8KJbbbbJbbbbJbbbbJbbbbJbbjZJbbj:;awaOaA2cdtfgQIdbawaraA2cdtfgLIdbgh:tawaDaA2cdtfgKIdlaLIdlgg:tg8JNaQIdlag:tggaKIdbah:tN:tghJbbbb9EEahJbbbb9BEg8EaiaraX2cdtfgrIdwghaiaOaX2cdtfgOIdwg59BEa8EarIdlgaaOIdlgy9BEa8EarIdbg8FaOIdbg8P9BEg8EahaiaDaX2cdtfgDIdwgI9BEa8EaaaDIdlg8R9BEa8Ea8FaDIdbg8S9BEg8Ea5aI9BEa8Eaya8R9BEa8Ea8Pa8S9BEh8Edna8Ja5ah:tNagaIah:tN:tghahNa8Ja8Pa8F:tNaga8Sa8F:tN:tg8Fa8FNa8Jayaa:tNaga8Raa:tN:tg8Ja8JNMMggJbbbb9Bmba8Eag:r:vh8KkaHcxfa8EUdbaHcwfaha8KNUdbaHclfa8Ja8KNUdbaHa8Fa8KNUdbazcxfhzaHczfhHascifhsavcufgvmbkkcbhHakaYcbyd:q:G:cjbHjjjjbbgDBd3akcvBd2dnadTmbaDhzinazaHBdbazclfhzadaHcefgH9hmbkkakcuaPcdtadcFFFF970Ecbyd:q:G:cjbHjjjjbbgvBdaakcoBd2akaPcbyd:q:G:cjbHjjjjbbg8ABd8KakcrBd2dnadci6mba8NcxfhzavhscbhHinasaHBdba8AaHfcdcbazIdbg8KJbbbb9DEa8KJbbbb9EV86bbasclfhsazczfhzaPaHcefgH9hmbkkdnalTmbcbhRinaRcdthHdna8LaRcefgRcdtfydbgza8LaHfydbgHSmbazaH9RhLa8MaHcdtfhKcbh8UinaKa8Ucdtfg8VydbgHcd4g8Wci2gxaHciGcdtgzyd:e:G:cjbfhHaxazydj:G:cjbfhzdnaeTmbaeaHcdtfydbhHaeazcdtfydbhzkdna8Ucefg8UaL9pmbamaHcdtfydbhYamazcdtfydbh3a8Aa8WfhEava8Wcdtfh8Xa8UhwinaKawcdtfgQydbgHcd4gzci2gAaHciGcdtgHyd:e:G:cjbfhsaAaHydj:G:cjbfhHdnaeTmbaeascdtfydbhsaeaHcdtfydbhHkdndnamaHcdtfydbaYSmbamascdtfydba39hmeka8AazfRbbgHaERbbgsVciSmbdnaHasGmba8Whsdna8Wa8XydbgHSmba8XhOinaOavaHgscdtfgrydbgHBdbarhOasaH9hmbkkdnazavazcdtfgOydbgHSmbinaOavaHgzcdtfgrydbgHBdbarhOazaH9hmbkkasazSmba8AazfgORbba8AasfgHRbbVciSmeavazcdtfasBdbaHaHRbbaORbbV86bbkdna8VydbciGaxfgzaDazcdtfgsydbgHSmbinasaDaHgzcdtfgOydbgHBdbaOhsazaH9hmbkkdnaQydbciGaAfgsaDascdtfgOydbgHSmbinaOaDaHgscdtfgrydbgHBdbarhOasaH9hmbkkazasSmbaDascdtfazBdbkawcefgwaL9hmbkka8UaL9hmbkkaRal9hmbkkdnadTmbcbhrinarhzdnaraDarcdtfgwydbgHSmbawhsinasaDaHgzcdtfgOydbgHBdbaOhsazaH9hmbkkawazBdbarcefgrad9hmbkcbh8Vabcbadcltz:djjjbhKdnadci6mbaqceGhEaDhYaeh3cbh8Windna8Na8WcltfgsIdxJbbbb9Bmbama8Wcx2gHfhxaeaHfhLcbhza8VhwinaKaYazfydbcltfhHdndnaeTmbamaLazc:e:G:cjbfydbcdtfydbcdtfhAamaLazcj:G:cjbfydbcdtfydbcdtfhra3azfydbhOxekaxazc:e:G:cjbfydbcdtfhAaxazcj:G:cjbfydbcdtfhrawhOkaHasIdbggaoamaOcdtfydbgQaC2cdtfgOIdbg8KasIdwg8FaOIdwg8JNaga8KNasIdlg8EaOIdlggNMMghN:tgaJbbbbJbbjZa8Fa8JahN:tg8Fa8FNaaaaNa8EagahN:tghahNMMga:r:vaaJbbbb9BEJ;As6nJbbjZaiarydbaX2cdtfgOIdwaiaQaX2cdtfgrIdwg5:tgaa8Jaaa8JNaOIdbarIdbgy:tg8Pa8KNagaOIdlarIdlgI:tg8RNMMgaN:tg8EaiaAydbaX2cdtfgOIdwa5:tg5a8Ja5a8JNaOIdbay:tg8Sa8KNagaOIdlaI:tgINMMg5N:tg8JNa8Pa8KaaN:tgya8Sa8Ka5N:tg8KNa8RagaaN:tgaaIaga5N:tggNMMJbbbbJbbjZa8Ea8ENayayNaaaaNMMa8Ja8JNa8Ka8KNagagNMMNg8K:rg8J:va8KJbbbb9BENgg:lg8Ka8KJbbjZ9EEg8Ka8KJ7;A9s89NJ:L9t9s::MNJ;ob;jZMJbbjZa8K:t:rNg8K:ta8KagJbbbb9DENg8Ka8Ja8KNaEEg8KNaHIdbMUdbaHaha8KNaHIdlMUdlaHa8Fa8KNaHIdwMUdwawcefhwazclfgzcx9hmbkkaYcxfhYa3cxfh3a8Vcifh8Va8Wcefg8WaP9hmbkcbhrinarhzdnaravarcdtfgsydbgHSmbinasavaHgzcdtfgOydbgHBdbaOhsazaH9hmbkkaKarc8W2fgHc3fJbbjZJbbj:;a8AazfRbbceGEg8KUdbaHc8Sfa8KUdbaHa8KUdxarcefgraP9hmbkkaqcdGhvcbhzaDhsaKhHindnazasydb9hmbJbbbbh8KdnaHcwfgOIdbg8Ja8JNaHIdbggagNaHclfgrIdbghahNMMgaJbbbb9BmbJbbjZaa:r:vh8KkaOa8Ja8KNUdbaraha8KNUdbaHaga8KNg8JUdbavmbaHJbbjZa8Ja8KJbbbb9BEUdbkasclfhsaHczfhHadazcefgz9hmbkcbhHaKhzindnaHaDydbgsSmbazaKascltfgsydwBdwazas8Pdb83dbkaDclfhDazczfhzadaHcefgH9hmbkkdnakyd2gzTmbazcdtakcxffc98fhHinaHydbcbydN:G:cjbH:bjjjbbaHc98fhHazcufgzmbkkakc;Wbf8Kjjjjbk9teiucbcbyd:y:G:cjbgeabcifc98GfgbBd:y:G:cjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd:y:G:cjbgeabcrfc94GfgbBd:y:G:cjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaikTeeucbabcbyd:y:G:cjbge9Rcifc98GaefgbBd:y:G:cjbdnabZbcztge9nmbabae9RcFFifcz4nb8Akkk8Rdbcj:Gdkzebbbdbbbbbbbebbbbc:q:Gdkxebbbdbbba:qbb'; // embed! wasm

	var wasmpack = new Uint8Array([
		32, 0, 65, 2, 1, 106, 34, 33, 3, 128, 11, 4, 13, 64, 6, 253, 10, 7, 15, 116, 127, 5, 8, 12, 40, 16, 19, 54, 20, 9, 27, 255, 113, 17, 42, 67,
		24, 23, 146, 148, 18, 14, 22, 45, 70, 69, 56, 114, 101, 21, 25, 63, 75, 136, 108, 28, 118, 29, 73, 115,
	]);

	if (typeof WebAssembly !== 'object') {
		return {
			supported: false,
		};
	}

	var instance;

	var ready = WebAssembly.instantiate(unpack(wasm), {}).then(function (result) {
		instance = result.instance;
		instance.exports.__wasm_call_ctors();
	});

	function unpack(data) {
		var result = new Uint8Array(data.length);
		for (var i = 0; i < data.length; ++i) {
			var ch = data.charCodeAt(i);
			result[i] = ch > 96 ? ch - 97 : ch > 64 ? ch - 39 : ch + 4;
		}
		var write = 0;
		for (var i = 0; i < data.length; ++i) {
			result[write++] = result[i] < 60 ? wasmpack[result[i]] : (result[i] - 60) * 64 + result[++i];
		}
		return result.buffer.slice(0, write);
	}

	function assert(cond) {
		if (!cond) {
			throw new Error('Assertion failed');
		}
	}

	function bytes(view) {
		return new Uint8Array(view.buffer, view.byteOffset, view.byteLength);
	}

	function generate(
		indices,
		index_count,
		vertex_positions,
		vertex_count,
		vertex_positions_stride,
		vertex_normals,
		vertex_normals_stride,
		vertex_uvs,
		vertex_uvs_stride,
		options
	) {
		var sbrk = instance.exports.sbrk;

		var resultp = sbrk(index_count * 16);
		var indicesp = indices ? sbrk(indices.byteLength) : 0;
		var positionsp = sbrk(vertex_positions.byteLength);
		var normalsp = sbrk(vertex_normals.byteLength);
		var uvsp = sbrk(vertex_uvs.byteLength);

		var heap = new Uint8Array(instance.exports.memory.buffer);
		if (indices) heap.set(bytes(indices), indicesp);
		heap.set(bytes(vertex_positions), positionsp);
		heap.set(bytes(vertex_normals), normalsp);
		heap.set(bytes(vertex_uvs), uvsp);

		instance.exports.meshopt_generateTangents(
			resultp,
			indicesp,
			index_count,
			positionsp,
			vertex_count,
			vertex_positions_stride * 4,
			normalsp,
			vertex_normals_stride * 4,
			uvsp,
			vertex_uvs_stride * 4,
			options
		);

		// heap may have grown
		heap = new Uint8Array(instance.exports.memory.buffer);

		var result = new Float32Array(heap.buffer, resultp, index_count * 4).slice();
		sbrk(resultp - sbrk(0));

		return result;
	}

	var tangentOptions = {
		Compatible: 1,
		ZeroFallback: 2,
	};

	return {
		ready: ready,
		supported: true,
		generateTangents: function (
			indices,
			vertex_positions,
			vertex_positions_stride,
			vertex_normals,
			vertex_normals_stride,
			vertex_uvs,
			vertex_uvs_stride,
			flags
		) {
			assert(
				indices === null ||
					indices instanceof Uint32Array ||
					indices instanceof Int32Array ||
					indices instanceof Uint16Array ||
					indices instanceof Int16Array
			);
			assert(indices === null || indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(vertex_normals instanceof Float32Array);
			assert(vertex_normals.length % vertex_normals_stride == 0);
			assert(vertex_normals_stride >= 3);
			assert(vertex_uvs instanceof Float32Array);
			assert(vertex_uvs.length % vertex_uvs_stride == 0);
			assert(vertex_uvs_stride >= 2);
			assert(vertex_positions.length / vertex_positions_stride == vertex_normals.length / vertex_normals_stride);
			assert(vertex_positions.length / vertex_positions_stride == vertex_uvs.length / vertex_uvs_stride);
			assert(indices !== null || (vertex_positions.length / vertex_positions_stride) % 3 == 0);

			var options = 0;
			for (var i = 0; i < (flags ? flags.length : 0); ++i) {
				assert(flags[i] in tangentOptions);
				options |= tangentOptions[flags[i]];
			}

			var vertex_count = vertex_positions.length / vertex_positions_stride;
			var index_count = indices ? indices.length : vertex_count;

			var indices32 = indices === null || indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			return generate(
				indices32,
				index_count,
				vertex_positions,
				vertex_count,
				vertex_positions_stride,
				vertex_normals,
				vertex_normals_stride,
				vertex_uvs,
				vertex_uvs_stride,
				options
			);
		},
	};
})();

export { MeshoptTangents };
