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
#include <atomic>

std::shared_mutex balanceMutex;
std::unordered_map<int, std::mutex> accountMutexes; // Per-account mutex map (fine-grained)
std::atomic<float> globalBalance = 100000.0f;       // Tracks global balance atomically
std::atomic<int> balanceRunning(0);                 // Tracks active balance computations

int generateRandomInt(int min, int max)
{
    thread_local static std::random_device rd;         // creates random device (unique to each thread to prevent race cons) (static to avoid reinitialization)
    thread_local static std::mt19937 gen(rd());        // Seeding the RNG (unique to each thread to prevent race cons) (static to avoid reinitialization)
    std::uniform_int_distribution<> distrib(min, max); // Create uniform int dist between min and max (inclusive)
    return distrib(gen);                               // Generate random number from the uniform int dist (inclusive)
}

std::vector<float> getInitialBalances(int num_accounts)
{
    if (num_accounts == 3)
    {
        return {40000.0f, 30000.0f, 30000.0f};
    }
    else if (num_accounts == 10)
    {
        return {10000.0f, 8000.0f, 12000.0f, 9000.0f, 15000.0f,
                7000.0f, 13000.0f, 6000.0f, 11000.0f, 9000.0f}; // 10 values array
    }
    else if (num_accounts == 20)
    {
        return {5000.0f, 1000.0f, 4000.0f, 6000.0f, 5000.0f,
                4000.0f, 6000.0f, 4000.0f, 5000.0f, 2000.0f,
                4000.0f, 9000.0f, 5000.0f, 4000.0f, 5000.0f,
                5000.0f, 4000.0f, 6000.0f, 7000.0f, 9000.0f}; // 20 values array
    }
    else if (num_accounts == 60)
    {
        return {12400.0f, 2000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f,
                1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f, 1500.0f, 1200.0f,
                1800.0f, 2200.0f, 1700.0f, 1000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f,
                1700.0f, 1000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f,
                1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f, 2500.0f, 1200.0f,
                1800.0f, 2200.0f, 1700.0f, 1000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f,
                1700.0f, 1000.0f, 1500.0f, 1200.0f, 1800.0f, 2200.0f, 1700.0f, 1000.0f}; // 60 values array
    }
    else
    {
        std::cerr << "Error: Unsupported number of accounts. Please choose either 3, 10, 20, or 60.\n";
        return {};
    }
}

void single_deposit(std::map<int, float> &bankAccounts, int account1, int account2, float amount)
{
    // check if the account1 has enough funds (greater than amount)
    if (bankAccounts[account1] > amount)
    {
        // Perform the deposit only if there are sufficient funds
        bankAccounts[account1] -= amount;
        bankAccounts[account2] += amount;
    }
}

void deposit(std::map<int, float> &bankAccounts, int account1, int account2, float amount)
{
    std::unique_lock<std::mutex> lock1(accountMutexes[low], std::defer_lock);
    std::unique_lock<std::mutex> lock2(accountMutexes[high], std::defer_lock);
    std::lock(lock1, lock2);

    if (bankAccounts[account1] < amount)
        return;

    bankAccounts[account1] -= amount;
    bankAccounts[account2] += amount;

    // Use atomic to update globalBalance atomically.
    {
        float currentBalance = globalBalance.load(std::memory_order_relaxed);             // Load atomically
        globalBalance.store(currentBalance - amount + amount, std::memory_order_relaxed); // Update atomically
    }
}

float single_balance(std::map<int, float> &bankAccounts)
{
    float total = 0.0f;
    for (const auto &account : bankAccounts)
    {
        total += account.second; // sum up the balances of all accounts
    }
    return total;
}

float balance(std::map<int, float> &bankAccounts)
{
    std::shared_lock<std::shared_mutex> lock(balanceMutex);
    balanceRunning.fetch_add(1, std::memory_order_relaxed); // Increment balanceRunning when starting balance calculation

    float total = 0.0f;
    for (const auto &account : bankAccounts)
    {
        total += account.second; // Sum up the balances of all accounts
    }

    balanceRunning.fetch_sub(1, std::memory_order_relaxed); // Decrement balanceRunning when done calculating balance
    return total;
}

