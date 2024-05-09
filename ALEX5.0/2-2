// ALEX5.0 2-2: 沒有移除離群值

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

    string keys_file_path = "testdata2/random_data_2_33_" + to_string(loop) + ".bin";
    
    auto keys = new KEY_TYPE[total_count];
    if (!load_binary_data(keys, total_count, keys_file_path)) 
    {
        cout << "=== load fail!!! " << keys_file_path << endl;
        delete[] keys;
        
        return;
    }
    
    int n_samples = 100000000;
    std::vector<Point> data(n_samples);
    for (int i = 0; i < n_samples; ++i) 
    {
        data[i] = {keys[i], -1};  // 初始時不分配聚類
    }
    // 選擇 k
    int k = 3;

    // 初始化聚類中心
    std::vector<double> centers(k);
    std::uniform_int_distribution<int> index_distribution(0, n_samples - 1);
    for (int i = 0; i < k; ++i) {
        centers[i] = data[index_distribution(generator)].value;
    }

    // k-means 主循環
    bool converged = false;
    int max_iterations = 100;
    int iterations = 0;

    while (!converged && iterations < max_iterations) 
    {
        converged = true;

        // 步驟 2：將每個數據點分配到最接近的聚類中心
        for (auto& point : data) {
            int closest_cluster = -1;
            double min_distance = std::numeric_limits<double>::max();
            for (int i = 0; i < k; ++i) {
                double dist = distance(point.value, centers[i]);
                if (dist < min_distance) {
                    min_distance = dist;
                    closest_cluster = i;
                }
            }

            if (point.cluster != closest_cluster) {
                point.cluster = closest_cluster;
                converged = false;
            }
        }

        // 步驟 3：根據分配的數據點更新聚類中心
        std::vector<double> new_centers(k, 0.0);
        std::vector<int> counts(k, 0);
        
        for (const auto& point : data) 
        {
            new_centers[point.cluster] += point.value;
            counts[point.cluster]++;
        }

        for (int i = 0; i < k; ++i) 
        {
            if (counts[i] > 0) {
                new_centers[i] /= counts[i];
            }
        }

        for (int i = 0; i < k; ++i) 
        {
            if (centers[i] != new_centers[i]) {
                converged = false;
                centers[i] = new_centers[i];
            }
        }

        iterations++;
    }

    // 使用3個vector來儲存每個群的成員
    std::vector<std::vector<Point>> clusters(k);
    for (const auto& point : data) {
        clusters[point.cluster].push_back(point);
    }


    cout << "center: " << centers[0] << " " << centers[1] << " " << centers[2] << endl;
    cout << "size: " << clusters[0].size() << " " << clusters[1].size() << " " << clusters[2].size() << endl;
    for(int i = 0; i < clusters[0].size(); i++)  
    {
        front.push_back({clusters[0][i].value, dis(gen)});
    }
    for(int i = 0; i < clusters[1].size(); i++)
    {
        middle.push_back({clusters[1][i].value, dis(gen)});
    }
    for(int i = 0; i < clusters[2].size(); i++)
    {
        back.push_back({clusters[2][i].value, dis(gen)});
    }
    //cout << front.size() << " " <<middle.size() << " " << back.size() << endl;
        
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

    
    return 0;
}
