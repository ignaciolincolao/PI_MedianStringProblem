
#include "StringEditor.h"
#include "OperatorList.h"
#include "levenshtein.h"
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


using namespace std;
struct record {
    int nOp; // Numero de veces * el weight correspondiente
    int pos; // Posición donde se realizara el cambio en la semilla actual
    int type; // El tipo de operación 0->Insertado 1->Sustitución 2->Borrado
    int sVal; // Caracter que se insertara o reemplazara
    int currentDist; // Distancias acumuladas entre todas las cadenas
    int newDist;// Distancia acumulada con la nueva semilla
    int totalDist; // Numero de distancias revisadas desde que se ejecuto el codigo
    int pTry; // La posición en la que se encontraba cuando encontro una semilla mejor
    float currentAvgDist; // Actual distancia promedio de la semilla
    float newAvgDist;// Nueva distancia promedio de la semilla
};


tuple <vector<char>, vector<int>> getWords(string letter);
void levenshtein(int** actions, char* wordsTargets, int* wordsTargetsPos, char* wordSource, size_t lenWordsTargetsPos, int lenWordSource, int* freemanWeights, size_t lenSet, int* totalDist, int bestDistance, int* currentDistance);
void createSeed(char* wordsTargets, int* wordsTargetsPos, size_t lenWordsTargetsPos, int* freemanWeights, size_t lenSet, float* seeds, int* totalDist);
record make_record(int nOp, int pos, int type, int sVal, int currentDist, int newDist, int totalDist, int pTry, float currentAvgDist, float newAvgDist);



record make_record(int nOp, int pos, int type,int sVal,int currentDist,int newDist, int totalDist,int pTry,float currentAvgDist,float newAvgDist){
    record myrecord = { nOp, pos, type, sVal, currentDist, newDist,  totalDist, pTry, currentAvgDist, newAvgDist };
    return myrecord;
}

string letter = "O";
string folder = "L360";


