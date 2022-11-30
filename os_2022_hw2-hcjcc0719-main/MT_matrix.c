#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#define _GNU_SOURCE
// Matrix x/y dimension
#define DIMENSION 10000
// Number of threads to spawn
int NUM_THREADS ;
#define gettid() ((pid_t)syscall(SYS_gettid))
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int m1_row, m1_col, m2_row, m2_col;

struct timeval begin, end;
long seconds;long microseconds;
double elapsed;

// Structure to pass to threads
struct Matrices {
	int id;
    int** a;
    int** b;    
    int** result;
};

// Initialise a matrix, either random or zeroes
int** init() {

    int** matrix = malloc(DIMENSION * sizeof(int));

    for (int i = 0; i < m1_row; i++) {
        matrix[i] = malloc(DIMENSION * sizeof(int));
        int j;
        
        for (int j = 0; j < m2_col; j++) {
            matrix[i][j] = 0;
        }
    }
    return matrix;
}

int** init_m1(char *argv[]) {

    FILE *fp = NULL;
    char buff[10];
    
    fp = fopen(argv[2], "r");
    fscanf(fp, "%s", buff);
    m1_row = atoi(buff);
    fscanf(fp, "%s", buff);
    m1_col = atoi(buff);
    
    int i;
    int** matrix = malloc(DIMENSION * sizeof(int));
    for (i = 0; i < m1_row; i++) {
        matrix[i] = malloc(DIMENSION * sizeof(int));
        int j;
        
        for (j = 0; j < m1_col; j++) {
            fscanf(fp, "%s", buff);
            matrix[i][j] = atoi(buff);
          
        }
    }
    fclose(fp);
    return matrix;
}

int** init_m2(char *argv[]) {

    FILE *fp = NULL;
    char buff[10];
    
    fp = fopen(argv[3], "r");
    fscanf(fp, "%s", buff);
    m2_row = atoi(buff);
    fscanf(fp, "%s", buff);
    m2_col = atoi(buff);
    
    int i;
    int** matrix = malloc(DIMENSION * sizeof(int));

    for (i = 0; i < m2_row; i++) {
        matrix[i] = malloc(DIMENSION * sizeof(int));
        int j;
        
        for (j = 0; j < m2_col; j++) {
            fscanf(fp, "%s", buff);
            matrix[i][j] = atoi(buff);
        }
    }
    fclose(fp);
    return matrix;
}

// Debug: Print matrix values
void print_matrix(int** matrix) {
    FILE *out_file = fopen("result.txt", "w");       

    int i;
    fprintf(out_file, "%d %d", m1_row, m2_col);

    for (i = 0; i < m1_row; i++) {
        fprintf(out_file, "\n");
        int j;

        for (j = 0; j < m2_col; j++) {
            fprintf(out_file,"%i ",matrix[i][j]);
        }
    }
    fclose(out_file); 
}

// Thread function 
void* runner(void* args) {
	struct Matrices* thread_data = (struct Matrices*) args;
	int id = (*thread_data).id;
	int i;
	int sum = 0;
    
    pid_t tid;
    tid = syscall(SYS_gettid);
    char *TIDinString = malloc(64 * sizeof(char));
    sprintf(TIDinString, "%d",tid);
    
    int k = id;
    while(k < m1_row)
    {
        for (i = 0; i < m2_col; i++) {
            int j;
            for (j = 0; j < m2_row; j++) {
                sum += (*thread_data).a[k][j] * (*thread_data).b[j][i];
            }

            (*thread_data).result[k][i] = sum;
            sum = 0;
        }
        k += NUM_THREADS;
    }
    
    pthread_mutex_lock(&mutex);
    
	int fd = open("/proc/thread_info", O_RDWR); 
    //lseek(fd, 0 , SEEK_SET);
	write(fd, TIDinString, strlen(TIDinString)); 
    //lseek(fd, 0 , SEEK_SET);
    close(fd);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

// Multithreaded multiplication algorithm
void multiply_multi(int** a, int** b, int** result) {
    pthread_t* threads;
    threads = malloc(NUM_THREADS * sizeof(pthread_t));
    struct Matrices thread_data[NUM_THREADS];
    int i;

    gettimeofday(&begin, 0);
    
    for (i = 0; i < NUM_THREADS; i++) {
        thread_data[i].id = i;
        thread_data[i].a = a;
        thread_data[i].b = b;
        thread_data[i].result = result;
        pthread_create(&threads[i], NULL, runner, &thread_data[i]);
    }
    
    for (i = 0; i < NUM_THREADS; i++) {
        
    	pthread_join(threads[i], NULL);
    }
    gettimeofday(&end, 0);
    seconds = end.tv_sec - begin.tv_sec;
    microseconds = end.tv_usec - begin.tv_usec;
    elapsed = seconds + microseconds*1e-6;
    
}

int main(int argc, char *argv[]) {
    
    NUM_THREADS = atoi(argv[1]);
    // Initialise matrices
    int** matrixA = init_m1(argv);
    int** matrixB = init_m2(argv);
    int** result_matrix = init();
    
   
    // Multi-threaded multiply and print result
    int fd = open("/proc/thread_info", O_RDWR); 
    char *PIDString = malloc(64 * sizeof(char));
    sprintf(PIDString, "%d",getpid());
	write(fd, PIDString , strlen(PIDString)); 
    close(fd);
    multiply_multi(matrixA, matrixB, result_matrix);
    
    fd = open("/proc/thread_info", O_RDWR);
    char buffer[BUFSIZ];
    read(fd, buffer, BUFSIZ);
    printf("%s\n", buffer);
    close(fd);
    print_matrix(result_matrix);
    printf("\n\tElapsed time: %.3f seconds.\n\n", elapsed);

    return 0;
}
