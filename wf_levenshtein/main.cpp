#include "kernel.cuh"
#include "StringEditor.h"
#include "OperatorList.h"

#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <chrono>
#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }


using namespace std;

struct record {
    int nOp; // Numero de veces * el weight correspondiente
    int pos; // Posici�n donde se realizara el cambio en la semilla actual
    int type; // El tipo de operaci�n 0->Insertado 1->Sustituci�n 2->Borrado
    int sVal; // Caracter que se insertara o reemplazara
    int currentDist; // Distancias acumuladas entre todas las cadenas
    int newDist;// Distancia acumulada con la nueva semilla
    int totalDist; // Numero de distancias revisadas desde que se ejecuto el codigo
    int pTry; // La posici�n en la que se encontraba cuando encontro una semilla mejor
    float currentAvgDist; // Actual distancia promedio de la semilla
    float newAvgDist;// Nueva distancia promedio de la semilla
};


tuple <vector<char>, vector<int>> getWords(string letter);
void levenshtein(int** actions, int* wordsTargetsPos, char* dev_wordsTargets, int* dev_wordsTargetsPos, char* wordSource, int lenWordSource, int* dev_weights, size_t lenSet, size_t pitchWeights, int* totalDist, int nWords, int* currentDistance);
int createSeed(int* wordsTargetsPos, char* dev_wordsTargets, int* dev_wordsTargetsPos, char* wordSource, int lenWordSource, int* dev_weights, size_t lenSet, size_t pitchWeights, int nWords);
tuple <int*, size_t > loadWeights(int* weights, size_t lenSet);
tuple <char*, int* > loadData(char* wordsTargets, int* wordsTargetsPos, size_t lenWordsTargets, size_t lenWordsTargetsPos);
record make_record(int nOp, int pos, int type, int sVal, int currentDist, int newDist, int totalDist, int pTry, float currentAvgDist, float newAvgDist);



inline void gpuAssert(cudaError_t code, const char* file, int line, bool abort = true)
{
    if (code != cudaSuccess)
    {
        fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        if (abort) exit(code);
    }
}




record make_record(int nOp, int pos, int type, int sVal, int currentDist, int newDist, int totalDist, int pTry, float currentAvgDist, float newAvgDist) {
    record myrecord = { nOp, pos, type, sVal, currentDist, newDist,  totalDist, pTry, currentAvgDist, newAvgDist };
    return myrecord;
}

string letter = "W";
string folder = "L360";
int threadPerBlock = 6;
int sizeData = 360;


