#include "OperatorList.h"

/* Insert = 0,Delete =1,Swap = 2*/
/*Expected input [Position Index ,Operation,SigmaValue]*/
/*Expected return [Cuantity,Index position,Operation(0,1,2),SigmaValue]*/
/*Sigma [0,1,2,3,4,5,6,7*/


int** pointerReader(int** pointersArray, const int datasetSize, const int sigmaSize, const int seedLengh)
{
    int** matrix = new int* [(sigmaSize * 2) + 1];
    int sum = 0;
    float average;
    /*fill the matrix with 0*/
    for (int i = 0; i < (sigmaSize * 2) + 1; i++) {
        matrix[i] = new int[seedLengh];
    }
    for (int i = 0; i < (sigmaSize * 2) + 1; i++) {
        for (int j = 0; j < seedLengh; j++) {
            matrix[i][j] = 0;
        }
    }
    /*Traverse an add operations to indexs*/
    for (int i = 0; i < datasetSize; i++)
    {
        int* mesh = pointersArray[i];
        sum += mesh[0];
        
        int j = 1; //J initialize at 1 because index 0 contains the distances value and had jumps of 3[position index,operation,sigma]
        while (mesh[j] != -1) {
            matrix[mesh[(j + 1)] * sigmaSize + mesh[j + 2]][mesh[j]]++;
            j += 3;
            /* for(int x=0;x<seedLengh;x++){
                 /*cout<<matrix[9][x]<<" ";
             }
             cout<<"\n"<<"";
              */
        }
    }
    //travel the array
    /*
    for (int i = 0; i < (sigmaSize * 2) + 1; i++) {
        for (int j = 0; j < seedLengh; j++) {
            cout << matrix[i][j] << " ";
        }
        cout << endl;
    }
    */
    return matrix;
}

/*Funcion que recibe como parametro una matriz del tipo nxm en donde las filas del indice 0-7 representan las inserciones,7-15 sustituciones y 16-23 eliminaciones, a su ves dentro de cada grupo de 8 representan los 8 caracteres posibles para este sigma
y finalmente el tamaño de las columnas representa el largo de la semilla */
vector<operatorVector> orderList(int** operationsMatrix, const int sigmaSize, const int seedLengh,char* seed, int* weights) {
    vector<operatorVector> list;
    operatorVector temp;
    int operation;
    int weight;
    for (int i = 0; i < (sigmaSize * 2) + 1; i++) {
        for (int j = 0; j < seedLengh; j++) {
            if (i <= 7) {
                operation = 0;
            }
            else if (i > 7 && i <= 15) {
                operation = 1;
            }
            else if (i > 15) {
                operation = 2;
            }
            if (operationsMatrix[i][j] != 0) {
                
                if (operation == 1) {
                    weight = weights[(sigmaSize * ((int)seed[j] - '0')) + (i - (operation * 8))];
                    temp.nOp = operationsMatrix[i][j]*weight;
                }
                else {
                    temp.nOp = operationsMatrix[i][j]* 2;
                }
                temp.pos = j;
                temp.type = operation;
                temp.sVal = i - (operation * 8);


                list.push_back(temp);
            }
        }
    }
    sort(list.begin(), list.end(), compare);
    /*
    for (operatorVector& x : list)
    {

        cout << x.nOp << " " << x.pos << " " << x.type << " " << x.sVal <<  endl;
    }
    */
    return list;
}

bool compare(const operatorVector &a, const operatorVector &b) {
    return a.nOp > b.nOp;
}