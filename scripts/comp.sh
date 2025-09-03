#!/bin/bash
TIMEOUT=20
OUTPUT="resultados.csv"

# Valores de N a probar
SIZES=(1000 2000 5000 10000 13000 15000)

# Encabezado CSV
echo "N,Tiempo_Secuencial,Tiempo_Paralelo,Speedup,FPS_Secuencial,FPS_Paralelo" > $OUTPUT

for N in "${SIZES[@]}"; do
    echo ">>> Ejecutando con N=$N"

    # Ejecutar secuencial
    timeout $TIMEOUT ./screensaver_seq $N > out_seq.txt 2>&1
    SEQ=$(grep "Tiempo promedio secuencial" out_seq.txt | awk '{print $4}')

    # Ejecutar paralelo
    timeout $TIMEOUT ./screensaver_par $N > out_par.txt 2>&1
    PAR=$(grep "Tiempo promedio paralelo" out_par.txt | awk '{print $4}')

    if [[ -n "$SEQ" && -n "$PAR" ]]; then
        SPEEDUP=$(echo "scale=3; $SEQ / $PAR" | bc -l)
        FPS_SEQ=$(echo "scale=2; 1 / $SEQ" | bc -l)
        FPS_PAR=$(echo "scale=2; 1 / $PAR" | bc -l)
        echo "$N,$SEQ,$PAR,$SPEEDUP,$FPS_SEQ,$FPS_PAR" >> $OUTPUT
    else
        echo "$N,ERROR,ERROR,ERROR,ERROR,ERROR" >> $OUTPUT
    fi
done

echo "Resultados guardados en $OUTPUT"
