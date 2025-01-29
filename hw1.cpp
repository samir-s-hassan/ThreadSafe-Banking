#include <iostream>
#include <map>
#include <unordered_map>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <mutex>
#include <random>
#include <thread>
#include <chrono>
#include <future>
#include <shared_mutex>

std::mutex bankMutex;    // declare a global mutex to protect the bank accounts (coarse-grained)
std::shared_mutex balanceMutex; // mutex to protect balance calculation (coarse-grained)
std::unordered_map<int, std::mutex> accountMutexes;
; // per-account mutex map (fine-grained)

// Define constants to change easily
const int NUM_ACCOUNTS = 5;     // Change number of bank accounts here, change line 115 too
const int NUM_ITERATIONS = 100; // Change number of iterations in do_work here
const int NUM_THREADS = 10;     // Change number of threads here

// DONE, single-threaded and multi-threaded
int generateRandomInt(int min, int max)
{
    thread_local static std::random_device rd;         // creates random device (unique to each thread to prevent race cons) (static to avoid reinitialization)
    thread_local static std::mt19937 gen(rd());        // Seeding the RNG (unique to each thread to prevent race cons) (static to avoid reinitialization)
    std::uniform_int_distribution<> distrib(min, max); // Create uniform int dist between min and max (inclusive)
    return distrib(gen);                               // Generate random number from the uniform int dist (inclusive)
}

// DONE, single-threaded application
void single_deposit(std::map<int, float> &bankAccounts, int account1, int account2, float amount)
{
    // if (bankAccounts[account1] < amount)
    // {
    //     throw std::runtime_error("Insufficient funds."); // check before withdrawal
    // }

    bankAccounts[account1] -= amount; // safe to subtract now
    bankAccounts[account2] += amount; // add to account2
}

// WIP, multi-threaded application
void deposit(std::map<int, float> &bankAccounts, int account1, int account2, float amount)
{
    // lock both account mutexes at the same time to avoid deadlock and automatically unlocks when function exits, INCORPORATE DETERMINISTIC ORDER
    std::scoped_lock lock(accountMutexes[account1], accountMutexes[account2]);

    // if (bankAccounts[account1] < amount)
    // {
    //     throw std::runtime_error("Insufficient funds.");
    // }

    bankAccounts[account1] -= amount;
    bankAccounts[account2] += amount;
}

// DONE, single-threaded application
float single_balance(std::map<int, float> &bankAccounts)
{
    float totalBalance = 0.0f;
    // iterate over all the bank accounts and calculate the total balance
    for (const std::pair<const int, float> &account : bankAccounts)
    {
        totalBalance += account.second; // add the balance of each account
    }
    return totalBalance;
}

// WIP, for multi-threaded application
float balance(std::map<int, float> &bankAccounts)
{
    std::shared_lock<std::shared_mutex> lock(balanceMutex); // shared lock allows concurrent reads
    float totalBalance = 0.0f;
    // iterate over all the bank accounts and calculate the total balance
    for (const std::pair<const int, float> &account : bankAccounts)
    {
        totalBalance += account.second; // add the balance of each account
    }
    return totalBalance;
    // if performance becomes an issue due to mutex, use a shared_mutex, IMPROVE LATER
}

// DONE
// for single_threaded application
float single_do_work(std::map<int, float> &bankAccounts)
{
    std::vector<int> accountIDs;
    // if bankAccounts is large, collecting all the account IDs into a vector incurs some overhead as you're copying all keys into a separate vector. If performance is a concern, consider using iterators or direct indexing rather than copying keys to a vector. for most typical sizes, this is acceptable.
    for (const auto &account : bankAccounts)
    {
        accountIDs.push_back(account.first);
    }

    auto loop_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_ITERATIONS; ++i)
    {
        if (generateRandomInt(0, 99) < 95) // 95% probability for deposit
        {
            int randomIndex1 = generateRandomInt(0, accountIDs.size() - 1);
            int randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            while (randomIndex1 == randomIndex2)
            {
                randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            }
            single_deposit(bankAccounts, accountIDs[randomIndex1], accountIDs[randomIndex2], 5000.0f);
        }
        else // 5% probability for balance
        {
            single_balance(bankAccounts);
        }
    }
    auto loop_end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(loop_end - loop_start).count();
}

