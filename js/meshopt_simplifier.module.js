// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2024, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function() {
	// Built with clang version 16.0.0
	// Built from meshoptimizer 0.20
	var wasm = "b9H79Tebbbe9Fk9Geueu9Geub9Gbb9Gsuuuuuuuuuuuu99uueu9Gvuuuuub9Gluuuub9Gquuuuuuu99uueu9Gwuuuuuu99ueu9Giuuue999Gluuuueu9GiuuueuizsdilvoirwDbqqbeqlve9Weiiviebeoweuec:G;jekr:Tewo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95bl8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bvQ9TW79O9V9Wt9F79P9T9W29P9M959q9V9P9Ut7boX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wbra9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbwl79IV9RbDDwebcekdxmq;H9Usdbk;N6iKuY99Hu8Jjjjjbc;W;ab9Rgs8Kjjjjbascwfcbc;Kbz:ljjjb8AascualcefgzcdtazcFFFFi0Ecbyd1:jjjbHjjjjbbgHBdwasceBd9OasaHBdbascuadcitadcFFFFe0Ecbyd1:jjjbHjjjjbbgOBdxascdBd9OasaOBdlasaeadalcbz:cjjjbascualcdtgAalcFFFFi0EgCcbyd1:jjjbHjjjjbbgXBdzasciBd9OasaCcbyd1:jjjbHjjjjbbgQBdCasclBd9Oalcd4alfhLcehKinaKgzcethKazaL6mbkcbhYascuazcdtgKazcFFFFi0Ecbyd1:jjjbHjjjjbbgLBdKascvBd9OaLcFeaKz:ljjjbh8AdnalTmbavcd4hEazcufh3inaiaYaE2cdtfg5ydlgzcH4az7c:F:b:DD2a5ydbgzcH4az7c;D;O:B8J27a5ydwgzcH4az7c:3F;N8N27hLcbhzdndnina8AaLa3GgLcdtfg8EydbgKcuSmeaiaKaE2cdtfa5cxz:ojjjbTmdazcefgzaLfhLaza39nmbxdkka8EaYBdbaYhKkaXaYcdtfaKBdbaYcefgYal9hmbkcbhzaQhKinaKazBdbaKclfhKalazcefgz9hmbkcbhzaXhKaQhLindnazaKydbg3SmbaLaQa3cdtfg3ydbBdba3azBdbkaKclfhKaLclfhLalazcefgz9hmbkkcbh8Fa8Acbyd:m:jjjbH:bjjjbbasclBd9Oasalcbyd1:jjjbHjjjjbbg3BdKascvBd9OasaCcbyd1:jjjbHjjjjbbgzBd3ascoBd9OasaCcbyd1:jjjbHjjjjbbgKBdaascrBd9OazcFeaAz:ljjjbhaaKcFeaAz:ljjjbhhdnalTmbaOcwfhgindnaHa8FgLcefg8Fcdtfydbg5aHaLcdtgzfydbgKSmba5aK9RhAaOaKcitfh8Jahazfh8KaaazfhYcbhEindndna8JaEcitfydbg8EaL9hmbaYaLBdba8KaLBdbxekdnaHa8Ecdtg8LfgzclfydbgKazydbgzSmbaOazcitg5fydbaLSmeaKaz9Rh8Mazcu7aKfh8Aaga5fhKcbhzina8AazSmeazcefhzaKydbh5aKcwfhKa5aL9hmbkaza8M6mekaha8LfgzaLa8EazydbcuSEBdbaYa8EaLaYydbcuSEBdbkaEcefgEaA9hmbkka8Fal9hmbkaXhKaQhLahh5aahEcbhzindndnazaKydbg8E9hmbdnaqTmbaqazfRbbTmba3azfcl86bbxdkdnazaLydbg8E9hmbaEydbh8Edna5ydbg8Acu9hmba8Ecu9hmba3azfcb86bbxika3azfhYdnaza8ASmbaza8ESmbaYce86bbxikaYcl86bbxdkdnazaQa8Ecdtg8Afydb9hmbdna5ydbgYcuSmbazaYSmbaEydbgAcuSmbazaASmbaha8Afydbg8JcuSmba8Ja8ESmbaaa8Afydbg8AcuSmba8Aa8ESmbdnaXaYcdtfydbaXa8Acdtfydb9hmbaXaAcdtfydbaXa8Jcdtfydb9hmba3azfcd86bbxlka3azfcl86bbxika3azfcl86bbxdka3azfcl86bbxeka3azfa3a8EfRbb86bbkaKclfhKaLclfhLa5clfh5aEclfhEalazcefgz9hmbkamceGTmba3hzalhKindnazRbbce9hmbazcl86bbkazcefhzaKcufgKmbkkcualcx2alc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbh8Kascwfasyd9Ogzcdtfa8KBdbasazcefgABd9Oa8Kaialavz:djjjbdndnaDmbcbhvxekcbh8EascwfaAcdtfcuaDal2gKcdtaKcFFFFi0Ecbyd1:jjjbHjjjjbbgvBdbasazcdfgABd9OalTmbarcd4cdth8AaDcdthYavhEinaohzawhKaEhLaDh5inaLazIdbaKIdbNUdbazclfhzaKclfhKaLclfhLa5cufg5mbkaoa8AfhoaEaYfhEa8Ecefg8Eal9hmbkkascwfaAcdtfcualc8S2gzalc;D;O;f8U0EgLcbyd1:jjjbHjjjjbbgKBdbasaAcefg5Bd9OaKcbazz:ljjjbh8FdndndnaDTmbascwfa5cdtfaLcbyd1:jjjbHjjjjbbgoBdbasaAcdfgKBd9Oaocbazz:ljjjb8AascwfaKcdtfcuaDal2gzcltgKazcFFFFb0Ecbyd1:jjjbHjjjjbbgqBdbasaAcifBd9OaqcbaKz:ljjjb8AadmexdkcbhocbhqadTmekcbhEaehKindna8KaKclfydbg8Ecx2fgzIdba8KaKydbg8Acx2fgLIdbg8N:tgya8KaKcwfydbgYcx2fg5IdlaLIdlg8P:tgINa5Idba8N:tg8RazIdla8P:tg8SN:tgRaRNa8Sa5IdwaLIdwg8U:tg8VNaIazIdwa8U:tg8SN:tgIaINa8Sa8RNa8VayN:tgyayNMM:rg8RJbbbb9ETmbaRa8R:vhRaya8R:vhyaIa8R:vhIka8FaXa8Acdtfydbc8S2fgzaIa8R:rg8RaINNg8SazIdbMUdbazaya8RayNg8WNg8VazIdlMUdlazaRa8RaRNg8XNg8YazIdwMUdwaza8WaINg8WazIdxMUdxaza8XaINg8ZazIdzMUdzaza8XayNg8XazIdCMUdCazaIa8RaRa8UNaIa8NNa8PayNMM:mg8PNg8NNgIazIdKMUdKazaya8NNgyazId3MUd3azaRa8NNgRazIdaMUdaaza8Na8PNg8NazId8KMUd8Kaza8RazIdyMUdya8FaXa8Ecdtfydbc8S2fgza8SazIdbMUdbaza8VazIdlMUdlaza8YazIdwMUdwaza8WazIdxMUdxaza8ZazIdzMUdzaza8XazIdCMUdCazaIazIdKMUdKazayazId3MUd3azaRazIdaMUdaaza8NazId8KMUd8Kaza8RazIdyMUdya8FaXaYcdtfydbc8S2fgza8SazIdbMUdbaza8VazIdlMUdlaza8YazIdwMUdwaza8WazIdxMUdxaza8ZazIdzMUdzaza8XazIdCMUdCazaIazIdKMUdKazayazId3MUd3azaRazIdaMUdaaza8NazId8KMUd8Kaza8RazIdyMUdyaKcxfhKaEcifgEad6mbkcbh8Eaeh8AincbhKina3aeaKc:G1jjbfydba8EfcdtfydbgLfRbbhzdndna3a8AaKfydbg5fRbbgEc99fcFeGcpe0mbazceSmbazcd9hmekdnaEcufcFeGce0mbaaa5cdtfydbaL9hmekdnazcufcFeGce0mbahaLcdtfydba59hmekdnaEcv2azfc:W1jjbfRbbTmbaXaLcdtfydbaXa5cdtfydb0mekJbbacJbbjZazceSEh8RaEceShAa8KaeaKc:K1jjbfydba8Efcdtfydbcx2fhzdna8KaLcx2fgEIdwa8Ka5cx2fgYIdwg8P:tgIaINaEIdbaYIdbg8U:tgyayNaEIdlaYIdlg8S:tgRaRNMM:rg8NJbbbb9ETmbaIa8N:vhIaRa8N:vhRaya8N:vhykJbbaca8RaAEh8XdnazIdwa8P:tg8RaIa8RaINazIdba8U:tg8YayNaRazIdla8S:tg8WNMMg8VN:tg8Ra8RNa8Yaya8VN:tgIaINa8WaRa8VN:tgyayNMM:rgRJbbbb9ETmba8RaR:vh8RayaR:vhyaIaR:vhIka8FaXa5cdtfydbc8S2fgzaIa8Xa8NNgRaINNg8VazIdbMUdbazayaRayNg8XNg8YazIdlMUdlaza8RaRa8RNg8NNg8WazIdwMUdwaza8XaINg8XazIdxMUdxaza8NaINg8ZazIdzMUdzaza8NayNg80azIdCMUdCazaIaRa8Ra8PNaIa8UNa8SayNMM:mg8PNg8NNgIazIdKMUdKazaya8NNgyazId3MUd3aza8Ra8NNg8RazIdaMUdaaza8Na8PNg8NazId8KMUd8KazaRazIdyMUdya8FaXaLcdtfydbc8S2fgza8VazIdbMUdbaza8YazIdlMUdlaza8WazIdwMUdwaza8XazIdxMUdxaza8ZazIdzMUdzaza80azIdCMUdCazaIazIdKMUdKazayazId3MUd3aza8RazIdaMUdaaza8NazId8KMUd8KazaRazIdyMUdykaKclfgKcx9hmbka8Acxfh8Aa8Ecifg8Ead6mbkaDTmbcbh8AinJbbbbh8Ua8Kaea8AcdtfgzclfydbgYcx2fgKIdwa8KazydbgAcx2fgLIdwg8W:tgyayNaKIdbaLIdbg8Z:tg8Ra8RNaKIdlaLIdlg80:tgRaRNMMg8Xa8Kazcwfydbg8Jcx2fgzIdwa8W:tg8NNayaya8NNa8RazIdba8Z:tg8PNaRazIdla80:tg8SNMMgIN:tJbbbbJbbjZa8Xa8Na8NNa8Pa8PNa8Sa8SNMMg8YNaIaIN:tg8V:va8VJbbbb9BEg8VNh81a8YayNa8NaIN:ta8VNhBa8Xa8SNaRaIN:ta8VNh83a8YaRNa8SaIN:ta8VNhUa8Xa8PNa8RaIN:ta8VNh85a8Ya8RNa8PaIN:ta8VNh86a8Ra8SNa8PaRN:tgIaINaRa8NNa8SayN:tgIaINaya8PNa8Na8RN:tgIaINMM:r:rhIavaAaD2cdtfhKava8JaD2cdtfhLavaYaD2cdtfh5a8W:mh87a80:mh88a8Z:mh89cbhEaDh8EJbbbbh8SJbbbbh8VJbbbbh8XJbbbbh8YJbbbbh8WJbbbbh8ZJbbbbh80Jbbbbh8:JbbbbhZinasc;WbfaEfgzcwfaIaBa5IdbaKIdbg8N:tgRNa81aLIdba8N:tg8PNMgyNUdbazclfaIaUaRNa83a8PNMg8RNUdbazaIa86aRNa85a8PNMgRNUdbazcxfaIa87ayNa88a8RNa8Na89aRNMMMg8NNUdbaIaya8RNNa8YMh8YaIayaRNNa8WMh8WaIa8RaRNNa8ZMh8ZaIa8Na8NNNa8UMh8UaIaya8NNNa8SMh8SaIa8Ra8NNNa8VMh8VaIaRa8NNNa8XMh8XaIayayNNa80Mh80aIa8Ra8RNNa8:Mh8:aIaRaRNNaZMhZaKclfhKa5clfh5aLclfhLaEczfhEa8Ecufg8EmbkaoaXaAcdtfydbgKc8S2fgzaZazIdbMUdbaza8:azIdlMUdlaza80azIdwMUdwaza8ZazIdxMUdxaza8WazIdzMUdzaza8YazIdCMUdCaza8XazIdKMUdKaza8VazId3MUd3aza8SazIdaMUdaaza8UazId8KMUd8KazaIazIdyMUdyaoaXaYcdtfydbgYc8S2fgzaZazIdbMUdbaza8:azIdlMUdlaza80azIdwMUdwaza8ZazIdxMUdxaza8WazIdzMUdzaza8YazIdCMUdCaza8XazIdKMUdKaza8VazId3MUd3aza8SazIdaMUdaaza8UazId8KMUd8KazaIazIdyMUdyaoaXa8JcdtfydbgAc8S2fgzaZazIdbMUdbaza8:azIdlMUdlaza80azIdwMUdwaza8ZazIdxMUdxaza8WazIdzMUdzaza8YazIdCMUdCaza8XazIdKMUdKaza8VazId3MUd3aza8SazIdaMUdaaza8UazId8KMUd8KazaIazIdyMUdyaqaKaD2cltfh8EcbhKaDh5ina8EaKfgzasc;WbfaKfgLIdbazIdbMUdbazclfgEaLclfIdbaEIdbMUdbazcwfgEaLcwfIdbaEIdbMUdbazcxfgzaLcxfIdbazIdbMUdbaKczfhKa5cufg5mbkaqaYaD2cltfh8EcbhKaDh5ina8EaKfgzasc;WbfaKfgLIdbazIdbMUdbazclfgEaLclfIdbaEIdbMUdbazcwfgEaLcwfIdbaEIdbMUdbazcxfgzaLcxfIdbazIdbMUdbaKczfhKa5cufg5mbkaqaAaD2cltfh8EcbhKaDh5ina8EaKfgzasc;WbfaKfgLIdbazIdbMUdbazclfgEaLclfIdbaEIdbMUdbazcwfgEaLcwfIdbaEIdbMUdbazcxfgzaLcxfIdbazIdbMUdbaKczfhKa5cufg5mbka8Acifg8Aad6mbkkdnabaeSmbabaeadcdtz:kjjjb8AkasydbhncbhzdnalTmbanclfhzanydbh5a3hKalhEcbhLincbazydbg8Ea59RaKRbbcpeGEaLfhLaKcefhKazclfhza8Eh5aEcufgEmbkaLce4hzkcuadaz9Rcifgmcx2amc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbh8Mascwfasyd9Ogzcdtfa8MBdbasazcefgKBd9OascwfaKcdtfcuamcdtamcFFFFi0Ecbyd1:jjjbHjjjjbbgwBdbasazcdfgKBd9OascwfaKcdtfaCcbyd1:jjjbHjjjjbbgOBdbasazcifgKBd9OascwfaKcdtfalcbyd1:jjjbHjjjjbbgcBdbasazclfBd9OJbbbbh8Zdnadak9nmbdnamci6mbaxaxNh8WaDclth9ca8McwfhJJbbbbh8ZinasabadggalaXz:cjjjbabhAcbhHcbhYincbhzindnaXaAazfydbgLcdtg8AfydbgEaXabazc;m1jjbfydbaYfcdtfydbgKcdtfydbg8ESmba3aKfRbbgecv2a3aLfRbbg5fc;G1jjbfRbbg8La5cv2aefg8Jc;G1jjbfRbbgiVcFeGTmbdna8Jc:W1jjbfRbbTmba8EaE0mekdna5ae9hmba5cufcFeGce0mbaaa8AfydbaK9hmeka8MaHcx2fg5aKaLaicFeGgEEBdla5aLaKaEEBdba5aEa8LGcb9hBdwaHcefhHkazclfgzcx9hmbkdnaYcifgYag9pmbaAcxfhAaHcifam9nmekkdnaHmbaghdxikcbheinJbbbbJbbjZa8FaXa8Maecx2fg5ydlgEa5ydbg8Ea5ydwgKEgYcdtfydbg8Lc8S2gifgzIdygI:vaIJbbbb9BEazIdwa8Ka8EaEaKEg8Acx2fgKIdwgRNazIdzaKIdbg8NNazIdaMgIaIMMaRNazIdlaKIdlg8PNazIdCaRNazId3MgIaIMMa8PNazIdba8NNazIdxa8PNazIdKMgIaIMMa8NNazId8KMMM:lNh8XJbbbbJbbjZa8FaXa8Ecdtfydbgdc8S2gLfgzIdygI:vaIJbbbb9BEazIdwa8KaEcx2fgKIdwg8RNazIdzaKIdbg8UNazIdaMgIaIMMa8RNazIdlaKIdlg8SNazIdCa8RNazId3MgIaIMMa8SNazIdba8UNazIdxa8SNazIdKMgIaIMMa8UNazId8KMMM:lNh8Ya5cwfhAa5clfh8JdnaDTmbaoaLfgLIdwa8RNaLIdza8UNaLIdaMgIaIMMa8RNaLIdla8SNaLIdCa8RNaLId3MgIaIMMa8SNaLIdba8UNaLIdxa8SNaLIdKMgIaIMMa8UNaLId8KMMMhyavaEaD2cdtfhKaqadaD2cltfhzaLIdyh8VaDhLinaKIdbgIJbbb;aNazcxfIdba8RazcwfIdbNa8UazIdbNa8SazclfIdbNMMMNaIaINa8VNayMMhyaKclfhKazczfhzaLcufgLmbkaoaifgLIdwaRNaLIdza8NNaLIdaMgIaIMMaRNaLIdla8PNaLIdCaRNaLId3MgIaIMMa8PNaLIdba8NNaLIdxa8PNaLIdKMgIaIMMa8NNaLId8KMMMh8Rava8AaD2cdtfhKaqa8LaD2cltfhzaLIdyh8UaDhLinaKIdbgIJbbb;aNazcxfIdbaRazcwfIdbNa8NazIdbNa8PazclfIdbNMMMNaIaINa8UNa8RMMh8RaKclfhKazczfhzaLcufgLmbka8Xa8R:lMh8Xa8Yay:lMh8Yka8JaEa8Aa8Ya8X9FgzEBdba5a8EaYazEBdbaAa8Ya8XazEUdbaecefgeaH9hmbkasc;Wbfcbcj;abz:ljjjb8AaJhzaHhKinasc;WbfazydbcO4c;8ZGfgLaLydbcefBdbazcxfhzaKcufgKmbkcbhzcbhKinasc;WbfazfgLydbh5aLaKBdba5aKfhKazclfgzcj;ab9hmbkcbhzaJhKinasc;WbfaKydbcO4c;8ZGfgLaLydbgLcefBdbawaLcdtfazBdbaKcxfhKaHazcefgz9hmbkagak9RgLci9Uh9ednalTmbcbhzaOhKinaKazBdbaKclfhKalazcefgz9hmbkkcbhTaccbalz:ljjjbhSaLcO9Uh9ha9ece4h9iasydlh6cbhicbh8Jdnina8Mawa8Jcdtfydbcx2fgAIdwgya8W9Emeaia9e9pmeJFFuuhIdna9iaH9pmba8Mawa9icdtfydbcx2fIdwJbb;aZNhIkdnayaI9ETmbaia9h0mdkdnaSaXaAydlg9kcdtg0fydbg5fg9mRbbaSaXaAydbgYcdtg9nfydbg9ofg9pRbbVmbdnana9ocdtfgzclfydbgKazydbgzSmbaKaz9Rh8Ea6azcitfhza8Ka5cx2fg8Acwfhda8AclfhCa8Ka9ocx2fg8Lcwfhra8Lclfh9qcbhKcehednindnaOazydbcdtfydbgLa5SmbaOazclfydbcdtfydbgEa5SmbaLaESmba8KaEcx2fgEIdba8KaLcx2fgLIdbg8R:tgIa9qIdbaLIdlgR:tg8NNa8LIdba8R:tg8PaEIdlaR:tgyN:taIaCIdbaR:tg8UNa8AIdba8R:tg8SayN:tNayarIdbaLIdwgR:tg8VNa8NaEIdwaR:tg8RN:tayadIdbaR:tgRNa8Ua8RN:tNa8Ra8PNa8VaIN:ta8Ra8SNaRaIN:tNMMJbbbb9FmdkazcwfhzaKcefgKa8E6hea8EaK9hmbkkaeceGTmba9icefh9ixeka8Fa5c8S2gLfgza8Fa9oc8S2gEfgKIdbazIdbMUdbazaKIdlazIdlMUdlazaKIdwazIdwMUdwazaKIdxazIdxMUdxazaKIdzazIdzMUdzazaKIdCazIdCMUdCazaKIdKazIdKMUdKazaKId3azId3MUd3azaKIdaazIdaMUdaazaKId8KazId8KMUd8KazaKIdyazIdyMUdydnaDTmbaoaLfgzaoaEfgKIdbazIdbMUdbazaKIdlazIdlMUdlazaKIdwazIdwMUdwazaKIdxazIdxMUdxazaKIdzazIdzMUdzazaKIdCazIdCMUdCazaKIdKazIdKMUdKazaKId3azId3MUd3azaKIdaazIdaMUdaazaKId8KazId8KMUd8KazaKIdyazIdyMUdya9ca9o2hea9ca52h8AaqhKaDhEinaKa8AfgzaKaefgLIdbazIdbMUdbazclfg8EaLclfIdba8EIdbMUdbazcwfg8EaLcwfIdba8EIdbMUdbazcxfgzaLcxfIdbazIdbMUdbaKczfhKaEcufgEmbkkaAcwfhKdndndndna3aYfgLRbbc9:fPdebdkaYhzinaOazcdtgzfa5BdbaQazfydbgzaY9hmbxikkaQa0fydbhzaQa9nfydbhYaOa9nfa9kBdbazh9kkaOaYcdtfa9kBdbka9pce86bba9mce86bbaKIdbgIa8Za8ZaI9DEh8ZaTcefhTcecdaLRbbceSEaifhika8Jcefg8JaH9hmbkkdnaTmbaghdxikdnalTmbcbhKaahzindnazydbgLcuSmbdnaKaOaLcdtg5fydbgL9hmbaaa5fydbhLkazaLBdbkazclfhzalaKcefgK9hmbkcbhKahhzindnazydbgLcuSmbdnaKaOaLcdtg5fydbgL9hmbaha5fydbhLkazaLBdbkazclfhzalaKcefgK9hmbkkcbhdabhzcbhEindnaOazydbcdtfydbgKaOazclfydbcdtfydbgLSmbaKaOazcwfydbcdtfydbg5SmbaLa5Smbabadcdtfg8EaKBdba8EclfaLBdba8Ecwfa5BdbadcifhdkazcxfhzaEcifgEag6mbkadak9nmdxbkkasabadalaXz:cjjjbkdnaPTmbaPa8Z:rUdbkasyd9Ogzcdtascwffc98fhXdninazTmeaXydbcbyd:m:jjjbH:bjjjbbaXc98fhXazcufhzxbkkasc;W;abf8Kjjjjbadk;:ieouabydlhvabydbclfcbaicdtz:ljjjbhoadci9UhrdnadTmbdnalTmbaehwadhDinaoalawydbcdtfydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbxdkkaehwadhDinaoawydbcdtfgqaqydbcefBdbawclfhwaDcufgDmbkkdnaiTmbcbhDaohwinawydbhqawaDBdbawclfhwaqaDfhDaicufgimbkkdnadci6mbarcearce0EhdinaecwfydbhwaeclfydbhDaeydbhidnalTmbalawcdtfydbhwalaDcdtfydbhDalaicdtfydbhikavaoaicdtfgqydbcitfaDBdbavaqydbcitfawBdlaqaqydbcefBdbavaoaDcdtfgqydbcitfawBdbavaqydbcitfaiBdlaqaqydbcefBdbavaoawcdtfgwydbcitfaiBdbavawydbcitfaDBdlawawydbcefBdbaecxfheadcufgdmbkkabydbcbBdbk:Zldouv998Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdnadTmbaicd4hvdnabTmbavcdthocbhraehwinabarcx2fgiaearav2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinalczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawaofhwarcefgrad9hmbxdkkavcdthrcbhwincbhiinalczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbkkdnabTmbadTmbJbbbbJbbjZalIdbalIdzgk:tJbbbb:xgqalIdlalIdCgx:tgmamaq9DEgqalIdwalIdKgm:tgPaPaq9DEgq:vaqJbbbb9BEhqinabaqabIdbak:tNUdbabclfgiaqaiIdbax:tNUdbabcwfgiaqaiIdbam:tNUdbabcxfhbadcufgdmbkkk8MbabaeadaialavcbcbcbcbcbaoarawaDz:bjjjbk8MbabaeadaialavaoarawaDaqakaxamaPz:bjjjbk;lOowud99wue99iul998Jjjjjbc;Wb9Rgw8KjjjjbdndnarmbcbhDxekawcwfcbc;Kbz:ljjjb8Aawcuadcx2adc;v:Q;v:Qe0Ecbyd1:jjjbHjjjjbbgqBdwawceBd9Oaqaeadaiz:djjjbawcuadcdtadcFFFFi0Egkcbyd1:jjjbHjjjjbbgxBdxawcdBd9Oadcd4adfhmceheinaegicetheaiam6mbkcbhmawcuaicdtgPaicFFFFi0Ecbyd1:jjjbHjjjjbbgsBdzawciBd9Odndnar:Zgz:rJbbbZMgH:lJbbb9p9DTmbaH:Ohexekcjjjj94hekaicufhOc:bwhAcbhCcbhXadhQinaChLaeaAgKcufaeaK9iEamgDcefaeaD9kEhYdndnadTmbaYcuf:YhHaqhiaxheadhmindndnaiIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhAxekcjjjj94hAkaAcCthAdndnaiclfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcqtaAVhAdndnaicwfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaeaAaCVBdbaicxfhiaeclfheamcufgmmbkascFeaPz:ljjjbhEcbh3cbh5indnaEaxa5cdtfydbgAcm4aA7c:v;t;h;Ev2gics4ai7aOGgmcdtfgCydbgecuSmbaeaASmbcehiinaEamaifaOGgmcdtfgCydbgecuSmeaicefhiaeaA9hmbkkaCaABdba3aecuSfh3a5cefg5ad9hmbxdkkascFeaPz:ljjjb8Acbh3kaDaYa3ar0giEhmaLa3aiEhCdna3arSmbaYaKaiEgAam9Rcd9imbdndnaXcl0mbdnaQ:ZgHaL:Zg8A:taY:Yg8EaD:Y:tg8Fa8EaK:Y:tgaa3:Zghaz:tNNNaHaz:taaNa8Aah:tNa8Aaz:ta8FNahaH:tNM:va8EMJbbbZMgH:lJbbb9p9DTmbaH:Ohexdkcjjjj94hexekamaAfcd9Theka3aQaiEhQaXcefgXcs9hmekkdndnaCmbcihicbhDxekcbhiawakcbyd1:jjjbHjjjjbbg5BdCawclBd9OdndnadTmbamcuf:YhHaqhiaxheadhmindndnaiIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhAxekcjjjj94hAkaAcCthAdndnaiclfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaCcqtaAVhAdndnaicwfIdbaHNJbbbZMg8A:lJbbb9p9DTmba8A:OhCxekcjjjj94hCkaeaAaCVBdbaicxfhiaeclfheamcufgmmbkascFeaPz:ljjjbhEcbhDcbh3inaxa3cdtgYfydbgAcm4aA7c:v;t;h;Ev2gics4ai7hecbhidndninaEaeaOGgmcdtfgCydbgecuSmednaxaecdtgCfydbaASmbaicefgiamfheaiaO9nmekka5aCfydbhixekaCa3BdbaDhiaDcefhDka5aYfaiBdba3cefg3ad9hmbkcuaDc32giaDc;j:KM;jb0EhexekascFeaPz:ljjjb8AcbhDcbhekawaecbyd1:jjjbHjjjjbbgeBdKawcvBd9Oaecbaiz:ljjjbhOavcd4hxdnadTmbaxcdth3a5hmalhAaqheadhEinaOamydbc32fgiaeIdbaiIdbMUdbaiaeclfIdbaiIdlMUdlaiaecwfIdbaiIdwMUdwaiaAc;81jjbalEgCIdbaiIdxMUdxaiaCIdlaiIdzMUdzaiaCIdwaiIdCMUdCaiaiIdKJbbjZMUdKamclfhmaAa3fhAaecxfheaEcufgEmbkkdnaDTmbaOcxfhiaDheinaictfgmamIdbJbbbbJbbjZaicxfIdbgH:vaHJbbbb9BEgHNUdbaic94fgmaHamIdbNUdbaic98fgmaHamIdbNUdbaiaHaiIdbNUdbaiclfgmaHamIdbNUdbaicwfgmaHamIdbNUdbaic3fhiaecufgembkkcbhAawcuaDcdtgYaDcFFFFi0Egicbyd1:jjjbHjjjjbbgeBd3awcoBd9Oawaicbyd1:jjjbHjjjjbbgEBdaaecFeaYz:ljjjbh3dnadTmbaoaoNh8Aaxcdthxalheina8Aaec;81jjbalEgmIdwaOa5ydbgCc32fgiIdC:tgHaHNamIdbaiIdx:tgHaHNamIdlaiIdz:tgHaHNMMNaqcwfIdbaiIdw:tgHaHNaqIdbaiIdb:tgHaHNaqclfIdbaiIdl:tgHaHNMMMhHdndna3aCcdtgifgmydbcuSmbaEaifIdbaH9ETmekamaABdbaEaifaHUdbka5clfh5aeaxfheaqcxfhqadaAcefgA9hmbkkaba3aYz:kjjjb8AcrhikaicdthiinaiTmeaic98fgiawcwffydbcbyd:m:jjjbH:bjjjbbxbkkawc;Wbf8KjjjjbaDk:Qdidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgoaDIdbgrarao9EEUdbaladfgDaoaDIdbgrarao9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkavJbbbb:xgvaoaoav9DEgoararao9DEk9DeeuabcFeaicdtz:ljjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcifc98GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd:q:jjjbgeabcrfc94GfgbBd:q:jjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd:q:jjjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd:q:jjjbfgdBd:q:jjjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:Iedbcjwk1eFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbebbbdbbbbbbbebbbeeebeebebbeeebebbbbbebebbbbbebbbdbbbbbbbbbbbbbbbeeeeebebbbbbebbbbbeebbbbbbbbbbbbbbbbbbbbbc1Dkxebbbdbbb:G9Kbb";

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

export { MeshoptSimplifier };