int main(int argc, char* argv[])
{

    if (argc > 1) {
        letter = argv[1];
        folder = argv[2];
        sizeData = stoi(argv[3]);
    }
    vector<record> records;
    int totalDist = 0;
    // Crea archivo para guardar semilla y resultado
    ofstream outfile("Out/" + folder + "/" + letter + ".txt");

    size_t lenWordsTargets = NULL;
    size_t lenWordsTargetsPos = NULL;
    vector<char> v_wordsTargets;
    vector<int> v_wordsTargetsPos;

    tie(v_wordsTargets, v_wordsTargetsPos) = getWords(letter);
    //cout << "extra palabras" << endl;
    lenWordsTargets = v_wordsTargets.size();
    lenWordsTargetsPos = v_wordsTargetsPos.size();
    int* wordsTargetsPos = (int*)malloc(sizeof(int) * lenWordsTargetsPos);
    copy(v_wordsTargetsPos.begin(), v_wordsTargetsPos.end(), wordsTargetsPos);
    char* wordsTargets = (char*)malloc(sizeof(char) * lenWordsTargets);

    int nWords = (int)lenWordsTargetsPos / 2;

    wordsTargets = &v_wordsTargets[0];
    const char* set = "01234567";
    size_t lenSet = strlen(set);
    int* dev_weights = NULL;
    char* dev_wordsTargets = NULL;
    int* dev_wordsTargetsPos = NULL;
    size_t pitchWeights;


    // Extrae la infomaci�n de los pesos de freeman
    string line;
    ifstream myfile("weights.txt");
    int row = 0;
    int col = NULL;
    int* freemanWeights = (int*)malloc(sizeof(int) * lenSet * lenSet);
    size_t position = 0;
    string weight;
    string delimiterSplit = " ";



    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            col = 0;
            while ((position = line.find(delimiterSplit)) != string::npos) {
                weight = line.substr(0, position);
                freemanWeights[lenSet * row + col] = stoi(weight);
                line.erase(0, position + delimiterSplit.length());
                col++;
            }
            freemanWeights[lenSet * row + col] = stoi(line);
            row++;
        }
        
        myfile.close();
    }


    tie(dev_weights, pitchWeights) = loadWeights(freemanWeights, lenSet);
    tie(dev_wordsTargets,dev_wordsTargetsPos) =  loadData(wordsTargets, wordsTargetsPos, lenWordsTargets, lenWordsTargetsPos);

    // Obtiene la semilla
    auto startTime = chrono::high_resolution_clock::now();



    int init, end;
    char* wordSource;
    int lenWordSource;
    float* seeds = (float*)malloc((nWords) * sizeof(float));
    for (int x = 0; x < nWords; x++)
    {

        init = wordsTargetsPos[x * 2];
        end = wordsTargetsPos[(x * 2) + 1];
        lenWordSource = end - init;
        wordSource = (char*)malloc(lenWordSource + 1 * sizeof(char));
        memcpy(wordSource, &wordsTargets[init], sizeof(char) * lenWordSource);
        wordSource[lenWordSource] = '\0';
        seeds[x] = createSeed(wordsTargetsPos, dev_wordsTargets, dev_wordsTargetsPos, wordSource, lenWordSource, dev_weights, lenSet, pitchWeights, nWords);
        totalDist += 359;
        free(wordSource);
        cout << x << endl;
    }

    int minIndex = 0;
    int min = seeds[0];
    for (int x = 0; x < nWords; x++)
    {
        if (seeds[x] < min)
        {
            minIndex = x;
            min = seeds[x];
        }
    }
    
    cout << "distancia promedio " << endl;
    cout << fixed;
    cout << setprecision(5) << (float)min / nWords << endl;
    init = wordsTargetsPos[minIndex * 2];
    end = wordsTargetsPos[(minIndex * 2) + 1];
    lenWordSource = end - init;
    wordSource = (char*)malloc(lenWordSource + 1 * sizeof(char));
    memcpy(wordSource, &wordsTargets[init], sizeof(char) * lenWordSource);
    wordSource[lenWordSource] = '\0';
    cout << "semilla inicial: " << endl << wordSource << endl;
    cout << "TotalDist: " << totalDist << endl;
    totalDist = totalDist / 2;
    outfile << "SetMedian AvgDist: " << fixed << setprecision(5) << (float)min / nWords << " TotalDist: " << totalDist << endl;
    outfile << letter << " " << wordSource << endl;
    
    cout << "Comienza a calcular distancias" << endl;
    
    int** currentActions;
    currentActions = (int**)malloc(nWords * sizeof(int*));
    int** summary;
    float bestDistance = min;
    int currentDistance = min;
    
    bool lOk = true;
    vector<operatorVector> orderActions;
    string bestSeed(wordSource);
    char* currentSeed = wordSource;
    int lenCurrentSeed = lenWordSource;
    int countTry = 0;
    string temp;

    levenshtein(currentActions,  wordsTargetsPos, dev_wordsTargets, dev_wordsTargetsPos, currentSeed, lenCurrentSeed, dev_weights, lenSet, pitchWeights,&totalDist, nWords, &currentDistance);
    summary = pointerReader(currentActions, nWords, lenSet, lenCurrentSeed);
    orderActions = orderList(summary, lenSet, lenCurrentSeed, currentSeed, freemanWeights);
    free(summary);
    while (lOk) 
    {
        lOk = false;
        countTry = 0;
        for (operatorVector& x : orderActions)
        {
            temp = StringEditor(bestSeed, x.pos, x.type, to_string(x.sVal));

            currentSeed = &temp[0];
            lenCurrentSeed = temp.size();
            levenshtein(currentActions, wordsTargetsPos, dev_wordsTargets, dev_wordsTargetsPos, currentSeed, lenCurrentSeed, dev_weights, lenSet, pitchWeights, &totalDist, nWords, &currentDistance);
            if (currentDistance < bestDistance) {
                records.push_back(make_record(x.nOp, x.pos, x.type, x.sVal, bestDistance, currentDistance, totalDist, countTry, (float)bestDistance / nWords, (float)currentDistance / nWords));
                lOk = true;
                break;
            }
            for (int l = 0; l < nWords; l++) {
                free(currentActions[l]);
            }
            countTry++;
        }
        if (currentDistance < bestDistance) {
            summary = pointerReader(currentActions, nWords, lenSet, lenCurrentSeed);
            for (int l = 0; l < nWords; l++) {
                free(currentActions[l]);
            }
            orderActions = orderList(summary, lenSet, lenCurrentSeed, currentSeed, freemanWeights);
            bestDistance = currentDistance;
            bestSeed = currentSeed;
            cout << currentSeed << endl << "distancia promedio" << (float)bestDistance / nWords << endl;
        }
        

    }
    auto endTime = chrono::high_resolution_clock::now();
    // Calculating total time taken by the program.
    double time_taken = chrono::duration_cast<chrono::nanoseconds>(endTime - startTime).count();
    time_taken *= 1e-9;
    cout << "Time taken by program is : " << fixed << time_taken << setprecision(9) << "sec" << endl;
    cout << bestSeed << endl;
    cout << (float)bestDistance / nWords << endl;

    outfile << "Mean AvgDist: " << fixed << setprecision(5) << (float)bestDistance / nWords << " TotalDist: " << totalDist << endl;
    outfile << letter << " " << bestSeed << endl;
    outfile << "Time taken by program is : " << fixed << time_taken << setprecision(9) << "sec" << endl;
    for (record& rec : records)
    {
        outfile << fixed << rec.nOp << " " << rec.pos << " " << rec.type << " " << rec.sVal << " " << rec.currentDist << " " << rec.newDist << " " << rec.totalDist << " " << rec.pTry << " " << rec.currentAvgDist << " " << rec.newAvgDist << endl;
    }

    outfile.close();
    cudaFree(dev_weights);
    cudaFree(dev_wordsTargets);
    cudaFree(dev_wordsTargetsPos);
    return 0;


}

