// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2023, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function() {
	"use strict";

	// Built with clang version 16.0.0
	// Built from meshoptimizer 0.19
	var wasm = "b9H79Tebbbe9sq9Geueu9Geub9Gbb9Gquuuuuuu99uueu9GPuuuuuuuuu99uuuueu9Gvuuuuub9Gluuuub9Giuuue999Gluuuueu9GiuuueuiPmdilvorwbDDbeDlve9Weiiviebeoweuecj;jekrNero9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95be8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bdX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbva9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbol79IV9RbrDwebcekdqkq;H9Amdbk8KbabaeadaialavcbcbaoarawaDcbcbz:cjjjbk:m9iiKuY99Ou8Jjjjjbcj;bb9RgP8KjjjjbaPcKfcbc;Kbz:jjjjb8AaPcualcdtgsalcFFFFi0Egzcbyd;S1jjbHjjjjbbgHBdKaPceBd94aPaHBdwaPazcbyd;S1jjbHjjjjbbgOBd3aPcdBd94aPaOBdxaPcuadcitadcFFFFe0Ecbyd;S1jjbHjjjjbbgABdaaPciBd94aPaABdzaPcwfaeadalcbz:djjjbaPazcbyd;S1jjbHjjjjbbgCBd8KaPclBd94aPazcbyd;S1jjbHjjjjbbgXBdyaPcvBd94arcd4hQalcd4alfhLcehKinaKgrcethKaraL6mbkcbhYaPcuarcdtgKarcFFFFi0Ecbyd;S1jjbHjjjjbbgLBd8SaPcoBd94aLcFeaKz:jjjjbh8AdnalTmbavcd4hEarcufh3inaiaYaE2cdtfg5ydlgrcH4ar7c:F:b:DD2a5ydbgrcH4ar7c;D;O:B8J27a5ydwgrcH4ar7c:3F;N8N27hLcbhrdndnina8AaLa3GgLcdtfg8EydbgKcuSmeaiaKaE2cdtfa5cxz:mjjjbTmdarcefgraLfhLara39nmbxdkka8EaYBdbaYhKkaCaYcdtfaKBdbaYcefgYal9hmbkcbhraXhKinaKarBdbaKclfhKalarcefgr9hmbkcbhraChKaXhLindnaraKydbg3SmbaLaXa3cdtfg3ydbBdba3arBdbkaKclfhKaLclfhLalarcefgr9hmbkkcbhLaPalcbyd;S1jjbHjjjjbbg5Bd8WaPcrBd94aPazcbyd;S1jjbHjjjjbbgrBd80aPcwBd94aPazcbyd;S1jjbHjjjjbbgKBdUaPcDBd94arcFeasz:jjjjbh8FaKcFeasz:jjjjbhadnalTmbaAcwfhhindnaHaLcdtgrfydbggTmbaAaOarfydbcitfh8Jaaarfh8Ka8FarfhYcbhEindndna8JaEcitfydbg8AaL9hmbaYaLBdba8KaLBdbxekdnaHa8Acdtgsfydbg8LTmbaAaOasfydbcitgrfydbaLSmea8Lcufh8EaharfhKcbhrina8EarSmearcefhraKydbh3aKcwfhKa3aL9hmbkara8L6mekaaasfgraLa8AarydbcuSEBdbaYa8AaLaYydbcuSEBdbkaEcefgEag9hmbkkaLcefgLal9hmbkaChKaXhLaah3a8FhEcbhrindndnaraKydbg8E9hmbdnaraLydbg8E9hmbaEydbh8Edna3ydbg8Acu9hmba8Ecu9hmba5arfcb86bbxika5arfhYdnara8ASmbara8ESmbaYce86bbxikaYcl86bbxdkdnaraXa8Ecdtg8Afydb9hmbdna3ydbgYcuSmbaraYSmbaEydbgscuSmbarasSmbaaa8AfydbggcuSmbaga8ESmba8Fa8Afydbg8AcuSmba8Aa8ESmbdnaCaYcdtfydbaCa8Acdtfydb9hmbaCascdtfydbaCagcdtfydb9hmba5arfcd86bbxlka5arfcl86bbxika5arfcl86bbxdka5arfcl86bbxeka5arfa5a8EfRbb86bbkaKclfhKaLclfhLa3clfh3aEclfhEalarcefgr9hmbkaqceGTmba5hralhKindnarRbbce9hmbarcl86bbkarcefhraKcufgKmbkkcualcx2alc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbhHaPcKfaPyd94grcdtfaHBdbaParcefgsBd94aHaialavz:ejjjbdnamTmbaPcKfascdtfcuamal2gKcdtaKcFFFFi0Ecbyd;S1jjbHjjjjbbggBdbaParcdfgsBd94dnalTmbaQcdth8AamcdthYcbh8EaghEinaohraxhKaEhLamh3inaLarIdbaKIdbNUdbarclfhraKclfhKaLclfhLa3cufg3mbkaoa8AfhoaEaYfhEa8Ecefg8Eal9hmbkkaghokaPcKfascdtfcualc8S2gralc;D;O;f8U0EgLcbyd;S1jjbHjjjjbbgKBdbaPascefg3Bd94aKcbarz:jjjjbhAdndndnamTmbaPcKfa3cdtfaLcbyd;S1jjbHjjjjbbgxBdbaPascdfgKBd94axcbarz:jjjjb8AaPcKfaKcdtfcuamal2grcltgKarcFFFFb0Ecbyd;S1jjbHjjjjbbgvBdbaPascifBd94avcbaKz:jjjjb8AadmexdkcbhxcbhvadTmekcbhEaehKindnaHaKclfydbg8Ecx2fgrIdbaHaKydbg8Acx2fgLIdbg8M:tg8NaHaKcwfydbgYcx2fg3IdlaLIdlgy:tg8PNa3Idba8M:tgIarIdlay:tg8RN:tg8Sa8SNa8Ra3IdwaLIdwgR:tg8UNa8ParIdwaR:tg8RN:tg8Pa8PNa8RaINa8Ua8NN:tg8Na8NNMM:rgIJbbbb9ETmba8SaI:vh8Sa8NaI:vh8Na8PaI:vh8PkaAaCa8Acdtfydbc8S2fgra8PaI:rgIa8PNNg8RarIdbMUdbara8NaIa8NNg8VNg8UarIdlMUdlara8SaIa8SNg8WNg8XarIdwMUdwara8Va8PNg8VarIdxMUdxara8Wa8PNg8YarIdzMUdzara8Wa8NNg8WarIdCMUdCara8PaIa8SaRNa8Pa8MNaya8NNMM:mgyNg8MNg8ParIdKMUdKara8Na8MNg8NarId3MUd3ara8Sa8MNg8SarIdaMUdaara8MayNg8MarId8KMUd8KaraIarIdyMUdyaAaCa8Ecdtfydbc8S2fgra8RarIdbMUdbara8UarIdlMUdlara8XarIdwMUdwara8VarIdxMUdxara8YarIdzMUdzara8WarIdCMUdCara8ParIdKMUdKara8NarId3MUd3ara8SarIdaMUdaara8MarId8KMUd8KaraIarIdyMUdyaAaCaYcdtfydbc8S2fgra8RarIdbMUdbara8UarIdlMUdlara8XarIdwMUdwara8VarIdxMUdxara8YarIdzMUdzara8WarIdCMUdCara8ParIdKMUdKara8NarId3MUd3ara8SarIdaMUdaara8MarId8KMUd8KaraIarIdyMUdyaKcxfhKaEcifgEad6mbkcbh8EaehYincbhKina5aeaKc:81jjbfydbg8Aa8EfcdtfydbgLfRbbhrdndna5aYaKfydbg3fRbbgEc99fcFeGcpe0mbarceSmbarcd9hmekdnaEcufcFeGce0mba8Fa3cdtfydbaL9hmekdnarcufcFeGce0mbaaaLcdtfydba39hmekdnaEcv2arfc:G1jjbfRbbTmbaCaLcdtfydbaCa3cdtfydb0mekJbbacJbbjZarceSEhIaEceShsaHaea8Acdtc:81jjbfydba8Efcdtfydbcx2fhrdnaHaLcx2fgEIdwaHa3cx2fg8AIdwgy:tg8Pa8PNaEIdba8AIdbgR:tg8Na8NNaEIdla8AIdlg8R:tg8Sa8SNMM:rg8MJbbbb9ETmba8Pa8M:vh8Pa8Sa8M:vh8Sa8Na8M:vh8NkJbbacaIasEh8WdnarIdway:tgIa8PaIa8PNarIdbaR:tg8Xa8NNa8SarIdla8R:tg8VNMMg8UN:tgIaINa8Xa8Na8UN:tg8Pa8PNa8Va8Sa8UN:tg8Na8NNMM:rg8SJbbbb9ETmbaIa8S:vhIa8Na8S:vh8Na8Pa8S:vh8PkaAaCa3cdtfydbc8S2fgra8Pa8Wa8MNg8Sa8PNNg8UarIdbMUdbara8Na8Sa8NNg8WNg8XarIdlMUdlaraIa8SaINg8MNg8VarIdwMUdwara8Wa8PNg8WarIdxMUdxara8Ma8PNg8YarIdzMUdzara8Ma8NNg8ZarIdCMUdCara8Pa8SaIayNa8PaRNa8Ra8NNMM:mgyNg8MNg8ParIdKMUdKara8Na8MNg8NarId3MUd3araIa8MNgIarIdaMUdaara8MayNg8MarId8KMUd8Kara8SarIdyMUdyaAaCaLcdtfydbc8S2fgra8UarIdbMUdbara8XarIdlMUdlara8VarIdwMUdwara8WarIdxMUdxara8YarIdzMUdzara8ZarIdCMUdCara8ParIdKMUdKara8NarId3MUd3araIarIdaMUdaara8MarId8KMUd8Kara8SarIdyMUdykaKclfgKcx9hmbkaYcxfhYa8Ecifg8Ead6mbkamTmbcbh8AinJbbbbhRaHaea8AcdtfgrclfydbgYcx2fgKIdwaHarydbgscx2fgLIdwg8V:tg8Na8NNaKIdbaLIdbg8Y:tgIaINaKIdlaLIdlg8Z:tg8Sa8SNMMg8WaHarcwfydbggcx2fgrIdwa8V:tg8MNa8Na8Na8MNaIarIdba8Y:tgyNa8SarIdla8Z:tg8RNMMg8PN:tJbbbbJbbjZa8Wa8Ma8MNayayNa8Ra8RNMMg8XNa8Pa8PN:tg8U:va8UJbbbb9BEg8UNh80a8Xa8NNa8Ma8PN:ta8UNh81a8Wa8RNa8Sa8PN:ta8UNhBa8Xa8SNa8Ra8PN:ta8UNh83a8WayNaIa8PN:ta8UNhUa8XaINaya8PN:ta8UNh85aIa8RNaya8SN:tg8Pa8PNa8Sa8MNa8Ra8NN:tg8Pa8PNa8NayNa8MaIN:tg8Pa8PNMM:r:rh8Paoasam2cdtfhKaoagam2cdtfhLaoaYam2cdtfh3a8V:mh86a8Z:mh87a8Y:mh88cbhEamh8EJbbbbh8RJbbbbh8UJbbbbh8WJbbbbh8XJbbbbh8VJbbbbh8YJbbbbh8ZJbbbbh89Jbbbbh8:inaPcjefaEfgrcwfa8Pa81a3IdbaKIdbg8M:tg8SNa80aLIdba8M:tgyNMg8NNUdbarclfa8Pa83a8SNaBayNMgINUdbara8Pa85a8SNaUayNMg8SNUdbarcxfa8Pa86a8NNa87aINa8Ma88a8SNMMMg8MNUdba8Pa8NaINNa8XMh8Xa8Pa8Na8SNNa8VMh8Va8PaIa8SNNa8YMh8Ya8Pa8Ma8MNNaRMhRa8Pa8Na8MNNa8RMh8Ra8PaIa8MNNa8UMh8Ua8Pa8Sa8MNNa8WMh8Wa8Pa8Na8NNNa8ZMh8Za8PaIaINNa89Mh89a8Pa8Sa8SNNa8:Mh8:aKclfhKa3clfh3aLclfhLaEczfhEa8Ecufg8EmbkaxaCascdtfydbgKc8S2fgra8:arIdbMUdbara89arIdlMUdlara8ZarIdwMUdwara8YarIdxMUdxara8VarIdzMUdzara8XarIdCMUdCara8WarIdKMUdKara8UarId3MUd3ara8RarIdaMUdaaraRarId8KMUd8Kara8ParIdyMUdyaxaCaYcdtfydbgYc8S2fgra8:arIdbMUdbara89arIdlMUdlara8ZarIdwMUdwara8YarIdxMUdxara8VarIdzMUdzara8XarIdCMUdCara8WarIdKMUdKara8UarId3MUd3ara8RarIdaMUdaaraRarId8KMUd8Kara8ParIdyMUdyaxaCagcdtfydbgsc8S2fgra8:arIdbMUdbara89arIdlMUdlara8ZarIdwMUdwara8YarIdxMUdxara8VarIdzMUdzara8XarIdCMUdCara8WarIdKMUdKara8UarId3MUd3ara8RarIdaMUdaaraRarId8KMUd8Kara8ParIdyMUdyavaKam2cltfh8EcbhKamh3ina8EaKfgraPcjefaKfgLIdbarIdbMUdbarclfgEaLclfIdbaEIdbMUdbarcwfgEaLcwfIdbaEIdbMUdbarcxfgraLcxfIdbarIdbMUdbaKczfhKa3cufg3mbkavaYam2cltfh8EcbhKamh3ina8EaKfgraPcjefaKfgLIdbarIdbMUdbarclfgEaLclfIdbaEIdbMUdbarcwfgEaLcwfIdbaEIdbMUdbarcxfgraLcxfIdbarIdbMUdbaKczfhKa3cufg3mbkavasam2cltfh8EcbhKamh3ina8EaKfgraPcjefaKfgLIdbarIdbMUdbarclfgEaLclfIdbaEIdbMUdbarcwfgEaLcwfIdbaEIdbMUdbarcxfgraLcxfIdbarIdbMUdbaKczfhKa3cufg3mbka8Acifg8Aad6mbkkdnabaeSmbabaeadcdtz1jjjb8Akcuadcx2adc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbh8KaPcKfaPyd94grcdtfa8KBdbaParcefgKBd94aPcKfaKcdtfcuadcdtadcFFFFi0Ecbyd;S1jjbHjjjjbbgqBdbaParcdfgKBd94aPcKfaKcdtfazcbyd;S1jjbHjjjjbbgiBdbaParcifgKBd94aPcKfaKcdtfalcbyd;S1jjbHjjjjbbgZBdbaParclfBd94Jbbbbh8Ydnadaw9nmbaDaDNh8Vamclthna8KcwfhcJbbbbh8YinaPcwfabadgOalaCz:djjjbcbh8LabhYcbhsincbhrindnaCaYarfydbgLcdtgefydbgEaCabarc:81jjbfydbasfcdtfydbgKcdtfydbg8ESmba5aKfRbbg8Acv2a5aLfRbbg3fc;q1jjbfRbbg8Ja3cv2a8Afggc;q1jjbfRbbgdVcFeGTmbdnagc:G1jjbfRbbTmba8EaE0mekdna3a8A9hmba3cufcFeGce0mba8FaefydbaK9hmeka8Ka8Lcx2fg3aKaLadcFeGgEEBdla3aLaKaEEBdba3aEa8JGcb9hBdwa8Lcefh8Lkarclfgrcx9hmbkaYcxfhYascifgsaO6mbkdndna8LTmbcbh8AinJbbbbJbbjZaAaCa8Ka8Acx2fg3ydlgEa3ydbg8Ea3ydwgKEgYcdtfydbg8Jc8S2gdfgrIdyg8P:va8PJbbbb9BEarIdwaHa8EaEaKEgecx2fgKIdwg8SNarIdzaKIdbg8MNarIdaMg8Pa8PMMa8SNarIdlaKIdlgyNarIdCa8SNarId3Mg8Pa8PMMayNarIdba8MNarIdxayNarIdKMg8Pa8PMMa8MNarId8KMMM:lNh8WJbbbbJbbjZaAaCa8Ecdtfydbghc8S2gLfgrIdyg8P:va8PJbbbb9BEarIdwaHaEcx2fgKIdwgINarIdzaKIdbgRNarIdaMg8Pa8PMMaINarIdlaKIdlg8RNarIdCaINarId3Mg8Pa8PMMa8RNarIdbaRNarIdxa8RNarIdKMg8Pa8PMMaRNarId8KMMM:lNh8Xa3cwfhsa3clfhgdnamTmbaxaLfgLIdwaINaLIdzaRNaLIdaMg8Pa8PMMaINaLIdla8RNaLIdCaINaLId3Mg8Pa8PMMa8RNaLIdbaRNaLIdxa8RNaLIdKMg8Pa8PMMaRNaLId8KMMMh8NaoaEam2cdtfhKavaham2cltfhraLIdyh8UamhLinaKIdbg8PJbbb;aNarcxfIdbaIarcwfIdbNaRarIdbNa8RarclfIdbNMMMNa8Pa8PNa8UNa8NMMh8NaKclfhKarczfhraLcufgLmbkaxadfgLIdwa8SNaLIdza8MNaLIdaMg8Pa8PMMa8SNaLIdlayNaLIdCa8SNaLId3Mg8Pa8PMMayNaLIdba8MNaLIdxayNaLIdKMg8Pa8PMMa8MNaLId8KMMMhIaoaeam2cdtfhKava8Jam2cltfhraLIdyhRamhLinaKIdbg8PJbbb;aNarcxfIdba8SarcwfIdbNa8MarIdbNayarclfIdbNMMMNa8Pa8PNaRNaIMMhIaKclfhKarczfhraLcufgLmbka8WaI:lMh8Wa8Xa8N:lMh8XkagaEaea8Xa8W9FgrEBdba3a8EaYarEBdbasa8Xa8WarEUdba8Acefg8Aa8L9hmbkaPcjefcbcj;abz:jjjjb8Aachra8LhKinaPcjefarydbcO4c;8ZGfgLaLydbcefBdbarcxfhraKcufgKmbkcbhrcbhKinaPcjefarfgLydbh3aLaKBdba3aKfhKarclfgrcj;ab9hmbkcbhrachKinaPcjefaKydbcO4c;8ZGfgLaLydbgLcefBdbaqaLcdtfarBdbaKcxfhKa8Larcefgr9hmbkaOaw9RgLci9Uh9cdnalTmbcbhraihKinaKarBdbaKclfhKalarcefgr9hmbkkcbhJaZcbalz:jjjjbh9eaLcO9UhTa9cce4hSaPydwh9haPydxh9iaPydzh6cbh9kcbhgdnina8Kaqagcdtfydbcx2fgsIdwg8Na8V9Emea9ka9c9pmeJFFuuh8PdnaSa8L9pmba8KaqaScdtfydbcx2fIdwJbb;aZNh8Pkdna8Na8P9ETmba9kaT0mdkdna9eaCasydlg0cdtg9mfydbg3fg9nRbba9eaCasydbgYcdtg9ofydbg9pfg9qRbbVmbdna9ha9pcdtgrfydbgLTmba6a9iarfydbcitfhraHa3cx2fgecwfhdaeclfhhaHa9pcx2fg8Jcwfhza8JclfhQcbhKceh8AdnindnaiarydbcdtfydbgEa3Smbaiarclfydbcdtfydbg8Ea3SmbaHa8Ecx2fg8EIdbaHaEcx2fgEIdbgI:tg8PaQIdbaEIdlg8S:tg8MNa8JIdbaI:tgya8EIdla8S:tg8NN:ta8PahIdba8S:tgRNaeIdbaI:tg8Ra8NN:tNa8NazIdbaEIdwg8S:tg8UNa8Ma8EIdwa8S:tgIN:ta8NadIdba8S:tg8SNaRaIN:tNaIayNa8Ua8PN:taIa8RNa8Sa8PN:tNMMJbbbb9DmdkarcwfhraKcefgKaL6h8AaLaK9hmbkka8AceGTmbaScefhSxekaAa3c8S2gLfgraAa9pc8S2gEfgKIdbarIdbMUdbaraKIdlarIdlMUdlaraKIdwarIdwMUdwaraKIdxarIdxMUdxaraKIdzarIdzMUdzaraKIdCarIdCMUdCaraKIdKarIdKMUdKaraKId3arId3MUd3araKIdaarIdaMUdaaraKId8KarId8KMUd8KaraKIdyarIdyMUdydnamTmbaxaLfgraxaEfgKIdbarIdbMUdbaraKIdlarIdlMUdlaraKIdwarIdwMUdwaraKIdxarIdxMUdxaraKIdzarIdzMUdzaraKIdCarIdCMUdCaraKIdKarIdKMUdKaraKId3arId3MUd3araKIdaarIdaMUdaaraKId8KarId8KMUd8KaraKIdyarIdyMUdyana9p2h8Aana32heavhKamhEinaKaefgraKa8AfgLIdbarIdbMUdbarclfg8EaLclfIdba8EIdbMUdbarcwfg8EaLcwfIdba8EIdbMUdbarcxfgraLcxfIdbarIdbMUdbaKczfhKaEcufgEmbkkascwfhKdndndndna5aYfgLRbbc9:fPdebdkaYhrinaiarcdtgrfa3BdbaXarfydbgraY9hmbxikkaXa9mfydbhraXa9ofydbhYaia9ofa0Bdbarh0kaiaYcdtfa0Bdbka9qce86bba9nce86bbaKIdbg8Pa8Ya8Ya8P9DEh8YaJcefhJcecdaLRbbceSEa9kfh9kkagcefgga8L9hmbkkaJTmbdnalTmbcbhKa8FhrindnarydbgLcuSmbdnaKaiaLcdtg3fydbgL9hmba8Fa3fydbhLkaraLBdbkarclfhralaKcefgK9hmbkcbhKaahrindnarydbgLcuSmbdnaKaiaLcdtg3fydbgL9hmbaaa3fydbhLkaraLBdbkarclfhralaKcefgK9hmbkkcbhdabhrcbhEindnaiarydbcdtfydbgKaiarclfydbcdtfydbgLSmbaKaiarcwfydbcdtfydbg3SmbaLa3Smbabadcdtfg8EaKBdba8EclfaLBdba8Ecwfa3BdbadcifhdkarcxfhraEcifgEaO9pmdxbkkaOhdxdkadaw0mbkkdnakTmbaka8Y:rUdbkaPyd94grcdtaPcKffc98fhCdninarTmeaCydbcbyd;W1jjbH:bjjjbbaCc98fhCarcufhrxbkkaPcj;bbf8Kjjjjbadk;pleouabydbcbaicdtz:jjjjb8Aadci9UhvdnadTmbabydbhodnalTmbaehradhwinaoalarydbcdtfydbcdtfgDaDydbcefBdbarclfhrawcufgwmbxdkkaehradhwinaoarydbcdtfgDaDydbcefBdbarclfhrawcufgwmbkkdnaiTmbabydbhrabydlhwcbhDaihoinawaDBdbawclfhwarydbaDfhDarclfhraocufgombkkdnadci6mbavceavce0EhqabydlhvabydwhrinaecwfydbhwaeclfydbhDaeydbhodnalTmbalawcdtfydbhwalaDcdtfydbhDalaocdtfydbhokaravaocdtfgdydbcitfaDBdbaradydbcitfawBdladadydbcefBdbaravaDcdtfgdydbcitfawBdbaradydbcitfaoBdladadydbcefBdbaravawcdtfgwydbcitfaoBdbarawydbcitfaDBdlawawydbcefBdbaecxfheaqcufgqmbkkdnaiTmbabydlhrabydbhwinararydbawydb9RBdbawclfhwarclfhraicufgimbkkk:Zldouv998Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdnadTmbaicd4hvdnabTmbavcdthocbhraehwinabarcx2fgiaearav2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinalczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawaofhwarcefgrad9hmbxdkkavcdthrcbhwincbhiinalczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbkkdnabTmbadTmbJbbbbJbbjZalIdbalIdzgk:tJbbbb:xgqalIdlalIdCgx:tgmamaq9DEgqalIdwalIdKgm:tgPaPaq9DEgq:vaqJbbbb9BEhqinabaqabIdbak:tNUdbabclfgiaqaiIdbax:tNUdbabcwfgiaqaiIdbam:tNUdbabcxfhbadcufgdmbkkk:Qdidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgoaDIdbgrarao9EEUdbaladfgDaoaDIdbgrarao9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkavJbbbb:xgvaoaoav9DEgoararao9DEk9DeeuabcFeaicdtz:jjjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd;01jjbgeabcifc98GfgbBd;01jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd;01jjbgeabcrfc94GfgbBd;01jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd;01jjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd;01jjbfgdBd;01jjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:cedbcjwk9PFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbeeebeebebbeeebebbbbbebebbbbbebbbdbbbbbbbbbbbbbbbeeeeebebbbbbebbbbbeebbbbbbc;Swkxebbbdbbbj9Kbb";

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

	// TODO: unify this and simplify
	function simplifyAttr(fun, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride, vertex_attributes, vertex_attributes_stride, target_index_count, target_error, options, attribute_weights) {
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
		var result = fun(ti, si, index_count, sp, vertex_count, vertex_positions_stride, sa, vertex_attributes_stride, target_index_count, target_error, options, te, sw, attribute_weights.length);
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

		simplifyWithAttributes: function(indices, vertex_positions, vertex_positions_stride, vertex_attributes, vertex_attributes_stride, target_index_count, target_error, attribute_weights, flags) {
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
			assert(attribute_weights instanceof Float32Array); // TODO: allow regular arrays?
			assert(vertex_attributes_stride >= attribute_weights.length);

			var options = 0;
			for (var i = 0; i < (flags ? flags.length : 0); ++i) {
				options |= simplifyOptions[flags[i]];
			}

			var indices32 = indices.BYTES_PER_ELEMENT == 4 ? indices : new Uint32Array(indices);
			var result = simplifyAttr(instance.exports.meshopt_simplifyWithAttributes, indices32, indices.length, vertex_positions, vertex_positions.length / vertex_positions_stride, vertex_positions_stride * 4, vertex_attributes, vertex_attributes_stride * 4, target_index_count, target_error, options, attribute_weights);
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
