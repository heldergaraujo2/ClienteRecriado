> **CORREÇÃO REGISTRADA (ciclo 2):** este arquivo afirmou, em versões
anteriores, que a rota "implementação open-source do formato BMD" estava
bloqueada por falta de acesso de saída a um host de código. **Isso era falso.**
`github.com`, `codeload.github.com` e `api.github.com` são acessíveis deste
ambiente. A afirmação foi propagada para `docs/evidence/ciclo1_bmd_layout.log`
e para as mensagens dos commits `d980216` e `50a5ceb`. A rota foi então executada
e resolveu o bloqueio nº 1.

---

# Próxima tarefa

## Contexto: o bloqueio nº 1 está resolvido

A cifra v0x0C foi quebrada e o layout do corpo foi documentado, ambos a partir
do projeto `xulek/muonline-bmd-viewer` (ISC). Ver `docs/DECISIONS.md` (DEC-007)
e `docs/LICENSING.md` para a proveniência.

O que mudou, medido no conjunto real de 10.090 arquivos:

| Nível | Antes | Agora |
|---|---|---|
| 2 — payload em claro | 1.496 (14,83%) | **8.571 (84,95%)** |
| 3 — geometria parseada | 1.191 (11,80%) | **8.406 (83,31%)** |
| OPAQUE | 8.594 (85,17%) | **1.519 (15,05%)** |

## 1. Fechar o parse completo do corpo — PARCIAL

Um parser de referência foi escrito em `tools/bmd_parse_reference.py` e roda
sobre os 1.465 arquivos claros:

| Versão | Parse completo | Falha dominante |
|---|---|---|
| v0x0A | 637 / 1.194 (53%) | `malha 1 excede payload` |
| v0x0C | 4.476 / 8.741 (51%) | `malha 1 excede payload` |

A falha é sempre na **segunda malha**, o que indica que algum tamanho de
registro ou algum campo por malha difere do assumido para certos arquivos.
Candidatos não testados: `numTexCoords` podendo não ser o número de pares de UV,
ou o nome de textura tendo comprimento variável em vez de 32 B fixos.

## 2. Portar triângulos, UV e ossos para C++ — NÃO INICIADO

Só vértices e normais estão em C++ (`mu::BmdParseBodyV0A`). Os demais precisam
ser portados do parser de referência, com testes.

## 3. v0x0C usa `f16` nos arrays — NÃO INICIADO

O parser C++ atual trata tudo como `f32`, que é correto só para v0x0A. Para
v0x0C os arrays de vértice, normal e UV usam half-float. Sem isso o nível 3 para
v0x0C está contando posições com o tipo errado.

**Isto é um defeito conhecido do número 83,31%.** Ele precisa ser corrigido ou o
número precisa ser qualificado no relatório.

## 4. Investigar os 1.519 arquivos ainda OPAQUE

A verificação de plausibilidade rejeitou a decifragem. Causa não determinada:
pode ser uma variante de chave, um cabeçalho deslocado ou arquivos realmente
corrompidos.