tuple <vector<char>, vector<int>> getWords(string letter)
{
    vector<char> wordsTargets;
    vector<int> wordsTargetsPos;

    int count, position_count = 0;
    string w;
    string line;
    ifstream myfile("./Data/" + folder + "/Data/" + letter);
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            count = 0;
            wordsTargetsPos.push_back(position_count + count);
            w = line.substr(2);
            //cout << w << endl;
            while (w[count] != '\0')
            {
                wordsTargets.push_back(w[count]);
                count++;
            }
            position_count = position_count + count;
            wordsTargetsPos.push_back(position_count);

        }
        myfile.close();
    }
    wordsTargets.push_back('\0');


    return make_tuple(wordsTargets, wordsTargetsPos);
}

tuple <int*, size_t > loadWeights(int* weights, size_t lenSet) {
    int* dev_weights;
    size_t pitchWeights;
    size_t host_weights = lenSet * sizeof(int);
    // Selecciona una GPU
    gpuErrchk(cudaSetDevice(0));
    // Reserva memoria en Cuda
    gpuErrchk(cudaMallocPitch((void**)&dev_weights, &pitchWeights, lenSet * sizeof(int), lenSet));
    // Carga los pesos de freeman
    gpuErrchk(cudaMemcpy2D(dev_weights,// Direccion del buffer
        pitchWeights, // pitch de la memoria
        weights, // Matriz original
        host_weights, // pitch del host
        lenSet * sizeof(int), // width
        lenSet, // height
        cudaMemcpyHostToDevice));
    return make_tuple(dev_weights, pitchWeights);

}

