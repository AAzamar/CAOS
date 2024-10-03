clearvars; format shortG;
blocks = 100;    % Número de bloques
assess = 10000;  % bits a evaluar por bloque
% Definición de parámetros
g = 10; p = 28; B = 8/3;
% Definición de condiciones iniciales
x(1) = 3;
y(1) = 7;
z(1) = 1;
% Método numérico de Euler
h = 0.009;       % Paso de Integración  0.016   0.009
N_sol = 2^21;   % Número de soluciones
comparador = 25.31;   % 25.8821                25.305
bitstream = zeros(1, N_sol);  % Preasignación del bitstream
for t = 1:N_sol
    % Método de Euler para el sistema de Lorenz
    x(t+1) = x(t) + h*(g*(y(t)-x(t)));
    y(t+1) = y(t) + h*(x(t)*(p-z(t))-y(t));
    z(t+1) = z(t) + h*(x(t)*y(t)-B*z(t));
    % Generación del bitstream según la comparación con z(t)
    if z(t) > comparador
        bitstream(t) = 1;
    else
        bitstream(t) = 0;
    end
end
bitstream = bitstream(4e5:N_sol);
N_sol = N_sol/2;
a = 0;  ii = 1; fr = 64;         % 128
nbit = zeros(1, N_sol);
for i = 1:(N_sol/fr)
    for p = 1:fr
        nbit(((N_sol/fr)*p)-a) = bitstream(ii);
        ii=ii+1;
    end
    a=a+1;
end
% filename = 'C:\cygwin64\sts-2.1.2\sts-2.1.2\data\data2.pi';
% fileID = fopen(filename, 'w');  % Abrir archivo para escritura
% for i = 1:length(nbit)
%    fprintf(fileID, '%d', nbit(i));  % Escribir cada bit en el archivo
% end

%% 
for i = 0 : 1 : blocks-1
    binaryData = nbit;
    binaryData = binaryData( ((assess*i)+1):((assess*i)+assess) );
    Pval1(i+1) = frequencyTest     (binaryData, 1, assess);
    Pval2(i+1) = blockFrequencyTest(binaryData, 1, assess);
    Pval3(i+1) = runTest           (binaryData, 1, assess);
    Pval4(i+1) = longestRunTest    (binaryData, 1, assess);
end
result = zeros(4,11);
p1 = histogramMetrics(Pval1,blocks,1);
p2 = histogramMetrics(Pval2,blocks,1);
p3 = histogramMetrics(Pval3,blocks,1);
p4 = histogramMetrics(Pval4,blocks,1);
result = [p1;p2;p3;p4];
for i = 1:size(result, 1)
    fprintf('%2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %8.5f\n', result(i, :));
end


%% Frequency
function Pval1 = frequencyTest(binaryData, blocks, assess)
n =  blocks * assess;                       % n = número total de bits (1e2*1e4=1e6)
suma = 0; sqrt2 = 1.41421356237309504880;
for i = 1:1: n
    suma = suma + ((2.*binaryData(i))-1); 
end
s_obs = abs(suma) / sqrt(n);                % Suma absoluta normalizada  sqrt(n)
f = s_obs/sqrt2;
Pval1 = erfc(f);                            % Función error complementario erfc

%disp(['Monobit Pval: ', num2str(Pval1)]);   % resultado parece acertado
end
%% Block Frequency
function Pval2 = blockFrequencyTest(binaryData, blocks, assess)
M = 128;                                    % Longitud de datos en sub-bloque
N = floor(assess/M);                        % Número de bloques generados
for i = 0 : 1 : N-1
    blocksum = 0;
    for j = 1 : 1 : M
       blocksum = blocksum + binaryData(j+(i*M));
    end
    pii = blocksum/M;
    PI(i+1) = pii;
    v(i+1) = (PI(i+1) - 0.5)^2;
end
chi2 = 4 * M * sum(v);
%Pval2 = gammainc(N/2,chi2/2);               % gammainc() añade error al resultado
Pval2 = cephes_mex(N/2,chi2/2);
%disp(['Block Frequency Chi2: ', num2str(chi2)]); % OK
%disp(['Block Frequency Pval: ', num2str(Pval2)]); 
end
%% Run Test
function Pval3 = runTest(binaryData, blocks, assess)
n = blocks*assess; V = 1;
pii = sum(binaryData(1:blocks*assess))/n;   
if abs(pii-0.5) > (2/sqrt(n))   % Comparación prescindible
    Pval3 = 0;
else
    for k = 2: 1 : n
        if binaryData(k) ~= binaryData(k-1)
            V = V + 1;
        end
    end
end
erfc_arg = abs( V - (2 * n * pii * (1-pii)) ) / (2 * sqrt(2*n) * pii * (1-pii) );
Pval3 = erfc(erfc_arg);
%disp(['Run Test Earg: ', num2str(erfc_arg)]); %OK
%disp(['Run Test Pval: ', num2str(Pval3)]);    % muy consistente
end
%% Longest Run
function Pval4 = longestRunTest(binaryData, blocks, assess)
K = 5; M = 128; n = assess;
nu = [ 0, 0, 0, 0, 0, 0]; V = [4,5,6,7,8,9];  
pii = [0.1174035788,0.242955959,0.249363483,0.17517706,0.102701071,0.112398847];
N = floor(n/M);
for i = 0 : 1 : N-1
    v_n_obs = 0;
    run = 0;
    for j = 1 : 1 : M
        if binaryData((i*M)+j) == 1
            run = run + 1;
            if run > v_n_obs
                v_n_obs = run;
            end
        else
            run = 0;
        end
    end
    if v_n_obs < V(1)
        nu(1) = nu(1) + 1;
    end
    for j = 1 : 1 : K+1
        if v_n_obs == V(j)
            nu(j) = nu(j) + 1;
        end
    end
    if v_n_obs > V(K+1)
        nu(K+1) = nu(K+1) + 1;
    end
end
chi2 = 0;
for i = 1 : 1 : K+1
    chi2 = chi2 + ( (nu(i) - N * pii(i)) * (nu(i) - N * pii(i)) ) / ( N * pii(i) );
end
%Pval4 = gammainc(K/2,chi2/2);
Pval4 = cephes_mex(K/2,chi2/2);
%disp(['Longest Run Test Chi2: ', num2str(chi2)]);  %OK
%disp(['Longest Run Test Pval: ', num2str(pval)]);
end
%% P-ValueT Histogram metrics
function result = histogramMetrics(Pval,blocks,arr)
fpb = zeros(1,10); pos = 0;
for j = 1 : 1 : blocks
    pos = 1+floor(Pval(j)*10);
    if pos >= 10
        pos = 10;
    end
    fpb(pos) = fpb(pos) + 1;
end
s = blocks;        % s parece ser el num de bloques analizados y no el tamaño de bloque 
mo10 = s/10; chi2 = 0;
for j = 1 : 1 : 10
    chi2 = chi2 + ( ((fpb(j) - mo10)^2) / mo10 );
end
% uniformity = gammainc(9/2, chi2/2);
uniformity = cephes_mex(9/2, chi2/2);
result(1:10) = fpb;
result(11) = uniformity;
end
%fclose(fileID);  % Cerrar el archivo
%figure;
%plot(bitstream);
%figure;
%plot(nbit);
%t = 0:1:N_sol;
%figure;
%plot(t, x, 'r', t, y, 'g', t, z, 'b');
%grid on;
