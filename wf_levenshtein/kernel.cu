#include "kernel.cuh"

__device__ int calculateWeight(int* weights, size_t lenSet, size_t pitchWeights, char x, char y)
{
    int xx = ((int)x-'0'); // transforma el caracter numero a int
    int yy = ((int)y-'0'); // transforma el caracter numero a int
    //printf("sustitución: %d %d\n", xx, yy);
    //printf("valor de: %d \n", weights[yy * pitchWeights / sizeof(int) + xx]);
    return weights[xx * pitchWeights / sizeof(int) + yy];
}

__global__ void getDistanceKernel(char* wordsTargets, int* wordsTargetsPos, char* wordSource, int lenWordSource, size_t* pitchMatrix, int** matrix, int* weights, size_t lenSet, size_t pitchWeights, int* distances, int nWords)
{
    //int tidx = blockIdx.x * (blockDim.x * blockDim.y) + (threadIdx.y * blockDim.x) + threadIdx.x;
    int tidx = threadIdx.x + blockDim.x*blockIdx.x;
    
    if (tidx < nWords) {
        //printf("idx %d", tidx);
        int len = wordsTargetsPos[(tidx * 2) + 1] - wordsTargetsPos[tidx * 2]; // largo de la palabra dentro del wordsTargetsPos
        int init = wordsTargetsPos[tidx * 2]; // inicio de la palabra dentro del wordsTargetsPos
        //int end = wordsTargetsPos[(tidx * 2) + 1]; // Fin de la palabra dentro del wordsTargetsPos
        int t1, t2, i, j, costo, res, ancho, min, del, in, sus;
        t1 = len;
        t2 = lenWordSource;


        // Verifica que exista algo que comparar
        if (t1 == 0) return;
        if (t2 == 0) return;
        ancho = pitchMatrix[tidx] / sizeof(int);
        int* m = matrix[tidx];

        // Rellena primera fila y primera columna
        for (i = 0; i <= t2; i++) m[i * ancho + 0] = i * 2;
        for (j = 0; j <= t1; j++) m[j] = j * 2;
        char vWordS, vWordT;
        // Recorremos resto de la matriz llenando pesos
        for (i = 1; i <= t2; i++) for (j = 1; j <= t1; j++)
        {
            vWordS = wordSource[i - 1];
            vWordT = wordsTargets[(init + j) - 1];
            costo = calculateWeight(weights, lenSet, pitchWeights, vWordS, vWordT);
            // Calcula el minimo 
            del = m[i * ancho + j - 1] + 2; // El mas dos es el costo de borrado
            in = m[(i - 1) * ancho + j] + 2; // El mas dos es el costo de inserción
            sus = m[(i - 1) * ancho + j - 1] + costo; // Costo de sustitución
            min = del < in ? del : in;
            min = min < sus ? min : sus;
            // guarda en la matriz el minimo
            m[i * ancho + j] = min;
        }

        res = m[t2 * ancho + t1];

        distances[tidx] = res;
    }
    return;

}

__global__ void levenshteinKernel(char* wordsTargets, int* wordsTargetsPos, char* wordSource, int lenWordSource, size_t* pitchMatrix, int** matrix, int** actions, int* weights, size_t lenSet, size_t pitchWeights, int nWords)
{
    //int tidx = blockIdx.x * (blockDim.x * blockDim.y) + (threadIdx.y * blockDim.x) + threadIdx.x;
    int tidx = threadIdx.x + blockDim.x * blockIdx.x;
    if (tidx < nWords) {
        int len = wordsTargetsPos[(tidx * 2) + 1] - wordsTargetsPos[tidx * 2]; // largo de la palabra dentro del wordsTargetsPos
        int init = wordsTargetsPos[tidx * 2]; // inicio de la palabra dentro del wordsTargetsPos
        int  i, j, costo, min, del, in, sus;
        const int t1 = len;
        const int t2 = lenWordSource;
        //printf("%d | %d |%d | %d | %d \n", t1, t2, init, end, len);
        for (i = 0; i <= t2; i++) 

        // Verifica que exista algo que comparar
        if (t1 == 0) return;
        if (t2 == 0) return;
        //ancho = pitchMatrix[tidx] / sizeof(int);
        const int ancho = pitchMatrix[tidx] / sizeof(int);
        //printf("pitch %d\n", pitchMatrix[tidx]);
        int* m = matrix[tidx];

        // Rellena primera fila y primera columna
        for (i = 0; i <= t2; i++) m[i * ancho + 0] = i * 2;
        for (j = 0; j <= t1; j++) {
            m[j] = j * 2;
        }
        // Recorremos resto de la matriz llenando pesos
        char vWordS, vWordT;
        for (i = 1; i <= t2; i++) for (j = 1; j <= t1; j++)
        {
            vWordS = wordSource[i - 1];
            vWordT = wordsTargets[(init + j) - 1];
            //printf("%d | %d \n", i, j);
            //printf("%c | %c \n", wordSource[i - 1], wordsTargets[(init + j) - 1]);
            costo = calculateWeight(weights, lenSet, pitchWeights, vWordS, vWordT);
            // Calcula el minimo 
            del = m[i * ancho + j - 1] + 2; // El mas dos es el costo de borrado
            in = m[(i - 1) * ancho + j] + 2; // El mas dos es el costo de inserción
            sus = m[(i - 1) * ancho + j - 1] + costo; // Costo de sustitución
            min = del < in ? del : in;
            min = min < sus ? min : sus;
            // guarda en la matriz el minimo
            m[i * ancho + j] = min;
        }
        
        int del_t, del_w, ins_t, ins_w, sus_t, sus_w, x, y, count,cDis, postarget, posSource;
        int* listActions = actions[tidx];
        x = t1;
        y = t2;
        count = 1;
        listActions[0] = min;
        //if (tidx == 0) {
        
        while (x != 0 || y != 0)
        {
            postarget = x - 1 < 0 ? 0 : x - 1; // La palabra dinamica
            posSource = y - 1 < 0 ? 0 : y - 1; // La palabra constante
            vWordS = wordSource[y - 1];
            vWordT = wordsTargets[(init + x) - 1];

            del_w = (y > 0) ? 2 : INT_MAX;
            del_t = (y > 0) ? m[posSource * ancho + x] + del_w : INT_MAX; // Borrado

            ins_w = (x > 0) ? 2 : INT_MAX;
            ins_t = (x > 0) ? m[y * ancho + postarget] + ins_w : INT_MAX; // inserrción

            sus_w = (x > 0 && y > 0) ? calculateWeight(weights, lenSet, pitchWeights, vWordS, vWordT) : INT_MAX;
            sus_t = (x > 0 && y > 0) ? m[posSource * ancho + postarget] + sus_w : INT_MAX;

            cDis = m[y * ancho + x];
            if (sus_t == cDis)
            {
                if (vWordS != vWordT) {
                    listActions[count] = posSource;
                    listActions[count + 1] = 1;
                    listActions[count + 2] = ((int)vWordT - '0');
                  
                    count += 3;
                }
                y--;
                x--;
            }
            else if (ins_t == cDis)
            {
                listActions[count] = posSource;
                listActions[count + 1] = 0;
                listActions[count + 2] = ((int)vWordT - '0');
                x--;
                count += 3;
            }
            else if (del_t == cDis)
            {
                listActions[count] = posSource;
                listActions[count + 1] = 2;
                listActions[count + 2] = NULL;
                y--;
                count += 3;
            }
            else
            {
                printf("error en la lista de operaciones");
                break;
            }

        }
        listActions[count] = -1;
        
    }

}
