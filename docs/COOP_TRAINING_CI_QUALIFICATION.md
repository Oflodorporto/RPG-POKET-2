# Qualificação isolada do treino cooperativo

Estado em 14/10/2026: quatro execuções técnicas autorizadas; a flag0 compila, mas a flag1 ainda falha no link Xtensa. A CI4 coletou o mapa e demonstrou o alcance excedido. A preparação seguinte extrai mecanicamente o instalador, sem alterar decisões OTA ou flags. Cada novo envio depende do gate responsável. Esta branch técnica não publica, atualiza nem oferece firmware para instalar.

## Fonte congelada e execução

- Base: `6a23dda54114db611b818b51e4b1b942019098fa`.
- Integração revisada de 69 arquivos: commit local `c405231a5dd55be1520b08269cac50e200388d21`; cópia reconstruída pelo conector `b117da8bd46eb1cb3fa4e051654dc9e32c6f126e`, com árvore idêntica `56a7bba85c83ba3584589502a81f3c69cde5dc01`. O gate usa o SHA reconstruído, disponível no histórico remoto, e confere também a árvore congelada.
- Branch exclusiva: `feat/coop-training-check`. Só um push nessa branch dispara o novo workflow; não há dispatch genérico, PR privilegiado ou gatilho de tag.
- As exceções posteriores revisadas estão fixadas por SHA-256 no gate: sincronização ESP32 por portMUX, extração do runtime e extração mecânica do instalador para unidades de tradução próprias. Renderer/arte e elegibilidade permanecem na TU original, evitando referências ODR a tabelas internas legadas. O instalador não inclui Save, Menu, View ou catálogo de arte.
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

Antes do primeiro envio em 14/10/2026: 36 checks de configuração/workflows e 59 fixtures positivas/negativas do registrador passaram, inclusive revisão independente. O wrapper focado passou: 6 builds nativos otimizados, 5 ASan/UBSan e 3 regressões PvP. O parser também foi conferido contra imagens ESP32-S3 históricas existentes. Naquele momento ainda não havia build ESP32 novo; as quatro execuções posteriores e seu resultado atual estão registradas abaixo. Na preparação da CI4 passaram 40 checks de configuração e 68 fixtures do recorder normal, sem alegar que isso resolve o link.

Após autorização explícita do envio, o transporte Git não encontrou autenticação disponível. O conector autorizado reconstruiu os objetos preservando árvores e modos de arquivos; os SHAs retornados e o readback de cada ref são registrados antes de acompanhar CI.

## Falha de link e próxima observação

- CI1 `38619347753`: a suposição de atomic lock-free falhou no alvo; corrigida por portMUX revisado e testado.
- CI2 `38621515603`: compilação C++ passou, link flag1 falhou com 259 diagnósticos l32r.
- CI3 `38627070256`, fonte `ff1a0cb32265a77fb5035f09ea97b8f3d608e4b3`, árvore `05eef84eac0a2144f3a1c4253dfe353f6cddd0f3`: nativos aprovados; flag0 preserva 2.913.522 bytes de programa e 55.732 bytes RAM estática; flag1 ainda falha. São 169 referências no clone installUpdate e quatro no task, todas para `.literal._Z17updateInstallTaskPv`. A contagem não mede distância, tamanho da função ou margem de flash.
- Até a CI3, objetos/mapas não foram guardados; naquele ponto não havia medição da colocação final que justificasse escolher outra correção. A CI4 abaixo preencheu essa lacuna.

A CI4 acrescentou `--verbose` ao CLI (somente logging) e um step separado de diagnóstico após a tentativa de compilação. O erro original permanece erro: sem `continue-on-error`, sem alteração de flags e sem substituir os gates de memória/identidade. O artifact separado aceita exatamente `link-diagnostics.txt` e `diagnostic-identity.json`; não envia objetos, ELF, imagem, artes nem arquivo instalável. O recorder normal e sua allowlist de nove arquivos não mudam.

