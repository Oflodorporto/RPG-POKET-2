# Hora local e dia/noite — dia1

Versão 2026.10.07-dia1, build2026100703. Segunda etapa da reorganização de Aeldra: base temporal e apresentação do mundo. Os saves permanecem Save10/96bytes e as artes do cartão permanecem as mesmas.

## Como testar na placa

1. Atualizar em Configurações > Atualizar. O botão Horário fica ao lado.
2. Em Horário, confirmar fuso UTC−3. Com hora automática e Wi-Fi conectado, aguardar a sincronização. Sem hora válida aparece “Hora não sincronizada”; os cenários conservam a paleta diurna.
3. Abrir Acertar. Selecionar ano, mês, dia, hora e minuto com Anterior/Próximo e ajustar com os botões grandes −/+ Valor. Aplicar ativa a hora manual. Cancelar preserva o relógio.
4. Conferir 05h (amanhecer), 07h (dia), 18h (crepúsculo) e 20h (noite) no mapa, cidade, exploração e acampamento. Só os fundos recebem a paleta; textos, botões, personagens e itens continuam legíveis. Dentro da cripta e nos menus a paleta continua própria do local.
5. Deixar um minuto sem tocar: relógio preto, brilho reduzido, data e período. Tocar retorna à tela anterior. Desligar Dia/noite em Horário conserva o relógio e deixa os cenários na paleta diurna.

## Regras e limites

- Fuso ajustável em horas inteiras de UTC−12 a UTC+14; padrão UTC−3. Não há cálculo automático de horário de verão nem fusos de meia hora nesta entrega.
- Hora automática usa NTP assíncrono; não consulta servidor a cada quadro. Sem Wi-Fi, a hora válida continua avançando enquanto a placa permanece ligada.
- Hora manual avança enquanto ligada. Após desligamento/reinício, ela deve ser acertada novamente. Não reutilizar uma data salva como se o dispositivo tivesse mantido o relógio desligado. Preferência automática/manual, fuso e ciclo são salvos em um registro separado dos personagens.
- A mudança automática de período aguarda um estado seguro; não troca atmosfera durante combate, viagem ou descanso. O fundo é colorido a partir da paleta em RAM, sem releitura do SD e sem alterar os arquivos de imagens.
- Não existem novos inimigos noturnos nesta versão. Hora não altera encontros existentes, preço, cura, recompensa ou estado dos personagens.
- Carta do hipogrifo, duas ofertas diárias, outros sete eventos e pergaminho continuam planejados. Próximo passo: persistência transacional por personagem, retorno seguro e recompensa única. Bytes90/91 do Save10 continuam reservados ao acampamento.

Testes automatizados cobrem calendário/bissexto, fuso, transbordamento do contador de tempo, ajuste manual, perda de persistência, retomada sem hora confiável, adiamento de período e preservação dos bytes de saves. Prévias usam o renderizador real. Compilação e teste físico são verificações separadas.
