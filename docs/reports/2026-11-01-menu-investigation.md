<!-- coderabbit-ai-generated:v1 -->

# Investigação da tela de verificação do cartão

O ELF `SCUS_974.72` fornecido corresponde ao SHA-256 indicado no guia. KERNEL, MANAGER e GAMECORE foram reconstruídos a partir de novas capturas no Linux; o ELF combinado gerado tem SHA-256 `56c69b75d4a8b780832a3e4aa3bb5fb2381fd87ad1bce71db0dbd5d7bf82c23b`. Os arquivos do jogo e as capturas permanecem fora do repositório.

## Estado 2

Na captura de entrada do GAMECORE, a tabela em `0x014684C0` direciona o estado 2 de `bootmccheck` ao endereço `0x0136F9D0`. A análise das instruções mostra que esse caminho configura o menu e verifica o bit 0 de um campo em `GetSpriteMenuClass(...) + 0x10`. Quando o bit está ativo, chama `SpriteMenuGetSelectMenuIdBySpriteMenuClass`. Isso aponta para uma etapa de seleção de menu; não identifica, por si só, por que o estado permaneceu 2 na execução histórica ou se uma operação de cartão ficou pendente.

A captura inicial de GAMECORE e o código desmontado refletem esta disposição específica dos módulos. Endereços podem mudar em outras execuções ou revisões.

## Nova execução

Um runner Linux instrumentado, com FILEIO, LIBSD e SG2IOPM1 carregados manualmente, foi interrompido por timeout após 180 segundos. Nas capturas aos 4, 30, 60, 90 e 110 segundos após a entrada do GAMECORE, o contador GP foi 0, 0, 0, 6 e 12; o estado da verificação ficou 0. A porta 0 do controle estava sincronizada a partir da captura de 30 segundos. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:94a402875eee9373f70feb7180e4ecc3f173da54a39236675c0bc0e20335137b -->

Essa execução mais lenta não chegou ao estado 2 e não comprova menu legível ou jogabilidade.

Uma segunda execução com a instrumentação anterior e carregamento inicial dos módulos IOP terminou após 300 segundos por timeout. Nas capturas aos 120, 150, 180, 210 e 240 segundos após a entrada do GAMECORE, o contador GP foi 16, 24, 33, 41 e 50, com estado da verificação e conclusão em 0. O rastreamento incluía o ponto de leitura da solicitação em `mcManager` e não registrou uma solicitação. Esse resultado não demonstra que o cartão seja a causa da espera. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:f52b2fb94a317055375a9b7f36adabb8b99d5d9ad7ff4bb569e2819c70cbea82 -->

A compilação usada nessas duas execuções utilizou as opções padrão sem otimização.

Com um novo build `-Og -g1`, o diagnóstico terminou após 160 segundos por timeout. As capturas aos 4, 30, 60, 90 e 110 segundos após a entrada do GAMECORE mostraram contador GP 0, 20, 43, 97 e 134 e estado 0, 0, 0, 0 e 2, com conclusão 0. O controle da porta 0 estava sincronizado. A instrumentação observou solicitações do tipo 1 em `mcManager`; a tabela de despacho desse módulo leva esse tipo a `iosMcGetInfo`. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:10a51a248dc2df9af92350471905ff5e58cda8fd502807e3146f895a7f4a107c -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:903ccf4437543725a38dfb8dc34f79fe631e2829933a03f361be5378d0c278db -->

Numa segunda execução otimizada, a instrumentação registrou 151 retornos com valor 1 em `iosMcGetInfo`. A tabela de mapeamento em `0x001E0C98` na captura associa esse valor ao código `0x13`, cuja string na própria imagem é `SCE_ENODEV`. O código de `iosMcGetInfo` aplica essa tabela ao retorno de `sceMc2GetInfo`; esse caminho indica cartão ausente. A ausência de cartão não explica, por si só, por que a seleção do menu não foi concluída.

<!-- /coderabbit-ai-generated:v1 --> Eventos X e Enter foram entregues à janela após a captura de 120 segundos. Na captura de 145 segundos, o estado permanecia 2, a conclusão 0 e o bit observado do menu em zero. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:c50d8f6325339363e9d71ee96f22e65eb4e0fbcc2f443660c8ca2c45010d95ad -->

Uma terceira execução rastreou os pacotes SIO2: ao enviar X e Enter na janela, o comando `0x42` da porta 0 apresentou respectivamente os botões ativos `0xBFFF` e `0xFFF7`. Na captura posterior, aos 145 segundos, o estado continuava 2, a conclusão 0 e o sinal do menu 0; o contador GP chegou a 214. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:ba4cb428cb5d707f67175c8973bce70735a0a6b5b9226865da5d69723cce7807 -->

Os botões chegaram ao emulador SIO2, mas o rastreamento da leitura `scePad2Read` continuou mostrando bytes `00 00` durante os pressionamentos. As capturas também mostraram os contadores dos buffers de leitura em zero. O driver DBCMAN chamava `sceSifSetDmaIntr` (importação `sifman` ordinal 32), que o emulador respondia com zero sem realizar a transferência.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:d02dac4e6d23ff303bc2c8786e7069286d85be2b58ad7cc80b8ff65fb064651c -->

