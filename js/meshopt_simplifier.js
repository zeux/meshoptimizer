// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2023, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function() {
	"use strict";

	// Built with clang version 15.0.6
	// Built from meshoptimizer 0.19
	var wasm = "b9H79TebbbecD9Geueu9Geub9Gbb9Gquuuuuuu99uueu9Gvuuuuub9Gluuuue999Giuuue999Gluuuueu9Giuuueuimxdilvorbwwbewlve9Weiiviebeoweuecj;jekr7oo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95beX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbla9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbvl79IV9RboDwebcekdDqq:XJxdbkp8WiKuP99Hu8Jjjjjbcj;bb9Rgq8KjjjjbaqcKfcbc;Kbz1jjjb8AaqcualcdtgkalcFFFFi0Egxcbyd;S1jjbHjjjjbbgmBdKaqceBd94aqamBdwaqaxcbyd;S1jjbHjjjjbbgPBd3aqcdBd94aqaPBdxaqcuadcitadcFFFFe0Ecbyd;S1jjbHjjjjbbgsBdaaqciBd94aqasBdzaqcwfaeadalcbz:cjjjbaqaxcbyd;S1jjbHjjjjbbgzBd8KaqclBd94aqaxcbyd;S1jjbHjjjjbbgHBdyaqcvBd94alcd4alfhOcehAinaAgCcethAaCaO6mbkcbhXaqcuaCcdtgAaCcFFFFi0Ecbyd;S1jjbHjjjjbbgOBd8SaqcoBd94aOcFeaAz1jjjbhQdnalTmbavcd4hLaCcufhKinaiaXaL2cdtfgYydlgCcH4aC7c:F:b:DD2aYydbgCcH4aC7c;D;O:B8J27aYydwgCcH4aC7c:3F;N8N27hOcbhCdndninaQaOaKGgOcdtfg8AydbgAcuSmeaiaAaL2cdtfaYcxz:ljjjbTmdaCcefgCaOfhOaCaK9nmbxdkka8AaXBdbaXhAkazaXcdtfaABdbaXcefgXal9hmbkcbhCaHhAinaAaCBdbaAclfhAalaCcefgC9hmbkcbhCazhAaHhOindnaCaAydbgKSmbaOaHaKcdtfgKydbBdbaKaCBdbkaAclfhAaOclfhOalaCcefgC9hmbkkcbhOaqalcbyd;S1jjbHjjjjbbgYBd8WaqcrBd94aqaxcbyd;S1jjbHjjjjbbgCBd80aqcwBd94aqaxcbyd;S1jjbHjjjjbbgABdUaqcDBd94aCcFeakz1jjjbhEaAcFeakz1jjjbh3dnalTmbascwfh5indnamaOcdtgCfydbg8ETmbasaPaCfydbcitfh8Fa3aCfhaaEaCfhXcbhLindndna8FaLcitfydbgQaO9hmbaXaOBdbaaaOBdbxekdnamaQcdtgkfydbghTmbasaPakfydbcitgCfydbaOSmeahcufh8Aa5aCfhAcbhCina8AaCSmeaCcefhCaAydbhKaAcwfhAaKaO9hmbkaCah6meka3akfgCaOaQaCydbcuSEBdbaXaQaOaXydbcuSEBdbkaLcefgLa8E9hmbkkaOcefgOal9hmbkazhAaHhOa3hKaEhLcbhCindndnaCaAydbg8A9hmbdnaCaOydbg8A9hmbaLydbh8AdnaKydbgQcu9hmba8Acu9hmbaYaCfcb86bbxikaYaCfhXdnaCaQSmbaCa8ASmbaXce86bbxikaXcl86bbxdkdnaCaHa8AcdtgQfydb9hmbdnaKydbgXcuSmbaCaXSmbaLydbgkcuSmbaCakSmba3aQfydbg8EcuSmba8Ea8ASmbaEaQfydbgQcuSmbaQa8ASmbdnazaXcdtfydbazaQcdtfydb9hmbazakcdtfydbaza8Ecdtfydb9hmbaYaCfcd86bbxlkaYaCfcl86bbxikaYaCfcl86bbxdkaYaCfcl86bbxekaYaCfaYa8AfRbb86bbkaAclfhAaOclfhOaKclfhKaLclfhLalaCcefgC9hmbkawceGTmbaYhCalhAindnaCRbbce9hmbaCcl86bbkaCcefhCaAcufgAmbkkcbhLcualcx2alc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbhmaqcKfaqyd94gCcdtfamBdbaqaCcefgABd94amaialavz:djjjb8AaqcKfaAcdtfcualc8S2gAalc;D;O;f8U0Ecbyd;S1jjbHjjjjbbgOBdbaqaCcdfBd94aOcbaAz1jjjbhsdnadTmbaehAindnamaAclfydbg8Acx2fgCIdbamaAydbgQcx2fgOIdbgg:tg8JamaAcwfydbgXcx2fgKIdlaOIdlg8K:tg8LNaKIdbag:tg8MaCIdla8K:tg8NN:tgyayNa8NaKIdwaOIdwg8P:tgINa8LaCIdwa8P:tg8NN:tg8La8LNa8Na8MNaIa8JN:tg8Ja8JNMM:rg8MJbbbb9ETmbaya8M:vhya8Ja8M:vh8Ja8La8M:vh8LkasazaQcdtfydbc8S2fgCa8La8M:rg8Ma8LNNg8NaCIdbMUdbaCa8Ja8Ma8JNg8RNgIaCIdlMUdlaCaya8MayNg8SNgRaCIdwMUdwaCa8Ra8LNg8RaCIdxMUdxaCa8Sa8LNg8UaCIdzMUdzaCa8Sa8JNg8SaCIdCMUdCaCa8La8Maya8PNa8LagNa8Ka8JNMM:mg8KNggNg8LaCIdKMUdKaCa8JagNg8JaCId3MUd3aCayagNgyaCIdaMUdaaCaga8KNggaCId8KMUd8KaCa8MaCIdyMUdyasaza8Acdtfydbc8S2fgCa8NaCIdbMUdbaCaIaCIdlMUdlaCaRaCIdwMUdwaCa8RaCIdxMUdxaCa8UaCIdzMUdzaCa8SaCIdCMUdCaCa8LaCIdKMUdKaCa8JaCId3MUd3aCayaCIdaMUdaaCagaCId8KMUd8KaCa8MaCIdyMUdyasazaXcdtfydbc8S2fgCa8NaCIdbMUdbaCaIaCIdlMUdlaCaRaCIdwMUdwaCa8RaCIdxMUdxaCa8UaCIdzMUdzaCa8SaCIdCMUdCaCa8LaCIdKMUdKaCa8JaCId3MUd3aCayaCIdaMUdaaCagaCId8KMUd8KaCa8MaCIdyMUdyaAcxfhAaLcifgLad6mbkcbh8AaehXincbhAinaYaeaAc:81jjbfydbgQa8AfcdtfydbgOfRbbhCdndnaYaXaAfydbgKfRbbgLc99fcFeGcpe0mbaCceSmbaCcd9hmekdnaLcufcFeGce0mbaEaKcdtfydbaO9hmekdnaCcufcFeGce0mba3aOcdtfydbaK9hmekdnaLcv2aCfc:G1jjbfRbbTmbazaOcdtfydbazaKcdtfydb0mekJbbacJbbjZaCceSEh8MaLceShkamaeaQcdtc:81jjbfydba8Afcdtfydbcx2fhCdnamaOcx2fgLIdwamaKcx2fgQIdwg8K:tg8La8LNaLIdbaQIdbg8P:tg8Ja8JNaLIdlaQIdlg8N:tgyayNMM:rggJbbbb9ETmba8Lag:vh8Layag:vhya8Jag:vh8JkJbbaca8MakEh8SdnaCIdwa8K:tg8Ma8La8Ma8LNaCIdba8P:tgRa8JNayaCIdla8N:tg8RNMMgIN:tg8Ma8MNaRa8JaIN:tg8La8LNa8RayaIN:tg8Ja8JNMM:rgyJbbbb9ETmba8May:vh8Ma8Jay:vh8Ja8Lay:vh8LkasazaKcdtfydbc8S2fgCa8La8SagNgya8LNNgIaCIdbMUdbaCa8Jaya8JNg8SNgRaCIdlMUdlaCa8Maya8MNggNg8RaCIdwMUdwaCa8Sa8LNg8SaCIdxMUdxaCaga8LNg8UaCIdzMUdzaCaga8JNg8VaCIdCMUdCaCa8Laya8Ma8KNa8La8PNa8Na8JNMM:mg8KNggNg8LaCIdKMUdKaCa8JagNg8JaCId3MUd3aCa8MagNg8MaCIdaMUdaaCaga8KNggaCId8KMUd8KaCayaCIdyMUdyasazaOcdtfydbc8S2fgCaIaCIdbMUdbaCaRaCIdlMUdlaCa8RaCIdwMUdwaCa8SaCIdxMUdxaCa8UaCIdzMUdzaCa8VaCIdCMUdCaCa8LaCIdKMUdKaCa8JaCId3MUd3aCa8MaCIdaMUdaaCagaCId8KMUd8KaCayaCIdyMUdykaAclfgAcx9hmbkaXcxfhXa8Acifg8Aad6mbkkdnabaeSmbabaeadcdtz:hjjjb8Akcuadcx2adc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbhaaqcKfaqyd94gCcdtfaaBdbaqaCcefgABd94aqcKfaAcdtfcuadcdtadcFFFFi0Ecbyd;S1jjbHjjjjbbg5BdbaqaCcdfgABd94aqcKfaAcdtfaxcbyd;S1jjbHjjjjbbgiBdbaqaCcifgABd94aqcKfaAcdtfalcbyd;S1jjbHjjjjbbg8WBdbaqaCclfBd94JbbbbhRdnadao9nmbararNh8Saacwfh8Xaqydwh8Yaqydxh8Zaqydzh80JbbbbhRinaqcwfabadgPalazz:cjjjbcbhhabhXcbhkincbhCindnazaXaCfydbgOcdtgefydbgLazabaCc:81jjbfydbakfcdtfydbgAcdtfydbg8ASmbaYaAfRbbgQcv2aYaOfRbbgKfc;q1jjbfRbbg8FaKcv2aQfg8Ec;q1jjbfRbbgdVcFeGTmbdna8Ec:G1jjbfRbbTmba8AaL0mekdnaKaQ9hmbaKcufcFeGce0mbaEaefydbaA9hmekaaahcx2fgKaAaOadcFeGgLEBdlaKaOaAaLEBdbaKaLa8FGcb9hBdwahcefhhkaCclfgCcx9hmbkaXcxfhXakcifgkaP6mbkdndnahTmbaahAahh8AinaAcwfgQJbbbbJbbjZasazaAydbgOcdtfydbc8S2fgCIdyg8L:va8LJbbbb9BEaCIdwamaAclfgeydbgKcx2fgLIdwg8LNaCIdzaLIdbg8JNaCIdaMg8Ma8MMMa8LNaCIdlaLIdlg8MNaCIdCa8LNaCId3Mg8La8LMMa8MNaCIdba8JNaCIdxa8MNaCIdKMg8La8LMMa8JNaCId8KMMM:lNgyJbbbbJbbjZasazaKaOaQydbgLEgQcdtfydbc8S2fgCIdyg8L:va8LJbbbb9BEaCIdwamaOaKaLEgXcx2fgLIdwg8LNaCIdzaLIdbg8JNaCIdaMg8Ma8MMMa8LNaCIdlaLIdlg8MNaCIdCa8LNaCId3Mg8La8LMMa8MNaCIdba8JNaCIdxa8MNaCIdKMg8La8LMMa8JNaCId8KMMM:lNg8Laya8L9FgCEUdbaeaKaXaCEBdbaAaOaQaCEBdbaAcxfhAa8Acufg8Ambkaqcjefcbcj;abz1jjjb8Aa8XhCahhAinaqcjefaCydbcO4c;8ZGfgOaOydbcefBdbaCcxfhCaAcufgAmbkcbhCcbhAinaqcjefaCfgOydbhKaOaABdbaKaAfhAaCclfgCcj;ab9hmbkcbhCa8XhAinaqcjefaAydbcO4c;8ZGfgOaOydbgOcefBdba5aOcdtfaCBdbaAcxfhAahaCcefgC9hmbkaPao9RgOci9Uh81dnalTmbcbhCaihAinaAaCBdbaAclfhAalaCcefgC9hmbkkcbhBa8Wcbalz1jjjbh83aOcO9UhUa81ce4h85cbh86cbhkdninaaa5akcdtfydbcx2fgXIdwg8Ja8S9Emea86a819pmeJFFuuh8Ldna85ah9pmbaaa5a85cdtfydbcx2fIdwJbb;aZNh8Lkdna8Ja8L9ETmba86aU0mdkdna83azaXydlg87cdtg88fydbgOfg89Rbba83azaXydbgecdtg8:fydbgZfgnRbbVmbdna8YaZcdtgCfydbgKTmba80a8ZaCfydbcitfhCamaOcx2fg8Ecwfhda8EclfhxamaZcx2fg8Fcwfhva8FclfhwcbhAcehQdnindnaiaCydbcdtfydbgLaOSmbaiaCclfydbcdtfydbg8AaOSmbama8Acx2fg8AIdbamaLcx2fgLIdbg8M:tg8LawIdbaLIdlgy:tggNa8FIdba8M:tg8Ka8AIdlay:tg8JN:ta8LaxIdbay:tg8PNa8EIdba8M:tg8Na8JN:tNa8JavIdbaLIdwgy:tgINaga8AIdway:tg8MN:ta8JadIdbay:tgyNa8Pa8MN:tNa8Ma8KNaIa8LN:ta8Ma8NNaya8LN:tNMMJbbbb9DmdkaCcwfhCaAcefgAaK6hQaKaA9hmbkkaQceGTmba85cefh85xekaXcwfhKasaOc8S2fgCasaZc8S2fgAIdbaCIdbMUdbaCaAIdlaCIdlMUdlaCaAIdwaCIdwMUdwaCaAIdxaCIdxMUdxaCaAIdzaCIdzMUdzaCaAIdCaCIdCMUdCaCaAIdKaCIdKMUdKaCaAId3aCId3MUd3aCaAIdaaCIdaMUdaaCaAId8KaCId8KMUd8KaCaAIdyaCIdyMUdydndndndnaYaefgARbbc9:fPdebdkaehCinaiaCcdtgCfaOBdbaHaCfydbgCae9hmbxikkaHa88fydbhCaHa8:fydbheaia8:fa87BdbaCh87kaiaecdtfa87Bdbkance86bba89ce86bbaKIdbg8LaRaRa8L9DEhRaBcefhBcecdaARbbceSEa86fh86kakcefgkah9hmbkkaBTmbdnalTmbcbhAaEhCindnaCydbgOcuSmbdnaAaiaOcdtgKfydbgO9hmbaEaKfydbhOkaCaOBdbkaCclfhCalaAcefgA9hmbkcbhAa3hCindnaCydbgOcuSmbdnaAaiaOcdtgKfydbgO9hmba3aKfydbhOkaCaOBdbkaCclfhCalaAcefgA9hmbkkcbhdabhCcbhLindnaiaCydbcdtfydbgAaiaCclfydbcdtfydbgOSmbaAaiaCcwfydbcdtfydbgKSmbaOaKSmbabadcdtfg8AaABdba8AclfaOBdba8AcwfaKBdbadcifhdkaCcxfhCaLcifgLaP9pmdxbkkaPhdxdkadao0mbkkdnaDTmbaDaR:rUdbkaqyd94gCcdtaqcKffc98fhzdninaCTmeazydbcbyd;W1jjbH:bjjjbbazc98fhzaCcufhCxbkkaqcj;bbf8Kjjjjbadk;pleouabydbcbaicdtz1jjjb8Aadci9UhvdnadTmbabydbhodnalTmbaehradhwinaoalarydbcdtfydbcdtfgDaDydbcefBdbarclfhrawcufgwmbxdkkaehradhwinaoarydbcdtfgDaDydbcefBdbarclfhrawcufgwmbkkdnaiTmbabydbhrabydlhwcbhDaihoinawaDBdbawclfhwarydbaDfhDarclfhraocufgombkkdnadci6mbavceavce0EhqabydlhvabydwhrinaecwfydbhwaeclfydbhDaeydbhodnalTmbalawcdtfydbhwalaDcdtfydbhDalaocdtfydbhokaravaocdtfgdydbcitfaDBdbaradydbcitfawBdladadydbcefBdbaravaDcdtfgdydbcitfawBdbaradydbcitfaoBdladadydbcefBdbaravawcdtfgwydbcitfaoBdbarawydbcitfaDBdlawawydbcefBdbaecxfheaqcufgqmbkkdnaiTmbabydlhrabydbhwinararydbawydb9RBdbawclfhwarclfhraicufgimbkkk:3ldouv998Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdnadTmbaicd4hvdnabTmbavcdthocbhraehwinabarcx2fgiaearav2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinalczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawaofhwarcefgrad9hmbxdkkavcdthrcbhwincbhiinalczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbkkalIdbalIdzgk:tJbbbb:xgqalIdlalIdCgx:tgmamaq9DEgqalIdwalIdKgm:tgPaPaq9DEhPdnabTmbadTmbJbbbbJbbjZaP:vaPJbbbb9BEhqinabaqabIdbak:tNUdbabclfgiaqaiIdbax:tNUdbabcwfgiaqaiIdbam:tNUdbabcxfhbadcufgdmbkkaPk:Qdidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgoaDIdbgrarao9EEUdbaladfgDaoaDIdbgrarao9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkavJbbbb:xgvaoaoav9DEgoararao9DEk9DeeuabcFeaicdtz1jjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd;01jjbgeabcifc98GfgbBd;01jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd;01jjbgeabcrfc94GfgbBd;01jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd;01jjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd;01jjbfgdBd;01jjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:cedbcjwk9PFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbeeebeebebbeeebebbbbbebebbbbbebbbdbbbbbbbbbbbbbbbeeeeebebbbbbebbbbbeebbbbbbc;Swkxebbbdbbbj9Kbb";

	var wasmpack = new Uint8Array([32,0,65,2,1,106,34,33,3,128,11,4,13,64,6,253,10,7,15,116,127,5,8,12,40,16,19,54,20,9,27,255,113,17,42,67,24,23,146,148,18,14,22,45,70,69,56,114,101,21,25,63,75,136,108,28,118,29,73,115]);

	if (typeof WebAssembly !== 'object') {
		return {
			supported: false,
		};
	}

	var instance;

	var ready =
		WebAssembly.instantiate(unpack(wasm), {})
		.then(function(result) {
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
			result[write++] = (result[i] < 60) ? wasmpack[result[i]] : (result[i] - 60) * 64 + result[++i];
		}
		return result.buffer.slice(0, write);
	}

	function assert(cond) {
		if (!cond) {
			throw new Error("Assertion failed");
		}
	}

	function bytes(view) {
		return new Uint8Array(view.buffer, view.byteOffset, view.byteLength);
	}

	function reorder(indices, vertices) {
		var sbrk = instance.exports.sbrk;
		var ip = sbrk(indices.length * 4);
		var rp = sbrk(vertices * 4);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		var indices8 = bytes(indices);
		heap.set(indices8, ip);
		var unique = instance.exports.meshopt_optimizeVertexFetchRemap(rp, ip, indices.length, vertices);
		// heap may have grown
		heap = new Uint8Array(instance.exports.memory.buffer);
		var remap = new Uint32Array(vertices);
		new Uint8Array(remap.buffer).set(heap.subarray(rp, rp + vertices * 4));
		indices8.set(heap.subarray(ip, ip + indices.length * 4));
		sbrk(ip - sbrk(0));

		for (var i = 0; i < indices.length; ++i)
			indices[i] = remap[indices[i]];

		return [remap, unique];
	}

	function maxindex(source) {
		var result = 0;
		for (var i = 0; i < source.length; ++i) {
			var index = source[i];
			result = result < index ? index : result;
		}
		return result;
	}

	function simplify(fun, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride, target_index_count, target_error, options) {
		var sbrk = instance.exports.sbrk;
		var te = sbrk(4);
		var ti = sbrk(index_count * 4);
		var sp = sbrk(vertex_count * vertex_positions_stride);
		var si = sbrk(index_count * 4);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(vertex_positions), sp);
		heap.set(bytes(indices), si);
		var result = fun(ti, si, index_count, sp, vertex_count, vertex_positions_stride, target_index_count, target_error, options, te);
		// heap may have grown
		heap = new Uint8Array(instance.exports.memory.buffer);
		var target = new Uint32Array(result);
		bytes(target).set(heap.subarray(ti, ti + result * 4));
		var error = new Float32Array(1);
		bytes(error).set(heap.subarray(te, te + 4));
		sbrk(te - sbrk(0));
		return [target, error[0]];
	}

	function simplifyScale(fun, vertex_positions, vertex_count, vertex_positions_stride) {
		var sbrk = instance.exports.sbrk;
		var sp = sbrk(vertex_count * vertex_positions_stride);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(vertex_positions), sp);
		var result = fun(sp, vertex_count, vertex_positions_stride);
		sbrk(sp - sbrk(0));
		return result;
	}

	var simplifyOptions = {
		LockBorder: 1,
	};

	return {
		ready: ready,
		supported: true,

		compactMesh: function(indices) {
			assert(indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array);
			assert(indices.length % 3 == 0);

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			return reorder(indices32, maxindex(indices) + 1);
		},

		simplify: function(indices, vertex_positions, vertex_positions_stride, target_index_count, target_error, flags) {
			assert(indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(target_index_count % 3 == 0);
			assert(target_error >= 0 && target_error <= 1);

			var options = 0;
			for (var i = 0; i < (flags ? flags.length : 0); ++i) {
				options |= simplifyOptions[flags[i]];
			}

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			var result = simplify(instance.exports.meshopt_simplify, indices32, indices.length, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4, target_index_count, target_error, options);
			result[0] = (indices instanceof Uint32Array) ? result[0] : new indices.constructor(result[0]);

			return result;
		},

		getScale: function(vertex_positions, vertex_positions_stride) {
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);

			return simplifyScale(instance.exports.meshopt_simplifyScale, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4);
		},
	};
})();

// UMD-style export for MeshoptSimplifier
if (typeof exports === 'object' && typeof module === 'object')
	module.exports = MeshoptSimplifier;
else if (typeof define === 'function' && define['amd'])
	define([], function() {
		return MeshoptSimplifier;
	});
else if (typeof exports === 'object')
	exports["MeshoptSimplifier"] = MeshoptSimplifier;
else
	(typeof self !== 'undefined' ? self : this).MeshoptSimplifier = MeshoptSimplifier;