int createSeed(int* wordsTargetsPos, char* dev_wordsTargets, int* dev_wordsTargetsPos, char* wordSource, int lenWordSource, int* dev_weights, size_t lenSet, size_t pitchWeights, int nWords)
{

    char* dev_wordSource = NULL;
    int* dev_distances = NULL;
    //cout << "este es la transformaci�n de la palabra " << wordSource << " || " << lenWordSource << endl;

    int** dev_matrix;

    size_t* pitchMatrix; // Array que contendra los pitch de cada matriz
    int* distances = (int*)malloc(nWords * sizeof(int));

    int len = NULL;

    // Reserva memoria para los punteros de las matrices mappeando y entregando acceso directamente
    gpuErrchk(cudaHostAlloc((void**)&dev_matrix, nWords * sizeof(int*), cudaHostAllocMapped));

    // Reserva memoria para los punteros de las matrices mappeando y entregando acceso directamente
    gpuErrchk(cudaHostAlloc((void**)&pitchMatrix, nWords * sizeof(size_t), cudaHostAllocMapped));



    //cout << "valores pith" << endl;
    // Reserva la memoria para cada matriz en la gpu
    // Si la transformaci�n de palabra es al reves pitchMatrix deberia ser un array que contiene los puntos a los pith 
    for (int x = 0; x < nWords; x++) {
        len = wordsTargetsPos[(x * 2) + 1] - wordsTargetsPos[x * 2];
        gpuErrchk(cudaMallocPitch((void**)&dev_matrix[x], &pitchMatrix[x], (len + 1) * sizeof(int), lenWordSource + 1));

    }

    // Asigna buffer para guardar las distancias generadas
    gpuErrchk(cudaMalloc((void**)&dev_distances, nWords * sizeof(int)));



    // Asigna b�feres de GPU para 1 vector de wordSource.
    gpuErrchk(cudaMalloc((void**)&dev_wordSource, lenWordSource * sizeof(int)));


    // Copia el vector wordSource a la gpu
    gpuErrchk(cudaMemcpy(dev_wordSource, wordSource, lenWordSource * sizeof(char), cudaMemcpyHostToDevice));

    // Launch a kernel on the GPU with one thread for each element.

    getDistanceKernel <<<sizeData/threadPerBlock, threadPerBlock >>> (dev_wordsTargets,
        dev_wordsTargetsPos,
        dev_wordSource,
        lenWordSource,
        pitchMatrix,
        dev_matrix,
        dev_weights,
        lenSet,
        pitchWeights,
        dev_distances,
        nWords);

    // Check for any errors launching the kernel
    gpuErrchk(cudaPeekAtLastError());

    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    gpuErrchk(cudaDeviceSynchronize());


    // Copia la memoria de distancias
    gpuErrchk(cudaMemcpy(distances, dev_distances, nWords * sizeof(int), cudaMemcpyDeviceToHost));

    int sum = 0;
    for (int x = 0; x < nWords; x++)
    {
        
        sum += distances[x];
        gpuErrchk(cudaFree(dev_matrix[x]));
    }
    //cout << sum << " ";
    //cout << endl;
    //cout << "suma final: " << sum << endl;
    gpuErrchk(cudaFree(dev_wordSource));
    gpuErrchk(cudaFreeHost(pitchMatrix));
    gpuErrchk(cudaFree(dev_distances));
    free(distances);
    return sum;

}

