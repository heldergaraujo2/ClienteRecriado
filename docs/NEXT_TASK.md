# Próxima tarefa

> **Atualizado no fim do ciclo 2.** A tarefa abaixo foi executada e o registro de
> triângulo foi decodificado em sua parte essencial. O que resta dela está na
> seção seguinte. Os números citados no texto original continuam válidos como
> registro do método.

## Localizar o índice de UV por canto do triângulo

Este é agora **o único bloqueio entre a malha montada e a malha texturizada**.

O array de UV está confirmado e legível (`52+vc*16+nc*20`, `ntc` entradas de 8 B
`[f32 u][f32 v]`; zero não-finitos em 250.450 leituras, quando o acaso daria
~976). Falta apenas saber **qual entrada pertence a cada canto**.

**Três hipóteses já descartadas — não repetir:**

1. Campo em +4 do registro: é **padding** (64,95% zero, 18,18% 0xCDCD, o
   preenchimento de memória não inicializada do MSVC).
2. Quarto canto em +24: **não existe** (offsets +24..+56 dão 50–81% de validade,
   nível de ruído, contra 94–100% em +0/+8/+16).
3. Relação implícita por posição: **refutada** — `numTexCoords` difere de
   `numVertices` e `numNormals` em 93% dos 802 arquivos medidos.

**Onde procurar:** os bytes 24..59 do registro de triângulo, não zero em 59,1%
dos registros e ainda sem identificação. É o único espaço do registro não
examinado.

---

## (Executada) Decodificar os 58 bytes restantes do registro de triângulo

### Por que esta tarefa

O bloqueio nº 1 mudou de natureza no ciclo 2. A cifra foi quebrada e o array de
triângulos foi **localizado** com controle estatístico — razão de 138,3× sobre o
acaso, mais confirmação independente pela posição do nome de textura em 87,9% dos
arquivos de malha única. O problema deixou de ser *onde estão os triângulos* e
passou a ser *o que há dentro de cada registro*.

É a única tarefa que separa 83,31% de "arquivos com vértices lidos" de "modelos
renderizáveis", e é pré-requisito do requisito de reaproveitar 100% dos assets
originais. Também destrava UV e LOD de assets originais.

### O que já se sabe

```
tstart = 52 + numVertices*16 + numNormals*20 + numTexCoords*8
stride = 64 bytes
índices de vértice em +2, +6 e +10   -> 98,05% válidos (controle limpo 2,35%)
```

Vale para **arquivos de malha única** (586 medidos). Para malhas múltiplas o
corpo se repete e não foi medido.

### O que já foi refutado — não repetir

- **Primeiro byte = tamanho do polígono.** Valores 3 e 4 somam 3,01% em 35.902
  registros, indistinguível de aleatório.
- **As tabelas de registro do projeto `xulek/muonline-bmd-viewer`.** Não
  correspondem a estes arquivos para vértice nem para normal; não há razão para
  esperar que correspondam para triângulo.
- **Stride 8/10/12/14/16.** Testados no ciclo 1, todos descartados. O stride é 64.
- **Busca ancorada em `uv_end` derivado de hipótese não confirmada.** Foi o erro
  de método que invalidou as tentativas anteriores.

### Teste concreto a executar

1. Para cada um dos 64 deslocamentos do registro, ler `u16` e medir a fração
   abaixo de `numVertices`, nos mesmos 586 arquivos de malha única.
2. **Controle obrigatório:** a mesma medição na região `f32` de vértices, onde não
   há índice. Deu 2,35%; acaso puro deu 0,71%.
3. Aceitar um campo como índice de vértice só com razão **acima de 10×** sobre o
   controle. Abaixo disso é ruído.
4. Repetir para índice de normal, comparando com `numNormals`.
5. Repetir o item 1 para os 6 primeiros bytes, que são os únicos já
   identificados, como verificação de que o método recupera o que já se sabe.

Reprodutível por `python3 tools/bmd_topology_probe.py <Data>`; o docstring
documenta o método e as duas armadilhas (controle contaminado dentro do próprio
array, e base de payload 4 vs 8 por versão).

### Regras que se aplicam

- Separar sempre **estrutura válida** de **payload utilizável** de **geometria
  renderizável**.
- Toda medição exige linha de base calculada, não apenas "parece bater".
- Se não houver progresso após esforço razoável, registrar como
  **topologia BMD: NÃO RESOLVIDO — tentativas documentadas: [lista]** e avançar.

## Tarefas seguintes, em ordem

1. Coordenadas UV — dependem de topologia para validação.
2. Ossos e animação — nenhum registro localizado ainda.
3. Estender a topologia para malhas múltiplas.
4. Investigar os 1.519 arquivos v0x0C cuja decifragem foi rejeitada.
5. Implementar VBO/VAO (DEC-008) — só depois que houver malha para desenhar, ou
   com geometria sintética **rotulada como "teste sintético de pipeline"**
   (DEC-009).

## O que NÃO fazer

- Não declarar renderização funcionando com base em geometria sintética.
- Não declarar ganho de desempenho: **não há baseline de FPS/frametime/VRAM**
  neste projeto, porque não há Windows nem GPU no ambiente.
- Não extrair topologia parcial e renderizar com ela. Malha silenciosamente
  errada é pior que nenhuma malha.
- Não adotar tabela de layout de fonte externa sem medir contra os arquivos.
