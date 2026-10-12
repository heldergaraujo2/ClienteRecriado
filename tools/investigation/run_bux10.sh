# Ciclo: ciclo14 A2
# Pergunta: Roda N=10 benchmark BuxCrypt mediana desvio padrão, reconciliação pico 3437 vs mediana
# Evidência: docs/evidence/ciclo14_buxcrypt_10runs.log
# Origem: /tmp/run_bux10.sh movido para backup permanente ciclo15 Parte A
# Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
# Data backup: 2026-10-12
# Branch: arena/519f8fc9-clienterecriado

#!/bin/bash
set -e
cd /home/user/ClienteRecriado
for i in $(seq 1 10); do
  ./build/bench_assets 2>&1 | grep "BuxCrypt 1 MiB" | awk '{print $2, $3, $4, $5, $6, $7}'
done
