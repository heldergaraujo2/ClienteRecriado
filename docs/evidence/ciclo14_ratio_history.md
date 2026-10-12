# Histórico reconciliação ratio ossos — 1271×, 1846,28×, 5538×

## Objetivo
Explicar explicitamente a proveniência de cada um dos três números que apareceram na história do achado de ossos BMD.

## População base (fixa desde ciclo9)
total_bmd 10090
valid 10023 99,34%
parsed level3 8406 83,31% vértice+normal
with_tri level4 8405 83,30% com triângulos
with_bones_loose real 3693 36,60% total 43,9329% geom (3693/8406) 7879 bones
with_bones_strict real 1186 11,75% total 14,1090% geom (1186/8406)

Controle mesma classe: 3 offsets por arquivo (100,200,payload/2), bc=1, com ciclo detection visited[64] self-loop parent==i rejeitado como ciclo A->A, overflow bc>64 rejeitado
ctrl_files 8405 (arquivos com geometria)
trials 25215 = 8405*3
loose_hits 2
strict_hits 0
ctrl per trial loose = 2/25215 = 0,007932% = 0.00007932
ctrl per file loose files 2 rate = 2/8405 = 0,023795% = 0.00023795

## 1271× — proveniência: erro de cálculo intermediário ciclo11 com parser simplificado single-mesh

**O que é:** Valor obtido por tool `/tmp/ciclo11_item1_ratio_raw.cpp` simplificada que usava parser single-mesh 52+... sem multi-mesh +-4, contava real 2966 em vez de 3693 correto (subcontou 727), denominador 7209 em vez de 8405. Cálculo: real simplificado 2966 / geom simplificado 7209 ≈41,14% / ctrl per file simplificado ≈0,032% → ~1271×.

**Por que está errado:** Parser single-mesh não decodifica modelos com múltiplas malhas (max_mc 30 geral, 9 com ossos), subconta real 2966 vs 3693 correto -727. Denominador também subcontado 7209 vs 8405. Valor intermediário descartado após correção multi-mesh em ciclo10 Fase A `0d84b0f` e reconciliação ciclo12 item1 `c45b0a0`.

**Evidência:** docs/CICLO11_RELATORIO.md: "Depois ciclo detection: controle NEW 2/25218 0,0079% per file 2/8406 0,0238% ratio 1846,28× definitivo (tool simplificada 1271× estava errada: parser single-mesh subcontou real 2966 vs 3693 correto, denominador 7209 vs 8405)"

**Status atual:** DESCARTADO, substituído por 1846,28×.

## 1846,28× — proveniência: valor definitivo correto per file loose mesma unidade

**O que é:** Ratio per file LOOSE com mesma unidade arquivo vs arquivo, altamente discriminativo.

Fórmula definitiva:
real per geom = 3693/8406 = 0.439329 = 43,9329%
ctrl per file = 2/8405 = 0.00023795 = 0,023795%
ratio = 0.439329 / 0.00023795 = 1846,28×
Fórmula compacta: (3693/8406) / (2/8405) = 1846,28×

**Por que é correto:** Mesma unidade (per file / per file), controle com ciclo detection visited[64] self-loop rejeitado como ciclo A->A, reduz FP de 8,32% (700/8405 sem ciclo detection) para 0,023795% (2/8405 com ciclo detection) fator ~1052×, mais correção unidade -3,5× explica salto 5,43×→1846×.

**Evidência:** docs/evidence/ciclo13_bones_ratio_final_calc.log passo a passo reproduzível, docs/evidence/ciclo13_bones_limits.log, mutool bmdreport 8406/8405/3693/1186, tool /tmp/ciclo12_final_ratio.cpp.

**Status atual:** DEFINITIVO, headline loose, usado em toda documentação futura como referência per file.

## 5538× — proveniência: cálculo per trial com unidades diferentes, INVÁLIDO não relacionado a 1271×

**O que é:** Ratio per trial LOOSE com unidades diferentes arquivo vs tentativa, NÃO deve ser usado.

Fórmula inválida:
real per geom = 3693/8406 = 43,9329%
ctrl per trial = 2/25215 = 0.00007932 = 0,007932%
ratio = 0.439329 / 0.00007932 = 5538,84× (às vezes arredondado 5538×)

**Por que é inválido:** Mistura unidade per file (real) com per trial (controle). Controle tem 3 tentativas por arquivo (trials 25215 = 8405*3), então per trial dilui denominador 3× vs per file, infla ratio 3× (1846,28× *3 = 5538,84×). Não é erro de parser como 1271×, é erro de unidade. Não relacionado a 1271×, é outro erro não relacionado.

**Evidência:** docs/evidence/ciclo13_bones_ratio_final_calc.log seção "ratio per trial LOOSE (unidades diferentes INVÁLIDO) — Não usar, unidades diferentes arquivo vs tentativa", docs/CURRENT_STATE.md ciclo13.

**Status atual:** INVÁLIDO, documentado apenas para evitar reutilização, nunca usar como headline.

## Resumo frase clara por número

- **1271×:** Proveniência tool simplificada ciclo11 single-mesh 52+... sem multi-mesh +-4 contava real 2966 subcontou 727 vs 3693 correto denominador 7209 vs 8405, erro de parser intermediário descartado em ciclo12 item1 `c45b0a0`, substituído por 1846,28×.

- **1846,28×:** Proveniência cálculo definitivo per file LOOSE mesma unidade (3693/8406) / (2/8405) = 43,9329% / 0,023795% = 1846,28× altamente discriminativo com ciclo detection visited[64] self-loop rejeitado reduz FP 8,32%→0,023795% ~1052×, valor definitivo usado como referência futura.

- **5538×:** Proveniência cálculo per trial LOOSE com unidades diferentes (3693/8406) / (2/25215) = 43,9329% / 0,007932% = 5538,84× ≈1846,28×*3 inflado 3× por misturar arquivo vs tentativa, INVÁLIDO não relacionado a 1271×, nunca usar.

## Referências cruzadas

- ciclo9: 5,43× marginal per geom vs per trial sem ciclo detection 2099/25218 8,32%
- ciclo10 Fase A `0d84b0f`: correção ratio per file 1846× com ciclo detection
- ciclo11 item1 `1728d5f` explica salto 5,43×→1846× números exatos antes/depois unidade e ciclo
- ciclo12 item1 `c45b0a0` reconcilia 1271× vs 1846× definitivo
- ciclo13 item3 `bf1b7f0` evidencia ratio per file 1846,28× calc determinístico passo a passo
