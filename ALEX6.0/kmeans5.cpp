// ALEX6.0 先用kmean.py分出五份後直接放進去計時

#include "../core/alex.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <random>
#include <chrono>
#include <mutex>
#include <fstream>
#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include "flags.h"
#include "utils.h"

#define KEY_TYPE double
#define PAYLOAD_TYPE double

using namespace std;
double sp[] = {0, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45};

const int total_count = 100000000;
mt19937_64 gen(random_device{}());
uniform_real_distribution<double> dis;

std::mutex resize_mutex;
vector< vector<double> > insert_cost_time(100);
vector< vector<double> > resize(100);
int total_retrain=0;

struct Point {
    double value;
    int cluster;  // 所屬聚類的索引
};

std::mutex total_time_mutex;
double bulkload_time = 0;
double total_lookup_time = 0;
double look_up_time1 = 0;
double look_up_time2 = 0;
double look_up_time3 = 0;
double look_up_time4 = 0;
double look_up_time5 = 0;

template <typename T>
string to_string(T value) 
{
    ostringstream os;
    os << value;
    return os.str();
}

void process_loop(int ii, int loop) 
{
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > area1;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > area2;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > area3;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > area4;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > area5;
    
    // 設置隨機數生成器和分布
    std::default_random_engine generator(loop);
    std::uniform_real_distribution<double> distribution(0.0, 1000000.0);

    string keys_file_path1 = "testdata2/random_data_4_20_" + to_string(loop) + "_1.bin";
    string keys_file_path2 = "testdata2/random_data_4_20_" + to_string(loop) + "_2.bin";
    string keys_file_path3 = "testdata2/random_data_4_20_" + to_string(loop) + "_3.bin";
    string keys_file_path4 = "testdata2/random_data_4_20_" + to_string(loop) + "_4.bin";
    string keys_file_path5 = "testdata2/random_data_4_20_" + to_string(loop) + "_5.bin";
    
    auto keys1 = new KEY_TYPE[int(total_count*0.2)];
    auto keys2 = new KEY_TYPE[int(total_count*0.2)];
    auto keys3 = new KEY_TYPE[int(total_count*0.2)];
    auto keys4 = new KEY_TYPE[int(total_count*0.2)];
    auto keys5 = new KEY_TYPE[int(total_count*0.2)];
    if (!load_binary_data(keys1, total_count*0.2, keys_file_path1)) 
    {
        cout << "=== load fail!!! " << keys_file_path1 << endl;
        delete[] keys1;
        delete[] keys2;
        delete[] keys3;
        return;
    }
    load_binary_data(keys2, total_count*0.2, keys_file_path2);
    load_binary_data(keys3, total_count*0.2, keys_file_path3);
    load_binary_data(keys4, total_count*0.2, keys_file_path4);
    load_binary_data(keys5, total_count*0.2, keys_file_path5);

    for(int i = 0; i < total_count*0.2; i++)  
    {
        area1.push_back({keys1[i], dis(gen)});
    }
    for(int i = 0; i < total_count*0.2; i++)
    {
        area2.push_back({keys2[i], dis(gen)});
    }
    for(int i = 0; i < total_count*0.2; i++)
    {
        area3.push_back({keys3[i], dis(gen)});
    }
    for(int i = 0; i < total_count*0.2; i++)
    {
        area4.push_back({keys4[i], dis(gen)});
    }
    for(int i = 0; i < total_count*0.2; i++)
    {
        area5.push_back({keys5[i], dis(gen)});
    }
        
        /////// bulkload ///////

        // Create ALEX index
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
    sort(area1.begin(), area1.end());
    sort(area2.begin(), area2.end());
    sort(area3.begin(), area3.end());
    sort(area4.begin(), area4.end());
    sort(area5.begin(), area5.end());

    auto bulkload_start = chrono::high_resolution_clock::now();
    index.bulk_load(area1.data(), area1.size()); 
    index.bulk_load(area2.data(), area2.size());
    index.bulk_load(area3.data(), area3.size());
    index.bulk_load(area4.data(), area4.size());
    index.bulk_load(area5.data(), area5.size());

    auto bulkload_end = chrono::high_resolution_clock::now();
        

    /////// lookup //////
    
    auto total_start_time = chrono::high_resolution_clock::now();
    for (int i = 0; i < area1.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = area1[i].first;
        index.find(key);
    }
    auto block1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < area2.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = area2[i].first;
        index.find(key);
    }
    auto block2 = chrono::high_resolution_clock::now();
    for (int i = 0; i < area3.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = area3[i].first;
        index.find(key);
    }
    auto block3 = chrono::high_resolution_clock::now();
    for (int i = 0; i < area4.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = area4[i].first;
        index.find(key);
    }
    auto block4 = chrono::high_resolution_clock::now();
    for (int i = 0; i < area5.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = area5[i].first;
        index.find(key);
    }
    auto total_end_time = chrono::high_resolution_clock::now();
    {
        lock_guard<mutex> lock(total_time_mutex);
        //auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(total_end_time - total_start_time);
        total_lookup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_end_time - total_start_time).count();
        look_up_time1 += std::chrono::duration_cast<std::chrono::nanoseconds>(block1 - total_start_time).count();
        look_up_time2 += std::chrono::duration_cast<std::chrono::nanoseconds>(block2 - block1).count();
        look_up_time3 += std::chrono::duration_cast<std::chrono::nanoseconds>(block3 - block2).count();
        look_up_time4 += std::chrono::duration_cast<std::chrono::nanoseconds>(block4 - block3).count();
        look_up_time5 += std::chrono::duration_cast<std::chrono::nanoseconds>(total_end_time - block4).count();
        bulkload_time += std::chrono::duration_cast<std::chrono::nanoseconds>(bulkload_end - bulkload_start).count();
        //total_retrain += index.stats_.num_expand_and_retrains;
     
        //cout << loop << " center: " << centers[0] << " " <<  centers[1] << " " << centers[2] << " "<<endl;
    }
    
}

int main(int argc, char* argv[]) 
{
    auto flags = parse_flags(argc, argv);
    auto ii = stoi(get_required(flags, "keys_file"));  //0-10

    string keys_file_type = get_required(flags, "keys_file_type");
    auto total_num_keys = stoi(get_required(flags, "total_num_keys"));
    auto batch_size = stoi(get_required(flags, "batch_size"));
    string lookup_distribution =
        get_with_default(flags, "lookup_distribution", "zipf");
    auto time_limit = stod(get_with_default(flags, "time_limit", "1000000000"));
    bool print_batch_stats = get_boolean_flag(flags, "print_batch_stats");


    vector<thread> threads;

    for (int loop = 0; loop < 100; ++loop) 
    {
        threads.emplace_back(process_loop, ii, loop);    
    }

    
    // Wait for all threads to finish
    for (auto& thread : threads) 
    {
        thread.join();
    }

    cout << "Bulkload time: " << bulkload_time/100 << endl;
    cout << "Total lookup time: " <<  total_lookup_time/100 <<endl;
    cout << "1st lookup time: " <<  look_up_time1/100 << endl;
    cout << "2nd lookup time: " <<  look_up_time2/100 << endl;
    cout << "3rd lookup time: " <<  look_up_time3/100 << endl;
    cout << "4th lookup time: " <<  look_up_time4/100 << endl;
    cout << "5th lookup time: " <<  look_up_time5/100 << endl;

    vector<int> plotx;
    vector<double> ploty1;
    vector<double> ploty2;
    
    return 0;
}
