# RPG POKET 2.0 — atualização pelo Wi-Fi

Entrega **2026.10.06-ota1**, 06/10/2026. Placa Waveshare SKU 29667. O usuário confirmou que guilda1 funciona na placa e que as animações deixaram de travar. Esta entrega ainda precisa de teste físico.

## O que mudou

- Personalização usa a pose parada, sem alternar com ataques ou deslocar o personagem durante a escolha das cores.
- Fúria do bárbaro: efeito exclusivo de 960 ms, com arco largo, rastros vermelhos/laranja e impacto crescente. Ataque comum continua com seu efeito anterior; regras de dano/mana não mudam.
- Símbolo verde de Wi-Fi no canto superior direito enquanto a conexão está ativa.
- Configurações → Atualização: verificar versão, mostrar a versão encontrada, confirmar ou cancelar, baixar, conferir, instalar, limpar temporários e reiniciar.
- A opção Recarregar artes foi substituída por Atualização. As artes são carregadas no início; a mesma versão publicada pode reparar artes ausentes/inválidas pela internet.
- Código, testes, artes compactadas e rotina de publicação estão em https://github.com/Oflodorporto/RPG-POKET-2.

## Primeira instalação — uma vez pelo cabo

Abra `firmware/RPG_POKET_2/RPG_POKET_2.ino` no Arduino IDE e grave manualmente. A versão anterior ainda não tem instalador pela internet. Mantenha ESP32S3 Dev Module, PSRAM OPI, flash 16 MB, core Espressif 3.3.12, USB CDC ativado, partição `app3M_fat9M_16MB` e Erase Flash desativado. Não mude partições nem apague saves.

Se o cartão ainda tem as artes da versão antiga, esta entrega inicialmente usa a arte simplificada, porque o pacote ganhou um fundo para a atualização. Assim que a Release estiver publicada, conecte ao Wi-Fi e use Atualização para baixar as artes correspondentes, sem remover o cartão. Como a versão de firmware já é a mesma, a placa oferece reparar apenas as artes e reiniciar.

Alternativa opcional: o ZIP do cartão tem a pasta RPGPOKET pronta. A preferência atual é experimentar o download na placa para não ficar removendo o cartão.

## Como funciona

A placa consulta `https://github.com/Oflodorporto/RPG-POKET-2/releases/latest/download/manifest.json`. O manifesto schema 2 informa versão, número crescente do build, placa, partição, formato dos saves, URLs, tamanhos, SHA-256 e CRC/quantidade das artes. Um manifesto incompatível, versão anterior, endereço fora deste repositório ou arquivo maior que a partição é recusado.

HTTPS usa o conjunto de certificados do core; não usa conexão insegura. A hora é sincronizada antes da consulta para validar certificados. Não há token do GitHub ou senha de Wi-Fi no código publicado. As senhas ficam apenas na configuração local da placa.

O download só começa depois de **Instalar**. Firmware e artes são baixados para `firmware.part` e `artes.part`, dentro de `/RPGPOKET`. Os arquivos são fechados e relidos para conferir SHA-256; o pacote de arte também passa por cabeçalho, tamanho, quantidade e CRC.

As novas artes ficam em `/RPGPOKET/artes_XXXXXXXX.pak`, onde XXXXXXXX é seu CRC. O arquivo da versão atual permanece intacto. O novo programa é escrito apenas na partição de aplicação livre e conferido pelo instalador do ESP32. A seleção do próximo boot acontece por último, após a validação dos dois arquivos. Os `.part` são removidos e o aparelho reinicia. Na inicialização seguinte, só depois de validar as artes da versão que está executando, são removidos os pacotes de arte antigos com o padrão reservado ao jogo. Outros arquivos do cartão não entram nessa limpeza; saves ficam em NVS.

Rede roda em tarefa separada. Cartão e LCD compartilham um bloqueio somente durante os trechos de transferência SPI; o toque continua por I2C. Os objetos HTTPS são destruídos antes de encerrar a tarefa, liberando a memória a cada consulta. Durante o jogo não há novas leituras periódicas de cartão.

