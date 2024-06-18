// ALEX5.0 1-1 1-2: create十萬 固定K=3 分/不分離群值   

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

void exportToCSV(const std::vector<std::vector<double>>& data, const std::string& filename) {
    std::ofstream csvFile(filename);

    if (!csvFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            csvFile << row[i];
            if (i < row.size() - 1) {
                csvFile << ","; // Add comma between elements
            }
        }
        csvFile << "\n"; // Add newline character at the end of each row
    }

    csvFile.close();
    std::cout << "Data exported to CSV file: " << filename << std::endl;
}

void process_loop(int ii, int loop) 
{
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > front;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > middle;
    vector< pair<KEY_TYPE, PAYLOAD_TYPE> > back;
    // 設置隨機數生成器和分布
    std::default_random_engine generator(loop);
    std::uniform_real_distribution<double> distribution(0.0, 1000000.0);

    // 生成 100,000 個在 0 到 1,000,000 范圍內的隨機值
    int n_samples = 100'000;
    std::vector<Point> data(n_samples);
    for (int i = 0; i < n_samples; ++i) 
    {
        data[i] = {distribution(generator), -1};  // 初始時不分配聚類
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

    // 檢查每個群的大小是否超過總數據集的 50%
    for (int i = 0; i < k; ++i) 
    {
        if (clusters[i].size() > n_samples / 2) 
        {
            std::cout << "Cluster " << i << " exceeds 50% of the total data size." << std::endl;
            return;  // 結束程序，因為不符合條件
        }
    }

    for (int i = 0; i < 3; ++i) 
    {
        // 計算每個數據點到聚類中心的距離，並存儲在 `distances` 向量中
        std::vector<std::pair<double, Point>> distances;
        for (const auto& point : clusters[i]) {
            double dist = std::abs(point.value- centers[i]);
            distances.push_back({dist, point});
        }

        // 按距離排序
        std::sort(distances.begin(), distances.end(), [](const auto& a, const auto& b) {
            return a.first > b.first; // 按距離降序排序
        });

        // 移除最遠的 5% 數據點
        int num_to_remove = (distances.size() * 5) / 100;
        distances.erase(distances.begin(), distances.begin() + num_to_remove);

        // 更新聚類中的成員
        clusters[i].clear();
        for (const auto& [dist, point] : distances) {
            clusters[i].push_back(point);
        }
    }



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
        // delete[] middle;
        //sort(front.begin(), front.end(), comparePairs);
        

    /////// lookup //////
    
    auto total_start_time = chrono::high_resolution_clock::now();
    for (int i = 0; i < front.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = front[i].first;
        index.find(key);
    }
    auto time_block1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < middle.size(); ++i) 
    {
        // Lookup operation
        KEY_TYPE key = middle[i].first;
        index.find(key);
    }
    auto time_block2 = chrono::high_resolution_clock::now();
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
        front_lookup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(time_block1 - total_start_time).count();
        middle_lookup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(time_block2 - time_block1).count();
        back_lookup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_end_time - time_block2).count();
        bulkload_time += std::chrono::duration_cast<std::chrono::nanoseconds>(bulkload_end - bulkload_start).count();
        //total_retrain += index.stats_.num_expand_and_retrains;
     
        cout << loop << " total lookup time: " << total_lookup_time <<endl;
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
    //cout << "front one lookup time: " <<  front_lookup_time/(100*front.size()) << " nanoseconds\n";
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
    //cout << "total retrain: " << double(total_retrain)/100.0 << " (loc & time cost recoeded)\n";
    //std::string filename = "result/1_4_" + to_string(int((1.0-sp[ii]*2)*100 + 0.001)) + "_insert_time.csv";
    //exportToCSV(insert_cost_time, filename);
    //plt::scatter(ploty1, ploty2);
    //plt::save("result/1_4_" + to_string(int((1.0-sp[ii]*2)*100 + 0.001)) +"_mix.jpg");
    //plt::scatter(plotx, ploty1);
    //plt::save("result/1_3_" + to_string(int((1.0-sp[ii]*2)*100 + 0.001)) +"_location.jpg");
    //plt::scatter(plotx, ploty2);
    //plt::save("result/1_3_" + to_string(int((1.0-sp[ii]*2)*100 + 0.001)) +"_time.jpg");
    
    return 0;
}
