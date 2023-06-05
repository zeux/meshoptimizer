// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2023, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function() {
	"use strict";

	// Built with clang version 15.0.6
	// Built from meshoptimizer 0.19
	var wasm = "b9H79Tebbbe9tq9Geueu9Geub9Gbb9Gquuuuuuu99uueu9GPuuuuuuuuuuu99uueu9Gvuuuuub9Gluuuue999Giuuue999Gluuuueu9GiuuueuiPmdilvorwbDDbeDlve9Weiiviebeoweuecj;jekrNero9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95be8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bdX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbva9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbol79IV9RbrDwebcekdqkq;J9Amdbk8KbabaeadaialavcbcbcbcbaoarawaDz:cjjjbk:k9iiKuY99Ou8Jjjjjbcj;bb9RgP8KjjjjbaPcKfcbc;Kbz:jjjjb8AaPcualcdtgsalcFFFFi0Egzcbyd;S1jjbHjjjjbbgHBdKaPceBd94aPaHBdwaPazcbyd;S1jjbHjjjjbbgOBd3aPcdBd94aPaOBdxaPcuadcitadcFFFFe0Ecbyd;S1jjbHjjjjbbgABdaaPciBd94aPaABdzaPcwfaeadalcbz:djjjbaPazcbyd;S1jjbHjjjjbbgCBd8KaPclBd94aPazcbyd;S1jjbHjjjjbbgXBdyaPcvBd94alcd4alfhQcehLinaLgKcethLaKaQ6mbkcbhYaPcuaKcdtgLaKcFFFFi0Ecbyd;S1jjbHjjjjbbgQBd8SaPcoBd94aQcFeaLz:jjjjbh8AdnalTmbavcd4hEaKcufh3inaiaYaE2cdtfg5ydlgKcH4aK7c:F:b:DD2a5ydbgKcH4aK7c;D;O:B8J27a5ydwgKcH4aK7c:3F;N8N27hQcbhKdndnina8AaQa3GgQcdtfg8EydbgLcuSmeaiaLaE2cdtfa5cxz:mjjjbTmdaKcefgKaQfhQaKa39nmbxdkka8EaYBdbaYhLkaCaYcdtfaLBdbaYcefgYal9hmbkcbhKaXhLinaLaKBdbaLclfhLalaKcefgK9hmbkcbhKaChLaXhQindnaKaLydbg3SmbaQaXa3cdtfg3ydbBdba3aKBdbkaLclfhLaQclfhQalaKcefgK9hmbkkcbhQaPalcbyd;S1jjbHjjjjbbg5Bd8WaPcrBd94aPazcbyd;S1jjbHjjjjbbgKBd80aPcwBd94aPazcbyd;S1jjbHjjjjbbgLBdUaPcDBd94aKcFeasz:jjjjbh8FaLcFeasz:jjjjbhadnalTmbaAcwfhhindnaHaQcdtgKfydbggTmbaAaOaKfydbcitfh8JaaaKfh8Ka8FaKfhYcbhEindndna8JaEcitfydbg8AaQ9hmbaYaQBdba8KaQBdbxekdnaHa8Acdtgsfydbg8LTmbaAaOasfydbcitgKfydbaQSmea8Lcufh8EahaKfhLcbhKina8EaKSmeaKcefhKaLydbh3aLcwfhLa3aQ9hmbkaKa8L6mekaaasfgKaQa8AaKydbcuSEBdbaYa8AaQaYydbcuSEBdbkaEcefgEag9hmbkkaQcefgQal9hmbkaChLaXhQaah3a8FhEcbhKindndnaKaLydbg8E9hmbdnaKaQydbg8E9hmbaEydbh8Edna3ydbg8Acu9hmba8Ecu9hmba5aKfcb86bbxika5aKfhYdnaKa8ASmbaKa8ESmbaYce86bbxikaYcl86bbxdkdnaKaXa8Ecdtg8Afydb9hmbdna3ydbgYcuSmbaKaYSmbaEydbgscuSmbaKasSmbaaa8AfydbggcuSmbaga8ESmba8Fa8Afydbg8AcuSmba8Aa8ESmbdnaCaYcdtfydbaCa8Acdtfydb9hmbaCascdtfydbaCagcdtfydb9hmba5aKfcd86bbxlka5aKfcl86bbxika5aKfcl86bbxdka5aKfcl86bbxeka5aKfa5a8EfRbb86bbkaLclfhLaQclfhQa3clfh3aEclfhEalaKcefgK9hmbkaxceGTmba5hKalhLindnaKRbbce9hmbaKcl86bbkaKcefhKaLcufgLmbkkcualcx2alc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbhHaPcKfaPyd94gKcdtfaHBdbaPaKcefgsBd94aHaialavz:ejjjb8AdndnaDmbcbhxxekcbh8EaPcKfascdtfcuaDal2gLcdtaLcFFFFi0Ecbyd;S1jjbHjjjjbbgxBdbaPaKcdfgsBd94alTmbarcd4cdth8AaDcdthYaxhEinaohKawhLaEhQaDh3inaQaKIdbaLIdbNUdbaKclfhKaLclfhLaQclfhQa3cufg3mbkaoa8AfhoaEaYfhEa8Ecefg8Eal9hmbkkaPcKfascdtfcualc8S2gKalc;D;O;f8U0EgQcbyd;S1jjbHjjjjbbgLBdbaPascefg3Bd94aLcbaKz:jjjjbhAdndndnaDTmbaPcKfa3cdtfaQcbyd;S1jjbHjjjjbbgoBdbaPascdfgLBd94aocbaKz:jjjjb8AaPcKfaLcdtfcuaDal2gKcltgLaKcFFFFb0Ecbyd;S1jjbHjjjjbbgwBdbaPascifBd94awcbaLz:jjjjb8AadmexdkcbhocbhwadTmekcbhEaehLindnaHaLclfydbg8Ecx2fgKIdbaHaLydbg8Acx2fgQIdbg8M:tg8NaHaLcwfydbgYcx2fg3IdlaQIdlgy:tg8PNa3Idba8M:tgIaKIdlay:tg8RN:tg8Sa8SNa8Ra3IdwaQIdwgR:tg8UNa8PaKIdwaR:tg8RN:tg8Pa8PNa8RaINa8Ua8NN:tg8Na8NNMM:rgIJbbbb9ETmba8SaI:vh8Sa8NaI:vh8Na8PaI:vh8PkaAaCa8Acdtfydbc8S2fgKa8PaI:rgIa8PNNg8RaKIdbMUdbaKa8NaIa8NNg8VNg8UaKIdlMUdlaKa8SaIa8SNg8WNg8XaKIdwMUdwaKa8Va8PNg8VaKIdxMUdxaKa8Wa8PNg8YaKIdzMUdzaKa8Wa8NNg8WaKIdCMUdCaKa8PaIa8SaRNa8Pa8MNaya8NNMM:mgyNg8MNg8PaKIdKMUdKaKa8Na8MNg8NaKId3MUd3aKa8Sa8MNg8SaKIdaMUdaaKa8MayNg8MaKId8KMUd8KaKaIaKIdyMUdyaAaCa8Ecdtfydbc8S2fgKa8RaKIdbMUdbaKa8UaKIdlMUdlaKa8XaKIdwMUdwaKa8VaKIdxMUdxaKa8YaKIdzMUdzaKa8WaKIdCMUdCaKa8PaKIdKMUdKaKa8NaKId3MUd3aKa8SaKIdaMUdaaKa8MaKId8KMUd8KaKaIaKIdyMUdyaAaCaYcdtfydbc8S2fgKa8RaKIdbMUdbaKa8UaKIdlMUdlaKa8XaKIdwMUdwaKa8VaKIdxMUdxaKa8YaKIdzMUdzaKa8WaKIdCMUdCaKa8PaKIdKMUdKaKa8NaKId3MUd3aKa8SaKIdaMUdaaKa8MaKId8KMUd8KaKaIaKIdyMUdyaLcxfhLaEcifgEad6mbkcbh8EaehYincbhLina5aeaLc:81jjbfydbg8Aa8EfcdtfydbgQfRbbhKdndna5aYaLfydbg3fRbbgEc99fcFeGcpe0mbaKceSmbaKcd9hmekdnaEcufcFeGce0mba8Fa3cdtfydbaQ9hmekdnaKcufcFeGce0mbaaaQcdtfydba39hmekdnaEcv2aKfc:G1jjbfRbbTmbaCaQcdtfydbaCa3cdtfydb0mekJbbacJbbjZaKceSEhIaEceShsaHaea8Acdtc:81jjbfydba8Efcdtfydbcx2fhKdnaHaQcx2fgEIdwaHa3cx2fg8AIdwgy:tg8Pa8PNaEIdba8AIdbgR:tg8Na8NNaEIdla8AIdlg8R:tg8Sa8SNMM:rg8MJbbbb9ETmba8Pa8M:vh8Pa8Sa8M:vh8Sa8Na8M:vh8NkJbbacaIasEh8WdnaKIdway:tgIa8PaIa8PNaKIdbaR:tg8Xa8NNa8SaKIdla8R:tg8VNMMg8UN:tgIaINa8Xa8Na8UN:tg8Pa8PNa8Va8Sa8UN:tg8Na8NNMM:rg8SJbbbb9ETmbaIa8S:vhIa8Na8S:vh8Na8Pa8S:vh8PkaAaCa3cdtfydbc8S2fgKa8Pa8Wa8MNg8Sa8PNNg8UaKIdbMUdbaKa8Na8Sa8NNg8WNg8XaKIdlMUdlaKaIa8SaINg8MNg8VaKIdwMUdwaKa8Wa8PNg8WaKIdxMUdxaKa8Ma8PNg8YaKIdzMUdzaKa8Ma8NNg8ZaKIdCMUdCaKa8Pa8SaIayNa8PaRNa8Ra8NNMM:mgyNg8MNg8PaKIdKMUdKaKa8Na8MNg8NaKId3MUd3aKaIa8MNgIaKIdaMUdaaKa8MayNg8MaKId8KMUd8KaKa8SaKIdyMUdyaAaCaQcdtfydbc8S2fgKa8UaKIdbMUdbaKa8XaKIdlMUdlaKa8VaKIdwMUdwaKa8WaKIdxMUdxaKa8YaKIdzMUdzaKa8ZaKIdCMUdCaKa8PaKIdKMUdKaKa8NaKId3MUd3aKaIaKIdaMUdaaKa8MaKId8KMUd8KaKa8SaKIdyMUdykaLclfgLcx9hmbkaYcxfhYa8Ecifg8Ead6mbkaDTmbcbh8AinJbbbbhRaHaea8AcdtfgKclfydbgYcx2fgLIdwaHaKydbgscx2fgQIdwg8V:tg8Na8NNaLIdbaQIdbg8Y:tgIaINaLIdlaQIdlg8Z:tg8Sa8SNMMg8WaHaKcwfydbggcx2fgKIdwa8V:tg8MNa8Na8Na8MNaIaKIdba8Y:tgyNa8SaKIdla8Z:tg8RNMMg8PN:tJbbbbJbbjZa8Wa8Ma8MNayayNa8Ra8RNMMg8XNa8Pa8PN:tg8U:va8UJbbbb9BEg8UNh80a8Xa8NNa8Ma8PN:ta8UNh81a8Wa8RNa8Sa8PN:ta8UNhBa8Xa8SNa8Ra8PN:ta8UNh83a8WayNaIa8PN:ta8UNhUa8XaINaya8PN:ta8UNh85aIa8RNaya8SN:tg8Pa8PNa8Sa8MNa8Ra8NN:tg8Pa8PNa8NayNa8MaIN:tg8Pa8PNMM:r:rh8PaxasaD2cdtfhLaxagaD2cdtfhQaxaYaD2cdtfh3a8V:mh86a8Z:mh87a8Y:mh88cbhEaDh8EJbbbbh8RJbbbbh8UJbbbbh8WJbbbbh8XJbbbbh8VJbbbbh8YJbbbbh8ZJbbbbh89Jbbbbh8:inaPcjefaEfgKcwfa8Pa81a3IdbaLIdbg8M:tg8SNa80aQIdba8M:tgyNMg8NNUdbaKclfa8Pa83a8SNaBayNMgINUdbaKa8Pa85a8SNaUayNMg8SNUdbaKcxfa8Pa86a8NNa87aINa8Ma88a8SNMMMg8MNUdba8Pa8NaINNa8XMh8Xa8Pa8Na8SNNa8VMh8Va8PaIa8SNNa8YMh8Ya8Pa8Ma8MNNaRMhRa8Pa8Na8MNNa8RMh8Ra8PaIa8MNNa8UMh8Ua8Pa8Sa8MNNa8WMh8Wa8Pa8Na8NNNa8ZMh8Za8PaIaINNa89Mh89a8Pa8Sa8SNNa8:Mh8:aLclfhLa3clfh3aQclfhQaEczfhEa8Ecufg8EmbkaoaCascdtfydbgLc8S2fgKa8:aKIdbMUdbaKa89aKIdlMUdlaKa8ZaKIdwMUdwaKa8YaKIdxMUdxaKa8VaKIdzMUdzaKa8XaKIdCMUdCaKa8WaKIdKMUdKaKa8UaKId3MUd3aKa8RaKIdaMUdaaKaRaKId8KMUd8KaKa8PaKIdyMUdyaoaCaYcdtfydbgYc8S2fgKa8:aKIdbMUdbaKa89aKIdlMUdlaKa8ZaKIdwMUdwaKa8YaKIdxMUdxaKa8VaKIdzMUdzaKa8XaKIdCMUdCaKa8WaKIdKMUdKaKa8UaKId3MUd3aKa8RaKIdaMUdaaKaRaKId8KMUd8KaKa8PaKIdyMUdyaoaCagcdtfydbgsc8S2fgKa8:aKIdbMUdbaKa89aKIdlMUdlaKa8ZaKIdwMUdwaKa8YaKIdxMUdxaKa8VaKIdzMUdzaKa8XaKIdCMUdCaKa8WaKIdKMUdKaKa8UaKId3MUd3aKa8RaKIdaMUdaaKaRaKId8KMUd8KaKa8PaKIdyMUdyawaLaD2cltfh8EcbhLaDh3ina8EaLfgKaPcjefaLfgQIdbaKIdbMUdbaKclfgEaQclfIdbaEIdbMUdbaKcwfgEaQcwfIdbaEIdbMUdbaKcxfgKaQcxfIdbaKIdbMUdbaLczfhLa3cufg3mbkawaYaD2cltfh8EcbhLaDh3ina8EaLfgKaPcjefaLfgQIdbaKIdbMUdbaKclfgEaQclfIdbaEIdbMUdbaKcwfgEaQcwfIdbaEIdbMUdbaKcxfgKaQcxfIdbaKIdbMUdbaLczfhLa3cufg3mbkawasaD2cltfh8EcbhLaDh3ina8EaLfgKaPcjefaLfgQIdbaKIdbMUdbaKclfgEaQclfIdbaEIdbMUdbaKcwfgEaQcwfIdbaEIdbMUdbaKcxfgKaQcxfIdbaKIdbMUdbaLczfhLa3cufg3mbka8Acifg8Aad6mbkkdnabaeSmbabaeadcdtz1jjjb8Akcuadcx2adc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbh8KaPcKfaPyd94gKcdtfa8KBdbaPaKcefgLBd94aPcKfaLcdtfcuadcdtadcFFFFi0Ecbyd;S1jjbHjjjjbbgvBdbaPaKcdfgLBd94aPcKfaLcdtfazcbyd;S1jjbHjjjjbbgiBdbaPaKcifgLBd94aPcKfaLcdtfalcbyd;S1jjbHjjjjbbgZBdbaPaKclfBd94Jbbbbh8Ydnadaq9nmbakakNh8VaDclthna8KcwfhcJbbbbh8YinaPcwfabadgOalaCz:djjjbcbh8LabhYcbhsincbhKindnaCaYaKfydbgQcdtgefydbgEaCabaKc:81jjbfydbasfcdtfydbgLcdtfydbg8ESmba5aLfRbbg8Acv2a5aQfRbbg3fc;q1jjbfRbbg8Ja3cv2a8Afggc;q1jjbfRbbgdVcFeGTmbdnagc:G1jjbfRbbTmba8EaE0mekdna3a8A9hmba3cufcFeGce0mba8FaefydbaL9hmeka8Ka8Lcx2fg3aLaQadcFeGgEEBdla3aQaLaEEBdba3aEa8JGcb9hBdwa8Lcefh8LkaKclfgKcx9hmbkaYcxfhYascifgsaO6mbkdndna8LTmbcbh8AinJbbbbJbbjZaAaCa8Ka8Acx2fg3ydlgEa3ydbg8Ea3ydwgLEgYcdtfydbg8Jc8S2gdfgKIdyg8P:va8PJbbbb9BEaKIdwaHa8EaEaLEgecx2fgLIdwg8SNaKIdzaLIdbg8MNaKIdaMg8Pa8PMMa8SNaKIdlaLIdlgyNaKIdCa8SNaKId3Mg8Pa8PMMayNaKIdba8MNaKIdxayNaKIdKMg8Pa8PMMa8MNaKId8KMMM:lNh8WJbbbbJbbjZaAaCa8Ecdtfydbghc8S2gQfgKIdyg8P:va8PJbbbb9BEaKIdwaHaEcx2fgLIdwgINaKIdzaLIdbgRNaKIdaMg8Pa8PMMaINaKIdlaLIdlg8RNaKIdCaINaKId3Mg8Pa8PMMa8RNaKIdbaRNaKIdxa8RNaKIdKMg8Pa8PMMaRNaKId8KMMM:lNh8Xa3cwfhsa3clfhgdnaDTmbaoaQfgQIdwaINaQIdzaRNaQIdaMg8Pa8PMMaINaQIdla8RNaQIdCaINaQId3Mg8Pa8PMMa8RNaQIdbaRNaQIdxa8RNaQIdKMg8Pa8PMMaRNaQId8KMMMh8NaxaEaD2cdtfhLawahaD2cltfhKaQIdyh8UaDhQinaLIdbg8PJbbb;aNaKcxfIdbaIaKcwfIdbNaRaKIdbNa8RaKclfIdbNMMMNa8Pa8PNa8UNa8NMMh8NaLclfhLaKczfhKaQcufgQmbkaoadfgQIdwa8SNaQIdza8MNaQIdaMg8Pa8PMMa8SNaQIdlayNaQIdCa8SNaQId3Mg8Pa8PMMayNaQIdba8MNaQIdxayNaQIdKMg8Pa8PMMa8MNaQId8KMMMhIaxaeaD2cdtfhLawa8JaD2cltfhKaQIdyhRaDhQinaLIdbg8PJbbb;aNaKcxfIdba8SaKcwfIdbNa8MaKIdbNayaKclfIdbNMMMNa8Pa8PNaRNaIMMhIaLclfhLaKczfhKaQcufgQmbka8WaI:lMh8Wa8Xa8N:lMh8XkagaEaea8Xa8W9FgKEBdba3a8EaYaKEBdbasa8Xa8WaKEUdba8Acefg8Aa8L9hmbkaPcjefcbcj;abz:jjjjb8AachKa8LhLinaPcjefaKydbcO4c;8ZGfgQaQydbcefBdbaKcxfhKaLcufgLmbkcbhKcbhLinaPcjefaKfgQydbh3aQaLBdba3aLfhLaKclfgKcj;ab9hmbkcbhKachLinaPcjefaLydbcO4c;8ZGfgQaQydbgQcefBdbavaQcdtfaKBdbaLcxfhLa8LaKcefgK9hmbkaOaq9RgQci9Uh9cdnalTmbcbhKaihLinaLaKBdbaLclfhLalaKcefgK9hmbkkcbhJaZcbalz:jjjjbh9eaQcO9UhTa9cce4hSaPydwh9haPydxh9iaPydzh6cbh9kcbhgdnina8Kavagcdtfydbcx2fgsIdwg8Na8V9Emea9ka9c9pmeJFFuuh8PdnaSa8L9pmba8KavaScdtfydbcx2fIdwJbb;aZNh8Pkdna8Na8P9ETmba9kaT0mdkdna9eaCasydlg0cdtg9mfydbg3fg9nRbba9eaCasydbgYcdtg9ofydbg9pfg9qRbbVmbdna9ha9pcdtgKfydbgQTmba6a9iaKfydbcitfhKaHa3cx2fgecwfhdaeclfhhaHa9pcx2fg8Jcwfhza8JclfhrcbhLceh8AdnindnaiaKydbcdtfydbgEa3SmbaiaKclfydbcdtfydbg8Ea3SmbaHa8Ecx2fg8EIdbaHaEcx2fgEIdbgI:tg8ParIdbaEIdlg8S:tg8MNa8JIdbaI:tgya8EIdla8S:tg8NN:ta8PahIdba8S:tgRNaeIdbaI:tg8Ra8NN:tNa8NazIdbaEIdwg8S:tg8UNa8Ma8EIdwa8S:tgIN:ta8NadIdba8S:tg8SNaRaIN:tNaIayNa8Ua8PN:taIa8RNa8Sa8PN:tNMMJbbbb9FmdkaKcwfhKaLcefgLaQ6h8AaQaL9hmbkka8AceGTmbaScefhSxekaAa3c8S2gQfgKaAa9pc8S2gEfgLIdbaKIdbMUdbaKaLIdlaKIdlMUdlaKaLIdwaKIdwMUdwaKaLIdxaKIdxMUdxaKaLIdzaKIdzMUdzaKaLIdCaKIdCMUdCaKaLIdKaKIdKMUdKaKaLId3aKId3MUd3aKaLIdaaKIdaMUdaaKaLId8KaKId8KMUd8KaKaLIdyaKIdyMUdydnaDTmbaoaQfgKaoaEfgLIdbaKIdbMUdbaKaLIdlaKIdlMUdlaKaLIdwaKIdwMUdwaKaLIdxaKIdxMUdxaKaLIdzaKIdzMUdzaKaLIdCaKIdCMUdCaKaLIdKaKIdKMUdKaKaLId3aKId3MUd3aKaLIdaaKIdaMUdaaKaLId8KaKId8KMUd8KaKaLIdyaKIdyMUdyana9p2h8Aana32heawhLaDhEinaLaefgKaLa8AfgQIdbaKIdbMUdbaKclfg8EaQclfIdba8EIdbMUdbaKcwfg8EaQcwfIdba8EIdbMUdbaKcxfgKaQcxfIdbaKIdbMUdbaLczfhLaEcufgEmbkkascwfhLdndndndna5aYfgQRbbc9:fPdebdkaYhKinaiaKcdtgKfa3BdbaXaKfydbgKaY9hmbxikkaXa9mfydbhKaXa9ofydbhYaia9ofa0BdbaKh0kaiaYcdtfa0Bdbka9qce86bba9nce86bbaLIdbg8Pa8Ya8Ya8P9DEh8YaJcefhJcecdaQRbbceSEa9kfh9kkagcefgga8L9hmbkkaJTmbdnalTmbcbhLa8FhKindnaKydbgQcuSmbdnaLaiaQcdtg3fydbgQ9hmba8Fa3fydbhQkaKaQBdbkaKclfhKalaLcefgL9hmbkcbhLaahKindnaKydbgQcuSmbdnaLaiaQcdtg3fydbgQ9hmbaaa3fydbhQkaKaQBdbkaKclfhKalaLcefgL9hmbkkcbhdabhKcbhEindnaiaKydbcdtfydbgLaiaKclfydbcdtfydbgQSmbaLaiaKcwfydbcdtfydbg3SmbaQa3Smbabadcdtfg8EaLBdba8EclfaQBdba8Ecwfa3BdbadcifhdkaKcxfhKaEcifgEaO9pmdxbkkaOhdxdkadaq0mbkkdnamTmbama8Y:rUdbkaPyd94gKcdtaPcKffc98fhCdninaKTmeaCydbcbyd;W1jjbH:bjjjbbaCc98fhCaKcufhKxbkkaPcj;bbf8Kjjjjbadk;pleouabydbcbaicdtz:jjjjb8Aadci9UhvdnadTmbabydbhodnalTmbaehradhwinaoalarydbcdtfydbcdtfgDaDydbcefBdbarclfhrawcufgwmbxdkkaehradhwinaoarydbcdtfgDaDydbcefBdbarclfhrawcufgwmbkkdnaiTmbabydbhrabydlhwcbhDaihoinawaDBdbawclfhwarydbaDfhDarclfhraocufgombkkdnadci6mbavceavce0EhqabydlhvabydwhrinaecwfydbhwaeclfydbhDaeydbhodnalTmbalawcdtfydbhwalaDcdtfydbhDalaocdtfydbhokaravaocdtfgdydbcitfaDBdbaradydbcitfawBdladadydbcefBdbaravaDcdtfgdydbcitfawBdbaradydbcitfaoBdladadydbcefBdbaravawcdtfgwydbcitfaoBdbarawydbcitfaDBdlawawydbcefBdbaecxfheaqcufgqmbkkdnaiTmbabydlhrabydbhwinararydbawydb9RBdbawclfhwarclfhraicufgimbkkk:3ldouv998Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdnadTmbaicd4hvdnabTmbavcdthocbhraehwinabarcx2fgiaearav2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinalczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawaofhwarcefgrad9hmbxdkkavcdthrcbhwincbhiinalczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbkkalIdbalIdzgk:tJbbbb:xgqalIdlalIdCgx:tgmamaq9DEgqalIdwalIdKgm:tgPaPaq9DEhPdnabTmbadTmbJbbbbJbbjZaP:vaPJbbbb9BEhqinabaqabIdbak:tNUdbabclfgiaqaiIdbax:tNUdbabcwfgiaqaiIdbam:tNUdbabcxfhbadcufgdmbkkaPk:Qdidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgoaDIdbgrarao9EEUdbaladfgDaoaDIdbgrarao9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkavJbbbb:xgvaoaoav9DEgoararao9DEk9DeeuabcFeaicdtz:jjjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd;01jjbgeabcifc98GfgbBd;01jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd;01jjbgeabcrfc94GfgbBd;01jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd;01jjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd;01jjbfgdBd;01jjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:cedbcjwk9PFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbeeebeebebbeeebebbbbbebebbbbbebbbdbbbbbbbbbbbbbbbeeeeebebbbbbebbbbbeebbbbbbc;Swkxebbbdbbbj9Kbb";

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

		simplifyWithAttributes: function(indices, vertex_positions, vertex_positions_stride, vertex_attributes, vertex_attributes_stride, attribute_weights, target_index_count, target_error, flags) {
			assert(indices instanceof Uint32Array || indices instanceof Int32Array || indices instanceof Uint16Array || indices instanceof Int16Array);
			assert(indices.length % 3 == 0);
			assert(vertex_positions instanceof Float32Array);
			assert(vertex_positions.length % vertex_positions_stride == 0);
			assert(vertex_positions_stride >= 3);
			assert(vertex_attributes instanceof Float32Array);
			assert(vertex_attributes.length % vertex_attributes_stride == 0);
			assert(vertex_attributes_stride >= 0);
			assert(target_index_count % 3 == 0);
			assert(target_error >= 0 && target_error <= 1);
			assert(Array.isArray(attribute_weights));
			assert(vertex_attributes_stride >= attribute_weights.length);

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

			return simplifyScale(instance.exports.meshopt_simplifyScale, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4);
		},
	};
})();

export { MeshoptSimplifier };
