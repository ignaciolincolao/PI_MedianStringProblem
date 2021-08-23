#ifndef HEADER_H
#define HEADER_H

#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <iostream>

__device__ int calculateWeight(int* weights, size_t lenSet, size_t pitchWeights, char x, char y);
__global__ void getDistanceKernel(char* wordsTargets, int* wordsTargetsPos, char* wordSource, int lenWordSource, size_t* pitchMatrix, int** matrix, int* weights, size_t lenSet, size_t pitchWeights, int* distances, int nWords);
__global__ void levenshteinKernel(char* wordsTargets, int* wordsTargetsPos, char* wordSource, int lenWordSource, size_t* pitchMatrix, int** matrix, int** actions, int* weights, size_t lenSet, size_t pitchWeights, int nWords);
#endif