Por flag, o diagnóstico registra comandos reais/response files permitidos, seções e símbolos dos objetos, metadata ABI, relocations/disassembly somente das seções afetadas e contexto do mapa existente. Arquivos ausentes, parciais ou truncados serão identificados como tais. Endereços finais VMA e distância instrução→literal precisam ser comparados entre as flags; offsets do objeto isoladamente não bastam. O [core3.3.12 já gera o mapa](https://raw.githubusercontent.com/espressif/arduino-esp32/3.3.12/platform.txt), sem nova opção de link.

Alternativas consideradas antes da escolha registrada abaixo:

1. `-mtext-section-literals` restrito à TU afetada: mecanismo documentado para aproximar pools; muda organização e possível deduplicação do código, exigindo prova das opções por objeto e nova medição de imagem/RAM. Não está aplicado nem autorizado globalmente.
2. Extração mecânica do updater: conserva flags, mas envolve fronteiras de estado, callbacks e headers legados de uma área sensível. Não autoriza mudar lógica OTA; requer revisão própria e atenção ODR.
3. Renderer separado com bridge read-only de arte: conserva updater/flags, mas adiciona interfaces e requer comparação pixel a pixel, ausência de cópias/alocações e medição de desempenho. Incluir diretamente os headers de arte na segunda TU já foi rejeitado pela revisão ODR.

O [GCC14.2 explica pools próximos de funções e a diferença para auto-litpools](https://gcc.gnu.org/onlinedocs/gcc-14.2.0/gcc/Xtensa-Options.html). O [linker GNU Xtensa documenta interleaving, deduplicação de literais e relaxamento de longcalls](https://sourceware.org/binutils/docs/ld/Xtensa.html). Isso fundamenta hipóteses, não comprova a causa desta falha sem o mapa.

Mesmo resolvido o link nesta base anterior à loja, a preparação do pacote coop deve integrar a shop publicada e repetir a qualificação do SHA combinado antes de qualquer proposta de liberação.

## CI4: causa medida e extração mínima seguinte

A execução `38630032831`, fonte `7c9e0db7276558772d2cd6df556590ca6b9e3921`, árvore `ae5de1b90ba23f13b192bbc1a0c22a152583336d`, preservou a falha original e coletou os dois textos previstos. O ZIP tem SHA-256 `c33eac715f9a3fe4354d2cd463ae9197ab9857578c5f31b06294068eeffb7341`; o relatório, `9ba0fa150fc3ea925e251cae280cd89ce7e22e0696a85885bb8fcdf7977a00a9`. Identidade/árvore limpas, sem truncamento ou dados faltantes. Nativos passaram; flag0 continuou em 2.913.522 bytes de programa e 55.732 bytes RAM.

Na flag1, o pool está em `0x42013b34` e o clone de installUpdate em `0x42053640`. O primeiro erro, instrução `+0x505` e literal relaxado `+0x8`, tem base PC alinhada `0x42053b48` e alvo `0x42013b3c`: distância de 262.156 bytes, 12 além do alcance de 262.144. Todos os 173 erros excedem o alcance por 12 a 2.336 bytes. A separação cresceu 10.904 bytes desde flag0; o corpo continua com 3.725 bytes nas duas flags. O pool continua com 980 bytes antes e 504 depois da relaxação. Portanto o dado aponta colocação das seções, não função gigante, mudança ABI ou esgotamento de flash/RAM. Os cálculos usam addends dos erros após relaxação, não offsets provisórios do objeto.

A correção local seguinte move apenas installUpdate para UpdateInstall.cpp. UpdateInstallBridge.h declara nove helpers e nove estados compartilhados, mais os dois tipos já existentes. Os estados passam a ter definição forte única no instalador; helpers têm definição forte única no sketch. O task e sua política Wi-Fi permanecem no sketch. O verificador recompõe o UpdateDevice.h original byte a byte, revertendo somente os movimentos/linkage declarados, e exige o SHA-256 original `1ef88bec8ffeb9315851e4a41f722310b423fb36247e6972d6045eaaa599b9ed`.

O runner isolado compila a TU de produção separadamente e a liga a seis harnesses, incluindo os cinco testes OTA atuais, com builds otimizados e ASan/UBSan. Inspeciona símbolos/closure e preserva logs textuais top-level no artifact nativo existente. Os dois entrypoints nativos existentes recebem somente o novo source nos casos afetados e macro SHA consistente entre TUs. Nenhum caso ou sanitizer é removido. Os qualificadores históricos e seus gates permanecem congelados; adaptar staging/comandos do futuro pacote12 é uma etapa própria, sem alterar contratos antigos para fazê-los passar.

O recorder de alvo mantém os tetos e passa a exigir exatamente uma definição forte do instalador e de cada estado, no novo objeto e ausente dos demais. A coleta textual inclui o terceiro objeto. A flag0 histórica permanece como comparação numérica explícita: mudar linkage pode mudar seu tamanho. O build ESP32 das duas flags, os mapas e os orçamentos finais ainda precisam comprovar a hipótese de correção. Não há resultado de CI5 nem qualificação física presumidos nesta preparação.

`-mtext-section-literals` continua sendo mecanismo relevante, mas o [GCC14.2 não implementa pragma target para Xtensa](https://gcc.gnu.org/onlinedocs/gcc-14.2.0/gcc/Function-Specific-Option-Pragmas.html). Não foi usado pragma não suportado nem flag global. Mover o renderer exigiria nova bridge de arte/canvas e apenas recuperaria parte da folga da TU original.

Verificação local desta extração: seis harnesses otimizados passaram, incluindo 27 casos de recovery, 16 readback, 48 short-write e 11 old/new-pack. A prova de equivalência, 43 checks de configuração e 109 fixtures do recorder passaram. O runtime ASan/UBSan/LSan local **não está qualificado**: LeakSanitizer abortou por ptrace e uma tentativa de lançamento fora desse sandbox falhou antes de executar. Nenhum detector foi desligado. A CI seguinte exige modo both, LeakSanitizer ativo e sucesso dos seis harnesses sanitizados antes de iniciar ESP32.
