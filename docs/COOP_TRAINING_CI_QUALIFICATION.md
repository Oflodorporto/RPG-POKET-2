# Qualificação isolada do treino cooperativo

Estado em 14/10/2026: três execuções técnicas autorizadas; a flag0 compila, mas a flag1 ainda falha no link Xtensa. A preparação seguinte coleta diagnóstico textual, sem alterar produção ou flags. Cada novo envio depende do gate responsável. Não é autorização para publicar, atualizar ou instalar firmware.

## Fonte congelada e execução

- Base: `6a23dda54114db611b818b51e4b1b942019098fa`.
- Integração revisada de 69 arquivos: commit local `c405231a5dd55be1520b08269cac50e200388d21`; cópia reconstruída pelo conector `b117da8bd46eb1cb3fa4e051654dc9e32c6f126e`, com árvore idêntica `56a7bba85c83ba3584589502a81f3c69cde5dc01`. O gate usa o SHA reconstruído, disponível no histórico remoto, e confere também a árvore congelada.
- Branch exclusiva: `feat/coop-training-check`. Só um push nessa branch dispara o novo workflow; não há dispatch genérico, PR privilegiado ou gatilho de tag.
- As exceções posteriores revisadas estão fixadas por SHA-256 no gate: sincronização ESP32 por portMUX e extração do runtime para outra unidade de tradução. Renderer/arte e elegibilidade permanecem na TU original, evitando referências ODR a tabelas internas legadas. A etapa diagnóstica seguinte não muda produção.
- `contents: read`, checkout com `persist-credentials: false`; nenhum novo segredo/credencial. Workflows e ferramentas históricos de release/PvP permanecem inalterados. Compras em lote/package11 não entram nesta base.
- Dois jobs limitados: nativo focado (15 min); após sucesso, compilação das duas flags no mesmo runner descartável (45 min). Não executa o agregado completo de release.

## Provas pretendidas

1. Testes de configuração com mutações negativas e parser de evidência, sem fingir compilação.
2. Seis testes nativos otimizados, cinco ASan/UBSan, três regressões PvP; incluem preservação Save30/RNG, saída/reentrada, inadimplência ACK, turnos, ondas/chefe, toque e rádio simulado. LeakSanitizer segue desabilitado no wrapper; não há alegação de LSan.
3. Core `esp32:esp32@3.3.12`, FQBN exatamente igual ao histórico: ESP32-S3, 240 MHz, flash QIO 16 MB, `app3M_fat9M_16MB`, PSRAM OPI, CDC/hwcdc e `EraseFlash=none`.
4. Compilar o SHA do evento com `compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0` e depois `=1`, em diretórios distintos dentro de `RUNNER_TEMP`, fora do checkout. `--export-binaries=false`; nenhum upload para placa.
5. Conferir FQBN/define nas opções efetivas do compilador, memória reportada pelo Arduino, seções do ELF e símbolos reais. Flag0 deve excluir o runtime coop; flag1 deve conter os objetos limitados por static_assert. A imagem deve ter cabeçalho ESP32-S3, versão herdada, segmentos íntegros, checksum e SHA256 anexado válidos.
6. Guardar SHA/tree e inventário de fontes, versões do CLI/core, hashes de ELF/imagem/logs e orçamento. A pasta de evidência recusa binários, symlinks, subdiretórios e nomes de manifesto de release.

## Limites de orçamento

- Imagem de cada variante deve caber na partição de aplicação de 3.145.728 bytes.
- Cap de qualificação para incremento flag1–flag0: 262.144 bytes de imagem e 16.384 bytes de RAM estática. São limites conservadores para interromper crescimento inesperado, não medições já obtidas. Falha exige investigar e rever explicitamente o cap, não aumentá-lo automaticamente.
- Objetos ELF: Session ≤6.144, Input ≤1.024, Device ≤256; armazenamento+controle da fila ≤3.072 bytes.
- Arte 666 e orçamento explícito anterior de PSRAM 7.638.272 bytes permanecem herdados; não há segunda framebuffer nem nova alocação PSRAM explícita na integração.
- Métrica do linker e diferença de imagem não medem heap livre, stack, alocações do driver, FPS, latência, touch ou estabilidade RF. Esses pontos continuam no checklist de duas placas.

## Identidade e artefatos

`ReleaseVersion.h` continua byte a byte com a identidade herdada `2026.10.14-pvp77`, build `2026101404`. Não é uma nova versão distribuível. Cada prova ganha identidade separada `coop-training-qualification/<SHA>/flag-0|1` e marcação `EXPERIMENTAL_COMPILE_EVIDENCE_ONLY`.

O upload de CI contém **somente `.log`, `.txt` e `.json`** em diretórios explícitos, retidos por sete dias. Imagens `.bin`, ELF, pacote de artes, `manifest.json` e pacote OTA não são enviados. Assim não existe download de firmware oferecido para instalação neste fluxo. Não há chamada ao publisher, release table, tags ou alteração de `main`.

