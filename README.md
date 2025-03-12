# ThreadSafe-Banking (hw1_bank)

## How to run

./run_nolocks.sh <num_accounts>  
./run_coarselocks.sh <num_accounts>  
./run_finelocks.sh <num_accounts>  
./run_uniquelocks.sh <num_accounts>  
./run_fastlocks.sh <num_accounts>  


Run any of the commands above in your terminal to see each program's execution time based on how it was implemented. Currently, the program only supports 3, 10, 20, and 60 for the number of accounts. Please enter one of those numbers then.

Example of a correct command: ./run_finelocks.sh 60

## Submission (Plots, etc.)

Official submission with multiple plots, execution times, corresponding code, and more

https://docs.google.com/spreadsheets/d/1mtUx7LQpBl23vn5DIu2pu2hS0-sxtfLj0VMvD3Qsl6U/edit?usp=sharing

## Notes

- This was run on a Sunlab machine with 16 CPUs (try 'less /proc/cpuinfo'), therefore any configuration with a higher number of parallel threads won't produce an actual parallel execution
- The Sunlab computers have a specific configuration that might not be replicable on other machines
- fastlocks.cpp is not a working implementation but my idea was to keep track of the amount of threads checking the total balance. If this reached 0, I can do a deposit. Since I did not implement it, I cannot tell if it this type of synchronization would be both correct and achieve speedup.
