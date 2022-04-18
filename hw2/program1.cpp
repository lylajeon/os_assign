#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <time.h>

using namespace std;

int main(int argc, char** argv){
    clock_t start, end;
    start = clock();
    string pool_method = string(argv[1]);

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
    
    // pooling 
    int** output = new int*[H/N];
    for (int i=0; i<H/N; i++){
        output[i] = new int[W/N];
    }

    for (int i=0; i<H; i+=N){
        for (int j=0; j<W; j+=N){
            // i,j (start)-> i/N, j/N 
            int maxNum, minNum, sum(0);
            for (int a=0; a<N; a++){
                for(int b=0; b<N; b++){
                    if(a==0 && b==0){
                        int temp = arr[i+a][j+b];
                        maxNum = temp;
                        minNum = temp;
                        sum = temp;
                    }else{
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
            if (pool_method == "max"){
                output[i/N][j/N] = maxNum;
            }else if (pool_method == "min"){
                output[i/N][j/N] = minNum;
            }else if (pool_method == "avg"){
                output[i/N][j/N] = sum / (N*N);
            }
        }
    }
    end = clock();

    // print output
    cout << (long long)(end-start) << "\n";
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