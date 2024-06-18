// ALEX4.0 1-3 舊range  bulkload 前中(分開)

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
    try 
    {
        auto keys = new KEY_TYPE[total_count];
        vector< pair<KEY_TYPE, PAYLOAD_TYPE> > front;
        vector< pair<KEY_TYPE, PAYLOAD_TYPE> > middle;
        vector< pair<KEY_TYPE, PAYLOAD_TYPE> > back;
        int temp = int((1.0-sp[ii]*2)*100 + 0.001);
        string keys_file_path = "testdata2/random_data_1_" + to_string(temp) + "_" + to_string(loop) + ".bin";
    
        if (!load_binary_data(keys, total_count, keys_file_path)) 
        {
            cout << "=== load fail!!! " << keys_file_path << endl;
            delete[] keys;
           
            return;
        }
        //sort(keys, keys + total_count);

        for(int i = 0; i < total_count*sp[ii]; i++)  
        {
            front.push_back({keys[i], dis(gen)});
        }
        for(int i = 0; i < total_count*(1.0-sp[ii]*2); i++)
        {
            middle.push_back({keys[ int(i+total_count*sp[ii])], dis(gen)});
        }
        for(int i = 0; i < total_count*sp[ii]; i++)
        {
            back.push_back({keys[ int(i+ total_count*(1.0-sp[ii]))], dis(gen)});
        }
        
        
        /////// bulkload ///////

        // Create ALEX index
        alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
        sort(front.begin(), front.begin() + int(total_count*sp[ii]));
        sort(middle.begin(), middle.begin() + int(total_count*(1.0-sp[ii]*2)));
        auto bulkload_start = chrono::high_resolution_clock::now();
        index.bulk_load(front.data(), int(total_count*sp[ii]) ); // front
        index.bulk_load(middle.data(), int(total_count*(1.0-sp[ii]*2)) ); // middle
        auto bulkload_end = chrono::high_resolution_clock::now();
        // delete[] middle;
        //sort(front.begin(), front.end(), comparePairs);


        /////// insert ///////
        
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
        int prev;
        // for(int i=0; i<total_count*(1.0-sp[ii]*2); i++)     
        // { 
        //     prev = index.stats_.num_expand_and_retrains;
        //     start_time = chrono::high_resolution_clock::now();
            
        //     index.insert(middle[i].first, middle[i].second);
        //     end_time = chrono::high_resolution_clock::now();
        //     //cout << front[i].first << front[i].second << " " << index.stats_.num_expand_and_retrains << "  " << prev << endl;
        //     if(index.stats_.num_expand_and_retrains != prev)        // resize happens
        //     {
        //         {
        //             lock_guard<mutex> lock(resize_mutex);
        //             resize[loop].push_back(double((i+total_count*sp[ii])/total_count));  //percentage
        //             insert_cost_time[loop].push_back(chrono::duration_cast<chrono::nanoseconds>(end_time - start_time).count());
        //             total_retrain++;
        //         }
        //         //cout << "expand & retrain at front inserting " << i << " : " << new_key << endl;
        //     }
            
        // }
        // delete[] front;
        // cout << "=== insert front ===\n";

        for(int i=0; i<total_count*sp[ii]; i++)     //back
        {
            prev = index.stats_.num_expand_and_retrains;
            start_time = chrono::high_resolution_clock::now();
            index.insert(back[i].first, back[i].second);
            end_time = chrono::high_resolution_clock::now();
            if(index.stats_.num_expand_and_retrains != prev)
            {
                {
                    lock_guard<mutex> lock(resize_mutex);
                    resize[loop].push_back(double( (i+ total_count*(1.0-sp[ii]))/total_count ));
                    insert_cost_time[loop].push_back(chrono::duration_cast<chrono::nanoseconds>(end_time - start_time).count());
                    //total_retrain++;
                }
                
                //cout << "expand & retrain at back inserting " << i << " : " << new_key << endl;
            }
        }
        // delete[] back;
        

        /////// lookup //////
        
        auto total_start_time = chrono::high_resolution_clock::now();
        for (int i = 0; i < total_count*sp[ii]; ++i) 
        {
            // Lookup operation
            KEY_TYPE key = front[i].first;
            index.find(key);
        }
        auto time_block1 = chrono::high_resolution_clock::now();
        for (int i = 0; i < total_count*(1.0-sp[ii]*2); ++i) 
        {
            // Lookup operation
            KEY_TYPE key = middle[i].first;
            index.find(key);
        }
        auto time_block2 = chrono::high_resolution_clock::now();
        for (int i = 0; i < total_count*sp[ii]; ++i) 
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
            total_retrain += index.stats_.num_expand_and_retrains;
            cout << loop << " expand & retrain: " << index.stats_.num_expand_and_retrains <<endl;
        }
        
        delete[] keys;
    } catch (const std::bad_alloc& e) 
    {
        std::cerr << "Memory allocation failed: " << e.what() << std::endl;
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
    cout << "front one lookup time: " <<  front_lookup_time/(100*total_count*sp[ii]) << " nanoseconds\n";
    cout << "middle one lookup time: " <<  middle_lookup_time/(100*total_count*(1.0-sp[ii]*2)) << " nanoseconds\n";
    cout << "back one lookup time: " <<  back_lookup_time/(100*total_count*sp[ii]) << " nanoseconds\n";

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
    cout << "total retrain: " << double(total_retrain)/100.0 << " (loc & time cost recoeded)\n";
    std::string filename = "result/1_3_" + to_string(int((1.0-sp[ii]*2)*100 + 0.001)) + "_insert_time.csv";
    exportToCSV(insert_cost_time, filename);
    plt::scatter(ploty1, ploty2);
    plt::save("result/1_3_" + to_string(int((1.0-sp[ii]*2)*100 + 0.001)) +"_mix.jpg");
    //plt::scatter(plotx, ploty1);
    //plt::save("result/1_3_" + to_string(int((1.0-sp[ii]*2)*100 + 0.001)) +"_location.jpg");
    //plt::scatter(plotx, ploty2);
    //plt::save("result/1_3_" + to_string(int((1.0-sp[ii]*2)*100 + 0.001)) +"_time.jpg");
    
    return 0;
}
