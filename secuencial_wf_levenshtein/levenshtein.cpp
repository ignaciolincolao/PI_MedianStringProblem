#include "levenshtein.h"

int calculateWeight(int* weights, size_t lenSet, char x, char y)
{
    int xx = ((int)x - '0'); // transforma el caracter numero a int
    int yy = ((int)y - '0'); // transforma el caracter numero a int
    //printf("lenset: %d", lenSet);
    //printf("sustitución: %d %d\n", xx, yy);
    //printf("valor de: %d \n", weights[yy * lenSet + xx]);
    return weights[xx * lenSet + yy];
}

void getDistanceKernel(char* wordsTargets, int* wordsTargetsPos, int* weights, size_t lenSet, int* distances,int nWords,int* totalDist)
{
    int* m = NULL;
    int t1, t2, i, j, costo, res, ancho,alto, min, del, in, sus;
    int tidx;
    int len, init;
    int init_s, end_s;
    char* wordSource;
    int lenWordSource;
    for (int s = 0; s < nWords; s++) {
        init_s = wordsTargetsPos[s * 2];
        end_s = wordsTargetsPos[(s * 2) + 1];
        lenWordSource = end_s - init_s;
        wordSource = (char*)malloc(lenWordSource + 1 * sizeof(char));
        memcpy(wordSource, &wordsTargets[init_s], sizeof(char) * lenWordSource);
        wordSource[lenWordSource] = '\0';
        for (int ss = s+1;  ss < nWords; ss++) {
            tidx = ss;
            //printf("idx %d", tidx);
            len = wordsTargetsPos[(tidx * 2) + 1] - wordsTargetsPos[tidx * 2]; // largo de la palabra dentro del wordsTargetsPos
            init = wordsTargetsPos[tidx * 2]; // inicio de la palabra dentro del wordsTargetsPos
            //int end = wordsTargetsPos[(tidx * 2) + 1]; // Fin de la palabra dentro del wordsTargetsPos
            

            /*
            if (tidx == 0) {
                printf("valor de lenSet en cuda y su tabla: %d\n", lenSet);
                for (int r = 0; r < lenSet; r++) {
                    for (int c = 0; c < lenSet; c++) {
                        printf(" %d", weights[r * pitchWeights / sizeof(int) + c]);
                    }
                    printf("\n");
                }
            }
            */
            // Calcula tamanios strings 
            t1 = len;
            t2 = lenWordSource;
            //printf("%d | %d |%d | %d | %d \n", t1, t2, init, end, len);


            // Verifica que exista algo que comparar
            if (t1 == 0) return;
            if (t2 == 0) return;
            //ancho = pitchMatrix[tidx] / sizeof(int);
            ancho = len + 1;
            alto = lenWordSource + 1;
            //printf("pitch %d\n", pitchMatrix[tidx]);
            m = (int*)malloc(alto *ancho*sizeof(int));


            /*
            if (tidx == 0) {
                for (i = 0; i <= t2; i++) {
                    for (j = 0; j <= t1; j++) {
                        printf("%d ", m[i * ancho + j]);
                    }
                    printf("\n");
                }
                printf("fin\n");
            }
            */
            // Rellena primera fila y primera columna
            for (int i = 0; i <= t2; i++) m[i * ancho + 0] = i * 2;
            for (int j = 0; j <= t1; j++) m[j] = j * 2;

            /*
            if (tidx == 0) {
                for (i = 0; i <= t2; i++) {
                    for (j = 0; j <= t1; j++) {
                        printf("%d ", m[i * ancho + j]);
                    }
                    printf("\n");
                }
                printf("fin\n");
            }
            */
            // Recorremos resto de la matriz llenando pesos
            for (i = 1; i <= t2; i++) for (j = 1; j <= t1; j++)
            {
                //printf("%d | %d \n", i, j);
                //printf("%c | %c \n", wordSource[i - 1], wordsTargets[(init + j) - 1]);
                if (wordSource[i - 1] == wordsTargets[(init + j) - 1]) costo = 0; else costo = calculateWeight(weights, lenSet, wordSource[i - 1], wordsTargets[(init + j) - 1]);
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
            /*
            if (tidx == 0) {
                printf("Matriz final\n");
                for (i = 0; i <= t2; i++) {
                    for (j = 0; j <= t1; j++) {
                        printf("%d ", m[i * ancho + j]);
                    }
                    printf("\n");
                }
                printf("fin Matriz\n");
            }
            */ 
            //printf("resultado final es ---> %d\n", res);
            distances[tidx]+= res;
            distances[s]+= res;
            //printf("resultado distances final es ---> %d\n", distances[tidx]);
            (*totalDist)+= 1;
            free(m);
        }   
        cout << s << endl;
    }

    return;

}

void levenshteinKernel(char* wordsTargets, int* wordsTargetsPos, char* wordSource, size_t lenWordsTargetsPos, int lenWordSource, int** actions, int* weights, size_t lenSet, int* totalDist, int nWords, float bestDistance, int* currentDistance)
{
    int* m = NULL;
    int t1, t2, i, j, costo, ancho, min, del, in, sus, alto, tidx,len,init;
    int del_t, del_w, ins_t, ins_w, sus_t, sus_w, x, y, count;
    int* listActions= NULL;
    int postarget, posSource, wt, w;
    for (int s = 0; s < nWords; s++) {
        tidx = s;
        len = wordsTargetsPos[(tidx * 2) + 1] - wordsTargetsPos[tidx * 2]; // largo de la palabra dentro del wordsTargetsPos
        init = wordsTargetsPos[tidx * 2]; // inicio de la palabra dentro del wordsTargetsPos
        //int end = wordsTargetsPos[(tidx * 2) + 1]; // Fin de la palabra dentro del wordsTargetsPos
        


        /*
        if (tidx == 0) {
            printf("valor de lenSet en cuda y su tabla: %d\n", lenSet);
            for (int r = 0; r < lenSet; r++) {
                for (int c = 0; c < lenSet; c++) {
                    printf(" %d", weights[r * pitchWeights / sizeof(int) + c]);
                }
                printf("\n");
            }
        }
        */
        // Calcula tamanios strings 
        t1 = len;
        t2 = lenWordSource;
        //printf("%d | %d |%d | %d | %d \n", t1, t2, init, end, len);


        // Verifica que exista algo que comparar
        if (t1 == 0) return;
        if (t2 == 0) return;
        //ancho = pitchMatrix[tidx] / sizeof(int);
        ancho = len + 1;
        alto = lenWordSource + 1;
        //printf("pitch %d\n", pitchMatrix[tidx]);
        m = (int*)malloc(alto * ancho * sizeof(int));



        // Rellena primera fila y primera columna
        for (i = 0; i <= t2; i++) m[i * ancho + 0] = i * 2;
        for (j = 0; j <= t1; j++) m[j] = j * 2;

        /*
        if (tidx == 0) {
            for (i = 0; i <= t2; i++) {
                for (j = 0; j <= t1; j++) {
                    printf("%d ", m[i * ancho + j]);
                }
                printf("\n");
            }
            printf("fin\n");
        }
        */
        // Recorremos resto de la matriz llenando pesos
        for (i = 1; i <= t2; i++) for (j = 1; j <= t1; j++)
        {
            //printf("%d | %d \n", i, j);
            //printf("%c | %c \n", wordSource[i - 1], wordsTargets[(init + j) - 1]);
            if (wordSource[i - 1] == wordsTargets[(init + j) - 1]) costo = 0; else costo = calculateWeight(weights, lenSet, wordSource[i - 1], wordsTargets[(init + j) - 1]);
            // Calcula el minimo 
            del = m[i * ancho + j - 1] + 2; // El mas dos es el costo de borrado
            in = m[(i - 1) * ancho + j] + 2; // El mas dos es el costo de inserción
            sus = m[(i - 1) * ancho + j - 1] + costo; // Costo de sustitución
            min = del < in ? del : in;
            min = min < sus ? min : sus;
            // guarda en la matriz el minimo
            m[i * ancho + j] = min;
        }

        /*
        if (tidx == 0) {
            printf("Matriz final\n");
            for (i = 0; i <= t2; i++) {
                for (j = 0; j <= t1; j++) {
                    printf("%d ", m[i * ancho + j]);
                }
                printf("\n");
            }
            printf("fin Matriz\n");
        }
        */
        // t1 la palabra original --> [][x]
        // t2 la palabra a la que se va a cambiar [y][]
        listActions = actions[tidx];
        x = t1;
        y = t2;
        count = 1;
        listActions[0] = m[t2 * ancho + t1];
        (*currentDistance) += listActions[0];
        //if (tidx == 0) {
        while (x != 0 || y != 0)
        {
            postarget = x - 1 < 0 ? 0 : x - 1; // La palabra dinamica
            posSource = y - 1 < 0 ? 0 : y - 1; // La palabra constante
            wt = wordSource[y - 1];
            w = wordsTargets[(init + x) - 1];

            del_w = (y > 0) ? 2 : INT_MAX;
            del_t = (y > 0) ? m[posSource * ancho + x] + del_w : INT_MAX; // Borrado

            ins_w = (x > 0) ? 2 : INT_MAX;
            ins_t = (x > 0) ? m[y * ancho + postarget] + ins_w : INT_MAX; // inserrción

            sus_w = (x > 0 && y > 0) ? calculateWeight(weights, lenSet, wt, w) : INT_MAX;
            sus_t = (x > 0 && y > 0) ? m[posSource * ancho + postarget] + sus_w : INT_MAX;

            //printf("\n%d %d %d %d| del ins sus xy0\n", del_t, ins_t, sus_t, m[y * ancho + x]);
            //printf("Distancia %d \n", m[t2 * ancho + t1]);

            if (sus_t == m[y * ancho + x])
            {
                if (wt != w) {
                    listActions[count] = posSource;
                    listActions[count + 1] = 1;
                    listActions[count + 2] = ((int)w - '0');
                    //printf("susticion de %d por %d\n", ((int)wt - '0'), ((int)w - '0'));
                    count += 3;
                }
                y--;
                x--;
            }
            else if (ins_t == m[y * ancho + x])
            {
                listActions[count] = posSource;
                listActions[count + 1] = 0;
                listActions[count + 2] = ((int)w - '0');
                //printf("insercion de %d\n", ((int)w - '0'));
                x--;
                count += 3;
            }
            else if (del_t == m[y * ancho + x])
            {
                listActions[count] = posSource;
                listActions[count + 1] = 2;
                listActions[count + 2] = NULL;
                //printf("borrado de %d\n", ((int)wt - '0'));
                y--;
                count += 3;
            }
            else
            {
                //printf("error en la lista de operaciones");
                break;
            }

        }
        listActions[count] = -1;
        free(m);
        (*totalDist) += 1;
        if ((*currentDistance) > bestDistance){
            listActions[0] = (*currentDistance);
            break;
        }
    }

    return;
}
