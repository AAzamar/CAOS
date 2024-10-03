#include "mex.h" // Header de MATLAB para MEX
#include "../include/cephes.h" // Incluye el header que declara las funciones cephes

// Función gateway entre MATLAB y C
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    double a, x;
    double result;
    // Validar número de argumentos de entrada y salida
    if (nrhs != 2) {
        mexErrMsgIdAndTxt("MATLAB:cephes_mex:invalidNumInputs", "Se requieren dos argumentos de entrada.");
    }
    if (nlhs > 1) {
        mexErrMsgIdAndTxt("MATLAB:cephes_mex:invalidNumOutputs", "Se espera un solo argumento de salida.");
    }
    // Obtener los valores de entrada (double) desde MATLAB
    a = mxGetScalar(prhs[0]);
    x = mxGetScalar(prhs[1]);
    result = cephes_igamc(a, x);
    // Asignar el resultado a la salida de MATLAB
    plhs[0] = mxCreateDoubleScalar(result);
}
