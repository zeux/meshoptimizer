// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptEncoder = (function () {
	'use strict';
	// Built with clang version 19.1.5-wasi-sdk
	// Built from meshoptimizer 0.22
	var wasm =
		'b9H79Tebbbe9nk9Geueu9Geub9Gbb9Gouuuuuueu9Gvuuuuueu9Gduueu9Gluuuueu9Gvuuuuub9Gouuuuuub9Gluuuub9GiuuueuiYKdilveoveovrrwrrDDoDbqqbelve9Weiiviebeoweuec;G:Qdkr:nlAo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bb8F9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949c919M9MWV9mW4W2be8A9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949c919M9MWVbd8F9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949c919M9MWV9c9V919U9KbiE9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949wWV79P9V9UblY9TW79O9V9Wt9FW9U9J9V9KW69U9KW949c919M9MWVbv8E9TW79O9V9Wt9FW9U9J9V9KW69U9KW949c919M9MWV9c9V919U9Kbo8A9TW79O9V9Wt9FW9U9J9V9KW69U9KW949wWV79P9V9UbrE9TW79O9V9Wt9FW9U9J9V9KW69U9KW949tWG91W9U9JWbwa9TW79O9V9Wt9FW9U9J9V9KW69U9KW949tWG91W9U9JW9c9V919U9KbDL9TW79O9V9Wt9FW9U9J9V9KWS9P2tWV9p9JtbqK9TW79O9V9Wt9FW9U9J9V9KWS9P2tWV9r919HtbkL9TW79O9V9Wt9FW9U9J9V9KWS9P2tWVT949WbxE9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94J9H9J9OWbsa9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94J9H9J9OW9ttV9P9Wbza9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9WbHK9TW79O9V9Wt9F79W9Ht9P9H29t9VVt9sW9T9H9WbOl79IV9RbADwebcekdLQq:j9OKdbk;Vge8Lu8Jjjjjbc;qw9Rgo8Kjjjjbdndnaembcbhrxekabcbyd;C:kjjbgwc:GeV86bbaoc;adfcbcjdz:vjjjb8AdnaiTmbaoc;adfadalzNjjjb8Akaoc;abfalfcbcbcjdal9RalcFe0Ez:vjjjb8Aaoc;abfaoc;adfalzNjjjb8AaocUf9cb83ibaoc8Wf9cb83ibaocyf9cb83ibaocaf9cb83ibaocKf9cb83ibaoczf9cb83ibao9cb83iwao9cb83ibcj;abal9Uc;WFbGcjdalca0EhDdnaicd6mbavcd9imbawTmbadcefhqaDci2gkal2hxaoc;alfclfhmaoc;qlfceVhPaoc;qofclVhsaoc;qofcKfhzaoc;qofczfhHcbhOincdhAcbhrdnavci6mbaz9cb83ibaH9cb83ibao9cb83i;yoao9cb83i;qoadaOfgrybbhCcbhXincbhQcbhLdninaralfhKarybbgYaC7aLVhLaQcP0meaKhraYhCaQcefgQaXfai6mbkkcbhCaoc;qofhQincwh8AcwhEdnaLaC93grcFeGg3cs0mbclhEa3ci0mba3cb9hcethEkdnarcw4cFeGg3cs0mbclh8Aa3ci0mba3cb9hceth8Aka8AaEfh3aQydbh5cwh8AcwhEdnarcz4cFeGg8Ecs0mbclhEa8Eci0mba8Ecb9hcethEka3a5fh3dnarcFFFFb0mbclh8AarcFFF8F0mbarcFFFr0ceth8AkaQa3aEfa8AfBdbaQclfhQaCcefgCcw9hmbkaKhraYhCaXczfgXai6mbkcbhrcehQashLinaQaraLydbaoc;qofarcdtfydb6EhraLclfhLaQcefgQcw9hmbkcihAkcbh3aoc;qlfcbcjdz:vjjjb8Aaoc;alfcwfcbBdbao9cb83i;alarclth8FadhaaDhhaqh5inaoc;qlfadcba3cufgrara30Eal2falzNjjjb8Aaiahaiah6EhgdnaDaia39Ra3aDfai6EgYcsfc9WGgraY9nmbaoc;qofaYfcbaraY9Rz:vjjjb8Akada3al2fh8Jcbh8Kina8Ka8FVcl4hXaoc;alfa8Kcdtfh8LaOh8Mcbh8Nina8NaOfhQdndndndndndna8KPldebidkaPa8Mc98GgLfhra5aLfh8Aaoc;qlfaQc98GgLfRbbhCcwhQinarRbbaQtaCVhCarcefhraQcwfgQca9hmbkaYTmla8Ncith8Ea8JaLfhEcbhKinaERbbhLcwhra8AhQinaQRbbartaLVhLaQcefhQarcwfgrca9hmbkaoc;qofaKfaLaC7aX93a8E486bba8Aalfh8AaEalfhEaLhCaKcefgKaY9hmbxlkkaYTmia8Mc9:Ghra8NcitcwGhEaoc;qlfaQceVfRbbcwtaoc;qlfaQc9:GfRbbVhLaoc;qofhQaghCinaQa5arfRbbcwtaaarfRbbVg8AaL9RgLcetaLcztcz91cs47cFFiGaE486bbaralfhraQcefhQa8AhLa3aCcufgC9hmbxikkaYTmda8JaQfhraoc;qlfaQfRbbhLaoc;qofhQaghCinaQarRbbg8AaL9RgLcetaLcKtcK91cr4786bbaQcefhQaralfhra8AhLa3aCcufgC9hmbxdkkaYTmeka8LydbhEcbhKaoc;qofhrincdhLcbhQinaLaraQfRbbcb9hfhLaQcefgQcz9hmbkclhCcbhQinaCaraQfRbbcd0fhCaQcefgQcz9hmbkcwh8AcbhQina8AaraQfRbbcP0fh8AaQcefgQcz9hmbkaLaCaLaC6EgQa8AaQa8A6EgQczaQcz6EaEfhEarczfhraKczfgKaY6mbka8LaEBdbka8Mcefh8Ma8Ncefg8Ncl9hmbka8Kcefg8KaA9hmbkaaaxfhaahakfhha5axfh5a3akfg3ai6mbkcbhrcehQamhLinaQaraLydbaoc;alfarcdtfydb6EhraLclfhLaQcefgChQaAaC9hmbkaoaOcd4fa8FcdVararcdSE86bbaOclfgOal6mbkkabaefhOabcefhralcd4gycbawEhzadcefh8Paoc;qofceVh8Faoc;abfceVhIcbhsdndninaias9nmeaoc;qofcbcjdz:vjjjb8AaOar9Raz6mdadasal2gLfhmcbhAascu7aiasaDfgQaiaQ6Efhha8PaLfhearcbazz:vjjjbgqazfh8EaDaias9RaQai6EgPcsfgrcl4cifcd4hkarc9WGg8KThHindndndndndndndndndndnawTmbaoaAcd4fRbbgLciGPlbedlbkaPTmdamaAfhraoc;abfaAfRbbhLaoc;qofhQaPhCinaQarRbbg8AaL9RgLcetaLcKtcK91cr4786bbaQcefhQaralfhra8AhLaCcufgCmbxikkaPTmiaAcitcwGhEaoc;abfaAceVfRbbcwtaoc;abfaAc9:GgrfRbbVhLamarfhraoc;qofhQaPhCinaQar8Vbbg8AaL9RgLcetaLcztcz91cs47cFFiGaE486bbaQcefhQaralfhra8AhLaCcufgCmbxdkkaIaAc98GgEfhraeaEfh8Aaoc;abfaEfRbbhCcwhQinarRbbaQtaCVhCarcefhraQcwfgQca9hmbkaPTmbaLcl4hYaAcitcKGh3amaEfhEcbhKinaERbbhLcwhra8AhQinaQRbbartaLVhLaQcefhQarcwfgrca9hmbkaoc;qofaKfaLaC7aY93a3486bba8Aalfh8AaEalfhEaLhCaKcefgKaP9hmbkkawmbcbhrxlkaPTmbdnaoRb;qombcbhrinaharSmda8FarfhQarcefgLhraQRbbTmbkaLaP9pmekdnavmbcehrxikakh5akh3dna8KTmbcbhYaoc;qofhra8FhEakh3akh5incuhKdnaoc;qofaYfRbbmbcbhLdninaLgQcsSmeaQcefhLaEaQfRbbTmbkkcucbaQcs6EhKkcdhLcbhQinaLaraQfRbbcb9hfhLaQcefgQcz9hmbkclhCcbhQinaCaraQfRbbcd0fhCaQcefgQcz9hmbkcwh8AcbhQina8AaraQfRbbcP0fh8AaQcefgQcz9hmbkaLaCaLaC6EgQa8AaQa8A6EgQczaQcz6Ea5fh5aQaKaQaK6Ea3fh3arczfhraEczfhEaYczfgYa8K6mbkka3aP6mea5aP6meaqaAcd4fgrarRbbciaAcetcoGtV86bbaOa8E9RaP6mwa8Eaoc;qofaPzNjjjbaPfh8ExlkaqaAcd4fgrarRbbcdaAcetcoGtV86bbxika3a59phrkaqaAcd4fgQaQRbbaraAcetcoGtV86bbkaOa8E9Rak6mla8Ecbakz:vjjjbggakfh3dndna8KmbaHhrxekdnaOa39RcK9pmbaHhrxekarcdtc:q1jjbfcj1jjbawEgXydxg8Jcetc;:FFFeGhxcuhYcua8Jtcu7cFeGh8Ncbh8Laoc;qofhLa8Fh5inaoc;qofa8LfhaczhQdndndna8JPDbeeeeeeedekdnaaRbbTmbcuhQxdkcbhQdninaQgrcsSmearcefhQa5arfRbbTmbkkcucbarcs6EhQxekcbhraxhQinaQa8NaLarfRbb9nfhQarcefgrcz9hmbkkcihrcbh8Ainarh8EaQhEczhQdndndndndnaXa8AcdtfydbgKPDbeeeeeeedekcuhQaaRbbmdcbhQdninaQgrcsSmearcefhQa5arfRbbTmbkkcucbarcs6EhQxekaKcetc;:FFFeGhQcuaKtcu7cFeGhCcbhrinaQaCaLarfRbb9nfhQarcefgrcz9hmbkka8AhraQaE6mekdndnaKaY9hmbaQaE9hmba8Ea8AaXa8EcdtfydbcwSEhrxeka8EhrkaEhQka8Acefg8Aci9hmbkaga8Lco4fgQaQRbbara8Lci4coGtV86bbdndndnaXarcdtfydbgYPDdbbbbbbbebkdncwaY9Tg8ETmbcuaYtcu7hQdndnaYceSmbcbh8MaLhainaahra8Eh8AcbhCinarRbbgEaQcFeGgKaEaK6EaCaYtVhCarcefhra8Acufg8Ambka3aC86bbaaa8Efhaa3cefh3a8Ma8Efg8Mcz6mbxdkkcbh8MaLhainaahra8Eh8AcbhCinarRbbgEaQcFeGgKaEaK6EaCcetVhCarcefhra8Acufg8Ambka3aC:T9cFe:d9c:c:qj:bw9:9c:q;c1:I1e:d9c:b:c:e1z9:9ca188bbaaa8Efhaa3cefh3a8Ma8Efg8Mcz6mbkkcbhrina3aLarfRbbgC86bba3aCaQcFeG9pfh3arcefgrcz9hmbxikkdnaYceSmbina3cb86bba3cefh3xbkkina3cb86bba3cefh3xbkka3aa8Pbb83bba3cwfaacwf8Pbb83bba3czfh3ka8Lczfg8La8K9pgrmeaLczfhLa5czfh5aOa39RcK9pmbkkarTmla3h8Ea3TmlkaAcefgAal9hmbkaoc;abfamaPcufal2falzNjjjb8AaPasfhsa8Ehra8EmbkcbhrxdkdnaOar9RazalfgQcKcaawEgLaQaL0EgC9pmbcbhrxdkdnaQaL9pmbarcbaCaQ9RgQz:vjjjbaQfhrkaraoc;adfalzNjjjbalfhrdnawTmbaraoayzNjjjbayfhrkarab9Rhrxekcbhrkaoc;qwf8KjjjjbarkCbabaeadaialcdz:bjjjbk9reduaecd4gdaefgicaaica0Eabcj;abae9Uc;WFbGcjdaeca0Egifcufai9Uae2aiadfaicl4cifcd4f2fcefkmbcbabBd;C:kjjbk:Ese5u8Jjjjjbc;ae9Rgl8Kjjjjbcbhvdnaici9UgocHfae0mbabcbyd;m:kjjbgrc;GeV86bbalc;abfcFecjez:vjjjb8AalcUfgw9cu83ibalc8WfgD9cu83ibalcyfgq9cu83ibalcafgk9cu83ibalcKfgx9cu83ibalczfgm9cu83ibal9cu83iwal9cu83ibabaefc9WfhPabcefgsaofhednaiTmbcmcsarcb9kgzEhHcbhOcbhAcbhCcbhXcbhQindnaeaP9nmbcbhvxikaQcufhvadaCcdtfgLydbhKaLcwfydbhYaLclfydbh8AcbhEdndndninalc;abfavcsGcitfgoydlh3dndndnaoydbgoaK9hmba3a8ASmekdnaoa8A9hmba3aY9hmbaEcefhExekaoaY9hmea3aK9hmeaEcdfhEkaEc870mdaXcufhvaLaEciGcx2goc;i1jjbfydbcdtfydbh3aLaoc;e1jjbfydbcdtfydbh8AaLaoc;a1jjbfydbcdtfydbhKcbhodnindnalavcsGcdtfydba39hmbaohYxdkcuhYavcufhvaocefgocz9hmbkkaOa3aOSgvaYce9iaYaH9oVgoGfhOdndndncbcsavEaYaoEgvcs9hmbarce9imba3a3aAa3cefaASgvEgAcefSmecmcsavEhvkasavaEcdtc;WeGV86bbavcs9hmea3aA9Rgvcetavc8F917hvinaeavcFb0crtavcFbGV86bbaecefheavcje6hoavcr4hvaoTmbka3hAxvkcPhvasaEcdtcPV86bba3hAkavTmiavaH9omicdhocehEaQhYxlkavcufhvaEclfgEc;ab9hmbkkdnaLceaYaOSceta8AaOSEcx2gvc;a1jjbfydbcdtfydbgKTaLavc;e1jjbfydbcdtfydbg8AceSGaLavc;i1jjbfydbcdtfydbg3cdSGaOcb9hGazGg5ce9hmbaw9cu83ibaD9cu83ibaq9cu83ibak9cu83ibax9cu83ibam9cu83ibal9cu83iwal9cu83ibcbhOkcbhEaXcufgvhodnindnalaocsGcdtfydba8A9hmbaEhYxdkcuhYaocufhoaEcefgEcz9hmbkkcbhodnindnalavcsGcdtfydba39hmbaohExdkcuhEavcufhvaocefgocz9hmbkkaOaKaOSg8EfhLdndnaYcm0mbaYcefhYxekcbcsa8AaLSgvEhYaLavfhLkdndnaEcm0mbaEcefhExekcbcsa3aLSgvEhEaLavfhLkc9:cua8EEh8FcbhvaEaYcltVgacFeGhodndndninavc:W1jjbfRbbaoSmeavcefgvcz9hmbxdkka5aKaO9havcm0VVmbasavc;WeV86bbxekasa8F86bbaeaa86bbaecefhekdna8EmbaKaA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombkaKhAkdnaYcs9hmba8AaA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombka8AhAkdnaEcs9hmba3aA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombka3hAkalaXcdtfaKBdbaXcefcsGhvdndnaYPzbeeeeeeeeeeeeeebekalavcdtfa8ABdbaXcdfcsGhvkdndnaEPzbeeeeeeeeeeeeeebekalavcdtfa3BdbavcefcsGhvkcihoalc;abfaQcitfgEaKBdlaEa8ABdbaQcefcsGhYcdhEavhXaLhOxekcdhoalaXcdtfa3BdbcehEaXcefcsGhXaQhYkalc;abfaYcitfgva8ABdlava3Bdbalc;abfaQaEfcsGcitfgva3BdlavaKBdbascefhsaQaofcsGhQaCcifgCai6mbkkdnaeaP9nmbcbhvxekcbhvinaeavfavc:W1jjbfRbb86bbavcefgvcz9hmbkaeab9Ravfhvkalc;aef8KjjjjbavkZeeucbhddninadcefgdc8F0meceadtae6mbkkadcrfcFeGcr9Uci2cdfabci9U2cHfkmbcbabBd;m:kjjbk:Adewu8Jjjjjbcz9Rhlcbhvdnaicvfae0mbcbhvabcbRb;m:kjjbc;qeV86bbal9cb83iwabcefhoabaefc98fhrdnaiTmbcbhwcbhDindnaoar6mbcbskadaDcdtfydbgqalcwfawaqav9Rgvavc8F91gv7av9Rc507gwcdtfgkydb9Rgvc8E91c9:Gavcdt7awVhvinaoavcFb0gecrtavcFbGV86bbavcr4hvaocefhoaembkakaqBdbaqhvaDcefgDai9hmbkkdnaoar9nmbcbskaocbBbbaoab9RclfhvkavkBeeucbhddninadcefgdc8F0meceadtae6mbkkadcwfcFeGcr9Uab2cvfk:bvli99dui99ludnaeTmbcuadcetcuftcu7:Zhvdndncuaicuftcu7:ZgoJbbbZMgr:lJbbb9p9DTmbar:Ohwxekcjjjj94hwkcbhicbhDinalclfIdbgrJbbbbJbbjZalIdbgq:lar:lMalcwfIdbgk:lMgr:varJbbbb9BEgrNhxaqarNhrdndnakJbbbb9GTmbaxhqxekJbbjZar:l:tgqaq:maxJbbbb9GEhqJbbjZax:l:tgxax:marJbbbb9GEhrkdndnalcxfIdbgxJbbj:;axJbbj:;9GEgkJbbjZakJbbjZ9FEavNJbbbZJbbb:;axJbbbb9GEMgx:lJbbb9p9DTmbax:Ohmxekcjjjj94hmkdndnaqJbbj:;aqJbbj:;9GEgxJbbjZaxJbbjZ9FEaoNJbbbZJbbb:;aqJbbbb9GEMgq:lJbbb9p9DTmbaq:OhPxekcjjjj94hPkdndnarJbbj:;arJbbj:;9GEgqJbbjZaqJbbjZ9FEaoNJbbbZJbbb:;arJbbbb9GEMgr:lJbbb9p9DTmbar:Ohsxekcjjjj94hskdndnadcl9hmbabaifgzas86bbazcifam86bbazcdfaw86bbazcefaP86bbxekabaDfgzas87ebazcofam87ebazclfaw87ebazcdfaP87ebkalczfhlaiclfhiaDcwfhDaecufgembkkk;hlld99eud99eudnaeTmbdndncuaicuftcu7:ZgvJbbbZMgo:lJbbb9p9DTmbao:Ohixekcjjjj94hikaic;8FiGhrinabcofcicdalclfIdb:lalIdb:l9EgialcwfIdb:lalaicdtfIdb:l9EEgialcxfIdb:lalaicdtfIdb:l9EEgiarV87ebdndnJbbj:;JbbjZalaicdtfIdbJbbbb9DEgoalaicd7cdtfIdbJ;Zl:1ZNNgwJbbj:;awJbbj:;9GEgDJbbjZaDJbbjZ9FEavNJbbbZJbbb:;awJbbbb9GEMgw:lJbbb9p9DTmbaw:Ohqxekcjjjj94hqkabcdfaq87ebdndnalaicefciGcdtfIdbJ;Zl:1ZNaoNgwJbbj:;awJbbj:;9GEgDJbbjZaDJbbjZ9FEavNJbbbZJbbb:;awJbbbb9GEMgw:lJbbb9p9DTmbaw:Ohqxekcjjjj94hqkabaq87ebdndnaoalaicufciGcdtfIdbJ;Zl:1ZNNgoJbbj:;aoJbbj:;9GEgwJbbjZawJbbjZ9FEavNJbbbZJbbb:;aoJbbbb9GEMgo:lJbbb9p9DTmbao:Ohixekcjjjj94hikabclfai87ebabcwfhbalczfhlaecufgembkkk;3viDue99eu8Jjjjjbcjd9Rgo8Kjjjjbadcd4hrdndndndnavcd9hmbadcl6meaohwarhDinawc:CuBdbawclfhwaDcufgDmbkaeTmiadcl6mdarcdthqalhkcbhxinaohwakhDarhminawawydbgPcbaDIdbgs:8cL4cFeGc:cufasJbbbb9BEgzaPaz9kEBdbaDclfhDawclfhwamcufgmmbkakaqfhkaxcefgxaeSmixbkkaeTmdxekaeTmekarcdthkavce9hhqadcl6hdcbhxindndndnaqmbadmdc:CuhDalhwarhminaDcbawIdbgs:8cL4cFeGc:cufasJbbbb9BEgPaDaP9kEhDawclfhwamcufgmmbxdkkc:CuhDdndnavPleddbdkadmdaohwalhmarhPinawcbamIdbgs:8cL4cFeGgzc;:bazc;:b0Ec:cufasJbbbb9BEBdbamclfhmawclfhwaPcufgPmbxdkkadmecbhwarhminaoawfcbalawfIdbgs:8cL4cFeGgPc8AaPc8A0Ec:cufasJbbbb9BEBdbawclfhwamcufgmmbkkadmbcbhwarhPinaDhmdnavceSmbaoawfydbhmkdndnalawfIdbgscjjj;8iamai9RcefgmcLt9R::NJbbbZJbbb:;asJbbbb9GEMgs:lJbbb9p9DTmbas:Ohzxekcjjjj94hzkabawfazcFFFrGamcKtVBdbawclfhwaPcufgPmbkkabakfhbalakfhlaxcefgxae9hmbkkaocjdf8Kjjjjbk;YqdXui998Jjjjjbc:qd9Rgv8Kjjjjbavc:Sefcbc;Kbz:vjjjb8AcbhodnadTmbcbhoaiTmbdndnabaeSmbaehrxekavcuadcdtgwadcFFFFi0Ecbyd;u:kjjbHjjjjbbgrBd:SeavceBd:mdaraeawzNjjjb8Akavc:GefcwfcbBdbav9cb83i:Geavc:Gefaradaiavc:Sefz:ojjjbavyd:GehDadci9Ugqcbyd;u:kjjbHjjjjbbheavc:Sefavyd:mdgkcdtfaeBdbavakcefgwBd:mdaecbaqz:vjjjbhxavc:SefawcdtfcuaicdtaicFFFFi0Ecbyd;u:kjjbHjjjjbbgmBdbavakcdfgPBd:mdalc;ebfhsaDheamhwinawalIdbasaeydbgzcwazcw6EcdtfIdbMUdbaeclfheawclfhwaicufgimbkavc:SefaPcdtfcuaqcdtadcFFFF970Ecbyd;u:kjjbHjjjjbbgPBdbdnadci6mbarheaPhwaqhiinawamaeydbcdtfIdbamaeclfydbcdtfIdbMamaecwfydbcdtfIdbMUdbaecxfheawclfhwaicufgimbkkakcifhoalc;ebfhHavc;qbfhOavheavyd:KehAavyd:OehCcbhzcbhwcbhXcehQinaehLcihkarawci2gKcdtfgeydbhsaeclfydbhdabaXcx2fgicwfaecwfydbgYBdbaiclfadBdbaiasBdbaxawfce86bbaOaYBdwaOadBdlaOasBdbaPawcdtfcbBdbdnazTmbcihkaLhiinaOakcdtfaiydbgeBdbakaeaY9haeas9haead9hGGfhkaiclfhiazcufgzmbkkaXcefhXcbhzinaCaAarazaKfcdtfydbcdtgifydbcdtfgYheaDaifgdydbgshidnasTmbdninaeydbawSmeaeclfheaicufgiTmdxbkkaeaYascdtfc98fydbBdbadadydbcufBdbkazcefgzci9hmbkdndnakTmbcuhwJbbbbh8Acbhdavyd:KehYavyd:OehKindndnaDaOadcdtfydbcdtgzfydbgembadcefhdxekadcs0hiamazfgsIdbhEasalcbadcefgdaiEcdtfIdbaHaecwaecw6EcdtfIdbMg3Udba3aE:th3aecdthiaKaYazfydbcdtfheinaPaeydbgzcdtfgsa3asIdbMgEUdbaEa8Aa8AaE9DgsEh8AazawasEhwaeclfheaic98fgimbkkadak9hmbkawcu9hmekaQaq9pmdindnaxaQfRbbmbaQhwxdkaqaQcefgQ9hmbxikkakczakcz6EhzaOheaLhOawcu9hmbkkaocdtavc:Seffc98fhedninaoTmeaeydbcbyd;q:kjjbH:bjjjbbaec98fheaocufhoxbkkavc:qdf8Kjjjjbk;IlevucuaicdtgvaicFFFFi0Egocbyd;u:kjjbHjjjjbbhralalyd9GgwcdtfarBdbalawcefBd9GabarBdbaocbyd;u:kjjbHjjjjbbhralalyd9GgocdtfarBdbalaocefBd9GabarBdlcuadcdtadcFFFFi0Ecbyd;u:kjjbHjjjjbbhralalyd9GgocdtfarBdbalaocefBd9GabarBdwabydbcbavz:vjjjb8Aadci9UhDdnadTmbabydbhoaehladhrinaoalydbcdtfgvavydbcefBdbalclfhlarcufgrmbkkdnaiTmbabydbhlabydlhrcbhvaihoinaravBdbarclfhralydbavfhvalclfhlaocufgombkkdnadci6mbabydlhrabydwhvcbhlinaecwfydbhoaeclfydbhdaraeydbcdtfgwawydbgwcefBdbavawcdtfalBdbaradcdtfgdadydbgdcefBdbavadcdtfalBdbaraocdtfgoaoydbgocefBdbavaocdtfalBdbaecxfheaDalcefgl9hmbkkdnaiTmbabydlheabydbhlinaeaeydbalydb9RBdbalclfhlaeclfheaicufgimbkkkQbabaeadaic;K1jjbz:njjjbkQbabaeadaic;m:jjjbz:njjjbk9DeeuabcFeaicdtz:vjjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk;:kivuo99lu8Jjjjjbcj;Hb9Rgl8Kjjjjbcbhvalc:m;Gbfcbc;Kbz:vjjjb8AalcuadcdtadcFFFFi0Egocbyd;u:kjjbHjjjjbbgrBd:m9GalceBd;S9Galcwfcbyd:8:kjjbBdbalcb8Pd:0:kjjb83ibalc;W;Gbfcwfcbyd;i:kjjbBdbalcb8Pd;a:kjjb83i;W9Gaicd4hwdndnadmbJFFuFhDJFFuuhqJFFuuhkJFFuFhxJFFuuhmJFFuFhPxekawcdthsaehzincbhiinalaifgHazaifIdbgDaHIdbgxaxaD9EEUdbalc;W;GbfaifgHaDaHIdbgxaxaD9DEUdbaiclfgicx9hmbkazasfhzavcefgvad9hmbkalIdwhqalId;49GhDalIdlhkalId;09GhxalIdbhmalId;W9GhPkdndnadTmbJbbbbJbbjZJbbbbaPam:tgPaPJbbbb9DEgPaxak:tgxaxaP9DEgxaDaq:tgDaDax9DEgD:vaDJbbbb9BEhDawcdthsarhHadhzindndnaDaeIdbam:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcv2c;j:KM;jbGhvdndnaDaeclfIdbak:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcq2cM;j:KMeGavVhvdndnaDaecwfIdbaq:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaHavaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcC2c:KM;j:KdGVBdbaeasfheaHclfhHazcufgzmbkalcbcj;Gbz:vjjjbhiarhHadheinaiaHydbgzcFrGcx2fgvavydbcefBdbaiazcq4cFrGcx2fgvavydlcefBdlaiazcC4cFrGcx2fgzazydwcefBdwaHclfhHaecufgembxdkkalcbcj;Gbz:vjjjb8AkcbhHcbhzcbhecbhvinalaHfgiydbhsaiazBdbaicwfgwydbhOawavBdbaiclfgiydbhwaiaeBdbasazfhzaOavfhvawaefheaHcxfgHcj;Gb9hmbkcbhHalaocbyd;u:kjjbHjjjjbbgiBd:q9GdnadTmbabhzinazaHBdbazclfhzadaHcefgH9hmbkabhHadhzinalaraHydbgecdtfydbcFrGcx2fgvavydbgvcefBdbaiavcdtfaeBdbaHclfhHazcufgzmbkaihHadhzinalaraHydbgecdtfydbcq4cFrGcx2fgvavydlgvcefBdlabavcdtfaeBdbaHclfhHazcufgzmbkabhHadhzinalaraHydbgecdtfydbcC4cFrGcx2fgvavydwgvcefBdwaiavcdtfaeBdbaHclfhHazcufgzmbkcbhHinabaiydbcdtfaHBdbaiclfhiadaHcefgH9hmbkkclhidninaic98Smealc:m;Gbfaifydbcbyd;q:kjjbH:bjjjbbaic98fhixbkkalcj;Hbf8Kjjjjbk9teiucbcbyd;y:kjjbgeabcifc98GfgbBd;y:kjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;teeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiaeydlBdlaiaeydwBdwaiaeydxBdxaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk:3eedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdxaialBdwaialBdlaialBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd;y:kjjbgeabcrfc94GfgbBd;y:kjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd;y:kjjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd;y:kjjbfgdBd;y:kjjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akkk;Qddbcjwk;mdbbbbdbbblbbbwbbbbbbbebbbdbbblbbbwbbbbbbbbbbbbbbbb4:h9w9N94:P:gW:j9O:ye9Pbbbbbbebbbdbbbebbbdbbbbbbbdbbbbbbbebbbbbbb:l29hZ;69:9kZ;N;76Z;rg97Z;z;o9xZ8J;B85Z;:;u9yZ;b;k9HZ:2;Z9DZ9e:l9mZ59A8KZ:r;T3Z:A:zYZ79OHZ;j4::8::Y:D9V8:bbbb9s:49:Z8R:hBZ9M9M;M8:L;z;o8:;8:PG89q;x:J878R:hQ8::M:B;e87bbbbbbjZbbjZbbjZ:E;V;N8::Y:DsZ9i;H;68:xd;R8:;h0838:;W:NoZbbbb:WV9O8:uf888:9i;H;68:9c9G;L89;n;m9m89;D8Ko8:bbbbf:8tZ9m836ZS:2AZL;zPZZ818EZ9e:lxZ;U98F8:819E;68:FFuuFFuuFFuuFFuFFFuFFFuFbc;mqkzebbbebbbdbbb9G:vbb'; // embed! wasm

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
		instance.exports.meshopt_encodeVertexVersion(0);
		instance.exports.meshopt_encodeIndexVersion(1);
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

	function reorder(fun, indices, vertices, optf) {
		var sbrk = instance.exports.sbrk;
		var ip = sbrk(indices.length * 4);
		var rp = sbrk(vertices * 4);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		var indices8 = bytes(indices);
		heap.set(indices8, ip);
		if (optf) {
			optf(ip, ip, indices.length, vertices);
		}
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

	function spatialsort(fun, positions, count, stride) {
		var sbrk = instance.exports.sbrk;
		var ip = sbrk(count * 4);
		var sp = sbrk(count * stride);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(positions), sp);
		fun(ip, sp, count, stride);
		// heap may have grown
		heap = new Uint8Array(instance.exports.memory.buffer);
		var remap = new Uint32Array(count);
		new Uint8Array(remap.buffer).set(heap.subarray(ip, ip + count * 4));
		sbrk(ip - sbrk(0));
		return remap;
	}

	function encode(fun, bound, source, count, size) {
		var sbrk = instance.exports.sbrk;
		var tp = sbrk(bound);
		var sp = sbrk(count * size);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(source), sp);
		var res = fun(tp, bound, sp, count, size);
		var target = new Uint8Array(res);
		target.set(heap.subarray(tp, tp + res));
		sbrk(tp - sbrk(0));
		return target;
	}

	function maxindex(source) {
		var result = 0;
		for (var i = 0; i < source.length; ++i) {
			var index = source[i];
			result = result < index ? index : result;
		}
		return result;
	}

	function index32(source, size) {
		assert(size == 2 || size == 4);
		if (size == 4) {
			return new Uint32Array(source.buffer, source.byteOffset, source.byteLength / 4);
		} else {
			var view = new Uint16Array(source.buffer, source.byteOffset, source.byteLength / 2);
			return new Uint32Array(view); // copies each element
		}
	}

	function filter(fun, source, count, stride, bits, insize, mode) {
		var sbrk = instance.exports.sbrk;
		var tp = sbrk(count * stride);
		var sp = sbrk(count * insize);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(bytes(source), sp);
		fun(tp, count, stride, bits, sp, mode);
		var target = new Uint8Array(count * stride);
		target.set(heap.subarray(tp, tp + count * stride));
		sbrk(tp - sbrk(0));
		return target;
	}

	return {
		ready: ready,
		supported: true,
		reorderMesh: function (indices, triangles, optsize) {
			var optf = triangles
				? optsize
					? instance.exports.meshopt_optimizeVertexCacheStrip
					: instance.exports.meshopt_optimizeVertexCache
				: undefined;
			return reorder(instance.exports.meshopt_optimizeVertexFetchRemap, indices, maxindex(indices) + 1, optf);
		},
		reorderPoints: function (positions, positions_stride) {
			assert(positions instanceof Float32Array);
			assert(positions.length % positions_stride == 0);
			assert(positions_stride >= 3);
			return spatialsort(instance.exports.meshopt_spatialSortRemap, positions, positions.length / positions_stride, positions_stride * 4);
		},
		encodeVertexBuffer: function (source, count, size) {
			assert(size > 0 && size <= 256);
			assert(size % 4 == 0);
			var bound = instance.exports.meshopt_encodeVertexBufferBound(count, size);
			return encode(instance.exports.meshopt_encodeVertexBuffer, bound, source, count, size);
		},
		encodeIndexBuffer: function (source, count, size) {
			assert(size == 2 || size == 4);
			assert(count % 3 == 0);
			var indices = index32(source, size);
			var bound = instance.exports.meshopt_encodeIndexBufferBound(count, maxindex(indices) + 1);
			return encode(instance.exports.meshopt_encodeIndexBuffer, bound, indices, count, 4);
		},
		encodeIndexSequence: function (source, count, size) {
			assert(size == 2 || size == 4);
			var indices = index32(source, size);
			var bound = instance.exports.meshopt_encodeIndexSequenceBound(count, maxindex(indices) + 1);
			return encode(instance.exports.meshopt_encodeIndexSequence, bound, indices, count, 4);
		},
		encodeGltfBuffer: function (source, count, size, mode) {
			var table = {
				ATTRIBUTES: this.encodeVertexBuffer,
				TRIANGLES: this.encodeIndexBuffer,
				INDICES: this.encodeIndexSequence,
			};
			assert(table[mode]);
			return table[mode](source, count, size);
		},
		encodeFilterOct: function (source, count, stride, bits) {
			assert(stride == 4 || stride == 8);
			assert(bits >= 1 && bits <= 16);
			return filter(instance.exports.meshopt_encodeFilterOct, source, count, stride, bits, 16);
		},
		encodeFilterQuat: function (source, count, stride, bits) {
			assert(stride == 8);
			assert(bits >= 4 && bits <= 16);
			return filter(instance.exports.meshopt_encodeFilterQuat, source, count, stride, bits, 16);
		},
		encodeFilterExp: function (source, count, stride, bits, mode) {
			assert(stride > 0 && stride % 4 == 0);
			assert(bits >= 1 && bits <= 24);
			var table = {
				Separate: 0,
				SharedVector: 1,
				SharedComponent: 2,
				Clamped: 3,
			};
			return filter(instance.exports.meshopt_encodeFilterExp, source, count, stride, bits, stride, mode ? table[mode] : 1);
		},
	};
})();

// export! MeshoptEncoder
if (typeof exports === 'object' && typeof module === 'object') module.exports = MeshoptEncoder;
else if (typeof define === 'function' && define['amd'])
	define([], function () {
		return MeshoptEncoder;
	});
else if (typeof exports === 'object') exports['MeshoptEncoder'] = MeshoptEncoder;
else (typeof self !== 'undefined' ? self : this).MeshoptEncoder = MeshoptEncoder;
