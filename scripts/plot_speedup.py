import pandas as pd
import matplotlib.pyplot as plt
import sys

if len(sys.argv) < 2:
    print("Uso: python plot_speedup.py resultados.csv")
    sys.exit(1)

df = pd.read_csv(sys.argv[1])

# Graficar tiempos
plt.figure(figsize=(8,5))
plt.plot(df["N"], df["Tiempo_Secuencial"], marker="o", label="Secuencial")
plt.plot(df["N"], df["Tiempo_Paralelo"], marker="o", label="Paralelo")
plt.xlabel("Número de Notas (N)")
plt.ylabel("Tiempo promedio por frame (segundos)")
plt.title("Comparación de tiempos")
plt.legend()
plt.grid(True)
plt.savefig("graphs/tiempos.png")

# Graficar speedup
plt.figure(figsize=(8,5))
plt.plot(df["N"], df["Speedup"], marker="s", color="purple")
plt.axhline(y=1, color="red", linestyle="--", label="Sin speedup")
plt.xlabel("Número de Notas (N)")
plt.ylabel("Speedup (Tseq / Tpar)")
plt.title("Speedup de la versión paralela")
plt.legend()
plt.grid(True)
plt.savefig("graphs/speedup.png")

# Graficar FPS
plt.figure(figsize=(8,5))
plt.plot(df["N"], df["FPS_Secuencial"], marker="o", label="Secuencial")
plt.plot(df["N"], df["FPS_Paralelo"], marker="o", label="Paralelo")
plt.xlabel("Número de Notas (N)")
plt.ylabel("FPS promedio")
plt.title("Comparación de FPS")
plt.legend()
plt.grid(True)
plt.savefig("graphs/fps.png")

print("Gráficas generadas: tiempos.png, speedup.png, fps.png")
