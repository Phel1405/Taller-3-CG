# Taller 3. Cortador de Volúmenes

Visualizador interactivo de volúmenes 3D en C++ con VTK. Permite cortar un volumen con un plano que se mueve con el mouse, y alternar entre la vista volumétrica, el resultado de `vtkClipVolume` y una malla de superficie extraída con Marching Cubes.

Taller 3 de Computación Gráfica, Pontificia Universidad Javeriana.

## Autores

- Juan Felipe Rodriguez
- Sofia Guerra Jiménez
- Sara Lucía Ocampo

## Requisitos

- Compilador de C++ compatible con C++17 o superior (GCC)
- CMake 3.12 o superior
- make
- VTK 9.0 o superior, con soporte de renderizado OpenGL2
- Librerías de OpenGL y del sistema de ventanas

## Compilación

```bash
git clone https://github.com/Phel1405/Taller-3-CG.git
cd Taller-3-CG
mkdir build && cd build
cmake ..
make
```

## Uso

```bash
./VolumeSlicer ../hand_raw.mha ../cta_volume_properties.txt
```

| Argumento | Descripción |
|---|---|
| `imagen.mha` | Volumen 3D en formato MetaImage |
| `parametros_volumen.txt` | Funciones de color, opacidad y gradiente, y parámetros de iluminación para el raycasting |

El archivo de parámetros acepta los comandos `color`, `opacity`, `gradient`, `shade`, `ambient`, `diffuse` y `specular`.

## Controles

| Tecla o acción | Efecto |
|---|---|
| Mouse sobre el volumen | Rota, desplaza y acerca la cámara (trackball) |
| Mouse sobre el widget del plano | Traslada y rota el plano de corte |
| `v` | Volumen con raycasting, cortado en vivo por el plano (vista inicial) |
| `k` | Volumen cortado con `vtkClipVolume` |
| `m` | Malla de Marching Cubes, cortada por el plano |
| `x` | Invierte el lado del volumen que se conserva |

La caja envolvente se muestra en rojo y los ejes de orientación aparecen en la esquina inferior izquierda.

## Cómo funciona

- **Vista `v`.** `vtkFixedPointVolumeRayCastMapper` recibe el plano con `AddClippingPlane()`, así que el corte se actualiza mientras se arrastra.
- **Vista `k`.** `vtkClipVolume` recorta el volumen con un segundo plano que solo se actualiza al soltar el mouse, para no recalcular el filtro en cada movimiento. La salida es una malla no estructurada, que se dibuja con `vtkDataSetMapper` y no con raycasting.
- **Vista `m`.** `vtkMarchingCubes` extrae la superficie con un isovalor fijo de 300, y `vtkClipPolyData` la corta con el mismo plano.

## Limitaciones

- La vista `k` no usa raycasting. El mapper de raycasting solo acepta `vtkImageData` y la salida de `vtkClipVolume` es un `vtkUnstructuredGrid`.
- La primera vez que se activan `k` y `m` se calculan los filtros, así que pueden tardar un momento.
- El isovalor de Marching Cubes está fijo en el código. Con otro volumen puede ser necesario ajustarlo.

## Estructura

```
.
├── CMakeLists.txt
├── VolumeSlicer.cxx
├── hand_raw.mha
├── cta_volume_properties.txt
└── docs/   (documento del taller)
```
