# Próxima tarefa

> **Ciclo 4 fechado** com `docs/CICLO4_RELATORIO.md`. Atualizado no mesmo ciclo
> em que mudou, conforme a regra permanente.

## O que mudou neste ciclo

`DEC-008` estava incompleta e era o que bloqueava tudo. **Foi completada** em
`DECISIONS.md` (DEC-008-A), com medição real onde este ambiente permite medir e
`BLOQUEADO` declarado onde não permite. A trava de `DEC-010` sobre código de
backend **deixou de valer por ausência de decisão** — mas continua valendo por
ausência de ambiente, e esse é um motivo diferente que precisa ficar separado.

> **Correção do ciclo 5.** Uma versão anterior comparava a expansão com o
> orçamento de um quadro. **Era inválido**: a expansão é custo de
> **carregamento**. O que se repete por quadro é o **volume** de submissão, e o
> custo dele está `BLOCKED`.

O critério de validação tem três partes: **A** (carregamento, medido — expandir
custa **5,16×** mais que subir o buffer indexado), **A'** (volume por quadro,
quantidade exata — **1,29×** e **63,9%** de envios redundantes) e **B** (custo
por quadro, `BLOQUEADA` — sem contexto GL).

## Próxima tarefa: estender o parser BMD para multi-mesh

**Por que esta, e não outra.** É a única descoberta do ciclo 4 que virou
trabalho conhecido em vez de pesquisa:

- O item 1 da auditoria mediu que a queda do nível 3 para o nível 4 (8.406 →
  4.149, perda de 50,6%) tem **causa única e total: `meshCount != 1` em 100,00%
  dos 4.257 arquivos**. Nenhum falha por tamanho, versão ou campo ausente.
- A causa é **deste parser**, não do formato. Ele lê um bloco de triângulos; com
  várias malhas a mesma sequência se repete e ler um bloco leria o errado em
  silêncio.
- A sequência por malha é a mesma já verificada. Falta iterar e somar offsets.
  É **implementação conhecida**, não investigação.
- Recupera **4.257 arquivos** — mais do que o parser lê hoje (4.149).
- É testável neste ambiente: não depende de GPU, contexto nem Windows.

**Critério de aceite:**

1. `mutool bmdreport` nível 4 acima de 4.149, com o denominador na saída
2. `skip_multi_mesh` caindo de 4.257 para o resto irredutível, medido e não estimado
3. Testes novos cobrindo malha com 2 e com 3 blocos, gerados em fixture
4. `TestTopologyIsSubsetOfGeometry` continuando a passar — a exaustividade das
   causas de pulo é o que impede uma quinta causa de aparecer em silêncio
5. Zero regressão: 372 checks atuais continuam passando, ASan+UBSan limpo

**O que isso NÃO destrava, e precisa ser dito antes de começar:** o nível 6 (UV)
continua em **zero**. Um modelo sem textura pode ser desenhado; um modelo
texturizado não. Multi-mesh aumenta a malha montável, não a renderizável.

## Depois disso

| Tarefa | Por quê | Bloqueio |
|---|---|---|
| Ordem dos 6 `f32` de transformação de osso | "posição antes de rotação" é inferência, não medição. Sem isso, esqueleto animado é chute | nenhum — testável aqui |
| Conteúdo dos 3 `f32` de keyframe | 77,00% dos arquivos têm o bloco; o conteúdo não foi determinado | nenhum — testável aqui |
| Escala vertical da altura de terreno | três caminhos fechados; sem ela, terreno tem proporção errada | nenhum — testável aqui |
| Índice de UV por canto | **nove** hipóteses descartadas. Critério de parada ativo: não reabrir sem hipótese genuinamente nova | investigação, não implementação |
| Backend VBO/VAO | decisão registrada, implementação conhecida | **GPU/contexto — BLOQUEADO** |
| Validação da parte B de DEC-008 | benchmark em contexto real | **GPU/contexto — BLOQUEADO** |
| Baseline de FPS/frametime/VRAM | objetivo declarado do projeto | **GPU — BLOQUEADO** |

## O que não adianta retomar

**As nove hipóteses de UV.** Todas testadas e descartadas, com controle válido
em cada uma. A nona (bytes 24..59 do triângulo como coordenadas UV por canto)
deu 79,42% em [0,1] — contra bytes aleatórios (24,82%) pareceria forte, razão
3,2×. **Esse controle é inválido**: a pergunta é "estes `f32` são UV", então o
controle correto é outro `f32` que não seja UV. O array de normais dá 70,34%,
razão de apenas **1,13×**. Rejeitada. A regra ficou registrada: **o controle tem
que ser da mesma classe do que se testa.**

**Criptoanálise dos 1.519 opacos.** XOR periódico (P=1..32), CFB, CBC-like,
rolling-sub e chaves derivadas já foram tentados. Sem fonte nova, não há o que
fazer.

## Verificação de conformidade, permanente

```
grep -rn "glGenBuffers\|wglCreateContext\|glDrawElements\|GL/gl.h" src/
```

Deve retornar **apenas comentários**. Se retornar código, ele foi escrito sem
ambiente que o execute e deve ser revertido. Verificado no ciclo 4: uma
ocorrência, comentário em `mesh.hpp:20`, zero includes gráficos em `src/`.
