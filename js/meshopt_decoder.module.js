// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2024, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
var MeshoptDecoder = (function () {
	// Built with clang version 18.1.2
	// Built from meshoptimizer 0.22
	var wasm_base =
		'b9H79Tebbbe8Fv9Gbb9Gvuuuuueu9Giuuub9Geueu9Giuuueuikqbeeedddillviebeoweuec:W:Odkr;leDo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bb8A9TW79O9V9Wt9F9KW9J9V9KW9wWVtW949c919M9MWVbeY9TW79O9V9Wt9F9KW9J9V9KW69U9KW949c919M9MWVbdE9TW79O9V9Wt9F9KW9J9V9KW69U9KW949tWG91W9U9JWbiL9TW79O9V9Wt9F9KW9J9V9KWS9P2tWV9p9JtblK9TW79O9V9Wt9F9KW9J9V9KWS9P2tWV9r919HtbvL9TW79O9V9Wt9F9KW9J9V9KWS9P2tWVT949Wbol79IV9Rbrq:785qdbk:PKi3ud9:du8Jjjjjbcj;kb9Rgv8Kjjjjbc9:hodnalTmbcuhoaiRbbgrc;WeGc:Ge9hmbarcsGgwce0mbc9:hoalcufadcd4cbawEgDadfgrcKcaawEgqaraq0Egk6mbaicefhxcj;abad9Uc;WFbGgocjdaocjd6EhmaialfgPar9Rgoadfhsavaoadz1jjjbgzceVhHcbhOdndninaeaO9nmeaPax9RaD6mdamaeaO9RaOamfae6EgAcsfgoc9WGhCabaOad2fhXaAcethQaxaDfhiaocl4cifcd4hLcbhKazcjdfhYaHh8AincbhodnawTmbaxaKcd4fRbbhokaocFeGhEcbh3inaih5dndndndnaEa3cet4ciGgoc9:fPdebdkaPa59RaA6mrazcj;cbfa3aA2fa5aAz1jjjb8Aa5aAfhixdkazcj;cbfa3aA2fcbaAz:jjjjb8Aa5hixekaPa59RaL6mva5aLfhidnaCTmbaPai9RcK6mbaocdtc:q1jjbfcj1jjbawEh8Eazcj;cbfa3aA2fh8Fczhacbhrcbhqina8Faqfhodndndndndndna8Ea5aaglc9Wfco4fRbbarcoG4ciGcdtfydbPDbedvivvvlvkao9cb83bbaocwf9cb83bbxlkaocbaiRbdai8Xbb9c:c:qj:bw9:9c:q;c1:I1e:d9c:b:c:e1z9:gh9cjjjjjz:dgg9qE86bbaoaicdfag9c8N1:NfgaRbbah9c8F1:Nc;:eGg8Jcr4a8JcFb0g8JE86beaocbaaa8JfgaRbbah9cjjjjjl:dgg9qE86bdaocbaaag9c8L1:NfgaRbbah9cjjjjjd:dgg9qE86biaocbaaag9c8K1:NfgaRbbah9cjjjjje:dgg9qE86blaocbaaag9c8J1:NfgaRbbah9cjjjj;ab:dgg9qE86bvaocbaaag9cg1:NfgaRbbah9cjjjja:dgg9qE86boaocbaaag9ch1:NfgaRbbah9cjjjjz:dgh9qE86braocbaaah9ca1:NfgaRbbai8Xbe9c:c:qj:bw9:9c:q;c1:I1e:d9c:b:c:e1z9:gh9cjjjjjz:dgg9qE86bwaoaaag9c8N1:NfgiRbbah9c8F1:Nc;:eGgacr4aacFb0gaE86bDaocbaiaafgiRbbah9cjjjjjl:dgg9qE86bqaocbaiag9c8L1:NfgiRbbah9cjjjjjd:dgg9qE86bkaocbaiag9c8K1:NfgiRbbah9cjjjjje:dgg9qE86bxaocbaiag9c8J1:NfgiRbbah9cjjjj;ab:dgg9qE86bmaocbaiag9cg1:NfgiRbbah9cjjjja:dgg9qE86bPaocbaiag9ch1:NfgiRbbah9cjjjjz:dgh9qE86bsaiah9ca1:NfhixikaoaiRblaiRbbgaco4g8Ja8JciSg8JE86bbaoaiclfa8Jfg8JRbbaacl4ciGg8Ka8KciSg8KE86beaoa8Ja8Kfg8JRbbaacd4ciGg8Ka8KciSg8KE86bdaoa8Ja8Kfg8JRbbaaciGgaaaciSgaE86biaoa8Jaafg8JRbbaiRbegaco4g8Ka8KciSg8KE86blaoa8Ja8Kfg8JRbbaacl4ciGg8Ka8KciSg8KE86bvaoa8Ja8Kfg8JRbbaacd4ciGg8Ka8KciSg8KE86boaoa8Ja8Kfg8JRbbaaciGgaaaciSgaE86braoa8Jaafg8JRbbaiRbdgaco4g8Ka8KciSg8KE86bwaoa8Ja8Kfg8JRbbaacl4ciGg8Ka8KciSg8KE86bDaoa8Ja8Kfg8JRbbaacd4ciGg8Ka8KciSg8KE86bqaoa8Ja8Kfg8JRbbaaciGgaaaciSgaE86bkaoa8JaafgaRbbaiRbigico4g8Ja8JciSg8JE86bxaoaaa8JfgaRbbaicl4ciGg8Ja8JciSg8JE86bmaoaaa8JfgaRbbaicd4ciGg8Ja8JciSg8JE86bPaoaaa8JfgaRbbaiciGgiaiciSgiE86bsaaaifhixdkaoaiRbwaiRbbgacl4g8Ja8JcsSg8JE86bbaoaicwfa8Jfg8JRbbaacsGgaaacsSgaE86beaoa8JaafgaRbbaiRbeg8Jcl4g8Ka8KcsSg8KE86bdaoaaa8KfgaRbba8JcsGg8Ja8JcsSg8JE86biaoaaa8JfgaRbbaiRbdg8Jcl4g8Ka8KcsSg8KE86blaoaaa8KfgaRbba8JcsGg8Ja8JcsSg8JE86bvaoaaa8JfgaRbbaiRbig8Jcl4g8Ka8KcsSg8KE86boaoaaa8KfgaRbba8JcsGg8Ja8JcsSg8JE86braoaaa8JfgaRbbaiRblg8Jcl4g8Ka8KcsSg8KE86bwaoaaa8KfgaRbba8JcsGg8Ja8JcsSg8JE86bDaoaaa8JfgaRbbaiRbvg8Jcl4g8Ka8KcsSg8KE86bqaoaaa8KfgaRbba8JcsGg8Ja8JcsSg8JE86bkaoaaa8JfgaRbbaiRbog8Jcl4g8Ka8KcsSg8KE86bxaoaaa8KfgaRbba8JcsGg8Ja8JcsSg8JE86bmaoaaa8JfgaRbbaiRbrgicl4g8Ja8JcsSg8JE86bPaoaaa8JfgaRbbaicsGgiaicsSgiE86bsaaaifhixekaoai8Pbb83bbaocwfaicwf8Pbb83bbaiczfhikdnalaC9pmbarcdfhralczfhaaqczfhqaPai9RcL0mekkalaC6moaimexokaCmva5Tmvka3cefg3cl9hmbkdndndndnawTmbasaKcd4fRbbgociGPibedrkaATmdazaKfh8Eazcj;cbfh5cbh3aYh8Fina8ERbbhra8FhoaAhqcbhlinaoa5alfRbbgace4cbaaceG9R7arfgr86bbaoadfhoalcefhlaqcufgqmbka8Fcefh8Fa8Ecefh8Ea5aAfh5a3cefg3cl9hmbxikkaATmeazaKfh8Eazcj;cbfhrcbhoceh8FinaYaofhla8E8VbbhqaAhacbhoinalaraoaAffRbbcwtaraofRbbg5Vc;:FiGce4cba5ceG9R7aqfgq87bbaladfhlaocefhoaacufgambkcdhoa8Ecdfh8EaraQfhra8FceGhlcbh8FalmbxdkkaATmbcbaocl49Rh8FazaKfRbbhqcwhoa8AhlinalRbbaotaqVhqalcefhlaocwfgoca9hmbkcbh5aYh8Einazcj;cbfa5fgaRbbhrcehocwhlinaaaoaA2fRbbaltarVhraocefhoalcwfglca9hmbkara8F93aq7hqcbhoa8Ehlinalaqao486bbalcefhlaocwfgoca9hmbka8Eadfh8Ea5cefg5aA9hmbkkaYclfhYa8Aclfh8AaKclfgKad6mbkaXazcjdfaAad2z1jjjb8AazazcjdfaAcufad2fadz1jjjb8AaAaOfhOaihxaimbkc9:hoxdkcbc99aPax9RakSEhoxekc9:hokavcj;kbf8Kjjjjbaok;cseHu8Jjjjjbc;ae9Rgv8Kjjjjbc9:hodnaeci9UgrcHfal0mbcuhoaiRbbgwc;WeGc;Ge9hmbawcsGgwce0mbavc;abfcFecjez:jjjjb8AavcUf9cu83ibavc8Wf9cu83ibavcyf9cu83ibavcaf9cu83ibavcKf9cu83ibavczf9cu83ibav9cu83iwav9cu83ibaialfc9WfhDaicefgqarfhidnaeTmbcmcsawceSEhkcbhxcbhmcbhPcbhwcbhlindnaiaD9nmbc9:hoxikdndnaqRbbgoc;Ve0mbavc;abfalaocu7gscl4fcsGcitfgzydlhrazydbhzdnaocsGgHak9pmbavawasfcsGcdtfydbaxaHEhoaHThsdndnadcd9hmbabaPcetfgHaz87ebaHclfao87ebaHcdfar87ebxekabaPcdtfgHazBdbaHcwfaoBdbaHclfarBdbkaxasfhxcdhHavawcdtfaoBdbawasfhwcehsalhOxdkdndnaHcsSmbaHc987aHamffcefhoxekaicefhoai8SbbgHcFeGhsdndnaHcu9mmbaohixekaicvfhiascFbGhscrhHdninao8SbbgOcFbGaHtasVhsaOcu9kmeaocefhoaHcrfgHc8J9hmbxdkkaocefhikasce4cbasceG9R7amfhokdndnadcd9hmbabaPcetfgHaz87ebaHclfao87ebaHcdfar87ebxekabaPcdtfgHazBdbaHcwfaoBdbaHclfarBdbkcdhHavawcdtfaoBdbcehsawcefhwalhOaohmxekdnaocpe0mbaxcefgHavawaDaocsGfRbbgocl49RcsGcdtfydbaocz6gzEhravawao9RcsGcdtfydbaHazfgAaocsGgHEhoaHThCdndnadcd9hmbabaPcetfgHax87ebaHclfao87ebaHcdfar87ebxekabaPcdtfgHaxBdbaHcwfaoBdbaHclfarBdbkcdhsavawcdtfaxBdbavawcefgwcsGcdtfarBdbcihHavc;abfalcitfgOaxBdlaOarBdbavawazfgwcsGcdtfaoBdbalcefcsGhOawaCfhwaxhzaAaCfhxxekaxcbaiRbbgOEgzaoc;:eSgHfhraOcsGhCaOcl4hAdndnaOcs0mbarcefhoxekarhoavawaA9RcsGcdtfydbhrkdndnaCmbaocefhxxekaohxavawaO9RcsGcdtfydbhokdndnaHTmbaicefhHxekaicdfhHai8SbegscFeGhzdnascu9kmbaicofhXazcFbGhzcrhidninaH8SbbgscFbGaitazVhzascu9kmeaHcefhHaicrfgic8J9hmbkaXhHxekaHcefhHkazce4cbazceG9R7amfgmhzkdndnaAcsSmbaHhsxekaHcefhsaH8SbbgicFeGhrdnaicu9kmbaHcvfhXarcFbGhrcrhidninas8SbbgHcFbGaitarVhraHcu9kmeascefhsaicrfgic8J9hmbkaXhsxekascefhskarce4cbarceG9R7amfgmhrkdndnaCcsSmbashixekascefhias8SbbgocFeGhHdnaocu9kmbascvfhXaHcFbGhHcrhodninai8SbbgscFbGaotaHVhHascu9kmeaicefhiaocrfgoc8J9hmbkaXhixekaicefhikaHce4cbaHceG9R7amfgmhokdndnadcd9hmbabaPcetfgHaz87ebaHclfao87ebaHcdfar87ebxekabaPcdtfgHazBdbaHcwfaoBdbaHclfarBdbkcdhsavawcdtfazBdbavawcefgwcsGcdtfarBdbcihHavc;abfalcitfgXazBdlaXarBdbavawaOcz6aAcsSVfgwcsGcdtfaoBdbawaCTaCcsSVfhwalcefcsGhOkaqcefhqavc;abfaOcitfgOarBdlaOaoBdbavc;abfalasfcsGcitfgraoBdlarazBdbawcsGhwalaHfcsGhlaPcifgPae6mbkkcbc99aiaDSEhokavc;aef8Kjjjjbaok:flevu8Jjjjjbcz9Rhvc9:hodnaecvfal0mbcuhoaiRbbc;:eGc;qe9hmbav9cb83iwaicefhraialfc98fhwdnaeTmbdnadcdSmbcbhDindnaraw6mbc9:skarcefhoar8SbbglcFeGhidndnalcu9mmbaohrxekarcvfhraicFbGhicrhldninao8SbbgdcFbGaltaiVhiadcu9kmeaocefhoalcrfglc8J9hmbxdkkaocefhrkabaDcdtfaic8Etc8F91aicd47avcwfaiceGcdtVgoydbfglBdbaoalBdbaDcefgDae9hmbxdkkcbhDindnaraw6mbc9:skarcefhoar8SbbglcFeGhidndnalcu9mmbaohrxekarcvfhraicFbGhicrhldninao8SbbgdcFbGaltaiVhiadcu9kmeaocefhoalcrfglc8J9hmbxdkkaocefhrkabaDcetfaic8Etc8F91aicd47avcwfaiceGcdtVgoydbfgl87ebaoalBdbaDcefgDae9hmbkkcbc99arawSEhokaok:Lvoeue99dud99eud99dndnadcl9hmbaeTmeindndnabcdfgd8Sbb:Yab8Sbbgi:Ygl:l:tabcefgv8Sbbgo:Ygr:l:tgwJbb;:9cawawNJbbbbawawJbbbb9GgDEgq:mgkaqaicb9iEalMgwawNakaqaocb9iEarMgqaqNMM:r:vglNJbbbZJbbb:;aDEMgr:lJbbb9p9DTmbar:Ohixekcjjjj94hikadai86bbdndnaqalNJbbbZJbbb:;aqJbbbb9GEMgq:lJbbb9p9DTmbaq:Ohdxekcjjjj94hdkavad86bbdndnawalNJbbbZJbbb:;awJbbbb9GEMgw:lJbbb9p9DTmbaw:Ohdxekcjjjj94hdkabad86bbabclfhbaecufgembxdkkaeTmbindndnabclfgd8Ueb:Yab8Uebgi:Ygl:l:tabcdfgv8Uebgo:Ygr:l:tgwJb;:FSawawNJbbbbawawJbbbb9GgDEgq:mgkaqaicb9iEalMgwawNakaqaocb9iEarMgqaqNMM:r:vglNJbbbZJbbb:;aDEMgr:lJbbb9p9DTmbar:Ohixekcjjjj94hikadai87ebdndnaqalNJbbbZJbbb:;aqJbbbb9GEMgq:lJbbb9p9DTmbaq:Ohdxekcjjjj94hdkavad87ebdndnawalNJbbbZJbbb:;awJbbbb9GEMgw:lJbbb9p9DTmbaw:Ohdxekcjjjj94hdkabad87ebabcwfhbaecufgembkkk;oiliui99iue99dnaeTmbcbhiabhlindndnJ;Zl81Zalcof8UebgvciV:Y:vgoal8Ueb:YNgrJb;:FSNJbbbZJbbb:;arJbbbb9GEMgw:lJbbb9p9DTmbaw:OhDxekcjjjj94hDkalclf8Uebhqalcdf8UebhkabaiavcefciGfcetfaD87ebdndnaoak:YNgwJb;:FSNJbbbZJbbb:;awJbbbb9GEMgx:lJbbb9p9DTmbax:OhDxekcjjjj94hDkabaiavciGfgkcd7cetfaD87ebdndnaoaq:YNgoJb;:FSNJbbbZJbbb:;aoJbbbb9GEMgx:lJbbb9p9DTmbax:OhDxekcjjjj94hDkabaiavcufciGfcetfaD87ebdndnJbbjZararN:tawawN:taoaoN:tgrJbbbbarJbbbb9GE:rJb;:FSNJbbbZMgr:lJbbb9p9DTmbar:Ohvxekcjjjj94hvkabakcetfav87ebalcwfhlaiclfhiaecufgembkkk9mbdnadcd4ae2gdTmbinababydbgecwtcw91:Yaece91cjjj98Gcjjj;8if::NUdbabclfhbadcufgdmbkkk9teiucbcbyd:K1jjbgeabcifc98GfgbBd:K1jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaik;LeeeudndnaeabVciGTmbabhixekdndnadcz9pmbabhixekabhiinaiaeydbBdbaiclfaeclfydbBdbaicwfaecwfydbBdbaicxfaecxfydbBdbaeczfheaiczfhiadc9Wfgdcs0mbkkadcl6mbinaiaeydbBdbaeclfheaiclfhiadc98fgdci0mbkkdnadTmbinaiaeRbb86bbaicefhiaecefheadcufgdmbkkabk;aeedudndnabciGTmbabhixekaecFeGc:b:c:ew2hldndnadcz9pmbabhixekabhiinaialBdbaicxfalBdbaicwfalBdbaiclfalBdbaiczfhiadc9Wfgdcs0mbkkadcl6mbinaialBdbaiclfhiadc98fgdci0mbkkdnadTmbinaiae86bbaicefhiadcufgdmbkkabkk81dbcjwk8Kbbbbdbbblbbbwbbbbbbbebbbdbbblbbbwbbbbc:Kwkl8WNbb'; // embed! base
	var wasm_simd =
		'b9H79TebbbeKl9Gbb9Gvuuuuueu9Giuuub9Geueuikqbbebeedddilve9Weeeviebeoweuec:q:6dkr;leDo9TW9T9VV95dbH9F9F939H79T9F9J9H229F9Jt9VV7bb8A9TW79O9V9Wt9F9KW9J9V9KW9wWVtW949c919M9MWVbdY9TW79O9V9Wt9F9KW9J9V9KW69U9KW949c919M9MWVblE9TW79O9V9Wt9F9KW9J9V9KW69U9KW949tWG91W9U9JWbvL9TW79O9V9Wt9F9KW9J9V9KWS9P2tWV9p9JtboK9TW79O9V9Wt9F9KW9J9V9KWS9P2tWV9r919HtbrL9TW79O9V9Wt9F9KW9J9V9KWS9P2tWVT949Wbwl79IV9RbDq;i9Nqlbzik9:evu8Jjjjjbcz9Rhbcbheincbhdcbhiinabcwfadfaicjuaead4ceGglE86bbaialfhiadcefgdcw9hmbkaec:q:yjjbfai86bbaecitc:q1jjbfab8Piw83ibaecefgecjd9hmbkk;GUlKud97dur978Jjjjjbcj;kb9Rgv8Kjjjjbc9:hodnalTmbcuhoaiRbbgrc;WeGc:Ge9hmbarcsGgwce0mbc9:hoalcufadcd4cbawEgDadfgrcKcaawEgqaraq0Egk6mbaicefhxavaialfgmar9Rgrad;8qbbcj;abad9Uc;WFbGgocjdaocjd6EhPdndndnadTmbaradfhscbhzinaeaz9nmdamax9RaD6miabazad2fhHaPaeaz9RazaPfae6EgOcsfgoc9WGgAci2hCaAcethXaxaDfhQaocl4cifcd4hLcbhKaAc;ab6hYincbhodnawTmbaxaKcd4fRbbhokaocFeGh8AcbhEindndndndna8AaEcet4ciGgoc9:fPdebdkamaQ9RaA6mwavcj;cbfaEaA2faQaA;8qbbaQaOfhQxdkavcj;cbfaEaA2fcbaA;8kbxekamaQ9RaL6moaoclVcbawEhravcj;cbfaEaA2fh3aQaLfhocbhidnaYmbamao9Rc;Gb6mbcbhlina3alfhidndndndndndnaQalco4fRbbgqciGarfPDbedibledibkaipxbbbbbbbbbbbbbbbbpklbxlkaiaopbblaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLg5cdp:mea5pmbzeHdOiAlCvXoQrLpxiiiiiiiiiiiiiiiip9og8Epxiiiiiiiiiiiiiiiip8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8Fpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ngacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spklba8Faoclffaac:q:yjjbfRbbfhoxikaiaopbbwaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLpxssssssssssssssssp9og8Epxssssssssssssssssp8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8Fpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ngacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spklba8Faocwffaac:q:yjjbfRbbfhoxdkaiaopbbbpklbaoczfhoxekaiaopbbdaoRbbg8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8FpsaoRbegacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPpklba8Faocdffaac:q:yjjbfRbbfhokdndndndndndnaqcd4ciGarfPDbedibledibkaipxbbbbbbbbbbbbbbbbpklzxlkaiaopbblaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLg5cdp:mea5pmbzeHdOiAlCvXoQrLpxiiiiiiiiiiiiiiiip9og8Epxiiiiiiiiiiiiiiiip8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8Fpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ngacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spklza8Faoclffaac:q:yjjbfRbbfhoxikaiaopbbwaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLpxssssssssssssssssp9og8Epxssssssssssssssssp8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8Fpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ngacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spklza8Faocwffaac:q:yjjbfRbbfhoxdkaiaopbbbpklzaoczfhoxekaiaopbbdaoRbbg8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8FpsaoRbegacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPpklza8Faocdffaac:q:yjjbfRbbfhokdndndndndndnaqcl4ciGarfPDbedibledibkaipxbbbbbbbbbbbbbbbbpklaxlkaiaopbblaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLg5cdp:mea5pmbzeHdOiAlCvXoQrLpxiiiiiiiiiiiiiiiip9og8Epxiiiiiiiiiiiiiiiip8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8Fpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ngacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spklaa8Faoclffaac:q:yjjbfRbbfhoxikaiaopbbwaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLpxssssssssssssssssp9og8Epxssssssssssssssssp8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8Fpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ngacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spklaa8Faocwffaac:q:yjjbfRbbfhoxdkaiaopbbbpklaaoczfhoxekaiaopbbdaoRbbg8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8FpsaoRbegacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPpklaa8Faocdffaac:q:yjjbfRbbfhokdndndndndndnaqco4arfPDbedibledibkaipxbbbbbbbbbbbbbbbbpkl8Wxlkaiaopbblaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLg5cdp:mea5pmbzeHdOiAlCvXoQrLpxiiiiiiiiiiiiiiiip9og8Epxiiiiiiiiiiiiiiiip8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ngqcitc:q1jjbfpbibaqc:q:yjjbfRbbgqpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spkl8Waqaoclffa8Fc:q:yjjbfRbbfhoxikaiaopbbwaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLpxssssssssssssssssp9og8Epxssssssssssssssssp8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ngqcitc:q1jjbfpbibaqc:q:yjjbfRbbgqpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spkl8Waqaocwffa8Fc:q:yjjbfRbbfhoxdkaiaopbbbpkl8WaoczfhoxekaiaopbbdaoRbbgqcitc:q1jjbfpbibaqc:q:yjjbfRbbgqpsaoRbeg8Fcitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPpkl8Waqaocdffa8Fc:q:yjjbfRbbfhokalc;abfhialcjefaA0meaihlamao9Rc;Fb0mbkkdnaiaA9pmbaici4hlinamao9RcK6mwa3aifhqdndndndndndnaQaico4fRbbalcoG4ciGarfPDbedibledibkaqpxbbbbbbbbbbbbbbbbpkbbxlkaqaopbblaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLg5cdp:mea5pmbzeHdOiAlCvXoQrLpxiiiiiiiiiiiiiiiip9og8Epxiiiiiiiiiiiiiiiip8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8Fpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ngacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spkbba8Faoclffaac:q:yjjbfRbbfhoxikaqaopbbwaopbbbg5clp:mea5pmbzeHdOiAlCvXoQrLpxssssssssssssssssp9og8Epxssssssssssssssssp8Jg5p5b9cjF;8;4;W;G;ab9:9cU1:Ng8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8Fpsa5p5e9cjF;8;4;W;G;ab9:9cU1:Ngacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPa8Ea5p9spkbba8Faocwffaac:q:yjjbfRbbfhoxdkaqaopbbbpkbbaoczfhoxekaqaopbbdaoRbbg8Fcitc:q1jjbfpbiba8Fc:q:yjjbfRbbg8FpsaoRbegacitc:q1jjbfpbibp9UpmbedilvorzHOACXQLpPpkbba8Faocdffaac:q:yjjbfRbbfhokalcdfhlaiczfgiaA6mbkkaohQaoTmokaEcefgEcl9hmbkdndndndnawTmbasaKcd4fRbbgociGPibedwkaATmdavcjdfaKfhlavaKfpbdbhhcbhiinalavcj;cbfaifgopblbggaoaAfpblbg8JpmbzeHdOiAlCvXoQrLg8KaoaXfpblbg8LaoaCfpblbg8MpmbzeHdOiAlCvXoQrLg8NpmbezHdiOAlvCXorQLg5cep9Ta5pxeeeeeeeeeeeeeeeeg8Ep9op9Hp9rg5a5pmbedibedibedibediahp9Ughp9Adbbaladfgoaha5a5pmlvorlvorlvorlvorp9Ughp9Adbbaoadfgoaha5a5pmwDqkwDqkwDqkwDqkp9Ughp9Adbbaoadfgoaha5a5pmxmPsxmPsxmPsxmPsp9Ughp9Adbbaoadfgoaha8Ka8NpmwDKYqk8AExm35Ps8E8Fg5cep9Ta5a8Ep9op9Hp9rg5a5pmbedibedibedibedip9Ughp9Adbbaoadfgoaha5a5pmlvorlvorlvorlvorp9Ughp9Adbbaoadfgoaha5a5pmwDqkwDqkwDqkwDqkp9Ughp9Adbbaoadfgoaha5a5pmxmPsxmPsxmPsxmPsp9Ughp9Adbbaoadfgoahaga8JpmwKDYq8AkEx3m5P8Es8Fgga8La8MpmwKDYq8AkEx3m5P8Es8Fg8JpmbezHdiOAlvCXorQLg5cep9Ta5a8Ep9op9Hp9rg5a5pmbedibedibedibedip9Ughp9Adbbaoadfgoaha5a5pmlvorlvorlvorlvorp9Ughp9Adbbaoadfgoaha5a5pmwDqkwDqkwDqkwDqkp9Ughp9Adbbaoadfgoaha5a5pmxmPsxmPsxmPsxmPsp9Ughp9Adbbaoadfgoahaga8JpmwDKYqk8AExm35Ps8E8Fg5cep9Ta5a8Ep9op9Hp9rg5a5pmbedibedibedibedip9Ug8Ep9Adbbaoadfgoa8Ea5a5pmlvorlvorlvorlvorp9Ug8Ep9Adbbaoadfgoa8Ea5a5pmwDqkwDqkwDqkwDqkp9Ug8Ep9Adbbaoadfgoa8Ea5a5pmxmPsxmPsxmPsxmPsp9Ughp9AdbbaoadfhlaiczfgiaA6mbxikkaATmeavcjdfaKfhlavaKfpbdbhhcbhiinalahavcj;cbfaifgopblbggaoaAfpblbg8JpmbzeHdOiAlCvXoQrLg8KaoaXfpblbg8LaoaCfpblbg8MpmbzeHdOiAlCvXoQrLg8NpmbezHdiOAlvCXorQLg5cep:nea5pxebebebebebebebebg8Ep9op:bep9rg5a5pmbedibedibedibedip:oeghp9Adbbaladfgoaha5a5pmlvorlvorlvorlvorp:oeghp9Adbbaoadfgoaha5a5pmwDqkwDqkwDqkwDqkp:oeghp9Adbbaoadfgoaha5a5pmxmPsxmPsxmPsxmPsp:oeghp9Adbbaoadfgoaha8Ka8NpmwDKYqk8AExm35Ps8E8Fg5cep:nea5a8Ep9op:bep9rg5a5pmbedibedibedibedip:oeghp9Adbbaoadfgoaha5a5pmlvorlvorlvorlvorp:oeghp9Adbbaoadfgoaha5a5pmwDqkwDqkwDqkwDqkp:oeghp9Adbbaoadfgoaha5a5pmxmPsxmPsxmPsxmPsp:oeghp9Adbbaoadfgoahaga8JpmwKDYq8AkEx3m5P8Es8Fgga8La8MpmwKDYq8AkEx3m5P8Es8Fg8JpmbezHdiOAlvCXorQLg5cep:nea5a8Ep9op:bep9rg5a5pmbedibedibedibedip:oeghp9Adbbaoadfgoaha5a5pmlvorlvorlvorlvorp:oeghp9Adbbaoadfgoaha5a5pmwDqkwDqkwDqkwDqkp:oeghp9Adbbaoadfgoaha5a5pmxmPsxmPsxmPsxmPsp:oeghp9Adbbaoadfgoahaga8JpmwDKYqk8AExm35Ps8E8Fg5cep:nea5a8Ep9op:bep9rg5a5pmbedibedibedibedip:oeg8Ep9Adbbaoadfgoa8Ea5a5pmlvorlvorlvorlvorp:oeg8Ep9Adbbaoadfgoa8Ea5a5pmwDqkwDqkwDqkwDqkp:oeg8Ep9Adbbaoadfgoa8Ea5a5pmxmPsxmPsxmPsxmPsp:oeghp9AdbbaoadfhlaiczfgiaA6mbxdkkaATmbcbhrcbaocl4go9Rc8FGhlavcjdfaKfhqavaKfpbdbh8Einaqa8Eavcj;cbfarfgipblbghaiaAfpblbggpmbzeHdOiAlCvXoQrLg8JaiaXfpblbg8KaiaCfpblbg8LpmbzeHdOiAlCvXoQrLg8MpmbezHdiOAlvCXorQLg5alp:Rea5aop:Sep9qg5a5pmbedibedibedibedip9rg8Ep9Adbbaqadfgia8Ea5a5pmlvorlvorlvorlvorp9rg8Ep9Adbbaiadfgia8Ea5a5pmwDqkwDqkwDqkwDqkp9rg8Ep9Adbbaiadfgia8Ea5a5pmxmPsxmPsxmPsxmPsp9rg8Ep9Adbbaiadfgia8Ea8Ja8MpmwDKYqk8AExm35Ps8E8Fg5alp:Rea5aop:Sep9qg5a5pmbedibedibedibedip9rg8Ep9Adbbaiadfgia8Ea5a5pmlvorlvorlvorlvorp9rg8Ep9Adbbaiadfgia8Ea5a5pmwDqkwDqkwDqkwDqkp9rg8Ep9Adbbaiadfgia8Ea5a5pmxmPsxmPsxmPsxmPsp9rg8Ep9Adbbaiadfgia8EahagpmwKDYq8AkEx3m5P8Es8Fgha8Ka8LpmwKDYq8AkEx3m5P8Es8FggpmbezHdiOAlvCXorQLg5alp:Rea5aop:Sep9qg5a5pmbedibedibedibedip9rg8Ep9Adbbaiadfgia8Ea5a5pmlvorlvorlvorlvorp9rg8Ep9Adbbaiadfgia8Ea5a5pmwDqkwDqkwDqkwDqkp9rg8Ep9Adbbaiadfgia8Ea5a5pmxmPsxmPsxmPsxmPsp9rg8Ep9Adbbaiadfgia8EahagpmwDKYqk8AExm35Ps8E8Fg5alp:Rea5aop:Sep9qg5a5pmbedibedibedibedip9rg8Ep9Adbbaiadfgia8Ea5a5pmlvorlvorlvorlvorp9rg8Ep9Adbbaiadfgia8Ea5a5pmwDqkwDqkwDqkwDqkp9rg8Ep9Adbbaiadfgia8Ea5a5pmxmPsxmPsxmPsxmPsp9rg8Ep9AdbbaiadfhqarczfgraA6mbkkaKclfgKad6mbkaHavcjdfaOad2;8qbbavavcjdfaOcufad2fad;8qbbaOazfhzc9:hoaQhxaQmbxlkkaeTmbaDalfhrcbhocuhlinaralaD9RglfaD6mdaPaeao9Raeao0Eaofgoae6mbkaial9Rhxkcbc99amax9RakSEhoxekc9:hokavcj;kbf8Kjjjjbaokwbz:bjjjbk::seHu8Jjjjjbc;ae9Rgv8Kjjjjbc9:hodnaeci9UgrcHfal0mbcuhoaiRbbgwc;WeGc;Ge9hmbawcsGgwce0mbavc;abfcFecje;8kbavcUf9cu83ibavc8Wf9cu83ibavcyf9cu83ibavcaf9cu83ibavcKf9cu83ibavczf9cu83ibav9cu83iwav9cu83ibaialfc9WfhDaicefgqarfhidnaeTmbcmcsawceSEhkcbhxcbhmcbhPcbhwcbhlindnaiaD9nmbc9:hoxikdndnaqRbbgoc;Ve0mbavc;abfalaocu7gscl4fcsGcitfgzydlhrazydbhzdnaocsGgHak9pmbavawasfcsGcdtfydbaxaHEhoaHThsdndnadcd9hmbabaPcetfgHaz87ebaHclfao87ebaHcdfar87ebxekabaPcdtfgHazBdbaHcwfaoBdbaHclfarBdbkaxasfhxcdhHavawcdtfaoBdbawasfhwcehsalhOxdkdndnaHcsSmbaHc987aHamffcefhoxekaicefhoai8SbbgHcFeGhsdndnaHcu9mmbaohixekaicvfhiascFbGhscrhHdninao8SbbgOcFbGaHtasVhsaOcu9kmeaocefhoaHcrfgHc8J9hmbxdkkaocefhikasce4cbasceG9R7amfhokdndnadcd9hmbabaPcetfgHaz87ebaHclfao87ebaHcdfar87ebxekabaPcdtfgHazBdbaHcwfaoBdbaHclfarBdbkcdhHavawcdtfaoBdbcehsawcefhwalhOaohmxekdnaocpe0mbaxcefgHavawaDaocsGfRbbgocl49RcsGcdtfydbaocz6gzEhravawao9RcsGcdtfydbaHazfgAaocsGgHEhoaHThCdndnadcd9hmbabaPcetfgHax87ebaHclfao87ebaHcdfar87ebxekabaPcdtfgHaxBdbaHcwfaoBdbaHclfarBdbkcdhsavawcdtfaxBdbavawcefgwcsGcdtfarBdbcihHavc;abfalcitfgOaxBdlaOarBdbavawazfgwcsGcdtfaoBdbalcefcsGhOawaCfhwaxhzaAaCfhxxekaxcbaiRbbgOEgzaoc;:eSgHfhraOcsGhCaOcl4hAdndnaOcs0mbarcefhoxekarhoavawaA9RcsGcdtfydbhrkdndnaCmbaocefhxxekaohxavawaO9RcsGcdtfydbhokdndnaHTmbaicefhHxekaicdfhHai8SbegscFeGhzdnascu9kmbaicofhXazcFbGhzcrhidninaH8SbbgscFbGaitazVhzascu9kmeaHcefhHaicrfgic8J9hmbkaXhHxekaHcefhHkazce4cbazceG9R7amfgmhzkdndnaAcsSmbaHhsxekaHcefhsaH8SbbgicFeGhrdnaicu9kmbaHcvfhXarcFbGhrcrhidninas8SbbgHcFbGaitarVhraHcu9kmeascefhsaicrfgic8J9hmbkaXhsxekascefhskarce4cbarceG9R7amfgmhrkdndnaCcsSmbashixekascefhias8SbbgocFeGhHdnaocu9kmbascvfhXaHcFbGhHcrhodninai8SbbgscFbGaotaHVhHascu9kmeaicefhiaocrfgoc8J9hmbkaXhixekaicefhikaHce4cbaHceG9R7amfgmhokdndnadcd9hmbabaPcetfgHaz87ebaHclfao87ebaHcdfar87ebxekabaPcdtfgHazBdbaHcwfaoBdbaHclfarBdbkcdhsavawcdtfazBdbavawcefgwcsGcdtfarBdbcihHavc;abfalcitfgXazBdlaXarBdbavawaOcz6aAcsSVfgwcsGcdtfaoBdbawaCTaCcsSVfhwalcefcsGhOkaqcefhqavc;abfaOcitfgOarBdlaOaoBdbavc;abfalasfcsGcitfgraoBdlarazBdbawcsGhwalaHfcsGhlaPcifgPae6mbkkcbc99aiaDSEhokavc;aef8Kjjjjbaok:flevu8Jjjjjbcz9Rhvc9:hodnaecvfal0mbcuhoaiRbbc;:eGc;qe9hmbav9cb83iwaicefhraialfc98fhwdnaeTmbdnadcdSmbcbhDindnaraw6mbc9:skarcefhoar8SbbglcFeGhidndnalcu9mmbaohrxekarcvfhraicFbGhicrhldninao8SbbgdcFbGaltaiVhiadcu9kmeaocefhoalcrfglc8J9hmbxdkkaocefhrkabaDcdtfaic8Etc8F91aicd47avcwfaiceGcdtVgoydbfglBdbaoalBdbaDcefgDae9hmbxdkkcbhDindnaraw6mbc9:skarcefhoar8SbbglcFeGhidndnalcu9mmbaohrxekarcvfhraicFbGhicrhldninao8SbbgdcFbGaltaiVhiadcu9kmeaocefhoalcrfglc8J9hmbxdkkaocefhrkabaDcetfaic8Etc8F91aicd47avcwfaiceGcdtVgoydbfgl87ebaoalBdbaDcefgDae9hmbkkcbc99arawSEhokaok:wPliuo97eue978Jjjjjbca9Rhiaec98Ghldndnadcl9hmbdnalTmbcbhvabhdinadadpbbbgocKp:RecKp:Sep;6egraocwp:RecKp:Sep;6earp;Geaoczp:RecKp:Sep;6egwp;Gep;Kep;LegDpxbbbbbbbbbbbbbbbbp:2egqarpxbbbjbbbjbbbjbbbjgkp9op9rp;Kegrpxbb;:9cbb;:9cbb;:9cbb;:9cararp;MeaDaDp;Meawaqawakp9op9rp;Kegrarp;Mep;Kep;Kep;Jep;Negwp;Mepxbbn0bbn0bbn0bbn0gqp;KepxFbbbFbbbFbbbFbbbp9oaopxbbbFbbbFbbbFbbbFp9op9qarawp;Meaqp;Kecwp:RepxbFbbbFbbbFbbbFbbp9op9qaDawp;Meaqp;Keczp:RepxbbFbbbFbbbFbbbFbp9op9qpkbbadczfhdavclfgval6mbkkalaeSmeaipxbbbbbbbbbbbbbbbbgqpklbaiabalcdtfgdaeciGglcdtgv;8qbbdnalTmbaiaipblbgocKp:RecKp:Sep;6egraocwp:RecKp:Sep;6earp;Geaoczp:RecKp:Sep;6egwp;Gep;Kep;LegDaqp:2egqarpxbbbjbbbjbbbjbbbjgkp9op9rp;Kegrpxbb;:9cbb;:9cbb;:9cbb;:9cararp;MeaDaDp;Meawaqawakp9op9rp;Kegrarp;Mep;Kep;Kep;Jep;Negwp;Mepxbbn0bbn0bbn0bbn0gqp;KepxFbbbFbbbFbbbFbbbp9oaopxbbbFbbbFbbbFbbbFp9op9qarawp;Meaqp;Kecwp:RepxbFbbbFbbbFbbbFbbp9op9qaDawp;Meaqp;Keczp:RepxbbFbbbFbbbFbbbFbp9op9qpklbkadaiav;8qbbskdnalTmbcbhvabhdinadczfgxaxpbbbgopxbbbbbbFFbbbbbbFFgkp9oadpbbbgDaopmbediwDqkzHOAKY8AEgwczp:Reczp:Sep;6egraDaopmlvorxmPsCXQL358E8FpxFubbFubbFubbFubbp9op;6eawczp:Sep;6egwp;Gearp;Gep;Kep;Legopxbbbbbbbbbbbbbbbbp:2egqarpxbbbjbbbjbbbjbbbjgmp9op9rp;Kegrpxb;:FSb;:FSb;:FSb;:FSararp;Meaoaop;Meawaqawamp9op9rp;Kegrarp;Mep;Kep;Kep;Jep;Negwp;Mepxbbn0bbn0bbn0bbn0gqp;KepxFFbbFFbbFFbbFFbbp9oaoawp;Meaqp;Keczp:Rep9qgoarawp;Meaqp;KepxFFbbFFbbFFbbFFbbp9ogrpmwDKYqk8AExm35Ps8E8Fp9qpkbbadaDakp9oaoarpmbezHdiOAlvCXorQLp9qpkbbadcafhdavclfgval6mbkkalaeSmbaiaeciGgvcitgdfcbcaad9R;8kbaiabalcitfglad;8qbbdnavTmbaiaipblzgopxbbbbbbFFbbbbbbFFgkp9oaipblbgDaopmbediwDqkzHOAKY8AEgwczp:Reczp:Sep;6egraDaopmlvorxmPsCXQL358E8FpxFubbFubbFubbFubbp9op;6eawczp:Sep;6egwp;Gearp;Gep;Kep;Legopxbbbbbbbbbbbbbbbbp:2egqarpxbbbjbbbjbbbjbbbjgmp9op9rp;Kegrpxb;:FSb;:FSb;:FSb;:FSararp;Meaoaop;Meawaqawamp9op9rp;Kegrarp;Mep;Kep;Kep;Jep;Negwp;Mepxbbn0bbn0bbn0bbn0gqp;KepxFFbbFFbbFFbbFFbbp9oaoawp;Meaqp;Keczp:Rep9qgoarawp;Meaqp;KepxFFbbFFbbFFbbFFbbp9ogrpmwDKYqk8AExm35Ps8E8Fp9qpklzaiaDakp9oaoarpmbezHdiOAlvCXorQLp9qpklbkalaiad;8qbbkk;4wllue97euv978Jjjjjbc8W9Rhidnaec98GglTmbcbhvabhoinaiaopbbbgraoczfgwpbbbgDpmlvorxmPsCXQL358E8Fgqczp:Segkclp:RepklbaopxbbjZbbjZbbjZbbjZpx;Zl81Z;Zl81Z;Zl81Z;Zl81Zakpxibbbibbbibbbibbbp9qp;6ep;NegkaraDpmbediwDqkzHOAKY8AEgrczp:Reczp:Sep;6ep;MegDaDp;Meakarczp:Sep;6ep;Megxaxp;Meakaqczp:Reczp:Sep;6ep;Megqaqp;Mep;Kep;Kep;Lepxbbbbbbbbbbbbbbbbp:4ep;Jepxb;:FSb;:FSb;:FSb;:FSgkp;Mepxbbn0bbn0bbn0bbn0grp;KepxFFbbFFbbFFbbFFbbgmp9oaxakp;Mearp;Keczp:Rep9qgxaDakp;Mearp;Keamp9oaqakp;Mearp;Keczp:Rep9qgkpmbezHdiOAlvCXorQLgrp5baipblbpEb:T:j83ibaocwfarp5eaipblbpEe:T:j83ibawaxakpmwDKYqk8AExm35Ps8E8Fgkp5baipblbpEd:T:j83ibaocKfakp5eaipblbpEi:T:j83ibaocafhoavclfgval6mbkkdnalaeSmbaiaeciGgvcitgofcbcaao9R;8kbaiabalcitfgwao;8qbbdnavTmbaiaipblbgraipblzgDpmlvorxmPsCXQL358E8Fgqczp:Segkclp:RepklaaipxbbjZbbjZbbjZbbjZpx;Zl81Z;Zl81Z;Zl81Z;Zl81Zakpxibbbibbbibbbibbbp9qp;6ep;NegkaraDpmbediwDqkzHOAKY8AEgrczp:Reczp:Sep;6ep;MegDaDp;Meakarczp:Sep;6ep;Megxaxp;Meakaqczp:Reczp:Sep;6ep;Megqaqp;Mep;Kep;Kep;Lepxbbbbbbbbbbbbbbbbp:4ep;Jepxb;:FSb;:FSb;:FSb;:FSgkp;Mepxbbn0bbn0bbn0bbn0grp;KepxFFbbFFbbFFbbFFbbgmp9oaxakp;Mearp;Keczp:Rep9qgxaDakp;Mearp;Keamp9oaqakp;Mearp;Keczp:Rep9qgkpmbezHdiOAlvCXorQLgrp5baipblapEb:T:j83ibaiarp5eaipblapEe:T:j83iwaiaxakpmwDKYqk8AExm35Ps8E8Fgkp5baipblapEd:T:j83izaiakp5eaipblapEi:T:j83iKkawaiao;8qbbkk:Pddiue978Jjjjjbc;ab9Rhidnadcd4ae2glc98GgvTmbcbheabhdinadadpbbbgocwp:Recwp:Sep;6eaocep:SepxbbjFbbjFbbjFbbjFp9opxbbjZbbjZbbjZbbjZp:Uep;Mepkbbadczfhdaeclfgeav6mbkkdnavalSmbaialciGgecdtgdVcbc;abad9R;8kbaiabavcdtfgvad;8qbbdnaeTmbaiaipblbgocwp:Recwp:Sep;6eaocep:SepxbbjFbbjFbbjFbbjFp9opxbbjZbbjZbbjZbbjZp:Uep;Mepklbkavaiad;8qbbkk9teiucbcbydj1jjbgeabcifc98GfgbBdj1jjbdndnabZbcztgd9nmbcuhiabad9RcFFifcz4nbcuSmekaehikaikkkebcjwklz:Dbb'; // embed! simd

	var detector = new Uint8Array([
		0, 97, 115, 109, 1, 0, 0, 0, 1, 4, 1, 96, 0, 0, 3, 3, 2, 0, 0, 5, 3, 1, 0, 1, 12, 1, 0, 10, 22, 2, 12, 0, 65, 0, 65, 0, 65, 0, 252, 10, 0, 0,
		11, 7, 0, 65, 0, 253, 15, 26, 11,
	]);
	var wasmpack = new Uint8Array([
		32, 0, 65, 2, 1, 106, 34, 33, 3, 128, 11, 4, 13, 64, 6, 253, 10, 7, 15, 116, 127, 5, 8, 12, 40, 16, 19, 54, 20, 9, 27, 255, 113, 17, 42, 67,
		24, 23, 146, 148, 18, 14, 22, 45, 70, 69, 56, 114, 101, 21, 25, 63, 75, 136, 108, 28, 118, 29, 73, 115,
	]);

	if (typeof WebAssembly !== 'object') {
		return {
			supported: false,
		};
	}

	var wasm = WebAssembly.validate(detector) ? unpack(wasm_simd) : unpack(wasm_base);

	var instance;

	var ready = WebAssembly.instantiate(wasm, {}).then(function (result) {
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
			result[write++] = result[i] < 60 ? wasmpack[result[i]] : (result[i] - 60) * 64 + result[++i];
		}
		return result.buffer.slice(0, write);
	}

	function decode(instance, fun, target, count, size, source, filter) {
		var sbrk = instance.exports.sbrk;
		var count4 = (count + 3) & ~3;
		var tp = sbrk(count4 * size);
		var sp = sbrk(source.length);
		var heap = new Uint8Array(instance.exports.memory.buffer);
		heap.set(source, sp);
		var res = fun(tp, count, size, sp, source.length);
		if (res == 0 && filter) {
			filter(tp, count4, size);
		}
		target.set(heap.subarray(tp, tp + count * size));
		sbrk(tp - sbrk(0));
		if (res != 0) {
			throw new Error('Malformed buffer data: ' + res);
		}
	}

	var filters = {
		NONE: '',
		OCTAHEDRAL: 'meshopt_decodeFilterOct',
		QUATERNION: 'meshopt_decodeFilterQuat',
		EXPONENTIAL: 'meshopt_decodeFilterExp',
	};

	var decoders = {
		ATTRIBUTES: 'meshopt_decodeVertexBuffer',
		TRIANGLES: 'meshopt_decodeIndexBuffer',
		INDICES: 'meshopt_decodeIndexSequence',
	};

	var workers = [];
	var requestId = 0;

	function createWorker(url) {
		var worker = {
			object: new Worker(url),
			pending: 0,
			requests: {},
		};

		worker.object.onmessage = function (event) {
			var data = event.data;

			worker.pending -= data.count;
			worker.requests[data.id][data.action](data.value);
			delete worker.requests[data.id];
		};

		return worker;
	}

	function initWorkers(count) {
		var source =
			'self.ready = WebAssembly.instantiate(new Uint8Array([' +
			new Uint8Array(wasm) +
			']), {})' +
			'.then(function(result) { result.instance.exports.__wasm_call_ctors(); return result.instance; });' +
			'self.onmessage = ' +
			workerProcess.name +
			';' +
			decode.toString() +
			workerProcess.toString();

		var blob = new Blob([source], { type: 'text/javascript' });
		var url = URL.createObjectURL(blob);

		for (var i = workers.length; i < count; ++i) {
			workers[i] = createWorker(url);
		}

		for (var i = count; i < workers.length; ++i) {
			workers[i].object.postMessage({});
		}

		workers.length = count;

		URL.revokeObjectURL(url);
	}

	function decodeWorker(count, size, source, mode, filter) {
		var worker = workers[0];

		for (var i = 1; i < workers.length; ++i) {
			if (workers[i].pending < worker.pending) {
				worker = workers[i];
			}
		}

		return new Promise(function (resolve, reject) {
			var data = new Uint8Array(source);
			var id = ++requestId;

			worker.pending += count;
			worker.requests[id] = { resolve: resolve, reject: reject };
			worker.object.postMessage({ id: id, count: count, size: size, source: data, mode: mode, filter: filter }, [data.buffer]);
		});
	}

	function workerProcess(event) {
		var data = event.data;
		if (!data.id) {
			return self.close();
		}
		self.ready.then(function (instance) {
			try {
				var target = new Uint8Array(data.count * data.size);
				decode(instance, instance.exports[data.mode], target, data.count, data.size, data.source, instance.exports[data.filter]);
				self.postMessage({ id: data.id, count: data.count, action: 'resolve', value: target }, [target.buffer]);
			} catch (error) {
				self.postMessage({ id: data.id, count: data.count, action: 'reject', value: error });
			}
		});
	}

	return {
		ready: ready,
		supported: true,
		useWorkers: function (count) {
			initWorkers(count);
		},
		decodeVertexBuffer: function (target, count, size, source, filter) {
			decode(instance, instance.exports.meshopt_decodeVertexBuffer, target, count, size, source, instance.exports[filters[filter]]);
		},
		decodeIndexBuffer: function (target, count, size, source) {
			decode(instance, instance.exports.meshopt_decodeIndexBuffer, target, count, size, source);
		},
		decodeIndexSequence: function (target, count, size, source) {
			decode(instance, instance.exports.meshopt_decodeIndexSequence, target, count, size, source);
		},
		decodeGltfBuffer: function (target, count, size, source, mode, filter) {
			decode(instance, instance.exports[decoders[mode]], target, count, size, source, instance.exports[filters[filter]]);
		},
		decodeGltfBufferAsync: function (count, size, source, mode, filter) {
			if (workers.length > 0) {
				return decodeWorker(count, size, source, decoders[mode], filters[filter]);
			}

			return ready.then(function () {
				var target = new Uint8Array(count * size);
				decode(instance, instance.exports[decoders[mode]], target, count, size, source, instance.exports[filters[filter]]);
				return target;
			});
		},
	};
})();

export { MeshoptDecoder };
