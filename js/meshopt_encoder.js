// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2024, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptEncoder = (function () {
	'use strict';
	// Built with clang version 18.1.2
	// Built from meshoptimizer 0.22
	var wasm =
		'b9H79Tebbbe9nk9Geueu9Geub9Gbb9Gouuuuuueu9Gvuuuuueu9Gduueu9Gluuuueu9Gvuuuuub9Gouuuuuub9Gluuuub9GiuuueuiYKdilveoveovrrwrrDDoDbqqbelve9Weiiviebeoweuec;q:Qdkr:nlAo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bb8F9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949c919M9MWV9mW4W2be8A9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949c919M9MWVbd8F9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949c919M9MWV9c9V919U9KbiE9TW79O9V9Wt9FW9U9J9V9KW9wWVtW949wWV79P9V9UblY9TW79O9V9Wt9FW9U9J9V9KW69U9KW949c919M9MWVbv8E9TW79O9V9Wt9FW9U9J9V9KW69U9KW949c919M9MWV9c9V919U9Kbo8A9TW79O9V9Wt9FW9U9J9V9KW69U9KW949wWV79P9V9UbrE9TW79O9V9Wt9FW9U9J9V9KW69U9KW949tWG91W9U9JWbwa9TW79O9V9Wt9FW9U9J9V9KW69U9KW949tWG91W9U9JW9c9V919U9KbDL9TW79O9V9Wt9FW9U9J9V9KWS9P2tWV9p9JtbqK9TW79O9V9Wt9FW9U9J9V9KWS9P2tWV9r919HtbkL9TW79O9V9Wt9FW9U9J9V9KWS9P2tWVT949WbxE9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94J9H9J9OWbsa9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94J9H9J9OW9ttV9P9Wbza9TW79O9V9Wt9F9V9Wt9P9T9P96W9wWVtW94SWt9J9O9sW9T9H9WbHK9TW79O9V9Wt9F79W9Ht9P9H29t9VVt9sW9T9H9WbOl79IV9RbADwebcekdLQq:j9NKdbk;3he8Lu8Jjjjjbc;qw9Rgo8Kjjjjbdndnaembcbhrxekabcbyd;e:kjjbgwc:GeV86bbaoc;adfcbcjdz:vjjjb8AdnaiTmbaoc;adfadalzNjjjb8Akaoc;abfalfcbcbcjdal9RalcFe0Ez:vjjjb8Aaoc;abfaoc;adfalzNjjjb8AaocUf9cb83ibaoc8Wf9cb83ibaocyf9cb83ibaocaf9cb83ibaocKf9cb83ibaoczf9cb83ibao9cb83iwao9cb83ibcj;abal9Uc;WFbGgDcjdaDcjd6Ehqdnaicd6mbavcd9imbawTmbaqci2hkaoc;alfclfhxaoc;qlfceVhmaoc;qofclVhPaoc;qofcKfhsaoc;qofczfhzcbhHincdhOcbhDdnavci6mbas9cb83ibaz9cb83ibao9cb83i;yoao9cb83i;qoadaHfgDybbhAcbhCincbhrcbhXdninaDalfhQaDybbgLaA7aXVhXarcP0meaQhDaLhAarcefgraCfai6mbkkcbhAaoc;qofhDincwhKcwhYdnaXaA93grcFeGg8Acs0mbclhYa8Aci0mba8Acb9hcethYkdnarcw4cFeGg8Acs0mbclhKa8Aci0mba8Acb9hcethKkaKaYfh8AaDydbhEcwhKcwhYdnarcz4cFeGg3cs0mbclhYa3ci0mba3cb9hcethYka8AaEfh8AdnarcK4grcs0mbclhKarci0mbarcb9hcethKkaDa8AaYfaKfBdbaDclfhDaAcefgAcw9hmbkaQhDaLhAaCczfgCai6mbkcbhDcehraPhXinaraDaXydbaoc;qofaDcdtfydb6EhDaXclfhXarcefgrcw9hmbkcihOkcbh5aoc;qlfcbcjdz:vjjjb8Aaoc;alfcwfcbBdbao9cb83i;alaDclth8Einaoc;qlfadcba5cufgDaDa50Eal2falzNjjjb8Adnaqaia59Ra5aqfai6EgLcsfc9WGgDaL9nmbaoc;qofaLfcbaDaL9Rz:vjjjb8Akada5al2fh8Fcbhainaaa8EVcl4h8Aaoc;alfaacdtfhCaHh3cbhEinaEaHfhrdndndndndndnaaPidebikama3c98GfhDaoc;qlfarc98GgXfRbbhKcwhrinaDRbbartaKVhKaDcefhDarcwfgrca9hmbkaLTmlaEcithQa8FaXfhAcbhYinaARbbhXcehDcwhrinaAaDfRbbartaXVhXaDcefhDarcwfgrca9hmbkaoc;qofaYfaXaK7a8A93aQ486bbaAalfhAaXhKaYcefgYaL9hmbxlkkaLTmiaEcitcwGhYaoc;qlfarceVfRbbcwtaoc;qlfarc9:GgDfRbbVhXa8FaDfhDaoc;qofhraLhAinaraD8VbbgKaX9RgXcetaXcztcz91cs47cFFiGaY486bbarcefhraDalfhDaKhXaAcufgAmbxikkaLTmda8FarfhDaoc;qlfarfRbbhXaoc;qofhraLhAinaraDRbbgKaX9RgXcetaXcKtcK91cr4786bbarcefhraDalfhDaKhXaAcufgAmbxdkkaLTmekaCydbhYcbhQaoc;qofhDincdhXcbhrinaXaDarfRbbcb9hfhXarcefgrcz9hmbkclhAcbhrinaAaDarfRbbcd0fhAarcefgrcz9hmbkcwhKcbhrinaKaDarfRbbcP0fhKarcefgrcz9hmbkaXaAaXaA6EgraKaraK6Egrczarcz6EaYfhYaDczfhDaQczfgQaL6mbkaCaYBdbka3cefh3aEcefgEcl9hmbkaacefgaaO9hmbka5akfg5ai6mbkcbhDcehraxhXinaraDaXydbaoc;alfaDcdtfydb6EhDaXclfhXarcefgAhraOaA9hmbkaoaHcd4fa8EcdVaDaDcdSE86bbaHclfgHal6mbkkabaefh8EabcefhDalcd4ghcbawEhgadcefh8Jaoc;qofceVhsaoc;abfceVh8Kcbh8Ldndninaia8L9nmeaoc;qofcbcjdz:vjjjb8Aa8EaD9Rag6mdada8Lal2grfh8McbhPaqaia8L9Ra8Laqfai6Egxcufhza8Jarfh8NaDcbagz:vjjjbgyagfh3axcsfgDcl4cifcd4heaDc9WGgOTh8PindndndndndndndndndndnawTmbaoaPcd4fRbbgXciGPibedlkaxTmda8MaPfhDaoc;abfaPfRbbhXaoc;qofhraxhAinaraDRbbgKaX9RgXcetaXcKtcK91cr4786bbarcefhraDalfhDaKhXaAcufgAmbxikkaxTmiaPcitcwGhYaoc;abfaPceVfRbbcwtaoc;abfaPc9:GgDfRbbVhXa8MaDfhDaoc;qofhraxhAinaraD8VbbgKaX9RgXcetaXcztcz91cs47cFFiGaY486bbarcefhraDalfhDaKhXaAcufgAmbxdkka8KaPc98GgYfhDa8NaYfhKaoc;abfaYfRbbhAcwhrinaDRbbartaAVhAaDcefhDarcwfgrca9hmbkaxTmbaXcl4hLaPcitcKGh8Aa8MaYfhYcbhQinaYRbbhXcwhDaKhrinarRbbaDtaXVhXarcefhraDcwfgDca9hmbkaoc;qofaQfaXaA7aL93a8A486bbaKalfhKaYalfhYaXhAaQcefgQax9hmbkkawmbcbhDxlkaxTmbdnaoRb;qombcbhDinazaDSmdasaDfhraDcefgXhDarRbbTmbkaXax9pmekdnavmbcehDxikaehEaeh8AdnaOTmbcbhLaoc;qofhDashYaeh8AaehEincuhQdnaoc;qofaLfRbbmbcbhXdninaXgrcsSmearcefhXaYarfRbbTmbkkcucbarcs6EhQkcdhXcbhrinaXaDarfRbbcb9hfhXarcefgrcz9hmbkclhAcbhrinaAaDarfRbbcd0fhAarcefgrcz9hmbkcwhKcbhrinaKaDarfRbbcP0fhKarcefgrcz9hmbkaXaAaXaA6EgraKaraK6Egrczarcz6EaEfhEaraQaraQ6Ea8Afh8AaDczfhDaYczfhYaLczfgLaO6mbkka8Aax6meaEax6meayaPcd4fgDaDRbbciaPcetcoGtV86bba8Ea39Rax6mwa3aoc;qofaxzNjjjbaxfh3xlkayaPcd4fgDaDRbbcdaPcetcoGtV86bbxika8AaE9phDkayaPcd4fgrarRbbaDaPcetcoGtV86bbka8Ea39Rae6mla3cbaez:vjjjbg5aefh8AdndnaOmba8PhDxekdna8Ea8A9RcK9pmba8PhDxekaDcdtc:q1jjbfcj1jjbawEgCydxgkcetc;:FFFeGhIcuhLcuaktcu7cFeGhHcbhmaoc;qofhXashEinaoc;qofamfhaczhrdndndnakPDbeeeeeeedekcuhraaRbbmecbhrdninargDcsSmeaDcefhraEaDfRbbTmbkkcucbaDcs6EhrxekcbhDaIhrinaraHaXaDfRbb9nfhraDcefgDcz9hmbkkcihDcbhKinaDh3arhYczhrdndndndndnaCaKcdtfydbgQPDbeeeeeeedekcuhraaRbbmdcbhrdninargDcsSmeaDcefhraEaDfRbbTmbkkcucbaDcs6EhrxekaQcetc;:FFFeGhrcuaQtcu7cFeGhAcbhDinaraAaXaDfRbb9nfhraDcefgDcz9hmbkkaKhDaraY6mekdndnaQaL9hmbaraY9hmba3aKaCa3cdtfydbcwSEhDxeka3hDkaYhrkaKcefgKci9hmbka5amco4fgrarRbbaDamci4coGtV86bbdndndnaCaDcdtfydbgLPDdbbbbbbbebkdncwaL9Tg3TmbcuaLtcu7hrdndnaLceSmbcbh8FaXhainaahDa3hKcbhAinaDRbbgYarcFeGgQaYaQ6EaAaLtVhAaDcefhDaKcufgKmbka8AaA86bbaaa3fhaa8Acefh8Aa8Fa3fg8Fcz6mbxdkkcbh8FaXhainaahDa3hKcbhAinaDRbbgYarcFeGgQaYaQ6EaAcetVhAaDcefhDaKcufgKmbka8AaA:T9cFe:d9c:c:qj:bw9:9c:q;c1:I1e:d9c:b:c:e1z9:9ca188bbaaa3fhaa8Acefh8Aa8Fa3fg8Fcz6mbkkcbhDina8AaXaDfRbbgA86bba8AaAarcFeG9pfh8AaDcefgDcz9hmbxikkdnaLceSmbina8Acb86bba8Acefh8Axbkkina8Acb86bba8Acefh8Axbkka8Aaa8Pbb83bba8Acwfaacwf8Pbb83bba8Aczfh8AkamczfgmaO9pgDmeaXczfhXaEczfhEa8Ea8A9RcK9pmbkkaDTmla8Ah3a8ATmlkaPcefgPal9hmbkaoc;abfa8Mazal2falzNjjjb8Aaxa8Lfh8La3hDa3mbkcbhrxdkcbhra8EaD9RagalfgXcKcaawEgAaXaA0EgK6mednaXaA9pmbaDcbaKaX9Rgrz:vjjjbarfhDkaDaoc;adfalzNjjjbalfhDdnawTmbaDaoahzNjjjbahfhDkaDab9Rhrxekcbhrkaoc;qwf8KjjjjbarkCbabaeadaialcdz:bjjjbk9ueduaecd4gdaefgicaaica0Eabcj;abae9Uc;WFbGgicjdaicjd6Egifcufai9Uae2aiadfaicl4cifcd4f2fcefkmbcbabBd;e:kjjbk:zse5u8Jjjjjbc;ae9Rgl8Kjjjjbcbhvdnaici9UgocHfae0mbabcbyd:0:kjjbgrc;GeV86bbalc;abfcFecjez:vjjjb8AalcUfgw9cu83ibalc8WfgD9cu83ibalcyfgq9cu83ibalcafgk9cu83ibalcKfgx9cu83ibalczfgm9cu83ibal9cu83iwal9cu83ibabaefc9WfhPabcefgsaofhednaiTmbcmcsarcb9kgzEhHcbhOcbhAcbhCcbhXcbhQindnaeaP9nmbcbhvxikaQcufhvadaCcdtfgLydbhKaLcwfydbhYaLclfydbh8AcbhEdndndninalc;abfavcsGcitfgoydlh3dndndnaoydbgoaK9hmba3a8ASmekdnaoa8A9hmba3aY9hmbaEcefhExekaoaY9hmea3aK9hmeaEcdfhEkaEc870mdaXcufhvaLaEciGcx2goc;i1jjbfydbcdtfydbh3aLaoc;e1jjbfydbcdtfydbh8AaLaoc;a1jjbfydbcdtfydbhKcbhodnindnalavcsGcdtfydba39hmbaohYxdkcuhYavcufhvaocefgocz9hmbkkaOa3aOSgvaYce9iaYaH9oVgoGfhOdndndncbcsavEaYaoEgvcs9hmbarce9imba3a3aAa3cefaASgvEgAcefSmecmcsavEhvkasavaEcdtc;WeGV86bbavcs9hmea3aA9Rgvcetavc8F917hvinaeavcFb0crtavcFbGV86bbaecefheavcje6hoavcr4hvaoTmbka3hAxvkcPhvasaEcdtcPV86bba3hAkavTmiavaH9omicdhocehEaQhYxlkavcufhvaEclfgEc;ab9hmbkkdnaLceaYaOSceta8AaOSEcx2gvc;a1jjbfydbcdtfydbgKTaLavc;e1jjbfydbcdtfydbg8AceSGaLavc;i1jjbfydbcdtfydbg3cdSGaOcb9hGazGg5ce9hmbaw9cu83ibaD9cu83ibaq9cu83ibak9cu83ibax9cu83ibam9cu83ibal9cu83iwal9cu83ibcbhOkcbhEaXcufgvhodnindnalaocsGcdtfydba8A9hmbaEhYxdkcuhYaocufhoaEcefgEcz9hmbkkcbhodnindnalavcsGcdtfydba39hmbaohExdkcuhEavcufhvaocefgocz9hmbkkaOaKaOSg8EfhLdndnaYcm0mbaYcefhYxekcbcsa8AaLSgvEhYaLavfhLkdndnaEcm0mbaEcefhExekcbcsa3aLSgvEhEaLavfhLkc9:cua8EEh8FcbhvaEaYcltVgacFeGhodndndninavc:W1jjbfRbbaoSmeavcefgvcz9hmbxdkka5aKaO9havcm0VVmbasavc;WeV86bbxekasa8F86bbaeaa86bbaecefhekdna8EmbaKaA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombkaKhAkdnaYcs9hmba8AaA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombka8AhAkdnaEcs9hmba3aA9Rgvcetavc8F917hvinaeavcFb0gocrtavcFbGV86bbavcr4hvaecefheaombka3hAkalaXcdtfaKBdbaXcefcsGhvdndnaYPzbeeeeeeeeeeeeeebekalavcdtfa8ABdbaXcdfcsGhvkdndnaEPzbeeeeeeeeeeeeeebekalavcdtfa3BdbavcefcsGhvkcihoalc;abfaQcitfgEaKBdlaEa8ABdbaQcefcsGhYcdhEavhXaLhOxekcdhoalaXcdtfa3BdbcehEaXcefcsGhXaQhYkalc;abfaYcitfgva8ABdlava3Bdbalc;abfaQaEfcsGcitfgva3BdlavaKBdbascefhsaQaofcsGhQaCcifgCai6mbkkcbhvaeaP0mbcbhvinaeavfavc:W1jjbfRbb86bbavcefgvcz9hmbkaeab9Ravfhvkalc;aef8KjjjjbavkZeeucbhddninadcefgdc8F0meceadtae6mbkkadcrfcFeGcr9Uci2cdfabci9U2cHfkmbcbabBd:0:kjjbk:ydewu8Jjjjjbcz9Rhlcbhvdnaicvfae0mbcbhvabcbRb:0:kjjbc;qeV86bbal9cb83iwabcefhoabaefc98fhrdnaiTmbcbhwcbhDindnaoar6mbcbskadaDcdtfydbgqalcwfawaqav9Rgvavc8F91gv7av9Rc507gwcdtfgkydb9Rgvc8E91c9:Gavcdt7awVhvinaoavcFb0gecrtavcFbGV86bbavcr4hvaocefhoaembkakaqBdbaqhvaDcefgDai9hmbkkcbhvaoar0mbaocbBbbaoab9RclfhvkavkBeeucbhddninadcefgdc8F0meceadtae6mbkkadcwfcFeGcr9Uab2cvfk:bvli99dui99ludnaeTmbcuadcetcuftcu7:Yhvdndncuaicuftcu7:YgoJbbbZMgr:lJbbb9p9DTmbar:Ohwxekcjjjj94hwkcbhicbhDinalclfIdbgrJbbbbJbbjZalIdbgq:lar:lMalcwfIdbgk:lMgr:varJbbbb9BEgrNhxaqarNhrdndnakJbbbb9GTmbaxhqxekJbbjZar:l:tgqaq:maxJbbbb9GEhqJbbjZax:l:tgxax:marJbbbb9GEhrkdndnalcxfIdbgxJbbj:;axJbbj:;9GEgkJbbjZakJbbjZ9FEavNJbbbZJbbb:;axJbbbb9GEMgx:lJbbb9p9DTmbax:Ohmxekcjjjj94hmkdndnaqJbbj:;aqJbbj:;9GEgxJbbjZaxJbbjZ9FEaoNJbbbZJbbb:;aqJbbbb9GEMgq:lJbbb9p9DTmbaq:OhPxekcjjjj94hPkdndnarJbbj:;arJbbj:;9GEgqJbbjZaqJbbjZ9FEaoNJbbbZJbbb:;arJbbbb9GEMgr:lJbbb9p9DTmbar:Ohsxekcjjjj94hskdndnadcl9hmbabaifgzas86bbazcifam86bbazcdfaw86bbazcefaP86bbxekabaDfgzas87ebazcofam87ebazclfaw87ebazcdfaP87ebkalczfhlaiclfhiaDcwfhDaecufgembkkk;hlld99eud99eudnaeTmbdndncuaicuftcu7:YgvJbbbZMgo:lJbbb9p9DTmbao:Ohixekcjjjj94hikaic;8FiGhrinabcofcicdalclfIdb:lalIdb:l9EgialcwfIdb:lalaicdtfIdb:l9EEgialcxfIdb:lalaicdtfIdb:l9EEgiarV87ebdndnJbbj:;JbbjZalaicdtfIdbJbbbb9DEgoalaicd7cdtfIdbJ;Zl:1ZNNgwJbbj:;awJbbj:;9GEgDJbbjZaDJbbjZ9FEavNJbbbZJbbb:;awJbbbb9GEMgw:lJbbb9p9DTmbaw:Ohqxekcjjjj94hqkabcdfaq87ebdndnalaicefciGcdtfIdbJ;Zl:1ZNaoNgwJbbj:;awJbbj:;9GEgDJbbjZaDJbbjZ9FEavNJbbbZJbbb:;awJbbbb9GEMgw:lJbbb9p9DTmbaw:Ohqxekcjjjj94hqkabaq87ebdndnaoalaicufciGcdtfIdbJ;Zl:1ZNNgoJbbj:;aoJbbj:;9GEgwJbbjZawJbbjZ9FEavNJbbbZJbbb:;aoJbbbb9GEMgo:lJbbb9p9DTmbao:Ohixekcjjjj94hikabclfai87ebabcwfhbalczfhlaecufgembkkk;3viDue99eu8Jjjjjbcjd9Rgo8Kjjjjbadcd4hrdndndndnavcd9hmbadcl6meaohwarhDinawc:CuBdbawclfhwaDcufgDmbkaeTmiadcl6mdarcdthqalhkcbhxinaohwakhDarhminawawydbgPcbaDIdbgs:8cL4cFeGc:cufasJbbbb9BEgzaPaz9kEBdbaDclfhDawclfhwamcufgmmbkakaqfhkaxcefgxaeSmixbkkaeTmdxekaeTmekarcdthkavce9hhqadcl6hdcbhxindndndnaqmbadmdc:CuhDalhwarhminaDcbawIdbgs:8cL4cFeGc:cufasJbbbb9BEgPaDaP9kEhDawclfhwamcufgmmbxdkkc:CuhDdndnavPleddbdkadmdaohwalhmarhPinawcbamIdbgs:8cL4cFeGgzc;:bazc;:b9kEc:cufasJbbbb9BEBdbamclfhmawclfhwaPcufgPmbxdkkadmecbhwarhminaoawfcbalawfIdbgs:8cL4cFeGgPc8AaPc8A9kEc:cufasJbbbb9BEBdbawclfhwamcufgmmbkkadmbcbhwarhPinaDhmdnavceSmbaoawfydbhmkdndnalawfIdbgscjjj;8iamai9RcefgmcLt9R::NJbbbZJbbb:;asJbbbb9GEMgs:lJbbb9p9DTmbas:Ohzxekcjjjj94hzkabawfazcFFFrGamcKtVBdbawclfhwaPcufgPmbkkabakfhbalakfhlaxcefgxae9hmbkkaocjdf8Kjjjjbk;HqdCui998Jjjjjbc:qd9Rgv8Kjjjjbavc:Sefcbc;Kbz:vjjjb8AcbhodnadTmbcbhoaiTmbdnabae9hmbavcuadcdtgradcFFFFi0Ecbyd:8:kjjbHjjjjbbgeBd:SeavceBd:mdaeabarzNjjjb8Akavc:GefcwfcbBdbav9cb83i:Geavc:Gefaeadaiavc:Sefz:ojjjbavyd:Gehwadci9UgDcbyd:8:kjjbHjjjjbbhravc:Sefavyd:mdgqcdtfarBdbavaqcefgkBd:mdarcbaDz:vjjjbhxavc:SefakcdtfcuaicdtaicFFFFi0Ecbyd:8:kjjbHjjjjbbgmBdbavaqcdfgPBd:mdawhramhkinakalIdbalarydbgscwascw6Ecdtfc;ebfIdbMUdbarclfhrakclfhkaicufgimbkavc:SefaPcdtfcuaDcdtadcFFFF970Ecbyd:8:kjjbHjjjjbbgPBdbdnadci6mbaehraPhkaDhiinakamarydbcdtfIdbamarclfydbcdtfIdbMamarcwfydbcdtfIdbMUdbarcxfhrakclfhkaicufgimbkkaqcifhoavc;qbfhzavhravyd:KehHavyd:OehOcbhscbhkcbhAcehCinarhXcihQaeakci2gLcdtfgrydbhdarclfydbhqabaAcx2fgicwfarcwfydbgKBdbaiclfaqBdbaiadBdbaxakfce86bbazaKBdwazaqBdlazadBdbaPakcdtfcbBdbdnasTmbcihQaXhiinazaQcdtfaiydbgrBdbaQaraK9harad9haraq9hGGfhQaiclfhiascufgsmbkkaAcefhAcbhsinaOaHaeasaLfcdtfydbcdtgifydbcdtfgKhrawaifgqydbgdhidnadTmbdninarydbakSmearclfhraicufgiTmdxbkkaraKadcdtfc98fydbBdbaqaqydbcufBdbkascefgsci9hmbkdndnaQTmbcuhkJbbbbhYcbhqavyd:KehKavyd:OehLindndnawazaqcdtfydbcdtgsfydbgrmbaqcefhqxekaqcs0hiamasfgdIdbh8AadalcbaqcefgqaiEcdtfIdbalarcwarcw6Ecdtfc;ebfIdbMgEUdbaEa8A:thEarcdthiaLaKasfydbcdtfhrinaParydbgscdtfgdaEadIdbMg8AUdba8AaYaYa8A9DgdEhYasakadEhkarclfhraic98fgimbkkaqaQ9hmbkakcu9hmekaCaD9pmdindnaxaCfRbbmbaChkxdkaDaCcefgC9hmbxikkaQczaQcz6EhsazhraXhzakcu9hmbkkaocdtavc:Seffc98fhrdninaoTmearydbcbyd:4:kjjbH:bjjjbbarc98fhraocufhoxbkkavc:qdf8Kjjjjbk;IlevucuaicdtgvaicFFFFi0Egocbyd:8:kjjbHjjjjbbhralalyd9GgwcdtfarBdbalawcefBd9GabarBdbaocbyd:8:kjjbHjjjjbbhralalyd9GgocdtfarBdbalaocefBd9GabarBdlcuadcdtadcFFFFi0Ecbyd:8:kjjbHjjjjbbhralalyd9GgocdtfarBdbalaocefBd9GabarBdwabydbcbavz:vjjjb8Aadci9UhDdnadTmbabydbhoaehladhrinaoalydbcdtfgvavydbcefBdbalclfhlarcufgrmbkkdnaiTmbabydbhlabydlhrcbhvaihoinaravBdbarclfhralydbavfhvalclfhlaocufgombkkdnadci6mbabydlhrabydwhvcbhlinaecwfydbhoaeclfydbhdaraeydbcdtfgwawydbgwcefBdbavawcdtfalBdbaradcdtfgdadydbgdcefBdbavadcdtfalBdbaraocdtfgoaoydbgocefBdbavaocdtfalBdbaecxfheaDalcefgl9hmbkkdnaiTmbabydlheabydbhlinaeaeydbalydb9RBdbalclfhlaeclfheaicufgimbkkkQbabaeadaic;K1jjbz:njjjbkQbabaeadaic;m:jjjbz:njjjbk9DeeuabcFeaicdtz:vjjjbhlcbhbdnadTmbindnalaeydbcdtfgiydbcu9hmbaiabBdbabcefhbkaeclfheadcufgdmbkkabk;Wkivuo99lu8Jjjjjbc;W;Gb9Rgl8Kjjjjbcbhvalcj;Gbfcbc;Kbz:vjjjb8AalcuadcdtadcFFFFi0Egocbyd:8:kjjbHjjjjbbgrBdj9GalceBd;G9GalcFFF;7rBdwal9cFFF;7;3FF:;Fb83dbalcFFF97Bd;S9Gal9cFFF;7FFF:;u83d;K9Gaicd4hwdndnadmbJFFuFhDJFFuuhqJFFuuhkJFFuFhxJFFuuhmJFFuFhPxekawcdthsaehzincbhiinalaifgHazaifIdbgDaHIdbgxaxaD9EEUdbalc;K;GbfaifgHaDaHIdbgxaxaD9DEUdbaiclfgicx9hmbkazasfhzavcefgvad9hmbkalIdwhqalId;S9GhDalIdlhkalId;O9GhxalIdbhmalId;K9GhPkdndnadTmbJbbbbJbbjZJbbbbaPam:tgPaPJbbbb9DEgPaxak:tgxaxaP9DEgxaDaq:tgDaDax9DEgD:vaDJbbbb9BEhDawcdthsarhHadhzindndnaDaeIdbam:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcv2c;j:KM;jbGhvdndnaDaeclfIdbak:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcq2cM;j:KMeGavVhvdndnaDaecwfIdbaq:tNJb;au9eNJbbbZMgx:lJbbb9p9DTmbax:Ohixekcjjjj94hikaHavaicztaicwtcj;GiGVaicsGVc:p;G:dKGcH2c;d;H:WKGcC2c:KM;j:KdGVBdbaeasfheaHclfhHazcufgzmbkalcbcj;Gbz:vjjjbhiarhHadheinaiaHydbgzcFrGcx2fgvavydbcefBdbaiazcq4cFrGcx2fgvavydlcefBdlaiazcC4cFrGcx2fgzazydwcefBdwaHclfhHaecufgembxdkkalcbcj;Gbz:vjjjb8AkcbhHcbhzcbhecbhvinalaHfgiydbhsaiazBdbaicwfgwydbhOawavBdbaiclfgiydbhwaiaeBdbasazfhzaOavfhvawaefheaHcxfgHcj;Gb9hmbkcbhHalaocbyd:8:kjjbHjjjjbbgiBd:e9GdnadTmbabhzinazaHBdbazclfhzadaHcefgH9hmbkabhHadhzinalaraHydbgecdtfydbcFrGcx2fgvavydbgvcefBdbaiavcdtfaeBdbaHclfhHazcufgzmbkaihHadhzinalaraHydbgecdtfydbcq4cFrGcx2fgvavydlgvcefBdlabavcdtfaeBdbaHclfhHazcufgzmbkabhHadhzinalaraHydbgecdtfydbcC4cFrGcx2fgvavydwgvcefBdwaiavcdtfaeBdbaHclfhHazcufgzmbkcbhHinabaiydbcdtfaHBdbaiclfhiadaHcefgH9hmbkkclhidninaic98Smealcj;Gbfaifydbcbyd:4:kjjbH:bjjjbbaic98fhixbkkalc;W;Gbf8Kjjjjbk9teiucbcbyd;a:kjjbgeabcifc98GfgbBd;a:kjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabk9teiucbcbyd;a:kjjbgeabcrfc94GfgbBd;a:kjjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik9:eiuZbhedndncbyd;a:kjjbgdaecztgi9nmbcuheadai9RcFFifcz4nbcuSmekadhekcbabae9Rcifc98Gcbyd;a:kjjbfgdBd;a:kjjbdnadZbcztge9nmbadae9RcFFifcz4nb8Akkk;sddbcjwk:0dbbbbdbbblbbbwbbbbbbbebbbdbbblbbbwbbbbbbbbbbbbbbbb4:h9w9N94:P:gW:j9O:ye9Pbbbbbbebbbdbbbebbbdbbbbbbbdbbbbbbbebbbbbbb:l29hZ;69:9kZ;N;76Z;rg97Z;z;o9xZ8J;B85Z;:;u9yZ;b;k9HZ:2;Z9DZ9e:l9mZ59A8KZ:r;T3Z:A:zYZ79OHZ;j4::8::Y:D9V8:bbbb9s:49:Z8R:hBZ9M9M;M8:L;z;o8:;8:PG89q;x:J878R:hQ8::M:B;e87bbbbbbjZbbjZbbjZ:E;V;N8::Y:DsZ9i;H;68:xd;R8:;h0838:;W:NoZbbbb:WV9O8:uf888:9i;H;68:9c9G;L89;n;m9m89;D8Ko8:bbbbf:8tZ9m836ZS:2AZL;zPZZ818EZ9e:lxZ;U98F8:819E;68:bc:0qkzebbbebbbdbbb9q:vbb'; // embed! wasm

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
