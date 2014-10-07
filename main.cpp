#include <iostream>
#include <time.h>
#include <cstdlib>
#include <cstdio>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

class Vector {
public:
    int size;
    int* value;

    Vector() {
        size = 0;
    }

    explicit Vector(int size) {
        this->size = size;
        value = new int[size];
        memset(value, 0, sizeof(int) * size);
    }

    void load(char* file) {
        FILE* input = fopen(file, "r");
        fscanf(input, "%d ", &size);
        value = new int[size];
        for (int i = 0; i < size; i++) {
            fscanf(input, "%d ", &(value[i]));
        }
        fclose(input);
    }

    void print() {
        for(int i = 0; i < size; i++) {
            std::cout << value[i] << ' ' ;
        }
        std::cout << std::endl << std::endl;
    }

    void generate(int max) {
        for (int i = 0; i < size; i++) {
            value[i] = rand() % max;
        }
    }

    ~Vector() {
        delete[] value;
    }
};

class TransposedMatrix {
public:
    int raw;
    int column;
    int** value;
    TransposedMatrix(int raw, int column) {
        this->raw = raw;
        this->column = column;
         value = new int*[column];
         for (int i = 0; i < column; i++) {
             value[i] = new int[raw];
         }
    }
    void print() {
        for (int i = 0; i < column; i++) {
            for (int j = 0; j < raw; j++) {
                std::cout << value[i][j] << ' ';
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    void generate(int max, int prob) {
        for (int i = 0; i < column; i++) {
            for (int j = 0; j < raw; j++) {
                if (rand() % prob == 0) {
                    value[i][j] = rand() % max;
                } else {
                    value[i][j] = 0;
                }
            }
        }
    }
    ~TransposedMatrix() {
        for (int i = 0; i < column; i++) {
            delete[] value[i];
        }
        delete[] value;
    }
};

class RareMatrix {
public:
    int** value;
    int column;
    RareMatrix(const TransposedMatrix& matrix) {
        column = matrix.column;
        value = new int*[column];
        for (int i = 0; i < column; i++) {
            int count = 0;
            for (int j = 0; j < matrix.raw; j++) {
                if (matrix.value[i][j] != 0) {
                    count++;
                }
            }
            value[i] = new int[2 * count + 1];//number of non-0 values + pairs {raw: value}
            value[i][0] = count;
            for (int j = 0, k = 1; j < matrix.raw; j++) {
                if (matrix.value[i][j] != 0) {
                    value[i][k] = j;
                    value[i][k + 1] = matrix.value[i][j];
                    k += 2;
                }
            }
        }
    }

    Vector* mul(Vector* v) {
        Vector* res = new Vector(column);
        for(int i = 0; i < column; i++) {
            int num = value[i][0];
            for (int j = 1; j < 2 * num + 1; j += 2) {
                int raw = value[i][j];
                int val = value[i][j + 1];
                res->value[raw] += val * v->value[i];
            }
        }
        return res;
    }

    void print() {
        for (int i = 0; i < column; i++) {
            for (int j = 0; j < 2 * value[i][0] + 1; j++) {
                std::cout << value[i][j] << ' ';
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    ~RareMatrix() {
        for (int i = 0; i < column; i++) {
            delete[] value[i];
        }
        delete[] value;
    }
};

struct Data {
    bool end;
    int from;
    int to;
};

Data* data;
RareMatrix* matrix;
Vector* v;
Vector* res;

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex4 = PTHREAD_MUTEX_INITIALIZER;

void* slave(void* param) {
    int index = *((int*)param);
    Data d;
    do {
        //only 1 slave can ask for job
        pthread_mutex_lock(&mutex3);
        std::cout << "slave " << index << " locked mutex3\n";
        std::cout << "slave " << index << " signales cond1\n";
        //ask for job
        pthread_cond_signal(&cond1);
        std::cout << "slave " << index << " wait cond2\n";
        pthread_cond_wait(&cond2, &mutex2);
        std::cout << "slave " << index << " waited cond2\n";

        //copy data
        d.end = data->end;
        d.from = data->from;
        d.to = data->to;
        std::cout << "slave " << index << " copied data\n";

        pthread_mutex_unlock(&mutex2);
        std::cout << "slave " << index << " unlocked mutex2\n";
        pthread_mutex_unlock(&mutex3);
        std::cout << "slave " << index << " unlocked mutex3\n";

        if (!d.end) {
            std::cout << "slave " << index << " calcs\n";
            //calculate
            Vector localResult(matrix->column);
            for(int i = d.from; i < d.to; i++) {
                int num = matrix->value[i][0];
                for (int j = 1; j < 2 * num + 1; j += 2) {
                    int raw = matrix->value[i][j];
                    int val = matrix->value[i][j + 1];
                    localResult.value[raw] += val * v->value[i];
                }
            }

            //write result
            pthread_mutex_lock(&mutex4);
            std::cout << "slave " << index << " locked mutex4\n";
            for (int i = 0; i < res->size; i++) {
                res->value[i] += localResult.value[i];
            }
            std::cout << "slave " << index << " write res\n";
            std::cout << "slave " << index << " unlocked mutex4\n";
            pthread_mutex_unlock(&mutex4);
        }
    } while(!d.end);
    return 0;
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

    TransposedMatrix* tMatrix = new TransposedMatrix(size, size);
    tMatrix->generate(max, 5);
    std::cout << "tr matrix:\n";
    tMatrix->print();
    matrix = new RareMatrix(*tMatrix);
    std::cout << "matrix:\n";
    matrix->print();
    delete tMatrix;
     v = new Vector(size);
    v->generate(max);
    std::cout << "vector:\n";
    v->print();
    data = new Data();
    res = new Vector(v->size);

    pthread_t* thread = new pthread_t[threadAmount];
    for (int i = 0; i < threadAmount; i++) {
        if (pthread_create(thread + i, NULL, slave, &i) != 0) {
            std::cout << "thread create error\n";
        }
    }

    //master
    int last = 0;
    while (last < matrix->column) {
        pthread_cond_wait(&cond1, &mutex1);
        std::cout << "master waited cond1\n";
        if (last < matrix->column) {
            data->from = last;
            data->to = last + blockSize;
            if (data->to > matrix->column) {
                data->to = matrix->column;
            }
            last = data->to;
            data->end = false;
        } else {
            data->end = true;
        }
        std::cout << "writen\n";
        std::cout << "master unlocked mutex1\n";
        pthread_mutex_unlock(&mutex1);
        std::cout << "master signaled cond2\n";
        pthread_cond_signal(&cond2);
    }
    std::cout << "master wait for threads\n";
    for (int i = 0; i < threadAmount; i++) {
        pthread_cond_wait(&cond1, &mutex1);
        data->end = true;
        pthread_cond_signal(&cond2);
        pthread_mutex_unlock(&mutex1);
    }
    for (int i = 0; i < threadAmount; i++) {
        pthread_join(thread[i], NULL);
    }
    //end master

//    pthread_mutex_lock(&mutex4);
//    std::cout << "master locked mutex4\n";
    std::cout << "res:\n";
    res->print();
    std::cout << "master unlocked mutex4\n";
//    pthread_mutex_unlock(&mutex4);

    pthread_cond_destroy(&cond1);
    pthread_cond_destroy(&cond2);
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    pthread_mutex_destroy(&mutex3);
    pthread_mutex_destroy(&mutex4);

    delete data;
    delete matrix;
    delete v;
    delete res;

    return 0;
}

