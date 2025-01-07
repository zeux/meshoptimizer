// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function () {
	// Built with clang version 19.1.5-wasi-sdk
	// Built from meshoptimizer 0.22
	var wasm =
		'b9H79Tebbbe9Hk9Geueu9Geub9Gbb9Gsuuuuuuuuuuuu99uueu9Gvuuuuub9Gvuuuuue999Gquuuuuuu99uueu9Gwuuuuuu99ueu9Giuuue999Gluuuueu9GiuuueuizsdilvoirwDbqqbeqlve9Weiiviebeoweuec:G:Pdkr:Tewo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95bl8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bvQ9TW79O9V9Wt9F79P9T9W29P9M959q9V9P9Ut7boX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbra9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbwl79IV9RbDDwebcekdmxq:x:yesdbk:Z9VvKue99euY99Ou8Jjjjjbc;W;qb9Rgs8Kjjjjbcbhzascxfcbc;Kbz:ljjjb8AdnabaeSmbabaeadcdtz:kjjjb8AkdnamcdGTmbalcrfci4gHcbyd:m:jjjbHjjjjbbheascxfasyd2gOcdtfaeBdbasaOcefBd2aecbaHz:ljjjbhAcbhlcbhednadTmbcbhlabheadhHinaAaeydbgOci4fgCaCRbbgCceaOcrGgOtV86bbaCcu7aO4ceGalfhlaeclfheaHcufgHmbkcualcdtalcFFFFi0Ehekaecbyd:m:jjjbHjjjjbbhzascxfasyd2gecdtfazBdbasaecefBd2alcd4alfhOcehHinaHgecethHaeaO6mbkcbhXcuaecdtgOaecFFFFi0Ecbyd:m:jjjbHjjjjbbhHascxfasyd2gCcdtfaHBdbasaCcefBd2aHcFeaOz:ljjjbhQdnadTmbaecufhLcbhKindndnaQabaXcdtfgYydbgCc:v;t;h;Ev2aLGgOcdtfgAydbgHcuSmbceheinazaHcdtfydbaCSmdaOaefhHaecefheaQaHaLGgOcdtfgAydbgHcu9hmbkkazaKcdtfaCBdbaAaKBdbaKhHaKcefhKkaYaHBdbaXcefgXad9hmbkkaQcbyd1:jjjbH:bjjjbbasasyd2cufBd2kcualcefgecdtaecFFFFi0Ecbyd:m:jjjbHjjjjbbh8Aascxfasyd2gecdtfa8ABdbasa8ABdlasaecefBd2cuadcitadcFFFFe0Ecbyd:m:jjjbHjjjjbbhEascxfasyd2gecdtfaEBdbasaEBdwasaecefBd2asclfabadalcbz:cjjjbcualcdtg3alcFFFFi0Eg5cbyd:m:jjjbHjjjjbbhLascxfasyd2gecdtfaLBdbasaecefBd2a5cbyd:m:jjjbHjjjjbbh8Eascxfasyd2gecdtfa8EBdbasaecefBd2alcd4alfhOcehHinaHgecethHaeaO6mbkcbhYcuaecdtgOaecFFFFi0Ecbyd:m:jjjbHjjjjbbhHascxfasyd2gCcdtfaHBdbasaCcefBd2aHcFeaOz:ljjjbhQdnalTmbavcd4hCaecufhKinaYhednazTmbazaYcdtfydbhekaiaeaC2cdtfgeydlgHcH4aH7c:F:b:DD2aeydbgHcH4aH7c;D;O:B8J27aeydwgecH4ae7c:3F;N8N27aKGhHaYcdth8FdndndnazTmbaQaHcdtfgAydbgecuSmeaiaza8FfydbaC2cdtfhXcehOinaiazaecdtfydbaC2cdtfaXcxz:ojjjbTmiaHaOfheaOcefhOaQaeaKGgHcdtfgAydbgecu9hmbxdkkaQaHcdtfgAydbgecuSmbaiaYaC2cdtfhXcehOinaiaeaC2cdtfaXcxz:ojjjbTmdaHaOfheaOcefhOaQaeaKGgHcdtfgAydbgecu9hmbkkaAaYBdbaYhekaLa8FfaeBdbaYcefgYal9hmbkcbhea8EhHinaHaeBdbaHclfhHalaecefge9hmbkcbheaLhHa8EhOindnaeaHydbgCSmbaOa8EaCcdtfgCydbBdbaCaeBdbkaHclfhHaOclfhOalaecefge9hmbkkcbhaaQcbyd1:jjjbH:bjjjbbasasyd2cufBd2alcbyd:m:jjjbHjjjjbbhKascxfasyd2gecdtfaKBdbasaecefBd2a5cbyd:m:jjjbHjjjjbbheascxfasyd2gHcdtfaeBdbasaHcefBd2a5cbyd:m:jjjbHjjjjbbhHascxfasyd2gOcdtfaHBdbasaOcefBd2aecFea3z:ljjjbhhaHcFea3z:ljjjbhgdnalTmbaEcwfh8Jindna8AaagOcefgacdtfydbgCa8AaOcdtgefydbgHSmbaCaH9Rh8FaEaHcitfh3agaefh8KahaefhYcbhAindndna3aAcitfydbgQaO9hmbaYaOBdba8KaOBdbxekdna8AaQcdtg8LfgeclfydbgHaeydbgeSmbaEaecitgCfydbaOSmeaHae9Rh8Maecu7aHfhXa8JaCfhHcbheinaXaeSmeaecefheaHydbhCaHcwfhHaCaO9hmbkaea8M6mekaga8LfgeaOaQaeydbcuSEBdbaYaQaOaYydbcuSEBdbkaAcefgAa8F9hmbkkaaal9hmbkaLhHa8EhOaghCahhAcbheindndnaeaHydbgQ9hmbdnaeaOydbgQ9hmbaAydbhQdnaCydbgXcu9hmbaQcu9hmbaKaefcb86bbxikaKaefhYdnaeaXSmbaeaQSmbaYce86bbxikaYcl86bbxdkdnaea8EaQcdtgXfydb9hmbdnaCydbgYcuSmbaeaYSmbaAydbg8FcuSmbaea8FSmbagaXfydbg3cuSmba3aQSmbahaXfydbgXcuSmbaXaQSmbdnaLaYcdtfydbgQaLaXcdtfydb9hmbaQaLa8FcdtfydbgXSmbaXaLa3cdtfydb9hmbaKaefcd86bbxlkaKaefcl86bbxikaKaefcl86bbxdkaKaefcl86bbxekaKaefaKaQfRbb86bbkaHclfhHaOclfhOaCclfhCaAclfhAalaecefge9hmbkdnaqTmbdndnazTmbazheaLhHalhOindnaqaeydbfRbbTmbaKaHydbfcl86bbkaeclfheaHclfhHaOcufgOmbxdkkaLhealhHindnaqRbbTmbaKaeydbfcl86bbkaqcefhqaeclfheaHcufgHmbkkaLhealhOaKhHindnaKaeydbfRbbcl9hmbaHcl86bbkaeclfheaHcefhHaOcufgOmbkkamceGTmbaKhealhHindnaeRbbce9hmbaecl86bbkaecefheaHcufgHmbkkcualcx2alc;v:Q;v:Qe0Ecbyd:m:jjjbHjjjjbbhaascxfasyd2gecdtfaaBdbasaecefBd2aaaialavazz:djjjbh8NdndnaDmbcbhycbh8Jxekcbh8JawhecbhHindnaeIdbJbbbb9ETmbasc;Wbfa8JcdtfaHBdba8Jcefh8JkaeclfheaDaHcefgH9hmbkcua8Jal2gecdtaecFFFFi0Ecbyd:m:jjjbHjjjjbbhyascxfasyd2gecdtfayBdbasaecefBd2alTmba8JTmbarcd4hYdnazTmba8Jcdth8FcbhQayhXinaoazaQcdtfydbaY2cdtfhAasc;WbfheaXhHa8JhOinaHaAaeydbcdtgCfIdbawaCfIdbNUdbaeclfheaHclfhHaOcufgOmbkaXa8FfhXaQcefgQal9hmbxdkka8Jcdth8FcbhQayhXinaoaQaY2cdtfhAasc;WbfheaXhHa8JhOinaHaAaeydbcdtgCfIdbawaCfIdbNUdbaeclfheaHclfhHaOcufgOmbkaXa8FfhXaQcefgQal9hmbkkcualc8S2gHalc;D;O;f8U0EgCcbyd:m:jjjbHjjjjbbheascxfasyd2gOcdtfaeBdbasaOcefBd2aecbaHz:ljjjbhqdndndndna8JTmbaCcbyd:m:jjjbHjjjjbbhvascxfasyd2gecdtfavBdbcehOasaecefBd2avcbaHz:ljjjb8Acua8Jal2gecltgHaecFFFFb0Ecbyd:m:jjjbHjjjjbbhrascxfasyd2gecdtfarBdbasaecefBd2arcbaHz:ljjjb8AadmexikcbhvadTmecbhrkcbhAabhHindnaaaHclfydbgQcx2fgeIdbaaaHydbgXcx2fgOIdbg8P:tgIaaaHcwfydbgYcx2fgCIdlaOIdlg8R:tg8SNaCIdba8P:tgRaeIdla8R:tg8UN:tg8Va8VNa8UaCIdwaOIdwg8W:tg8XNa8SaeIdwa8W:tg8UN:tg8Sa8SNa8UaRNa8XaIN:tgIaINMM:rgRJbbbb9ETmba8VaR:vh8VaIaR:vhIa8SaR:vh8SkaqaLaXcdtfydbc8S2fgea8SaR:rgRa8SNNg8UaeIdbMUdbaeaIaRaINg8YNg8XaeIdlMUdlaea8VaRa8VNg8ZNg80aeIdwMUdwaea8Ya8SNg8YaeIdxMUdxaea8Za8SNg81aeIdzMUdzaea8ZaINg8ZaeIdCMUdCaea8SaRa8Va8WNa8Sa8PNa8RaINMM:mg8RNg8PNg8SaeIdKMUdKaeaIa8PNgIaeId3MUd3aea8Va8PNg8VaeIdaMUdaaea8Pa8RNg8PaeId8KMUd8KaeaRaeIdyMUdyaqaLaQcdtfydbc8S2fgea8UaeIdbMUdbaea8XaeIdlMUdlaea80aeIdwMUdwaea8YaeIdxMUdxaea81aeIdzMUdzaea8ZaeIdCMUdCaea8SaeIdKMUdKaeaIaeId3MUd3aea8VaeIdaMUdaaea8PaeId8KMUd8KaeaRaeIdyMUdyaqaLaYcdtfydbc8S2fgea8UaeIdbMUdbaea8XaeIdlMUdlaea80aeIdwMUdwaea8YaeIdxMUdxaea81aeIdzMUdzaea8ZaeIdCMUdCaea8SaeIdKMUdKaeaIaeId3MUd3aea8VaeIdaMUdaaea8PaeId8KMUd8KaeaRaeIdyMUdyaHcxfhHaAcifgAad6mbkcbh8FabhXinaba8FcdtfhQcbhHinaKaQaHc;a1jjbfydbcdtfydbgOfRbbhedndnaKaXaHfydbgCfRbbgAc99fcFeGcpe0mbaec99fcFeGc;:e6mekdnaAcufcFeGce0mbahaCcdtfydbaO9hmekdnaecufcFeGce0mbagaOcdtfydbaC9hmekdnaAcv2aefc:G1jjbfRbbTmbaLaOcdtfydbaLaCcdtfydb0mekJbbacJbbacJbbjZaecFeGceSEaAceSEh8ZdnaaaQaHc;e1jjbfydbcdtfydbcx2fgeIdwaaaCcx2fgAIdwg8R:tg8VaaaOcx2fgYIdwa8R:tg8Sa8SNaYIdbaAIdbg8W:tgIaINaYIdlaAIdlg8U:tgRaRNMMg8PNa8Va8SNaeIdba8W:tg80aINaRaeIdla8U:tg8YNMMg8Xa8SN:tg8Va8VNa80a8PNa8XaIN:tg8Sa8SNa8Ya8PNa8XaRN:tgIaINMM:rgRJbbbb9ETmba8VaR:vh8VaIaR:vhIa8SaR:vh8SkaqaLaCcdtfydbc8S2fgea8Sa8Za8P:rNgRa8SNNg8XaeIdbMUdbaeaIaRaINg8ZNg80aeIdlMUdlaea8VaRa8VNg8PNg8YaeIdwMUdwaea8Za8SNg8ZaeIdxMUdxaea8Pa8SNg81aeIdzMUdzaea8PaINgBaeIdCMUdCaea8SaRa8Va8RNa8Sa8WNa8UaINMM:mg8RNg8PNg8SaeIdKMUdKaeaIa8PNgIaeId3MUd3aea8Va8PNg8VaeIdaMUdaaea8Pa8RNg8PaeId8KMUd8KaeaRaeIdyMUdyaqaLaOcdtfydbc8S2fgea8XaeIdbMUdbaea80aeIdlMUdlaea8YaeIdwMUdwaea8ZaeIdxMUdxaea81aeIdzMUdzaeaBaeIdCMUdCaea8SaeIdKMUdKaeaIaeId3MUd3aea8VaeIdaMUdaaea8PaeId8KMUd8KaeaRaeIdyMUdykaHclfgHcx9hmbkaXcxfhXa8Fcifg8Fad6mbkdna8JTmbcbhXinJbbbbh8WaaabaXcdtfgeclfydbgYcx2fgHIdwaaaeydbg8Fcx2fgOIdwg8Y:tgIaINaHIdbaOIdbg81:tg8Va8VNaHIdlaOIdlgB:tgRaRNMMg8Zaaaecwfydbg3cx2fgeIdwa8Y:tg8PNaIaIa8PNa8VaeIdba81:tg8RNaRaeIdlaB:tg8UNMMg8SN:tJbbbbJbbjZa8Za8Pa8PNa8Ra8RNa8Ua8UNMMg80Na8Sa8SN:tg8X:va8XJbbbb9BEg8XNh83a80aINa8Pa8SN:ta8XNhUa8Za8UNaRa8SN:ta8XNh85a80aRNa8Ua8SN:ta8XNh86a8Za8RNa8Va8SN:ta8XNh87a80a8VNa8Ra8SN:ta8XNh88a8Va8UNa8RaRN:tg8Sa8SNaRa8PNa8UaIN:tg8Sa8SNaIa8RNa8Pa8VN:tg8Sa8SNMM:rJbbbZNh8Saya8Fa8J2gwcdtfhHaya3a8J2g8LcdtfhOayaYa8J2gicdtfhCa8Y:mh89aB:mh8:a81:mhZcbhAa8JhQJbbbbh8UJbbbbh8XJbbbbh8ZJbbbbh80Jbbbbh8YJbbbbh81JbbbbhBJbbbbhnJbbbbhcinasc;WbfaAfgecwfa8SaUaCIdbaHIdbg8P:tgRNa83aOIdba8P:tg8RNMgINUdbaeclfa8Sa86aRNa85a8RNMg8VNUdbaea8Sa88aRNa87a8RNMgRNUdbaecxfa8Sa89aINa8:a8VNa8PaZaRNMMMg8PNUdba8SaIa8VNNa80Mh80a8SaIaRNNa8YMh8Ya8Sa8VaRNNa81Mh81a8Sa8Pa8PNNa8WMh8Wa8SaIa8PNNa8UMh8Ua8Sa8Va8PNNa8XMh8Xa8SaRa8PNNa8ZMh8Za8SaIaINNaBMhBa8Sa8Va8VNNanMhna8SaRaRNNacMhcaHclfhHaCclfhCaOclfhOaAczfhAaQcufgQmbkava8Fc8S2fgeacaeIdbMUdbaeanaeIdlMUdlaeaBaeIdwMUdwaea81aeIdxMUdxaea8YaeIdzMUdzaea80aeIdCMUdCaea8ZaeIdKMUdKaea8XaeId3MUd3aea8UaeIdaMUdaaea8WaeId8KMUd8Kaea8SaeIdyMUdyavaYc8S2fgeacaeIdbMUdbaeanaeIdlMUdlaeaBaeIdwMUdwaea81aeIdxMUdxaea8YaeIdzMUdzaea80aeIdCMUdCaea8ZaeIdKMUdKaea8XaeId3MUd3aea8UaeIdaMUdaaea8WaeId8KMUd8Kaea8SaeIdyMUdyava3c8S2fgeacaeIdbMUdbaeanaeIdlMUdlaeaBaeIdwMUdwaea81aeIdxMUdxaea8YaeIdzMUdzaea80aeIdCMUdCaea8ZaeIdKMUdKaea8XaeId3MUd3aea8UaeIdaMUdaaea8WaeId8KMUd8Kaea8SaeIdyMUdyarawcltfhQcbhHa8JhCinaQaHfgeasc;WbfaHfgOIdbaeIdbMUdbaeclfgAaOclfIdbaAIdbMUdbaecwfgAaOcwfIdbaAIdbMUdbaecxfgeaOcxfIdbaeIdbMUdbaHczfhHaCcufgCmbkaraicltfhQcbhHa8JhCinaQaHfgeasc;WbfaHfgOIdbaeIdbMUdbaeclfgAaOclfIdbaAIdbMUdbaecwfgAaOcwfIdbaAIdbMUdbaecxfgeaOcxfIdbaeIdbMUdbaHczfhHaCcufgCmbkara8LcltfhQcbhHa8JhCinaQaHfgeasc;WbfaHfgOIdbaeIdbMUdbaeclfgAaOclfIdbaAIdbMUdbaecwfgAaOcwfIdbaAIdbMUdbaecxfgeaOcxfIdbaeIdbMUdbaHczfhHaCcufgCmbkaXcifgXad6mbkkcbhOxekcehOcbhrkcbh3dndnamcwGg9cmbJbbbbh8UcbhJcbhocbhCxekcbhea5cbyd:m:jjjbHjjjjbbhCascxfasyd2gHcdtfaCBdbasaHcefBd2dnalTmbaChHinaHaeBdbaHclfhHalaecefge9hmbkkdnaOmbcbh8Finaba8FcdtfhYcbhXinaLaYaXcdtgec;a1jjbfydbcdtfydbcdtfydbhHdnaCaLaYaefydbcdtfydbgOcdtfgAydbgeaOSmbinaAaCaegOcdtfgQydbgeBdbaQhAaOae9hmbkkdnaCaHcdtfgAydbgeaHSmbinaAaCaegHcdtfgQydbgeBdbaQhAaHae9hmbkkdnaOaHSmbaCaOaHaOaH0EcdtfaOaHaOaH6EBdbkaXcefgXci9hmbka8Fcifg8Fad6mbkkcbhJdnalTmbcbhQindnaLaQcdtgefydbaQ9hmbaQhHdnaCaefgXydbgeaQSmbaXhOinaOaCaegHcdtfgAydbgeBdbaAhOaHae9hmbkkaXaHBdbkaQcefgQal9hmbkcbheaLhOaChHcbhJindndnaeaOydbgA9hmbdnaeaHydbgA9hmbaHaJBdbaJcefhJxdkaHaCaAcdtfydbBdbxekaHaCaAcdtfydbBdbkaOclfhOaHclfhHalaecefge9hmbkkcuaJcltgeaJcjjjjiGEcbyd:m:jjjbHjjjjbbhoascxfasyd2gHcdtfaoBdbasaHcefBd2aocbaez:ljjjbhAdnalTmbaChOaahealhQinaecwfIdbh8SaeclfIdbhIaAaOydbcltfgHaeIdbaHIdbMUdbaHclfgXaIaXIdbMUdbaHcwfgXa8SaXIdbMUdbaHcxfgHaHIdbJbbjZMUdbaOclfhOaecxfheaQcufgQmbkkdnaJTmbaAheaJhHinaecxfgOIdbh8SaOcbBdbaeaeIdbJbbbbJbbjZa8S:va8SJbbbb9BEg8SNUdbaeclfgOa8SaOIdbNUdbaecwfgOa8SaOIdbNUdbaeczfheaHcufgHmbkkdnalTmbaChOaahealhQinaAaOydbcltfgHcxfgXaecwfIdbaHcwfIdb:tg8Sa8SNaeIdbaHIdb:tg8Sa8SNaeclfIdbaHclfIdb:tg8Sa8SNMMg8SaXIdbgIaIa8S9DEUdbaOclfhOaecxfheaQcufgQmbkkdnaJmbcbhJJFFuuh8UxekaAcxfheaAhHaJhOinaHaeIdbUdbaeczfheaHclfhHaOcufgOmbkJFFuuh8UaAheaJhHinaeIdbg8Sa8Ua8Ua8S9EEh8UaeclfheaHcufgHmbkkasydlh9ednalTmba9eclfhea9eydbhAaKhHalhQcbhOincbaeydbgXaA9RaHRbbcpeGEaOfhOaHcefhHaeclfheaXhAaQcufgQmbkaOce4h3kcuada39RcifgTcx2aTc;v:Q;v:Qe0Ecbyd:m:jjjbHjjjjbbhDascxfasyd2gecdtfaDBdbasaecefBd2cuaTcdtaTcFFFFi0Ecbyd:m:jjjbHjjjjbbhSascxfasyd2gecdtfaSBdbasaecefBd2a5cbyd:m:jjjbHjjjjbbh8Mascxfasyd2gecdtfa8MBdbasaecefBd2alcbyd:m:jjjbHjjjjbbh9hascxfasyd2gecdtfa9hBdbasaecefBd2axaxNa8NJbbjZamclGEg83a83N:vhcJbbbbhndnadak9nmbdnaTci6mba8Jclth9iaDcwfh6JbbbbhBJbbbbhninasclfabadalaLz:cjjjbabh3cbhEcbh5inaba5cdtfhwcbheindnaLa3aefydbgOcdtg8FfydbgQaLawaec;q1jjbfydbcdtfydbgHcdtg8LfydbgXSmbaKaHfRbbgYcv2aKaOfRbbgAfc;G1jjbfRbbg8AaAcv2aYfgic;G1jjbfRbbg8KVcFeGTmbdnaXaQ9nmbaic:G1jjbfRbbcFeGmekaAcufhQdnaAaY9hmbaQcFeGce0mbaha8FfydbaH9hmekdndnaAclSmbaYcl9hmekdnaQcFeGce0mbaha8FfydbaH9hmdkaYcufcFeGce0mbaga8LfydbaO9hmekaDaEcx2fgAaHaOa8KcFeGgQEBdlaAaOaHaQEBdbaAaQa8AGcb9hBdwaEcefhEkaeclfgecx9hmbkdna5cifg5ad9pmba3cxfh3aEcifaT9nmekkaETmdcbhYinaqaLaDaYcx2fgAydbgQcdtg3fydbc8S2fgeIdwaaaAydlgXcx2fgHIdwg8VNaeIdzaHIdbgRNaeIdaMg8Sa8SMMa8VNaeIdlaHIdlg8PNaeIdCa8VNaeId3Mg8Sa8SMMa8PNaeIdbaRNaeIdxa8PNaeIdKMg8Sa8SMMaRNaeId8KMMM:lh8SJbbbbJbbjZaeIdygI:vaIJbbbb9BEhIdndnaAydwg8FmbJFFuuh8XxekJbbbbJbbjZaqaLaXcdtfydbc8S2fgeIdyg8R:va8RJbbbb9BEaeIdwaaaQcx2fgHIdwg8RNaeIdzaHIdbg8WNaeIdaMg8Xa8XMMa8RNaeIdlaHIdlg8XNaeIdCa8RNaeId3Mg8Ra8RMMa8XNaeIdba8WNaeIdxa8XNaeIdKMg8Ra8RMMa8WNaeId8KMMM:lNh8XkaIa8SNh8Zdna8JTmbavaQc8S2fgOIdwa8VNaOIdzaRNaOIdaMg8Sa8SMMa8VNaOIdla8PNaOIdCa8VNaOId3Mg8Sa8SMMa8PNaOIdbaRNaOIdxa8PNaOIdKMg8Sa8SMMaRNaOId8KMMMh8SayaXa8J2gwcdtfhHaraQa8J2g8LcltfheaOIdyh8Ra8JhOinaHIdbgIaIa8RNaecxfIdba8VaecwfIdbNaRaeIdbNa8PaeclfIdbNMMMgIaIM:tNa8SMh8SaHclfhHaeczfheaOcufgOmbkdndna8FmbJbbbbhIxekavaXc8S2fgOIdwaaaQcx2fgeIdwgRNaOIdzaeIdbg8PNaOIdaMgIaIMMaRNaOIdlaeIdlg8RNaOIdCaRNaOId3MgIaIMMa8RNaOIdba8PNaOIdxa8RNaOIdKMgIaIMMa8PNaOId8KMMMhIaya8LcdtfhHarawcltfheaOIdyh8Wa8JhOinaHIdbg8Va8Va8WNaecxfIdbaRaecwfIdbNa8PaeIdbNa8RaeclfIdbNMMMg8Va8VM:tNaIMhIaHclfhHaeczfheaOcufgOmbkaI:lhIka8Za8S:lMh8Za8XaIMh8XaKaQfRbbcd9hmbdnagahaha3fydbaXSEa8Ea3fydbgwcdtfydbg3cu9hmba8EaXcdtfydbh3kavawc8S2fgOIdwaaa3cx2fgeIdwg8VNaOIdzaeIdbgRNaOIdaMg8Sa8SMMa8VNaOIdlaeIdlg8PNaOIdCa8VNaOId3Mg8Sa8SMMa8PNaOIdbaRNaOIdxa8PNaOIdKMg8Sa8SMMaRNaOId8KMMMh8Saya3a8J2g8LcdtfhHarawa8J2gicltfheaOIdyh8Ra8JhOinaHIdbgIaIa8RNaecxfIdba8VaecwfIdbNaRaeIdbNa8PaeclfIdbNMMMgIaIM:tNa8SMh8SaHclfhHaeczfheaOcufgOmbkdndna8FmbJbbbbhIxekava3c8S2fgOIdwaaawcx2fgeIdwgRNaOIdzaeIdbg8PNaOIdaMgIaIMMaRNaOIdlaeIdlg8RNaOIdCaRNaOId3MgIaIMMa8RNaOIdba8PNaOIdxa8RNaOIdKMgIaIMMa8PNaOId8KMMMhIayaicdtfhHara8LcltfheaOIdyh8Wa8JhOinaHIdbg8Va8Va8WNaecxfIdbaRaecwfIdbNa8PaeIdbNa8RaeclfIdbNMMMg8Va8VM:tNaIMhIaHclfhHaeczfheaOcufgOmbkaI:lhIka8Za8S:lMh8Za8XaIMh8XkaAa8Za8Xa8Za8X9FgeEUdwaAaXaQaea8FTVgeEBdlaAaQaXaeEBdbaYcefgYaE9hmbkasc;Wbfcbcj;qbz:ljjjb8Aa6heaEhHinasc;WbfaeydbcA4cF8FGgOcFAaOcFA6EcdtfgOaOydbcefBdbaecxfheaHcufgHmbkcbhecbhHinasc;WbfaefgOydbhAaOaHBdbaAaHfhHaeclfgecj;qb9hmbkcbhea6hHinasc;WbfaHydbcA4cF8FGgOcFAaOcFA6EcdtfgOaOydbgOcefBdbaSaOcdtfaeBdbaHcxfhHaEaecefge9hmbkadak9RgOci9Uh9kdnalTmbcbhea8MhHinaHaeBdbaHclfhHalaecefge9hmbkkcbh0a9hcbalz:ljjjbh5aOcO9Uh9ma9kce4h9nasydwh9ocbh8Kcbh8AdninaDaSa8Acdtfydbcx2fgiIdwg8Sac9Emea8Ka9k9pmeJFFuuhIdna9naE9pmbaDaSa9ncdtfydbcx2fIdwJbb;aZNhIkdna8SaI9ETmba8San9ETmba8Ka9m0mdkdna5aLaiydlgwcdtg9pfydbgAfg9qRbba5aLaiydbg3cdtg9rfydbgefg9sRbbVmbaKa3fRbbh9tdna9eaecdtfgHclfydbgOaHydbgHSmbaOaH9RhQaaaAcx2fhYaaaecx2fh8Fa9oaHcitfhecbhHceh8Ldnindna8MaeydbcdtfydbgOaASmba8MaeclfydbcdtfydbgXaASmbaOaXSmbaaaXcx2fgXIdbaaaOcx2fgOIdbg8V:tg8Sa8FIdlaOIdlgR:tg8WNa8FIdba8V:tg8XaXIdlaR:tgIN:tg8Pa8SaYIdlaR:tg8ZNaYIdba8V:tg80aIN:tgRNaIa8FIdwaOIdwg8R:tg8YNa8WaXIdwa8R:tg8VN:tg8WaIaYIdwa8R:tg81Na8Za8VN:tgINa8Va8XNa8Ya8SN:tg8Ra8Va80Na81a8SN:tg8SNMMa8Pa8PNa8Wa8WNa8Ra8RNMMaRaRNaIaINa8Sa8SNMMN:rJbbj8:N9FmdkaecwfheaHcefgHaQ6h8LaQaH9hmbkka8LceGTmba9ncefh9nxekdndndndna9tc9:fPdebdka3heina8MaecdtgefawBdba8Eaefydbgea39hmbxikkdnagahaha9rfydbawSEa8Ea9rfydbg3cdtfydbgecu9hmba8Ea9pfydbheka8Ma9rfawBdbaehwka8Ma3cdtfawBdbka9sce86bba9qce86bbaiIdwg8Sanana8S9DEhna0cefh0cecda9tceSEa8Kfh8Kka8Acefg8AaE9hmbkka0TmddnalTmbcbhXcbh8Findna8Ma8FcdtgefydbgOa8FSmbaLaOcdtfydbh3dna8FaLaefydb9hgwmbaqa3c8S2fgeaqa8Fc8S2fgHIdbaeIdbMUdbaeaHIdlaeIdlMUdlaeaHIdwaeIdwMUdwaeaHIdxaeIdxMUdxaeaHIdzaeIdzMUdzaeaHIdCaeIdCMUdCaeaHIdKaeIdKMUdKaeaHId3aeId3MUd3aeaHIdaaeIdaMUdaaeaHId8KaeId8KMUd8KaeaHIdyaeIdyMUdyka8JTmbavaOc8S2fgeava8Fc8S2g8LfgHIdbaeIdbMUdbaeaHIdlaeIdlMUdlaeaHIdwaeIdwMUdwaeaHIdxaeIdxMUdxaeaHIdzaeIdzMUdzaeaHIdCaeIdCMUdCaeaHIdKaeIdKMUdKaeaHId3aeId3MUd3aeaHIdaaeIdaMUdaaeaHId8KaeId8KMUd8KaeaHIdyaeIdyMUdya9iaO2hYarhHa8JhAinaHaYfgeaHaXfgOIdbaeIdbMUdbaeclfgQaOclfIdbaQIdbMUdbaecwfgQaOcwfIdbaQIdbMUdbaecxfgeaOcxfIdbaeIdbMUdbaHczfhHaAcufgAmbkawmbJbbbbJbbjZaqa8LfgeIdyg8S:va8SJbbbb9BEaeIdwaaa3cx2fgHIdwg8SNaeIdzaHIdbgINaeIdaMg8Va8VMMa8SNaeIdlaHIdlg8VNaeIdCa8SNaeId3Mg8Sa8SMMa8VNaeIdbaINaeIdxa8VNaeIdKMg8Sa8SMMaINaeId8KMMM:lNg8SaBaBa8S9DEhBkaXa9ifhXa8Fcefg8Fal9hmbkcbhHahheindnaeydbgOcuSmbdnaHa8MaOcdtgAfydbgO9hmbcuhOahaAfydbgAcuSmba8MaAcdtfydbhOkaeaOBdbkaeclfhealaHcefgH9hmbkcbhHagheindnaeydbgOcuSmbdnaHa8MaOcdtgAfydbgO9hmbcuhOagaAfydbgAcuSmba8MaAcdtfydbhOkaeaOBdbkaeclfhealaHcefgH9hmbkkaBana8JEhBcbhAabhecbhQindna8MaeydbcdtfydbgHa8MaeclfydbcdtfydbgOSmbaHa8MaecwfydbcdtfydbgXSmbaOaXSmbabaAcdtfgYaHBdbaYcwfaXBdbaYclfaOBdbaAcifhAkaecxfheaQcifgQad6mbkdndna9cTmbaAak9nmba8UaB9FTmbcbhdabhecbhHindnaoaCaeydbgOcdtfydbcdtfIdbaB9ETmbabadcdtfgQaOBdbaQclfaeclfydbBdbaQcwfaecwfydbBdbadcifhdkaecxfheaHcifgHaA6mbkJFFuuh8UaJTmeaoheaJhHJFFuuh8SinaeIdbgIa8Sa8SaI9EEg8Va8SaIaB9EgOEh8Sa8Va8UaOEh8UaeclfheaHcufgHmbxdkkaAhdkadak0mbxdkkasclfabadalaLz:cjjjbkdndnadak0mbadhOxekdna9cmbadhOxekdna8Uac9FmbadhOxekina8UJbb;aZNg8Saca8Sac9DEh8VJbbbbh8SdnaJTmbaoheaJhHinaeIdbgIa8SaIa8V9FEa8SaIa8S9EEh8SaeclfheaHcufgHmbkkcbhOabhecbhHindnaoaCaeydbgAcdtfydbcdtfIdba8V9ETmbabaOcdtfgQaABdbaQclfaeclfydbBdbaQcwfaecwfydbBdbaOcifhOkaecxfheaHcifgHad6mbkJFFuuh8UdnaJTmbaoheaJhHJFFuuhIinaeIdbgRaIaIaR9EEg8PaIaRa8V9EgAEhIa8Pa8UaAEh8UaeclfheaHcufgHmbkkdnaOad9hmbadhOxdka8Sanana8S9DEhnaOak9nmeaOhda8Uac9FmbkkdnamcjjjjlGTmbazmbaOTmbcbhLabheinaKaeydbgAfRbbc3thXaecwfgYydbhHdndnahaAcdtg3fydbaeclfg8FydbgCSmbcbhQagaCcdtfydbaA9hmekcjjjj94hQkaeaXaQVaAVBdbaKaCfRbbc3thXdndnahaCcdtfydbaHSmbcbhQagaHcdtfydbaC9hmekcjjjj94hQka8FaXaQVaCVBdbaKaHfRbbc3thQdndnahaHcdtfydbaASmbcbhCaga3fydbaH9hmekcjjjj94hCkaYaQaCVaHVBdbaecxfheaLcifgLaO6mbkkdnazTmbaOTmbaOheinabazabydbcdtfydbBdbabclfhbaecufgembkkdnaPTmbaPa83an:rNUdbkasyd2gecdtascxffc98fhHdninaeTmeaHydbcbyd1:jjjbH:bjjjbbaHc98fhHaecufhexbkkasc;W;qbf8KjjjjbaOk;Yieouabydlhvabydbclfcbaicdtz:ljjjbhoadci9UhrdnadTmbdnalTmbaehwadhDinaoalawydbcdtfydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbxdkkaehwadhDinaoawydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbkkdnaiTmbcbhDaohwinawydbhqawaDBdbawclfhwaqaDfhDaicufgimbkkdnadci6mbinaecwfydbhwaeclfydbhDaeydbhidnalTmbalawcdtfydbhwalaDcdtfydbhDalaicdtfydbhikavaoaicdtfgqydbcitfaDBdbavaqydbcitfawBdlaqaqydbcefBdbavaoaDcdtfgqydbcitfawBdbavaqydbcitfaiBdlaqaqydbcefBdbavaoawcdtfgwydbcitfaiBdbavawydbcitfaDBdlawawydbcefBdbaecxfhearcufgrmbkkabydbcbBdbk;Qodvuv998Jjjjjbca9Rgvczfcwfcbyd11jjbBdbavcb8Pdj1jjb83izavcwfcbydN1jjbBdbavcb8Pd:m1jjb83ibdnadTmbaicd4hodnabmbdnalTmbcbhrinaealarcdtfydbao2cdtfhwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkarcefgrad9hmbxikkaocdthrcbhwincbhiinavczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbxdkkdnalTmbcbhrinabarcx2fgiaealarcdtfydbao2cdtfgwIdbUdbaiawIdlUdlaiawIdwUdwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkarcefgrad9hmbxdkkaocdthlcbhraehwinabarcx2fgiaearao2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinavczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawalfhwarcefgrad9hmbkkJbbbbavIdbavIdzgk:tgqaqJbbbb9DEgqavIdlavIdCgx:tgmamaq9DEgqavIdwavIdKgm:tgPaPaq9DEhPdnabTmbadTmbJbbbbJbbjZaP:vaPJbbbb9BEhqinabaqabIdbak:tNUdbabclfgvaqavIdbax:tNUdbabcwfgvaqavIdbam:tNUdbabcxfhbadcufgdmbkkaPk8MbabaeadaialavcbcbcbcbcbaoarawaDz:bjjjbk8MbabaeadaialavaoarawaDaqakaxamaPz:bjjjbk:DCoDud99rue99iul998Jjjjjbc;Wb9Rgw8KjjjjbdndnarmbcbhDxekawcxfcbc;Kbz:ljjjb8Aawcuadcx2adc;v:Q;v:Qe0Ecbyd:m:jjjbHjjjjbbgqBdxawceBd2aqaeadaicbz:djjjb8AawcuadcdtadcFFFFi0Egkcbyd:m:jjjbHjjjjbbgxBdzawcdBd2adcd4adfhmceheinaegicetheaiam6mbkcbhPawcuaicdtgsaicFFFFi0Ecbyd:m:jjjbHjjjjbbgzBdCawciBd2dndnar:ZgH:rJbbbZMgO:lJbbb9p9DTmbaO:Ohexekcjjjj94hekaicufhAc:bwhmcbhCadhXcbhQinaChLaeamgKcufaeaK9iEaPgDcefaeaD9kEhYdndnadTmbaYcuf:YhOaqhiaxheadhmindndnaiIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcCthCdndnaiclfIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhExekcjjjj94hEkaEcqtaCVhCdndnaicwfIdbaONJbbbZMg8A:lJbbb9p9DTmba8A:OhExekcjjjj94hEkaeaCaEVBdbaicxfhiaeclfheamcufgmmbkazcFeasz:ljjjbh3cbh5cbhPindna3axaPcdtfydbgCcm4aC7c:v;t;h;Ev2gics4ai7aAGgmcdtfgEydbgecuSmbaeaCSmbcehiina3amaifaAGgmcdtfgEydbgecuSmeaicefhiaeaC9hmbkkaEaCBdba5aecuSfh5aPcefgPad9hmbxdkkazcFeasz:ljjjb8Acbh5kaDaYa5ar0giEhPaLa5aiEhCdna5arSmbaYaKaiEgmaP9Rcd9imbdndnaQcl0mbdnaX:ZgOaL:Zg8A:taY:Yg8EaD:Y:tg8Fa8EaK:Y:tgaa5:ZghaH:tNNNaOaH:taaNa8Aah:tNa8AaH:ta8FNahaO:tNM:va8EMJbbbZMgO:lJbbb9p9DTmbaO:Ohexdkcjjjj94hexekaPamfcd9Theka5aXaiEhXaQcefgQcs9hmekkdndnaCmbcihicbhDxekcbhiawakcbyd:m:jjjbHjjjjbbg5BdKawclBd2aPcuf:Yh8AdndnadTmbaqhiaxheadhmindndnaiIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhCxekcjjjj94hCkaCcCthCdndnaiclfIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhExekcjjjj94hEkaEcqtaCVhCdndnaicwfIdba8ANJbbbZMgO:lJbbb9p9DTmbaO:OhExekcjjjj94hEkaeaCaEVBdbaicxfhiaeclfheamcufgmmbkazcFeasz:ljjjbh3cbhDcbhYindndndna3axaYcdtgKfydbgCcm4aC7c:v;t;h;Ev2gics4ai7aAGgmcdtfgEydbgecuSmbcehiinaxaecdtgefydbaCSmdamaifheaicefhia3aeaAGgmcdtfgEydbgecu9hmbkkaEaYBdbaDhiaDcefhDxeka5aefydbhika5aKfaiBdbaYcefgYad9hmbkcuaDc32giaDc;j:KM;jb0EhexekazcFeasz:ljjjb8AcbhDcbhekawaecbyd:m:jjjbHjjjjbbgeBd3awcvBd2aecbaiz:ljjjbhEavcd4hKdnadTmbdnalTmbaKcdth3a5hCaqhealhmadhAinaEaCydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiamIdbaiIdxMUdxaiamclfIdbaiIdzMUdzaiamcwfIdbaiIdCMUdCaiaiIdKJbbjZMUdKaCclfhCaecxfheama3fhmaAcufgAmbxdkka5hmaqheadhCinaEamydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiaiIdxJbbbbMUdxaiaiIdzJbbbbMUdzaiaiIdCJbbbbMUdCaiaiIdKJbbjZMUdKamclfhmaecxfheaCcufgCmbkkdnaDTmbaEhiaDheinaiaiIdbJbbbbJbbjZaicKfIdbgO:vaOJbbbb9BEgONUdbaiclfgmaOamIdbNUdbaicwfgmaOamIdbNUdbaicxfgmaOamIdbNUdbaiczfgmaOamIdbNUdbaicCfgmaOamIdbNUdbaic3fhiaecufgembkkcbhCawcuaDcdtgYaDcFFFFi0Egicbyd:m:jjjbHjjjjbbgeBdaawcoBd2awaicbyd:m:jjjbHjjjjbbg3Bd8KaecFeaYz:ljjjbhxdnadTmbJbbjZJbbjZa8A:vaPceSEaoNgOaONh8AaKcdthPalheina8Aaec;81jjbalEgmIdwaEa5ydbgAc32fgiIdC:tgOaONamIdbaiIdx:tgOaONamIdlaiIdz:tgOaONMMNaqcwfIdbaiIdw:tgOaONaqIdbaiIdb:tgOaONaqclfIdbaiIdl:tgOaONMMMhOdndnaxaAcdtgifgmydbcuSmba3aifIdbaO9ETmekamaCBdba3aifaOUdbka5clfh5aqcxfhqaeaPfheadaCcefgC9hmbkkabaxaYz:kjjjb8AcrhikaicdthiinaiTmeaic98fgiawcxffydbcbyd1:jjjbH:bjjjbbxbkkawc;Wbf8KjjjjbaDk:Ydidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgvaDIdbgoaoav9EEUdbaladfgDavaDIdbgoaoav9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkJbbbbavavJbbbb9DEgvaoaoav9DEgvararav9DEk9DeeuabcFeaicdtz:ljjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcifc98GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;teeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiaeydlBdlaiaeydwBdwaiaeydxBdxaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk:3eedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdxaialBdwaialBdlaialBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcrfc94GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd:q:jjjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd:q:jjjbfgdBd:q:jjjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:Iedbcjwk1eFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbeeebeebebbeeebebbbbbebebbbbbbbbbebbbdbbbbbbbebbbebbbdbbbbbbbbbbbeeeeebebbebbebebbbeebbbbbbbbbbbbbbbbbbbbbc1Dkxebbbdbbb:GNbb'; // embed! wasm

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

		// set this to true to be able to use simplifyPoints and simplifyWithAttributes
		// note that these functions are experimental and may change interface/behavior in a way that will require revising calling code
		useExperimentalFeatures: false,

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
				assert(this.useExperimentalFeatures || flags[i] != 'Prune'); // set useExperimentalFeatures to use experimental flags like Prune
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
			assert(this.useExperimentalFeatures); // set useExperimentalFeatures to use this; note that this function is experimental and may change interface in a way that will require revising calling code
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
			assert(this.useExperimentalFeatures); // set useExperimentalFeatures to use this; note that this function is experimental and may change interface in a way that will require revising calling code
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
	};
})();

export { MeshoptSimplifier };
