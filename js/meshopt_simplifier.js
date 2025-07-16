// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function () {
	'use strict';
	// Built with clang version 19.1.5-wasi-sdk
	// Built from meshoptimizer 0.24
	var wasm =
		'b9H79Tebbbe:nes9Geueu9Geub9Gbb9Gsuuuuuuuuuuuu99uueu9Gvuuuuub9Gruuuuuuub9Gvuuuuue999Gvuuuuueu9Gquuuuuuu99uueu9Gquuuuuuuu99ueu9Gruuuuuu99eu9Gwuuuuuu99ueu9Giuuue999Gluuuueu9GiuuueuiCAdilvorlwiDqkxmbPPbelve9Weiiviebeoweuec:G:Pdkr;Eeqo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95br8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bwQ9TW79O9V9Wt9F79P9T9W29P9M959t29V9W9W95bDX9TW79O9V9Wt9F79P9T9W29P9M959qV919UWbqQ9TW79O9V9Wt9F79P9T9W29P9M959q9V9P9Ut7bkX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbxa9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbml79IV9RbPDwebcekdHOq:I;deAdbk:R9KoEue99euY99Oue9:8Jjjjjbc;W;qb9Rgs8Kjjjjbcbhzascxfcbc;Kbz:qjjjb8AdnabaeSmbabaeadcdtz:pjjjb8AkdndnamcdGmbascxfhHcbhOxekasalcrfci4gecbyd1:jjjbHjjjjbbgABdxasceBd2aAcbaez:qjjjbhCcbhlcbhednadTmbcbhlabheadhAinaCaeydbgXci4fgQaQRbbgQceaXcrGgXtV86bbaQcu7aX4ceGalfhlaeclfheaAcufgAmbkcualcdtalcFFFFi0EhekascCfhHasaecbyd1:jjjbHjjjjbbgOBdzascdBd2alcd4alfhXcehAinaAgecethAaeaX6mbkcdhzcbhLascuaecdtgAaecFFFFi0Ecbyd1:jjjbHjjjjbbgXBdCasciBd2aXcFeaAz:qjjjbhKdnadTmbaecufhYcbh8AindndnaKabaLcdtfgEydbgQc:v;t;h;Ev2aYGgXcdtfgCydbgAcuSmbceheinaOaAcdtfydbaQSmdaXaefhAaecefheaKaAaYGgXcdtfgCydbgAcu9hmbkkaOa8AcdtfaQBdbaCa8ABdba8AhAa8Acefh8AkaEaABdbaLcefgLad9hmbkkaKcbyd:m:jjjbH:bjjjbbascdBd2kcbh3aHcualcefgecdtaecFFFFi0Ecbyd1:jjjbHjjjjbbg5Bdbasa5BdlasazceVgeBd2ascxfaecdtfcuadcitadcFFFFe0Ecbyd1:jjjbHjjjjbbg8EBdbasa8EBdwasazcdfgeBd2asclfabadalcbz:cjjjbascxfaecdtfcualcdtgealcFFFFi0Eg8Fcbyd1:jjjbHjjjjbbgABdbasazcifgXBd2ascxfaXcdtfa8Fcbyd1:jjjbHjjjjbbgaBdbasazclVBd2aAaaaialavaOascxfz:djjjbalcbyd1:jjjbHjjjjbbhCascxfasyd2ghcdtfaCBdbasahcefgXBd2ascxfaXcdtfa8Fcbyd1:jjjbHjjjjbbgXBdbasahcdfgQBd2ascxfaQcdtfa8Fcbyd1:jjjbHjjjjbbgQBdbasahcifggBd2aXcFeaez:qjjjbh8JaQcFeaez:qjjjbh8KdnalTmba8Ecwfh8Lindna5a3gQcefg3cdtfydbgKa5aQcdtgefydbgXSmbaKaX9Rhza8EaXcitfhHa8Kaefh8Ma8JaefhEcbhYindndnaHaYcitfydbg8AaQ9hmbaEaQBdba8MaQBdbxekdna5a8Acdtg8NfgeclfydbgXaeydbgeSmba8EaecitgKfydbaQSmeaXae9Rhyaecu7aXfhLa8LaKfhXcbheinaLaeSmeaecefheaXydbhKaXcwfhXaKaQ9hmbkaeay6meka8Ka8NfgeaQa8AaeydbcuSEBdbaEa8AaQaEydbcuSEBdbkaYcefgYaz9hmbkka3al9hmbkaAhXaahQa8KhKa8JhYcbheindndnaeaXydbg8A9hmbdnaeaQydbg8A9hmbaYydbh8AdnaKydbgLcu9hmba8Acu9hmbaCaefcb86bbxikaCaefhEdnaeaLSmbaea8ASmbaEce86bbxikaEcl86bbxdkdnaeaaa8AcdtgLfydb9hmbdnaKydbgEcuSmbaeaESmbaYydbgzcuSmbaeazSmba8KaLfydbgHcuSmbaHa8ASmba8JaLfydbgLcuSmbaLa8ASmbdnaAaEcdtfydbg8AaAaLcdtfydb9hmba8AaAazcdtfydbgLSmbaLaAaHcdtfydb9hmbaCaefcd86bbxlkaCaefcl86bbxikaCaefcl86bbxdkaCaefcl86bbxekaCaefaCa8AfRbb86bbkaXclfhXaQclfhQaKclfhKaYclfhYalaecefge9hmbkdnaqTmbdndnaOTmbaOheaAhXalhQindnaqaeydbfRbbTmbaCaXydbfcl86bbkaeclfheaXclfhXaQcufgQmbxdkkaAhealhXindnaqRbbTmbaCaeydbfcl86bbkaqcefhqaeclfheaXcufgXmbkkaAhealhQaChXindnaCaeydbfRbbcl9hmbaXcl86bbkaeclfheaXcefhXaQcufgQmbkkamceGTmbaChealhXindnaeRbbce9hmbaecl86bbkaecefheaXcufgXmbkkcbh8Pascxfagcdtfcualcx2alc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbg3BdbasahclfgHBd2a3aialavaOz:ejjjbhIdndnaDmbcbh8Rcbh8Lxekcbh8LawhecbhXindnaeIdbJbbbb9ETmbasc;Wbfa8LcdtfaXBdba8Lcefh8LkaeclfheaDaXcefgX9hmbkascxfaHcdtfcua8Lal2gecdtaecFFFFi0Ecbyd1:jjjbHjjjjbbg8RBdbasahcvfgHBd2alTmba8LTmbarcd4hEdnaOTmba8Lcdthzcbh8Aa8RhLinaoaOa8AcdtfydbaE2cdtfhYasc;WbfheaLhXa8LhQinaXaYaeydbcdtgKfIdbawaKfIdbNUdbaeclfheaXclfhXaQcufgQmbkaLazfhLa8Acefg8Aal9hmbxdkka8Lcdthzcbh8Aa8RhLinaoa8AaE2cdtfhYasc;WbfheaLhXa8LhQinaXaYaeydbcdtgKfIdbawaKfIdbNUdbaeclfheaXclfhXaQcufgQmbkaLazfhLa8Acefg8Aal9hmbkkascxfaHcdtfcualc8S2gealc;D;O;f8U0EgQcbyd1:jjjbHjjjjbbgXBdbasaHcefgKBd2aXcbaez:qjjjbhqcbhgdna8LTmbascxfaKcdtfaQcbyd1:jjjbHjjjjbbggBdbasaHcdfgXBd2agcbaez:qjjjb8AascxfaXcdtfcua8Lal2gecltgXaecFFFFb0Ecbyd1:jjjbHjjjjbbg8PBdbasaHcifBd2a8PcbaXz:qjjjb8AkdnadTmbcbhYabhXindna3aXclfydbg8Acx2fgeIdba3aXydbgLcx2fgQIdbg8S:tgRa3aXcwfydbgEcx2fgKIdlaQIdlg8U:tg8VNaKIdba8S:tg8WaeIdla8U:tg8XN:tg8Ya8YNa8XaKIdwaQIdwg8Z:tg80Na8VaeIdwa8Z:tg8XN:tg8Va8VNa8Xa8WNa80aRN:tgRaRNMM:rg8WJbbbb9ETmba8Ya8W:vh8YaRa8W:vhRa8Va8W:vh8VkaqaAaLcdtfydbc8S2fgea8Va8W:rg8Wa8VNNg8XaeIdbMUdbaeaRa8WaRNg81Ng80aeIdlMUdlaea8Ya8Wa8YNgBNg83aeIdwMUdwaea81a8VNg81aeIdxMUdxaeaBa8VNgUaeIdzMUdzaeaBaRNgBaeIdCMUdCaea8Va8Wa8Ya8ZNa8Va8SNa8UaRNMM:mg8UNg8SNg8VaeIdKMUdKaeaRa8SNgRaeId3MUd3aea8Ya8SNg8YaeIdaMUdaaea8Sa8UNg8SaeId8KMUd8Kaea8WaeIdyMUdyaqaAa8Acdtfydbc8S2fgea8XaeIdbMUdbaea80aeIdlMUdlaea83aeIdwMUdwaea81aeIdxMUdxaeaUaeIdzMUdzaeaBaeIdCMUdCaea8VaeIdKMUdKaeaRaeId3MUd3aea8YaeIdaMUdaaea8SaeId8KMUd8Kaea8WaeIdyMUdyaqaAaEcdtfydbc8S2fgea8XaeIdbMUdbaea80aeIdlMUdlaea83aeIdwMUdwaea81aeIdxMUdxaeaUaeIdzMUdzaeaBaeIdCMUdCaea8VaeIdKMUdKaeaRaeId3MUd3aea8YaeIdaMUdaaea8SaeId8KMUd8Kaea8WaeIdyMUdyaXcxfhXaYcifgYad6mbkkdnalTmbJ;n;m;m89J:v:;;w8ZamczGEh8ZcbhQaAhKa3hXaqheindnaQaKydb9hmbaecxfgYaYIdbJbbbbMUdbaeczfgYaYIdbJbbbbMUdbaecCfgYaYIdbJbbbbMUdbaea8ZaecyfgYIdbg8SNg8VaeIdbMUdbaeclfg8Aa8Va8AIdbMUdbaecwfg8Aa8Va8AIdbMUdbaecKfg8Aa8AIdbaXIdbg8Ya8VN:tUdbaXcwfIdbhRaec3fg8Aa8AIdba8VaXclfIdbg8WN:tUdbaecafg8Aa8AIdba8VaRN:tUdbaec8Kfg8AIdbh8UaYa8Sa8VMUdba8Aa8Ua8VaRaRNa8Ya8YNa8Wa8WNMMNMUdbkaKclfhKaXcxfhXaec8SfhealaQcefgQ9hmbkkdnadTmbcbhzabhLinabazcdtfh8AcbhXinaCa8AaXc;a1jjbfydbcdtfydbgQfRbbhedndnaCaLaXfydbgKfRbbgYc99fcFeGcpe0mbaec99fcFeGc;:e6mekdnaYcufcFeGce0mba8JaKcdtfydbaQ9hmekdnaecufcFeGce0mba8KaQcdtfydbaK9hmekdnaYcv2aefc:G1jjbfRbbTmbaAaQcdtfydbaAaKcdtfydb0mekJbbacJbbacJbbjZaecFeGceSEaYceSEhBdna3a8AaXc;e1jjbfydbcdtfydbcx2fgeIdwa3aKcx2fgYIdwg8U:tg8Ya3aQcx2fgEIdwa8U:tg8Va8VNaEIdbaYIdbg8Z:tgRaRNaEIdlaYIdlg8X:tg8Wa8WNMMg8SNa8Ya8VNaeIdba8Z:tg83aRNa8WaeIdla8X:tg81NMMg80a8VN:tg8Ya8YNa83a8SNa80aRN:tg8Va8VNa81a8SNa80a8WN:tgRaRNMM:rg8WJbbbb9ETmba8Ya8W:vh8YaRa8W:vhRa8Va8W:vh8VkaqaAaKcdtfydbc8S2fgea8VaBa8S:rNg8Wa8VNNg80aeIdbMUdbaeaRa8WaRNgBNg83aeIdlMUdlaea8Ya8Wa8YNg8SNg81aeIdwMUdwaeaBa8VNgBaeIdxMUdxaea8Sa8VNgUaeIdzMUdzaea8SaRNg85aeIdCMUdCaea8Va8Wa8Ya8UNa8Va8ZNa8XaRNMM:mg8UNg8SNg8VaeIdKMUdKaeaRa8SNgRaeId3MUd3aea8Ya8SNg8YaeIdaMUdaaea8Sa8UNg8SaeId8KMUd8Kaea8WaeIdyMUdyaqaAaQcdtfydbc8S2fgea80aeIdbMUdbaea83aeIdlMUdlaea81aeIdwMUdwaeaBaeIdxMUdxaeaUaeIdzMUdzaea85aeIdCMUdCaea8VaeIdKMUdKaeaRaeId3MUd3aea8YaeIdaMUdaaea8SaeId8KMUd8Kaea8WaeIdyMUdykaXclfgXcx9hmbkaLcxfhLazcifgzad6mbka8LTmbcbhLinJbbbbh8Za3abaLcdtfgeclfydbgEcx2fgXIdwa3aeydbgzcx2fgQIdwg81:tgRaRNaXIdbaQIdbgU:tg8Ya8YNaXIdlaQIdlg85:tg8Wa8WNMMgBa3aecwfydbgHcx2fgeIdwa81:tg8SNaRaRa8SNa8YaeIdbaU:tg8UNa8WaeIdla85:tg8XNMMg8VN:tJbbbbJbbjZaBa8Sa8SNa8Ua8UNa8Xa8XNMMg83Na8Va8VN:tg80:va80Jbbbb9BEg80Nh86a83aRNa8Sa8VN:ta80Nh87aBa8XNa8Wa8VN:ta80Nh88a83a8WNa8Xa8VN:ta80Nh89aBa8UNa8Ya8VN:ta80Nh8:a83a8YNa8Ua8VN:ta80NhZa8Ya8XNa8Ua8WN:tg8Va8VNa8Wa8SNa8XaRN:tg8Va8VNaRa8UNa8Sa8YN:tg8Va8VNMM:rJbbbZNh8Va8Raza8L2gwcdtfhXa8RaHa8L2g8NcdtfhQa8RaEa8L2g5cdtfhKa81:mhna85:mhcaU:mh9ccbhYa8Lh8AJbbbbh8XJbbbbh80JbbbbhBJbbbbh83Jbbbbh81JbbbbhUJbbbbh85JbbbbhJJbbbbh9einasc;WbfaYfgecwfa8Va87aKIdbaXIdbg8S:tg8WNa86aQIdba8S:tg8UNMgRNUdbaeclfa8Va89a8WNa88a8UNMg8YNUdbaea8VaZa8WNa8:a8UNMg8WNUdbaecxfa8VanaRNaca8YNa8Sa9ca8WNMMMg8SNUdba8VaRa8YNNa83Mh83a8VaRa8WNNa81Mh81a8Va8Ya8WNNaUMhUa8Va8Sa8SNNa8ZMh8Za8VaRa8SNNa8XMh8Xa8Va8Ya8SNNa80Mh80a8Va8Wa8SNNaBMhBa8VaRaRNNa85Mh85a8Va8Ya8YNNaJMhJa8Va8Wa8WNNa9eMh9eaXclfhXaKclfhKaQclfhQaYczfhYa8Acufg8Ambkagazc8S2fgea9eaeIdbMUdbaeaJaeIdlMUdlaea85aeIdwMUdwaeaUaeIdxMUdxaea81aeIdzMUdzaea83aeIdCMUdCaeaBaeIdKMUdKaea80aeId3MUd3aea8XaeIdaMUdaaea8ZaeId8KMUd8Kaea8VaeIdyMUdyagaEc8S2fgea9eaeIdbMUdbaeaJaeIdlMUdlaea85aeIdwMUdwaeaUaeIdxMUdxaea81aeIdzMUdzaea83aeIdCMUdCaeaBaeIdKMUdKaea80aeId3MUd3aea8XaeIdaMUdaaea8ZaeId8KMUd8Kaea8VaeIdyMUdyagaHc8S2fgea9eaeIdbMUdbaeaJaeIdlMUdlaea85aeIdwMUdwaeaUaeIdxMUdxaea81aeIdzMUdzaea83aeIdCMUdCaeaBaeIdKMUdKaea80aeId3MUd3aea8XaeIdaMUdaaea8ZaeId8KMUd8Kaea8VaeIdyMUdya8Pawcltfh8AcbhXa8LhKina8AaXfgeasc;WbfaXfgQIdbaeIdbMUdbaeclfgYaQclfIdbaYIdbMUdbaecwfgYaQcwfIdbaYIdbMUdbaecxfgeaQcxfIdbaeIdbMUdbaXczfhXaKcufgKmbka8Pa5cltfh8AcbhXa8LhKina8AaXfgeasc;WbfaXfgQIdbaeIdbMUdbaeclfgYaQclfIdbaYIdbMUdbaecwfgYaQcwfIdbaYIdbMUdbaecxfgeaQcxfIdbaeIdbMUdbaXczfhXaKcufgKmbka8Pa8Ncltfh8AcbhXa8LhKina8AaXfgeasc;WbfaXfgQIdbaeIdbMUdbaeclfgYaQclfIdbaYIdbMUdbaecwfgYaQcwfIdbaYIdbMUdbaecxfgeaQcxfIdbaeIdbMUdbaXczfhXaKcufgKmbkaLcifgLad6mbkkcbhQdndnamcwGgTmbJbbbbhBcbhScbhvcbhixekcbhSa8Fcbyd1:jjjbHjjjjbbhiascxfasyd2gecdtfaiBdbasaecefgXBd2ascxfaXcdtfcuaialabadaAz:fjjjbgKcltaKcjjjjiGEcbyd1:jjjbHjjjjbbgvBdbasaecdfBd2avaKaia3alz:gjjjbJFFuuhBaKTmbavheaKhXinaeIdbg8VaBaBa8V9EEhBaeclfheaXcufgXmbkaKhSkasydlh9hdnalTmba9hclfhea9hydbhKaChXalhYcbhQincbaeydbg8AaK9RaXRbbcpeGEaQfhQaXcefhXaeclfhea8AhKaYcufgYmbkaQce4hQkcbhocuadaQ9Rcifg9icx2a9ic;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbhDascxfasyd2g6cdtfaDBdbasa6cefgeBd2ascxfaecdtfcua9icdta9icFFFFi0Ecbyd1:jjjbHjjjjbbgrBdbasa6cdfgeBd2ascxfaecdtfa8Fcbyd1:jjjbHjjjjbbgyBdbasa6cifgeBd2ascxfaecdtfalcbyd1:jjjbHjjjjbbg9kBdbasa6clfg8FBd2axaxNaIJbbjZamclGEg86a86N:vh9eJbbbbhJdndndndnadak9nmba9ici6measydwh0a8Lclth9maDcwfh9nJbbbbh85JbbbbhJinasclfabadalaAz:cjjjbabhzcbh8EcbhhinabahcdtfhHcbheindnaAazaefydbgQcdtgEfydbgYaAaHaec;q1jjbfydbcdtfydbgXcdtgwfydbg8ASmbaCaXfRbbgLcv2aCaQfRbbgKfc;G1jjbfRbbg5aKcv2aLfg8Nc;G1jjbfRbbg8MVcFeGTmbdna8AaY9nmba8Nc:G1jjbfRbbcFeGmekaKcufhYdnaKaL9hmbaYcFeGce0mba8JaEfydbaX9hmekdndnaKclSmbaLcl9hmekdnaYcFeGce0mba8JaEfydbaX9hmdkaLcufcFeGce0mba8KawfydbaQ9hmekaDa8Ecx2fgKaXaQa8McFeGgYEBdlaKaQaXaYEBdbaKaYa5Gcb9hBdwa8Ecefh8Ekaeclfgecx9hmbkdnahcifghad9pmbazcxfhza8Ecifa9i9nmekka8ETmecbhLinaqaAaDaLcx2fgKydbgYcdtgzfydbc8S2fgeIdwa3aKydlg8Acx2fgXIdwg8YNaeIdzaXIdbg8WNaeIdaMg8Va8VMMa8YNaeIdlaXIdlg8SNaeIdCa8YNaeId3Mg8Va8VMMa8SNaeIdba8WNaeIdxa8SNaeIdKMg8Va8VMMa8WNaeId8KMMM:lh8VJbbbbJbbjZaeIdygR:vaRJbbbb9BEhRdndnaKydwgEmbJFFuuh8XxekJbbbbJbbjZaqaAa8Acdtfydbc8S2fgeIdyg8U:va8UJbbbb9BEaeIdwa3aYcx2fgXIdwg8UNaeIdzaXIdbg8ZNaeIdaMg8Xa8XMMa8UNaeIdlaXIdlg8XNaeIdCa8UNaeId3Mg8Ua8UMMa8XNaeIdba8ZNaeIdxa8XNaeIdKMg8Ua8UMMa8ZNaeId8KMMM:lNh8XkaRa8VNh80dna8LTmbagaYc8S2fgQIdwa8YNaQIdza8WNaQIdaMg8Va8VMMa8YNaQIdla8SNaQIdCa8YNaQId3Mg8Va8VMMa8SNaQIdba8WNaQIdxa8SNaQIdKMg8Va8VMMa8WNaQId8KMMMh8Va8Ra8Aa8L2gHcdtfhXa8PaYa8L2gwcltfheaQIdyh8Ua8LhQinaXIdbgRaRa8UNaecxfIdba8YaecwfIdbNa8WaeIdbNa8SaeclfIdbNMMMgRaRM:tNa8VMh8VaXclfhXaeczfheaQcufgQmbkdndnaEmbJbbbbhRxekaga8Ac8S2fgQIdwa3aYcx2fgeIdwg8WNaQIdzaeIdbg8SNaQIdaMgRaRMMa8WNaQIdlaeIdlg8UNaQIdCa8WNaQId3MgRaRMMa8UNaQIdba8SNaQIdxa8UNaQIdKMgRaRMMa8SNaQId8KMMMhRa8RawcdtfhXa8PaHcltfheaQIdyh8Za8LhQinaXIdbg8Ya8Ya8ZNaecxfIdba8WaecwfIdbNa8SaeIdbNa8UaeclfIdbNMMMg8Ya8YM:tNaRMhRaXclfhXaeczfheaQcufgQmbkaR:lhRka80a8V:lMh80a8XaRMh8XaCaYfRbbcd9hmbdna8Ka8Ja8Jazfydba8ASEaaazfydbgHcdtfydbgzcu9hmbaaa8AcdtfydbhzkagaHc8S2fgQIdwa3azcx2fgeIdwg8YNaQIdzaeIdbg8WNaQIdaMg8Va8VMMa8YNaQIdlaeIdlg8SNaQIdCa8YNaQId3Mg8Va8VMMa8SNaQIdba8WNaQIdxa8SNaQIdKMg8Va8VMMa8WNaQId8KMMMh8Va8Raza8L2gwcdtfhXa8PaHa8L2g8NcltfheaQIdyh8Ua8LhQinaXIdbgRaRa8UNaecxfIdba8YaecwfIdbNa8WaeIdbNa8SaeclfIdbNMMMgRaRM:tNa8VMh8VaXclfhXaeczfheaQcufgQmbkdndnaEmbJbbbbhRxekagazc8S2fgQIdwa3aHcx2fgeIdwg8WNaQIdzaeIdbg8SNaQIdaMgRaRMMa8WNaQIdlaeIdlg8UNaQIdCa8WNaQId3MgRaRMMa8UNaQIdba8SNaQIdxa8UNaQIdKMgRaRMMa8SNaQId8KMMMhRa8Ra8NcdtfhXa8PawcltfheaQIdyh8Za8LhQinaXIdbg8Ya8Ya8ZNaecxfIdba8WaecwfIdbNa8SaeIdbNa8UaeclfIdbNMMMg8Ya8YM:tNaRMhRaXclfhXaeczfheaQcufgQmbkaR:lhRka80a8V:lMh80a8XaRMh8XkaKa80a8Xa80a8X9FgeEUdwaKa8AaYaeaETVgeEBdlaKaYa8AaeEBdbaLcefgLa8E9hmbkasc;Wbfcbcj;qbz:qjjjb8Aa9nhea8EhXinasc;WbfaeydbcA4cF8FGgQcFAaQcFA6EcdtfgQaQydbcefBdbaecxfheaXcufgXmbkcbhecbhXinasc;WbfaefgQydbhKaQaXBdbaKaXfhXaeclfgecj;qb9hmbkcbhea9nhXinasc;WbfaXydbcA4cF8FGgQcFAaQcFA6EcdtfgQaQydbgQcefBdbaraQcdtfaeBdbaXcxfhXa8Eaecefge9hmbkadak9RgQci9Uh9odnalTmbcbheayhXinaXaeBdbaXclfhXalaecefge9hmbkkcbh9pa9kcbalz:qjjjbhhaQcO9Uh9qa9oce4h9rcbh8Mcbh5dninaDara5cdtfydbcx2fg8NIdwg8Va9e9Emea8Ma9o9pmeJFFuuhRdna9ra8E9pmbaDara9rcdtfydbcx2fIdwJbb;aZNhRkdna8VaR9ETmba8VaJ9ETmba8Ma9q0mdkdnahaAa8NydlgHcdtg9sfydbgKfg9tRbbahaAa8Nydbgzcdtg9ufydbgefg9vRbbVmbaCazfRbbh9wdna9haecdtfgXclfydbgQaXydbgXSmbaQaX9RhYa3aKcx2fhLa3aecx2fhEa0aXcitfhecbhXcehwdnindnayaeydbcdtfydbgQaKSmbayaeclfydbcdtfydbg8AaKSmbaQa8ASmba3a8Acx2fg8AIdba3aQcx2fgQIdbg8Y:tg8VaEIdlaQIdlg8W:tg8ZNaEIdba8Y:tg8Xa8AIdla8W:tgRN:tg8Sa8VaLIdla8W:tg80NaLIdba8Y:tg83aRN:tg8WNaRaEIdwaQIdwg8U:tg81Na8Za8AIdwa8U:tg8YN:tg8ZaRaLIdwa8U:tgUNa80a8YN:tgRNa8Ya8XNa81a8VN:tg8Ua8Ya83NaUa8VN:tg8VNMMa8Sa8SNa8Za8ZNa8Ua8UNMMa8Wa8WNaRaRNa8Va8VNMMN:rJbbj8:N9FmdkaecwfheaXcefgXaY6hwaYaX9hmbkkawceGTmba9rcefh9rxekdndndndna9wc9:fPdebdkazheinayaecdtgefaHBdbaaaefydbgeaz9hmbxikkdna8Ka8Ja8Ja9ufydbaHSEaaa9ufydbgzcdtfydbgecu9hmbaaa9sfydbhekaya9ufaHBdbaehHkayazcdtfaHBdbka9vce86bba9tce86bba8NIdwg8VaJaJa8V9DEhJa9pcefh9pcecda9wceSEa8Mfh8Mka5cefg5a8E9hmbkka9pTmednalTmbcbh8AcbhEindnayaEcdtgefydbgQaESmbaAaQcdtfydbhzdnaEaAaefydb9hgHmbaqazc8S2fgeaqaEc8S2fgXIdbaeIdbMUdbaeaXIdlaeIdlMUdlaeaXIdwaeIdwMUdwaeaXIdxaeIdxMUdxaeaXIdzaeIdzMUdzaeaXIdCaeIdCMUdCaeaXIdKaeIdKMUdKaeaXId3aeId3MUd3aeaXIdaaeIdaMUdaaeaXId8KaeId8KMUd8KaeaXIdyaeIdyMUdyka8LTmbagaQc8S2fgeagaEc8S2gwfgXIdbaeIdbMUdbaeaXIdlaeIdlMUdlaeaXIdwaeIdwMUdwaeaXIdxaeIdxMUdxaeaXIdzaeIdzMUdzaeaXIdCaeIdCMUdCaeaXIdKaeIdKMUdKaeaXId3aeId3MUd3aeaXIdaaeIdaMUdaaeaXId8KaeId8KMUd8KaeaXIdyaeIdyMUdya9maQ2hLa8PhXa8LhKinaXaLfgeaXa8AfgQIdbaeIdbMUdbaeclfgYaQclfIdbaYIdbMUdbaecwfgYaQcwfIdbaYIdbMUdbaecxfgeaQcxfIdbaeIdbMUdbaXczfhXaKcufgKmbkaHmbJbbbbJbbjZaqawfgeIdyg8V:va8VJbbbb9BEaeIdwa3azcx2fgXIdwg8VNaeIdzaXIdbgRNaeIdaMg8Ya8YMMa8VNaeIdlaXIdlg8YNaeIdCa8VNaeId3Mg8Va8VMMa8YNaeIdbaRNaeIdxa8YNaeIdKMg8Va8VMMaRNaeId8KMMM:lNg8Va85a85a8V9DEh85ka8Aa9mfh8AaEcefgEal9hmbkcbhXa8JheindnaeydbgQcuSmbdnaXayaQcdtgKfydbgQ9hmbcuhQa8JaKfydbgKcuSmbayaKcdtfydbhQkaeaQBdbkaeclfhealaXcefgX9hmbkcbhXa8KheindnaeydbgQcuSmbdnaXayaQcdtgKfydbgQ9hmbcuhQa8KaKfydbgKcuSmbayaKcdtfydbhQkaeaQBdbkaeclfhealaXcefgX9hmbkka85aJa8LEh85cbhKabhecbhYindnayaeydbcdtfydbgXayaeclfydbcdtfydbgQSmbaXayaecwfydbcdtfydbg8ASmbaQa8ASmbabaKcdtfgLaXBdbaLcwfa8ABdbaLclfaQBdbaKcifhKkaecxfheaYcifgYad6mbkdndnaTmbaKhdxekdnaKak0mbaKhdxekdnaBa859FmbaKhdxekJFFuuhBcbhdabhecbhXindnavaiaeydbgQcdtfydbcdtfIdbg8Va859ETmbaeclf8Pdbh9xabadcdtfgYaQBdbaYclfa9x83dba8VaBaBa8V9EEhBadcifhdkaecxfheaXcifgXaK6mbkkadak0mbkkadTmdxekasclfabadalaAz:cjjjbJbbbbhJkcbhQabhecbhXindnaAaeydbg8AcdtfydbgKaAaeclfydbgLcdtfydbgYSmbaKaAaecwfydbgzcdtfydbgESmbaYaESmbabaQcdtfgKa8ABdbaKcwfazBdbaKclfaLBdbaQcifhQkaecxfheaXcifgXad6mbkdndnaTmbaQhoxekdnaQak0mbaQhoxekdnaBa9e9FmbaQhoxekcehYinaBJbb;aZNg8Va9ea8Va9e9DEh8YJbbbbh8VdnaSTmbavheaShAinaeIdbgRa8VaRa8Y9FEa8VaRa8V9EEh8VaeclfheaAcufgAmbkkJFFuuhBcbhoabhecbhAindnavaiaeydbgXcdtfydbcdtfIdbgRa8Y9ETmbaeclf8Pdbh9xabaocdtfgKaXBdbaKclfa9x83dbaRaBaBaR9EEhBaocifhokaecxfheaAcifgAaQ6mbkdnaYaoaQ9hVceGmbaQhoxdka8VaJaJa8V9DEhJaoak9nmecbhYaohQaBa9e9FmbkkdnamcjjjjlGTmbaOmbaoTmbcbhYabheinaCaeydbgQfRbbc3th8AaecwfgLydbhAdndna8JaQcdtgzfydbaeclfgEydbgXSmbcbhKa8KaXcdtfydbaQ9hmekcjjjj94hKkaea8AaKVaQVBdbaCaXfRbbc3th8Adndna8JaXcdtfydbaASmbcbhKa8KaAcdtfydbaX9hmekcjjjj94hKkaEa8AaKVaXVBdbaCaAfRbbc3thKdndna8JaAcdtfydbaQSmbcbhXa8KazfydbaA9hmekcjjjj94hXkaLaKaXVaAVBdbaecxfheaYcifgYao6mbkkaOTmbaoTmbaoheinabaOabydbcdtfydbBdbabclfhbaecufgembkkdnaPTmbaPa86aJ:rNUdbka6cdtascxffcxfhednina8FTmeaeydbcbyd:m:jjjbH:bjjjbbaec98fhea8Fcufh8Fxbkkasc;W;qbf8Kjjjjbaok;Yieouabydlhvabydbclfcbaicdtz:qjjjbhoadci9UhrdnadTmbdnalTmbaehwadhDinaoalawydbcdtfydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbxdkkaehwadhDinaoawydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbkkdnaiTmbcbhDaohwinawydbhqawaDBdbawclfhwaqaDfhDaicufgimbkkdnadci6mbinaecwfydbhwaeclfydbhDaeydbhidnalTmbalawcdtfydbhwalaDcdtfydbhDalaicdtfydbhikavaoaicdtfgqydbcitfaDBdbavaqydbcitfawBdlaqaqydbcefBdbavaoaDcdtfgqydbcitfawBdbavaqydbcitfaiBdlaqaqydbcefBdbavaoawcdtfgwydbcitfaiBdbavawydbcitfaDBdlawawydbcefBdbaecxfhearcufgrmbkkabydbcbBdbk:todDue99aicd4aifhrcehwinawgDcethwaDar6mbkcuaDcdtgraDcFFFFi0Ecbyd1:jjjbHjjjjbbhwaoaoyd9GgqcefBd9GaoaqcdtfawBdbawcFearz:qjjjbhkdnaiTmbalcd4hlaDcufhxcbhminamhDdnavTmbavamcdtfydbhDkcbadaDal2cdtfgDydlgwawcjjjj94SEgwcH4aw7c:F:b:DD2cbaDydbgwawcjjjj94SEgwcH4aw7c;D;O:B8J27cbaDydwgDaDcjjjj94SEgDcH4aD7c:3F;N8N27axGhwamcdthPdndndnavTmbakawcdtfgrydbgDcuSmeadavaPfydbal2cdtfgsIdbhzcehqinaqhrdnadavaDcdtfydbal2cdtfgqIdbaz9CmbaqIdlasIdl9CmbaqIdwasIdw9BmlkarcefhqakawarfaxGgwcdtfgrydbgDcu9hmbxdkkakawcdtfgrydbgDcuSmbadamal2cdtfgsIdbhzcehqinaqhrdnadaDal2cdtfgqIdbaz9CmbaqIdlasIdl9CmbaqIdwasIdw9BmikarcefhqakawarfaxGgwcdtfgrydbgDcu9hmbkkaramBdbamhDkabaPfaDBdbamcefgmai9hmbkkakcbyd:m:jjjbH:bjjjbbaoaoyd9GcufBd9GdnaeTmbaiTmbcbhDaehwinawaDBdbawclfhwaiaDcefgD9hmbkcbhDaehwindnaDabydbgrSmbawaearcdtfgrydbBdbaraDBdbkawclfhwabclfhbaiaDcefgD9hmbkkk;Qodvuv998Jjjjjbca9Rgvczfcwfcbyd11jjbBdbavcb8Pdj1jjb83izavcwfcbydN1jjbBdbavcb8Pd:m1jjb83ibdnadTmbaicd4hodnabmbdnalTmbcbhrinaealarcdtfydbao2cdtfhwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkarcefgrad9hmbxikkaocdthrcbhwincbhiinavczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbxdkkdnalTmbcbhrinabarcx2fgiaealarcdtfydbao2cdtfgwIdbUdbaiawIdlUdlaiawIdwUdwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkarcefgrad9hmbxdkkaocdthlcbhraehwinabarcx2fgiaearao2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawalfhwarcefgrad9hmbkkJbbbbavIdbavIdzgk:tgqaqJbbbb9DEgqavIdlavIdCgx:tgmamaq9DEgqavIdwavIdKgm:tgPaPaq9DEhPdnabTmbadTmbJbbbbJbbjZaP:vaPJbbbb9BEhqinabaqabIdbak:tNUdbabclfgvaqavIdbax:tNUdbabcwfgvaqavIdbam:tNUdbabcxfhbadcufgdmbkkaPk:ZlewudnaeTmbcbhvabhoinaoavBdbaoclfhoaeavcefgv9hmbkkdnaiTmbcbhrinadarcdtfhwcbhDinalawaDcdtgvc;a1jjbfydbcdtfydbcdtfydbhodnabalawavfydbcdtfydbgqcdtfgkydbgvaqSmbinakabavgqcdtfgxydbgvBdbaxhkaqav9hmbkkdnabaocdtfgkydbgvaoSmbinakabavgocdtfgxydbgvBdbaxhkaoav9hmbkkdnaqaoSmbabaqaoaqao0Ecdtfaqaoaqao6EBdbkaDcefgDci9hmbkarcifgrai6mbkkdnaembcbskcbhxindnalaxcdtgvfydbax9hmbaxhodnabavfgDydbgvaxSmbaDhqinaqabavgocdtfgkydbgvBdbakhqaoav9hmbkkaDaoBdbkaxcefgxae9hmbkcbhvabhocbhkindndnavalydbgq9hmbdnavaoydbgq9hmbaoakBdbakcefhkxdkaoabaqcdtfydbBdbxekaoabaqcdtfydbBdbkaoclfhoalclfhlaeavcefgv9hmbkakk;Jiilud99duabcbaecltz:qjjjbhvdnalTmbadhoaihralhwinarcwfIdbhDarclfIdbhqavaoydbcltfgkarIdbakIdbMUdbakclfgxaqaxIdbMUdbakcwfgxaDaxIdbMUdbakcxfgkakIdbJbbjZMUdbaoclfhoarcxfhrawcufgwmbkkdnaeTmbavhraehkinarcxfgoIdbhDaocbBdbararIdbJbbbbJbbjZaD:vaDJbbbb9BEgDNUdbarclfgoaDaoIdbNUdbarcwfgoaDaoIdbNUdbarczfhrakcufgkmbkkdnalTmbinavadydbcltfgrcxfgkaicwfIdbarcwfIdb:tgDaDNaiIdbarIdb:tgDaDNaiclfIdbarclfIdb:tgDaDNMMgDakIdbgqaqaD9DEUdbadclfhdaicxfhialcufglmbkkdnaeTmbavcxfhrinabarIdbUdbarczfhrabclfhbaecufgembkkk8MbabaeadaialavcbcbcbcbcbaoarawaDz:bjjjbk8MbabaeadaialavaoarawaDaqakaxamaPz:bjjjbk;28Joque99due99iuq998Jjjjjbc;Wb9Rgq8Kjjjjbcbhkaqcxfcbc;Kbz:qjjjb8Aaqcualcx2alc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbgxBdxaqceBd2axaialavcbz:ejjjb8AaqcualcdtalcFFFFi0Egmcbyd1:jjjbHjjjjbbgiBdzaqcdBd2adci9UhParco9UhsdndnJbbjZJ9VO:d86awawJ9VO:d869DE:vgw:lJbbb9p9DTmbaw:Ohzxekcjjjj94hzkdndnaombazcd9imekdnalTmbazcuf:YhwdnaoTmbcbhvaihHaxhOindndnaoavfRbbTmbavcjjjjlVhAxekdndnaOclfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhAxekcjjjj94hAkaAcqthAdndnaOcwfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhXxekcjjjj94hXkaAaXVhAdndnaOIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhXxekcjjjj94hXkaAaXcCtVhAkaHaABdbaHclfhHaOcxfhOalavcefgv9hmbxdkkaxhvaihOalhHindndnavIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhAxekcjjjj94hAkaAcCthAdndnavclfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhXxekcjjjj94hXkaXcqtaAVhAdndnavcwfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhXxekcjjjj94hXkaOaAaXVBdbavcxfhvaOclfhOaHcufgHmbkkadTmbcbhkaehvcbhOinakaiavclfydbcdtfydbgHaiavcwfydbcdtfydbgA9haiavydbcdtfydbgXaH9haXaA9hGGfhkavcxfhvaOcifgOad6mbkkarci9UhQdndnas:Z:rJbbbZMgw:lJbbb9p9DTmbaw:Ohvxekcjjjj94hvkaQ:ZhLcbhKc:bwhHdndninaPhYaHhXazhrakg8AaQ9pmeaXar9Rcd9imeavaXcufavaX9iEarcefavar9kEhsdnalTmbascuf:YhwdnaoTmbcbhvaihzaxhOindndnaoavfRbbTmbavcjjjjlVhHxekdndnaOclfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhHxekcjjjj94hHkaHcqthHdndnaOcwfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhPxekcjjjj94hPkaHaPVhHdndnaOIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhPxekcjjjj94hPkaHaPcCtVhHkazaHBdbazclfhzaOcxfhOalavcefgv9hmbxdkkaxhvaihOalhzindndnavIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhHxekcjjjj94hHkaHcCthHdndnavclfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhPxekcjjjj94hPkaPcqtaHVhHdndnavcwfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhPxekcjjjj94hPkaOaHaPVBdbavcxfhvaOclfhOazcufgzmbkkcbhOdnadTmbaehvcbhzinaOaiavclfydbcdtfydbgHaiavcwfydbcdtfydbgP9haiavydbcdtfydbgAaH9haAaP9hGGfhOavcxfhvazcifgzad6mbkkaYhPaOhkaXhHashzdnaOaQ9nmbaOhPa8AhkashHarhzkdndnaKcl0mbdnaY:Zgwa8A:ZgC:tas:YgEar:Y:tg3aEaX:Y:tg5aO:Zg8EaL:tNNNawaL:ta5NaCa8E:tNaCaL:ta3Na8Eaw:tNM:vaEMJbbbZMgw:lJbbb9p9DTmbaw:Ohvxdkcjjjj94hvxekazaHfcd9ThvkaKcefgKcs9hmbxdkka8AhkarhzkdndndnakmbJbbjZhwcbhicdhvaDmexdkalcd4alfhHcehOinaOgvcethOavaH6mbkcbhOaqcuavcdtgravcFFFFi0Ecbyd1:jjjbHjjjjbbgsBdCaqciBd2aqamcbyd1:jjjbHjjjjbbgXBdKaqclBd2dndndndnalTmbazcuf:YhwaoTmecbhOaihHaxhzindndnaoaOfRbbTmbaOcjjjjlVhPxekdndnazclfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhPxekcjjjj94hPkaPcqthPdndnazcwfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhAxekcjjjj94hAkaPaAVhPdndnazIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhAxekcjjjj94hAkaPaAcCtVhPkaHaPBdbaHclfhHazcxfhzalaOcefgO9hmbxikkascFearz:qjjjb8AcbhrcbhvxdkaxhOaihzalhHindndnaOIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhPxekcjjjj94hPkaPcCthPdndnaOclfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhAxekcjjjj94hAkaAcqtaPVhPdndnaOcwfIdbawNJbbbZMgC:lJbbb9p9DTmbaC:OhAxekcjjjj94hAkazaPaAVBdbaOcxfhOazclfhzaHcufgHmbkkascFearz:qjjjbhAavcufhocbhrcbhsindndndnaAaiascdtgKfydbgHcm4aH7c:v;t;h;Ev2gvcs4av7aoGgzcdtfgPydbgOcuSmbcehvinaiaOcdtgOfydbaHSmdazavfhOavcefhvaAaOaoGgzcdtfgPydbgOcu9hmbkkaPasBdbarhvarcefhrxekaXaOfydbhvkaXaKfavBdbascefgsal9hmbkcuarc8S2gOarc;D;O;f8U0EhvkcbhAaqavcbyd1:jjjbHjjjjbbgvBd3aqcvBd2avcbaOz:qjjjbhOdnadTmbaehiinJbbnnJbbjZaXaiydbgHcdtfydbgvaXaiclfydbgzcdtfydbgsSavaXaicwfydbgPcdtfydbgKSGgoEh8Fdnaxazcx2fgzIdbaxaHcx2fgHIdbg8E:tgCaxaPcx2fgPIdlaHIdlg3:tgwNaPIdba8E:tgEazIdla3:tgaN:tgLaLNaaaPIdwaHIdwg5:tghNawazIdwa5:tgaN:tgwawNaaaENahaCN:tgCaCNMM:rgEJbbbb9ETmbaLaE:vhLaCaE:vhCawaE:vhwkaOavc8S2fgvavIdbawa8FaE:rNgEawNNgaMUdbavaCaEaCNghNggavIdlMUdlavaLaEaLNg8FNg8JavIdwMUdwavahawNghavIdxMUdxava8FawNg8KavIdzMUdzava8FaCNg8FavIdCMUdCavawaEaLa5Nawa8ENa3aCNMM:mg3Ng8ENgwavIdKMUdKavaCa8ENgCavId3MUd3avaLa8ENgLavIdaMUdaava8Ea3Ng8EavId8KMUd8KavaEavIdyMUdydnaombaOasc8S2fgvaaavIdbMUdbavagavIdlMUdlava8JavIdwMUdwavahavIdxMUdxava8KavIdzMUdzava8FavIdCMUdCavawavIdKMUdKavaCavId3MUd3avaLavIdaMUdaava8EavId8KMUd8KavaEavIdyMUdyaOaKc8S2fgvaaavIdbMUdbavagavIdlMUdlava8JavIdwMUdwavahavIdxMUdxava8KavIdzMUdzava8FavIdCMUdCavawavIdKMUdKavaCavId3MUd3avaLavIdaMUdaava8EavId8KMUd8KavaEavIdyMUdykaicxfhiaAcifgAad6mbkkcbhHaqcuarcdtgvarcFFFFi0Egicbyd1:jjjbHjjjjbbgzBdaaqcoBd2aqaicbyd1:jjjbHjjjjbbgiBd8KaqcrBd2azcFeavz:qjjjbhsdnalTmbaXhzinJbbbbJbbjZaOazydbgPc8S2fgvIdygw:vawJbbbb9BEavIdwaxcwfIdbgwNavIdzaxIdbgCNavIdaMgLaLMMawNavIdlaxclfIdbgLNavIdCawNavId3MgwawMMaLNavIdbaCNavIdxaLNavIdKMgwawMMaCNavId8KMMM:lNhwdndnasaPcdtgvfgPydbcuSmbaiavfIdbaw9ETmekaPaHBdbaiavfawUdbkazclfhzaxcxfhxalaHcefgH9hmbkkJbbbbhwdnarTmbinaiIdbgCawawaC9DEhwaiclfhiarcufgrmbkkakcd4akfhOcehiinaigvcethiavaO6mbkcbhiaqcuavcdtgOavcFFFFi0Ecbyd1:jjjbHjjjjbbgzBdyazcFeaOz:qjjjbhPdnadTmbavcufhAcbhrcbhxindnaXaeaxcdtfgvydbcdtfydbgiaXavclfydbcdtfydbgOSmbaiaXavcwfydbcdtfydbgvSmbaOavSmbasavcdtfydbhHdndnasaOcdtfydbgvasaicdtfydbgi9pmbavaH9pmbaHhlaihoavhHxekdnaHai9pmbaHav9pmbaihlavhoxekavhlaHhoaihHkabarcx2fgvaHBdbavcwfaoBdbavclfalBdbdnaPaoc:3F;N8N2alc:F:b:DD27aHc;D;O:B8J27aAGgOcdtfgvydbgicuSmbcehzinazhvdnabaicx2fgiydbaH9hmbaiydlal9hmbaiydwaoSmikavcefhzaPaOavfaAGgOcdtfgvydbgicu9hmbkkavarBdbarcefhrkaxcifgxad6mbkarci2hikdnaDmbcwhvxdkaw:rhwcwhvkaDawUdbkavcdthvdninavTmeavc98fgvaqcxffydbcbyd:m:jjjbH:bjjjbbxbkkaqc;Wbf8Kjjjjbaik:0ldwue9:8Jjjjjbc;Wb9Rgr8Kjjjjbcbhwarcxfcbc;Kbz:qjjjb8AdnabaeSmbabaeadcdtz:pjjjb8AkarcualcdtalcFFFFi0EgDcbyd1:jjjbHjjjjbbgqBdxarceBd2aqcbaialavcbarcxfz:djjjbcualcx2alc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbhkarcxfaryd2gxcdtgmfakBdbaraxcefgPBd2akaialavcbz:ejjjb8AarcxfaPcdtfaDcbyd1:jjjbHjjjjbbgvBdbaraxcdfgiBd2arcxfaicdtfcuavalaeadaqz:fjjjbgecltaecjjjjiGEcbyd1:jjjbHjjjjbbgiBdbaiaeavakalz:gjjjbdnadTmbaoaoNhocbhwabhlcbhkindnaiavalydbgecdtfydbcdtfIdbao9ETmbalclf8PdbhsabawcdtfgqaeBdbaqclfas83dbawcifhwkalcxfhlakcifgkad6mbkkaxcifhlamarcxffcwfhkdninalTmeakydbcbyd:m:jjjbH:bjjjbbakc98fhkalcufhlxbkkarc;Wbf8Kjjjjbawk:DCoDud99rue99iul998Jjjjjbc;Wb9Rgw8KjjjjbdndnarmbcbhDxekawcxfcbc;Kbz:qjjjb8Aawcuadcx2adc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbgqBdxawceBd2aqaeadaicbz:ejjjb8AawcuadcdtadcFFFFi0Egkcbyd1:jjjbHjjjjbbgxBdzawcdBd2adcd4adfhmceheinaegicetheaiam6mbkcbhPawcuaicdtgsaicFFFFi0Ecbyd1:jjjbHjjjjbbgzBdCawciBd2dndnar:ZgH:rJbbbZMgO:lJbbb9p9DTmbaO:Ohexekcjjjj94hekaicufhAc:bwhmcbhCadhXcbhQinaChLaeamgKcufaeaK9iEaPgDcefaeaD9kEhYdndnadTmbaYcuf:YhOaqhiaxheadhmindndnaiIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcCthCdndnaiclfIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhExekcjjjj94hEkaEcqtaCVhCdndnaicwfIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhExekcjjjj94hEkaeaCaEVBdbaicxfhiaeclfheamcufgmmbkazcFeasz:qjjjbh3cbh5cbhPindna3axaPcdtfydbgCcm4aC7c:v;t;h;Ev2gics4ai7aAGgmcdtfgEydbgecuSmbaeaCSmbcehiina3amaifaAGgmcdtfgEydbgecuSmeaicefhiaeaC9hmbkkaEaCBdba5aecuSfh5aPcefgPad9hmbxdkkazcFeasz:qjjjb8Acbh5kaDaYa5ar0giEhPaLa5aiEhCdna5arSmbaYaKaiEgmaP9Rcd9imbdndnaQcl0mbdnaX:ZgOaL:Zg8A:taY:Yg8EaD:Y:tg8Fa8EaK:Y:tgaa5:ZghaH:tNNNaOaH:taaNa8Aah:tNa8AaH:ta8FNahaO:tNM:va8EMJbbbZMgO:lJbbb9p9DTmbaO:Ohexdkcjjjj94hexekaPamfcd9Theka5aXaiEhXaQcefgQcs9hmekkdndnaCmbcihicbhDxekcbhiawakcbyd1:jjjbHjjjjbbg5BdKawclBd2aPcuf:Yh8AdndnadTmbaqhiaxheadhmindndnaiIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhCxekcjjjj94hCkaCcCthCdndnaiclfIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhExekcjjjj94hEkaEcqtaCVhCdndnaicwfIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhExekcjjjj94hEkaeaCaEVBdbaicxfhiaeclfheamcufgmmbkazcFeasz:qjjjbh3cbhDcbhYindndndna3axaYcdtgKfydbgCcm4aC7c:v;t;h;Ev2gics4ai7aAGgmcdtfgEydbgecuSmbcehiinaxaecdtgefydbaCSmdamaifheaicefhia3aeaAGgmcdtfgEydbgecu9hmbkkaEaYBdbaDhiaDcefhDxeka5aefydbhika5aKfaiBdbaYcefgYad9hmbkcuaDc32giaDc;j:KM;jb0EhexekazcFeasz:qjjjb8AcbhDcbhekawaecbyd1:jjjbHjjjjbbgeBd3awcvBd2aecbaiz:qjjjbhEavcd4hKdnadTmbdnalTmbaKcdth3a5hCaqhealhmadhAinaEaCydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiamIdbaiIdxMUdxaiamclfIdbaiIdzMUdzaiamcwfIdbaiIdCMUdCaiaiIdKJbbjZMUdKaCclfhCaecxfheama3fhmaAcufgAmbxdkka5hmaqheadhCinaEamydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiaiIdxJbbbbMUdxaiaiIdzJbbbbMUdzaiaiIdCJbbbbMUdCaiaiIdKJbbjZMUdKamclfhmaecxfheaCcufgCmbkkdnaDTmbaEhiaDheinaiaiIdbJbbbbJbbjZaicKfIdbgO:vaOJbbbb9BEgONUdbaiclfgmaOamIdbNUdbaicwfgmaOamIdbNUdbaicxfgmaOamIdbNUdbaiczfgmaOamIdbNUdbaicCfgmaOamIdbNUdbaic3fhiaecufgembkkcbhCawcuaDcdtgYaDcFFFFi0Egicbyd1:jjjbHjjjjbbgeBdaawcoBd2awaicbyd1:jjjbHjjjjbbg3Bd8KaecFeaYz:qjjjbhxdnadTmbJbbjZJbbjZa8A:vaPceSEaoNgOaONh8AaKcdthPalheina8Aaec;81jjbalEgmIdwaEa5ydbgAc32fgiIdC:tgOaONamIdbaiIdx:tgOaONamIdlaiIdz:tgOaONMMNaqcwfIdbaiIdw:tgOaONaqIdbaiIdb:tgOaONaqclfIdbaiIdl:tgOaONMMMhOdndnaxaAcdtgifgmydbcuSmba3aifIdbaO9ETmekamaCBdba3aifaOUdbka5clfh5aqcxfhqaeaPfheadaCcefgC9hmbkkabaxaYz:pjjjb8AcrhikaicdthiinaiTmeaic98fgiawcxffydbcbyd:m:jjjbH:bjjjbbxbkkawc;Wbf8KjjjjbaDk:Ydidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgvaDIdbgoaoav9EEUdbaladfgDavaDIdbgoaoav9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkJbbbbavavJbbbb9DEgvaoaoav9DEgvararav9DEk9DeeuabcFeaicdtz:qjjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcifc98GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;teeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiaeydlBdlaiaeydwBdwaiaeydxBdxaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk:3eedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdxaialBdwaialBdlaialBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcrfc94GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd:q:jjjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd:q:jjjbfgdBd:q:jjjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akkk:Iedbcjwk1eFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbeeebeebebbeeebebbbbbebebbbbbbbbbebbbdbbbbbbbebbbebbbdbbbbbbbbbbbeeeeebebbebbebebbbeebbbbbbbbbbbbbbbbbbbbbc1Dkxebbbdbbb:GNbb'; // embed! wasm

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

	function simplifySloppy(
		fun,
		indices,
		index_count,
		vertex_positions,
		vertex_count,
		vertex_positions_stride,
		vertex_lock,
		target_index_count,
		target_error
	) {
		var sbrk = instance.exports.sbrk;
		var te = sbrk(4);
		var ti = sbrk(index_count * 4);
		var sp = sbrk(vertex_count * vertex_positions_stride);
		var si = sbrk(index_count * 4);
		var vl = vertex_lock ? sbrk(vertex_count) : 0;
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(vertex_positions), sp);
		heap.set(bytes(indices), si);
		if (vertex_lock) {
			heap.set(bytes(vertex_lock), vl);
		}
		var result = fun(ti, si, index_count, sp, vertex_count, vertex_positions_stride, vl, target_index_count, target_error, te);
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
		Regularize: 16,
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

		simplifySloppy: function (indices, vertex_positions, vertex_positions_stride, vertex_lock, target_index_count, target_error) {
			assert(
				indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array
			);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(vertex_lock == null || vertex_lock instanceof Uint8Array);
			assert(vertex_lock == null || vertex_lock.length == vertex_positions.length / vertex_positions_stride);
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
				vertex_lock ? new Uint8Array(vertex_lock) : null,
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
