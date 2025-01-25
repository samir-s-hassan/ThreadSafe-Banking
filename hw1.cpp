#include <iostream>
#include <map>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <mutex>
#include <random>
#include <thread>
#include <chrono>
#include <future>

std::mutex bankMutex;    // declare a global mutex to protect the bank accounts (coarse-grained)
std::mutex balanceMutex; // mutex to protect balance calculation (coarse-grained)

// Courtesy of Nicholas Thomas, Generates a random int between min and max (inclusive)
int generateRandomInt(int min, int max)
{
    thread_local static std::random_device rd;         // creates random device (unique to each thread to prevent race cons) (static to avoid reinitialization)
    thread_local static std::mt19937 gen(rd());        // Seeding the RNG (unique to each thread to prevent race cons) (static to avoid reinitialization)
    std::uniform_int_distribution<> distrib(min, max); // Create uniform int dist between min and max (inclusive)
    return distrib(gen);                               // Generate random number from the uniform int dist (inclusive)
}

void deposit(std::map<int, float> &bankAccounts, int account1, int account2, float amount)
{
    // lock the mutex to ensure atomic access to the bank accounts
    // RAII-style lock, meaning that it will automatically release the lock when it goes out of scope (which happens at the end of the deposit function).
    std::lock_guard<std::mutex> lock(bankMutex);
    bankAccounts[account1] -= amount; // subtract from account1
    bankAccounts[account2] += amount; // add to account2
}

float balance(std::map<int, float> &bankAccounts)
{
    // lock the mutex to ensure atomic access to the bank accounts
    std::lock_guard<std::mutex> lock(balanceMutex);
    float totalBalance = 0.0f;
    // iterate over all the bank accounts and calculate the total balance
    for (const std::pair<const int, float> &account : bankAccounts)
    {
        totalBalance += account.second; // add the balance of each account
    }
    return totalBalance;
}

void do_work(std::map<int, float> &bankAccounts)
{
    std::vector<int> accountIDs;
    {
        // lock the mutex here so no other thread can access bankAccounts while we are working
        std::lock_guard<std::mutex> lock(bankMutex);
        for (const auto &account : bankAccounts)
        {
            accountIDs.push_back(account.first); // collect all account IDs into this vector
        }
    }
    // chose 5 for my "some number of iterations"
    for (int i = 0; i < 5; ++i)
    {
        if (generateRandomInt(0, 99) < 95) // 95% probability for deposit
        {
            int randomIndex1 = generateRandomInt(0, accountIDs.size() - 1);
            int randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            while (randomIndex1 == randomIndex2)
            {
                randomIndex2 = generateRandomInt(0, accountIDs.size() - 1); // make sure indices are different
            }
            {
                // lock the mutex before the deposit to ensure atomicity
                std::lock_guard<std::mutex> lock(bankMutex);
                deposit(bankAccounts, accountIDs[randomIndex1], accountIDs[randomIndex2], 5000.0f);
            }
        }
        else // 5% probability for balance
        {
            {
                // lock the mutex before reading the balance
                std::lock_guard<std::mutex> lock(bankMutex);
                balance(bankAccounts);
            }
        }
    }
}