float single_do_work(std::map<int, float> &bankAccounts, int numIterations)
{
    std::vector<int> accountIDs;
    // collect account IDs (single-threaded, no locks needed)
    for (const auto &account : bankAccounts)
    {
        accountIDs.push_back(account.first);
    }

    auto loop_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numIterations; ++i)
    {
        if (generateRandomInt(0, 99) < 95) // 95% probability for deposit
        {
            int randomIndex1 = generateRandomInt(0, accountIDs.size() - 1);
            int randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            while (randomIndex1 == randomIndex2)
            {
                randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            }
            int acc1 = accountIDs[randomIndex1];
            int acc2 = accountIDs[randomIndex2];
            // perform deposit transaction
            single_deposit(bankAccounts, acc1, acc2, 5000.0f);
        }
        else // 5% probability for balance check
        {
            single_balance(bankAccounts);
        }
    }

    auto loop_end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(loop_end - loop_start).count();
}

float do_work(std::map<int, float> &bankAccounts, int numIterations, int numThreads)
{
    std::vector<int> accountIDs;
    // collect all account IDs without locking. step is done outside the critical section to avoid unnecessary locking.
    {
        for (const auto &account : bankAccounts)
        {
            accountIDs.push_back(account.first);
        }
    }

    auto loop_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numIterations / numThreads; ++i)
    {
        if (generateRandomInt(0, 99) < 95) // 95% probability for deposit
        {
            int randomIndex1 = generateRandomInt(0, accountIDs.size() - 1);
            int randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            while (randomIndex1 == randomIndex2)
            {
                randomIndex2 = generateRandomInt(0, accountIDs.size() - 1);
            }
            int account1 = accountIDs[randomIndex1];
            int account2 = accountIDs[randomIndex2];
            // Perform the deposit operation
            deposit(bankAccounts, account1, account2, 5000.0f);
        }
        else // 5% probability for balance
        {
            balance(bankAccounts);
        }
    }

    auto loop_end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(loop_end - loop_start).count();
}

int main(int argc, char *argv[])
{
    // Step 1: Parse the command-line arguments to set NUM_ACCOUNTS, NUM_THREADS, and NUM_ITERATIONS
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <num_accounts> <num_threads> <num_iterations>" << std::endl;
        return 1;
    }

    const int NUM_ACCOUNTS = std::stoi(argv[1]);
    const int NUM_THREADS = std::stoi(argv[2]);
    const int NUM_ITERATIONS = std::stoi(argv[3]);

    // Step 2: Define a map where each account has a unique ID (int) and a balance (float)
    std::map<int, float> bankAccounts;
    std::cout << std::endl;

    // Step 2.0: creating different float arrays such that I can work with whichever one to see different contention effects
    std::vector<float> initialBalances = getInitialBalances(NUM_ACCOUNTS);

    // Step 2.1: choosing an array to use and populating it
    float initialBalanceSum = 0;
    for (int i = 0; i < NUM_ACCOUNTS; ++i)
    {
        bankAccounts[i + 1] = initialBalances[i];
        initialBalanceSum += initialBalances[i];
        accountMutexes[i];
    }
    // Check if the sum is correct
    if (initialBalanceSum != 100000.0f)
    {
        std::cout << "Error: Initial balance is inconsistent!  " << static_cast<int>(initialBalanceSum) << std::endl;
    }

    // Print the current configuration
    std::cout << "Running with NUM_ACCOUNTS = " << NUM_ACCOUNTS
              << ", NUM_THREADS = " << NUM_THREADS
              << ", NUM_ITERATIONS = " << NUM_ITERATIONS << std::endl;

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
                                 float exec_time = do_work(bankAccounts, NUM_ITERATIONS, NUM_THREADS);
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
        // std::cout << "Thread execution time: " << exec_time_i * 1000 << " milliseconds" << std::endl;
        if (exec_time_i > maxExecutionTime)
        {
            maxExecutionTime = exec_time_i; // update the max execution time
        }
    }
    // verify final balance
    float finalBalance = balance(bankAccounts);
    if (finalBalance != 100000.0f)
    {
        std::cout << "Error: Final balance is inconsistent!  " << static_cast<int>(finalBalance) << std::endl; // Display the inconsistent balance
    }

    // Step 7: Single-threaded execution

    // do_work for a single thread
    float total_exec_time_single = single_do_work(bankAccounts, NUM_ITERATIONS);
    std::cout << "\nMax multi-threaded execution time: " << maxExecutionTime * 1000 << " milliseconds\n";
    std::cout << "Single-threaded execution time:    " << total_exec_time_single * 1000 << " milliseconds\n";
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
    std::cout << "<----------------------------------------------------------------------->" << std::endl;
    // remove all elements from the map
    bankAccounts.clear();
    return 0;
}