// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2024, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function() {
	"use strict";
	// Built with clang version 18.1.2
	// Built from meshoptimizer 0.21
	var wasm = "b9H79Tebbbe9Hk9Geueu9Geub9Gbb9Gsuuuuuuuuuuuu99uueu9Gvuuuuub9Gvuuuuue999Gquuuuuuu99uueu9Gwuuuuuu99ueu9Giuuue999Gluuuueu9GiuuueuizsdilvoirwDbqqbeqlve9Weiiviebeoweuecj;jekr:Tewo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95bl8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bvQ9TW79O9V9Wt9F79P9T9W29P9M959q9V9P9Ut7boX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbra9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbwl79IV9RbDDwebcekdmxq:f97sdbk:39si8Au8A99zu8Jjjjjbc;W;ab9Rgs8Kjjjjbcbhzascxfcbc;Kbz:ljjjb8AdnabaeSmbabaeadcdtz:kjjjb8AkdndnamcdGmbcbhHxekasalcrfci4gecbyd;S1jjbHjjjjbbgOBdxasceBd2aOcbaez:ljjjbhAcbhlcbhednadTmbcbhlabheadhOinaAaeydbgCci4fgXaXRbbgXceaCcrGgCtV86bbaXcu7aC4ceGalfhlaeclfheaOcufgOmbkcualcdtalcFFFFi0Ehekasaecbyd;S1jjbHjjjjbbgHBdzascdBd2alcd4alfhCcehOinaOgecethOaeaC6mbkcdhzcbhQascuaecdtgOaecFFFFi0Ecbyd;S1jjbHjjjjbbgCBdCasciBd2aCcFeaOz:ljjjbhLdnadTmbaecufhXcbhKinabaQcdtfgYydbgAc:v;t;h;Ev2hCcbhedndninaLaCaXGgCcdtfg8AydbgOcuSmeaHaOcdtfydbaASmdaecefgeaCfhCaeaX9nmbxdkkaHaKcdtfaABdba8AaKBdbaKhOaKcefhKkaYaOBdbaQcefgQad9hmbkkaLcbyd;O1jjbH:bjjjbbascdBd2kascxfazcdtfcualcefgecdtaecFFFFi0Ecbyd;S1jjbHjjjjbbgEBdbasaEBdlasazceVgeBd2ascxfaecdtfcuadcitadcFFFFe0Ecbyd;S1jjbHjjjjbbg3Bdbasa3BdwasazcdfgeBd2asclfabadalcbz:cjjjbascxfaecdtfcualcdtg5alcFFFFi0Eg8Ecbyd;S1jjbHjjjjbbgOBdbasazcifgeBd2ascxfaecdtfa8Ecbyd;S1jjbHjjjjbbg8FBdbasazclVgaBd2alcd4alfhXcehCinaCgecethCaeaX6mbkcbhKascxfaacdtfghcuaecdtgCaecFFFFi0Ecbyd;S1jjbHjjjjbbgXBdbasazcvVggBd2aXcFeaCz:ljjjbhQdnalTmbavcd4hAaecufhCinaKhednaHTmbaHaKcdtfydbhekaiaeaA2cdtfgeydlgXcH4aX7c:F:b:DD2aeydbgXcH4aX7c;D;O:B8J27aeydwgecH4ae7c:3F;N8N27aCGheaKcdth8JdndndndndnaHTmbaHa8JfhYcbhXinaQaecdtfgLydbg8AcuSmlaiaHa8AcdtfydbaA2cdtfaiaYydbaA2cdtfcxz:ojjjbTmiaXcefgXaefaCGheaXaC9nmbxdkkaiaKaA2cdtfhYcbhXinaQaecdtfgLydbg8AcuSmiaia8AaA2cdtfaYcxz:ojjjbTmdaXcefgXaefaCGheaXaC9nmbkkcbhLkaLydbgecu9hmekaLaKBdbaKhekaOa8JfaeBdbaKcefgKal9hmbkcbhea8FhCinaCaeBdbaCclfhCalaecefge9hmbkcbheaOhCa8FhXindnaeaCydbgASmbaXa8FaAcdtfgAydbBdbaAaeBdbkaCclfhCaXclfhXalaecefge9hmbkkcbh8KaQcbyd;O1jjbH:bjjjbbasaaBd2ahalcbyd;S1jjbHjjjjbbgABdbasagBd2ascxfagcdtfa8Ecbyd;S1jjbHjjjjbbgeBdbasazcofgCBd2ascxfaCcdtfa8Ecbyd;S1jjbHjjjjbbgCBdbasazcrfg8LBd2aecFea5z:ljjjbh8MaCcFea5z:ljjjbh8NdnalTmba3cwfhyindnaEa8KgXcefg8Kcdtfydbg8AaEaXcdtgefydbgCSmba8AaC9Rh8Ja3aCcitfh5a8Naefhga8MaefhKcbhLindndna5aLcitfydbgQaX9hmbaKaXBdbagaXBdbxekdnaEaQcdtgafgeclfydbgCaeydbgeSmba3aecitg8AfydbaXSmeaCae9Rhhaecu7aCfhYaya8AfhCcbheinaYaeSmeaecefheaCydbh8AaCcwfhCa8AaX9hmbkaeah6meka8NaafgeaXaQaeydbcuSEBdbaKaQaXaKydbcuSEBdbkaLcefgLa8J9hmbkka8Kal9hmbkaOhCaHhLa8FhXa8Nh8Aa8MhQcbheindndnaeaCydbgY9hmbdnaqTmbaehYdnaHTmbaLydbhYkaqaYfRbbTmbaAaefcl86bbxdkdnaeaXydbgY9hmbaQydbhYdna8AydbgKcu9hmbaYcu9hmbaAaefcb86bbxikaAaefh8JdnaeaKSmbaeaYSmba8Jce86bbxika8Jcl86bbxdkdnaea8FaYcdtgKfydb9hmbdna8Aydbg8JcuSmbaea8JSmbaQydbg5cuSmbaea5Smba8NaKfydbgacuSmbaaaYSmba8MaKfydbgKcuSmbaKaYSmbdnaOa8JcdtfydbaOaKcdtfydb9hmbaOa5cdtfydbaOaacdtfydb9hmbaAaefcd86bbxlkaAaefcl86bbxikaAaefcl86bbxdkaAaefcl86bbxekaAaefaAaYfRbb86bbkaCclfhCaLclfhLaXclfhXa8Aclfh8AaQclfhQalaecefge9hmbkamceGTmbaAhealhCindnaeRbbce9hmbaecl86bbkaecefheaCcufgCmbkkascxfa8Lcdtfcualcx2alc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbggBdbasazcwVg8JBd2agaialavaHz:djjjbh8PdndnaDmbcbhvxekascxfa8JcdtfcualaD2gecdtaecFFFFi0Ecbyd;S1jjbHjjjjbbgvBdbasazcDVg8JBd2alTmbarcd4hYdnaHTmbaDcdthKcbhLavhQinaoaHaLcdtfydbaY2cdtfheawhCaQhXaDh8AinaXaeIdbaCIdbNUdbaeclfheaCclfhCaXclfhXa8Acufg8AmbkaQaKfhQaLcefgLal9hmbxdkkaYcdthYaDcdthKcbhQavhLinaoheawhCaLhXaDh8AinaXaeIdbaCIdbNUdbaeclfheaCclfhCaXclfhXa8Acufg8AmbkaoaYfhoaLaKfhLaQcefgQal9hmbkkascxfa8Jcdtfcualc8S2gealc;D;O;f8U0EgXcbyd;S1jjbHjjjjbbgCBdbasa8Jcefg8ABd2aCcbaez:ljjjbh8KdndndnaDTmbascxfa8AcdtfaXcbyd;S1jjbHjjjjbbgqBdbasa8JcdfgCBd2aqcbaez:ljjjb8AascxfaCcdtfcualaD2gecltgCaecFFFFb0Ecbyd;S1jjbHjjjjbbgwBdbasa8JcifBd2awcbaCz:ljjjb8AadmexdkcbhqcbhwadTmekcbhLabhCindnagaCclfydbgQcx2fgeIdbagaCydbgYcx2fgXIdbgI:tg8RagaCcwfydbgKcx2fg8AIdlaXIdlg8S:tgRNa8AIdbaI:tg8UaeIdla8S:tg8VN:tg8Wa8WNa8Va8AIdwaXIdwg8X:tg8YNaRaeIdwa8X:tg8VN:tgRaRNa8Va8UNa8Ya8RN:tg8Ra8RNMM:rg8UJbbbb9ETmba8Wa8U:vh8Wa8Ra8U:vh8RaRa8U:vhRka8KaOaYcdtfydbc8S2fgeaRa8U:rg8UaRNNg8VaeIdbMUdbaea8Ra8Ua8RNg8ZNg8YaeIdlMUdlaea8Wa8Ua8WNg80Ng81aeIdwMUdwaea8ZaRNg8ZaeIdxMUdxaea80aRNgBaeIdzMUdzaea80a8RNg80aeIdCMUdCaeaRa8Ua8Wa8XNaRaINa8Sa8RNMM:mg8SNgINgRaeIdKMUdKaea8RaINg8RaeId3MUd3aea8WaINg8WaeIdaMUdaaeaIa8SNgIaeId8KMUd8Kaea8UaeIdyMUdya8KaOaQcdtfydbc8S2fgea8VaeIdbMUdbaea8YaeIdlMUdlaea81aeIdwMUdwaea8ZaeIdxMUdxaeaBaeIdzMUdzaea80aeIdCMUdCaeaRaeIdKMUdKaea8RaeId3MUd3aea8WaeIdaMUdaaeaIaeId8KMUd8Kaea8UaeIdyMUdya8KaOaKcdtfydbc8S2fgea8VaeIdbMUdbaea8YaeIdlMUdlaea81aeIdwMUdwaea8ZaeIdxMUdxaeaBaeIdzMUdzaea80aeIdCMUdCaeaRaeIdKMUdKaea8RaeId3MUd3aea8WaeIdaMUdaaeaIaeId8KMUd8Kaea8UaeIdyMUdyaCcxfhCaLcifgLad6mbkcbh8JabhYinaba8JcdtfhQcbhCinaAaQaCcj1jjbfydbcdtfydbgXfRbbhedndnaAaYaCfydbg8AfRbbgLc99fcFeGcpe0mbaeceSmbaecd9hmekdnaLcufcFeGce0mba8Ma8AcdtfydbaX9hmekdnaecufcFeGce0mba8NaXcdtfydba8A9hmekdnaLcv2aefc:q1jjbfRbbTmbaOaXcdtfydbaOa8Acdtfydb0mekdnagaXcx2fgKIdwaga8Acx2fgiIdwg8S:tgRaRNaKIdbaiIdbg8X:tg8Ra8RNaKIdlaiIdlg8V:tg8Ua8UNMM:rgIJbbbb9ETmbaRaI:vhRa8UaI:vh8Ua8RaI:vh8RkJbbacJbbacJbbjZaeceSEaLceSEh80dnagaQaCc:e1jjbfydbcdtfydbcx2fgeIdwa8S:tg8WaRa8WaRNaeIdba8X:tg81a8RNa8UaeIdla8V:tg8ZNMMg8YN:tg8Wa8WNa81a8Ra8YN:tgRaRNa8Za8Ua8YN:tg8Ra8RNMM:rg8UJbbbb9ETmba8Wa8U:vh8Wa8Ra8U:vh8RaRa8U:vhRka8KaOa8Acdtfydbc8S2fgeaRa80aINg8UaRNNg8YaeIdbMUdbaea8Ra8Ua8RNg80Ng81aeIdlMUdlaea8Wa8Ua8WNgINg8ZaeIdwMUdwaea80aRNg80aeIdxMUdxaeaIaRNgBaeIdzMUdzaeaIa8RNg83aeIdCMUdCaeaRa8Ua8Wa8SNaRa8XNa8Va8RNMM:mg8SNgINgRaeIdKMUdKaea8RaINg8RaeId3MUd3aea8WaINg8WaeIdaMUdaaeaIa8SNgIaeId8KMUd8Kaea8UaeIdyMUdya8KaOaXcdtfydbc8S2fgea8YaeIdbMUdbaea81aeIdlMUdlaea8ZaeIdwMUdwaea80aeIdxMUdxaeaBaeIdzMUdzaea83aeIdCMUdCaeaRaeIdKMUdKaea8RaeId3MUd3aea8WaeIdaMUdaaeaIaeId8KMUd8Kaea8UaeIdyMUdykaCclfgCcx9hmbkaYcxfhYa8Jcifg8Jad6mbkaDTmbcbhYinJbbbbh8XagabaYcdtfgeclfydbgKcx2fgCIdwagaeydbgicx2fgXIdwg8Z:tg8Ra8RNaCIdbaXIdbgB:tg8Wa8WNaCIdlaXIdlg83:tg8Ua8UNMMg80agaecwfydbg8Jcx2fgeIdwa8Z:tgINa8Ra8RaINa8WaeIdbaB:tg8SNa8UaeIdla83:tg8VNMMgRN:tJbbbbJbbjZa80aIaINa8Sa8SNa8Va8VNMMg81NaRaRN:tg8Y:va8YJbbbb9BEg8YNhUa81a8RNaIaRN:ta8YNh85a80a8VNa8UaRN:ta8YNh86a81a8UNa8VaRN:ta8YNh87a80a8SNa8WaRN:ta8YNh88a81a8WNa8SaRN:ta8YNh89a8Wa8VNa8Sa8UN:tgRaRNa8UaINa8Va8RN:tgRaRNa8Ra8SNaIa8WN:tgRaRNMM:r:rhRavaiaD2cdtfhCava8JaD2cdtfhXavaKaD2cdtfh8Aa8Z:mh8:a83:mhZaB:mhncbhLaDhQJbbbbh8VJbbbbh8YJbbbbh80Jbbbbh81Jbbbbh8ZJbbbbhBJbbbbh83JbbbbhcJbbbbh9cinasc;WbfaLfgecwfaRa85a8AIdbaCIdbgI:tg8UNaUaXIdbaI:tg8SNMg8RNUdbaeclfaRa87a8UNa86a8SNMg8WNUdbaeaRa89a8UNa88a8SNMg8UNUdbaecxfaRa8:a8RNaZa8WNaIana8UNMMMgINUdbaRa8Ra8WNNa81Mh81aRa8Ra8UNNa8ZMh8ZaRa8Wa8UNNaBMhBaRaIaINNa8XMh8XaRa8RaINNa8VMh8VaRa8WaINNa8YMh8YaRa8UaINNa80Mh80aRa8Ra8RNNa83Mh83aRa8Wa8WNNacMhcaRa8Ua8UNNa9cMh9caCclfhCa8Aclfh8AaXclfhXaLczfhLaQcufgQmbkaqaOaicdtfydbgCc8S2fgea9caeIdbMUdbaeacaeIdlMUdlaea83aeIdwMUdwaeaBaeIdxMUdxaea8ZaeIdzMUdzaea81aeIdCMUdCaea80aeIdKMUdKaea8YaeId3MUd3aea8VaeIdaMUdaaea8XaeId8KMUd8KaeaRaeIdyMUdyaqaOaKcdtfydbgKc8S2fgea9caeIdbMUdbaeacaeIdlMUdlaea83aeIdwMUdwaeaBaeIdxMUdxaea8ZaeIdzMUdzaea81aeIdCMUdCaea80aeIdKMUdKaea8YaeId3MUd3aea8VaeIdaMUdaaea8XaeId8KMUd8KaeaRaeIdyMUdyaqaOa8Jcdtfydbgic8S2fgea9caeIdbMUdbaeacaeIdlMUdlaea83aeIdwMUdwaeaBaeIdxMUdxaea8ZaeIdzMUdzaea81aeIdCMUdCaea80aeIdKMUdKaea8YaeId3MUd3aea8VaeIdaMUdaaea8XaeId8KMUd8KaeaRaeIdyMUdyawaCaD2cltfhQcbhCaDh8AinaQaCfgeasc;WbfaCfgXIdbaeIdbMUdbaeclfgLaXclfIdbaLIdbMUdbaecwfgLaXcwfIdbaLIdbMUdbaecxfgeaXcxfIdbaeIdbMUdbaCczfhCa8Acufg8AmbkawaKaD2cltfhQcbhCaDh8AinaQaCfgeasc;WbfaCfgXIdbaeIdbMUdbaeclfgLaXclfIdbaLIdbMUdbaecwfgLaXcwfIdbaLIdbMUdbaecxfgeaXcxfIdbaeIdbMUdbaCczfhCa8Acufg8AmbkawaiaD2cltfhQcbhCaDh8AinaQaCfgeasc;WbfaCfgXIdbaeIdbMUdbaeclfgLaXclfIdbaLIdbMUdbaecwfgLaXcwfIdbaLIdbMUdbaecxfgeaXcxfIdbaeIdbMUdbaCczfhCa8Acufg8AmbkaYcifgYad6mbkkasydlhJcbhednalTmbaJclfheaJydbh8AaAhCalhLcbhXincbaeydbgQa8A9RaCRbbcpeGEaXfhXaCcefhCaeclfheaQh8AaLcufgLmbkaXce4hekcuadae9Rcifg8Lcx2a8Lc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbhhascxfasyd2gecdtfahBdbasaecefgCBd2ascxfaCcdtfcua8Lcdta8LcFFFFi0Ecbyd;S1jjbHjjjjbbgzBdbasaecdfgCBd2ascxfaCcdtfa8Ecbyd;S1jjbHjjjjbbg3BdbasaecifgCBd2ascxfaCcdtfalcbyd;S1jjbHjjjjbbg9eBdbasaeclfBd2a8PJbbjZamclGEhcJbbbbh83dnadak9nmbdna8Lci6mbaxaxNacacN:vhBaDclthTahcwfhSJbbbbh83inasclfabadgoalaOz:cjjjbabhicbhEcbhyinabaycdtfh8JcbheindnaOaiaefydbgXcdtgKfydbg8AaOa8Jaec:S1jjbfydbcdtfydbgCcdtfydbgLSmbaAaCfRbbgYcv2aAaXfRbbgQfc;a1jjbfRbbgaaQcv2aYfg5c;a1jjbfRbbgdVcFeGTmbdnaLa8A9nmba5c:q1jjbfRbbcFeGmekdnaQaY9hmbaQcufcFeGce0mba8MaKfydbaC9hmekahaEcx2fg8AaCaXadcFeGgLEBdla8AaXaCaLEBdba8AaLaaGcb9hBdwaEcefhEkaeclfgecx9hmbkdnaycifgyao9pmbaicxfhiaEcifa8L9nmekkdnaEmbaohdxikcbhYinJbbbbJbbjZa8KaOahaYcx2fg8AydlgLa8AydbgQa8AydwgCEgicdtfydbgac8S2gdfgeIdygR:vaRJbbbb9BEaeIdwagaQaLaCEgKcx2fgCIdwg8UNaeIdzaCIdbgINaeIdaMgRaRMMa8UNaeIdlaCIdlg8SNaeIdCa8UNaeId3MgRaRMMa8SNaeIdbaINaeIdxa8SNaeIdKMgRaRMMaINaeId8KMMM:lNh80JbbbbJbbjZa8KaOaQcdtfydbgyc8S2gXfgeIdygR:vaRJbbbb9BEaeIdwagaLcx2fgCIdwg8WNaeIdzaCIdbg8XNaeIdaMgRaRMMa8WNaeIdlaCIdlg8VNaeIdCa8WNaeId3MgRaRMMa8VNaeIdba8XNaeIdxa8VNaeIdKMgRaRMMa8XNaeId8KMMM:lNh81a8Acwfh8Ja8Aclfh5dnaDTmbaqaXfgXIdwa8WNaXIdza8XNaXIdaMgRaRMMa8WNaXIdla8VNaXIdCa8WNaXId3MgRaRMMa8VNaXIdba8XNaXIdxa8VNaXIdKMgRaRMMa8XNaXId8KMMMh8RavaLaD2cdtfhCawayaD2cltfheaXIdyh8YaDhXinaCIdbgRJbbb;aNaecxfIdba8WaecwfIdbNa8XaeIdbNa8VaeclfIdbNMMMNaRaRNa8YNa8RMMh8RaCclfhCaeczfheaXcufgXmbkaqadfgXIdwa8UNaXIdzaINaXIdaMgRaRMMa8UNaXIdla8SNaXIdCa8UNaXId3MgRaRMMa8SNaXIdbaINaXIdxa8SNaXIdKMgRaRMMaINaXId8KMMMh8WavaKaD2cdtfhCawaaaD2cltfheaXIdyh8XaDhXinaCIdbgRJbbb;aNaecxfIdba8UaecwfIdbNaIaeIdbNa8SaeclfIdbNMMMNaRaRNa8XNa8WMMh8WaCclfhCaeczfheaXcufgXmbka80a8W:lMh80a81a8R:lMh81ka5aLaKa81a809FgeEBdba8AaQaiaeEBdba8Ja81a80aeEUdbaYcefgYaE9hmbkasc;Wbfcbcj;abz:ljjjb8AaSheaEhCinasc;WbfaeydbcO4c;8ZGfgXaXydbcefBdbaecxfheaCcufgCmbkcbhecbhCinasc;WbfaefgXydbh8AaXaCBdba8AaCfhCaeclfgecj;ab9hmbkcbheaShCinasc;WbfaCydbcO4c;8ZGfgXaXydbgXcefBdbazaXcdtfaeBdbaCcxfhCaEaecefge9hmbkaoak9RgXci9Uh9hdnalTmbcbhea3hCinaCaeBdbaCclfhCalaecefge9hmbkkcbh9ia9ecbalz:ljjjbh6aXcO9Uh9ka9hce4h0asydwh9mcbhdcbh5dninahaza5cdtfydbcx2fg8JIdwg8RaB9Emeada9h9pmeJFFuuhRdna0aE9pmbahaza0cdtfydbcx2fIdwJbb;aZNhRkdna8RaR9ETmbada9k0mdkdna6aOa8Jydlg9ncdtg9ofydbg8Afg9pRbba6aOa8Jydbgicdtg9qfydbg9rfg9sRbbVmbdnaJa9rcdtfgeclfydbgCaeydbgeSmbaCae9RhQa9maecitfheaga8Acx2fgKcwfhyaKclfh8Eaga9rcx2fgacwfhmaaclfhrcbhCcehYdnindna3aeydbcdtfydbgXa8ASmba3aeclfydbcdtfydbgLa8ASmbaXaLSmbagaLcx2fgLIdbagaXcx2fgXIdbg8W:tgRarIdbaXIdlg8U:tg8XNaaIdba8W:tg8VaLIdla8U:tg8RN:tgIaRa8EIdba8U:tg8YNaKIdba8W:tg80a8RN:tg8UNa8RamIdbaXIdwg8S:tg81Na8XaLIdwa8S:tg8WN:tg8Xa8RayIdba8S:tg8ZNa8Ya8WN:tg8RNa8Wa8VNa81aRN:tg8Sa8Wa80Na8ZaRN:tgRNMMaIaINa8Xa8XNa8Sa8SNMMa8Ua8UNa8Ra8RNaRaRNMMN:rJbbj8:N9FmdkaecwfheaCcefgCaQ6hYaQaC9hmbkkaYceGTmba0cefh0xeka8Ka8Ac8S2gXfgea8Ka9rc8S2gLfgCIdbaeIdbMUdbaeaCIdlaeIdlMUdlaeaCIdwaeIdwMUdwaeaCIdxaeIdxMUdxaeaCIdzaeIdzMUdzaeaCIdCaeIdCMUdCaeaCIdKaeIdKMUdKaeaCId3aeId3MUd3aeaCIdaaeIdaMUdaaeaCId8KaeId8KMUd8KaeaCIdyaeIdyMUdydnaDTmbaqaXfgeaqaLfgCIdbaeIdbMUdbaeaCIdlaeIdlMUdlaeaCIdwaeIdwMUdwaeaCIdxaeIdxMUdxaeaCIdzaeIdzMUdzaeaCIdCaeIdCMUdCaeaCIdKaeIdKMUdKaeaCId3aeId3MUd3aeaCIdaaeIdaMUdaaeaCId8KaeId8KMUd8KaeaCIdyaeIdyMUdyaTa9r2hYaTa8A2hKawhCaDhLinaCaKfgeaCaYfgXIdbaeIdbMUdbaeclfgQaXclfIdbaQIdbMUdbaecwfgQaXcwfIdbaQIdbMUdbaecxfgeaXcxfIdbaeIdbMUdbaCczfhCaLcufgLmbkka8JcwfhCdndndndnaAaifgXRbbc9:fPdebdkaiheina3aecdtgefa8ABdba8Faefydbgeai9hmbxikka8Fa9ofydbhea8Fa9qfydbhia3a9qfa9nBdbaeh9nka3aicdtfa9nBdbka9sce86bba9pce86bbaCIdbgRa83a83aR9DEh83a9icefh9icecdaXRbbceSEadfhdka5cefg5aE9hmbkkdna9imbaohdxikdnalTmbcbhCa8MheindnaeydbgXcuSmbdnaCa3aXcdtg8AfydbgX9hmba8Ma8AfydbhXkaeaXBdbkaeclfhealaCcefgC9hmbkcbhCa8NheindnaeydbgXcuSmbdnaCa3aXcdtg8AfydbgX9hmba8Na8AfydbhXkaeaXBdbkaeclfhealaCcefgC9hmbkkcbhdabhecbhLindna3aeydbcdtfydbgCa3aeclfydbcdtfydbgXSmbaCa3aecwfydbcdtfydbg8ASmbaXa8ASmbabadcdtfgQaCBdbaQcwfa8ABdbaQclfaXBdbadcifhdkaecxfheaLcifgLao6mbkadak9nmdxbkkasclfabadalaOz:cjjjbkdnaHTmbadTmbadheinabaHabydbcdtfydbBdbabclfhbaecufgembkkdnaPTmbaPaca83:rNUdbkasyd2gecdtascxffc98fhOdninaeTmeaOydbcbyd;O1jjbH:bjjjbbaOc98fhOaecufhexbkkasc;W;abf8Kjjjjbadk;Yieouabydlhvabydbclfcbaicdtz:ljjjbhoadci9UhrdnadTmbdnalTmbaehwadhDinaoalawydbcdtfydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbxdkkaehwadhDinaoawydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbkkdnaiTmbcbhDaohwinawydbhqawaDBdbawclfhwaqaDfhDaicufgimbkkdnadci6mbinaecwfydbhwaeclfydbhDaeydbhidnalTmbalawcdtfydbhwalaDcdtfydbhDalaicdtfydbhikavaoaicdtfgqydbcitfaDBdbavaqydbcitfawBdlaqaqydbcefBdbavaoaDcdtfgqydbcitfawBdbavaqydbcitfaiBdlaqaqydbcefBdbavaoawcdtfgwydbcitfaiBdbavawydbcitfaDBdlawawydbcefBdbaecxfhearcufgrmbkkabydbcbBdbk;Podvuv998Jjjjjbca9RgvcFFF;7rBd3av9cFFF;7;3FF:;Fb83dCavcFFF97Bdzav9cFFF;7FFF:;u83dwdnadTmbaicd4hodnabmbdnalTmbcbhrinaealarcdtfydbao2cdtfhwcbhiinavcCfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavcwfaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkarcefgrad9hmbxikkaocdthrcbhwincbhiinavcCfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbavcwfaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbxdkkdnalTmbcbhrinabarcx2fgiaealarcdtfydbao2cdtfgwIdbUdbaiawIdlUdlaiawIdwUdwcbhiinavcCfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavcwfaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkarcefgrad9hmbxdkkaocdthlcbhraehwinabarcx2fgiaearao2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinavcCfaifgDawaifIdbgqaDIdbgkakaq9EEUdbavcwfaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawalfhwarcefgrad9hmbkkJbbbbavIdwavIdCgk:tgqaqJbbbb9DEgqavIdxavIdKgx:tgmamaq9DEgqavIdzavId3gm:tgPaPaq9DEhPdnabTmbadTmbJbbbbJbbjZaP:vaPJbbbb9BEhqinabaqabIdbak:tNUdbabclfgvaqavIdbax:tNUdbabcwfgvaqavIdbam:tNUdbabcxfhbadcufgdmbkkaPk8MbabaeadaialavcbcbcbcbcbaoarawaDz:bjjjbk8MbabaeadaialavaoarawaDaqakaxamaPz:bjjjbk;3Aowud99wue99iul998Jjjjjbc;Wb9Rgw8KjjjjbdndnarmbcbhDxekawcxfcbc;Kbz:ljjjb8Aawcuadcx2adc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbgqBdxawceBd2aqaeadaicbz:djjjb8AawcuadcdtadcFFFFi0Egkcbyd;S1jjbHjjjjbbgxBdzawcdBd2adcd4adfhmceheinaegicetheaiam6mbkcbhmawcuaicdtgPaicFFFFi0Ecbyd;S1jjbHjjjjbbgsBdCawciBd2dndnar:Zgz:rJbbbZMgH:lJbbb9p9DTmbaH:Ohexekcjjjj94hekaicufhOc:bwhAcbhCcbhXadhQinaChLaeaAgKcufaeaK9iEamgDcefaeaD9kEhYdndnadTmbaYcuf:YhHaqhiaxheadhmindndnaiIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhAxekcjjjj94hAkaAcCthAdndnaiclfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcqtaAVhAdndnaicwfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaeaAaCVBdbaicxfhiaeclfheamcufgmmbkascFeaPz:ljjjbhEcbh3cbh5indnaEaxa5cdtfydbgAcm4aA7c:v;t;h;Ev2gics4ai7aOGgmcdtfgCydbgecuSmbaeaASmbcehiinaEamaifaOGgmcdtfgCydbgecuSmeaicefhiaeaA9hmbkkaCaABdba3aecuSfh3a5cefg5ad9hmbxdkkascFeaPz:ljjjb8Acbh3kaDaYa3ar0giEhmaLa3aiEhCdna3arSmbaYaKaiEgAam9Rcd9imbdndnaXcl0mbdnaQ:ZgHaL:Zg8A:taY:Yg8EaD:Y:tg8Fa8EaK:Y:tgaa3:Zghaz:tNNNaHaz:taaNa8Aah:tNa8Aaz:ta8FNahaH:tNM:va8EMJbbbZMgH:lJbbb9p9DTmbaH:Ohexdkcjjjj94hexekamaAfcd9Theka3aQaiEhQaXcefgXcs9hmekkdndnaCmbcihicbhDxekcbhiawakcbyd;S1jjbHjjjjbbg5BdKawclBd2dndnadTmbamcuf:YhHaqhiaxheadhmindndnaiIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhAxekcjjjj94hAkaAcCthAdndnaiclfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcqtaAVhAdndnaicwfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaeaAaCVBdbaicxfhiaeclfheamcufgmmbkascFeaPz:ljjjbhEcbhDcbh3inaxa3cdtgYfydbgAcm4aA7c:v;t;h;Ev2gics4ai7hecbhidndninaEaeaOGgmcdtfgCydbgecuSmednaxaecdtgCfydbaASmbaicefgiamfheaiaO9nmekka5aCfydbhixekaCa3BdbaDhiaDcefhDka5aYfaiBdba3cefg3ad9hmbkcuaDc32giaDc;j:KM;jb0EhexekascFeaPz:ljjjb8AcbhDcbhekawaecbyd;S1jjbHjjjjbbgeBd3awcvBd2aecbaiz:ljjjbhCavcd4hxdnadTmbdnalTmbaxcdthEa5hAalheaqhmadhOinaCaAydbc32fgiamIdbaiIdbMUdbaiamclfIdbaiIdlMUdlaiamcwfIdbaiIdwMUdwaiaeIdbaiIdxMUdxaiaeclfIdbaiIdzMUdzaiaecwfIdbaiIdCMUdCaiaiIdKJbbjZMUdKaAclfhAaeaEfheamcxfhmaOcufgOmbxdkka5hmaqheadhAinaCamydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiaiIdxJbbbbMUdxaiaiIdzJbbbbMUdzaiaiIdCJbbbbMUdCaiaiIdKJbbjZMUdKamclfhmaecxfheaAcufgAmbkkdnaDTmbaChiaDheinaiaiIdbJbbbbJbbjZaicKfIdbgH:vaHJbbbb9BEgHNUdbaiclfgmaHamIdbNUdbaicwfgmaHamIdbNUdbaicxfgmaHamIdbNUdbaiczfgmaHamIdbNUdbaicCfgmaHamIdbNUdbaic3fhiaecufgembkkcbhAawcuaDcdtgYaDcFFFFi0Egicbyd;S1jjbHjjjjbbgeBdaawcoBd2awaicbyd;S1jjbHjjjjbbgEBd8KaecFeaYz:ljjjbh3dnadTmbaoaoNh8Aaxcdthxalheina8Aaec;C1jjbalEgmIdwaCa5ydbgOc32fgiIdC:tgHaHNamIdbaiIdx:tgHaHNamIdlaiIdz:tgHaHNMMNaqcwfIdbaiIdw:tgHaHNaqIdbaiIdb:tgHaHNaqclfIdbaiIdl:tgHaHNMMMhHdndna3aOcdtgifgmydbcuSmbaEaifIdbaH9ETmekamaABdbaEaifaHUdbka5clfh5aeaxfheaqcxfhqadaAcefgA9hmbkkaba3aYz:kjjjb8AcrhikaicdthiinaiTmeaic98fgiawcxffydbcbyd;O1jjbH:bjjjbbxbkkawc;Wbf8KjjjjbaDk:Odieui99iu8Jjjjjbca9RgicFFF;7rBd3ai9cFFF;7;3FF:;Fb83dCaicFFF97Bdzai9cFFF;7FFF:;u83dwdndnaembJbbjFhlJbbjFhvJbbjFhoxekadcd4cdthrcbhwincbhdinaicCfadfgDabadfIdbglaDIdbgvaval9EEUdbaicwfadfgDalaDIdbgvaval9DEUdbadclfgdcx9hmbkabarfhbawcefgwae9hmbkaiIdzaiId3:thoaiIdxaiIdK:thvaiIdwaiIdC:thlkJbbbbalalJbbbb9DEglavaval9DEglaoaoal9DEk9DeeuabcFeaicdtz:ljjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd;W1jjbgeabcifc98GfgbBd;W1jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd;W1jjbgeabcrfc94GfgbBd;W1jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd;W1jjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd;W1jjbfgdBd;W1jjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:bedbcjwk9Oebbbdbbbbbbbebbbeeebeebebbeeebebbbbbebebbbbbebbbdbbbbbbbbbbbbbbbeeeeebebbbbbebbbbbeebbbbbbbbbbbbbbbbbbbbbc;Owkxebbbdbbbj9Kbb";

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

	function simplifyAttr(fun, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride, vertex_attributes, vertex_attributes_stride, attribute_weights, vertex_lock, target_index_count, target_error, options) {
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
		var result = fun(ti, si, index_count, sp, vertex_count, vertex_positions_stride, sa, vertex_attributes_stride, sw, attribute_weights.length, vl, target_index_count, target_error, options, te);
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

	function simplifyPoints(fun, vertex_positions, vertex_count, vertex_positions_stride, vertex_colors, vertex_colors_stride, color_weight, target_vertex_count) {
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
	};

	return {
		ready: ready,
		supported: true,

		// set this to true to be able to use simplifyPoints and simplifyWithAttributes
		// note that these functions are experimental and may change interface/behavior in a way that will require revising calling code
		useExperimentalFeatures: false,

		compactMesh: function(indices) {
			assert(indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array);
			assert(indices.length % 3 == 0);

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			return reorder(instance.exports.meshopt_optimizeVertexFetchRemap, indices32, maxindex(indices) + 1);
		},

		simplify: function(indices, vertex_positions, vertex_positions_stride, target_index_count, target_error, flags) {
			assert(indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array);
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
			var result = simplify(instance.exports.meshopt_simplify, indices32, indices.length, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4, target_index_count, target_error, options);
			result[0] = (indices instanceof Uint32Array) ? result[0] : new indices.constructor(result[0]);

			return result;
		},

		simplifyWithAttributes: function(indices, vertex_positions, vertex_positions_stride, vertex_attributes, vertex_attributes_stride, attribute_weights, vertex_lock, target_index_count, target_error, flags) {
			assert(this.useExperimentalFeatures); // set useExperimentalFeatures to use this; note that this function is experimental and may change interface in a way that will require revising calling code
			assert(indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(vertex_attributes instanceof Float32Array);
			assert(vertex_attributes.length % vertex_attributes_stride == 0);
			assert(vertex_attributes_stride >= 0);
			assert(vertex_lock == null || vertex_lock.length == vertex_positions.length);
			assert(target_index_count >= 0 && target_index_count <= indices.length);
			assert(target_index_count % 3 == 0);
			assert(target_error >= 0);
			assert(Array.isArray(attribute_weights));
			assert(vertex_attributes_stride >= attribute_weights.length);
			assert(attribute_weights.length <= 16);

			var options = 0;
			for (var i = 0; i < (flags ? flags.length : 0); ++i) {
				assert(flags[i] in simplifyOptions);
				options |= simplifyOptions[flags[i]];
			}

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			var result = simplifyAttr(instance.exports.meshopt_simplifyWithAttributes, indices32, indices.length, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4, vertex_attributes, vertex_attributes_stride * 4, new Float32Array(attribute_weights), vertex_lock ? new Uint8Array(vertex_lock) : null, target_index_count, target_error, options);
			result[0] = (indices instanceof Uint32Array) ? result[0] : new indices.constructor(result[0]);

			return result;
		},

		getScale: function(vertex_positions, vertex_positions_stride) {
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			return simplifyScale(instance.exports.meshopt_simplifyScale, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4);
		},

		simplifyPoints: function(vertex_positions, vertex_positions_stride, target_vertex_count, vertex_colors, vertex_colors_stride, color_weight) {
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
				return simplifyPoints(instance.exports.meshopt_simplifyPoints, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4, vertex_colors, vertex_colors_stride * 4, color_weight, target_vertex_count);
			} else {
				return simplifyPoints(instance.exports.meshopt_simplifyPoints, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4, undefined, 0, 0, target_vertex_count);
			}
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
