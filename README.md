# CAOS
PRNG chaos based, and the NIST assessments

Se utiliza el método númerico de Euler para evaluar el modelo caótico de Lorenz generando para generar un bitstream al cual se le aplican las evaluaciones del NIST (las primeras 4 de 15) para determinar la aleatoriedad de los bits generados.
El archivo cephes_mex.c se debe ubicar junto a cephes.c en la carpeta \src de la suit NIST que es el puente para llamar a la función cephes.c. Se compila el archivo cephes_mex.c en matlab con:
    mex cephes_mex.c cephes.c
Esto genera el archivo cephes_mex.mex en la carpeta \src y ahora la función puede llamarse desde matlab.
