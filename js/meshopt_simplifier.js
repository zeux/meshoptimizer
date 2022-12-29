// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2022, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptSimplifier = (function() {
	"use strict";

	// Built with clang version 15.0.6
	// Built from meshoptimizer 0.18
	var wasm = "b9H79Tebbbe9yk9Geueu9Geub9Gbb9Gquuuuuuu99uueu9GPuuuuuuuuu99uuuueu9Gvuuuuub9Gluuuue999Gduub9Giuuue999Gluuuueu9GiuuueuisPdilvorwDbqqbeqlve9Weiiviebeoweuecj;jekrNero9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bbz9TW79O9V9Wt9F79P9T9W29P9M95be8E9TW79O9V9Wt9F79P9T9W29P9M959x9Pt9OcttV9P9I91tW7bdX9TW79O9V9Wt9F79P9T9W29P9M959t9J9H2Wboa9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9Wbrl79IV9RbwDwebcekdkxq:E9nPdbk8KbabaeadaialavcbcbaoarawaDcbcbz:cjjjbk:mUiKuY99Pu8Jjjjjbcj;bb9RgP8KjjjjbaPcKfcbc;Kbz:kjjjb8AaPcualcdtgsalcFFFFi0Egzcbyd;S1jjbHjjjjbbgHBdKaPceBd94aPaHBdwaPazcbyd;S1jjbHjjjjbbgOBd3aPcdBd94aPaOBdxaPcuadcitadcFFFFe0Ecbyd;S1jjbHjjjjbbgABdaaPciBd94aPaABdzaPcwfaeadalcbz:djjjbaPazcbyd;S1jjbHjjjjbbgCBd8KaPclBd94aPazcbyd;S1jjbHjjjjbbgXBdyaPcvBd94arcd4hQalcd4alfhLcehKinaKgrcethKaraL6mbkcbhYaPcuarcdtgKarcFFFFi0Ecbyd;S1jjbHjjjjbbgLBd8SaPcoBd94aLcFeaKz:kjjjbh8AdnalTmbavcd4hEarcufh3inaiaYaE2cdtfg5ydlgrcH4ar7c:F:b:DD2a5ydbgrcH4ar7c;D;O:B8J27a5ydwgrcH4ar7c:3F;N8N27hLcbhrdndnina8AaLa3GgLcdtfg8EydbgKcuSmeaiaKaE2cdtfa5cxz:njjjbTmdarcefgraLfhLara39nmbxdkka8EaYBdbaYhKkaCaYcdtfaKBdbaYcefgYal9hmbkcbhraXhKinaKarBdbaKclfhKalarcefgr9hmbkcbhraChKaXhLindnaraKydbg3SmbaLaXa3cdtfg3ydbBdba3arBdbkaKclfhKaLclfhLalarcefgr9hmbkkcbhLaPalcbyd;S1jjbHjjjjbbg5Bd8WaPcrBd94aPazcbyd;S1jjbHjjjjbbgrBd80aPcwBd94aPazcbyd;S1jjbHjjjjbbgKBdUaPcDBd94arcFeasz:kjjjbh8FaKcFeasz:kjjjbhadnalTmbaAcwfhhindnaHaLcdtgrfydbggTmbaAaOarfydbcitfh8Jaaarfh8Ka8FarfhYcbhEindndna8JaEcitfydbg8AaL9hmbaYaLBdba8KaLBdbxekdnaHa8Acdtgsfydbg8LTmbaAaOasfydbcitgrfydbaLSmea8Lcufh8EaharfhKcbhrina8EarSmearcefhraKydbh3aKcwfhKa3aL9hmbkara8L6mekaaasfgraLa8AarydbcuSEBdbaYa8AaLaYydbcuSEBdbkaEcefgEag9hmbkkaLcefgLal9hmbkaChKaXhLaah3a8FhEcbhrindndnaraKydbg8E9hmbdnaraLydbg8E9hmbaEydbh8Edna3ydbg8Acu9hmba8Ecu9hmba5arfcb86bbxika5arfhYdnara8ASmbara8ESmbaYce86bbxikaYcl86bbxdkdnaraXa8Ecdtg8Afydb9hmbdna3ydbgYcuSmbaraYSmbaEydbgscuSmbarasSmbaaa8AfydbggcuSmbaga8ESmba8Fa8Afydbg8AcuSmba8Aa8ESmbdnaCaYcdtfydbaCa8Acdtfydb9hmbaCascdtfydbaCagcdtfydb9hmba5arfcd86bbxlka5arfcl86bbxika5arfcl86bbxdka5arfcl86bbxeka5arfa5a8EfRbb86bbkaKclfhKaLclfhLa3clfh3aEclfhEalarcefgr9hmbkaqceGTmba5hralhKindnarRbbce9hmbarcl86bbkarcefhraKcufgKmbkkcualc8S2alc;D;O;f8U0Ecbyd;S1jjbHjjjjbbh8KaPcKfaPyd94gYcdtfa8KBdbaPaYcefgsBd94a8Kaialavz:ejjjb8AdnalTmbdnamTmbaQcdth8Aa8KcxfhEcbh8Eina8Ka8Ec8S2fgr9cb83dxarc8Kf9cb83dbarc3f9cb83dbarcCf9cb83dbaohraxhKaEhLamh3inaLarIdbaKIdbNUdbarclfhraKclfhKaLclfhLa3cufg3mbkaoa8AfhoaEc8SfhEa8Ecefg8Eal9hmbxdkka8KcxfhralhKinar9cb83dbarcKf9cb83dbarczf9cb83dbarcwf9cb83dbarc8SfhraKcufgKmbkkaPcKfascdtfcualc:Se2gralc;0:l;0k0Ecbyd;S1jjbHjjjjbbgKBdbaPaYcdfBd94aKcbarz:kjjjbhodnadTmba8Kcxfh8EaPcjefc8Sfh8Lcbh8Aindna8Kaea8AcdtfgrclfydbgYc8S2g3fgKIdba8Karydbgsc8S2gEfgLIdbg8M:tg8Na8Karcwfydbggc8S2g8JfgrIdlaLIdlgy:tg8PNarIdba8M:tgIaKIdlay:tg8RN:tg8Sa8SNa8RarIdwaLIdwgR:tg8UNa8PaKIdwaR:tg8VN:tg8Wa8WNa8VaINa8Ua8NN:tg8Xa8XNMM:rg8YJbbbb9ETmba8Sa8Y:vh8Sa8Xa8Y:vh8Xa8Wa8Y:vh8WkaPa8Y:rg8YUd:Oecbhra8Lcbcjez:kjjjb8Aa8Va8VNa8Na8NNa8Ra8RNMMg8Za8UNa8Va8Va8UNa8NaINa8Ra8PNMMg80N:tJbbbbJbbjZa8Za8Ua8UNaIaINa8Pa8PNMMg81Na80a80N:tgB:vaBJbbbb9BEgBNh83a81a8VNa8Ua80N:taBNhUa8Za8PNa8Ra80N:taBNh85a81a8RNa8Pa80N:taBNh86a8ZaINa8Na80N:taBNh87a81a8NNaIa80N:taBNh88a8Ya8SaRNa8Wa8MNaya8XNMM:mg8NNg80a8NNhIa8Sa80Nh8Ra8Xa80Nh8Ua8Wa80Nh8Va8EaEfhLa8Ea3fh3a8Ea8JfhEa8Ya8SNg80a8XNhBa80a8WNh8Za8Ya8XNg8Na8WNh81a8Sa80Nh8Sa8Xa8NNh89a8Wa8Ya8WNNh8:aR:mhRay:mhya8M:mh8MinaPcjefarfgKc;Sbfa8YaUa3arfIdbaLarfIdbg8N:tg80Na83aEarfIdba8N:tg8PNMg8WNUdbaKc;mbfa8Ya86a80Na85a8PNMg8XNUdbaKc8Sfa8Ya88a80Na87a8PNMg80NUdbaKc:mefa8YaRa8WNaya8XNa8Na8Ma80NMMMg8NNUdbaPa8Ya80a80NNa8:Mg8:Udjea8Ya8Wa8XNNaBMhBa8Ya8Wa80NNa8ZMh8Za8Ya8Xa80NNa81Mh81a8Ya8Na8NNNaIMhIa8Ya8Wa8NNNa8RMh8Ra8Ya8Xa8NNNa8UMh8Ua8Ya80a8NNNa8VMh8Va8Ya8Wa8WNNa8SMh8Sa8Ya8Xa8XNNa89Mh89arclfgrca9hmbkaPaIUd:KeaPa8RUd:GeaPa8UUd:CeaPa8VUd:yeaPaBUdNeaPa8ZUd:qeaPa81Ud:meaPa8SUd1eaPa89Ud:eeaoaCascdtfydbc:Se2faPcjefz:fjjjbaoaCaYcdtfydbc:Se2faPcjefz:fjjjbaoaCagcdtfydbc:Se2faPcjefz:fjjjba8Acifg8Aad6mbkaPcjefc8Sfhscbh8Aincbh8Eina5aea8Ecdtc:81jjbfydbg3a8AfcdtfydbgYfRbbhrdndna5aea8Ea8AfcdtfydbgKfRbbgLc99fcFeGcpe0mbarceSmbarcd9hmekdnaLcufcFeGce0mba8FaKcdtfydbaY9hmekdnarcufcFeGce0mbaaaYcdtfydbaK9hmekdnaLcv2arfc:G1jjbfRbbTmbaCaYcdtfydbaCaKcdtfydb0mekJbbacJbbjZarceSEh80aLceShEa8Kaea3cdtc:81jjbfydba8Afcdtfydbc8S2fhrdna8KaYc8S2fgLIdwa8KaKc8S2fg3Idwg8P:tg8Ya8YNaLIdba3IdbgI:tg8Wa8WNaLIdla3Idlg8R:tg8Xa8XNMM:rg8NJbbbb9ETmba8Ya8N:vh8Ya8Xa8N:vh8Xa8Wa8N:vh8WkJbbaca80aEEh8VdnarIdwa8P:tg80a8Ya80a8YNarIdbaI:tgBa8WNa8XarIdla8R:tg8ZNMMg8UN:tg80a80NaBa8Wa8UN:tg8Ya8YNa8Za8Xa8UN:tg8Wa8WNMM:rg8XJbbbb9ETmba80a8X:vh80a8Wa8X:vh8Wa8Ya8X:vh8YkaPa8Va8NNg8XUd:OeaPa8Xa80Ng8Na8WNg8UUdNeaPa8Na8YNg8VUd:qeaPa8Xa8WNg81a8YNgBUd:meaPa80a8NNg8ZUd1eaPa8Wa81Ng81Ud:eeaPa8Ya8Xa8YNNg8SUdjeaPa8Xa80a8PNa8YaINa8Ra8WNMM:mg8PNg8Na8PNg8PUd:KeaPa80a8NNg80Ud:GeaPa8Wa8NNg8WUd:CeaPa8Ya8NNg8YUd:yecbhrascbcjez:kjjjb8AaoaCaKcdtfydbc:Se2fgEa8SaEIdbMUdbaEa81aEIdlMUdlaEa8ZaEIdwMUdwaEaBaEIdxMUdxaEa8VaEIdzMUdzaEa8UaEIdCMUdCaEa8YaEIdKMUdKaEa8WaEId3MUd3aEa80aEIdaMUdaaEa8PaEId8KMUd8KaEa8XaEIdyMUdyinaEarfgKc8Sfg3aPcjefarfgLc8SfIdba3IdbMUdbaKc;mbfg3aLc;mbfIdba3IdbMUdbaKc;Sbfg3aLc;SbfIdba3IdbMUdbaKc:mefgKaLc:mefIdbaKIdbMUdbarclfgrca9hmbkaoaCaYcdtfydbc:Se2fgEa8SaEIdbMUdbaEa81aEIdlMUdlaEa8ZaEIdwMUdwaEaBaEIdxMUdxaEa8VaEIdzMUdzaEa8UaEIdCMUdCaEa8YaEIdKMUdKaEa8WaEId3MUd3aEa80aEIdaMUdaaEa8PaEId8KMUd8KaEa8XaEIdyMUdycbhrinaEarfgKc8Sfg3aPcjefarfgLc8SfIdba3IdbMUdbaKc;mbfg3aLc;mbfIdba3IdbMUdbaKc;Sbfg3aLc;SbfIdba3IdbMUdbaKc:mefgKaLc:mefIdbaKIdbMUdbarclfgrca9hmbkka8Ecefg8Eci9hmbka8Acifg8Aad6mbkkdnabaeSmbabaeadcdtz:jjjjb8Akcuadcx2adc;v:Q;v:Qe0Ecbyd;S1jjbHjjjjbbhhaPcKfaPyd94grcdtfahBdbaParcefgKBd94aPcKfaKcdtfcuadcdtadcFFFFi0Ecbyd;S1jjbHjjjjbbgvBdbaParcdfgKBd94aPcKfaKcdtfazcbyd;S1jjbHjjjjbbgiBdbaParcifgKBd94aPcKfaKcdtfalcbyd;S1jjbHjjjjbbgZBdbaParclfBd94JbbbbhBdnadaw9nmbaDaDNh8Vahcwfhna8KcxfhxJbbbbhBinaPcwfabadgmalaCz:djjjbcbh8LabhYcbhsincbhrindnaCaYarfydbgLcdtgefydbgEaCabarc:81jjbfydbasfcdtfydbgKcdtfydbg8ESmba5aKfRbbg8Acv2a5aLfRbbg3fc;q1jjbfRbbg8Ja3cv2a8Afggc;q1jjbfRbbgdVcFeGTmbdnagc:G1jjbfRbbTmba8EaE0mekdna3a8A9hmba3cufcFeGce0mba8FaefydbaK9hmekaha8Lcx2fg3aKaLadcFeGgEEBdla3aLaKaEEBdba3aEa8JGcb9hBdwa8Lcefh8Lkarclfgrcx9hmbkaYcxfhYascifgsam6mbkdndna8LTmbcbheinaoaCahaecx2fgEydbg8Ecdtfydbc:Se2fgLIdwa8KaEydlg8Ac8S2gKfgrIdwg8XNaLIdzarIdbg80NaLIdaMg8Ya8YMMa8XNaLIdlarIdlg8NNaLIdCa8XNaLId3Mg8Ya8YMMa8NNaLIdba80NaLIdxa8NNaLIdKMg8Ya8YMMa80NaLId8KMMMh8WaCa8Aa8EaEydwgYEgscdtfydbhgaEcwfh8JaEclfhdaxaKfh3aLIdyh8Pcbhrina3arfIdbg8YJbbb;aNaLarfgKc:mefIdba8XaKc;SbfIdbNa80aKc8SfIdbNa8NaKc;mbfIdbNMMMNa8Ya8YNa8PNa8WMMh8Warclfgrca9hmbkaoagc:Se2fgLIdwa8Ka8Ea8AaYEgYc8S2gKfgrIdwg80NaLIdzarIdbg8NNaLIdaMg8Ya8YMMa80NaLIdlarIdlg8PNaLIdCa80NaLId3Mg8Ya8YMMa8PNaLIdba8NNaLIdxa8PNaLIdKMg8Ya8YMMa8NNaLId8KMMMh8XaxaKfh3aLIdyhIcbhrina3arfIdbg8YJbbb;aNaLarfgKc:mefIdba80aKc;SbfIdbNa8NaKc8SfIdbNa8PaKc;mbfIdbNMMMNa8Ya8YNaINa8XMMh8Xarclfgrca9hmbkada8AaYa8W:lg8Ya8X:lg8W9FgrEBdbaEa8EasarEBdba8Ja8Ya8WarEUdbaecefgea8L9hmbkaPcjefcbcj;abz:kjjjb8Aanhra8LhKinaPcjefarydbcO4c;8ZGfgLaLydbcefBdbarcxfhraKcufgKmbkcbhrcbhKinaPcjefarfgLydbh3aLaKBdba3aKfhKarclfgrcj;ab9hmbkcbhranhKinaPcjefaKydbcO4c;8ZGfgLaLydbgLcefBdbavaLcdtfarBdbaKcxfhKa8Larcefgr9hmbkamaw9RgLci9UhcdnalTmbcbhraihKinaKarBdbaKclfhKalarcefgr9hmbkkcbh9caZcbalz:kjjjbhJaLcO9Uh9eacce4hTcbhScbhsdninahavascdtfydbcx2fgYIdwg8Wa8V9EmeaSac9pmeJFFuuh8YdnaTa8L9pmbahavaTcdtfydbcx2fIdwJbb;aZNh8Ykdna8Wa8Y9ETmbaSa9e0mdkdnaJaCaYydlg9hcdtg9ifydbgLfg6RbbaJaCaYydbgecdtg9kfydbg0fg9mRbbVmbdnaHa0cdtgrfydbg3TmbaAaOarfydbcitfhra8KaLc8S2fggcwfhdagclfhza8Ka0c8S2fg8Jcwfhqa8JclfhQcbhKceh8AdnindnaiarydbcdtfydbgEaLSmbaiarclfydbcdtfydbg8EaLSmba8Ka8Ec8S2fg8EIdba8KaEc8S2fgEIdbg8X:tg8YaQIdbaEIdlg80:tg8NNa8JIdba8X:tg8Pa8EIdla80:tg8WN:ta8YazIdba80:tgINagIdba8X:tg8Ra8WN:tNa8WaqIdbaEIdwg80:tg8UNa8Na8EIdwa80:tg8XN:ta8WadIdba80:tg80NaIa8XN:tNa8Xa8PNa8Ua8YN:ta8Xa8RNa80a8YN:tNMMJbbbb9DmdkarcwfhraKcefgKa36h8Aa3aK9hmbkka8AceGTmbaTcefhTxekaYcwfhKaoaLc:Se2faoa0c:Se2fz:fjjjbdndndndna5aefg3Rbbc9:fPdebdkaehrinaiarcdtgrfaLBdbaXarfydbgrae9hmbxikkaXa9ifydbhraXa9kfydbheaia9kfa9hBdbarh9hkaiaecdtfa9hBdbka9mce86bba6ce86bbaKIdbg8YaBaBa8Y9DEhBa9ccefh9ccecda3RbbceSEaSfhSkascefgsa8L9hmbkka9cTmbdnalTmbcbhKa8FhrindnarydbgLcuSmbdnaKaiaLcdtg3fydbgL9hmba8Fa3fydbhLkaraLBdbkarclfhralaKcefgK9hmbkcbhKaahrindnarydbgLcuSmbdnaKaiaLcdtg3fydbgL9hmbaaa3fydbhLkaraLBdbkarclfhralaKcefgK9hmbkkcbhdabhrcbhEindnaiarydbcdtfydbgKaiarclfydbcdtfydbgLSmbaKaiarcwfydbcdtfydbg3SmbaLa3Smbabadcdtfg8EaKBdba8EclfaLBdba8Ecwfa3BdbadcifhdkarcxfhraEcifgEam9pmdxbkkamhdxdkadaw0mbkkdnakTmbakaB:rUdbkaPyd94grcdtaPcKffc98fhKdninarTmeaKydbcbyd;W1jjbH:bjjjbbaKc98fhKarcufhrxbkkaPcj;bbf8Kjjjjbadk;pleouabydbcbaicdtz:kjjjb8Aadci9UhvdnadTmbabydbhodnalTmbaehradhwinaoalarydbcdtfydbcdtfgDaDydbcefBdbarclfhrawcufgwmbxdkkaehradhwinaoarydbcdtfgDaDydbcefBdbarclfhrawcufgwmbkkdnaiTmbabydbhrabydlhwcbhDaihoinawaDBdbawclfhwarydbaDfhDarclfhraocufgombkkdnadci6mbavceavce0EhqabydlhvabydwhrinaecwfydbhwaeclfydbhDaeydbhodnalTmbalawcdtfydbhwalaDcdtfydbhDalaocdtfydbhokaravaocdtfgdydbcitfaDBdbaradydbcitfawBdladadydbcefBdbaravaDcdtfgdydbcitfawBdbaradydbcitfaoBdladadydbcefBdbaravawcdtfgwydbcitfaoBdbarawydbcitfaDBdlawawydbcefBdbaecxfheaqcufgqmbkkdnaiTmbabydlhrabydbhwinararydbawydb9RBdbawclfhwarclfhraicufgimbkkk:3ldouv998Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdnadTmbaicd4hvdnabTmbavcdthocbhraehwinabarc8S2fgiaearav2cdtfgDIdbUdbaiaDIdlUdlaiaDIdwUdwcbhiinalczfaifgDawaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkawaofhwarcefgrad9hmbxdkkavcdthrcbhwincbhiinalczfaifgDaeaifIdbgqaDIdbgkakaq9EEUdbalaifgDaqaDIdbgkakaq9DEUdbaiclfgicx9hmbkaearfheawcefgwad9hmbkkalIdbalIdzgk:tJbbbb:xgqalIdlalIdCgx:tgmamaq9DEgqalIdwalIdKgm:tgPaPaq9DEhPdnabTmbadTmbJbbbbJbbjZaP:vaPJbbbb9BEhqinabaqabIdbak:tNUdbabclfgiaqaiIdbax:tNUdbabcwfgiaqaiIdbam:tNUdbabc8SfhbadcufgdmbkkaPk:3deluabaeIdbabIdbMUdbabaeIdlabIdlMUdlabaeIdwabIdwMUdwabaeIdxabIdxMUdxabaeIdzabIdzMUdzabaeIdCabIdCMUdCabaeIdKabIdKMUdKabaeId3abId3MUd3abaeIdaabIdaMUdaabaeId8KabId8KMUd8KabaeIdyabIdyMUdycbhdinabadfgic8Sfglaeadfgvc8SfIdbalIdbMUdbaic;mbfglavc;mbfIdbalIdbMUdbaic;Sbfglavc;SbfIdbalIdbMUdbaic:mefgiavc:mefIdbaiIdbMUdbadclfgdca9hmbkk:Qdidui99ducbhi8Jjjjjbca9Rglczfcwfcbyd11jjbBdbalcb8Pdj1jjb83izalcwfcbydN1jjbBdbalcb8Pd:m1jjb83ibdndnaembJbbjFhvJbbjFhoJbbjFhrxekadcd4cdthwincbhdinalczfadfgDabadfIdbgoaDIdbgrarao9EEUdbaladfgDaoaDIdbgrarao9DEUdbadclfgdcx9hmbkabawfhbaicefgiae9hmbkalIdwalIdK:thralIdlalIdC:thoalIdbalIdz:thvkavJbbbb:xgvaoaoav9DEgoararao9DEk9DeeuabcFeaicdtz:kjjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk9teiucbcbyd;01jjbgeabcifc98GfgbBd;01jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd;01jjbgeabcrfc94GfgbBd;01jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd;01jjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd;01jjbfgdBd;01jjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akk6eiucbhidnadTmbdninabRbbglaeRbbgv9hmeaecefheabcefhbadcufgdmbxdkkalav9Rhikaikk:cedbcjwk9PFFuuFFuuFFuuFFuFFFuFFFuFbbbbbbbbeeebeebebbeeebebbbbbebebbbbbebbbdbbbbbbbbbbbbbbbeeeeebebbbbbebbbbbeebbbbbbc;Swkxebbbdbbbj9Kbb";

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

		// TODO: flags wrt attribute weights order?
		simplifyWithAttributes: function(indices, vertex_positions, vertex_positions_stride, vertex_attributes, vertex_attributes_stride, target_index_count, target_error, flags, attribute_weights) {
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
