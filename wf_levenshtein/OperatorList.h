#ifndef OPERATORLIST_H
#define OPERATORLIST_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <tuple>
using namespace std;

struct operatorVector
{
    int nOp;
    int pos;
    int type;
    int sVal;
};
bool compare(const operatorVector &a, const operatorVector &b);
int** pointerReader(int** pointersArray, const int datasetSize, const int sigmaSize, const int seedLengh);
vector<operatorVector> orderList(int** operationsMatrix, const int sigmaSize, const int seedLengh, char* seed, int* weights);

#endif

