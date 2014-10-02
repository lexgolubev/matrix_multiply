#include <iostream>
#include <time.h>
#include <cstdlib>
#include <cstdio>
#include <string.h>

using namespace std;

struct TransposedMatrix {
    int raw;
    int column;
    int** columns;
    TransposedMatrix(int raw, int column) {
        this->raw = raw;
        this->column = column;
         columns = new int*[column];
         for (int i = 0; i < column; i++) {
             columns[i] = new int[raw];
         }
    }
    void print() {
        for (int i = 0; i < column; i++) {
            for (int j = 0; j < raw; j++) {
                std::cout << columns[i][j] << ' ';
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

struct RareMatrix {
    int** columns;
    int column;
    RareMatrix(const TransposedMatrix& matrix) {
        column = matrix.column;
        columns = new int*[column];
        for (int i = 0; i < column; i++) {
            int count = 0;
            for (int j = 0; j < matrix.raw; j++) {
                if (matrix.columns[i][j] != 0) {
                    count++;
                }
            }
            columns[i] = new int[2 * count + 1];//non-0 values + pairs {raw: value}
            columns[i][0] = count;
            for (int j = 0, k = 1; j < matrix.raw; j++) {
                if (matrix.columns[i][j] != 0) {
                    columns[i][k] = j;
                    columns[i][k + 1] = matrix.columns[i][j];
                    k += 2;
                }
            }
        }
    }
    void print() {
        for (int i = 0; i < column; i++) {
            for (int j = 0; j < 2 * columns[i][0] + 1; j++) {
                std::cout << columns[i][j] << ' ';
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

TransposedMatrix* generateMatrix(int raw, int column, int max, int prob) {
    TransposedMatrix* matrix = new TransposedMatrix(raw, column);
    for (int i = 0; i < column; i++) {
        for (int j = 0; j < raw; j++) {
            if (rand() % prob == 0) {
                matrix->columns[i][j] = rand() % max;
            } else {
                matrix->columns[i][j] = 0;
            }
        }
    }
    return matrix;
}

int* generateVector(int num, int max) {
    int* res = new int[num];
    for (int i = 0; i < num; i++) {
        res[i] = rand() % max;
    }
    return res;
}

int* mul(const RareMatrix& matrix, int* v) {
    int* res = new int[matrix.column];
    memset(res, 0, sizeof(int) * matrix.column);
    for(int i = 0; i < matrix.column; i++) {
        int num = matrix.columns[i][0];
        for (int j = 1; j < 2 * num + 1; j += 2) {
            int raw = matrix.columns[i][j];
            int value = matrix.columns[i][j + 1];
            res[raw] += value * v[raw];
        }
    }
    return res;
}

void print(int* v, int size) {
    for(int i = 0; i < size; i++) {
        std::cout << v[i] << ' ' ;
    }
    std::cout << std::endl << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        return 1;
    }
    int size;
    int threadAmount;
    int blockSize;
    int max;
    sscanf(argv[1], "%d", &size);
    sscanf(argv[2], "%d", &threadAmount);
    sscanf(argv[3], "%d", &blockSize);
    sscanf(argv[4], "%d", &max);

    TransposedMatrix* tMatrix = generateMatrix(size, size, max, 10);
    tMatrix->print();
    RareMatrix* matrix = new RareMatrix(*tMatrix);
    matrix->print();
    int* v = generateVector(size, max);
    print(v, size);
    int* res = mul(*matrix, v);
    print(res, size);

    return 0;
}

