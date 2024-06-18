// ALEX3.0  繪圖找retrain發生的位置(%數)

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

const double total_count = 100000000;
mt19937_64 gen(random_device{}());
uniform_real_distribution<double> dis;
uniform_real_distribution<double> range1(0.0, 10000.0);
uniform_real_distribution<double> range3(100000000.0, 100010000.0);

vector< vector<double> > resize(100);

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

void process_loop(int ii, int loop, const string& keys_file_type, int total_num_keys, chrono::nanoseconds& total_time, mutex& time_mutex, chrono::nanoseconds& total_load_time, mutex& load_time_mutex,  mutex& resize_mutex) 
{
    auto keys = new KEY_TYPE[total_num_keys];
    auto values = new pair<KEY_TYPE, PAYLOAD_TYPE>[total_num_keys];
    int temp = int((1.0-sp[ii]*2)*100 + 0.001);
    string keys_file_path = "random_data_3_" + to_string(temp) + "_" + to_string(loop) + ".bin";
   
    if (!load_binary_data(keys, total_num_keys, keys_file_path)) 
    {
        cout << "=== load fail!!! " << keys_file_path << endl;
        delete[] keys;
        delete[] values;
        return;
    }
    sort(keys, keys + total_num_keys);
    for(int i = 0; i < total_num_keys; i++)
    {
        values[i].first = keys[i];
        values[i].second = dis(gen);
    }
    
    //cout << "=== loading ===\n";

    // Create ALEX index
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
    index.bulk_load(values, total_num_keys*(1.0-sp[ii]*2)); // middle

    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > front;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > back;
    for(int i=0; i<total_count*sp[ii]; i++)         //front
    {
        KEY_TYPE new_key = range1(gen);
        PAYLOAD_TYPE new_payload = dis(gen);
        front.push_back({new_key,new_payload});
    }
    //sort(front.begin(), front.end(), comparePairs);
    
    for(long long int i=0; i<total_count*sp[ii]; i++)     
    { 
        
        int prev = index.stats_.num_expand_and_retrains;
        index.insert(front[i].first, front[i].second);
        
        if(index.stats_.num_expand_and_retrains != prev)        // resize happens
        {
            {
                lock_guard<mutex> lock(resize_mutex);
                resize[loop].push_back(double(i/total_count));  //percentage
                
            }
            //cout << "expand & retrain at front inserting " << i << " : " << new_key << endl;
        }
        
    }

    //cout << "=== back ===\n";
    for(int i=0; i<total_count*sp[ii]; i++) 
    {
        KEY_TYPE new_key = range3(gen);
        PAYLOAD_TYPE new_payload = dis(gen);
        back.push_back({new_key,new_payload});
    }
    //sort(back.begin(), back.end(), comparePairs);

    for(int i=0; i<total_count*sp[ii]; i++)     //back
    {
        int prev = index.stats_.num_expand_and_retrains;
        index.insert(back[i].first, back[i].second);
        if(index.stats_.num_expand_and_retrains != prev)
        {
            {
                lock_guard<mutex> lock(resize_mutex);
                resize[loop].push_back(double( (i+ total_count*(1-sp[ii]))/total_count ));
            }
            
            //cout << "expand & retrain at back inserting " << i << " : " << new_key << endl;
        }
    }


    //cout << "=== look up ===\n";
    auto start_time = chrono::high_resolution_clock::now();
    for (int i = 0; i < total_num_keys; ++i) 
    {
        // Lookup operation
        KEY_TYPE key = keys[i];
        index.find(key);
    }
    auto end_time = chrono::high_resolution_clock::now();
    {
        lock_guard<mutex> lock(time_mutex);
        total_time += chrono::duration_cast<chrono::nanoseconds>(end_time - start_time);
        cout << loop << " expand & retrain: " << index.stats_.num_expand_and_retrains <<endl;
    }
    
    
    delete[] keys;
    delete[] values;
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

    

    double sum = 0;
    chrono::nanoseconds total_time(0);
    chrono::nanoseconds total_load_time(0);
    int count = 0;

    vector<thread> threads;

    mutex time_mutex;
    mutex load_time_mutex;
    mutex resize_mutex;
    for (int loop = 0; loop < 100; ++loop) 
    {
        threads.emplace_back(process_loop, ii, loop, ref(keys_file_type), total_num_keys*(1-sp[ii]*2), ref(total_time) , ref(time_mutex), ref(total_load_time) , ref(load_time_mutex), ref(resize_mutex) );    
    }

    
    // Wait for all threads to finish
    for (auto& thread : threads) 
    {
        thread.join();
    }

    cout << "Total time: " <<  total_time.count()/100 << " nanoseconds\n";

    vector<int> plotx;
    vector<double> ploty;
    int total_retrain=0;
    for(int i=0; i<100; i++)
    {
        for(int j=0; j<resize[i].size(); j++)
        {
            if(resize[i][j]!= 0.0)
            {
                plotx.push_back(i);
                ploty.push_back(resize[i][j]);
                total_retrain++;
            }
        }
        
    }
    cout << "retrain: " << double(total_retrain)/100.0 << "\n";
    cout << "size: " << resize[0].size() << "\n"; 
    plt::scatter(plotx, ploty);
    plt::save("minimal.jpg");
    return 0;
}
