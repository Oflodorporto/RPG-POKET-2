# Revisão independente local — 14/10/2026

Nenhum defeito funcional restante encontrado nos três adaptadores novos após a correção das fronteiras abaixo:

- callback de rádio apenas copia bytes limitados para fila fixa;
- Endpoint e UI somente no loop;
- geração da ligação e da sessão impedem entrega/envio após reinício do objeto;
- captura de toque separada do despacho, com cancelamento após overflow;
- revisão, hash, disponibilidade e geração de interação invalidam gestos antigos;
- pausa/recuperação do mesmo estado cancela gesto capturado antes da pausa;
- transformações retrato/paisagem inversas e retângulos recortados;
- validação de produção mais filtro ocioso sem descartar histórico válido de dungeon/PvP;
- nenhuma referência mutável à campanha ou função de persistência retida.

Reexecução independente dos binários: otimizado e ASan/UBSan passaram 230.981 verificações, sem alocações C++ instrumentadas. Hash final estável/perturbado: 230425349.

Hashes revisados:

- CoopTraining.h: `4e1baba82dcd244a34b118a6ea06ea64c7ea04ad9c90284a21340200acd80baa`
- CoopPortraitBridge.h: `4f0669fb57a11414744ec89bb91ef5d8c73495f614543370004dcc154685e219`
- CoopRadioPort.h: `af9ed85bef8159208f9aa0007eb08e1a8c540e8ca3d89422ea71a0db5d4b5e43`
- tests.cpp: `4e09724658bdb19c73391ca809fd404f3a03c4376e4331046471f1d3829825f1`

A revisão não modificou arquivos. Não foram validados escalonamento real entre núcleos, hardware ESP-NOW, orientação física, memória ou tempo de placa. `firmware/`, `tests/`, `tools/`, `.github/` e README principal não têm modificações rastreadas. Esta revisão não aprova publicação ou habilitação no menu.
