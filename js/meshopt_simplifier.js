// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2024, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function() {
	"use strict";
	// Built with clang version 18.1.2
	// Built from meshoptimizer 0.20
	var wasm = "b9H79Tebbbe9Fk9Geueu9Geub9Gbb9Gsuuuuuuuuuuuu99uueu9Gvuuuuub9Gluuuub9Gquuuuuuu99uueu9Gwuuuuuu99ueu9Giuuue999Gluuuueu9GiuuueuizsdilvoirwDbqqbeqlve9Weiiviebeoweuecj;jekr:Tewo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95bl8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bvQ9TW79O9V9Wt9F79P9T9W29P9M959q9V9P9Ut7boX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbra9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbwl79IV9RbDDwebcekdxmq;T9Vsdbk;v6iKuY99Hu8Jjjjjbc;W;ab9Rgs8Kjjjjbascxfcbc;Kbz:ljjjb8AascualcefgzcdtazcFFFFi0Ecbyd;O1jjbHjjjjbbgHBdxasceBd2asaHBdlascuadcitadcFFFFe0Ecbyd;O1jjbHjjjjbbgOBdzascdBd2asaOBdwasclfaeadalcbz:cjjjbascualcdtgAalcFFFFi0EgCcbyd;O1jjbHjjjjbbgXBdCasciBd2asaCcbyd;O1jjbHjjjjbbgQBdKasclBd2alcd4alfhLcehKinaKgzcethKazaL6mbkcbhYascuazcdtgKazcFFFFi0Ecbyd;O1jjbHjjjjbbgLBd3ascvBd2aLcFeaKz:ljjjbh8AdnalTmbavcd4hEazcufh3inaiaYaE2cdtfg5ydlgzcH4az7c:F:b:DD2a5ydbgzcH4az7c;D;O:B8J27a5ydwgzcH4az7c:3F;N8N27hLcbhzdndnina8AaLa3GgLcdtfg8EydbgKcuSmeaiaKaE2cdtfa5cxz:ojjjbTmdazcefgzaLfhLaza39nmbxdkka8EaYBdbaYhKkaXaYcdtfaKBdbaYcefgYal9hmbkcbhzaQhKinaKazBdbaKclfhKalazcefgz9hmbkcbhzaXhKaQhLindnazaKydbg3SmbaLaQa3cdtfg3ydbBdba3azBdbkaKclfhKaLclfhLalazcefgz9hmbkkcbh8Fa8Acbyd;S1jjbH:bjjjbbasclBd2asalcbyd;O1jjbHjjjjbbg3Bd3ascvBd2asaCcbyd;O1jjbHjjjjbbgzBdaascoBd2asaCcbyd;O1jjbHjjjjbbgKBd8KascrBd2azcFeaAz:ljjjbhaaKcFeaAz:ljjjbhhdnalTmbaOcwfhgindnaHa8FgLcefg8Fcdtfydbg5aHaLcdtgzfydbgKSmba5aK9RhAaOaKcitfh8Jahazfh8KaaazfhYcbhEindndna8JaEcitfydbg8EaL9hmbaYaLBdba8KaLBdbxekdnaHa8Ecdtg8LfgzclfydbgKazydbgzSmbaOazcitg5fydbaLSmeaKaz9Rh8Mazcu7aKfh8Aaga5fhKcbhzina8AazSmeazcefhzaKydbh5aKcwfhKa5aL9hmbkaza8M6mekaha8LfgzaLa8EazydbcuSEBdbaYa8EaLaYydbcuSEBdbkaEcefgEaA9hmbkka8Fal9hmbkaXhKaQhLahh5aahEcbhzindndnazaKydbg8E9hmbdnaqTmbaqazfRbbTmba3azfcl86bbxdkdnazaLydbg8E9hmbaEydbh8Edna5ydbg8Acu9hmba8Ecu9hmba3azfcb86bbxika3azfhYdnaza8ASmbaza8ESmbaYce86bbxikaYcl86bbxdkdnazaQa8Ecdtg8Afydb9hmbdna5ydbgYcuSmbazaYSmbaEydbgAcuSmbazaASmbaha8Afydbg8JcuSmba8Ja8ESmbaaa8Afydbg8AcuSmba8Aa8ESmbdnaXaYcdtfydbaXa8Acdtfydb9hmbaXaAcdtfydbaXa8Jcdtfydb9hmba3azfcd86bbxlka3azfcl86bbxika3azfcl86bbxdka3azfcl86bbxeka3azfa3a8EfRbb86bbkaKclfhKaLclfhLa5clfh5aEclfhEalazcefgz9hmbkamceGTmba3hzalhKindnazRbbce9hmbazcl86bbkazcefhzaKcufgKmbkkascualcx2alc;v:Q;v:Qe0Ecbyd;O1jjbHjjjjbbg8KBdycwhAascwBd2a8Kaialavz:djjjbcbhvdnaDTmbcbh8EascuaDal2gzcdtazcFFFFi0Ecbyd;O1jjbHjjjjbbgvBd8ScDhAascDBd2alTmbarcd4cdth8AaDcdthYavhEinaohzawhKaEhLaDh5inaLazIdbaKIdbNUdbazclfhzaKclfhKaLclfhLa5cufg5mbkaoa8AfhoaEaYfhEa8Ecefg8Eal9hmbkkascxfaAcdtfcualc8S2gzalc;D;O;f8U0EgLcbyd;O1jjbHjjjjbbgKBdbasaAcefg5Bd2aKcbazz:ljjjbh8FdndndnaDTmbascxfa5cdtfaLcbyd;O1jjbHjjjjbbgqBdbasaAcdVgKBd2aqcbazz:ljjjb8AascxfaKcdtfcuaDal2gzcltgKazcFFFFb0Ecbyd;O1jjbHjjjjbbgwBdbasaAcifBd2awcbaKz:ljjjb8AadmexdkcbhqcbhwadTmekcbhEaehKindna8KaKclfydbg8Ecx2fgzIdba8KaKydbg8Acx2fgLIdbg8N:tgya8KaKcwfydbgYcx2fg5IdlaLIdlg8P:tgINa5Idba8N:tg8RazIdla8P:tg8SN:tgRaRNa8Sa5IdwaLIdwg8U:tg8VNaIazIdwa8U:tg8SN:tgIaINa8Sa8RNa8VayN:tgyayNMM:rg8RJbbbb9ETmbaRa8R:vhRaya8R:vhyaIa8R:vhIka8FaXa8Acdtfydbc8S2fgzaIa8R:rg8RaINNg8SazIdbMUdbazaya8RayNg8WNg8VazIdlMUdlazaRa8RaRNg8XNg8YazIdwMUdwaza8WaINg8WazIdxMUdxaza8XaINg8ZazIdzMUdzaza8XayNg8XazIdCMUdCazaIa8RaRa8UNaIa8NNa8PayNMM:mg8PNg8NNgIazIdKMUdKazaya8NNgyazId3MUd3azaRa8NNgRazIdaMUdaaza8Na8PNg8NazId8KMUd8Kaza8RazIdyMUdya8FaXa8Ecdtfydbc8S2fgza8SazIdbMUdbaza8VazIdlMUdlaza8YazIdwMUdwaza8WazIdxMUdxaza8ZazIdzMUdzaza8XazIdCMUdCazaIazIdKMUdKazayazId3MUd3azaRazIdaMUdaaza8NazId8KMUd8Kaza8RazIdyMUdya8FaXaYcdtfydbc8S2fgza8SazIdbMUdbaza8VazIdlMUdlaza8YazIdwMUdwaza8WazIdxMUdxaza8ZazIdzMUdzaza8XazIdCMUdCazaIazIdKMUdKazayazId3MUd3azaRazIdaMUdaaza8NazId8KMUd8Kaza8RazIdyMUdyaKcxfhKaEcifgEad6mbkcbh8Jaeh8Ainaea8Jcdtfh8EcbhKina3a8EaKcj1jjbfydbcdtfydbgLfRbbhzdndna3a8AaKfydbg5fRbbgEc99fcFeGcpe0mbazceSmbazcd9hmekdnaEcufcFeGce0mbaaa5cdtfydbaL9hmekdnazcufcFeGce0mbahaLcdtfydba59hmekdnaEcv2azfc:q1jjbfRbbTmbaXaLcdtfydbaXa5cdtfydb0mekdna8KaLcx2fgYIdwa8Ka5cx2fgAIdwg8P:tgIaINaYIdbaAIdbg8U:tgyayNaYIdlaAIdlg8S:tg8Ra8RNMM:rg8NJbbbb9ETmbaIa8N:vhIa8Ra8N:vh8Raya8N:vhykJbbacJbbacJbbjZazceSEaEceSEh8Xdna8Ka8EaKc:e1jjbfydbcdtfydbcx2fgzIdwa8P:tgRaIaRaINazIdba8U:tg8YayNa8RazIdla8S:tg8WNMMg8VN:tgRaRNa8Yaya8VN:tgIaINa8Wa8Ra8VN:tgyayNMM:rg8RJbbbb9ETmbaRa8R:vhRaya8R:vhyaIa8R:vhIka8FaXa5cdtfydbc8S2fgzaIa8Xa8NNg8RaINNg8VazIdbMUdbazaya8RayNg8XNg8YazIdlMUdlazaRa8RaRNg8NNg8WazIdwMUdwaza8XaINg8XazIdxMUdxaza8NaINg8ZazIdzMUdzaza8NayNg80azIdCMUdCazaIa8RaRa8PNaIa8UNa8SayNMM:mg8PNg8NNgIazIdKMUdKazaya8NNgyazId3MUd3azaRa8NNgRazIdaMUdaaza8Na8PNg8NazId8KMUd8Kaza8RazIdyMUdya8FaXaLcdtfydbc8S2fgza8VazIdbMUdbaza8YazIdlMUdlaza8WazIdwMUdwaza8XazIdxMUdxaza8ZazIdzMUdzaza80azIdCMUdCazaIazIdKMUdKazayazId3MUd3azaRazIdaMUdaaza8NazId8KMUd8Kaza8RazIdyMUdykaKclfgKcx9hmbka8Acxfh8Aa8Jcifg8Jad6mbkaDTmbcbh8AinJbbbbh8Ua8Kaea8AcdtfgzclfydbgYcx2fgKIdwa8KazydbgAcx2fgLIdwg8W:tgyayNaKIdbaLIdbg8Z:tgRaRNaKIdlaLIdlg80:tg8Ra8RNMMg8Xa8Kazcwfydbg8Jcx2fgzIdwa8W:tg8NNayaya8NNaRazIdba8Z:tg8PNa8RazIdla80:tg8SNMMgIN:tJbbbbJbbjZa8Xa8Na8NNa8Pa8PNa8Sa8SNMMg8YNaIaIN:tg8V:va8VJbbbb9BEg8VNh81a8YayNa8NaIN:ta8VNhBa8Xa8SNa8RaIN:ta8VNh83a8Ya8RNa8SaIN:ta8VNhUa8Xa8PNaRaIN:ta8VNh85a8YaRNa8PaIN:ta8VNh86aRa8SNa8Pa8RN:tgIaINa8Ra8NNa8SayN:tgIaINaya8PNa8NaRN:tgIaINMM:r:rhIavaAaD2cdtfhKava8JaD2cdtfhLavaYaD2cdtfh5a8W:mh87a80:mh88a8Z:mh89cbhEaDh8EJbbbbh8SJbbbbh8VJbbbbh8XJbbbbh8YJbbbbh8WJbbbbh8ZJbbbbh80Jbbbbh8:JbbbbhZinasc;WbfaEfgzcwfaIaBa5IdbaKIdbg8N:tg8RNa81aLIdba8N:tg8PNMgyNUdbazclfaIaUa8RNa83a8PNMgRNUdbazaIa86a8RNa85a8PNMg8RNUdbazcxfaIa87ayNa88aRNa8Na89a8RNMMMg8NNUdbaIayaRNNa8YMh8YaIaya8RNNa8WMh8WaIaRa8RNNa8ZMh8ZaIa8Na8NNNa8UMh8UaIaya8NNNa8SMh8SaIaRa8NNNa8VMh8VaIa8Ra8NNNa8XMh8XaIayayNNa80Mh80aIaRaRNNa8:Mh8:aIa8Ra8RNNaZMhZaKclfhKa5clfh5aLclfhLaEczfhEa8Ecufg8EmbkaqaXaAcdtfydbgKc8S2fgzaZazIdbMUdbaza8:azIdlMUdlaza80azIdwMUdwaza8ZazIdxMUdxaza8WazIdzMUdzaza8YazIdCMUdCaza8XazIdKMUdKaza8VazId3MUd3aza8SazIdaMUdaaza8UazId8KMUd8KazaIazIdyMUdyaqaXaYcdtfydbgYc8S2fgzaZazIdbMUdbaza8:azIdlMUdlaza80azIdwMUdwaza8ZazIdxMUdxaza8WazIdzMUdzaza8YazIdCMUdCaza8XazIdKMUdKaza8VazId3MUd3aza8SazIdaMUdaaza8UazId8KMUd8KazaIazIdyMUdyaqaXa8JcdtfydbgAc8S2fgzaZazIdbMUdbaza8:azIdlMUdlaza80azIdwMUdwaza8ZazIdxMUdxaza8WazIdzMUdzaza8YazIdCMUdCaza8XazIdKMUdKaza8VazId3MUd3aza8SazIdaMUdaaza8UazId8KMUd8KazaIazIdyMUdyawaKaD2cltfh8EcbhKaDh5ina8EaKfgzasc;WbfaKfgLIdbazIdbMUdbazclfgEaLclfIdbaEIdbMUdbazcwfgEaLcwfIdbaEIdbMUdbazcxfgzaLcxfIdbazIdbMUdbaKczfhKa5cufg5mbkawaYaD2cltfh8EcbhKaDh5ina8EaKfgzasc;WbfaKfgLIdbazIdbMUdbazclfgEaLclfIdbaEIdbMUdbazcwfgEaLcwfIdbaEIdbMUdbazcxfgzaLcxfIdbazIdbMUdbaKczfhKa5cufg5mbkawaAaD2cltfh8EcbhKaDh5ina8EaKfgzasc;WbfaKfgLIdbazIdbMUdbazclfgEaLclfIdbaEIdbMUdbazcwfgEaLcwfIdbaEIdbMUdbazcxfgzaLcxfIdbazIdbMUdbaKczfhKa5cufg5mbka8Acifg8Aad6mbkkdnabaeSmbabaeadcdtz:kjjjb8AkasydlhncbhzdnalTmbanclfhzanydbh5a3hKalhEcbhLincbazydbg8Ea59RaKRbbcpeGEaLfhLaKcefhKazclfhza8Eh5aEcufgEmbkaLce4hzkcuadaz9Rcifgmcx2amc;v:Q;v:Qe0Ecbyd;O1jjbHjjjjbbh8Mascxfasyd2gzcdtfa8MBdbasazcefgKBd2ascxfaKcdtfcuamcdtamcFFFFi0Ecbyd;O1jjbHjjjjbbgeBdbasazcdfgKBd2ascxfaKcdtfaCcbyd;O1jjbHjjjjbbgOBdbasazcifgKBd2ascxfaKcdtfalcbyd;O1jjbHjjjjbbgcBdbasazclfBd2Jbbbbh8Zdnadak9nmbdnamci6mbaxaxNh8WaDclth9ca8McwfhJJbbbbh8ZinasclfabadgoalaXz:cjjjbabhAcbhdcbhginabagcdtfh8JcbhzindnaXaAazfydbgLcdtgYfydbg5aXa8Jazc:S1jjbfydbcdtfydbgKcdtfydbgESmba3aKfRbbg8Acv2a3aLfRbbg8Efc;a1jjbfRbbgia8Ecv2a8Afg8Lc;a1jjbfRbbgHVcFeGTmbdnaEa59nmba8Lc:q1jjbfRbbcFeGmekdna8Ea8A9hmba8EcufcFeGce0mbaaaYfydbaK9hmeka8Madcx2fg5aKaLaHcFeGgEEBdla5aLaKaEEBdba5aEaiGcb9hBdwadcefhdkazclfgzcx9hmbkdnagcifggao9pmbaAcxfhAadcifam9nmekkdnadmbaohdxikcbh8AinJbbbbJbbjZa8FaXa8Ma8Acx2fg5ydlgEa5ydbg8Ea5ydwgKEgAcdtfydbgic8S2gHfgzIdygI:vaIJbbbb9BEazIdwa8Ka8EaEaKEgYcx2fgKIdwg8RNazIdzaKIdbg8NNazIdaMgIaIMMa8RNazIdlaKIdlg8PNazIdCa8RNazId3MgIaIMMa8PNazIdba8NNazIdxa8PNazIdKMgIaIMMa8NNazId8KMMM:lNh8XJbbbbJbbjZa8FaXa8Ecdtfydbggc8S2gLfgzIdygI:vaIJbbbb9BEazIdwa8KaEcx2fgKIdwgRNazIdzaKIdbg8UNazIdaMgIaIMMaRNazIdlaKIdlg8SNazIdCaRNazId3MgIaIMMa8SNazIdba8UNazIdxa8SNazIdKMgIaIMMa8UNazId8KMMM:lNh8Ya5cwfh8Ja5clfh8LdnaDTmbaqaLfgLIdwaRNaLIdza8UNaLIdaMgIaIMMaRNaLIdla8SNaLIdCaRNaLId3MgIaIMMa8SNaLIdba8UNaLIdxa8SNaLIdKMgIaIMMa8UNaLId8KMMMhyavaEaD2cdtfhKawagaD2cltfhzaLIdyh8VaDhLinaKIdbgIJbbb;aNazcxfIdbaRazcwfIdbNa8UazIdbNa8SazclfIdbNMMMNaIaINa8VNayMMhyaKclfhKazczfhzaLcufgLmbkaqaHfgLIdwa8RNaLIdza8NNaLIdaMgIaIMMa8RNaLIdla8PNaLIdCa8RNaLId3MgIaIMMa8PNaLIdba8NNaLIdxa8PNaLIdKMgIaIMMa8NNaLId8KMMMhRavaYaD2cdtfhKawaiaD2cltfhzaLIdyh8UaDhLinaKIdbgIJbbb;aNazcxfIdba8RazcwfIdbNa8NazIdbNa8PazclfIdbNMMMNaIaINa8UNaRMMhRaKclfhKazczfhzaLcufgLmbka8XaR:lMh8Xa8Yay:lMh8Yka8LaEaYa8Ya8X9FgzEBdba5a8EaAazEBdba8Ja8Ya8XazEUdba8Acefg8Aad9hmbkasc;Wbfcbcj;abz:ljjjb8AaJhzadhKinasc;WbfazydbcO4c;8ZGfgLaLydbcefBdbazcxfhzaKcufgKmbkcbhzcbhKinasc;WbfazfgLydbh5aLaKBdba5aKfhKazclfgzcj;ab9hmbkcbhzaJhKinasc;WbfaKydbcO4c;8ZGfgLaLydbgLcefBdbaeaLcdtfazBdbaKcxfhKadazcefgz9hmbkaoak9RgLci9Uh9ednalTmbcbhzaOhKinaKazBdbaKclfhKalazcefgz9hmbkkcbhTaccbalz:ljjjbhSaLcO9Uh9ha9ece4h9iasydwh6cbhHcbh8Ldnina8Maea8Lcdtfydbcx2fg8JIdwgya8W9EmeaHa9e9pmeJFFuuhIdna9iad9pmba8Maea9icdtfydbcx2fIdwJbb;aZNhIkdnayaI9ETmbaHa9h0mdkdnaSaXa8Jydlg9kcdtg0fydbg5fg9mRbbaSaXa8JydbgAcdtg9nfydbg9ofg9pRbbVmbdnana9ocdtfgzclfydbgKazydbgzSmbaKaz9Rh8Ea6azcitfhza8Ka5cx2fgYcwfhgaYclfhCa8Ka9ocx2fgicwfhraiclfh9qcbhKceh8AdnindnaOazydbcdtfydbgLa5SmbaOazclfydbcdtfydbgEa5SmbaLaESmba8KaEcx2fgEIdba8KaLcx2fgLIdbgR:tgIa9qIdbaLIdlg8R:tg8NNaiIdbaR:tg8PaEIdla8R:tgyN:taIaCIdba8R:tg8UNaYIdbaR:tg8SayN:tNayarIdbaLIdwg8R:tg8VNa8NaEIdwa8R:tgRN:tayagIdba8R:tg8RNa8UaRN:tNaRa8PNa8VaIN:taRa8SNa8RaIN:tNMMJbbbb9FmdkazcwfhzaKcefgKa8E6h8Aa8EaK9hmbkka8AceGTmba9icefh9ixeka8Fa5c8S2gLfgza8Fa9oc8S2gEfgKIdbazIdbMUdbazaKIdlazIdlMUdlazaKIdwazIdwMUdwazaKIdxazIdxMUdxazaKIdzazIdzMUdzazaKIdCazIdCMUdCazaKIdKazIdKMUdKazaKId3azId3MUd3azaKIdaazIdaMUdaazaKId8KazId8KMUd8KazaKIdyazIdyMUdydnaDTmbaqaLfgzaqaEfgKIdbazIdbMUdbazaKIdlazIdlMUdlazaKIdwazIdwMUdwazaKIdxazIdxMUdxazaKIdzazIdzMUdzazaKIdCazIdCMUdCazaKIdKazIdKMUdKazaKId3azId3MUd3azaKIdaazIdaMUdaazaKId8KazId8KMUd8KazaKIdyazIdyMUdya9ca9o2h8Aa9ca52hYawhKaDhEinaKaYfgzaKa8AfgLIdbazIdbMUdbazclfg8EaLclfIdba8EIdbMUdbazcwfg8EaLcwfIdba8EIdbMUdbazcxfgzaLcxfIdbazIdbMUdbaKczfhKaEcufgEmbkka8JcwfhKdndndndna3aAfgLRbbc9:fPdebdkaAhzinaOazcdtgzfa5BdbaQazfydbgzaA9hmbxikkaQa0fydbhzaQa9nfydbhAaOa9nfa9kBdbazh9kkaOaAcdtfa9kBdbka9pce86bba9mce86bbaKIdbgIa8Za8ZaI9DEh8ZaTcefhTcecdaLRbbceSEaHfhHka8Lcefg8Lad9hmbkkdnaTmbaohdxikdnalTmbcbhKaahzindnazydbgLcuSmbdnaKaOaLcdtg5fydbgL9hmbaaa5fydbhLkazaLBdbkazclfhzalaKcefgK9hmbkcbhKahhzindnazydbgLcuSmbdnaKaOaLcdtg5fydbgL9hmbaha5fydbhLkazaLBdbkazclfhzalaKcefgK9hmbkkcbhdabhzcbhEindnaOazydbcdtfydbgKaOazclfydbcdtfydbgLSmbaKaOazcwfydbcdtfydbg5SmbaLa5Smbabadcdtfg8EaKBdba8Ecwfa5Bdba8EclfaLBdbadcifhdkazcxfhzaEcifgEao6mbkadak9nmdxbkkasclfabadalaXz:cjjjbkdnaPTmbaPa8Z:rUdbkasyd2gzcdtascxffc98fhXdninazTmeaXydbcbyd;S1jjbH:bjjjbbaXc98fhXazcufhzxbkkasc;W;abf8Kjjjjbadk;Yieouabydlhvabydbclfcbaicdtz:ljjjbhoadci9UhrdnadTmbdnalTmbaehwadhDinaoalawydbcdtfydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbxdkkaehwadhDinaoawydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbkkdnaiTmbcbhDaohwinawydbhqawaDBdbawclfhwaqaDfhDaicufgimbkkdnadci6mbinaecwfydbhwaeclfydbhDaeydbhidnalTmbalawcdtfydbhwalaDcdtfydbhDalaicdtfydbhikavaoaicdtfgqydbcitfaDBdbavaqydbcitfawBdlaqaqydbcefBdbavaoaDcdtfgqydbcitfawBdbavaqydbcitfaiBdlaqaqydbcefBdbavaoawcdtfgwydbcitfaiBdbavawydbcitfaDBdlawawydbcefBdbaecxfhearcufgrmbkkabydbcbBdbk:2ldouv998Jjjjjbca9RglcFFF;7rBd3al9cFFF;7;3FF:;Fb83dCalcFFF97Bdzal9cFFF;7FFF:;u83dwdnadTmbaicd4hvdnabTmbavcdthocbhraehwinabarcx2fgiaearav2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinalcCfaifgDawaifIdbgqaDIdbgkakaq9EEUdbalcwfaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawaofhwarcefgrad9hmbxdkkavcdthrcbhwincbhiinalcCfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbalcwfaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbkkdnabTmbadTmbJbbbbJbbjZJbbbbalIdwalIdCgk:tgqaqJbbbb9DEgqalIdxalIdKgx:tgmamaq9DEgqalIdzalId3gm:tgPaPaq9DEgq:vaqJbbbb9BEhqinabaqabIdbak:tNUdbabclfgiaqaiIdbax:tNUdbabcwfgiaqaiIdbam:tNUdbabcxfhbadcufgdmbkkk8MbabaeadaialavcbcbcbcbcbaoarawaDz:bjjjbk8MbabaeadaialavaoarawaDaqakaxamaPz:bjjjbk;0Aowud99wue99iul998Jjjjjbc;Wb9Rgw8KjjjjbdndnarmbcbhDxekawcxfcbc;Kbz:ljjjb8Aawcuadcx2adc;v:Q;v:Qe0Ecbyd;O1jjbHjjjjbbgqBdxawceBd2aqaeadaiz:djjjbawcuadcdtadcFFFFi0Egkcbyd;O1jjbHjjjjbbgxBdzawcdBd2adcd4adfhmceheinaegicetheaiam6mbkcbhmawcuaicdtgPaicFFFFi0Ecbyd;O1jjbHjjjjbbgsBdCawciBd2dndnar:Zgz:rJbbbZMgH:lJbbb9p9DTmbaH:Ohexekcjjjj94hekaicufhOc:bwhAcbhCcbhXadhQinaChLaeaAgKcufaeaK9iEamgDcefaeaD9kEhYdndnadTmbaYcuf:YhHaqhiaxheadhmindndnaiIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhAxekcjjjj94hAkaAcCthAdndnaiclfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcqtaAVhAdndnaicwfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaeaAaCVBdbaicxfhiaeclfheamcufgmmbkascFeaPz:ljjjbhEcbh3cbh5indnaEaxa5cdtfydbgAcm4aA7c:v;t;h;Ev2gics4ai7aOGgmcdtfgCydbgecuSmbaeaASmbcehiinaEamaifaOGgmcdtfgCydbgecuSmeaicefhiaeaA9hmbkkaCaABdba3aecuSfh3a5cefg5ad9hmbxdkkascFeaPz:ljjjb8Acbh3kaDaYa3ar0giEhmaLa3aiEhCdna3arSmbaYaKaiEgAam9Rcd9imbdndnaXcl0mbdnaQ:ZgHaL:Zg8A:taY:Yg8EaD:Y:tg8Fa8EaK:Y:tgaa3:Zghaz:tNNNaHaz:taaNa8Aah:tNa8Aaz:ta8FNahaH:tNM:va8EMJbbbZMgH:lJbbb9p9DTmbaH:Ohexdkcjjjj94hexekamaAfcd9Theka3aQaiEhQaXcefgXcs9hmekkdndnaCmbcihicbhDxekcbhiawakcbyd;O1jjbHjjjjbbg5BdKawclBd2dndnadTmbamcuf:YhHaqhiaxheadhmindndnaiIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhAxekcjjjj94hAkaAcCthAdndnaiclfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcqtaAVhAdndnaicwfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaeaAaCVBdbaicxfhiaeclfheamcufgmmbkascFeaPz:ljjjbhEcbhDcbh3inaxa3cdtgYfydbgAcm4aA7c:v;t;h;Ev2gics4ai7hecbhidndninaEaeaOGgmcdtfgCydbgecuSmednaxaecdtgCfydbaASmbaicefgiamfheaiaO9nmekka5aCfydbhixekaCa3BdbaDhiaDcefhDka5aYfaiBdba3cefg3ad9hmbkcuaDc32giaDc;j:KM;jb0EhexekascFeaPz:ljjjb8AcbhDcbhekawaecbyd;O1jjbHjjjjbbgeBd3awcvBd2aecbaiz:ljjjbhCavcd4hxdnadTmbdnalTmbaxcdthEa5hAalheaqhmadhOinaCaAydbc32fgiamIdbaiIdbMUdbaiamclfIdbaiIdlMUdlaiamcwfIdbaiIdwMUdwaiaeIdbaiIdxMUdxaiaeclfIdbaiIdzMUdzaiaecwfIdbaiIdCMUdCaiaiIdKJbbjZMUdKaAclfhAaeaEfheamcxfhmaOcufgOmbxdkka5hmaqheadhAinaCamydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiaiIdxJbbbbMUdxaiaiIdzJbbbbMUdzaiaiIdCJbbbbMUdCaiaiIdKJbbjZMUdKamclfhmaecxfheaAcufgAmbkkdnaDTmbaChiaDheinaiaiIdbJbbbbJbbjZaicKfIdbgH:vaHJbbbb9BEgHNUdbaiclfgmaHamIdbNUdbaicwfgmaHamIdbNUdbaicxfgmaHamIdbNUdbaiczfgmaHamIdbNUdbaicCfgmaHamIdbNUdbaic3fhiaecufgembkkcbhAawcuaDcdtgYaDcFFFFi0Egicbyd;O1jjbHjjjjbbgeBdaawcoBd2awaicbyd;O1jjbHjjjjbbgEBd8KaecFeaYz:ljjjbh3dnadTmbaoaoNh8Aaxcdthxalheina8Aaec;C1jjbalEgmIdwaCa5ydbgOc32fgiIdC:tgHaHNamIdbaiIdx:tgHaHNamIdlaiIdz:tgHaHNMMNaqcwfIdbaiIdw:tgHaHNaqIdbaiIdb:tgHaHNaqclfIdbaiIdl:tgHaHNMMMhHdndna3aOcdtgifgmydbcuSmbaEaifIdbaH9ETmekamaABdbaEaifaHUdbka5clfh5aeaxfheaqcxfhqadaAcefgA9hmbkkaba3aYz:kjjjb8AcrhikaicdthiinaiTmeaic98fgiawcxffydbcbyd;S1jjbH:bjjjbbxbkkawc;Wbf8KjjjjbaDk:Odieui99iu8Jjjjjbca9RgicFFF;7rBd3ai9cFFF;7;3FF:;Fb83dCaicFFF97Bdzai9cFFF;7FFF:;u83dwdndnaembJbbjFhlJbbjFhvJbbjFhoxekadcd4cdthrcbhwincbhdinaicCfadfgDabadfIdbglaDIdbgvaval9EEUdbaicwfadfgDalaDIdbgvaval9DEUdbadclfgdcx9hmbkabarfhbawcefgwae9hmbkaiIdzaiId3:thoaiIdxaiIdK:thvaiIdwaiIdC:thlkJbbbbalalJbbbb9DEglavaval9DEglaoaoal9DEk9DeeuabcFeaicdtz:ljjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd;W1jjbgeabcifc98GfgbBd;W1jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd;W1jjbgeabcrfc94GfgbBd;W1jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd;W1jjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd;W1jjbfgdBd;W1jjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:bedbcjwk9Oebbbdbbbbbbbebbbeeebeebebbeeebebbbbbebebbbbbebbbdbbbbbbbbbbbbbbbeeeeebebbbbbebbbbbeebbbbbbbbbbbbbbbbbbbbbc;Owkxebbbdbbbj9Kbb";

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
			assert(target_error >= 0 && target_error <= 1);
			assert(Array.isArray(attribute_weights));
			assert(vertex_attributes_stride >= attribute_weights.length);
			assert(attribute_weights.length <= 16);

			var options = 0;
			for (var i = 0; i < (flags ? flags.length : 0); ++i) {
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
