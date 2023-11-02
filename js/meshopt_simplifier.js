// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2023, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function() {
	"use strict";
	// Built with clang version 16.0.0
	// Built from meshoptimizer 0.20
	var wasm = "b9H79Tebbbe9Ek9Geueu9Geub9Gbb9GPuuuuuuuuuuu99uueu9Gvuuuuub9Gluuuub9Gquuuuuuu99uueu9Gwuuuuuu99ueu9Giuuue999Gluuuueu9GiuuueuizsdilvoirwDbqqbeqlve9Weiiviebeoweuec:G;jekr:Tewo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95bl8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bvQ9TW79O9V9Wt9F79P9T9W29P9M959q9V9P9Ut7boX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbra9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbwl79IV9RbDDwebcekdxmq::9Usdbk;i6iKuY99Ou8Jjjjjbc;W;ab9RgP8KjjjjbaPcwfcbc;Kbz:ljjjb8AaPcualcefgscdtascFFFFi0Ecbyd1:jjjbHjjjjbbgzBdwaPceBd9OaPazBdbaPcuadcitadcFFFFe0Ecbyd1:jjjbHjjjjbbgHBdxaPcdBd9OaPaHBdlaPaeadalcbz:cjjjbaPcualcdtgOalcFFFFi0EgAcbyd1:jjjbHjjjjbbgCBdzaPciBd9OaPaAcbyd1:jjjbHjjjjbbgXBdCaPclBd9Oalcd4alfhQcehLinaLgscethLasaQ6mbkcbhKaPcuascdtgLascFFFFi0Ecbyd1:jjjbHjjjjbbgQBdKaPcvBd9OaQcFeaLz:ljjjbhYdnalTmbavcd4h8AascufhEinaiaKa8A2cdtfg3ydlgscH4as7c:F:b:DD2a3ydbgscH4as7c;D;O:B8J27a3ydwgscH4as7c:3F;N8N27hQcbhsdndninaYaQaEGgQcdtfg5ydbgLcuSmeaiaLa8A2cdtfa3cxz:ojjjbTmdascefgsaQfhQasaE9nmbxdkka5aKBdbaKhLkaCaKcdtfaLBdbaKcefgKal9hmbkcbhsaXhLinaLasBdbaLclfhLalascefgs9hmbkcbhsaChLaXhQindnasaLydbgESmbaQaXaEcdtfgEydbBdbaEasBdbkaLclfhLaQclfhQalascefgs9hmbkkcbh8EaYcbyd:m:jjjbH:bjjjbbaPclBd9OaPalcbyd1:jjjbHjjjjbbgEBdKaPcvBd9OaPaAcbyd1:jjjbHjjjjbbgsBd3aPcoBd9OaPaAcbyd1:jjjbHjjjjbbgLBdaaPcrBd9OascFeaOz:ljjjbh8FaLcFeaOz:ljjjbhadnalTmbaHcwfhhindnaza8EgQcefg8Ecdtfydbg3azaQcdtgsfydbgLSmba3aL9RhOaHaLcitfhgaaasfh8Ja8FasfhKcbh8Aindndnaga8Acitfydbg5aQ9hmbaKaQBdba8JaQBdbxekdnaza5cdtg8KfgsclfydbgLasydbgsSmbaHascitg3fydbaQSmeaLas9Rh8Lascu7aLfhYaha3fhLcbhsinaYasSmeascefhsaLydbh3aLcwfhLa3aQ9hmbkasa8L6mekaaa8KfgsaQa5asydbcuSEBdbaKa5aQaKydbcuSEBdbka8Acefg8AaO9hmbkka8Eal9hmbkaChLaXhQaah3a8Fh8AcbhsindndnasaLydbg59hmbdnasaQydbg59hmba8Aydbh5dna3ydbgYcu9hmba5cu9hmbaEasfcb86bbxikaEasfhKdnasaYSmbasa5SmbaKce86bbxikaKcl86bbxdkdnasaXa5cdtgYfydb9hmbdna3ydbgKcuSmbasaKSmba8AydbgOcuSmbasaOSmbaaaYfydbggcuSmbaga5Smba8FaYfydbgYcuSmbaYa5SmbdnaCaKcdtfydbaCaYcdtfydb9hmbaCaOcdtfydbaCagcdtfydb9hmbaEasfcd86bbxlkaEasfcl86bbxikaEasfcl86bbxdkaEasfcl86bbxekaEasfaEa5fRbb86bbkaLclfhLaQclfhQa3clfh3a8Aclfh8Aalascefgs9hmbkaxceGTmbaEhsalhLindnasRbbce9hmbascl86bbkascefhsaLcufgLmbkkcualcx2alc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbh8JaPcwfaPyd9Ogscdtfa8JBdbaPascefgOBd9Oa8Jaialavz:djjjbdndnaDmbcbhvxekcbh5aPcwfaOcdtfcuaDal2gLcdtaLcFFFFi0Ecbyd1:jjjbHjjjjbbgvBdbaPascdfgOBd9OalTmbarcd4cdthYaDcdthKavh8AinaohsawhLa8AhQaDh3inaQasIdbaLIdbNUdbasclfhsaLclfhLaQclfhQa3cufg3mbkaoaYfhoa8AaKfh8Aa5cefg5al9hmbkkaPcwfaOcdtfcualc8S2gsalc;D;O;f8U0EgQcbyd1:jjjbHjjjjbbgLBdbaPaOcefg3Bd9OaLcbasz:ljjjbh8EdndndnaDTmbaPcwfa3cdtfaQcbyd1:jjjbHjjjjbbgoBdbaPaOcdfgLBd9Oaocbasz:ljjjb8AaPcwfaLcdtfcuaDal2gscltgLascFFFFb0Ecbyd1:jjjbHjjjjbbgwBdbaPaOcifBd9OawcbaLz:ljjjb8AadmexdkcbhocbhwadTmekcbh8AaehLindna8JaLclfydbg5cx2fgsIdba8JaLydbgYcx2fgQIdbg8M:tg8Na8JaLcwfydbgKcx2fg3IdlaQIdlgy:tg8PNa3Idba8M:tgIasIdlay:tg8RN:tg8Sa8SNa8Ra3IdwaQIdwgR:tg8UNa8PasIdwaR:tg8RN:tg8Pa8PNa8RaINa8Ua8NN:tg8Na8NNMM:rgIJbbbb9ETmba8SaI:vh8Sa8NaI:vh8Na8PaI:vh8Pka8EaCaYcdtfydbc8S2fgsa8PaI:rgIa8PNNg8RasIdbMUdbasa8NaIa8NNg8VNg8UasIdlMUdlasa8SaIa8SNg8WNg8XasIdwMUdwasa8Va8PNg8VasIdxMUdxasa8Wa8PNg8YasIdzMUdzasa8Wa8NNg8WasIdCMUdCasa8PaIa8SaRNa8Pa8MNaya8NNMM:mgyNg8MNg8PasIdKMUdKasa8Na8MNg8NasId3MUd3asa8Sa8MNg8SasIdaMUdaasa8MayNg8MasId8KMUd8KasaIasIdyMUdya8EaCa5cdtfydbc8S2fgsa8RasIdbMUdbasa8UasIdlMUdlasa8XasIdwMUdwasa8VasIdxMUdxasa8YasIdzMUdzasa8WasIdCMUdCasa8PasIdKMUdKasa8NasId3MUd3asa8SasIdaMUdaasa8MasId8KMUd8KasaIasIdyMUdya8EaCaKcdtfydbc8S2fgsa8RasIdbMUdbasa8UasIdlMUdlasa8XasIdwMUdwasa8VasIdxMUdxasa8YasIdzMUdzasa8WasIdCMUdCasa8PasIdKMUdKasa8NasId3MUd3asa8SasIdaMUdaasa8MasId8KMUd8KasaIasIdyMUdyaLcxfhLa8Acifg8Aad6mbkcbh5aehYincbhLinaEaeaLc:G1jjbfydba5fcdtfydbgQfRbbhsdndnaEaYaLfydbg3fRbbg8Ac99fcFeGcpe0mbasceSmbascd9hmekdna8AcufcFeGce0mba8Fa3cdtfydbaQ9hmekdnascufcFeGce0mbaaaQcdtfydba39hmekdna8Acv2asfc:W1jjbfRbbTmbaCaQcdtfydbaCa3cdtfydb0mekJbbacJbbjZasceSEhIa8AceShOa8JaeaLc:K1jjbfydba5fcdtfydbcx2fhsdna8JaQcx2fg8AIdwa8Ja3cx2fgKIdwgy:tg8Pa8PNa8AIdbaKIdbgR:tg8Na8NNa8AIdlaKIdlg8R:tg8Sa8SNMM:rg8MJbbbb9ETmba8Pa8M:vh8Pa8Sa8M:vh8Sa8Na8M:vh8NkJbbacaIaOEh8WdnasIdway:tgIa8PaIa8PNasIdbaR:tg8Xa8NNa8SasIdla8R:tg8VNMMg8UN:tgIaINa8Xa8Na8UN:tg8Pa8PNa8Va8Sa8UN:tg8Na8NNMM:rg8SJbbbb9ETmbaIa8S:vhIa8Na8S:vh8Na8Pa8S:vh8Pka8EaCa3cdtfydbc8S2fgsa8Pa8Wa8MNg8Sa8PNNg8UasIdbMUdbasa8Na8Sa8NNg8WNg8XasIdlMUdlasaIa8SaINg8MNg8VasIdwMUdwasa8Wa8PNg8WasIdxMUdxasa8Ma8PNg8YasIdzMUdzasa8Ma8NNg8ZasIdCMUdCasa8Pa8SaIayNa8PaRNa8Ra8NNMM:mgyNg8MNg8PasIdKMUdKasa8Na8MNg8NasId3MUd3asaIa8MNgIasIdaMUdaasa8MayNg8MasId8KMUd8Kasa8SasIdyMUdya8EaCaQcdtfydbc8S2fgsa8UasIdbMUdbasa8XasIdlMUdlasa8VasIdwMUdwasa8WasIdxMUdxasa8YasIdzMUdzasa8ZasIdCMUdCasa8PasIdKMUdKasa8NasId3MUd3asaIasIdaMUdaasa8MasId8KMUd8Kasa8SasIdyMUdykaLclfgLcx9hmbkaYcxfhYa5cifg5ad6mbkaDTmbcbhYinJbbbbhRa8JaeaYcdtfgsclfydbgKcx2fgLIdwa8JasydbgOcx2fgQIdwg8V:tg8Na8NNaLIdbaQIdbg8Y:tgIaINaLIdlaQIdlg8Z:tg8Sa8SNMMg8Wa8Jascwfydbggcx2fgsIdwa8V:tg8MNa8Na8Na8MNaIasIdba8Y:tgyNa8SasIdla8Z:tg8RNMMg8PN:tJbbbbJbbjZa8Wa8Ma8MNayayNa8Ra8RNMMg8XNa8Pa8PN:tg8U:va8UJbbbb9BEg8UNh80a8Xa8NNa8Ma8PN:ta8UNh81a8Wa8RNa8Sa8PN:ta8UNhBa8Xa8SNa8Ra8PN:ta8UNh83a8WayNaIa8PN:ta8UNhUa8XaINaya8PN:ta8UNh85aIa8RNaya8SN:tg8Pa8PNa8Sa8MNa8Ra8NN:tg8Pa8PNa8NayNa8MaIN:tg8Pa8PNMM:r:rh8PavaOaD2cdtfhLavagaD2cdtfhQavaKaD2cdtfh3a8V:mh86a8Z:mh87a8Y:mh88cbh8AaDh5Jbbbbh8RJbbbbh8UJbbbbh8WJbbbbh8XJbbbbh8VJbbbbh8YJbbbbh8ZJbbbbh89Jbbbbh8:inaPc;Wbfa8Afgscwfa8Pa81a3IdbaLIdbg8M:tg8SNa80aQIdba8M:tgyNMg8NNUdbasclfa8Pa83a8SNaBayNMgINUdbasa8Pa85a8SNaUayNMg8SNUdbascxfa8Pa86a8NNa87aINa8Ma88a8SNMMMg8MNUdba8Pa8NaINNa8XMh8Xa8Pa8Na8SNNa8VMh8Va8PaIa8SNNa8YMh8Ya8Pa8Ma8MNNaRMhRa8Pa8Na8MNNa8RMh8Ra8PaIa8MNNa8UMh8Ua8Pa8Sa8MNNa8WMh8Wa8Pa8Na8NNNa8ZMh8Za8PaIaINNa89Mh89a8Pa8Sa8SNNa8:Mh8:aLclfhLa3clfh3aQclfhQa8Aczfh8Aa5cufg5mbkaoaCaOcdtfydbgLc8S2fgsa8:asIdbMUdbasa89asIdlMUdlasa8ZasIdwMUdwasa8YasIdxMUdxasa8VasIdzMUdzasa8XasIdCMUdCasa8WasIdKMUdKasa8UasId3MUd3asa8RasIdaMUdaasaRasId8KMUd8Kasa8PasIdyMUdyaoaCaKcdtfydbgKc8S2fgsa8:asIdbMUdbasa89asIdlMUdlasa8ZasIdwMUdwasa8YasIdxMUdxasa8VasIdzMUdzasa8XasIdCMUdCasa8WasIdKMUdKasa8UasId3MUd3asa8RasIdaMUdaasaRasId8KMUd8Kasa8PasIdyMUdyaoaCagcdtfydbgOc8S2fgsa8:asIdbMUdbasa89asIdlMUdlasa8ZasIdwMUdwasa8YasIdxMUdxasa8VasIdzMUdzasa8XasIdCMUdCasa8WasIdKMUdKasa8UasId3MUd3asa8RasIdaMUdaasaRasId8KMUd8Kasa8PasIdyMUdyawaLaD2cltfh5cbhLaDh3ina5aLfgsaPc;WbfaLfgQIdbasIdbMUdbasclfg8AaQclfIdba8AIdbMUdbascwfg8AaQcwfIdba8AIdbMUdbascxfgsaQcxfIdbasIdbMUdbaLczfhLa3cufg3mbkawaKaD2cltfh5cbhLaDh3ina5aLfgsaPc;WbfaLfgQIdbasIdbMUdbasclfg8AaQclfIdba8AIdbMUdbascwfg8AaQcwfIdba8AIdbMUdbascxfgsaQcxfIdbasIdbMUdbaLczfhLa3cufg3mbkawaOaD2cltfh5cbhLaDh3ina5aLfgsaPc;WbfaLfgQIdbasIdbMUdbasclfg8AaQclfIdba8AIdbMUdbascwfg8AaQcwfIdba8AIdbMUdbascxfgsaQcxfIdbasIdbMUdbaLczfhLa3cufg3mbkaYcifgYad6mbkkdnabaeSmbabaeadcdtz:kjjjb8AkaPydbhZcbhsdnalTmbaZclfhsaZydbh3aEhLalh8AcbhQincbasydbg5a39RaLRbbcpeGEaQfhQaLcefhLasclfhsa5h3a8Acufg8AmbkaQce4hskcuadas9Rcifgrcx2arc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbh8LaPcwfaPyd9Ogscdtfa8LBdbaPascefgLBd9OaPcwfaLcdtfcuarcdtarcFFFFi0Ecbyd1:jjjbHjjjjbbgxBdbaPascdfgLBd9OaPcwfaLcdtfaAcbyd1:jjjbHjjjjbbgHBdbaPascifgLBd9OaPcwfaLcdtfalcbyd1:jjjbHjjjjbbgnBdbaPasclfBd9OJbbbbh8Ydnadaq9nmbdnarci6mbakakNh8VaDclthca8Lcwfh9cJbbbbh8YinaPabadghalaCz:cjjjbabhOcbhzcbhKincbhsindnaCaOasfydbgQcdtgYfydbg8AaCabasc;m1jjbfydbaKfcdtfydbgLcdtfydbg5SmbaEaLfRbbgecv2aEaQfRbbg3fc;G1jjbfRbbg8Ka3cv2aefggc;G1jjbfRbbgiVcFeGTmbdnagc:W1jjbfRbbTmba5a8A0mekdna3ae9hmba3cufcFeGce0mba8FaYfydbaL9hmeka8Lazcx2fg3aLaQaicFeGg8AEBdla3aQaLa8AEBdba3a8Aa8KGcb9hBdwazcefhzkasclfgscx9hmbkdnaKcifgKah9pmbaOcxfhOazcifar9nmekkdnazmbahhdxikcbheinJbbbbJbbjZa8EaCa8Laecx2fg3ydlg8Aa3ydbg5a3ydwgLEgKcdtfydbg8Kc8S2gifgsIdyg8P:va8PJbbbb9BEasIdwa8Ja5a8AaLEgYcx2fgLIdwg8SNasIdzaLIdbg8MNasIdaMg8Pa8PMMa8SNasIdlaLIdlgyNasIdCa8SNasId3Mg8Pa8PMMayNasIdba8MNasIdxayNasIdKMg8Pa8PMMa8MNasId8KMMM:lNh8WJbbbbJbbjZa8EaCa5cdtfydbgdc8S2gQfgsIdyg8P:va8PJbbbb9BEasIdwa8Ja8Acx2fgLIdwgINasIdzaLIdbgRNasIdaMg8Pa8PMMaINasIdlaLIdlg8RNasIdCaINasId3Mg8Pa8PMMa8RNasIdbaRNasIdxa8RNasIdKMg8Pa8PMMaRNasId8KMMM:lNh8Xa3cwfhOa3clfhgdnaDTmbaoaQfgQIdwaINaQIdzaRNaQIdaMg8Pa8PMMaINaQIdla8RNaQIdCaINaQId3Mg8Pa8PMMa8RNaQIdbaRNaQIdxa8RNaQIdKMg8Pa8PMMaRNaQId8KMMMh8Nava8AaD2cdtfhLawadaD2cltfhsaQIdyh8UaDhQinaLIdbg8PJbbb;aNascxfIdbaIascwfIdbNaRasIdbNa8RasclfIdbNMMMNa8Pa8PNa8UNa8NMMh8NaLclfhLasczfhsaQcufgQmbkaoaifgQIdwa8SNaQIdza8MNaQIdaMg8Pa8PMMa8SNaQIdlayNaQIdCa8SNaQId3Mg8Pa8PMMayNaQIdba8MNaQIdxayNaQIdKMg8Pa8PMMa8MNaQId8KMMMhIavaYaD2cdtfhLawa8KaD2cltfhsaQIdyhRaDhQinaLIdbg8PJbbb;aNascxfIdba8SascwfIdbNa8MasIdbNayasclfIdbNMMMNa8Pa8PNaRNaIMMhIaLclfhLasczfhsaQcufgQmbka8WaI:lMh8Wa8Xa8N:lMh8Xkaga8AaYa8Xa8W9FgsEBdba3a5aKasEBdbaOa8Xa8WasEUdbaecefgeaz9hmbkaPc;Wbfcbcj;abz:ljjjb8Aa9chsazhLinaPc;WbfasydbcO4c;8ZGfgQaQydbcefBdbascxfhsaLcufgLmbkcbhscbhLinaPc;WbfasfgQydbh3aQaLBdba3aLfhLasclfgscj;ab9hmbkcbhsa9chLinaPc;WbfaLydbcO4c;8ZGfgQaQydbgQcefBdbaxaQcdtfasBdbaLcxfhLazascefgs9hmbkahaq9RgQci9UhJdnalTmbcbhsaHhLinaLasBdbaLclfhLalascefgs9hmbkkcbh9eancbalz:ljjjbhTaQcO9UhSaJce4h9haPydlh9icbhicbhgdnina8Laxagcdtfydbcx2fgOIdwg8Na8V9EmeaiaJ9pmeJFFuuh8Pdna9haz9pmba8Laxa9hcdtfydbcx2fIdwJbb;aZNh8Pkdna8Na8P9ETmbaiaS0mdkdnaTaCaOydlg6cdtg9kfydbg3fg0RbbaTaCaOydbgKcdtg9mfydbg9nfg9oRbbVmbdnaZa9ncdtfgsclfydbgLasydbgsSmbaLas9Rh5a9iascitfhsa8Ja3cx2fgYcwfhdaYclfhAa8Ja9ncx2fg8Kcwfh9pa8Kclfh9qcbhLcehednindnaHasydbcdtfydbgQa3SmbaHasclfydbcdtfydbg8Aa3SmbaQa8ASmba8Ja8Acx2fg8AIdba8JaQcx2fgQIdbgI:tg8Pa9qIdbaQIdlg8S:tg8MNa8KIdbaI:tgya8AIdla8S:tg8NN:ta8PaAIdba8S:tgRNaYIdbaI:tg8Ra8NN:tNa8Na9pIdbaQIdwg8S:tg8UNa8Ma8AIdwa8S:tgIN:ta8NadIdba8S:tg8SNaRaIN:tNaIayNa8Ua8PN:taIa8RNa8Sa8PN:tNMMJbbbb9FmdkascwfhsaLcefgLa56hea5aL9hmbkkaeceGTmba9hcefh9hxeka8Ea3c8S2gQfgsa8Ea9nc8S2g8AfgLIdbasIdbMUdbasaLIdlasIdlMUdlasaLIdwasIdwMUdwasaLIdxasIdxMUdxasaLIdzasIdzMUdzasaLIdCasIdCMUdCasaLIdKasIdKMUdKasaLId3asId3MUd3asaLIdaasIdaMUdaasaLId8KasId8KMUd8KasaLIdyasIdyMUdydnaDTmbaoaQfgsaoa8AfgLIdbasIdbMUdbasaLIdlasIdlMUdlasaLIdwasIdwMUdwasaLIdxasIdxMUdxasaLIdzasIdzMUdzasaLIdCasIdCMUdCasaLIdKasIdKMUdKasaLId3asId3MUd3asaLIdaasIdaMUdaasaLId8KasId8KMUd8KasaLIdyasIdyMUdyaca9n2heaca32hYawhLaDh8AinaLaYfgsaLaefgQIdbasIdbMUdbasclfg5aQclfIdba5IdbMUdbascwfg5aQcwfIdba5IdbMUdbascxfgsaQcxfIdbasIdbMUdbaLczfhLa8Acufg8AmbkkaOcwfhLdndndndnaEaKfgQRbbc9:fPdebdkaKhsinaHascdtgsfa3BdbaXasfydbgsaK9hmbxikkaXa9kfydbhsaXa9mfydbhKaHa9mfa6Bdbash6kaHaKcdtfa6Bdbka9oce86bba0ce86bbaLIdbg8Pa8Ya8Ya8P9DEh8Ya9ecefh9ececdaQRbbceSEaifhikagcefggaz9hmbkkdna9embahhdxikdnalTmbcbhLa8FhsindnasydbgQcuSmbdnaLaHaQcdtg3fydbgQ9hmba8Fa3fydbhQkasaQBdbkasclfhsalaLcefgL9hmbkcbhLaahsindnasydbgQcuSmbdnaLaHaQcdtg3fydbgQ9hmbaaa3fydbhQkasaQBdbkasclfhsalaLcefgL9hmbkkcbhdabhscbh8AindnaHasydbcdtfydbgLaHasclfydbcdtfydbgQSmbaLaHascwfydbcdtfydbg3SmbaQa3Smbabadcdtfg5aLBdba5clfaQBdba5cwfa3Bdbadcifhdkascxfhsa8Acifg8Aah6mbkadaq9nmdxbkkaPabadalaCz:cjjjbkdnamTmbama8Y:rUdbkaPyd9OgscdtaPcwffc98fhCdninasTmeaCydbcbyd:m:jjjbH:bjjjbbaCc98fhCascufhsxbkkaPc;W;abf8Kjjjjbadk;:ieouabydlhvabydbclfcbaicdtz:ljjjbhoadci9UhrdnadTmbdnalTmbaehwadhDinaoalawydbcdtfydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbxdkkaehwadhDinaoawydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbkkdnaiTmbcbhDaohwinawydbhqawaDBdbawclfhwaqaDfhDaicufgimbkkdnadci6mbarcearce0EhdinaecwfydbhwaeclfydbhDaeydbhidnalTmbalawcdtfydbhwalaDcdtfydbhDalaicdtfydbhikavaoaicdtfgqydbcitfaDBdbavaqydbcitfawBdlaqaqydbcefBdbavaoaDcdtfgqydbcitfawBdbavaqydbcitfaiBdlaqaqydbcefBdbavaoawcdtfgwydbcitfaiBdbavawydbcitfaDBdlawawydbcefBdbaecxfheadcufgdmbkkabydbcbBdbk:Zldouv998Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdnadTmbaicd4hvdnabTmbavcdthocbhraehwinabarcx2fgiaearav2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinalczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawaofhwarcefgrad9hmbxdkkavcdthrcbhwincbhiinalczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbkkdnabTmbadTmbJbbbbJbbjZalIdbalIdzgk:tJbbbb:xgqalIdlalIdCgx:tgmamaq9DEgqalIdwalIdKgm:tgPaPaq9DEgq:vaqJbbbb9BEhqinabaqabIdbak:tNUdbabclfgiaqaiIdbax:tNUdbabcwfgiaqaiIdbam:tNUdbabcxfhbadcufgdmbkkk8KbabaeadaialavcbcbcbcbaoarawaDz:bjjjbk8KbabaeadaialavaoarawaDaqakaxamz:bjjjbk;lOowud99wue99iul998Jjjjjbc;Wb9Rgw8KjjjjbdndnarmbcbhDxekawcwfcbc;Kbz:ljjjb8Aawcuadcx2adc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbgqBdwawceBd9Oaqaeadaiz:djjjbawcuadcdtadcFFFFi0Egkcbyd1:jjjbHjjjjbbgxBdxawcdBd9Oadcd4adfhmceheinaegicetheaiam6mbkcbhmawcuaicdtgPaicFFFFi0Ecbyd1:jjjbHjjjjbbgsBdzawciBd9Odndnar:Zgz:rJbbbZMgH:lJbbb9p9DTmbaH:Ohexekcjjjj94hekaicufhOc:bwhAcbhCcbhXadhQinaChLaeaAgKcufaeaK9iEamgDcefaeaD9kEhYdndnadTmbaYcuf:YhHaqhiaxheadhmindndnaiIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhAxekcjjjj94hAkaAcCthAdndnaiclfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcqtaAVhAdndnaicwfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaeaAaCVBdbaicxfhiaeclfheamcufgmmbkascFeaPz:ljjjbhEcbh3cbh5indnaEaxa5cdtfydbgAcm4aA7c:v;t;h;Ev2gics4ai7aOGgmcdtfgCydbgecuSmbaeaASmbcehiinaEamaifaOGgmcdtfgCydbgecuSmeaicefhiaeaA9hmbkkaCaABdba3aecuSfh3a5cefg5ad9hmbxdkkascFeaPz:ljjjb8Acbh3kaDaYa3ar0giEhmaLa3aiEhCdna3arSmbaYaKaiEgAam9Rcd9imbdndnaXcl0mbdnaQ:ZgHaL:Zg8A:taY:Yg8EaD:Y:tg8Fa8EaK:Y:tgaa3:Zghaz:tNNNaHaz:taaNa8Aah:tNa8Aaz:ta8FNahaH:tNM:va8EMJbbbZMgH:lJbbb9p9DTmbaH:Ohexdkcjjjj94hexekamaAfcd9Theka3aQaiEhQaXcefgXcs9hmekkdndnaCmbcihicbhDxekcbhiawakcbyd1:jjjbHjjjjbbg5BdCawclBd9OdndnadTmbamcuf:YhHaqhiaxheadhmindndnaiIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhAxekcjjjj94hAkaAcCthAdndnaiclfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcqtaAVhAdndnaicwfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaeaAaCVBdbaicxfhiaeclfheamcufgmmbkascFeaPz:ljjjbhEcbhDcbh3inaxa3cdtgYfydbgAcm4aA7c:v;t;h;Ev2gics4ai7hecbhidndninaEaeaOGgmcdtfgCydbgecuSmednaxaecdtgCfydbaASmbaicefgiamfheaiaO9nmekka5aCfydbhixekaCa3BdbaDhiaDcefhDka5aYfaiBdba3cefg3ad9hmbkcuaDc32giaDc;j:KM;jb0EhexekascFeaPz:ljjjb8AcbhDcbhekawaecbyd1:jjjbHjjjjbbgeBdKawcvBd9Oaecbaiz:ljjjbhOavcd4hxdnadTmbaxcdth3a5hmalhAaqheadhEinaOamydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiaAc;81jjbalEgCIdbaiIdxMUdxaiaCIdlaiIdzMUdzaiaCIdwaiIdCMUdCaiaiIdKJbbjZMUdKamclfhmaAa3fhAaecxfheaEcufgEmbkkdnaDTmbaOcxfhiaDheinaictfgmamIdbJbbbbJbbjZaicxfIdbgH:vaHJbbbb9BEgHNUdbaic94fgmaHamIdbNUdbaic98fgmaHamIdbNUdbaiaHaiIdbNUdbaiclfgmaHamIdbNUdbaicwfgmaHamIdbNUdbaic3fhiaecufgembkkcbhAawcuaDcdtgYaDcFFFFi0Egicbyd1:jjjbHjjjjbbgeBd3awcoBd9Oawaicbyd1:jjjbHjjjjbbgEBdaaecFeaYz:ljjjbh3dnadTmbaoaoNh8Aaxcdthxalheina8Aaec;81jjbalEgmIdwaOa5ydbgCc32fgiIdC:tgHaHNamIdbaiIdx:tgHaHNamIdlaiIdz:tgHaHNMMNaqcwfIdbaiIdw:tgHaHNaqIdbaiIdb:tgHaHNaqclfIdbaiIdl:tgHaHNMMMhHdndna3aCcdtgifgmydbcuSmbaEaifIdbaH9ETmekamaABdbaEaifaHUdbka5clfh5aeaxfheaqcxfhqadaAcefgA9hmbkkaba3aYz:kjjjb8AcrhikaicdthiinaiTmeaic98fgiawcwffydbcbyd:m:jjjbH:bjjjbbxbkkawc;Wbf8KjjjjbaDk:Qdidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgoaDIdbgrarao9EEUdbaladfgDaoaDIdbgrarao9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkavJbbbb:xgvaoaoav9DEgoararao9DEk9DeeuabcFeaicdtz:ljjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcifc98GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcrfc94GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd:q:jjjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd:q:jjjbfgdBd:q:jjjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:Iedbcjwk1eFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbebbbdbbbbbbbebbbeeebeebebbeeebebbbbbebebbbbbebbbdbbbbbbbbbbbbbbbeeeeebebbbbbebbbbbeebbbbbbbbbbbbbbbbbbbbbc1Dkxebbbdbbb:G9Kbb";

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

	function simplifyAttr(fun, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride, vertex_attributes, vertex_attributes_stride, attribute_weights, target_index_count, target_error, options) {
		var sbrk = instance.exports.sbrk;
		var te = sbrk(4);
		var ti = sbrk(index_count * 4);
		var sp = sbrk(vertex_count * vertex_positions_stride);
		var sa = sbrk(vertex_count * vertex_attributes_stride);
		var sw = sbrk(attribute_weights.length * 4);
		var si = sbrk(index_count * 4);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(vertex_positions), sp);
		heap.set(bytes(vertex_attributes), sa);
		heap.set(bytes(attribute_weights), sw);
		heap.set(bytes(indices), si);
		var result = fun(ti, si, index_count, sp, vertex_count, vertex_positions_stride, sa, vertex_attributes_stride, sw, attribute_weights.length, target_index_count, target_error, options, te);
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

		simplifyWithAttributes: function(indices, vertex_positions, vertex_positions_stride, vertex_attributes, vertex_attributes_stride, attribute_weights, target_index_count, target_error, flags) {
			assert(this.useExperimentalFeatures); // set useExperimentalFeatures to use this; note that this function is experimental and may change interface in a way that will require revising calling code
			assert(indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(vertex_attributes instanceof Float32Array);
			assert(vertex_attributes.length % vertex_attributes_stride == 0);
			assert(vertex_attributes_stride >= 0);
			assert(target_index_count >= 0 && target_index_count <= indices.length);
			assert(target_index_count % 3 == 0);
			assert(target_error >= 0 && target_error <= 1);
			assert(Array.isArray(attribute_weights));
			assert(vertex_attributes_stride >= attribute_weights.length);
			assert(attribute_weights.length <= 16);

			var options = 0;
			for (var i = 0; i < (flags ? flags.length : 0); ++i) {
				options |= simplifyOptions[flags[i]];
			}

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			var result = simplifyAttr(instance.exports.meshopt_simplifyWithAttributes, indices32, indices.length, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4, vertex_attributes, vertex_attributes_stride * 4, new Float32Array(attribute_weights), target_index_count, target_error, options);
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
