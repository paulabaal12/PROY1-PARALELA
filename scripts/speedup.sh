#!/bin/bash
N=5000
TIMEOUT=10

# Ejecutar versiones y guardar salida
echo "Ejecutando secuencial..."
timeout $TIMEOUT ./screensaver_seq $N > out_seq.txt 2>&1
echo "Ejecutando paralelo..."
timeout $TIMEOUT ./screensaver_par $N > out_par.txt 2>&1

# Extraer promedios
SEQ=$(grep -m1 "Tiempo promedio secuencial" out_seq.txt | awk '{print $4}')
PAR=$(grep -m1 "Tiempo promedio paralelo" out_par.txt | awk '{print $4}')

FPS_SEQ=$(grep -m1 "FPS promedio" out_seq.txt | awk '{print $3}')
FPS_PAR=$(grep -m1 "FPS promedio" out_par.txt | awk '{print $3}')

echo "Tiempo promedio secuencial: $SEQ"
echo "Tiempo promedio paralelo: $PAR"
echo "FPS promedio secuencial: $FPS_SEQ"
echo "FPS promedio paralelo: $FPS_PAR"

if [[ -n "$SEQ" && -n "$PAR" ]]; then
    SPEEDUP=$(echo "scale=2; $SEQ / $PAR" | bc -l)
    echo "Speedup (tiempo) = $SPEEDUP x"
fi

if [[ -n "$FPS_SEQ" && -n "$FPS_PAR" ]]; then
    SPEEDUP_FPS=$(echo "scale=2; $FPS_PAR / $FPS_SEQ" | bc -l)
    echo "Speedup (FPS) = $SPEEDUP_FPS x"
fi
