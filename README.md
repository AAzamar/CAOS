# CAOS
PRNG chaos based, and the NIST assessments

Se utiliza el método númerico de Euler para evaluar el modelo caótico de Lorenz para generar un bitstream al cual se le aplican las evaluaciones del NIST (las primeras 4 de 15) y así determinar la aleatoriedad de los bits generados.
El archivo cephes_mex.c se debe ubicar junto a cephes.c en la carpeta \src de la suit NIST que es el puente para llamar a la función cephes.c desde matlab. Se compila el archivo cephes_mex.c en matlab con:
    mex cephes_mex.c cephes.c
Esto genera el archivo cephes_mex.mex en la carpeta \src y ahora la función puede llamarse desde matlab. test.c es para programarlo en RP2040 y transmitir la información mediante tramas API y un canal seguro. Se utiliza el DMA de RP2040 para alojar en un buffer los datos del FIFO del ADC de manera continúa muestreando a 500k Hz mientras el primer CORE genera el bitstream y lo aloja en un segundo buffer, así el segundo CORE se encarga de XOR los dos buffers y generar un API frame con su correspondiente checksum; el checksum se genera insitu con la sumatoria CRC que permite el DMA.