int main()
{
    // Step 1: Define a map where each account has a unique ID (int) and a balance (float)
    std::map<int, float> bankAccounts;
    std::cout << std::endl;

    // Step 2.0: creating different float arrays such that I can work with whichever one to see different contention effects
    float values1[1] = {100000.0f};                                        // 1 value array
    float values3[3] = {40000.0f, 30000.0f, 30000.0f};                     // 3 values array
    float values5[5] = {10000.0f, 15000.0f, 20000.0f, 25000.0f, 30000.0f}; // 5 values array
    float values10[10] = {10000.0f, 8000.0f, 12000.0f, 9000.0f, 15000.0f,
                          7000.0f, 13000.0f, 6000.0f, 11000.0f, 9000.0f}; // 10 values array
    float values20[20] = {5000.0f, 1000.0f, 4000.0f, 6000.0f, 5000.0f,
                          4000.0f, 6000.0f, 4000.0f, 5000.0f, 2000.0f,
                          4000.0f, 9000.0f, 5000.0f, 4000.0f, 5000.0f,
                          5000.0f, 4000.0f, 6000.0f, 7000.0f, 9000.0f}; // 20 values array
    float values60[60] = {12400.0f, 2000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f,
                          1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f, 1500.0f, 1200.0f,
                          1800.0f, 2200.0f, 1700.0f, 1000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f,
                          1700.0f, 1000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f,
                          1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f, 2500.0f, 1200.0f,
                          1800.0f, 2200.0f, 1700.0f, 1000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f,
                          1700.0f, 1000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f}; // 60 values array

    // Step 2.1: choosing an array to use and populating it. edit loop iterations and array name
    for (int i = 0; i < 3; ++i)
    {
        bankAccounts.insert({i + 1, values3[i]});
    }
    // Check if the sum is correct
    float initialBalance = balance(bankAccounts);
    if (initialBalance != 100000.0f)
    {
        std::cout << "Error: Initial balance is inconsistent!" << std::endl;
    }

    // Step 6: Multi-threading
    const int numThreads = 4; // CHANGE number of threads to create as needed
    std::vector<std::thread> threads;
    std::vector<std::promise<float>> promises(numThreads); // promises to store exec_time_i
    std::vector<std::future<float>> futures;               // futures to retrieve exec_time_i
    // link the promises to futures
    for (auto &promise : promises)
    {
        futures.push_back(promise.get_future());
    }
    auto start = std::chrono::high_resolution_clock::now(); // start measuring TOTAL TIME
    // spawn the threads from our main thread
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([&, t]()
                             {
                                 auto thread_start = std::chrono::high_resolution_clock::now();
                                 do_work(bankAccounts);
                                 auto thread_end = std::chrono::high_resolution_clock::now();
                                 std::chrono::duration<float> thread_duration = thread_end - thread_start;
                                 promises[t].set_value(thread_duration.count()); // store time in promise
                             });
    }
    // join all threads
    for (auto &thread : threads)
    {
        thread.join();
    }
    auto end = std::chrono::high_resolution_clock::now(); // end measuring TOTAL TIME
    std::chrono::duration<float> duration = end - start;
    float multi_threaded_time = duration.count();
    // print execution times
    float totalExecutionTime = 0.0f;
    for (auto &future : futures)
    {
        float exec_time_i = future.get();
        totalExecutionTime += exec_time_i;
        std::cout << "Thread execution time: " << exec_time_i << " seconds" << std::endl;
    }
    // verify final balance
    float finalBalance = balance(bankAccounts);
    if (finalBalance != 100000.0f)
    {
        std::cout << "Error: Final balance is inconsistent!" << std::endl;
    }
    std::cout << "Total execution time: " << totalExecutionTime << " seconds\n"
              << std::endl;
    return 0;
}

// std::map has automatic uniqueness, efficient lookups, ordered keys, and memory management
// std::unordered_map is fast for lookups, you lose the automatic sorting of keys (account IDs) that you get with std::map
// std::vector is simple, but it's inefficient for large numbers of accounts due to the linear search for lookups. managing uniqueness and ordering would add extra work
// std::list has additional overhead for managing a doubly-linked list and manually manage ordering and uniqueness
// std:array or std::vector with fixed size would need account IDs need to be mapped to valid array indices, and the size must be known beforehand or managed manually

// Step 3
// std::srand(std::time(0)); // Seed the random number generator with current time
// // Get account IDs into a vector for random selection
// std::vector<int> accountIDs;
// for (const auto &account : bankAccounts)
// {
//     accountIDs.push_back(account.first); // Add account IDs to vector
// }

// // generate two random indices to select accounts
// int randomIndex1 = std::rand() % accountIDs.size(); // random index between 0 and size-1
// int randomIndex2 = std::rand() % accountIDs.size(); // another random index
// while (randomIndex1 == randomIndex2)
// { // make sure both indices are not the same
//     randomIndex2 = std::rand() % accountIDs.size();
// }
// // perform the deposit
// deposit(bankAccounts, accountIDs[randomIndex1], accountIDs[randomIndex2], 5000.0f);
// // print the updated balances
// std::cout << "Updated bank account balances:" << std::endl;
// for (const auto &account : bankAccounts)
// {
//     std::cout << "Account ID: " << account.first << ", Balance: " << account.second << std::endl;
// }

// // Step 4
// float finalBalance = balance(bankAccounts);
// std::cout << "Total balance of all accounts after deposit: " << finalBalance << std::endl;
// std::cout << "" << std::endl;
