<!-- coderabbit-ai-generated:v1 -->

# Investigação da imagem do menu

A execução Linux diagnóstica usa o ELF reconstruído e a injeção manual de FILEIO, LIBSD e SG2IOPM1 descritos no [relatório anterior](2026-11-01-menu-investigation.md). Não é um boot normal nem uma execução no Xbox.

## O que foi observado

Na captura com fragmentos de menu, os dois circuitos de apresentação usam `DISPFB=0x9000`, com `PMODE=0x8023` e `SMODE2=1`; o quadro apresentado tem 512 × 448 pixels. Uma leitura independente do framebuffer CT24, antes do processamento de campos, reproduziu os fragmentos. O defeito já está presente na VRAM, embora isso não exclua outros problemas na apresentação.

O rastreamento de primitivas observou triângulos texturizados com PSMT4, TBP 10880 e CBP 16188. A VRAM salva imediatamente antes do primeiro desenho com essa textura contém padrões em faixas na região lida como PSMT4. A mesma região é usada por um framebuffer temporário em FBP 336; a inspeção encontrou nela os bytes de uma cor desse buffer. Isso aponta para conteúdo inadequado no momento da leitura, mas não estabelece se o erro é carregamento, ordem de operações ou endereço.

Na execução instrumentada até o primeiro uso da textura, <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:8181ac2c651950f8fdc31ba4cb049ca1ddbd890ffa33147c28f957d54247ac70 -->

nenhum dos 126 comandos de transferência

<!-- /coderabbit-ai-generated:v1 --> recebidos pelo backend teve destino direto TBP 10880. O jogo chamou `texTransRequest` para sprites e executou `texTransResolve`; descritores NTO2 e dados não zerados associados aos pedidos aparecem na RAM. Essas observações não confirmam a integridade dos dados nem mostram em que ponto uma transferência esperada deixou de ser produzida ou entregue. Em particular, somente os comandos que chegaram a `BeginTransfer` foram contados.

## Reprodução e próxima investigação

Os instrumentos temporários, capturas e logs permanecem em `inputs/sotc/diagnostics/2026-11-01-image` e `inputs/sotc/analysis` no workspace, fora do repositório por conterem dados do jogo. Seus caminhos dependem dos builds de diagnóstico em `/tmp`; não são uma distribuição executável. O registro de validação do workspace identifica os scripts, hashes e resultados.

O próximo passo é rastrear a construção e o envio dos comandos gerados por `texTransResolve` para os pedidos dos sprites, incluindo o caminho VIF/GIF e a integridade dos recursos carregados. Em seguida, reproduzir qualquer causa identificada com teste isolado e comparar novamente uma captura do menu. Nenhuma correção de renderização foi aplicada nesta investigação, e a imagem continua ilegível.
