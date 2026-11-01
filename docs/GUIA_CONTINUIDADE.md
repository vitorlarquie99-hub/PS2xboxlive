<!-- coderabbit-ai-generated:v1 -->

# Guia de continuidade — Shadow of the Colossus no Xbox

Atualizado em 1º de novembro de 2026. Este repositório contém código experimental e ferramentas de investigação. **Ainda não existe jogo jogável, pacote UWP ou execução validada no Xbox.** O teste mais recente é um executável de diagnóstico Linux que utiliza módulos IOP originais carregados manualmente.

## Ponto de partida

O alvo investigado é a versão PS2 `SCUS_974.72`, SHA-256 `d16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1`. O disco, os módulos, capturas de RAM e código de jogo recompilado são insumos externos e não são distribuídos neste repositório. Use uma cópia do jogo obtida legalmente. A identidade acima não garante compatibilidade com outras revisões.

O repositório originário informado pelo usuário é [PS2toXBOX99](https://github.com/vdsvvd9999/PS2toXBOX99). Este trabalho também se baseia no [PS2Recomp](https://github.com/ran-j/PS2Recomp). O ponto de partida desta continuidade foi o commit `e5636981a1c9de368f6b71e819c3dc188dfedbf4` deste repositório; consulte o histórico para o commit que contém este guia.

## Progresso feito

1. **Arquivos e reconstrução:** ferramentas auditam ELF, IOPRP e XFF e preparam a configuração de recompilação. KERNEL, MANAGER e GAMECORE foram reconstruídos a partir de capturas locais e executados em um diagnóstico combinado. A geração processou 9.024 funções, 26.624 locais de importação e registrou 670 avisos de fluxo de controle que precisam de investigação.
2. **Recompilador:** suporte a valores relocados em tempo de execução para LUI, ADDIU, LW, SW, J e JAL, incluindo campos, delay slot e retorno. `general.recompile_addresses` preserva funções convidadas selecionadas em vez de substituí-las automaticamente por stubs.
3. **IOP e arquivos:** memória compartilhada para módulos e heap, carregamento de IRX, chamadas de CD/DVD e I/O, além de captura de alvos ausentes. Um erro de arquivos não encontrados foi atribuído a links simbólicos ignorados pela ISO virtual; a instalação diagnóstica corrigida usa arquivos reais/hardlinks.
4. **Vídeo e sincronização:** eventos GS SIGNAL/FINISH são entregues ao INTC quando habilitados. O contador gráfico avançou, e o diagnóstico alcançou inicialização de estágio e verificação de memory card. A saída observada ainda é uma tela preta com indicador de carregamento.
5. **Controle:** conclusão/IRQ SIO2, callbacks VBlank, barramento e negociação de controle digital, analógico e pacote de pressão. O driver original executou a sequência de configuração e começou leituras periódicas; nas cinco capturas finais a porta 0 apresentou Pad2=1, sync=1 e status DBC=1. Pressão é aproximada a partir dos botões, e vibração física ainda não está implementada.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:9db8a129e342195ac2bb856599ee20aae14758e8db7a3f38c3fd7e8dfeb61a1c -->

6. **Ferramentas e testes:** nesta continuação, a suíte C++ passou 457 casos, incluindo a regressão da leitura CD para IOP. Os resultados anteriores de cinco suítes IOP e 18 casos Python continuam aplicáveis aos arquivos que não mudaram. Os testes são do ambiente Linux e não comprovam desempenho, jogabilidade ou compatibilidade do Xbox.

<!-- /coderabbit-ai-generated:v1 -->

Relatos detalhados: [reconstrução e limitações](reports/2026-11-01-boot-reconstruction.md), [SIO2 e Pad2](reports/2026-11-01-sio2-sequence.md), [investigação Pad2](reports/2026-11-01-pad2-investigation.md) e [guia específico do jogo](../shadow_of_the_colossus/README.md). Relatórios anteriores descrevem estados históricos e podem ter sido superados por relatórios posteriores.

## Como continuar

1. Clone o repositório público e selecione a branch ou commit desta entrega. Leia o [README](../README.md) e o [guia do jogo](../shadow_of_the_colossus/README.md). Os comandos abaixo requerem CMake 3.21+, compilador C++20 e Python 3.
2. Verifique o ELF de sua própria cópia e execute os testes das ferramentas:

   ```sh
   python3 shadow_of_the_colossus/inspect_elf.py /caminho/SCUS_974.72
   python3 -m unittest discover -s shadow_of_the_colossus/tests
   cmake -S ps2xIOP -B build/iop -DPS2X_IOP_BUILD_TESTS=ON
   cmake --build build/iop
   ctest --test-dir build/iop --output-on-failure
   ```

3. Use o guia específico do jogo para compilar `ps2_analyzer` e `ps2_recomp`, auditar os XFF, extrair símbolos e preparar a configuração. A reconstrução observada depende dos endereços de módulos capturados, do disco correspondente e das ferramentas Celosia na revisão `df6b12b69e8c8c1d0012f5ac5b834ddb818bb2c2`. Ela não é uma construção reproduzível completa apenas com este repositório: os runners instrumentados, capturas e dados combinados eram locais e exigem reconstrução ou restauração do backup de continuidade.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:101ebffb59b371ee62d9737dff900bf2d8936efca8d6014b697d4a56f687c2ac -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:8cfbe3d1337c4424bb3fb282dd0b035504c5bdc3dcae55e85a920b4c39c40d17 -->

4. A correção de `sceSifSetDmaIntr` permite que os botões cheguem ao jogo. No diagnóstico, Down seguido de X selecionou a opção 1 e concluiu o diálogo, embora o estado do diálogo permanecesse 2. A falha seguinte foi identificada e corrigida: `sceCdReadIOPm` escrevia na RAM EE em vez da IOP. Na execução corrigida, a conclusão continuou 1, o callback permaneceu válido e o contador GP avançou até 363 aos 210 segundos. A imagem ainda é incompleta; investigue renderização e avanço do fluxo antes de anunciar boot jogável. Consulte o [relatório do menu e leitura IOP](reports/2026-11-01-menu-investigation.md).

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->
5. Corrija renderização, carregamento de arquivos, áudio e inicialização IOP normal. Reavalie os 670 avisos de fluxo, relocação ao vivo e posições variáveis dos módulos. Implemente SIO2 DMA e vibração conforme necessário, e teste o caminho de pressão real quando houver suporte no host.
6. Só após um boot repetível no desktop, avalie compilador, gráficos, entrada, armazenamento, áudio, empacotamento e restrições UWP do Xbox Developer Mode. Valide no hardware, registre erros e medições de desempenho e faça testes de regressão em outras versões do jogo.

## Evidências e limites

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:554367313eb48c59c1c79f4c2a3412e9ae219fb3bf37ed4adb9205c5725a7974 -->

O diagnóstico corrigido mais recente foi encerrado deliberadamente pelo wrapper após aproximadamente 245 segundos. Usou Linux, Xvfb/raylib/OpenGL e injeção dos módulos FILEIO, LIBSD e SG2IOPM1; concluiu o diálogo do cartão e preservou o callback que antes era sobrescrito. Não provou inicialização IOP normal, menu legível ou jogabilidade. Dados do disco, dumps de RAM, executáveis gerados e logs locais permanecem fora do repositório público.

<!-- /coderabbit-ai-generated:v1 -->

Se o backup de diagnóstico tiver sido baixado junto desta entrega, ele contém ferramentas, rastros e logs de pesquisa, com caminhos absolutos do sandbox que exigem adaptação. Não contém disco, capturas de RAM nem build jogável; alguns arquivos são instrumentação de experimento, não ferramentas de produção. Confira licenças e conteúdo antes de redistribuí-lo. A reconstrução completa ainda precisa de dados externos e de etapas manuais.