void levenshtein(int** actions, int* wordsTargetsPos, char* dev_wordsTargets, int* dev_wordsTargetsPos, char* wordSource, int lenWordSource, int* dev_weights, size_t lenSet, size_t pitchWeights,int *totalDist, int nWords, int* currentDistance)
{
    char* dev_wordSource = NULL;
    int** dev_matrix;
    int** dev_actions;

    (*currentDistance) = 0;
    /*
    cudaEvent_t startEvent, stopEvent, dummyEvent;
    gpuErrchk(cudaEventCreate(&startEvent));
    gpuErrchk(cudaEventCreate(&stopEvent));
    gpuErrchk(cudaEventCreate(&dummyEvent));
    gpuErrchk(cudaEventRecord(startEvent, 0));
    float ms; // elapsed time in milliseconds
    */
    
    
    size_t* pitchMatrix = (size_t*)malloc(nWords * sizeof(size_t)); // Array que contendra los pitch de cada matriz
    
    int len = NULL;
    // Selecciona una GPU

    // Reserva memoria para los punteros de las matrices mappeando y entregando acceso directamente
    gpuErrchk(cudaHostAlloc((void**)&dev_matrix, nWords * sizeof(int*), cudaHostAllocMapped));

    // Reserva memoria para las acciones de cada matriz mappeando y entregando acceso directamente

    gpuErrchk(cudaHostAlloc((void**)&dev_actions, nWords * sizeof(int*), cudaHostAllocMapped));

    // Reserva memoria para los punteros de las matrices mappeando y entregando acceso directamente
    gpuErrchk(cudaHostAlloc((void**)&pitchMatrix, nWords * sizeof(size_t), cudaHostAllocMapped));

    //cout << "valores pith" << endl;
    // Reserva la memoria para cada matriz en la gpu
    // Si la transformaci�n de palabra es al reves pitchMatrix deberia ser un array que contiene los puntos a los pith 
    for (int x = 0; x < nWords; x++) {
        len = wordsTargetsPos[(x * 2) + 1] - wordsTargetsPos[x * 2];
        gpuErrchk(cudaMallocPitch((void**)&dev_matrix[x], &pitchMatrix[x], (len + 1) * sizeof(int), lenWordSource + 1));
        // Reserva memoria para las acciones en cada combinaci�n
        actions[x] = (int*)malloc((((len + lenWordSource)*3) + 2) * sizeof(int));
        gpuErrchk(cudaMalloc((void**)&dev_actions[x], ((((len - 1) + (lenWordSource - 1)) * 3) + 2) * sizeof(int))); // Se suma 2, uno es de la distancia y el otro es para el "-1" que indica el final
    }
    //cout << endl;








    // Asigna b�feres de GPU para 1 vector de wordSource.
    gpuErrchk(cudaMalloc((void**)&dev_wordSource, lenWordSource * sizeof(int)));

    // Copia el vector wordSource a la gpu
    gpuErrchk(cudaMemcpy(dev_wordSource, wordSource, lenWordSource * sizeof(char), cudaMemcpyHostToDevice));

    // Launch a kernel on the GPU with one thread for each element.
    levenshteinKernel <<<sizeData / threadPerBlock, threadPerBlock >>> (dev_wordsTargets,
        dev_wordsTargetsPos,
        dev_wordSource,
        lenWordSource,
        pitchMatrix,
        dev_matrix,
        dev_actions,
        dev_weights,
        lenSet,
        pitchWeights,
        nWords);

    // Check for any errors launching the kernel
    gpuErrchk(cudaPeekAtLastError());

    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    gpuErrchk(cudaDeviceSynchronize());


    for (int x = 0; x < nWords; x++) {
        len = wordsTargetsPos[(x * 2) + 1] - wordsTargetsPos[x * 2];
        (*totalDist) += 1;
        gpuErrchk(cudaMemcpy(actions[x], dev_actions[x], ((((len - 1) + (lenWordSource - 1)) * 3) + 2) * sizeof(int), cudaMemcpyDeviceToHost));
        (*currentDistance) += actions[x][0];
        gpuErrchk(cudaFree(dev_matrix[x]));
        gpuErrchk(cudaFree(dev_actions[x]));
    } 

    /*
    gpuErrchk(cudaEventRecord(stopEvent, 0));
    gpuErrchk(cudaEventSynchronize(stopEvent));
    gpuErrchk(cudaEventElapsedTime(&ms, startEvent, stopEvent));
    gpuErrchk(cudaEventDestroy(startEvent));
    gpuErrchk(cudaEventDestroy(stopEvent));
    gpuErrchk(cudaEventDestroy(dummyEvent))
    printf("execute (ms): %f\n", ms);
    */
    
    // Copy la lista de acciones de cada 
    gpuErrchk(cudaFree(dev_wordSource));
    gpuErrchk(cudaFreeHost(pitchMatrix));
    gpuErrchk(cudaFreeHost(dev_matrix));
    gpuErrchk(cudaFreeHost(dev_actions));
;

}


tuple <char*, int* > loadData(char* wordsTargets, int* wordsTargetsPos, size_t lenWordsTargets, size_t lenWordsTargetsPos) {
    char* dev_wordsTargets = NULL;
    int* dev_wordsTargetsPos = NULL;


    // Asigna b�feres de GPU para 1 vector de wordsTargets.
    gpuErrchk(cudaMalloc((void**)&dev_wordsTargets, lenWordsTargets * sizeof(char)));

    // Asigna b�feres de GPU para 1 vector de wordsTargetsPos.
    gpuErrchk(cudaMalloc((void**)&dev_wordsTargetsPos, lenWordsTargetsPos * sizeof(int)));

    // Copia el vector wordsTargets a la gpu
    gpuErrchk(cudaMemcpy(dev_wordsTargets, wordsTargets, lenWordsTargets * sizeof(char), cudaMemcpyHostToDevice));

    // Copia el vector de wordsTargetsPos a la gpu
    gpuErrchk(cudaMemcpy(dev_wordsTargetsPos, wordsTargetsPos, lenWordsTargetsPos * sizeof(int), cudaMemcpyHostToDevice));

    return make_tuple(dev_wordsTargets, dev_wordsTargetsPos);

}

