# Ferramentas de investigação — backup para evitar perda em restauração de ambiente

Esta pasta contém scripts/ferramentas de descoberta únicos que foram criados em /tmp durante ciclos 1-14 e que implementam hipóteses de parsing testadas, calculadoras de ratio, probes de offset, etc.

Regra permanente ciclo15: NENHUM script/ferramenta de descoberta deve existir apenas fora do repositório a partir de agora — tudo que produzir uma descoberta documentada deve ter seu código-fonte preservado no repositório (categoria 2a Parte A), desde a criação, não apenas ao final do ciclo.

Cada arquivo tem cabeçalho com: ciclo que produziu, pergunta que respondeu, referência ao log de evidência em docs/evidence/.

Categoria (b) clones re-obteníveis: /tmp/cliente_ref (Cliente.git 88edfc73) e /tmp/bmdref (xulek/muonline-bmd-viewer 3f11bf89) NÃO precisam ser commitados, URL/commit/hash já documentado em LICENSING.md/DEPENDENCIES.md.

Categoria (c) dados brutos voláteis grandes: /tmp/cliente_ref/Data/ 2,4GB NÃO commitar (viola regra não publicar assets proprietários), documentado caminho e hash.

Categoria (d) artefatos build/binários temporários: descartáveis, código-fonte já no repo.

Inventário completo: docs/evidence/ciclo15_workspace_inventory.log
Manifesto: docs/evidence/ciclo15_workspace_manifest.md

