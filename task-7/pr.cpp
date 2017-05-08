#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

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
        std::cout<<">>>\n";
        for (int i = 0; i < vector_size; i++) {
            for (int j = 0; j < vector_size; j++)
                std::cout<<matrix_data[i][j]<<" ";
            std::cout<<"\n";
        }
        std::cout<<"<<<\n";
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

    matrix multiplication(matrix other_matrix) {
        matrix result = matrix(vector_size);
        result.init_matrix();

        for (int i = 0; i < vector_size; i++)
        for (int j = 0; j < vector_size; j++) {
            int sum = 0;
            for (int k = 0; k < vector_size; k++)
                sum += matrix_data[i][k] * other_matrix.matrix_data[k][j];
            result.matrix_data[i][j] = sum;
        }

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

    void add_matrix(matrix other_matrix) {
        for (int i = 0; i < vector_size; i++)
        for (int j = 0; j < vector_size; j++) {
            matrix_data[i][j] += other_matrix.matrix_data[i][j];
        }
    }

    void add_part(matrix part, int row, int column) {
        for (int i = 0; i < BLOCK_ROWS_COUNT; i++)
        for (int j = 0; j < BLOCK_ROWS_COUNT; j++) {
            matrix_data[row + i][column + j] = part.matrix_data[i][j];
        }
    }

};

matrix input_matrix;

void exec_handlers();
matrix aggregate_data();

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

    aggregate_data().print_matrix();
}

matrix aggregate_data() {
    matrix result_matrix = matrix();
    result_matrix.init_matrix();
    char buf[BUFFER_SIZE];

    for (int i = 0; i < ROWS_COUNT; i+=BLOCK_ROWS_COUNT) {
        for (int j = 0; j < ROWS_COUNT; j+=BLOCK_ROWS_COUNT) {
            memset(buf, 0, sizeof(buf));
            read(pipes[i + j/BLOCK_ROWS_COUNT], buf, sizeof(char)*BUFFER_SIZE);
            std::string serialized = buf;
            matrix temp;
            // deserialization
            temp = temp.deserialize_block(serialized);
            result_matrix.add_part(temp, i, j);
        }
    }

    return result_matrix;
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

	// Time to get data from the pipe (order number)
    int number;
    read(read_descr, &number, sizeof(int));
    int i = (number / BLOCK_ROWS_COUNT) * BLOCK_ROWS_COUNT,
        j = (number % BLOCK_ROWS_COUNT) * BLOCK_ROWS_COUNT;

    matrix result_matrix = matrix(BLOCK_ROWS_COUNT);
    result_matrix.init_matrix();

    // Cij = Sum(s=0..block_rows_count-1){Ais*Asj}
    for (int k = 0; k < ROWS_COUNT; k+=2) {
        result_matrix.add_matrix(input_matrix.get_matrix_block(i, k).multiplication(input_matrix.get_matrix_block(k, j)));
    }

    // serialize result
    std::string result_serialized = result_matrix.serialize_block();
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
    for (int i = 0; i < ROWS_COUNT; i++) {
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
            // send the order number (process will be able to determine position of block)
            write(fd[1], &i, sizeof(int));
            pipes[i] = fd[0];
        }
    }
}

void err_sys(char *msg) {
    perror(msg);
    exit(1);
}
