#include <iostream>
#include <string>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <sys/wait.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <cstdio>

using namespace std;

int main(int argc, char** argv){
    clock_t start, end;
    start = clock();
    string pool_method = string(argv[1]);
    int total_process_num = atoi(argv[2]);

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
    
    // make input file 
    int part = H/total_process_num;
    int remain = H % total_process_num;
    for (int i=0; i<total_process_num; i++){
        fstream input_stream;
        string inputtxt_name = "process"+to_string(i)+"_input";
        string outputtxt_name = "process"+to_string(i)+"_output";
        input_stream.open(inputtxt_name, ios::out);

        // remain 없다고 생각하고 짠것. 
        input_stream << part << " "<< W << " "<< N <<"\n";
        for (int ii=part*i; ii<part*(i+1); ii++){
            for(int j=0; j<W; j++){
                if (j!= W-1){
                    input_stream << arr[ii][j]<< " ";
                }else{
                    input_stream << arr[ii][j]<< "\n";
                }
            }
        }
        input_stream.close();
    }

    pid_t pid[total_process_num];
    for (int i=0; i<total_process_num; i++){
        pid[i] = fork();
        if (pid[i] < 0){
            return -1;
        }else if (pid[i] == 0){
            char* argv[] = {(char *)"./program1", (char*)pool_method.c_str(), NULL};
            string inputtxt_name = "process"+to_string(i)+"_input";
            string outputtxt_name = "process"+to_string(i)+"_output";   
            int inputfile = open(inputtxt_name.c_str(), O_RDONLY);
            int outputfile = open(outputtxt_name.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            //replace standard input with input file
            dup2(inputfile, STDIN_FILENO);
            //replace standard output with output file
            dup2(outputfile, STDOUT_FILENO);
            //close unused file descriptors
            close(inputfile);
            close(outputfile);
            //execute program1
            execvp(argv[0], argv);
            exit(0);
        }
    }
    int status;
    for (int i=0; i<total_process_num; i++){
        waitpid(pid[i], &status, 0);
    }
    
    // pooling 
    end = clock();
    
    // print output
    cout << (double)(end-start) << "\n";
    for (int i=0; i<total_process_num; i++){
        string outputtxt_name = "process"+to_string(i)+"_output";
        ifstream output_stream(outputtxt_name);
        int indiv_time;
        output_stream >> indiv_time;
        for (int j=0; j<W/N; j++){
            int temp;
            output_stream >> temp;
            if(j != W/N-1){
                cout << temp<< " ";
            }else{
                cout << temp <<"\n";
            }
        }
        output_stream.close();
    }
    
    // remove temporary file for processes
    for(int i=0; i<total_process_num; i++){
        string inputtxt_name = "process"+to_string(i)+"_input";
        string outputtxt_name = "process"+to_string(i)+"_output";   
        remove(inputtxt_name.c_str());
        remove(outputtxt_name.c_str());
    }

    return 0;
}