Mesmo com CI verde, liberação depende de revisão do SHA, qualificação física em duas placas e autorização própria. As duas capturas de lobby/combate entregues são do renderer C++ já testado; não são fotografias das placas.

Referências de formato: [opções de build do Arduino CLI](https://github.com/arduino/arduino-cli/blob/master/internal/arduino/builder/build_options_manager.go) e [formato de imagem ESP32-S3](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/firmware-image-format.html). O parser da imagem cobre a imagem simples sem assinatura usada neste fluxo; não substitui teste de boot/OTA ou secure boot.

## Histórico da preparação inicial

Antes do primeiro envio em 14/10/2026: 36 checks de configuração/workflows e 59 fixtures positivas/negativas do registrador passaram, inclusive revisão independente. O wrapper focado passou: 6 builds nativos otimizados, 5 ASan/UBSan e 3 regressões PvP. O parser também foi conferido contra imagens ESP32-S3 históricas existentes. Naquele momento ainda não havia build ESP32 novo; as três execuções posteriores e seu resultado atual estão registradas abaixo. A preparação diagnóstica atual já passa 40 checks de configuração e 68 fixtures do recorder normal, sem alegar que isso resolve o link.

Após autorização explícita do envio, o transporte Git não encontrou autenticação disponível. O conector autorizado reconstruiu os objetos preservando árvores e modos de arquivos; os SHAs retornados e o readback de cada ref são registrados antes de acompanhar CI.

## Falha de link e próxima observação

- CI1 `38619347753`: a suposição de atomic lock-free falhou no alvo; corrigida por portMUX revisado e testado.
- CI2 `38621515603`: compilação C++ passou, link flag1 falhou com 259 diagnósticos l32r.
- CI3 `38627070256`, fonte `ff1a0cb32265a77fb5035f09ea97b8f3d608e4b3`, árvore `05eef84eac0a2144f3a1c4253dfe353f6cddd0f3`: nativos aprovados; flag0 preserva 2.913.522 bytes de programa e 55.732 bytes RAM estática; flag1 ainda falha. São 169 referências no clone installUpdate e quatro no task, todas para `.literal._Z17updateInstallTaskPv`. A contagem não mede distância, tamanho da função ou margem de flash.
- Objetos/mapas das execuções anteriores não foram guardados. Portanto ainda não há medição da colocação final que justifique escolher outra correção.

A próxima preparação acrescenta `--verbose` ao CLI (somente logging) e um step separado de diagnóstico após a tentativa de compilação. O erro original permanece erro: sem `continue-on-error`, sem alteração de flags e sem substituir os gates de memória/identidade. O artifact separado aceita exatamente `link-diagnostics.txt` e `diagnostic-identity.json`; não envia objetos, ELF, imagem, artes nem arquivo instalável. O recorder normal e sua allowlist de nove arquivos não mudam.

Por flag, o diagnóstico pretende registrar comandos reais/response files permitidos, seções e símbolos dos objetos, metadata ABI, relocations/disassembly somente das seções afetadas e contexto do mapa existente. Arquivos ausentes, parciais ou truncados serão identificados como tais. Endereços finais VMA e distância instrução→literal precisam ser comparados entre as flags; offsets do objeto isoladamente não bastam. O [core3.3.12 já gera o mapa](https://raw.githubusercontent.com/espressif/arduino-esp32/3.3.12/platform.txt), sem nova opção de link.

Alternativas a comparar somente após essas medidas:

1. `-mtext-section-literals` restrito à TU afetada: mecanismo documentado para aproximar pools; muda organização e possível deduplicação do código, exigindo prova das opções por objeto e nova medição de imagem/RAM. Não está aplicado nem autorizado globalmente.
2. Extração mecânica do updater: conserva flags, mas envolve fronteiras de estado, callbacks e headers legados de uma área sensível. Não autoriza mudar lógica OTA; requer revisão própria e atenção ODR.
3. Renderer separado com bridge read-only de arte: conserva updater/flags, mas adiciona interfaces e requer comparação pixel a pixel, ausência de cópias/alocações e medição de desempenho. Incluir diretamente os headers de arte na segunda TU já foi rejeitado pela revisão ODR.

O [GCC14.2 explica pools próximos de funções e a diferença para auto-litpools](https://gcc.gnu.org/onlinedocs/gcc-14.2.0/gcc/Xtensa-Options.html). O [linker GNU Xtensa documenta interleaving, deduplicação de literais e relaxamento de longcalls](https://sourceware.org/binutils/docs/ld/Xtensa.html). Isso fundamenta hipóteses, não comprova a causa desta falha sem o mapa.

Mesmo resolvido o link nesta base anterior à loja, a preparação do pacote coop deve integrar a shop publicada e repetir a qualificação do SHA combinado antes de qualquer proposta de liberação.
