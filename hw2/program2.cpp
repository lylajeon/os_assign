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
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    string pool_method = string(argv[1]);
    int total_process_num = atoi(argv[2]);

    int H, W, N, temp;
    cin >> H >> W >> N;
    
    // make input file for each process
    int quot = (H/N)/total_process_num;
    int remain = (H/N) % total_process_num;
    // int point = 0; //pointer to current row
    for (int i=0; i<total_process_num; i++){
        fstream input_stream;
        string inputtxt_name = "process"+to_string(i)+"_input";
        string outputtxt_name = "process"+to_string(i)+"_output";
        input_stream.open(inputtxt_name, ios::out);

        int block_num;
        if (i < remain){
            block_num = quot +1;
        }else{
            block_num = quot;
        }
        int row_num = block_num * N;
        
        input_stream << row_num<< " "<< W << " "<< N <<"\n"; //H W N
        // write elements
        for (int i=0; i<row_num; i++){
            for (int j=0; j<W; j++){
                int temp;
                cin >> temp;
                input_stream << temp << " ";
            }
            input_stream <<"\n";
        }
        input_stream.close();
    }

    // make child processes and run pooling by using program1
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
    // wait for all child processes to end
    int status;
    for (int i=0; i<total_process_num; i++){
        waitpid(pid[i], &status, 0);
    }
    
    // pooling end
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long runtime = (long long)( (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1e6);

    // print output for each processes by increasing order
    cout << runtime << "\n";
    
    int count = 0;
    for (int i=0; i<total_process_num; i++){
        string outputtxt_name = "process"+to_string(i)+"_output";
        ifstream output_stream(outputtxt_name);
        string line;
        getline(output_stream, line);

        int count = 0;
        
        getline(output_stream, line);
        while(!output_stream.eof()){
            if (output_stream.eof()){
                break;
            }
            cout << line <<endl;
            getline(output_stream, line);
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