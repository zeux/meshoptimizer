// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function () {
	'use strict';
	// Built with clang version 19.1.5-wasi-sdk
	// Built from meshoptimizer 0.24
	var wasm =
		'b9H79Tebbbe:mes9Geueu9Geub9Gbb9Gsuuuuuuuuuuuu99uueu9Gvuuuuub9Gruuuuuuub9Gvuuuuue999Gvuuuuueu9Gquuuuuuu99uueu9GDuuuuuuu99ueu9Gruuuuuu99eu9Gwuuuuuu99ueu9Giuuue999Gluuuueu9GiuuueuiCAdilvorlwiDqkxmbPPbelve9Weiiviebeoweuec:G:Pdkr;Eeqo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95br8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bwQ9TW79O9V9Wt9F79P9T9W29P9M959t29V9W9W95bDX9TW79O9V9Wt9F79P9T9W29P9M959qV919UWbqQ9TW79O9V9Wt9F79P9T9W29P9M959q9V9P9Ut7bkX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbxa9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbml79IV9RbPDwebcekdHOq:X:7eAdbk;d9Iv8Aue99euY99Au8Jjjjjbc;W;qb9Rgs8Kjjjjbcbhzascxfcbc;Kbz:qjjjb8AdnabaeSmbabaeadcdtz:pjjjb8AkdndnamcdGmbascxfhHcbhOxekasalcrfci4gecbyd1:jjjbHjjjjbbgABdxasceBd2aAcbaez:qjjjbhCcbhlcbhednadTmbcbhlabheadhAinaCaeydbgXci4fgQaQRbbgQceaXcrGgXtV86bbaQcu7aX4ceGalfhlaeclfheaAcufgAmbkcualcdtalcFFFFi0EhekascCfhHasaecbyd1:jjjbHjjjjbbgOBdzascdBd2alcd4alfhXcehAinaAgecethAaeaX6mbkcdhzcbhLascuaecdtgAaecFFFFi0Ecbyd1:jjjbHjjjjbbgXBdCasciBd2aXcFeaAz:qjjjbhKdnadTmbaecufhYcbh8AindndnaKabaLcdtfgEydbgQc:v;t;h;Ev2aYGgXcdtfgCydbgAcuSmbceheinaOaAcdtfydbaQSmdaXaefhAaecefheaKaAaYGgXcdtfgCydbgAcu9hmbkkaOa8AcdtfaQBdbaCa8ABdba8AhAa8Acefh8AkaEaABdbaLcefgLad9hmbkkaKcbyd:m:jjjbH:bjjjbbascdBd2kcbh3aHcualcefgecdtaecFFFFi0Ecbyd1:jjjbHjjjjbbg5Bdbasa5BdlasazceVgeBd2ascxfaecdtfcuadcitadcFFFFe0Ecbyd1:jjjbHjjjjbbg8EBdbasa8EBdwasazcdfgeBd2asclfabadalcbz:cjjjbascxfaecdtfcualcdtgealcFFFFi0Eg8Fcbyd1:jjjbHjjjjbbgABdbasazcifgXBd2ascxfaXcdtfa8Fcbyd1:jjjbHjjjjbbgaBdbasazclVBd2aAaaaialavaOascxfz:djjjbalcbyd1:jjjbHjjjjbbhCascxfasyd2ghcdtfaCBdbasahcefgXBd2ascxfaXcdtfa8Fcbyd1:jjjbHjjjjbbgXBdbasahcdfgQBd2ascxfaQcdtfa8Fcbyd1:jjjbHjjjjbbgQBdbasahcifggBd2aXcFeaez:qjjjbh8JaQcFeaez:qjjjbh8KdnalTmba8Ecwfh8Lindna5a3gQcefg3cdtfydbgKa5aQcdtgefydbgXSmbaKaX9Rhza8EaXcitfhHa8Kaefh8Ma8JaefhEcbhYindndnaHaYcitfydbg8AaQ9hmbaEaQBdba8MaQBdbxekdna5a8Acdtg8NfgeclfydbgXaeydbgeSmba8EaecitgKfydbaQSmeaXae9Rhyaecu7aXfhLa8LaKfhXcbheinaLaeSmeaecefheaXydbhKaXcwfhXaKaQ9hmbkaeay6meka8Ka8NfgeaQa8AaeydbcuSEBdbaEa8AaQaEydbcuSEBdbkaYcefgYaz9hmbkka3al9hmbkaAhXaahQa8KhKa8JhYcbheindndnaeaXydbg8A9hmbdnaeaQydbg8A9hmbaYydbh8AdnaKydbgLcu9hmba8Acu9hmbaCaefcb86bbxikaCaefhEdnaeaLSmbaea8ASmbaEce86bbxikaEcl86bbxdkdnaeaaa8AcdtgLfydb9hmbdnaKydbgEcuSmbaeaESmbaYydbgzcuSmbaeazSmba8KaLfydbgHcuSmbaHa8ASmba8JaLfydbgLcuSmbaLa8ASmbdnaAaEcdtfydbg8AaAaLcdtfydb9hmba8AaAazcdtfydbgLSmbaLaAaHcdtfydb9hmbaCaefcd86bbxlkaCaefcl86bbxikaCaefcl86bbxdkaCaefcl86bbxekaCaefaCa8AfRbb86bbkaXclfhXaQclfhQaKclfhKaYclfhYalaecefge9hmbkdnaqTmbdndnaOTmbaOheaAhXalhQindnaqaeydbfRbbTmbaCaXydbfcl86bbkaeclfheaXclfhXaQcufgQmbxdkkaAhealhXindnaqRbbTmbaCaeydbfcl86bbkaqcefhqaeclfheaXcufgXmbkkaAhealhQaChXindnaCaeydbfRbbcl9hmbaXcl86bbkaeclfheaXcefhXaQcufgQmbkkamceGTmbaChealhXindnaeRbbce9hmbaecl86bbkaecefheaXcufgXmbkkascxfagcdtfcualcx2alc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbg3BdbasahclfgHBd2a3aialavaOz:ejjjbh8PdndnaDmbcbhIcbh8Lxekcbh8LawhecbhXindnaeIdbJbbbb9ETmbasc;Wbfa8LcdtfaXBdba8Lcefh8LkaeclfheaDaXcefgX9hmbkascxfaHcdtfcua8Lal2gecdtaecFFFFi0Ecbyd1:jjjbHjjjjbbgIBdbasahcvfgHBd2alTmba8LTmbarcd4hEdnaOTmba8Lcdthzcbh8AaIhLinaoaOa8AcdtfydbaE2cdtfhYasc;WbfheaLhXa8LhQinaXaYaeydbcdtgKfIdbawaKfIdbNUdbaeclfheaXclfhXaQcufgQmbkaLazfhLa8Acefg8Aal9hmbxdkka8Lcdthzcbh8AaIhLinaoa8AaE2cdtfhYasc;WbfheaLhXa8LhQinaXaYaeydbcdtgKfIdbawaKfIdbNUdbaeclfheaXclfhXaQcufgQmbkaLazfhLa8Acefg8Aal9hmbkkascxfaHcdtfcualc8S2gealc;D;O;f8U0EgQcbyd1:jjjbHjjjjbbgXBdbasaHcefgKBd2aXcbaez:qjjjbhqdndndna8LTmbascxfaKcdtfaQcbyd1:jjjbHjjjjbbggBdbasaHcdfgXBd2agcbaez:qjjjb8AascxfaXcdtfcua8Lal2gecltgXaecFFFFb0Ecbyd1:jjjbHjjjjbbgrBdbasaHcifBd2arcbaXz:qjjjb8AadmexdkcbhgcbhradTmekcbhYabhXindna3aXclfydbg8Acx2fgeIdba3aXydbgLcx2fgQIdbg8R:tg8Sa3aXcwfydbgEcx2fgKIdlaQIdlgR:tg8UNaKIdba8R:tg8VaeIdlaR:tg8WN:tg8Xa8XNa8WaKIdwaQIdwg8Y:tg8ZNa8UaeIdwa8Y:tg8WN:tg8Ua8UNa8Wa8VNa8Za8SN:tg8Sa8SNMM:rg8VJbbbb9ETmba8Xa8V:vh8Xa8Sa8V:vh8Sa8Ua8V:vh8UkaqaAaLcdtfydbc8S2fgea8Ua8V:rg8Va8UNNg8WaeIdbMUdbaea8Sa8Va8SNg80Ng8ZaeIdlMUdlaea8Xa8Va8XNg81NgBaeIdwMUdwaea80a8UNg80aeIdxMUdxaea81a8UNg83aeIdzMUdzaea81a8SNg81aeIdCMUdCaea8Ua8Va8Xa8YNa8Ua8RNaRa8SNMM:mgRNg8RNg8UaeIdKMUdKaea8Sa8RNg8SaeId3MUd3aea8Xa8RNg8XaeIdaMUdaaea8RaRNg8RaeId8KMUd8Kaea8VaeIdyMUdyaqaAa8Acdtfydbc8S2fgea8WaeIdbMUdbaea8ZaeIdlMUdlaeaBaeIdwMUdwaea80aeIdxMUdxaea83aeIdzMUdzaea81aeIdCMUdCaea8UaeIdKMUdKaea8SaeId3MUd3aea8XaeIdaMUdaaea8RaeId8KMUd8Kaea8VaeIdyMUdyaqaAaEcdtfydbc8S2fgea8WaeIdbMUdbaea8ZaeIdlMUdlaeaBaeIdwMUdwaea80aeIdxMUdxaea83aeIdzMUdzaea81aeIdCMUdCaea8UaeIdKMUdKaea8SaeId3MUd3aea8XaeIdaMUdaaea8RaeId8KMUd8Kaea8VaeIdyMUdyaXcxfhXaYcifgYad6mbkcbhzabhLinabazcdtfh8AcbhXinaCa8AaXc;a1jjbfydbcdtfydbgQfRbbhedndnaCaLaXfydbgKfRbbgYc99fcFeGcpe0mbaec99fcFeGc;:e6mekdnaYcufcFeGce0mba8JaKcdtfydbaQ9hmekdnaecufcFeGce0mba8KaQcdtfydbaK9hmekdnaYcv2aefc:G1jjbfRbbTmbaAaQcdtfydbaAaKcdtfydb0mekJbbacJbbacJbbjZaecFeGceSEaYceSEh81dna3a8AaXc;e1jjbfydbcdtfydbcx2fgeIdwa3aKcx2fgYIdwgR:tg8Xa3aQcx2fgEIdwaR:tg8Ua8UNaEIdbaYIdbg8Y:tg8Sa8SNaEIdlaYIdlg8W:tg8Va8VNMMg8RNa8Xa8UNaeIdba8Y:tgBa8SNa8VaeIdla8W:tg80NMMg8Za8UN:tg8Xa8XNaBa8RNa8Za8SN:tg8Ua8UNa80a8RNa8Za8VN:tg8Sa8SNMM:rg8VJbbbb9ETmba8Xa8V:vh8Xa8Sa8V:vh8Sa8Ua8V:vh8UkaqaAaKcdtfydbc8S2fgea8Ua81a8R:rNg8Va8UNNg8ZaeIdbMUdbaea8Sa8Va8SNg81NgBaeIdlMUdlaea8Xa8Va8XNg8RNg80aeIdwMUdwaea81a8UNg81aeIdxMUdxaea8Ra8UNg83aeIdzMUdzaea8Ra8SNgUaeIdCMUdCaea8Ua8Va8XaRNa8Ua8YNa8Wa8SNMM:mgRNg8RNg8UaeIdKMUdKaea8Sa8RNg8SaeId3MUd3aea8Xa8RNg8XaeIdaMUdaaea8RaRNg8RaeId8KMUd8Kaea8VaeIdyMUdyaqaAaQcdtfydbc8S2fgea8ZaeIdbMUdbaeaBaeIdlMUdlaea80aeIdwMUdwaea81aeIdxMUdxaea83aeIdzMUdzaeaUaeIdCMUdCaea8UaeIdKMUdKaea8SaeId3MUd3aea8XaeIdaMUdaaea8RaeId8KMUd8Kaea8VaeIdyMUdykaXclfgXcx9hmbkaLcxfhLazcifgzad6mbka8LTmbcbhLinJbbbbh8Ya3abaLcdtfgeclfydbgEcx2fgXIdwa3aeydbgzcx2fgQIdwg80:tg8Sa8SNaXIdbaQIdbg83:tg8Xa8XNaXIdlaQIdlgU:tg8Va8VNMMg81a3aecwfydbgHcx2fgeIdwa80:tg8RNa8Sa8Sa8RNa8XaeIdba83:tgRNa8VaeIdlaU:tg8WNMMg8UN:tJbbbbJbbjZa81a8Ra8RNaRaRNa8Wa8WNMMgBNa8Ua8UN:tg8Z:va8ZJbbbb9BEg8ZNh85aBa8SNa8Ra8UN:ta8ZNh86a81a8WNa8Va8UN:ta8ZNh87aBa8VNa8Wa8UN:ta8ZNh88a81aRNa8Xa8UN:ta8ZNh89aBa8XNaRa8UN:ta8ZNh8:a8Xa8WNaRa8VN:tg8Ua8UNa8Va8RNa8Wa8SN:tg8Ua8UNa8SaRNa8Ra8XN:tg8Ua8UNMM:rJbbbZNh8UaIaza8L2gwcdtfhXaIaHa8L2g8NcdtfhQaIaEa8L2g5cdtfhKa80:mhZaU:mhna83:mhccbhYa8Lh8AJbbbbh8WJbbbbh8ZJbbbbh81JbbbbhBJbbbbh80Jbbbbh83JbbbbhUJbbbbh9cJbbbbhJinasc;WbfaYfgecwfa8Ua86aKIdbaXIdbg8R:tg8VNa85aQIdba8R:tgRNMg8SNUdbaeclfa8Ua88a8VNa87aRNMg8XNUdbaea8Ua8:a8VNa89aRNMg8VNUdbaecxfa8UaZa8SNana8XNa8Raca8VNMMMg8RNUdba8Ua8Sa8XNNaBMhBa8Ua8Sa8VNNa80Mh80a8Ua8Xa8VNNa83Mh83a8Ua8Ra8RNNa8YMh8Ya8Ua8Sa8RNNa8WMh8Wa8Ua8Xa8RNNa8ZMh8Za8Ua8Va8RNNa81Mh81a8Ua8Sa8SNNaUMhUa8Ua8Xa8XNNa9cMh9ca8Ua8Va8VNNaJMhJaXclfhXaKclfhKaQclfhQaYczfhYa8Acufg8Ambkagazc8S2fgeaJaeIdbMUdbaea9caeIdlMUdlaeaUaeIdwMUdwaea83aeIdxMUdxaea80aeIdzMUdzaeaBaeIdCMUdCaea81aeIdKMUdKaea8ZaeId3MUd3aea8WaeIdaMUdaaea8YaeId8KMUd8Kaea8UaeIdyMUdyagaEc8S2fgeaJaeIdbMUdbaea9caeIdlMUdlaeaUaeIdwMUdwaea83aeIdxMUdxaea80aeIdzMUdzaeaBaeIdCMUdCaea81aeIdKMUdKaea8ZaeId3MUd3aea8WaeIdaMUdaaea8YaeId8KMUd8Kaea8UaeIdyMUdyagaHc8S2fgeaJaeIdbMUdbaea9caeIdlMUdlaeaUaeIdwMUdwaea83aeIdxMUdxaea80aeIdzMUdzaeaBaeIdCMUdCaea81aeIdKMUdKaea8ZaeId3MUd3aea8WaeIdaMUdaaea8YaeId8KMUd8Kaea8UaeIdyMUdyarawcltfh8AcbhXa8LhKina8AaXfgeasc;WbfaXfgQIdbaeIdbMUdbaeclfgYaQclfIdbaYIdbMUdbaecwfgYaQcwfIdbaYIdbMUdbaecxfgeaQcxfIdbaeIdbMUdbaXczfhXaKcufgKmbkara5cltfh8AcbhXa8LhKina8AaXfgeasc;WbfaXfgQIdbaeIdbMUdbaeclfgYaQclfIdbaYIdbMUdbaecwfgYaQcwfIdbaYIdbMUdbaecxfgeaQcxfIdbaeIdbMUdbaXczfhXaKcufgKmbkara8Ncltfh8AcbhXa8LhKina8AaXfgeasc;WbfaXfgQIdbaeIdbMUdbaeclfgYaQclfIdbaYIdbMUdbaecwfgYaQcwfIdbaYIdbMUdbaecxfgeaQcxfIdbaeIdbMUdbaXczfhXaKcufgKmbkaLcifgLad6mbkkcbhQdndnamcwGg9embJbbbbh81cbhTcbhvcbhixekcbhTa8Fcbyd1:jjjbHjjjjbbhiascxfasyd2gecdtfaiBdbasaecefgXBd2ascxfaXcdtfcuaialabadaAz:fjjjbgKcltaKcjjjjiGEcbyd1:jjjbHjjjjbbgvBdbasaecdfBd2avaKaia3alz:gjjjbJFFuuh81aKTmbavheaKhXinaeIdbg8Ua81a81a8U9EEh81aeclfheaXcufgXmbkaKhTkasydlhSdnalTmbaSclfheaSydbhKaChXalhYcbhQincbaeydbg8AaK9RaXRbbcpeGEaQfhQaXcefhXaeclfhea8AhKaYcufgYmbkaQce4hQkcbhocuadaQ9Rcifg9hcx2a9hc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbhDascxfasyd2g9icdtfaDBdbasa9icefgeBd2ascxfaecdtfcua9hcdta9hcFFFFi0Ecbyd1:jjjbHjjjjbbg6Bdbasa9icdfgeBd2ascxfaecdtfa8Fcbyd1:jjjbHjjjjbbgyBdbasa9icifgeBd2ascxfaecdtfalcbyd1:jjjbHjjjjbbg9kBdbasa9iclfg8FBd2axaxNa8PJbbjZamclGEg85a85N:vhJJbbbbh9cdndndndnadak9nmba9hci6mea8Lclth0aDcwfh9mJbbbbhUJbbbbh9cinasclfabadalaAz:cjjjbabhzcbh8EcbhhinabahcdtfhHcbheindnaAazaefydbgQcdtgEfydbgYaAaHaec;q1jjbfydbcdtfydbgXcdtgwfydbg8ASmbaCaXfRbbgLcv2aCaQfRbbgKfc;G1jjbfRbbg5aKcv2aLfg8Nc;G1jjbfRbbg8MVcFeGTmbdna8AaY9nmba8Nc:G1jjbfRbbcFeGmekaKcufhYdnaKaL9hmbaYcFeGce0mba8JaEfydbaX9hmekdndnaKclSmbaLcl9hmekdnaYcFeGce0mba8JaEfydbaX9hmdkaLcufcFeGce0mba8KawfydbaQ9hmekaDa8Ecx2fgKaXaQa8McFeGgYEBdlaKaQaXaYEBdbaKaYa5Gcb9hBdwa8Ecefh8Ekaeclfgecx9hmbkdnahcifghad9pmbazcxfhza8Ecifa9h9nmekka8ETmecbhLinaqaAaDaLcx2fgKydbgYcdtgzfydbc8S2fgeIdwa3aKydlg8Acx2fgXIdwg8XNaeIdzaXIdbg8VNaeIdaMg8Ua8UMMa8XNaeIdlaXIdlg8RNaeIdCa8XNaeId3Mg8Ua8UMMa8RNaeIdba8VNaeIdxa8RNaeIdKMg8Ua8UMMa8VNaeId8KMMM:lh8UJbbbbJbbjZaeIdyg8S:va8SJbbbb9BEh8SdndnaKydwgEmbJFFuuh8WxekJbbbbJbbjZaqaAa8Acdtfydbc8S2fgeIdygR:vaRJbbbb9BEaeIdwa3aYcx2fgXIdwgRNaeIdzaXIdbg8YNaeIdaMg8Wa8WMMaRNaeIdlaXIdlg8WNaeIdCaRNaeId3MgRaRMMa8WNaeIdba8YNaeIdxa8WNaeIdKMgRaRMMa8YNaeId8KMMM:lNh8Wka8Sa8UNh8Zdna8LTmbagaYc8S2fgQIdwa8XNaQIdza8VNaQIdaMg8Ua8UMMa8XNaQIdla8RNaQIdCa8XNaQId3Mg8Ua8UMMa8RNaQIdba8VNaQIdxa8RNaQIdKMg8Ua8UMMa8VNaQId8KMMMh8UaIa8Aa8L2gHcdtfhXaraYa8L2gwcltfheaQIdyhRa8LhQinaXIdbg8Sa8SaRNaecxfIdba8XaecwfIdbNa8VaeIdbNa8RaeclfIdbNMMMg8Sa8SM:tNa8UMh8UaXclfhXaeczfheaQcufgQmbkdndnaEmbJbbbbh8Sxekaga8Ac8S2fgQIdwa3aYcx2fgeIdwg8VNaQIdzaeIdbg8RNaQIdaMg8Sa8SMMa8VNaQIdlaeIdlgRNaQIdCa8VNaQId3Mg8Sa8SMMaRNaQIdba8RNaQIdxaRNaQIdKMg8Sa8SMMa8RNaQId8KMMMh8SaIawcdtfhXaraHcltfheaQIdyh8Ya8LhQinaXIdbg8Xa8Xa8YNaecxfIdba8VaecwfIdbNa8RaeIdbNaRaeclfIdbNMMMg8Xa8XM:tNa8SMh8SaXclfhXaeczfheaQcufgQmbka8S:lh8Ska8Za8U:lMh8Za8Wa8SMh8WaCaYfRbbcd9hmbdna8Ka8Ja8Jazfydba8ASEaaazfydbgHcdtfydbgzcu9hmbaaa8AcdtfydbhzkagaHc8S2fgQIdwa3azcx2fgeIdwg8XNaQIdzaeIdbg8VNaQIdaMg8Ua8UMMa8XNaQIdlaeIdlg8RNaQIdCa8XNaQId3Mg8Ua8UMMa8RNaQIdba8VNaQIdxa8RNaQIdKMg8Ua8UMMa8VNaQId8KMMMh8UaIaza8L2gwcdtfhXaraHa8L2g8NcltfheaQIdyhRa8LhQinaXIdbg8Sa8SaRNaecxfIdba8XaecwfIdbNa8VaeIdbNa8RaeclfIdbNMMMg8Sa8SM:tNa8UMh8UaXclfhXaeczfheaQcufgQmbkdndnaEmbJbbbbh8Sxekagazc8S2fgQIdwa3aHcx2fgeIdwg8VNaQIdzaeIdbg8RNaQIdaMg8Sa8SMMa8VNaQIdlaeIdlgRNaQIdCa8VNaQId3Mg8Sa8SMMaRNaQIdba8RNaQIdxaRNaQIdKMg8Sa8SMMa8RNaQId8KMMMh8SaIa8NcdtfhXarawcltfheaQIdyh8Ya8LhQinaXIdbg8Xa8Xa8YNaecxfIdba8VaecwfIdbNa8RaeIdbNaRaeclfIdbNMMMg8Xa8XM:tNa8SMh8SaXclfhXaeczfheaQcufgQmbka8S:lh8Ska8Za8U:lMh8Za8Wa8SMh8WkaKa8Za8Wa8Za8W9FgeEUdwaKa8AaYaeaETVgeEBdlaKaYa8AaeEBdbaLcefgLa8E9hmbkasc;Wbfcbcj;qbz:qjjjb8Aa9mhea8EhXinasc;WbfaeydbcA4cF8FGgQcFAaQcFA6EcdtfgQaQydbcefBdbaecxfheaXcufgXmbkcbhecbhXinasc;WbfaefgQydbhKaQaXBdbaKaXfhXaeclfgecj;qb9hmbkcbhea9mhXinasc;WbfaXydbcA4cF8FGgQcFAaQcFA6EcdtfgQaQydbgQcefBdba6aQcdtfaeBdbaXcxfhXa8Eaecefge9hmbkadak9RgQci9Uh9ndnalTmbcbheayhXinaXaeBdbaXclfhXalaecefge9hmbkkcbh9oa9kcbalz:qjjjbhhaQcO9Uh9pa9nce4h9qasydwh9rcbh8Mcbh5dninaDa6a5cdtfydbcx2fg8NIdwg8UaJ9Emea8Ma9n9pmeJFFuuh8Sdna9qa8E9pmbaDa6a9qcdtfydbcx2fIdwJbb;aZNh8Skdna8Ua8S9ETmba8Ua9c9ETmba8Ma9p0mdkdnahaAa8NydlgHcdtg9sfydbgKfg9tRbbahaAa8Nydbgzcdtg9ufydbgefg9vRbbVmbaCazfRbbh9wdnaSaecdtfgXclfydbgQaXydbgXSmbaQaX9RhYa3aKcx2fhLa3aecx2fhEa9raXcitfhecbhXcehwdnindnayaeydbcdtfydbgQaKSmbayaeclfydbcdtfydbg8AaKSmbaQa8ASmba3a8Acx2fg8AIdba3aQcx2fgQIdbg8X:tg8UaEIdlaQIdlg8V:tg8YNaEIdba8X:tg8Wa8AIdla8V:tg8SN:tg8Ra8UaLIdla8V:tg8ZNaLIdba8X:tgBa8SN:tg8VNa8SaEIdwaQIdwgR:tg80Na8Ya8AIdwaR:tg8XN:tg8Ya8SaLIdwaR:tg83Na8Za8XN:tg8SNa8Xa8WNa80a8UN:tgRa8XaBNa83a8UN:tg8UNMMa8Ra8RNa8Ya8YNaRaRNMMa8Va8VNa8Sa8SNa8Ua8UNMMN:rJbbj8:N9FmdkaecwfheaXcefgXaY6hwaYaX9hmbkkawceGTmba9qcefh9qxekdndndndna9wc9:fPdebdkazheinayaecdtgefaHBdbaaaefydbgeaz9hmbxikkdna8Ka8Ja8Ja9ufydbaHSEaaa9ufydbgzcdtfydbgecu9hmbaaa9sfydbhekaya9ufaHBdbaehHkayazcdtfaHBdbka9vce86bba9tce86bba8NIdwg8Ua9ca9ca8U9DEh9ca9ocefh9ocecda9wceSEa8Mfh8Mka5cefg5a8E9hmbkka9oTmednalTmbcbh8AcbhEindnayaEcdtgefydbgQaESmbaAaQcdtfydbhzdnaEaAaefydb9hgHmbaqazc8S2fgeaqaEc8S2fgXIdbaeIdbMUdbaeaXIdlaeIdlMUdlaeaXIdwaeIdwMUdwaeaXIdxaeIdxMUdxaeaXIdzaeIdzMUdzaeaXIdCaeIdCMUdCaeaXIdKaeIdKMUdKaeaXId3aeId3MUd3aeaXIdaaeIdaMUdaaeaXId8KaeId8KMUd8KaeaXIdyaeIdyMUdyka8LTmbagaQc8S2fgeagaEc8S2gwfgXIdbaeIdbMUdbaeaXIdlaeIdlMUdlaeaXIdwaeIdwMUdwaeaXIdxaeIdxMUdxaeaXIdzaeIdzMUdzaeaXIdCaeIdCMUdCaeaXIdKaeIdKMUdKaeaXId3aeId3MUd3aeaXIdaaeIdaMUdaaeaXId8KaeId8KMUd8KaeaXIdyaeIdyMUdya0aQ2hLarhXa8LhKinaXaLfgeaXa8AfgQIdbaeIdbMUdbaeclfgYaQclfIdbaYIdbMUdbaecwfgYaQcwfIdbaYIdbMUdbaecxfgeaQcxfIdbaeIdbMUdbaXczfhXaKcufgKmbkaHmbJbbbbJbbjZaqawfgeIdyg8U:va8UJbbbb9BEaeIdwa3azcx2fgXIdwg8UNaeIdzaXIdbg8SNaeIdaMg8Xa8XMMa8UNaeIdlaXIdlg8XNaeIdCa8UNaeId3Mg8Ua8UMMa8XNaeIdba8SNaeIdxa8XNaeIdKMg8Ua8UMMa8SNaeId8KMMM:lNg8UaUaUa8U9DEhUka8Aa0fh8AaEcefgEal9hmbkcbhXa8JheindnaeydbgQcuSmbdnaXayaQcdtgKfydbgQ9hmbcuhQa8JaKfydbgKcuSmbayaKcdtfydbhQkaeaQBdbkaeclfhealaXcefgX9hmbkcbhXa8KheindnaeydbgQcuSmbdnaXayaQcdtgKfydbgQ9hmbcuhQa8KaKfydbgKcuSmbayaKcdtfydbhQkaeaQBdbkaeclfhealaXcefgX9hmbkkaUa9ca8LEhUcbhKabhecbhYindnayaeydbcdtfydbgXayaeclfydbcdtfydbgQSmbaXayaecwfydbcdtfydbg8ASmbaQa8ASmbabaKcdtfgLaXBdbaLcwfa8ABdbaLclfaQBdbaKcifhKkaecxfheaYcifgYad6mbkdndna9eTmbaKak9nmba81aU9FTmbcbhdabhecbhXindnavaiaeydbgQcdtfydbcdtfIdbaU9ETmbabadcdtfgYaQBdbaYclfaeclfydbBdbaYcwfaecwfydbBdbadcifhdkaecxfheaXcifgXaK6mbkJFFuuh81aTTmeavheaThXJFFuuh8UinaeIdbg8Sa8Ua8Ua8S9EEg8Xa8Ua8SaU9EgQEh8Ua8Xa81aQEh81aeclfheaXcufgXmbxdkkaKhdkadak0mbkkadTmdxekasclfabadalaAz:cjjjbJbbbbh9ckcbhKabhecbhXindnaAaeydbg8AcdtfydbgQaAaeclfydbgLcdtfydbgYSmbaQaAaecwfydbgzcdtfydbgESmbaYaESmbabaKcdtfgQa8ABdbaQcwfazBdbaQclfaLBdbaKcifhKkaecxfheaXcifgXad6mbkdndna9embaKhoxekdnaKak0mbaKhoxekdna81aJ9FmbaKhoxekina81Jbb;aZNg8UaJa8UaJ9DEh8XJbbbbh8UdnaTTmbavheaThAinaeIdbg8Sa8Ua8Sa8X9FEa8Ua8Sa8U9EEh8UaeclfheaAcufgAmbkkcbhoabhecbhAindnavaiaeydbgXcdtfydbcdtfIdba8X9ETmbabaocdtfgQaXBdbaQclfaeclfydbBdbaQcwfaecwfydbBdbaocifhokaecxfheaAcifgAaK6mbkJFFuuh81dnaTTmbavheaThAJFFuuh8SinaeIdbg8Va8Sa8Sa8V9EEg8Ra8Sa8Va8X9EgXEh8Sa8Ra81aXEh81aeclfheaAcufgAmbkkdnaoaK9hmbaKhoxdka8Ua9ca9ca8U9DEh9caoak9nmeaohKa81aJ9FmbkkdnamcjjjjlGTmbaOmbaoTmbcbhYabheinaCaeydbgQfRbbc3th8AaecwfgLydbhAdndna8JaQcdtgzfydbaeclfgEydbgXSmbcbhKa8KaXcdtfydbaQ9hmekcjjjj94hKkaea8AaKVaQVBdbaCaXfRbbc3th8Adndna8JaXcdtfydbaASmbcbhKa8KaAcdtfydbaX9hmekcjjjj94hKkaEa8AaKVaXVBdbaCaAfRbbc3thKdndna8JaAcdtfydbaQSmbcbhXa8KazfydbaA9hmekcjjjj94hXkaLaKaXVaAVBdbaecxfheaYcifgYao6mbkkaOTmbaoTmbaoheinabaOabydbcdtfydbBdbabclfhbaecufgembkkdnaPTmbaPa85a9c:rNUdbka9icdtascxffcxfhednina8FTmeaeydbcbyd:m:jjjbH:bjjjbbaec98fhea8Fcufh8Fxbkkasc;W;qbf8Kjjjjbaok;Yieouabydlhvabydbclfcbaicdtz:qjjjbhoadci9UhrdnadTmbdnalTmbaehwadhDinaoalawydbcdtfydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbxdkkaehwadhDinaoawydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbkkdnaiTmbcbhDaohwinawydbhqawaDBdbawclfhwaqaDfhDaicufgimbkkdnadci6mbinaecwfydbhwaeclfydbhDaeydbhidnalTmbalawcdtfydbhwalaDcdtfydbhDalaicdtfydbhikavaoaicdtfgqydbcitfaDBdbavaqydbcitfawBdlaqaqydbcefBdbavaoaDcdtfgqydbcitfawBdbavaqydbcitfaiBdlaqaqydbcefBdbavaoawcdtfgwydbcitfaiBdbavawydbcitfaDBdlawawydbcefBdbaecxfhearcufgrmbkkabydbcbBdbk:todDue99aicd4aifhrcehwinawgDcethwaDar6mbkcuaDcdtgraDcFFFFi0Ecbyd1:jjjbHjjjjbbhwaoaoyd9GgqcefBd9GaoaqcdtfawBdbawcFearz:qjjjbhkdnaiTmbalcd4hlaDcufhxcbhminamhDdnavTmbavamcdtfydbhDkcbadaDal2cdtfgDydlgwawcjjjj94SEgwcH4aw7c:F:b:DD2cbaDydbgwawcjjjj94SEgwcH4aw7c;D;O:B8J27cbaDydwgDaDcjjjj94SEgDcH4aD7c:3F;N8N27axGhwamcdthPdndndnavTmbakawcdtfgrydbgDcuSmeadavaPfydbal2cdtfgsIdbhzcehqinaqhrdnadavaDcdtfydbal2cdtfgqIdbaz9CmbaqIdlasIdl9CmbaqIdwasIdw9BmlkarcefhqakawarfaxGgwcdtfgrydbgDcu9hmbxdkkakawcdtfgrydbgDcuSmbadamal2cdtfgsIdbhzcehqinaqhrdnadaDal2cdtfgqIdbaz9CmbaqIdlasIdl9CmbaqIdwasIdw9BmikarcefhqakawarfaxGgwcdtfgrydbgDcu9hmbkkaramBdbamhDkabaPfaDBdbamcefgmai9hmbkkakcbyd:m:jjjbH:bjjjbbaoaoyd9GcufBd9GdnaeTmbaiTmbcbhDaehwinawaDBdbawclfhwaiaDcefgD9hmbkcbhDaehwindnaDabydbgrSmbawaearcdtfgrydbBdbaraDBdbkawclfhwabclfhbaiaDcefgD9hmbkkk;Qodvuv998Jjjjjbca9Rgvczfcwfcbyd11jjbBdbavcb8Pdj1jjb83izavcwfcbydN1jjbBdbavcb8Pd:m1jjb83ibdnadTmbaicd4hodnabmbdnalTmbcbhrinaealarcdtfydbao2cdtfhwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkarcefgrad9hmbxikkaocdthrcbhwincbhiinavczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbxdkkdnalTmbcbhrinabarcx2fgiaealarcdtfydbao2cdtfgwIdbUdbaiawIdlUdlaiawIdwUdwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkarcefgrad9hmbxdkkaocdthlcbhraehwinabarcx2fgiaearao2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawalfhwarcefgrad9hmbkkJbbbbavIdbavIdzgk:tgqaqJbbbb9DEgqavIdlavIdCgx:tgmamaq9DEgqavIdwavIdKgm:tgPaPaq9DEhPdnabTmbadTmbJbbbbJbbjZaP:vaPJbbbb9BEhqinabaqabIdbak:tNUdbabclfgvaqavIdbax:tNUdbabcwfgvaqavIdbam:tNUdbabcxfhbadcufgdmbkkaPk:ZlewudnaeTmbcbhvabhoinaoavBdbaoclfhoaeavcefgv9hmbkkdnaiTmbcbhrinadarcdtfhwcbhDinalawaDcdtgvc;a1jjbfydbcdtfydbcdtfydbhodnabalawavfydbcdtfydbgqcdtfgkydbgvaqSmbinakabavgqcdtfgxydbgvBdbaxhkaqav9hmbkkdnabaocdtfgkydbgvaoSmbinakabavgocdtfgxydbgvBdbaxhkaoav9hmbkkdnaqaoSmbabaqaoaqao0Ecdtfaqaoaqao6EBdbkaDcefgDci9hmbkarcifgrai6mbkkdnaembcbskcbhxindnalaxcdtgvfydbax9hmbaxhodnabavfgDydbgvaxSmbaDhqinaqabavgocdtfgkydbgvBdbakhqaoav9hmbkkaDaoBdbkaxcefgxae9hmbkcbhvabhocbhkindndnavalydbgq9hmbdnavaoydbgq9hmbaoakBdbakcefhkxdkaoabaqcdtfydbBdbxekaoabaqcdtfydbBdbkaoclfhoalclfhlaeavcefgv9hmbkakk;Jiilud99duabcbaecltz:qjjjbhvdnalTmbadhoaihralhwinarcwfIdbhDarclfIdbhqavaoydbcltfgkarIdbakIdbMUdbakclfgxaqaxIdbMUdbakcwfgxaDaxIdbMUdbakcxfgkakIdbJbbjZMUdbaoclfhoarcxfhrawcufgwmbkkdnaeTmbavhraehkinarcxfgoIdbhDaocbBdbararIdbJbbbbJbbjZaD:vaDJbbbb9BEgDNUdbarclfgoaDaoIdbNUdbarcwfgoaDaoIdbNUdbarczfhrakcufgkmbkkdnalTmbinavadydbcltfgrcxfgkaicwfIdbarcwfIdb:tgDaDNaiIdbarIdb:tgDaDNaiclfIdbarclfIdb:tgDaDNMMgDakIdbgqaqaD9DEUdbadclfhdaicxfhialcufglmbkkdnaeTmbavcxfhrinabarIdbUdbarczfhrabclfhbaecufgembkkk8MbabaeadaialavcbcbcbcbcbaoarawaDz:bjjjbk8MbabaeadaialavaoarawaDaqakaxamaPz:bjjjbk;J5oDue99iue99iuq998Jjjjjbc;Wb9RgD8KjjjjbcbhqaDcxfcbc;Kbz:qjjjb8AaDcualcx2alc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbgkBdxaDceBd2akaialavcbz:ejjjb8AaDcualcdtalcFFFFi0Egxcbyd1:jjjbHjjjjbbgiBdzaDcdBd2adci9Uhmaoco9UhPdndnJbbjZJ9VO:d86ararJ9VO:d869DE:vgr:lJbbb9p9DTmbar:Ohsxekcjjjj94hskdnascd9imbdnalTmbascuf:ZhrakhvaihzalhHindndnavIdbarNJbbbZMgO:lJbbb9p9DTmbaO:OhAxekcjjjj94hAkaAcCthAdndnavclfIdbarNJbbbZMgO:lJbbb9p9DTmbaO:OhCxekcjjjj94hCkaCcqtaAVhAdndnavcwfIdbarNJbbbZMgO:lJbbb9p9DTmbaO:OhCxekcjjjj94hCkazaAaCVBdbavcxfhvazclfhzaHcufgHmbkkadTmbcbhqaehvcbhzinaqaiavclfydbcdtfydbgHaiavcwfydbcdtfydbgA9haiavydbcdtfydbgCaH9haCaA9hGGfhqavcxfhvazcifgzad6mbkkaoci9UhXdndnaP:Z:rJbbbZMgr:lJbbb9p9DTmbar:Ohvxekcjjjj94hvkaX:ZhQcbhLc:bwhHdndninamhKaHhCashoaqgYaX9pmeaCao9Rcd9imeavaCcufavaC9iEaocefavao9kEhPdnalTmbaPcuf:YhrakhvaihzalhsindndnavIdbarNJbbbZMgO:lJbbb9p9DTmbaO:OhHxekcjjjj94hHkaHcCthHdndnavclfIdbarNJbbbZMgO:lJbbb9p9DTmbaO:Ohmxekcjjjj94hmkamcqtaHVhHdndnavcwfIdbarNJbbbZMgO:lJbbb9p9DTmbaO:Ohmxekcjjjj94hmkazaHamVBdbavcxfhvazclfhzascufgsmbkkcbhzdnadTmbaehvcbhsinazaiavclfydbcdtfydbgHaiavcwfydbcdtfydbgm9haiavydbcdtfydbgAaH9haAam9hGGfhzavcxfhvascifgsad6mbkkaKhmazhqaChHaPhsdnazaX9nmbazhmaYhqaPhHaohskdndnaLcl0mbdnaK:ZgraY:ZgO:taP:Yg8Aao:Y:tgEa8AaC:Y:tg3az:Zg5aQ:tNNNaraQ:ta3NaOa5:tNaOaQ:taENa5ar:tNM:va8AMJbbbZMgr:lJbbb9p9DTmbar:Ohvxdkcjjjj94hvxekasaHfcd9ThvkaLcefgLcs9hmbxdkkaYhqaohskdndndnaqmbJbbjZhrcbhicdhvawmexdkalcd4alfhHcehvinavgzcethvazaH6mbkcbhvaDcuazcdtgCazcFFFFi0Ecbyd1:jjjbHjjjjbbgPBdCaDciBd2aDaxcbyd1:jjjbHjjjjbbgoBdKaDclBd2dndnalTmbascuf:YhrakhvaihsalhHindndnavIdbarNJbbbZMgO:lJbbb9p9DTmbaO:Ohmxekcjjjj94hmkamcCthmdndnavclfIdbarNJbbbZMgO:lJbbb9p9DTmbaO:OhAxekcjjjj94hAkaAcqtamVhmdndnavcwfIdbarNJbbbZMgO:lJbbb9p9DTmbaO:OhAxekcjjjj94hAkasamaAVBdbavcxfhvasclfhsaHcufgHmbkaPcFeaCz:qjjjbhAazcufhCcbhPcbhLindndndnaAaiaLcdtgKfydbgHcm4aH7c:v;t;h;Ev2gvcs4av7aCGgscdtfgmydbgzcuSmbcehvinaiazcdtgzfydbaHSmdasavfhzavcefhvaAazaCGgscdtfgmydbgzcu9hmbkkamaLBdbaPhvaPcefhPxekaoazfydbhvkaoaKfavBdbaLcefgLal9hmbkcuaPc8S2gvaPc;D;O;f8U0EhixekaPcFeaCz:qjjjb8AcbhPcbhikcbhAaDaicbyd1:jjjbHjjjjbbgiBd3aDcvBd2aicbavz:qjjjbhzdnadTmbaehiinJbbnnJbbjZaoaiydbgHcdtfydbgvaoaiclfydbgscdtfydbgLSavaoaicwfydbgmcdtfydbgKSGgCEh8Ednakascx2fgsIdbakaHcx2fgHIdbg5:tgOakamcx2fgmIdlaHIdlgE:tgrNamIdba5:tg8AasIdlaE:tg8FN:tgQaQNa8FamIdwaHIdwg3:tgaNarasIdwa3:tg8FN:tgrarNa8Fa8ANaaaON:tgOaONMM:rg8AJbbbb9ETmbaQa8A:vhQaOa8A:vhOara8A:vhrkazavc8S2fgvavIdbara8Ea8A:rNg8AarNNg8FMUdbavaOa8AaONgaNghavIdlMUdlavaQa8AaQNg8ENggavIdwMUdwavaaarNgaavIdxMUdxava8EarNg8JavIdzMUdzava8EaONg8EavIdCMUdCavara8AaQa3Nara5NaEaONMM:mgENg5NgravIdKMUdKavaOa5NgOavId3MUd3avaQa5NgQavIdaMUdaava5aENg5avId8KMUd8Kava8AavIdyMUdydnaCmbazaLc8S2fgva8FavIdbMUdbavahavIdlMUdlavagavIdwMUdwavaaavIdxMUdxava8JavIdzMUdzava8EavIdCMUdCavaravIdKMUdKavaOavId3MUd3avaQavIdaMUdaava5avId8KMUd8Kava8AavIdyMUdyazaKc8S2fgva8FavIdbMUdbavahavIdlMUdlavagavIdwMUdwavaaavIdxMUdxava8JavIdzMUdzava8EavIdCMUdCavaravIdKMUdKavaOavId3MUd3avaQavIdaMUdaava5avId8KMUd8Kava8AavIdyMUdykaicxfhiaAcifgAad6mbkkcbhHaDcuaPcdtgvaPcFFFFi0Egicbyd1:jjjbHjjjjbbgsBdaaDcoBd2aDaicbyd1:jjjbHjjjjbbgiBd8KaDcrBd2ascFeavz:qjjjbhLdnalTmbaohsinJbbbbJbbjZazasydbgmc8S2fgvIdygr:varJbbbb9BEavIdwakcwfIdbgrNavIdzakIdbgONavIdaMgQaQMMarNavIdlakclfIdbgQNavIdCarNavId3MgrarMMaQNavIdbaONavIdxaQNavIdKMgrarMMaONavId8KMMM:lNhrdndnaLamcdtgvfgmydbcuSmbaiavfIdbar9ETmekamaHBdbaiavfarUdbkasclfhsakcxfhkalaHcefgH9hmbkkJbbbbhrdnaPTmbinaiIdbgOararaO9DEhraiclfhiaPcufgPmbkkaqcd4aqfhzcehiinaigvcethiavaz6mbkcbhiaDcuavcdtgzavcFFFFi0Ecbyd1:jjjbHjjjjbbgsBdyascFeazz:qjjjbhmdnadTmbavcufhAcbhlcbhqindnaoaeaqcdtfgvydbcdtfydbgiaoavclfydbcdtfydbgzSmbaiaoavcwfydbcdtfydbgvSmbazavSmbaLavcdtfydbhHdndnaLazcdtfydbgvaLaicdtfydbgi9pmbavaH9pmbaHhCaihkavhHxekdnaHai9pmbaHav9pmbaihCavhkxekavhCaHhkaihHkabalcx2fgvaHBdbavcwfakBdbavclfaCBdbdnamakc:3F;N8N2aCc:F:b:DD27aHc;D;O:B8J27aAGgzcdtfgvydbgicuSmbcehsinashvdnabaicx2fgiydbaH9hmbaiydlaC9hmbaiydwakSmikavcefhsamazavfaAGgzcdtfgvydbgicu9hmbkkavalBdbalcefhlkaqcifgqad6mbkalci2hikdnawmbcwhvxdkar:rhrcwhvkawarUdbkavcdthvdninavTmeavc98fgvaDcxffydbcbyd:m:jjjbH:bjjjbbxbkkaDc;Wbf8Kjjjjbaik::lewu8Jjjjjbc;Wb9Rgr8Kjjjjbcbhwarcxfcbc;Kbz:qjjjb8AdnabaeSmbabaeadcdtz:pjjjb8AkarcualcdtalcFFFFi0EgDcbyd1:jjjbHjjjjbbgqBdxarceBd2aqcbaialavcbarcxfz:djjjbcualcx2alc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbhkarcxfaryd2gxcdtgmfakBdbaraxcefgPBd2akaialavcbz:ejjjb8AarcxfaPcdtfaDcbyd1:jjjbHjjjjbbgiBdbaraxcdfgvBd2arcxfavcdtfcuaialaeadaqz:fjjjbgecltaecjjjjiGEcbyd1:jjjbHjjjjbbgqBdbaqaeaiakalz:gjjjbdnadTmbaoaoNhocbhwabhlcbhkindnaqaialydbgecdtfydbcdtfIdbao9ETmbabawcdtfgvaeBdbavclfalclfydbBdbavcwfalcwfydbBdbawcifhwkalcxfhlakcifgkad6mbkkaxcifhlamarcxffcwfhkdninalTmeakydbcbyd:m:jjjbH:bjjjbbakc98fhkalcufhlxbkkarc;Wbf8Kjjjjbawk:DCoDud99rue99iul998Jjjjjbc;Wb9Rgw8KjjjjbdndnarmbcbhDxekawcxfcbc;Kbz:qjjjb8Aawcuadcx2adc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbgqBdxawceBd2aqaeadaicbz:ejjjb8AawcuadcdtadcFFFFi0Egkcbyd1:jjjbHjjjjbbgxBdzawcdBd2adcd4adfhmceheinaegicetheaiam6mbkcbhPawcuaicdtgsaicFFFFi0Ecbyd1:jjjbHjjjjbbgzBdCawciBd2dndnar:ZgH:rJbbbZMgO:lJbbb9p9DTmbaO:Ohexekcjjjj94hekaicufhAc:bwhmcbhCadhXcbhQinaChLaeamgKcufaeaK9iEaPgDcefaeaD9kEhYdndnadTmbaYcuf:YhOaqhiaxheadhmindndnaiIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcCthCdndnaiclfIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhExekcjjjj94hEkaEcqtaCVhCdndnaicwfIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhExekcjjjj94hEkaeaCaEVBdbaicxfhiaeclfheamcufgmmbkazcFeasz:qjjjbh3cbh5cbhPindna3axaPcdtfydbgCcm4aC7c:v;t;h;Ev2gics4ai7aAGgmcdtfgEydbgecuSmbaeaCSmbcehiina3amaifaAGgmcdtfgEydbgecuSmeaicefhiaeaC9hmbkkaEaCBdba5aecuSfh5aPcefgPad9hmbxdkkazcFeasz:qjjjb8Acbh5kaDaYa5ar0giEhPaLa5aiEhCdna5arSmbaYaKaiEgmaP9Rcd9imbdndnaQcl0mbdnaX:ZgOaL:Zg8A:taY:Yg8EaD:Y:tg8Fa8EaK:Y:tgaa5:ZghaH:tNNNaOaH:taaNa8Aah:tNa8AaH:ta8FNahaO:tNM:va8EMJbbbZMgO:lJbbb9p9DTmbaO:Ohexdkcjjjj94hexekaPamfcd9Theka5aXaiEhXaQcefgQcs9hmekkdndnaCmbcihicbhDxekcbhiawakcbyd1:jjjbHjjjjbbg5BdKawclBd2aPcuf:Yh8AdndnadTmbaqhiaxheadhmindndnaiIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhCxekcjjjj94hCkaCcCthCdndnaiclfIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhExekcjjjj94hEkaEcqtaCVhCdndnaicwfIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhExekcjjjj94hEkaeaCaEVBdbaicxfhiaeclfheamcufgmmbkazcFeasz:qjjjbh3cbhDcbhYindndndna3axaYcdtgKfydbgCcm4aC7c:v;t;h;Ev2gics4ai7aAGgmcdtfgEydbgecuSmbcehiinaxaecdtgefydbaCSmdamaifheaicefhia3aeaAGgmcdtfgEydbgecu9hmbkkaEaYBdbaDhiaDcefhDxeka5aefydbhika5aKfaiBdbaYcefgYad9hmbkcuaDc32giaDc;j:KM;jb0EhexekazcFeasz:qjjjb8AcbhDcbhekawaecbyd1:jjjbHjjjjbbgeBd3awcvBd2aecbaiz:qjjjbhEavcd4hKdnadTmbdnalTmbaKcdth3a5hCaqhealhmadhAinaEaCydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiamIdbaiIdxMUdxaiamclfIdbaiIdzMUdzaiamcwfIdbaiIdCMUdCaiaiIdKJbbjZMUdKaCclfhCaecxfheama3fhmaAcufgAmbxdkka5hmaqheadhCinaEamydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiaiIdxJbbbbMUdxaiaiIdzJbbbbMUdzaiaiIdCJbbbbMUdCaiaiIdKJbbjZMUdKamclfhmaecxfheaCcufgCmbkkdnaDTmbaEhiaDheinaiaiIdbJbbbbJbbjZaicKfIdbgO:vaOJbbbb9BEgONUdbaiclfgmaOamIdbNUdbaicwfgmaOamIdbNUdbaicxfgmaOamIdbNUdbaiczfgmaOamIdbNUdbaicCfgmaOamIdbNUdbaic3fhiaecufgembkkcbhCawcuaDcdtgYaDcFFFFi0Egicbyd1:jjjbHjjjjbbgeBdaawcoBd2awaicbyd1:jjjbHjjjjbbg3Bd8KaecFeaYz:qjjjbhxdnadTmbJbbjZJbbjZa8A:vaPceSEaoNgOaONh8AaKcdthPalheina8Aaec;81jjbalEgmIdwaEa5ydbgAc32fgiIdC:tgOaONamIdbaiIdx:tgOaONamIdlaiIdz:tgOaONMMNaqcwfIdbaiIdw:tgOaONaqIdbaiIdb:tgOaONaqclfIdbaiIdl:tgOaONMMMhOdndnaxaAcdtgifgmydbcuSmba3aifIdbaO9ETmekamaCBdba3aifaOUdbka5clfh5aqcxfhqaeaPfheadaCcefgC9hmbkkabaxaYz:pjjjb8AcrhikaicdthiinaiTmeaic98fgiawcxffydbcbyd:m:jjjbH:bjjjbbxbkkawc;Wbf8KjjjjbaDk:Ydidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgvaDIdbgoaoav9EEUdbaladfgDavaDIdbgoaoav9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkJbbbbavavJbbbb9DEgvaoaoav9DEgvararav9DEk9DeeuabcFeaicdtz:qjjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcifc98GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;teeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiaeydlBdlaiaeydwBdwaiaeydxBdxaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk:3eedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdxaialBdwaialBdlaialBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcrfc94GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd:q:jjjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd:q:jjjbfgdBd:q:jjjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akkk:Iedbcjwk1eFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbeeebeebebbeeebebbbbbebebbbbbbbbbebbbdbbbbbbbebbbebbbdbbbbbbbbbbbeeeeebebbebbebebbbeebbbbbbbbbbbbbbbbbbbbbc1Dkxebbbdbbb:GNbb'; // embed! wasm

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

	function reorder(fun, indices, vertices) {
		var sbrk = instance.exports.sbrk;
		var ip = sbrk(indices.length * 4);
		var rp = sbrk(vertices * 4);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		var indices8 = bytes(indices);
		heap.set(indices8, ip);
		var unique = fun(rp, ip, indices.length, vertices);
		// heap may have grown
		heap = new Uint8Array(instance.exports.memory.buffer);
		var remap = new Uint32Array(vertices);
		new Uint8Array(remap.buffer).set(heap.subarray(rp, rp + vertices * 4));
		indices8.set(heap.subarray(ip, ip + indices.length * 4));
		sbrk(ip - sbrk(0));

		for (var i = 0; i < indices.length; ++i) indices[i] = remap[indices[i]];

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

	function simplifyAttr(
		fun,
		indices,
		index_count,
		vertex_positions,
		vertex_count,
		vertex_positions_stride,
		vertex_attributes,
		vertex_attributes_stride,
		attribute_weights,
		vertex_lock,
		target_index_count,
		target_error,
		options
	) {
		var sbrk = instance.exports.sbrk;
		var te = sbrk(4);
		var ti = sbrk(index_count * 4);
		var sp = sbrk(vertex_count * vertex_positions_stride);
		var sa = sbrk(vertex_count * vertex_attributes_stride);
		var sw = sbrk(attribute_weights.length * 4);
		var si = sbrk(index_count * 4);
		var vl = vertex_lock ? sbrk(vertex_count) : 0;
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(vertex_positions), sp);
		heap.set(bytes(vertex_attributes), sa);
		heap.set(bytes(attribute_weights), sw);
		heap.set(bytes(indices), si);
		if (vertex_lock) {
			heap.set(bytes(vertex_lock), vl);
		}
		var result = fun(
			ti,
			si,
			index_count,
			sp,
			vertex_count,
			vertex_positions_stride,
			sa,
			vertex_attributes_stride,
			sw,
			attribute_weights.length,
			vl,
			target_index_count,
			target_error,
			options,
			te
		);
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

	function simplifyPoints(
		fun,
		vertex_positions,
		vertex_count,
		vertex_positions_stride,
		vertex_colors,
		vertex_colors_stride,
		color_weight,
		target_vertex_count
	) {
		var sbrk = instance.exports.sbrk;
		var ti = sbrk(target_vertex_count * 4);
		var sp = sbrk(vertex_count * vertex_positions_stride);
		var sc = sbrk(vertex_count * vertex_colors_stride);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(vertex_positions), sp);
		if (vertex_colors) {
			heap.set(bytes(vertex_colors), sc);
		}
		var result = fun(ti, sp, vertex_count, vertex_positions_stride, sc, vertex_colors_stride, color_weight, target_vertex_count);
		// heap may have grown
		heap = new Uint8Array(instance.exports.memory.buffer);
		var target = new Uint32Array(result);
		bytes(target).set(heap.subarray(ti, ti + result * 4));
		sbrk(ti - sbrk(0));
		return target;
	}

	function simplifySloppy(fun, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride, target_index_count, target_error) {
		var sbrk = instance.exports.sbrk;
		var te = sbrk(4);
		var ti = sbrk(index_count * 4);
		var sp = sbrk(vertex_count * vertex_positions_stride);
		var si = sbrk(index_count * 4);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(vertex_positions), sp);
		heap.set(bytes(indices), si);
		var result = fun(ti, si, index_count, sp, vertex_count, vertex_positions_stride, target_index_count, target_error, te);
		// heap may have grown
		heap = new Uint8Array(instance.exports.memory.buffer);
		var target = new Uint32Array(result);
		bytes(target).set(heap.subarray(ti, ti + result * 4));
		var error = new Float32Array(1);
		bytes(error).set(heap.subarray(te, te + 4));
		sbrk(te - sbrk(0));
		return [target, error[0]];
	}

	function simplifyPrune(fun, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride, target_error) {
		var sbrk = instance.exports.sbrk;
		var ti = sbrk(index_count * 4);
		var sp = sbrk(vertex_count * vertex_positions_stride);
		var si = sbrk(index_count * 4);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(vertex_positions), sp);
		heap.set(bytes(indices), si);
		var result = fun(ti, si, index_count, sp, vertex_count, vertex_positions_stride, target_error);
		// heap may have grown
		heap = new Uint8Array(instance.exports.memory.buffer);
		var target = new Uint32Array(result);
		bytes(target).set(heap.subarray(ti, ti + result * 4));
		sbrk(ti - sbrk(0));
		return target;
	}

	var simplifyOptions = {
		LockBorder: 1,
		Sparse: 2,
		ErrorAbsolute: 4,
		Prune: 8,
		_InternalDebug: 1 << 30, // internal, don't use!
	};

	return {
		ready: ready,
		supported: true,

		compactMesh: function (indices) {
			assert(
				indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array
			);
			assert(indices.length % 3 == 0);

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			return reorder(instance.exports.meshopt_optimizeVertexFetchRemap, indices32, maxindex(indices) + 1);
		},

		simplify: function (indices, vertex_positions, vertex_positions_stride, target_index_count, target_error, flags) {
			assert(
				indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array
			);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(target_index_count >= 0 && target_index_count <= indices.length);
			assert(target_index_count % 3 == 0);
			assert(target_error >= 0);

			var options = 0;
			for (var i = 0; i < (flags ? flags.length : 0); ++i) {
				assert(flags[i] in simplifyOptions);
				options |= simplifyOptions[flags[i]];
			}

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			var result = simplify(
				instance.exports.meshopt_simplify,
				indices32,
				indices.length,
				vertex_positions,
				vertex_positions.length / vertex_positions_stride,
				vertex_positions_stride * 4,
				target_index_count,
				target_error,
				options
			);
			result[0] = indices instanceof Uint32Array ? result[0] : new indices.constructor(result[0]);

			return result;
		},

		simplifyWithAttributes: function (
			indices,
			vertex_positions,
			vertex_positions_stride,
			vertex_attributes,
			vertex_attributes_stride,
			attribute_weights,
			vertex_lock,
			target_index_count,
			target_error,
			flags
		) {
			assert(
				indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array
			);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(vertex_attributes instanceof Float32Array);
			assert(vertex_attributes.length % vertex_attributes_stride == 0);
			assert(vertex_attributes_stride >= 0);
			assert(vertex_lock == null || vertex_lock instanceof Uint8Array);
			assert(vertex_lock == null || vertex_lock.length == vertex_positions.length / vertex_positions_stride);
			assert(target_index_count >= 0 && target_index_count <= indices.length);
			assert(target_index_count % 3 == 0);
			assert(target_error >= 0);
			assert(Array.isArray(attribute_weights));
			assert(vertex_attributes_stride >= attribute_weights.length);
			assert(attribute_weights.length <= 32);
			for (var i = 0; i < attribute_weights.length; ++i) {
				assert(attribute_weights[i] >= 0);
			}

			var options = 0;
			for (var i = 0; i < (flags ? flags.length : 0); ++i) {
				assert(flags[i] in simplifyOptions);
				options |= simplifyOptions[flags[i]];
			}

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			var result = simplifyAttr(
				instance.exports.meshopt_simplifyWithAttributes,
				indices32,
				indices.length,
				vertex_positions,
				vertex_positions.length / vertex_positions_stride,
				vertex_positions_stride * 4,
				vertex_attributes,
				vertex_attributes_stride * 4,
				new Float32Array(attribute_weights),
				vertex_lock ? new Uint8Array(vertex_lock) : null,
				target_index_count,
				target_error,
				options
			);
			result[0] = indices instanceof Uint32Array ? result[0] : new indices.constructor(result[0]);

			return result;
		},

		getScale: function (vertex_positions, vertex_positions_stride) {
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			return simplifyScale(
				instance.exports.meshopt_simplifyScale,
				vertex_positions,
				vertex_positions.length / vertex_positions_stride,
				vertex_positions_stride * 4
			);
		},

		simplifyPoints: function (vertex_positions, vertex_positions_stride, target_vertex_count, vertex_colors, vertex_colors_stride, color_weight) {
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(target_vertex_count >= 0 && target_vertex_count <= vertex_positions.length / vertex_positions_stride);
			if (vertex_colors) {
				assert(vertex_colors instanceof Float32Array);
				assert(vertex_colors.length % vertex_colors_stride == 0);
				assert(vertex_colors_stride >= 3);
				assert(vertex_positions.length / vertex_positions_stride == vertex_colors.length / vertex_colors_stride);
				return simplifyPoints(
					instance.exports.meshopt_simplifyPoints,
					vertex_positions,
					vertex_positions.length / vertex_positions_stride,
					vertex_positions_stride * 4,
					vertex_colors,
					vertex_colors_stride * 4,
					color_weight,
					target_vertex_count
				);
			} else {
				return simplifyPoints(
					instance.exports.meshopt_simplifyPoints,
					vertex_positions,
					vertex_positions.length / vertex_positions_stride,
					vertex_positions_stride * 4,
					undefined,
					0,
					0,
					target_vertex_count
				);
			}
		},

		simplifySloppy: function (indices, vertex_positions, vertex_positions_stride, target_index_count, target_error) {
			assert(
				indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array
			);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(target_index_count >= 0 && target_index_count <= indices.length);
			assert(target_index_count % 3 == 0);
			assert(target_error >= 0);

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			var result = simplifySloppy(
				instance.exports.meshopt_simplifySloppy,
				indices32,
				indices.length,
				vertex_positions,
				vertex_positions.length / vertex_positions_stride,
				vertex_positions_stride * 4,
				target_index_count,
				target_error
			);
			result[0] = indices instanceof Uint32Array ? result[0] : new indices.constructor(result[0]);

			return result;
		},

		simplifyPrune: function (indices, vertex_positions, vertex_positions_stride, target_error) {
			assert(
				indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array
			);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(target_error >= 0);

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			var result = simplifyPrune(
				instance.exports.meshopt_simplifyPrune,
				indices32,
				indices.length,
				vertex_positions,
				vertex_positions.length / vertex_positions_stride,
				vertex_positions_stride * 4,
				target_error
			);
			result = indices instanceof Uint32Array ? result : new indices.constructor(result);

			return result;
		},
	};
})();

// export! MeshoptSimplifier
if (typeof exports === 'object' && typeof module === 'object') module.exports = MeshoptSimplifier;
else if (typeof define === 'function' && define['amd'])
	define([], function () {
		return MeshoptSimplifier;
	});
else if (typeof exports === 'object') exports['MeshoptSimplifier'] = MeshoptSimplifier;
else (typeof self !== 'undefined' ? self : this).MeshoptSimplifier = MeshoptSimplifier;
