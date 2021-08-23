#ifndef LEVENSHTEIN_H
#define LEVENSHTEIN_H

#include <iostream>
using namespace std;

int calculateWeight(int* weights, size_t lenSet, char x, char y);
void getDistanceKernel(char* wordsTargets, int* wordsTargetsPos, int* weights, size_t lenSet, int* distances, int nWords, int *totalDist);
void levenshteinKernel(char* wordsTargets, int* wordsTargetsPos, char* wordSource, size_t lenWordsTargetsPos, int lenWordSource, int** actions, int* weights, size_t lenSet, int* totalDist, int nWords, float bestDistance, int* currentDistance);
#endif

