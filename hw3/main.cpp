#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <tuple>
#include <algorithm>
#include <map>
#include <cstdlib>

#define UNIT 32

using namespace std;

string PageOption;
int inst_count;

vector<tuple<int, int>> free_list;
vector<tuple<int, int>> alloc_list; // can use as fifo_list
// stack
vector<tuple<int, int>> lruStack_list;
// lfu, mfu
map<tuple<int,int>, int> fuCount;
map<tuple<int,int>, string> PMRefByte;  // key 들은 alloc_list 에 존재하는 tuple들. inst_count 8로 되기 직전에 initialize
int clockPtr = -1;
map<tuple<int,int>, int> PMRefBitClock;

// free_list 에 존재하는 블럭을 합칠 수 있으면 다 합치는 함수
void check_buddy(int frameIdx, int num_frame){
    bool check = false;
    sort(free_list.begin(), free_list.end(),
         [](tuple<int,int> a, tuple<int,int> b){return get<1>(a) < get<1>(b);});
    int idx;
    int blocksize;
    for (int i=0; i<free_list.size(); i++){
        idx = get<0>(free_list[i]);
        blocksize = get<1>(free_list[i]);
        if (blocksize == num_frame){
            int xorOut = idx ^ frameIdx;
            double two_log = log2((double)blocksize);
            if (two_log == (int) two_log){
                free_list.erase(free_list.begin()+i);
                check = true;
                break;
            }
        }
    }
    if (!check){
        free_list.push_back(make_tuple(frameIdx, num_frame));
        return;
    }else{
        int smaller = (idx < frameIdx) ? idx : frameIdx;
        int size = num_frame *2;
        check_buddy(smaller, size);
    }
}

// num_page 를 physical memory 에 넣을 수 있으면 alloc_list에 넣고, physical memory의 starting index, allocate한 frame 수를 return
// + (luStack_list, fuCount 등 update)
tuple<int,int> allocate(int num_page){
    int idx, num_frame;
    // TWO CASES: NEED PAGE REPLACEMENT
    if (free_list.size() == 0){ // no empty place
        idx = -1;
        num_frame = -1;
        return make_tuple(idx, num_frame);
    }
    sort(free_list.begin(), free_list.end(), [](const tuple<int, int>& a, const tuple<int, int>& b){
        return get<1>(a) < get<1>(b);
    });
    int block_target = get<1>(free_list[free_list.size()-1]);
    if (block_target < num_page){   // no block size bigger than request
        idx = -1;
        num_frame = -1;
        return make_tuple(idx, num_frame);
    }

    bool canAlloc = false;
    while(!canAlloc){
        vector<tuple<int,int>> alloc_candidate;
        for (int j=0; j<free_list.size(); j++){
            int start = get<0>(free_list[j]);
            int end = get<1>(free_list[j]);
            if ((end >= num_page)&&(end/2 < num_page)){
                alloc_candidate.push_back(make_tuple(start, end));
            }
        }
        if (alloc_candidate.size() == 0){ // 다 request 보다 큰 경우
            // sort free_list by block size
            sort(free_list.begin(), free_list.end(), [](const tuple<int, int>& a, const tuple<int, int>& b){
                return get<1>(a) < get<1>(b);
            });
            // ASSUMPTION: can allocate memory in Physical Memory
            //             at least one empty place where block size is larger than request
            // find appropriate block size target
            for (int j=0; j<free_list.size(); j++){
                int end = get<1>(free_list[j]);
                if (end > num_page){block_target = end; break;}
            }
            // find all tuples that have frame size same as target
            vector<tuple<int,int>> temp_vec;
            for (int j=0; j<free_list.size(); j++){
                int start = get<0>(free_list[j]);
                int end = get<1>(free_list[j]);
                if (end == block_target){temp_vec.push_back(make_tuple(start, end));}
            }
            // sort the candidate tuples by start index
            sort(temp_vec.begin(), temp_vec.end());
            // find the tuple that has smallest index
            tuple<int,int> temp_tuple = temp_vec.front();
            // remove the tuple from free_list
            vector<tuple<int,int>>::iterator it = find(free_list.begin(), free_list.end(), temp_tuple);
            free_list.erase(it);
            // add 2 tuples to free_list
            int new_start = get<0>(temp_tuple);
            int new_end = get<1>(temp_tuple);
            free_list.push_back(make_tuple(new_start, new_end/2));
            free_list.push_back(make_tuple(new_start+new_end/2, new_end/2));
        }else{  // found appropriate empty place to allocate
            // sort alloc_candidate by start index
            sort(alloc_candidate.begin(), alloc_candidate.end());
            // find the tuple that has smallest index
            tuple<int,int> temp_tuple = alloc_candidate.front();
            vector<tuple<int,int>>::iterator it = find(free_list.begin(), free_list.end(), temp_tuple);
            // erase the tuple from free_list
            free_list.erase(it);

            if (PageOption == "stack"){
                lruStack_list.push_back(temp_tuple);
            }else if ((PageOption == "lfu")|| (PageOption == "mfu")){
                if (fuCount.find(temp_tuple) == fuCount.end()){
                    fuCount[temp_tuple] = 1;
                }else{
                    fuCount[temp_tuple]++;
                }
            }else if (PageOption == "sampled"){
                if (PMRefByte.find(temp_tuple) == PMRefByte.end()){
                    PMRefByte[temp_tuple] = "1";
                }else{
                    PMRefByte[temp_tuple] = "1" + PMRefByte[temp_tuple];
                }
            }else if (PageOption == "second-chance"){
                clockPtr = alloc_list.size();
                PMRefBitClock[temp_tuple] = 1;
            }
            // push the tuple to allocated list in PM
            alloc_list.push_back(temp_tuple);
            // cout <<"allocated "<< get<0>(temp_tuple) <<" "<< get<1>(temp_tuple) << endl;
            idx = get<0>(temp_tuple);
            num_frame = get<1>(temp_tuple);
            canAlloc = true;
            break;
        }
    }
    return make_tuple(idx, num_frame);
}

