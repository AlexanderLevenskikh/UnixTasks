#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

// stl and other cpp libs
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#define ROWS_COUNT 4
#define BLOCK_ROWS_COUNT 2
#define MAX_NUMBER_LENGTH 10

#define BUFFER_SIZE 4096

int pipes[ROWS_COUNT];

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

    std::string serialize_block() {
        std::stringstream ss;

        for (int i = 0; i < vector_size; i++) {
            for (int j = 0; j < vector_size; j++) {
                if(i != 0 || j != 0) {
                    ss << " ";
                }
                ss << matrix_data[i][j];
            }
        }

        return ss.str();
    }

    matrix deserialize_block(std::string str) {
        matrix result = matrix(BLOCK_ROWS_COUNT);
        result.init_matrix();

        std::string buf;
        std::stringstream ss(str);

        std::vector<std::string> tokens; // Create vector to hold our words

        while (ss >> buf)
            tokens.push_back(buf);

        int i = 0, j = 0, value;

        for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++) {
            std::istringstream iss(*it);
            iss >> value;
            result.matrix_data[i][j] = value;
            if (j == BLOCK_ROWS_COUNT - 1) {
                j = 0; i++;
            } else {
                j++;
            }
        }

        return result;
    }

    matrix get_matrix_block(int row, int column) {
        matrix result = matrix(BLOCK_ROWS_COUNT);
        result.init_matrix();

        for (int i = 0; i < BLOCK_ROWS_COUNT; i++)
        for (int j = 0; j < BLOCK_ROWS_COUNT; j++)
            result.matrix_data[i][j] = matrix_data[row + i][column + j];

        return result;
    }

    matrix square() {
        matrix result = matrix(vector_size);
        result.init_matrix();

        for (int i = 0; i < vector_size; i++)
        for (int j = 0; j < vector_size; j++) {
            int sum = 0;
            for (int k = 0; k < vector_size; k++)
                sum += matrix_data[i][k] * matrix_data[k][j];
            result.matrix_data[i][j] = sum;
        }

        return result;
    }

};

matrix input_matrix;

void exec_handlers();


int main(int argc, char **argv) {
    char *matrixFileName;

    if (argc != 2) {
        err_sys("Incorrect arguments format. Usage: ./pr matrix_file");
    }

    matrixFileName = argv[1];

    input_matrix = matrix();
    input_matrix.parse_file(matrixFileName);

    exec_handlers();

    // While all child proc are not exited
    int wpid, i, res;
	while((wpid = wait(NULL)) > 0);

	matrix aggregated = aggregate_data();
}

matrix aggregate_data() {

}

void run_process(int read_descr, int write_descr, int delay) {
    fd_set set;
	struct timeval timeout;
	FD_ZERO (&set);
	FD_SET (read_descr, &set);

	timeout.tv_sec = delay;
	timeout.tv_usec = 0;
	if ((select(FD_SETSIZE, &set, NULL, NULL, &timeout)) == -1) {
		err_sys("Errror in select\n");
	}

	// Time to get data from the pipe (serialized data)
    char buf[BUFFER_SIZE];
    read(read_descr, buf, sizeof(char)*BUFFER_SIZE);
    std::string serialized = buf;
    matrix temp;
    // deserialization
    temp = temp.deserialize_block(serialized);
    // multiply on self
    temp = temp.square();
    // serialize result
    std::string result_serialized = temp.serialize_block();
    char * result_serialized_cstr = (char*) result_serialized.c_str();


    FD_SET (write_descr, &set);
	if ((select(FD_SETSIZE, &set, NULL, NULL, &timeout)) == -1) {
		err_sys("Errror in select\n");
	}
	// write to pipe
	write(write_descr, result_serialized_cstr, sizeof(char)*result_serialized.length());
}

void exec_handlers() {
    // Handlers count == count of blocks (ROWS_COUNT)
    for (int i = 0; i < ROWS_COUNT; i+=BLOCK_ROWS_COUNT)
    for (int j = 0; j < ROWS_COUNT; j+=BLOCK_ROWS_COUNT) {
        int fd[2];
        if (pipe(fd) == -1) {
            err_sys("Can't create pipe\n");
        }
        pid_t pid;

        if ((pid = fork()) == -1) {
            err_sys("Can't fork the process\n");
        } else if(pid == 0) {
            run_process(fd[0], fd[1], 2);
            exit(0);
        } else {
            std::string str = input_matrix.get_matrix_block(i, j).serialize_block();
            char *serialized_matrix_part = (char*) str.c_str();
            write(fd[1], serialized_matrix_part, sizeof(char)*str.length());
            pipes[i] = fd[0];
        }
    }
}

void err_sys(char *msg) {
    perror(msg);
    exit(1);
}
