#include <iostream>
#include <string>
#include <time.h>
#include <pthread.h>

using namespace std;

struct input_for_thread{
    int** input;
    int** output;
    int start_row; // based on original matrix
    int end_row; // except this row
    int H;
    int W;
    int N;
    string pool_method;
};

// function for each thread
void* pooling(void* data){
    struct input_for_thread* input_data = (input_for_thread*) data;
    int** arr = (*input_data).input;
    int** output = (*input_data).output;
    int start_row = (*input_data).start_row;
    int end_row = (*input_data).end_row;
    int H = (*input_data).H;
    int W = (*input_data).W;
    int N = (*input_data).N;
    string pool_method = (*input_data).pool_method;
    
    for (int i=start_row; i<end_row; i+=N){
        for (int j=0; j<W; j+=N){
            // i,j (start)-> i/N, j/N 
            int maxNum, minNum, sum(0);
            for (int a=0; a<N; a++){
                for(int b=0; b<N; b++){
                    if(a==0 && b==0){ //start
                        if ((i+a <H )&&(j+b < W)){
                            int temp = arr[i+a][j+b];
                            maxNum = temp;
                            minNum = temp;
                            sum = temp;
                        }
                    }else{
                        if ((i+a <H )&&(j+b < W)){
                            int temp = arr[i+a][j+b];
                            if (maxNum < temp){
                                maxNum = temp;
                            }
                            if (minNum > temp){
                                minNum = temp;
                            }
                            sum += temp;
                        }
                    }
                }
            }
            if (pool_method == "max"){
                output[i/N][j/N] = maxNum;
            }else if (pool_method == "min"){
                output[i/N][j/N] = minNum;
            }else if (pool_method == "avg"){
                output[i/N][j/N] = sum / (N*N);
            }
        }
    }
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char** argv){
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    string pool_method = string(argv[1]);
    int total_thread_num = atoi(argv[2]);
    // initialize threads
    pthread_t threads[total_thread_num];

    int H, W, N, temp;
    cin >> H >> W >> N;
    
    // initialize 
    int** arr = new int*[H];
    for (int i=0; i<H; i++){
        for (int j=0; j<W; j++){
            if (j==0) {arr[i] = new int[W];}
            int temp;
            cin >> temp;
            arr[i][j] = temp;
        }
    }
    
    // pooling output initialize
    int** output = new int*[H/N]();
    for (int i=0; i<H/N; i++){
        output[i] = new int[W/N] ();
    }
    struct input_for_thread inputs[total_thread_num];
    int part = (H/N)/total_thread_num;
    int remain = (H/N) % total_thread_num;
    int point = 0;  //pointer for pointing current row 
    for (int i=0; i<total_thread_num; i++){
        inputs[i].input = arr;
        inputs[i].output = output;
        inputs[i].H = H;
        inputs[i].W = W;
        inputs[i].N = N;
        inputs[i].pool_method = pool_method;

        if (part == 0){  // H/N < total_thread_num
            if(i < remain){
                inputs[i].start_row = point;
                point += N;
                inputs[i].end_row = point;
            }else{
                inputs[i].start_row = H;
                inputs[i].end_row = H;
            }
        }else{   // H/N >= total_thread_num
            if(remain == 0){  // H/N is multiple of total_thread_num
                inputs[i].start_row = point;
                point += part * N;
                inputs[i].end_row = point;
            }else{  // not a multiple of total_thread_num
                if (i < remain){
                    inputs[i].start_row = point;
                    point += (part+1) * N;
                    inputs[i].end_row = point;
                }else{
                    inputs[i].start_row = point;
                    point += (part) * N;
                    inputs[i].end_row = point;
                }
            }
        }
    }
    
    int res;
    void* thread_result;
    for (int i=0; i<total_thread_num; i++){
        res = pthread_create(&(threads[i]), NULL, pooling, (void*) &inputs[i]);
    }
    
    for(int i=0; i<total_thread_num; i++){
        res = pthread_join(threads[i], &thread_result);
        
        if (res != 0){
            printf("[Main] join thread (%d) failed\n", i);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long runtime = (long long)( (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1e6);

    // print output
    cout << runtime << "\n";
    for (int i=0; i<H/N; i++){
        for (int j=0; j<W/N; j++){
            if(j != W/N-1){
                cout << output[i][j]<< " ";
            }else{
                cout << output[i][j]<<"\n";
            }
        }
    }

    return 0;
}