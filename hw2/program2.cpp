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
    
    // make input file for each process
    int part = (H/N)/total_process_num;
    int remain = (H/N) % total_process_num;
    int point = 0; //pointer to current row
    for (int i=0; i<total_process_num; i++){
        fstream input_stream;
        string inputtxt_name = "process"+to_string(i)+"_input";
        string outputtxt_name = "process"+to_string(i)+"_output";
        input_stream.open(inputtxt_name, ios::out);

        int start_row(0), end_row(0), portion(0); 
        if (part == 0){ // H/N <total_process_num
            if(i < remain){
                start_row = point;
                portion = N;
                point += portion;
                end_row = point;
            }else{
                start_row = H;
                end_row = H;
                portion = 0;
            }
        }else{ // H/N >= total_process_num
            if(remain == 0){ //H/N can be divided by total_process_num
                start_row = point;
                portion = part * N;
                point += portion;
                end_row = point;
            }else{
                if (i < remain){
                    start_row = point;
                    portion = (part+1)*N;
                    point += portion;
                    end_row = point;
                }else{
                    start_row = point;
                    portion = (part)*N;
                    point += portion;
                    end_row = point;
                }
            }
        }
        
        input_stream << portion << " "<< W << " "<< N <<"\n"; //H W N
        // write elements
        for (int ii=start_row; ii<end_row; ii++){
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
    end = clock();
    
    // print output for each processes by increasing order
    cout << (double)(end-start) << "\n";
    int count = 0;
    for (int i=0; i<total_process_num; i++){
        string outputtxt_name = "process"+to_string(i)+"_output";
        ifstream output_stream(outputtxt_name);
        int indiv_time;
        output_stream >> indiv_time;
        int portion(0);
        if (part == 0){
            if (i < remain){
                portion = N/N;
            }else{
                portion = 0;
            }
        }else{
            if(remain == 0){
                portion = part;
            }else{
                if (i < remain){
                    portion = part+1;
                }else{
                    portion = part;
                }
            }
        }

        for(int a=0; a<portion; a++){
            for (int j=0; j<W/N; j++){
                int temp;
                output_stream >> temp;
                if(j != W/N-1){
                    cout << temp<< " ";
                }else{
                    cout << temp <<"\n";
                }
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