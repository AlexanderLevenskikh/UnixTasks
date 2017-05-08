#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// stl and other cpp libs
#include <iostream>
#include <vector>

#define ROWS_COUNT 4
#define BLOCK_ROWS_COUNT 2

void err_sys(char *);

struct matrix {
    std::vector<std::vector<int> > matrix_data;
    int vector_size;

    matrix() {
        vector_size = ROWS_COUNT;
    }

    matrix(int n) {
        vector_size = n;
    }

    void copy_matrix(matrix &other_matrix) {

        for (int i = 0; i < vector_size; i++) {
            std::vector<int> current_vector;
            matrix_data.push_back(current_vector);

            for (int j = 0; j < vector_size; j++)
                matrix_data[i].push_back(other_matrix.matrix_data[i][j]);
        }

    }

    void parse_file(char *filename) {
        FILE *fp;
        if ((fp = fopen(filename, "r")) == NULL) {
            char msg[100];
            snprintf(msg, 100*sizeof(char), "Can't open file %s.\n", filename);
            err_sys(msg);
        }

        for (int i = 0; i < vector_size; i++) {
            std::vector<int> current_vector;
            matrix_data.push_back(current_vector);
            int current_number;
            for (int j = 0; j < vector_size-1; j++) {
                fscanf(fp, "%d ", &current_number);
                matrix_data[i].push_back(current_number);
            }
            fscanf(fp, "%d\n", &current_number);
            matrix_data[i].push_back(current_number);

        }

        fclose(fp);
    }

    void init_matrix() {
        for (int i = 0; i < vector_size; i++) {
            std::vector<int> current_vector;
            matrix_data.push_back(current_vector);
            for (int j = 0; j < vector_size; j++)
             matrix_data[i].push_back(0);
        }
    }

    void print_matrix() {
        for (int i = 0; i < vector_size; i++) {
            for (int j = 0; j < vector_size; j++)
                std::cout<<matrix_data[i][j]<<" ";
            std::cout<<"\n";
        }
    }

    matrix get_matrix_block(int row, int column) {
        matrix result = matrix(BLOCK_ROWS_COUNT);
        result.init_matrix();

        for (int i = 0; i < BLOCK_ROWS_COUNT; i++)
        for (int j = 0; j < BLOCK_ROWS_COUNT; j++)
            result.matrix_data[i][j] = matrix_data[row + i][column + j];

        return result;
    }

    matrix multiply(matrix other_matrix) {
        matrix result = matrix(vector_size);
        result.init_matrix();

        for (int i = 0; i < vector_size; i++)
        for (int j = 0; j < vector_size; j++) {
            int sum = 0;
            for (int k = 0; k < vector_size; k++)
                sum += matrix_data[i][k] * matrix_data[k][j];
            result[i][j] = sum;
        }

        return result;
    }
};

int main(int argc, char **argv) {
    char *firstMatrixFileName, *secondMatrixFileName;

    switch(argc) {
        case 2:
            firstMatrixFileName = argv[1];
            secondMatrixFileName = argv[1];
            break;
        case 3:
            firstMatrixFileName = argv[1];
            secondMatrixFileName = argv[2];
            break;
        default:
            err_sys("Incorrect arguments format. Usage: ./pr file1 [file2]");
    }

    matrix first_matrix = matrix(), second_matrix = matrix();
    first_matrix.parse_file(firstMatrixFileName);

    if (argc == 2) {
        second_matrix.copy_matrix(first_matrix);
    } else {
        second_matrix.parse_file(secondMatrixFileName);
    }


}

void err_sys(char *msg) {
    perror(msg);
    exit(1);
}
