// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2023, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptEncoder = (function() {
	// Built with clang version 16.0.0
	// Built from meshoptimizer 0.20
	var wasm = "b9H79TebbbeJq9Geueu9Geub9Gbb9Gvuuuuueu9Gduueu9Gluuuueu9Gvuuuuub9Gouuuuuub9Gluuuub9GiuuueuiKLdilevlevlooroowwvwbDDbelve9Weiiviebeoweuec:W;kekr;RiOo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bb8A9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949c919M9MWVbe8F9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949c919M9MWV9c9V919U9KbdE9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949wWV79P9V9UbiY9TW79O9V9Wt9FW9U9J9V9KW69U9KW949c919M9MWVbl8E9TW79O9V9Wt9FW9U9J9V9KW69U9KW949c919M9MWV9c9V919U9Kbv8A9TW79O9V9Wt9FW9U9J9V9KW69U9KW949wWV79P9V9UboE9TW79O9V9Wt9FW9U9J9V9KW69U9KW949tWG91W9U9JWbra9TW79O9V9Wt9FW9U9J9V9KW69U9KW949tWG91W9U9JW9c9V919U9KbwL9TW79O9V9Wt9FW9U9J9V9KWS9P2tWV9p9JtbDK9TW79O9V9Wt9FW9U9J9V9KWS9P2tWV9r919HtbqL9TW79O9V9Wt9FW9U9J9V9KWS9P2tWVT949WbkE9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94J9H9J9OWbPa9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94J9H9J9OW9ttV9P9Wbsa9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9WbzK9TW79O9V9Wt9F79W9Ht9P9H29t9VVt9sW9T9H9WbHl79IV9RbODwebcekdQXq:g9sLdbk;3keYu8Jjjjjbcjo9Rgv8Kjjjjbcbhodnalcefae0mbabcbRb:S:kjjbc:GeV86bbavcjdfcbcjdzNjjjb8AdnaiTmbavcjdfadalz:tjjjb8Akabaefhrabcefhwavalfcbcbcjdal9RalcFe0EzNjjjb8Aavavcjdfalz:tjjjbhDcj;abal9UhodndndndndnalTmbaoc;WFbGgecjdaecjd6Ehqcbhkdninakai9pmiaDcjlfcbcjdzNjjjb8Aaqaiak9Rakaqfai6Egxcsfgecl4cifcd4hmadakal2fhPdndndndndnaec9WGgsTmbcbhzcehHaPhOawhAxekdnaxTmbcbhAcehHaPhCinaDaAfRbbhXaDcjlfheaChoaxhQinaeaoRbbgLaX9RgXcetaX;acr4786bbaoalfhoaecefheaLhXaQcufgQmbkaraw9Ram6miawcbamzNjjjbgeTmiaCcefhCaeamfhwaAcefgAal6hHaAal9hmbxvkkaraw9Ram6mvawcbamzNjjjb8AceheinawgXamfhwalaegoSmldnaraw9Ram6mbaocefheawcbamzNjjjb8AaXmekkaoal6hHxekindnaxTmbaDazfRbbhXaDcjlfheaOhoaxhQinaeaoRbbgLaX9RgXcetaX;acr4786bbaoalfhoaecefheaLhXaQcufgQmbkkaraA9Ram6mearaAcbamzNjjjbgKamfgw9RcK6mdcbhYaDcjlfhAinaDcjlfaYfh8AcwhCczhLcehQindndnaQce9hmbcuhoa8ARbbmecbhodninaogecsSmeaecefhoaAaefcefRbbTmbkkcucbaecs6EhoxekaQcetc;:FFFeGhocuaQtcu7cFeGhXcbheinaoaXaAaefRbb9nfhoaecefgecz9hmbkkaoaLaoaL6geEhLaQaCaeEhCaQcetgQcw6mbkdndndndnaCcufPdiebkaKaYco4fgeaeRbbcdciaCclSEaYci4coGtV86bbaCcw9hmeawa8A8Pbb83bbawcwfa8Acwf8Pbb83bbawczfhwxdkaKaYco4fgeaeRbbceaYci4coGtV86bbkdncwaC9TgEmbinawcb86bbawcefhwxbkkcuaCtcu7h8Acbh3aAh5ina5heaEhQcbhoinaeRbbgLa8AcFeGgXaLaX6EaoaCtVhoaecefheaQcufgQmbkawao86bba5aEfh5awcefhwa3aEfg3cz6mbkcbheindnaAaefRbbgoaX6mbawao86bbawcefhwkaecefgecz9hmbkkdnaYczfgYas9pmbaAczfhAaraw9RcL0mekkaYas6meawTmeaOcefhOazcefgzal6hHawhAazalSmixbkkcbhwaHceGTmexikcbhwaHceGmdkaDaPaxcufal2falz:tjjjb8AaxakfhkawmbkcbhoxokcbhoxvkaiTmekcbhoaraw9Ralcaalca0E6mialc8F9nmexdkcbhoaecufca6mdkawcbcaal9RgezNjjjbaefhwkawaDcjdfalz:tjjjbalfab9Rhokavcjof8Kjjjjbaok9heeuaecaaeca0Eabcj;abae9Uc;WFbGgdcjdadcjd6Egdfcufad9Uae2adcl4cifcd4adV2fcefkmbcbabBd:S:kjjbk;rse3u8Jjjjjbc;ae9Rgl8Kjjjjbcbhvdnaici9UgocHfae0mbabcbyd:C:kjjbgrc;GeV86bbalc;abfcFecjezNjjjb8AalcUfgw9cu83ibalc8WfgD9cu83ibalcyfgq9cu83ibalcafgk9cu83ibalcKfgx9cu83ibalczfgm9cu83ibal9cu83iwal9cu83ibabaefc9WfhPabcefgsaofhednaiTmbcmcsarcb9kgzEhHcbhOcbhAcbhCcbhXcbhQindnaeaP9nmbcbhvxikaQcufhvadaCcdtfgoydbhLaocwfydbhKaoclfydbhYcbh8Adndninalc;abfavcsGcitfgoydlhEdndndnaoydbgoaL9hmbaEaYSmekdnaoaY9hmbaEaK9hmba8Acefh8AxekaoaK9hmeaEaL9hmea8Acdfh8Aka8Ac870mdaXcufhvada8AciGcx2goc:y1jjbfydbaCfcdtfydbhEadaocN1jjbfydbaCfcdtfydbhKadaoc:q1jjbfydbaCfcdtfydbhLcbhodnindnalavcsGcdtfydbaE9hmbaohYxdkcuhYavcufhvaocefgocz9hmbkkaEaOSgvaYce9iaYaH9oVgoGh3dndndndndncbcsavEaYaoEgvcs9hmbarce9imbaEaEaAaEcefaASgvEgAcefSmecmcsavEhvkasava8Acdtc;WeGV86bbavcs9hmeaEaA9Rgvcetavc8F917hvinaeavcFb0crtavcFbGV86bbaecefheavcje6hoavcr4hvaoTmbkaEhAxdkcPhvasa8AcdtcPV86bbaEhAkavTmbavaH9imekalaXcdtfaEBdbaXcefcsGhXkaOa3fhOalc;abfaQcitfgvaKBdlavaEBdbalc;abfaQcefcsGgvcitfgoaEBdlaoaLBdbavcefhoxikavcufhva8Aclfg8Ac;ab9hmbkkdnadceaKaOScetaYaOSEcx2gvc:q1jjbfydbaCfcdtfydbgLTadavcN1jjbfydbaCfcdtfydbg8AceSGadavc:y1jjbfydbaCfcdtfydbgYcdSGaOcb9hGazGg5ce9hmbaw9cu83ibaD9cu83ibaq9cu83ibak9cu83ibax9cu83ibam9cu83ibal9cu83iwal9cu83ibcbhOkcbhEaXcufgvhodnindnalaocsGcdtfydba8A9hmbaEhKxdkcuhKaocufhoaEcefgEcz9hmbkkcbhodnindnalavcsGcdtfydbaY9hmbaohExdkcuhEavcufhvaocefgocz9hmbkkaOaLaOSg8Efh3dndnaKcm0mbaKcefhKxekcbcsa8Aa3SgvEhKa3avfh3kdndnaEcm0mbaEcefhExekcbcsaYa3SgvEhEa3avfh3kc9:cua8EEh8FaEaKcltVhocbhvdndndninavcj1jjbfRbbaocFeGSmeavcefgvcz9hmbxdkka5aLaO9havcm0VVmbasavc;WeV86bbxekasa8F86bbaeao86bbaecefhekdna8EmbaLaA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombkaLhAkdnaKcs9hmba8AaA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombka8AhAkdnaEcs9hmbaYaA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombkaYhAkalaXcdtfaLBdbaXcefcsGhvdndnaKPzbeeeeeeeeeeeeeebekalavcdtfa8ABdbaXcdfcsGhvkdndnaEPzbeeeeeeeeeeeeeebekalavcdtfaYBdbavcefcsGhvkalc;abfaQcitfgoaLBdlaoa8ABdbalc;abfaQcefcsGcitfgoa8ABdlaoaYBdbalc;abfaQcdfcsGcitfgoaYBdlaoaLBdbaQcifhoavhXa3hOkascefhsaocsGhQaCcifgCai6mbkkcbhvaeaP0mbcbhvinaeavfavcj1jjbfRbb86bbavcefgvcz9hmbkaeab9Ravfhvkalc;aef8KjjjjbavkZeeucbhddninadcefgdc8F0meceadtae6mbkkadcrfcFeGcr9Uci2cdfabci9U2cHfkmbcbabBd:C:kjjbk:ydewu8Jjjjjbcz9Rhlcbhvdnaicvfae0mbcbhvabcbRb:C:kjjbc;qeV86bbal9cb83iwabcefhoabaefc98fhrdnaiTmbcbhwcbhDindnaoar6mbcbskadaDcdtfydbgqalcwfawaqav9Rgvavc8F91gv7av9Rc507gwcdtfgkydb9Rgvc8E91c9:Gavcdt7awVhvinaoavcFb0gecrtavcFbGV86bbavcr4hvaocefhoaembkakaqBdbaqhvaDcefgDai9hmbkkcbhvaoar0mbaocbBbbaoab9RclfhvkavkBeeucbhddninadcefgdc8F0meceadtae6mbkkadcwfcFeGcr9Uab2cvfk:dvli99dui99ludnaeTmbcuadcetcuftcu7:Yhvdndncuaicuftcu7:YgoJbbbZMgr:lJbbb9p9DTmbar:Ohwxekcjjjj94hwkcbhicbhDinalclfIdbgrJbbbbJbbjZalIdbgq:lar:lMalcwfIdbgk:lMgr:varJbbbb9BEgrNhxaqarNhralcxfIdbhqdndnakJbbbb9GTmbaxhkxekJbbjZar:l:tgkak:maxJbbbb9GEhkJbbjZax:l:tgxax:marJbbbb9GEhrkdndnaqJbbj:;aqJbbj:;9GEgxJbbjZaxJbbjZ9FEavNJbbbZJbbb:;aqJbbbb9GEMgq:lJbbb9p9DTmbaq:Ohmxekcjjjj94hmkdndnakJbbj:;akJbbj:;9GEgqJbbjZaqJbbjZ9FEaoNJbbbZJbbb:;akJbbbb9GEMgq:lJbbb9p9DTmbaq:OhPxekcjjjj94hPkdndnarJbbj:;arJbbj:;9GEgqJbbjZaqJbbjZ9FEaoNJbbbZJbbb:;arJbbbb9GEMgr:lJbbb9p9DTmbar:Ohsxekcjjjj94hskdndnadcl9hmbabaifgzas86bbazcifam86bbazcdfaw86bbazcefaP86bbxekabaDfgzas87ebazcofam87ebazclfaw87ebazcdfaP87ebkalczfhlaiclfhiaDcwfhDaecufgembkkk;klld99eud99eudnaeTmbdndncuaicuftcu7:YgvJbbbZMgo:lJbbb9p9DTmbao:Ohixekcjjjj94hikaic;8FiGhrinabcofcicdalclfIdb:lalIdb:l9EgialcwfIdb:lalaicdtfIdb:l9EEgialcxfIdb:lalaicdtfIdb:l9EEgiarV87ebdndnalaicefciGcdtfIdbJ;Zl:1ZNJbbj:;JbbjZalaicdtfIdbJbbbb9DEgoNgwJbbj:;awJbbj:;9GEgDJbbjZaDJbbjZ9FEavNJbbbZJbbb:;awJbbbb9GEMgw:lJbbb9p9DTmbaw:Ohqxekcjjjj94hqkabaq87ebdndnaoalaicdfciGcdtfIdbJ;Zl:1ZNNgwJbbj:;awJbbj:;9GEgDJbbjZaDJbbjZ9FEavNJbbbZJbbb:;awJbbbb9GEMgw:lJbbb9p9DTmbaw:Ohqxekcjjjj94hqkabcdfaq87ebdndnaoalaicufciGcdtfIdbJ;Zl:1ZNNgoJbbj:;aoJbbj:;9GEgwJbbjZawJbbjZ9FEavNJbbbZJbbb:;aoJbbbb9GEMgo:lJbbb9p9DTmbao:Ohixekcjjjj94hikabclfai87ebabcwfhbalczfhlaecufgembkkk:Hvdxue998Jjjjjbcjd9Rgo8Kjjjjbadcd4hrdndndndnavcd9hmbadcl6mearcearce0EhwaohDinaDc:CuBdbaDclfhDawcufgwmbkaeTmiadcl6mdarcearce0EhqarcdthkalhxcbhminaohDaxhwaqhPinaDaDydbgsawydbgzcL4cFeGc:cufcbazEgzasaz9kEBdbawclfhwaDclfhDaPcufgPmbkaxakfhxamcefgmae9hmbkkaeTmdxekaeTmekavcb9hadcl6gqVhHarcearce0Ehkarcdthrceai9Rhmcbhdindndndnavce9hmbaqmdc:CuhwalhDakhPinawaDydbgscL4cFeGc:cufcbasEgsawas9kEhwaDclfhDaPcufgPmbxdkkc:CuhwaHmbaohDalhPakhsinaDaPydbgzcL4cFeGgxc8Aaxc8A9kEc:cufcbazEBdbaPclfhPaDclfhDascufgsmbkkaqmbcbhDakhsinawhPdnavceSmbaoaDfydbhPkdndnalaDfIdbgOcjjj;8iamaPfgPcLt9R::NJbbbZJbbb:;aOJbbbb9GEMgO:lJbbb9p9DTmbaO:Ohzxekcjjjj94hzkabaDfazcFFFrGaPcKtVBdbaDclfhDascufgsmbkkabarfhbalarfhladcefgdae9hmbkkaocjdf8KjjjjbkFkdCui998Jjjjjbc:Gd9Rgv8Kjjjjbavc:4efcbc;KbzNjjjb8AcbhodnadTmbcbhoaiTmbdnabae9hmbavcuadcdtgoadcFFFFi0Ecbyd:K:kjjbHjjjjbbgeBd:4eavceBd:ydaeabaoz:tjjjb8Akavc:OefcwfcbBdbav9cb83i:Oeavc:Oefaeadaiavc:4efz:njjjbcuaicdtgraicFFFFi0Egwcbyd:K:kjjbHjjjjbbhoavc:4efavyd:ydgDcdtfaoBdbavaDcefgqBd:ydaoavyd:Oegkarz:tjjjbhxavc:4efaqcdtfadci9Ugmcbyd:K:kjjbHjjjjbbgoBdbavaDcdfgrBd:ydaocbamzNjjjbhPavc:4efarcdtfawcbyd:K:kjjbHjjjjbbgsBdbavaDcifgqBd:ydaxhoashrinaralIdbalaoydbgwcwawcw6Ecdtfc;ebfIdbMUdbaoclfhoarclfhraicufgimbkavc:4efaqcdtfcuamcdtadcFFFF970Ecbyd:K:kjjbHjjjjbbgqBdbavaDclfBd:yddnadci6mbamceamce0EhiaehoaqhrinarasaoydbcdtfIdbasaoclfydbcdtfIdbMasaocwfydbcdtfIdbMUdbaocxfhoarclfhraicufgimbkkavc;qbfhzavhoavyd:SehHavyd:WehOcbhwcbhrcbhAcehCinaohXcihQaearci2gLcdtfgocwfydbhdaoydbhDabaAcx2fgiclfaoclfydbgKBdbaiaDBdbaicwfadBdbaParfce86bbazadBdwazaKBdlazaDBdbaqarcdtfcbBdbdnawTmbcihQaXhiinazaQcdtfaiydbgoBdbaQaoaD9haoaK9hGaoad9hGfhQaiclfhiawcufgwmbkkaAcefhAaxaDcdtfgoaoydbcufBdbaxaKcdtfgoaoydbcufBdbaxadcdtfgoaoydbcufBdbcbhwinaOaHaeawaLfcdtfydbcdtgifydbcdtfgKhoakaifgDydbgdhidnadTmbdninaoydbarSmeaoclfhoaicufgiTmdxbkkaoadcdtaKfc98fydbBdbaDaDydbcufBdbkawcefgwci9hmbkdndndnaQTmbcuhrJbbbbhYcbhDavyd:SehKavyd:WehLindndnakazaDcdtfydbcdtgofydbgimbaDcefhDxekaDcs0hwasaofgdIdbh8AadalcbaDcefgDawEcdtfIdbalaxaofydbgwcwawcw6Ecdtfc;ebfIdbMgEUdbaEa8A:thEaicdthiaLaKaofydbcdtfhoinaqaoydbgwcdtfgdaEadIdbMg8AUdba8AaYaYa8A9DgdEhYawaradEhraoclfhoaic98fgimbkkaDaQ9hmbkarcu9hmekaCam9pmeindnaPaCfRbbmbaChrxdkamaCcefgC9hmbxdkkaQczaQcz6EhwazhoaXhzarcu9hmekkavyd:ydhokaocdtavc:4effc98fhrdninaoTmearydbcbyd:G:kjjbH:bjjjbbarc98fhraocufhoxbkkavc:Gdf8Kjjjjbk;UlevucuaicdtgvaicFFFFi0Egocbyd:K:kjjbHjjjjbbhralalyd9GgwcdtfarBdbalawcefBd9GabarBdbaocbyd:K:kjjbHjjjjbbhralalyd9GgocdtfarBdbalaocefBd9GabarBdlcuadcdtadcFFFFi0Ecbyd:K:kjjbHjjjjbbhralalyd9GgocdtfarBdbalaocefBd9GabarBdwabydbcbavzNjjjb8Aadci9UhwdnadTmbabydbhoaehladhrinaoalydbcdtfgvavydbcefBdbalclfhlarcufgrmbkkdnaiTmbabydbhlabydlhrcbhvaihoinaravBdbarclfhralydbavfhvalclfhlaocufgombkkdnadci6mbawceawce0EhDabydlhrabydwhvcbhlinaecwfydbhoaeclfydbhdaraeydbcdtfgwawydbgwcefBdbavawcdtfalBdbaradcdtfgdadydbgdcefBdbavadcdtfalBdbaraocdtfgoaoydbgocefBdbavaocdtfalBdbaecxfheaDalcefgl9hmbkkdnaiTmbabydlheabydbhlinaeaeydbalydb9RBdbalclfhlaeclfheaicufgimbkkkQbabaeadaic:01jjbz:mjjjbkQbabaeadaic:C:jjjbz:mjjjbk9DeeuabcFeaicdtzNjjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk;:kivuo99lu8Jjjjjbcj;Hb9Rgl8Kjjjjbcbhvalc1;Gbfcbc;KbzNjjjb8AalcuadcdtadcFFFFi0Egocbyd:K:kjjbHjjjjbbgrBd19GalceBd;O9Galcwfcbyd:m:kjjbBdbalcb8Pd:e:kjjb83ibalc;W;Gbfcwfcbyd:y:kjjbBdbalcb8Pd:q:kjjb83i;W9Gaicd4hwdndnadmbJFFuFhDJFFuuhqJFFuuhkJFFuFhxJFFuuhmJFFuFhPxekawcdthsaehzincbhiinalaifgHazaifIdbgDaHIdbgxaxaD9EEUdbalc;W;GbfaifgHaDaHIdbgxaxaD9DEUdbaiclfgicx9hmbkazasfhzavcefgvad9hmbkalIdwhqalId;49GhDalIdlhkalId;09GhxalIdbhmalId;W9GhPkdndnadTmbJbbbbJbbjZaPam:tJbbbb:xgPaxak:tgxaxaP9DEgxaDaq:tgDaDax9DEgD:vaDJbbbb9BEhDawcdthsarhHadhzindndnaDaeIdbam:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcv2c;j:KM;jbGhvdndnaDaeclfIdbak:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcq2cM;j:KMeGavVhvdndnaDaecwfIdbaq:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaHavaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcC2c:KM;j:KdGVBdbaeasfheaHclfhHazcufgzmbkalcbcj;GbzNjjjbhiarhHadheinaiaHydbgzcFrGcx2fgvavydbcefBdbaiazcq4cFrGcx2fgvavydlcefBdlaiazcC4cFrGcx2fgzazydwcefBdwaHclfhHaecufgembxdkkalcbcj;GbzNjjjb8AkcbhHcbhzcbhecbhvinalaHfgiydbhsaiazBdbaicwfgwydbhOawavBdbaiclfgiydbhwaiaeBdbasazfhzaOavfhvawaefheaHcxfgHcj;Gb9hmbkcbhialaocbyd:K:kjjbHjjjjbbgzBd:m9GdnadTmbabhHinaHaiBdbaHclfhHadaicefgi9hmbkadTmbabhiadhHinalaraiydbgecdtfydbcFrGcx2fgvavydbgvcefBdbazavcdtfaeBdbaiclfhiaHcufgHmbkazhiadhHinalaraiydbgecdtfydbcq4cFrGcx2fgvavydlgvcefBdlabavcdtfaeBdbaiclfhiaHcufgHmbkabhiadhHinalaraiydbgecdtfydbcC4cFrGcx2fgvavydwgvcefBdwazavcdtfaeBdbaiclfhiaHcufgHmbkadTmbcbhiinabazydbcdtfaiBdbazclfhzadaicefgi9hmbkkclhidninaic98Smealc1;Gbfaifydbcbyd:G:kjjbH:bjjjbbaic98fhixbkkalcj;Hbf8Kjjjjbk9teiucbcbyd:O:kjjbgeabcifc98GfgbBd:O:kjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd:O:kjjbgeabcrfc94GfgbBd:O:kjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd:O:kjjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd:O:kjjbfgdBd:O:kjjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akkk:6ddbcjwk:Cdb4:h9w9N94:P:gW:j9O:ye9Pbbbbbbebbbdbbbebbbdbbbbbbbdbbbbbbbebbbbbbb:l29hZ;69:9kZ;N;76Z;rg97Z;z;o9xZ8J;B85Z;:;u9yZ;b;k9HZ:2;Z9DZ9e:l9mZ59A8KZ:r;T3Z:A:zYZ79OHZ;j4::8::Y:D9V8:bbbb9s:49:Z8R:hBZ9M9M;M8:L;z;o8:;8:PG89q;x:J878R:hQ8::M:B;e87bbbbbbjZbbjZbbjZ:E;V;N8::Y:DsZ9i;H;68:xd;R8:;h0838:;W:NoZbbbb:WV9O8:uf888:9i;H;68:9c9G;L89;n;m9m89;D8Ko8:bbbbf:8tZ9m836ZS:2AZL;zPZZ818EZ9e:lxZ;U98F8:819E;68:FFuuFFuuFFuuFFuFFFuFFFuFbc:Cqkzebbbebbbdbbb8WWbb";

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

		for (var i = 0; i < indices.length; ++i)
			indices[i] = remap[indices[i]];

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
		reorderMesh: function(indices, triangles, optsize) {
			var optf = triangles ? (optsize ? instance.exports.meshopt_optimizeVertexCacheStrip : instance.exports.meshopt_optimizeVertexCache) : undefined;
			return reorder(instance.exports.meshopt_optimizeVertexFetchRemap, indices, maxindex(indices) + 1, optf);
		},
		reorderPoints: function(positions, positions_stride) {
			assert(positions instanceof Float32Array);
			assert(positions.length % positions_stride == 0);
			assert(positions_stride >= 3);
			return spatialsort(instance.exports.meshopt_spatialSortRemap, positions, positions.length / positions_stride, positions_stride * 4);
		},
		encodeVertexBuffer: function(source, count, size) {
			assert(size > 0 && size <= 256);
			assert(size % 4 == 0);
			var bound = instance.exports.meshopt_encodeVertexBufferBound(count, size);
			return encode(instance.exports.meshopt_encodeVertexBuffer, bound, source, count, size);
		},
		encodeIndexBuffer: function(source, count, size) {
			assert(size == 2 || size == 4);
			assert(count % 3 == 0);
			var indices = index32(source, size);
			var bound = instance.exports.meshopt_encodeIndexBufferBound(count, maxindex(indices) + 1);
			return encode(instance.exports.meshopt_encodeIndexBuffer, bound, indices, count, 4);
		},
		encodeIndexSequence: function(source, count, size) {
			assert(size == 2 || size == 4);
			var indices = index32(source, size);
			var bound = instance.exports.meshopt_encodeIndexSequenceBound(count, maxindex(indices) + 1);
			return encode(instance.exports.meshopt_encodeIndexSequence, bound, indices, count, 4);
		},
		encodeGltfBuffer: function(source, count, size, mode) {
			var table = {
				ATTRIBUTES: this.encodeVertexBuffer,
				TRIANGLES: this.encodeIndexBuffer,
				INDICES: this.encodeIndexSequence,
			};
			assert(table[mode]);
			return table[mode](source, count, size);
		},
		encodeFilterOct: function(source, count, stride, bits) {
			assert(stride == 4 || stride == 8);
			assert(bits >= 1 && bits <= 16);
			return filter(instance.exports.meshopt_encodeFilterOct, source, count, stride, bits, 16);
		},
		encodeFilterQuat: function(source, count, stride, bits) {
			assert(stride == 8);
			assert(bits >= 4 && bits <= 16);
			return filter(instance.exports.meshopt_encodeFilterQuat, source, count, stride, bits, 16);
		},
		encodeFilterExp: function(source, count, stride, bits, mode) {
			assert(stride > 0 && stride % 4 == 0);
			assert(bits >= 1 && bits <= 24);
			var table = {
				Separate: 0,
				SharedVector: 1,
				SharedComponent: 2,
			};
			return filter(instance.exports.meshopt_encodeFilterExp, source, count, stride, bits, stride, mode ? table[mode] : 1);
		},
	};
})();

export { MeshoptEncoder };