**Limites reais:** SHA-256 é conferência de integridade, não assinatura própria do autor. Não há promessa de rollback automático caso um firmware novo tenha um defeito de inicialização. FAT32 não garante imunidade a cortes de energia. Use alimentação estável e mantenha o cartão inserido durante a atualização. As garantias de fluxo foram testadas no computador; HTTPS, montagem do cartão, boot OTA e comportamento com alimentação real ainda exigem teste na placa. Nenhuma atualização foi acionada automaticamente pelo assistente.

## Publicação de próximas versões

1. Aumentar `FW_BUILD` e mudar `FW_VERSION` em ReleaseVersion.h; não reutilizar a versão de uma Release.
2. Manter firmware, AssetCatalog.h e artes.pak compatíveis; o gerador de manifesto recusa diferenças.
3. Passar os testes e a compilação, e enviar ao GitHub.
4. O envio de firmware/artes a main inicia a rotina de publicação. Também é possível usar Actions → Publicar versão → Run workflow com a mesma versão do código.
5. O GitHub gera uma Release com firmware.bin, artes.pak e manifest.json. A placa encontra a última Release quando o usuário toca em Verificar versão.

O pacote de artes no repositório fica compactado como artes.pak.gz; a rotina descompacta antes de publicar artes.pak. Isso reduz o tamanho dos downloads do código. A atualização na placa recebe artes.pak diretamente.

## Validação no computador

Compilação final: **1.432.770 bytes (45%)**, globais **52.476 bytes (16%)**. As fontes foram comparadas com a cópia efetivamente compilada. Testes de regras, saves, migrações, slots, guilda, clube, teclado, cartão, renderer e controles passaram. Renderer verificou 50 fundos distintos, limites de texto, 1008 quadros de viagem, pose estável e indicador verde.

O teste usa a função real de instalação e injeta falhas em montagem, espaço, download, hash, publicação da arte, início/leitura/escrita/finalização OTA e seleção de boot. Conferiu preservação da arte antiga e dos arquivos alheios, limpeza e reparo só das artes. Outro teste usa a tarefa real de consulta e verifica liberação de HTTPS nas saídas de sucesso e falha. São substitutos de rede/cartão/flash no computador, não testes em hardware.

As 2012 comparações SHA-256 da Heltec original e congelada permanecem iguais. Não houve gravação automática, `.rpg`, mudança de partições ou alteração de save real.

## Cinco próximos testes pelo usuário

1. Gravar este sketch uma vez pelo cabo, sem apagar a flash, e conferir os três personagens existentes.
2. Conferir pose parada e todas as cores na criação; usar Fúria do bárbaro e comparar com Ataque.
3. Conectar/desligar Wi-Fi e conferir o símbolo verde.
4. Abrir Atualização: conferir versão, cancelar sem download e reparar as artes da mesma versão. Verificar reinício, imagens e saves.
5. Com uma próxima Release numerada publicada, testar a atualização completa do firmware e das artes. Primeiro testar perda de conexão durante download; testes de corte de alimentação/rollback ficam pendentes até haver preparo de recuperação apropriado.

Registros: `validacao/ota1-*.log`, `ota1-fontes-compiladas.json`, `ota1-publicacao.json`, `ota1-heltec-preservada.json`, `previa-ota1.png`.

## Publicação concluída

Release **v2026.10.06-ota1** publicada. A rotina do GitHub concluiu testes, compilação e publicação com sucesso. Os três arquivos da Release foram baixados por HTTPS e conferidos contra o manifesto. Firmware da Release: 1432864 bytes; SHA-256 `4f2b2e59faa613bf3cefa25a7ac1d0151ee433b94ca38664c99986d0e05939aa`. Artes: 5821264 bytes; SHA-256 `c0fcf4eed046c25718c9e3678ba60304635f3d0e3776897eb42b7465c900ebc0`.

A placa já pode consultar esta Release depois da primeira gravação manual do atualizador. Teste real permanece pendente.