float do_work(std::map<int, float> &bankAccounts)
{
    std::vector<int> accountIDs;
    // if bankAccounts is large, collecting all the account IDs into a vector incurs some overhead as you're copying all keys into a separate vector. If performance is a concern, consider using iterators or direct indexing rather than copying keys to a vector. for most typical sizes, this is acceptable.
    { // lock only while collecting account IDs
        std::lock_guard<std::mutex> lock(bankMutex);
        for (const auto &account : bankAccounts)
        {
            accountIDs.push_back(account.first);
        }
    }
    auto loop_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_ITERATIONS; ++i)
    {
        if (generateRandomInt(0, 99) < 95) // 95% probability for deposit
        {
            int randomIndex1 = generateRandomInt(0, accountIDs.size() - 1);
            int randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            while (randomIndex1 == randomIndex2)
            {
                randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            }
            {
                std::lock_guard<std::mutex> lock(bankMutex);
                deposit(bankAccounts, accountIDs[randomIndex1], accountIDs[randomIndex2], 5000.0f);
            }
        }
        else // 5% probability for balance
        {
            std::lock_guard<std::mutex> lock(bankMutex);
            balance(bankAccounts);
        }
    }
    auto loop_end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(loop_end - loop_start).count();
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
    for (int i = 0; i < NUM_ACCOUNTS; ++i)
    {
        bankAccounts.insert({i + 1, values5[i]});
    }
    // Check if the sum is correct
    float initialBalance = balance(bankAccounts);
    if (initialBalance != 100000.0f)
    {
        std::cout << "Error: Initial balance is inconsistent!" << std::endl;
    }

    // Step 6: Multi-threading
    std::vector<std::thread> threads;
    std::vector<std::promise<float>> promises(NUM_THREADS); // promises to store exec_time_i, execution time
    std::vector<std::future<float>> futures;                // futures to retrieve exec_time_i
    // link the promises to futures
    for (auto &promise : promises)
    {
        futures.push_back(promise.get_future());
    }
    // spawn the threads from our main thread
    for (int t = 0; t < NUM_THREADS; ++t)
    {
        threads.emplace_back([&, t]()
                             {
                                 // measure our do_work time
                                 float exec_time = do_work(bankAccounts);
                                 promises[t].set_value(exec_time); // store time in promise
                             });
    }
    // join all threads
    for (auto &thread : threads)
    {
        thread.join();
    }
    // print execution times
    float maxExecutionTime = 0.0f;
    for (auto &future : futures)
    {
        float exec_time_i = future.get();
        std::cout << "Thread execution time: " << exec_time_i << " seconds" << std::endl;
        if (exec_time_i > maxExecutionTime)
        {
            maxExecutionTime = exec_time_i; // update the max execution time
        }
    }
    // verify final balance
    float finalBalance = balance(bankAccounts);
    if (finalBalance != 100000.0f)
    {
        std::cout << "Error: Final balance is inconsistent!" << std::endl;
    }

    // Step 7: Single-threaded execution
    // same number of iterations as multi-threaded execution
    float total_exec_time_single = 0.0f;
    for (int i = 0; i < (NUM_THREADS * NUM_ITERATIONS); ++i)
    {
        // do_work for a single thread
        total_exec_time_single += single_do_work(bankAccounts);
    }
    std::cout << "\nMax multi-threaded execution time: " << maxExecutionTime << " seconds\n";
    std::cout << "Single-threaded execution time:    " << total_exec_time_single << " seconds\n";
    // calculate and print the performance difference
    float performance_ratio = total_exec_time_single / maxExecutionTime;
    if (performance_ratio > 1)
    {
        std::cout << "\nThe multi-threaded performance is " << performance_ratio << " times faster than the single-threaded performance.\n\n";
    }
    else
    {
        std::cout << "\nThe multi-threaded performance is " << (1 / performance_ratio) << " times slower than the single-threaded performance.\n\n";
    }
    // remove all elements from the map
    bankAccounts.clear();
    return 0;
}