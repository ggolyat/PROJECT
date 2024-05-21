// 產生k=5的側資
#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>
#include <algorithm>
#include <thread>

double sp = 0.2;

const double total_count = 100000000;

template <typename T>
std::string to_string(T value) 
{
    std::ostringstream os;
    os << value;
    return os.str();
}

void generate_random_data(int index) 
{
    std::mt19937_64 gen(std::random_device{}());
    std::uniform_real_distribution<double> range1(0.0, 10000.0);
    std::uniform_real_distribution<double> range2(5000000.0, 5002000.0);
    std::uniform_real_distribution<double> range3(10000000.0, 10002000.0);
    std::uniform_real_distribution<double> range4(15000000.0, 15002000.0);
    std::uniform_real_distribution<double> range5(20000000.0, 20010000.0);


    double range_count = total_count * sp;

    std::vector<double> data;

    for (int i = 0; i < range_count; ++i) {
        data.push_back(range1(gen));
    }
    for (int i = 0; i < range_count; ++i) {
        data.push_back(range2(gen));
    }
    for (int i = 0; i < range_count; ++i) {
        data.push_back(range3(gen));
    }
    for (int i = 0; i < range_count; ++i) {
        data.push_back(range4(gen));
    }
    for (int i = 0; i < range_count; ++i) {
        data.push_back(range5(gen));
    }
    std::ofstream file("random_data_4_" +to_string(sp*100)+ "_"+ to_string(index) + ".bin", std::ios::out | std::ios::binary);

    //std::ofstream file("random_data_2_" + to_string(sp*100) + "_" + to_string(index) + ".bin", std::ios::out | std::ios::binary);
    for (const auto& value : data) 
    {
        file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    file.close();

    std::cout << "Random data generated and saved to random_data_4_" << to_string(sp*100) << index << ".bin" << std::endl;
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 100; ++i) 
    {
        threads.emplace_back(generate_random_data, i);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
