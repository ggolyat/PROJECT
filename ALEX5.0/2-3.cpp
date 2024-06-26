// ALEX5.0 2-3: 100M dataset 分三等份

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

double distance(double a, double b) {
    return std::abs(a - b);
}

std::mutex total_time_mutex;
double bulkload_time = 0;
double total_lookup_time = 0;
double front_lookup_time = 0;
double middle_lookup_time = 0;
double back_lookup_time = 0;






template <typename T>
string to_string(T value) 
{
    ostringstream os;
    os << value;
    return os.str();
}

bool comparePairs(const pair<double, double>& a, const pair<double, double>& b) 
{
    return a.first < b.first;
}



void process_loop(int ii, int loop) 
{
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > front;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > middle;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > back;
    
    // 設置隨機數生成器和分布
    std::default_random_engine generator(loop);
    std::uniform_real_distribution<double> distribution(0.0, 1000000.0);

    string keys_file_path = "testdata2/random_data_2_90_" + to_string(loop) + ".bin";
    
    auto keys = new KEY_TYPE[total_count];
    if (!load_binary_data(keys, total_count, keys_file_path)) 
    {
        cout << "=== load fail!!! " << keys_file_path << endl;
        delete[] keys;
        
        return;
    }
    
    int n_samples = 100000000;
    // std::vector<Point> data(n_samples);
    // for (int i = 0; i < n_samples; ++i) 
    // {
    //     data[i] = {keys[i], -1};  // 初始時不分配聚類
    // }
    std::size_t third = n_samples / 3;  // 每個部分的大小
    sort(keys, keys+n_samples);
    for(int i = 0; i < third; i++)  
    {
        front.push_back({keys[i], dis(gen)});
    }
    for(int i = third; i < third*2; i++)
    {
        middle.push_back({keys[i], dis(gen)});
    }
    for(int i = third*2; i < n_samples; i++)
    {
        back.push_back({keys[i], dis(gen)});
    }
        
        /////// bulkload ///////

        // Create ALEX index
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
    sort(front.begin(), front.end());
    sort(middle.begin(), middle.end());
    sort(back.begin(), back.end());

    auto bulkload_start = chrono::high_resolution_clock::now();
    index.bulk_load(front.data(), front.size()); 
    index.bulk_load(middle.data(), middle.size()); 
    index.bulk_load(back.data(), back.size()); 

    auto bulkload_end = chrono::high_resolution_clock::now();
        

    /////// lookup //////
    
    auto total_start_time = chrono::high_resolution_clock::now();
    for (int i = 0; i < front.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = front[i].first;
        index.find(key);
    }
    for (int i = 0; i < middle.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = middle[i].first;
        index.find(key);
    }
    for (int i = 0; i < back.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = back[i].first;
        index.find(key);
    }
    auto total_end_time = chrono::high_resolution_clock::now();
    {
        lock_guard<mutex> lock(total_time_mutex);
        //auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(total_end_time - total_start_time);
        total_lookup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_end_time - total_start_time).count();
        // front_lookup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(time_block1 - total_start_time).count();
        // middle_lookup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(time_block2 - time_block1).count();
        // back_lookup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_end_time - time_block2).count();
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
    cout << "Total lookup time: " <<  total_lookup_time/100 << " nanoseconds\n";
    //cout << "cluster center: " <<  front_lookup_time/(100*front.size()) << " nanoseconds\n";
    //cout << "middle one lookup time: " <<  middle_lookup_time/(100*middle.size()) << " nanoseconds\n";
    //cout << "back one lookup time: " <<  back_lookup_time/(100*back.size()) << " nanoseconds\n";

    vector<int> plotx;
    vector<double> ploty1;
    vector<double> ploty2;
    
    for(int i=0; i<100; i++)
    {
        for(int j=0; j<resize[i].size(); j++)
        {
            if(resize[i][j]!= 0.0)
            {
                plotx.push_back(i);
                ploty1.push_back(resize[i][j]);
                ploty2.push_back(insert_cost_time[i][j]);
                //total_retrain++;
            }
        }
        
    }
    
    return 0;
}