O suporte a essa chamada reutiliza a transferência SIF já existente e agenda sua conclusão para o callback do IOP quando as interrupções estão habilitadas. Na execução otimizada com essa correção, X produziu `FF BF` na leitura do jogo e o sinal do menu mudou de 0 para 1; Enter produziu `F7 FF`. Na captura posterior, o estado ainda era 2 e a conclusão 0. O teste estabelece entrega de entrada à leitura do jogo e ativação do sinal de seleção, sem demonstrar que a tela foi concluída, que o jogo é jogável ou que funciona no Xbox. A transferência e seu callback são simulados de forma simplificada, não com temporização exata de hardware.

## Conclusão do diálogo e falha seguinte

Uma nova execução manteve a correção de DMA e enviou **Down e X**, nessa ordem, após a captura de 120 segundos. O rastreamento do retorno `SpriteMenuGetSelectMenuIdBySpriteMenuClass` registrou seleção 1. A flag de conclusão em `0x011E0158` passou de 0 para 1. Isso confirma que o diálogo pode terminar pelo caminho escolhido; o estado em `0x014664BC` permanece 2 e não deve ser usado sozinho como indicador de bloqueio.

Aos 90/120/150/180/210 segundos após GAMECORE, o contador GP foi 126/185/220/220/220 e a flag de conclusão foi 0/0/1/1/1. Em seguida, o thread de processamento gráfico tentou chamar `0xFF1F2EE0` a partir de `iFinish`, em `0x001C38B4`. Antes da seleção, o callback em `0x001E1168` era `0x01345F60`; depois ele e parte do código KERNEL estavam sobrescritos. Os bytes encontrados em `0x001C38A0` correspondem a bytes em `NICO.DAT`, offset `0x8AF58EA0`. Essa correspondência identifica a origem dos dados, sem provar qual operação escolheu o destino incorreto.

A captura de tela posterior ainda mostra fundo preto e fragmentos de texto/faixas, sem menu legível ou cenário. O contador GP parou de avançar após a falha. Portanto, concluir o diálogo não conclui o boot jogável.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:d6105113d6c57fe1e4966576820eb265e94394cc67daa1073b32515fab50aac0 -->

Um teste separado, sem a injeção manual dos módulos, terminou por timeout em 35 segundos sem observar entrada em GAMECORE. O último despacho registrado repetia `0x0010A950`. Esse teste limitado não comprova uma espera permanente, mas também não valida a inicialização IOP normal.

## Causa identificada e correção

Um watchpoint de hardware no callback capturou a primeira escrita incorreta na cadeia `readHostRange` → `readCdSectors` → `sceCdRead` → `sceCdReadIOPm`, chamada por `iosIopRead`. A implementação de `sceCdReadIOPm` delegava à leitura destinada ao EE. Como o destino dessa API é um endereço IOP, ela escrevia os setores no espaço de memória errado, incluindo o KERNEL.

A correção lê os setores para um buffer temporário, valida integralmente o intervalo IOP e grava com `writeIopMemory`. Ela não usa o endereço EE numericamente igual nem tenta disposições alternativas de argumentos. O caminho convencional `sceCdRead` continua sendo responsável pelas leituras destinadas ao EE. Trata-se de reparo da separação já documentada entre as duas memórias, e a operação permanece síncrona no modelo simplificado do runtime.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:7501fd7162365f18c962b4cae14a71e688c6d896b9c00a6d88db572c66916655 -->

A suíte C++ passou **457/457** casos. O novo teste usa uma imagem sintética de dois setores e verifica o setor escolhido, guardas IOP, preservação de toda a RAM EE e rejeição de destinos/tamanhos inválidos, runtime ausente e arquivo ausente. As cinco suítes IOP do registro anterior continuam aplicáveis; não foram repetidas nesta etapa.

## Execução após a correção da leitura IOP

O mesmo diagnóstico, com Down/X após 120 segundos e sem watchpoint, confirmou a conclusão do diálogo. Aos 90/120/150/180/210 segundos, os contadores GP foram **127/188/250/307/363** e a conclusão foi **0/0/1/1/1**. O callback permaneceu `0x01345F60` em todas as cinco capturas, sem repetir o salto inválido anterior durante a janela observada. O processo foi encerrado deliberadamente pelo wrapper após aproximadamente 245 segundos; isso não é uma saída normal do jogo.

A imagem capturada após a conclusão ainda contém fundo preto e fragmentos de texto/faixas. Enter e X adicionais foram entregues perto do final, sem uma captura posterior que comprove mudança de cena. Não há menu legível, cenário ou jogabilidade comprovados. O teste continua utilizando injeção de FILEIO/LIBSD/SG2IOPM1 e código gerado a partir das capturas locais. A correção elimina a sobrescrita reproduzida, mas não valida um boot normal ou uma versão de console.

## Pendências de entrega

Ainda faltam renderização e fluxo jogável verificáveis, validação de áudio e armazenamento, inicialização IOP sem injeção, portabilidade e pacote UWP, desempenho e testes no Xbox. O checkout não contém projeto/manifesto UWP, e o sandbox Linux não tem MSVC, MSBuild, MakeAppx ou console conectado. As correções deste relatório são de runtime; não constituem aplicativo Xbox pronto.


<!-- /coderabbit-ai-generated:v1 -->


<!-- /coderabbit-ai-generated:v1 -->


<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->
