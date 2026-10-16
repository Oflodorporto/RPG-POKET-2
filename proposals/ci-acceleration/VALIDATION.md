# Validação local antes do envio

Data: 2026-10-15. Execução isolada; nenhum commit, push, workflow remoto ou
publisher novo foi executado nesta validação.

- 174 testes de infraestrutura do benchmark: PASS em Python normal e `-O`.
- 30 testes da proposta de promoção exata: PASS em Python normal e `-O`.
- Teste original de configuração: 1.780 probes de mutação/histórico e 562 de
  seleção de artifacts: PASS. São probes offline com fixtures; não são o
  agregado real, builds ESP32 ou OTA de produto.
- Teste original de escopo: 161 probes negativos offline: PASS.
- Reprodução offline do lock oficial: PASS, 27 distribuições,
  SHA-256 `d03e64207ca2c45c81f01ecf64ff8faf230f6552e12c7d96d552d68491c2fa78`.
- YAML e sintaxe de todos os blocos Bash: cobertos nos testes do workflow.
- Diff fora de `proposals/` contra a árvore fonte `5ad62cddfe052bea09bf99c29989731722088981`: vazio.
- `git diff --check`: PASS.

Os testes do coletor reconstruíram o inventário real da fonte congelada e
validaram seu digest sem alterar pins de produção. Os demais cenários de
artefatos/API são fixtures claramente sintéticos. Não há autenticação implícita
em arquivos JSON fornecidos localmente. Rejeições abrangem identidade, tentativa,
repo/fork, cache incorreto, ZIP/digest/tamanho, entradas perigosas, evidências,
157 checkpoints, resume, LSan e medições incompletas.

A execução real ainda precisa demonstrar download das distribuições oficiais,
instalação limpa e duas qualificações completas no GitHub Actions. Nenhuma
estimativa ou teste unitário foi contado como ganho de tempo. O novo setup fixo
estabelece sua própria baseline cold, separada das observações históricas.

Revisão independente integrada concluída: sem bloqueador técnico conhecido
para apresentar o diff e executar o benchmark isolado após o gate de envio.
O revisor repetiu 174 + 30 testes em ambos os modos e conferiu os contratos
CLI/JSON/staging no código oficial da CLI fixada. As duas observações levantadas
foram resolvidas: logs/comandos finais obrigatórios no coletor e guarda precoce
do compilador antes dos downloads. Instalação e CI reais permanecem pendentes.

Este parecer técnico não é aprovação de publisher. Ativação em main segue
fora deste experimento e exige apresentação e aprovação específica.