int main(int argc, char* argv[])
{

    if (argc > 1) {
        letter = argv[1];
        folder = argv[2];
    }
    vector<record> records;
    int totalDist = 0;
    // Crea archivo para guardar semilla y resultado
    ofstream outfile("Out/"+folder +"/" + letter + ".txt");

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

    wordsTargets = &v_wordsTargets[0];
    const char* set = "01234567";
    size_t lenSet = strlen(set);


    // Extrae la infomación de los pesos de freeman
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

    // Fin extrae información del weights
    /*
    for (int a = 0; a < lenSet * lenSet; a++) {
        cout << " " << freemanWeights[a];
        if ((a + 1) % lenSet == 0) {
            cout << endl;
        }

    }

    for (int a = 0; a < lenWordsTargetsPos; a++) {
        cout << " " << wordsTargetsPos[a];
        if (a % 2 != 0) {
            cout << endl;
        }
    }

    for (int a = 0; a < lenWordsTargets; a++) {
        cout << wordsTargets[a];
    }
    cout << "\n";
    */

    // Obtiene la semilla
    auto startTime = chrono::high_resolution_clock::now();


    int nWords = (int)lenWordsTargetsPos / 2;
    int init, end;
    char* wordSource;
    int lenWordSource;
    float* seeds = (float*)malloc(nWords * sizeof(float));
    createSeed(wordsTargets, wordsTargetsPos, lenWordsTargetsPos, freemanWeights, lenSet, seeds, &totalDist);

    int minIndex = 0;
    float min = seeds[0];
    for (int x = 0; x < lenWordsTargetsPos / 2; x++)
    {
        if (seeds[x] < min)
        {
            minIndex = x;
            min = seeds[x];
        }
    }
    //cout << minIndex << "||" << min << endl;
    
    cout << "distancia promedio " << endl;
    cout << fixed;
    cout << setprecision(5) << (float)min/nWords << endl;
    init = wordsTargetsPos[minIndex * 2];
    end = wordsTargetsPos[(minIndex * 2) + 1];
    lenWordSource = end - init;
    wordSource = (char*)malloc(lenWordSource + 1 * sizeof(char));
    memcpy(wordSource, &wordsTargets[init], sizeof(char) * lenWordSource);
    wordSource[lenWordSource] = '\0';

    cout << "semilla inicial: " << endl << wordSource << endl;
    cout << "TotalDist: " << totalDist<< endl;
    totalDist = totalDist / 2;
    outfile << "SetMedian AvgDist: " << fixed << setprecision(5) << (float)min / nWords << " TotalDist: " << totalDist << endl;
    outfile << letter << " " << wordSource << endl;
    // Comiensa a calcular 
    cout << "Comienza a calcular distancias" << endl;
    int** currentActions;
    currentActions = (int**)malloc(nWords * sizeof(int*));
    int** summary;
    int bestDistance = min;
    int currentDistance = 0;

    bool lOk = true;
    vector<operatorVector> orderActions;
    string bestSeed(wordSource);
    char* currentSeed = wordSource;
    int lenCurrentSeed = lenWordSource;
    int countTry = 0;
    string temp;
   
    levenshtein(currentActions, wordsTargets, wordsTargetsPos, currentSeed, lenWordsTargetsPos, lenCurrentSeed, freemanWeights, lenSet, &totalDist, bestDistance, &currentDistance);
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
            //cout << bestSeed << endl;
            //cout << temp << endl;
            currentSeed = &temp[0];
            lenCurrentSeed = temp.size();
            //for (int n = 0; n < lenCurrentSeed; n++) {
            //    cout << currentSeed[n];
            //}
            //cout << endl;
            levenshtein(currentActions, wordsTargets, wordsTargetsPos, currentSeed, lenWordsTargetsPos, lenCurrentSeed, freemanWeights, lenSet, &totalDist, bestDistance, &currentDistance);
            if (currentDistance < bestDistance) {
                records.push_back(make_record(x.nOp, x.pos, x.type, x.sVal, bestDistance, currentDistance, totalDist, countTry, (float)bestDistance/nWords, (float)currentDistance / nWords));
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
    free(freemanWeights);

    
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



void createSeed(char* wordsTargets, int* wordsTargetsPos, size_t lenWordsTargetsPos, int* freemanWeights, size_t lenSet, float* seeds, int* totalDist)
{


    int nWords = (int)lenWordsTargetsPos / 2;
    int* distances = (int*)calloc(nWords, sizeof(int));

    int len = NULL;

    // Launch a kernel on the GPU with one thread for each element.
    unsigned int thread = nWords;
    getDistanceKernel(wordsTargets,
        wordsTargetsPos,
        freemanWeights,
        lenSet,
        distances,
        nWords,
        totalDist);

    int sum = 0;
    for (int x = 0; x < nWords; x++)
    {
        //cout << distances[x] << " ";
        seeds[x] = (float)distances[x];
    }


    free(distances);
    return;

}


void levenshtein(int** actions, char* wordsTargets, int* wordsTargetsPos, char* wordSource, size_t lenWordsTargetsPos, int lenWordSource, int* freemanWeights, size_t lenSet, int* totalDist, int bestDistance, int* currentDistance)
{
    int nWords = (int)lenWordsTargetsPos / 2;

    int len = NULL;


    //cout << "valores pith" << endl;
    // Reserva la memoria para cada matriz en la gpu
    // Si la transformación de palabra es al reves pitchMatrix deberia ser un array que contiene los puntos a los pith 
    for (int x = 0; x < nWords; x++) {
        len = wordsTargetsPos[(x * 2) + 1] - wordsTargetsPos[x * 2];
        actions[x] = (int*)malloc(((len * lenWordSource) + 2) * sizeof(int));
    }
    //cout << endl;
    (*currentDistance) = 0;
    levenshteinKernel(wordsTargets,
        wordsTargetsPos,
        wordSource,
        lenWordsTargetsPos,
        lenWordSource,
        actions,
        freemanWeights,
        lenSet,
        totalDist,
        nWords,
        bestDistance,
        currentDistance);
}