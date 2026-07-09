# Dinámica y Sincronización en Láseres Acoplados

> Motor numérico en **C11** y **XPP-AUT** para automatizar simulaciones estocásticas con retardo, ejecutar barridos paramétricos y clasificar regímenes dinámicos en láseres semiconductores acoplados.

[![C11](https://img.shields.io/badge/C-11-00599C.svg?logo=c&logoColor=white)](https://en.cppreference.com/w/c/11)
![XPP-AUT](https://img.shields.io/badge/Engine-XPP--AUT-5B5B5B)
[![DOI](https://img.shields.io/badge/DOI-10.53287%2Fqpea8929bo19y-blue)](https://doi.org/10.53287/qpea8929bo19y)

---

## Publicación

Los resultados y la metodología de este repositorio fueron publicados y seleccionados como **[portada del número 43](assets/portada.png)** de la *Revista Boliviana de Física*:

> Daniel Polo-Alcoba, Gonzalo Marcelo Ramírez-Ávila. *Uso de periodicidades en la descripción de la dinámica y sincronización en un sistema transmisor receptor de láseres*. Revista Boliviana de Física, 43(43), 3–10, 2023.  
> **DOI:** [10.53287/qpea8929bo19y](https://doi.org/10.53287/qpea8929bo19y) · [SciELO](http://www.scielo.org.bo/scielo.php?script=sci_arttext&pid=S1562-38232023000200002) · [PDF](http://www.scielo.org.bo/pdf/rbf/v43n43/v43n43_a02.pdf)

---

## 1. Problema y motivación

La caracterización de la dinámica mediante diagramas de bifurcación para un parámetro de control y planos de periodicidad en el espacio de parámetros son técnicas consolidadas en **sistemas dinámicos discretos**, pero carecían de desarrollo sistemático en **sistemas continuos**.

Por otra parte, aunque existen referencias con indicadores estadísticos para delimitar regiones de sincronización entre láseres acoplados, estos criterios por sí solos no informan sobre el **comportamiento dinámico interno** del enganche (si ocurre en un régimen estacionario, periódico o caótico).

Este trabajo resuelve esa brecha en tres etapas:
1. **Extiende el mapeo de periodicidades a un sistema continuo** mediante el conteo de extremos locales post-transitorio.
2. **Delimita las regiones de sincronización completa** a partir de dos indicadores estadísticos de potencia.
3. **Introduce por primera vez en sistemas continuos el cruce de ambos enfoques**, combinando los mapas de periodicidad con las zonas de enganche para determinar el régimen dinámico exacto de cada región sincronizada.

---

## 2. Metodología y arquitectura

Para ejecutar este análisis a escala sobre el modelo continuo de Mirasso / Lang-Kobayashi (`models/laserdel1.ode`), se desarrolló una herramienta CLI en **C11** (`bin/laser`) que orquesta la integración numérica en **XPP-AUT** y procesa las series temporales resultantes.

```text
models/laserdel1.ode ──▶ [ bin/laser (C11) ] ──▶ XPP-AUT (EDO continua con retardo)
                                │
                                ▼
                     Series temporales (.dat)
                                │
                                ▼
                   [ bin/laser analyze ] ──▶ Métricas CSV (error, period, extrema) ──▶ gnuplot
```

El pipeline implementa los tres componentes del estudio:
1. **Integración continua y barridos (`run`, `sweep1d`, `sweep2d`):** Automatiza corridas individuales y barridos paramétricos en 1 y 2 ejes con inyección dinámica de variables (`--set k=v`) y soporte de reanudación ante interrupciones (`--resume`).
2. **Caracterización dinámica continua (`--metric extrema`, `period`):** Descarta el transitorio (`--transient`) y contabiliza extremos locales sobre la serie continua para distinguir estados estacionarios, periódicos y caóticos.
3. **Detección de sincronización y cruce de mapas (`--metric error`):** Evalúa la razón media de potencias ($\langle \epsilon_P \rangle$) junto con su error estándar relativo ($\Delta\epsilon_P\%$) para aislar las zonas de enganche y superponerlas con el mapa de periodicidades.

---