int main(int argc, char** argv){
    string option = string (argv[1]);
    int cur = option.find("=");
    int len = option.size() - cur - 1;

    PageOption = option.substr(cur+1, len);

    // input
    int num_inst, num_process, virtual_size, physical_size;
    cin >>num_inst >> num_process >> virtual_size >> physical_size;

    // assign
    int PhyMemFrameNum = physical_size / UNIT;
    int PTPageNum = virtual_size / UNIT;
    string PhyMemPID[PhyMemFrameNum];
    string PhyMemAID[PhyMemFrameNum];

    string PageTableAID[num_process][PTPageNum];
    string PageTableVALID[num_process][PTPageNum];
    string PageTableFI[num_process][PTPageNum];

    int AID_CUR[num_process];

    // Physical Memory related

    // PTidxStart[pid][aid] = starting index
    // PTnumPages[pid][aid] = number of pages
    // PMnumFrames[pid][aid] = whether page of aid exist in pagetable
    vector<int> PTidxStart[num_process];
    vector<int> PTnumPages[num_process];
    vector<int> PMnumFrames[num_process];

    fill(&PhyMemPID[0], &PhyMemPID[0]+sizeof(PhyMemPID)/sizeof(string), "-");
    fill(&PhyMemAID[0], &PhyMemAID[0]+sizeof(PhyMemAID)/sizeof(string), "-");
    fill(&PageTableAID[0][0], &PageTableAID[0][0]+sizeof(PageTableAID)/sizeof(string), "-");
    fill(&PageTableVALID[0][0], &PageTableVALID[0][0]+sizeof(PageTableVALID)/sizeof(string), "-");
    fill(&PageTableFI[0][0], &PageTableFI[0][0]+sizeof(PageTableFI)/sizeof(string), "-");
    fill(&AID_CUR[0], &AID_CUR[0]+sizeof(AID_CUR)/sizeof(AID_CUR[0]), 0);

    free_list.push_back(make_tuple(0, PhyMemFrameNum));

    // get instruction if "optimal" page replacement policy
    vector<tuple<int,int,int>> opt_inst_list;
    if (PageOption == "optimal"){
        // get all instructions
        for (int count=0; count < num_inst; count++){
            int inst_type, fir, sec;
            cin >> inst_type >> fir >> sec;
            opt_inst_list.push_back(make_tuple(inst_type, fir, sec));
        }
    }

    int pageFaultCount = 0;
    // conduct instruction including optimal
    for (inst_count=0; inst_count <num_inst; inst_count++){ // For each instruction
        int inst_type;
        if (PageOption == "optimal"){
            inst_type = get<0>(opt_inst_list[inst_count]);
        }else{
            cin >> inst_type;
        }

        if (inst_type == 0){    // memory allocation
            int pid, num_page;
            if (PageOption == "optimal"){
                pid = get<1>(opt_inst_list[inst_count]);
                num_page = get<2>(opt_inst_list[inst_count]);
            }else{
                cin >> pid >> num_page;
            }

            int start = 0;
            for (start = 0; start < PTPageNum; start++){
                bool canAlloc = false;
                if (PageTableAID[pid][start] == "-"){
                    for (int j=0; j<num_page; j++){
                        if ((start + j < PTPageNum)&&(PageTableAID[pid][start+j] == "-")){
                            canAlloc = true;
                        }else {break;}
                    }
                    if (canAlloc){break;}
                }
            }
            // page table aid, valid update
            for(int j=0; j<num_page; j++){
                PageTableAID[pid][start+j] = to_string(AID_CUR[pid]);
                PageTableVALID[pid][start+j] = to_string(0);
            }
            // store starting index and number of pages to PTidxStart and PTnumPages
            PTidxStart[pid].push_back(start);
            PTnumPages[pid].push_back(num_page);
            PMnumFrames[pid].push_back(-1);
            // page table current aid update
            AID_CUR[pid]++;

        }else if (inst_type == 1){  // memory access
            int pid, aid;
            if (PageOption == "optimal"){
                pid = get<1>(opt_inst_list[inst_count]);
                aid = get<2>(opt_inst_list[inst_count]);
            }else{
                cin >> pid >> aid;
            }


            int vm_start_idx = PTidxStart[pid][aid];
            int vm_num_page_request = PTnumPages[pid][aid];
            string frameExist = PageTableVALID[pid][vm_start_idx];

            // if frame not exist in PM
            if (frameExist == "0"){
                pageFaultCount ++;

                // check if there is enough room for allocating memory
                tuple<int,int> alloc_tuple = allocate(vm_num_page_request);

                int pmIdx = get<0>(alloc_tuple);
                int pmNumFrame = get<1>(alloc_tuple);

                // page replacement needed
                while (pmIdx == -1){
                    tuple<int,int> temp_tuple;
                    if (PageOption == "fifo"){
                        temp_tuple = alloc_list.front();
                    }else if (PageOption == "stack"){
                        temp_tuple = lruStack_list.front();
                        vector<tuple<int,int>>::iterator it2 = find(lruStack_list.begin(), lruStack_list.end(), temp_tuple);
                        lruStack_list.erase(it2);
                    }else if (PageOption == "sampled"){
                        int minRefByte;
                        for (auto iter=PMRefByte.begin(); iter != PMRefByte.end(); iter++){
                            if (iter == PMRefByte.begin()){
                                char* temp = (char*)iter->second.c_str();
                                minRefByte = (int) strtoull(temp, &temp, 2);
                            }else{
                                char* temp = (char*)iter->second.c_str();
                                int temp_ref = (int) strtoull(temp, &temp, 2);
                                if (minRefByte > temp_ref){
                                    minRefByte = temp_ref;
                                }
                            }
                        }
                        vector<tuple<int,int>> replace_cand;
                        for (auto iter:PMRefByte){
                            char* temp = (char*)iter.second.c_str();
                            int temp_ref = (int) strtoull(temp, &temp, 2);
                            if (minRefByte == temp_ref){
                                replace_cand.push_back(iter.first);
                            }
                        }
                        if (replace_cand.size() == 1){  // if only one candidate, replace it
                            temp_tuple = replace_cand[0];
                            PMRefByte.erase(temp_tuple);
                        }else{  // if more than one candidate, randomly choose one
                            vector<int> idx_list;
                            for (auto iter = alloc_list.begin(); iter < alloc_list.end(); iter++){
                                if (find(replace_cand.begin(), replace_cand.end(), *iter) != replace_cand.end()){
                                    idx_list.push_back(iter-alloc_list.begin());
                                }
                            }
                            vector<int> sorted_list = idx_list;
                            sort(sorted_list.begin(), sorted_list.end());
                            int target_idx = sorted_list[0];

                            for (int i=0; i<idx_list.size(); i++){
                                if (idx_list[i] == target_idx){
                                    temp_tuple = alloc_list[idx_list[i]];
                                    break;
                                }
                            }
                            PMRefByte.erase(temp_tuple);
                        }

                    }else if (PageOption == "second-chance"){   // TODO: choose the frame that need to be replaced and update needed things
                        bool found = false;
                        while(!found){
                            tuple<int,int> tuple = alloc_list[clockPtr];
                            int usedbit = PMRefBitClock[tuple];
                            if(usedbit == 1){
                                PMRefBitClock[tuple] = 0;
                            }else {
                                temp_tuple = tuple;
                                found = true;
                                break;
                            }
                            clockPtr = (clockPtr + 1) % alloc_list.size();
                        }
                        PMRefBitClock.erase(temp_tuple);

                    }else if ((PageOption == "lfu")||(PageOption == "mfu")){    // least, most frequently used using map
                        // find out appropriate page frame to replace
                        int minFreq, maxFreq;
                        for (auto iter=fuCount.begin(); iter != fuCount.end(); iter++){
                            if (iter == fuCount.begin()){
                                minFreq = maxFreq = iter->second;
                            }else{
                                if (minFreq > iter->second){
                                    minFreq = iter->second;
                                }
                                if (maxFreq < iter->second){
                                    maxFreq = iter->second;
                                }
                            }
                        }
                        vector<tuple<int,int>> fu_candidate;
                        for (auto iter=fuCount.begin(); iter !=fuCount.end(); iter ++){
                            if (PageOption == "lfu"){
                                if (iter->second == minFreq){
                                    fu_candidate.push_back(iter->first);
                                }
                            }else if (PageOption == "mfu"){
                                if (iter->second == maxFreq){
                                    fu_candidate.push_back(iter->first);
                                }
                            }
                        }

                        if (fu_candidate.size() == 1){  // if only one candidate, replace it
                            temp_tuple = fu_candidate[0];
                            fuCount.erase(temp_tuple);
                        }else{  // if more than one candidate, randomly choose one
                            vector<int> idx_list;
                            for (auto iter = alloc_list.begin(); iter < alloc_list.end(); iter++){
                                if (find(fu_candidate.begin(), fu_candidate.end(), *iter) != fu_candidate.end()){
                                    idx_list.push_back(iter-alloc_list.begin());
                                }
                            }
                            vector<int> sorted_list = idx_list;
                            sort(sorted_list.begin(), sorted_list.end());
                            int target_idx = sorted_list[0];

                            for (int i=0; i<idx_list.size(); i++){
                                if (idx_list[i] == target_idx){
                                    temp_tuple = alloc_list[idx_list[i]];
                                    break;
                                }
                            }
                            fuCount.erase(temp_tuple);
                        }
                    }else if (PageOption == "optimal"){
                        temp_tuple = alloc_list.front(); // TODO: if fixed everything, delete this line.
//                        vector<tuple<int,int>> cur_frame_list;  // 현재 PM에 있는 pid,aid 쌍 모음
//                        for (auto frame : alloc_list){
//                             int start_pm_idx = get<0>(frame);
//                             int pm_pid = stoi(PhyMemPID[start_pm_idx]);
//                             int pm_aid = stoi(PhyMemAID[start_pm_idx]);
//                             cur_frame_list.push_back(make_tuple(pm_pid, pm_aid));
//                        }
//
//
//                        vector<tuple<int,int>> stack_acc_frame;
//                        for (int ptr = opt_inst_list.size()-1; ptr>inst_count; ptr--){
//                            tuple<int,int,int> inst_ptr = opt_inst_list[ptr];
//                            int inst_type = get<0>(inst_ptr);
//                            tuple<int,int> inst_tuple = make_tuple(get<1>(inst_ptr), get<2>(inst_ptr));
//                            if (inst_type == 1){
//                                vector<tuple<int,int>>::iterator iter = find(cur_frame_list.begin(), cur_frame_list.end(), inst_tuple);
//                                vector<tuple<int,int>>::iterator iter2 = find(stack_acc_frame.begin(), stack_acc_frame.end(), inst_tuple);
//                                if (iter != cur_frame_list.end()){
//                                    if (iter2 != stack_acc_frame.end()){
//                                        stack_acc_frame.erase(iter2);
//                                    }
//                                    stack_acc_frame.push_back(inst_tuple);
//                                }
//                            }
//                        }
//
//                        if (stack_acc_frame.size() < cur_frame_list.size()){
//
//                            // TODO: number of elements not in stack_acc_frame but in cur_frame_list 에서 가장 PM에서 smallest index 가지는 것으로 temp_tuple (pm starting index, number of frames) 설정
//                            vector<tuple<int,int>> frame_cand_list(cur_frame_list.size());
//                            sort(stack_acc_frame.begin(), stack_acc_frame.end());
//                            sort(cur_frame_list.begin(), cur_frame_list.end());
//                            set_difference(stack_acc_frame.begin(), stack_acc_frame.end(),
//                                          cur_frame_list.begin(), cur_frame_list.end(), frame_cand_list.begin());
//
//                            int temp_pid = get<0>(frame_cand_list[0]);
//                            int temp_aid = get<1>(frame_cand_list[0]);
//                            int temp_vm_st_idx = PTidxStart[temp_pid][temp_aid];
//                            int temp_pm_idx = stoi(PageTableFI[temp_pid][temp_vm_st_idx]);
//
//                            int min_pm_idx = temp_pm_idx;
//                            int min_pm_num_frame = PMnumFrames[temp_pid][temp_aid];
//                            for (int i=0; i<frame_cand_list.size(); i++){
//                                int pid = get<0>(frame_cand_list[i]);
//                                int aid = get<1>(frame_cand_list[i]);
//                                int vm_st_idx = PTidxStart[pid][aid];
//                                int pm_idx = stoi(PageTableFI[pid][vm_st_idx]);
//                                int num_frame = PMnumFrames[pid][aid];
//                                if (min_pm_idx > pm_idx){
//                                    min_pm_idx = pm_idx;
//                                    min_pm_num_frame = num_frame;
//                                }
//                            }
//
//                            temp_tuple = make_tuple(min_pm_idx, min_pm_num_frame);
//                        }
//                        else{
//
//                            // TODO: stack_acc_frame.front()에 있는 것에 해당하는 것으로 temp_tuple 설정.
//                            tuple<int,int> pid_aid = stack_acc_frame.front();
//                            int temp_pid = get<0>(pid_aid);
//                            int temp_aid = get<1>(pid_aid);
//                            int vm_st_idx = PTidxStart[temp_pid][temp_aid];
//                            int pm_idx = stoi(PageTableFI[temp_pid][vm_st_idx]);
//                            int num_frame = PMnumFrames[temp_pid][temp_aid];
//                            temp_tuple = make_tuple(pm_idx, num_frame);
//                        }
                    }
                    // update
                    vector<tuple<int,int>>::iterator it = find(alloc_list.begin(), alloc_list.end(), temp_tuple);
                    alloc_list.erase(it);

                    check_buddy(get<0>(temp_tuple), get<1>(temp_tuple));

                    int pmidx = get<0>(temp_tuple);
                    int pid = stoi(PhyMemPID[pmidx]);
                    int aid = stoi(PhyMemAID[pmidx]);
                    int num_page = PTnumPages[pid][aid];
                    int num_frame = PMnumFrames[pid][aid];
                    int vmidx = PTidxStart[pid][aid];

                    // update process's page table related
                    for (int j=0; j<num_page; j++){
                        PageTableVALID[pid][vmidx+j] = "0";
                        PageTableFI[pid][vmidx+j] = "-";
                    }
                    PMnumFrames[pid][aid] = -1;

                    // update physical memory related
                    for (int j=0; j<num_frame; j++){
                        PhyMemPID[pmidx+j] = "-";
                        PhyMemAID[pmidx+j] = "-";
                    }
                    // check
                    alloc_tuple = allocate(vm_num_page_request);
                    pmIdx = get<0>(alloc_tuple);
                    pmNumFrame = get<1>(alloc_tuple);
                }

                // allocate frame (update PhyMemPID, PhyMemAID)
                for (int j=0; j<pmNumFrame; j++){
                    PhyMemPID[pmIdx+j] = to_string(pid);
                    PhyMemAID[pmIdx+j] = to_string(aid);
                }
                // and update PMnumFrames, PageTableFI
                PMnumFrames[pid][aid] = pmNumFrame;
                for (int j=0; j<vm_num_page_request; j++){
                    PageTableVALID[pid][vm_start_idx+j] = to_string(1);
                    PageTableFI[pid][vm_start_idx+j] = to_string(pmIdx);
                }

            }else{  // if frame exist in Physical memory
                if (PageOption == "stack"){
                    tuple<int,int> temp = make_tuple(vm_start_idx, vm_num_page_request);
                    vector<tuple<int,int>>::iterator iter = find(lruStack_list.begin(), lruStack_list.end(), temp);
                    lruStack_list.erase(iter);
                    lruStack_list.push_back(temp);
                }else if ((PageOption == "lfu")|| (PageOption == "mfu")){
                    int pm_start_idx = stoi(PageTableFI[pid][vm_start_idx]);
                    int pm_num_frame = PMnumFrames[pid][aid];
                    fuCount[make_tuple(pm_start_idx, pm_num_frame)]++;
                }else if (PageOption == "sampled"){
                    int pm_start_idx = stoi(PageTableFI[pid][vm_start_idx]);
                    int pm_num_frame = PMnumFrames[pid][aid];
                    tuple<int, int> temp = make_tuple(pm_start_idx, pm_num_frame);
                    PMRefByte[temp] = "1"+PMRefByte[temp];
                }else if (PageOption == "second-chance"){
                    int pm_start_idx = stoi(PageTableFI[pid][vm_start_idx]);
                    int pm_num_frame = PMnumFrames[pid][aid];
                    tuple<int, int> temp = make_tuple(pm_start_idx, pm_num_frame);
                    PMRefBitClock[temp] = 1;

                    for (int i=0; i<alloc_list.size(); i++){
                        if (alloc_list[i] == temp){
                            clockPtr = (i+1)% alloc_list.size();
                        }
                    }
                }
            }

            // need refreshment after 8 instructions
            if (PageOption == "sampled"){
                if (inst_count %8 == 7){
                    for (auto key:PMRefByte){
                        key.second = "";
                    }
                }else{
                    int pm_start_idx = stoi(PageTableFI[pid][vm_start_idx]);
                    int pm_num_frame = PMnumFrames[pid][aid];
                    tuple<int, int> temp = make_tuple(pm_start_idx, pm_num_frame);
                    for (auto key:PMRefByte){
                        if (key.first != temp){
                            key.second = "0"+key.second;
                        }
                    }
                }
            }


        // memory release
        }else if (inst_type == 2){
            int pid, aid;
            if (PageOption == "optimal"){
                pid = get<1>(opt_inst_list[inst_count]);
                aid = get<2>(opt_inst_list[inst_count]);
            }else{
                cin >> pid >> aid;
            }

            int vm_start_idx = PTidxStart[pid][aid];
            int vm_num_page = PTnumPages[pid][aid];

            string frameExist = PageTableVALID[pid][vm_start_idx];
            int frameIdx = (frameExist == "1") ? stoi(PageTableFI[pid][vm_start_idx]) : -1;
            // release in VM
            PTidxStart[pid][aid] = -1;
            for (int j=0; j<vm_num_page; j++){
                PageTableAID[pid][vm_start_idx+j] = "-";
                PageTableVALID[pid][vm_start_idx+j] = "-";
                PageTableFI[pid][vm_start_idx+j] = "-";
            }

            // if exist in PM, release that frame
            if (frameExist == "1"){
                int num_frame = PMnumFrames[pid][aid];
                for (int j=0; j<num_frame; j++){
                    PhyMemPID[frameIdx+j] = "-";
                    PhyMemAID[frameIdx+j] = "-";
                }
                tuple<int,int> temp_tuple = make_tuple(frameIdx, num_frame);
                vector<tuple<int,int>>::iterator iter = find(alloc_list.begin(), alloc_list.end(), temp_tuple);
                vector<tuple<int,int>>::iterator iter2 = find(lruStack_list.begin(), lruStack_list.end(), temp_tuple);
                alloc_list.erase(iter);
                lruStack_list.erase(iter2);
                fuCount.erase(temp_tuple);
                PMRefByte.erase(temp_tuple);
                PMRefBitClock.erase(temp_tuple);

                check_buddy(frameIdx, num_frame);
            }

        }else{
            cout << "wrong type"<< endl;
        }

        // output
        // Line 1
        printf("%-30s", ">> Physical Memory (PID): ");
        string PhyMemPidString = "";
        for (int i=0; i<PhyMemFrameNum; i++){
            if (i % 4 ==0){
                PhyMemPidString += "|";
            }
            PhyMemPidString += PhyMemPID[i];
        }
        PhyMemPidString += "|\n";
        cout << PhyMemPidString;
        // Line 2
        printf("%-30s", ">> Physical Memory (AID): ");
        string PhyMemAidString = "";
        for (int i=0; i<PhyMemFrameNum; i++){
            if (i % 4 ==0){
                PhyMemAidString += "|";
            }
            PhyMemAidString += PhyMemAID[i];
        }
        PhyMemAidString += "|\n";
        cout << PhyMemAidString;
        // Line 3
        for (int PID=0; PID<num_process; PID++){
            // Line {3 + PID * 3}
            printf(">> PID(%d) %-20s", PID, "Page Table (AID): ");
            string PageAIDString = "";
            for (int i=0; i<PTPageNum; i++){
                if (i % 4 ==0){
                    PageAIDString += "|";
                }
                PageAIDString += PageTableAID[PID][i];
            }
            PageAIDString += "|\n";
            cout << PageAIDString;

            // Line {4 + PID * 3}
            printf(">> PID(%d) %-20s", PID, "Page Table (Valid): ");
            string PageValString = "";
            for (int i=0; i<PTPageNum; i++){
                if (i % 4 ==0){
                    PageValString += "|";
                }
                PageValString += PageTableVALID[PID][i];
            }
            PageValString += "|\n";
            cout << PageValString;

            // Line {4 + PID * 3}
            printf(">> PID(%d) %-20s", PID, "Page Table (FI): ");
            string PageFIString = "";
            for (int i=0; i<PTPageNum; i++){
                if (i % 4 ==0){
                    PageFIString += "|";
                }
                PageFIString += PageTableFI[PID][i];
            }
            PageFIString += "|\n";
            cout << PageFIString;
        }
        printf("\n");
    }
    printf("%d\n", pageFaultCount);
    return 0;
}