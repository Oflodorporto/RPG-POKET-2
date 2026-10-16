# Validação local da adaptação coopdiag77

Data: 15/10/2026. Fonte: 6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3; árvore conferida 5ad62cddfe052bea09bf99c29989731722088981. A cópia local preserva os 1.178 arquivos da fonte, mas seu HEAD ainda é o ancestral c8738eb; não é falsamente rotulada como checkout do commit remoto.

- 30 métodos de teste adversarial passaram em Python 3.12.14 normal.
- Os mesmos 30 passaram com Python -O.
- O fixture lê o plano real de tools/test_coopdiag77_local.py: 156 tarefas mais preparação, com comandos preservados. Logs e payloads dos testes são sintéticos.
- Não houve execução das 157 tarefas, recompilação ESP32, novo teste OTA, alteração de workflow ativo, publicação ou benchmark remoto.
- O verificador sempre retorna publish_authorized=false, authenticated_provenance=false, lease_consumed=false e real_byte_ota_executed_by_this_tool=false.
- A revisão independente de 14/10 valia para o protótipo anterior de 143 grupos. A adaptação atual precisa de nova revisão.

Os testes preservam rejeições por procedência/IDs/branch/tentativa, SHA/árvore/inventário, plan/commands/checkpoints/logs, sanitizers enfraquecidos, digests/tamanhos, validade/expiração, main divergente, tag existente, replay no snapshot, bytes/manifesto, ZIP malformado ou com traversal/symlink/executável, JSON ambíguo e modo otimizado.

Comandos:

    PYTHONDONTWRITEBYTECODE=1 python3 proposals/qualified-artifact-promotion/tests/test_verify_candidate.py
    PYTHONDONTWRITEBYTECODE=1 python3 -O proposals/qualified-artifact-promotion/tests/test_verify_candidate.py

O ZIP interno experimental continua exigindo um layout estrito. O ZIP externo gerado por upload-artifact precisa de adaptador próprio com digest/tamanho de transporte vinculados ao artifact ID e tentativa; não se presume que ele tenha o mesmo digest do ZIP interno. Nenhum protótipo é conectado a um publicador.
