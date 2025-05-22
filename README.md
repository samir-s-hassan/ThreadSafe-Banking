# ThreadSafe-Banking

ThreadSafe-Banking is a course assignment project that explores different synchronization techniques in a multi-threaded banking system. The goal is to evaluate performance and correctness when handling concurrent operations (like transfers and balance checks) across multiple bank accounts.

This project is part of a course assignment and is not licensed for public use.

## How to run

./run_nolocks.sh <num_accounts>  
./run_coarselocks.sh <num_accounts>  
./run_finelocks.sh <num_accounts>  
./run_uniquelocks.sh <num_accounts>  
./run_fastlocks.sh <num_accounts>  


Run any of the commands above in your terminal to see each program's execution time based on how it was implemented. Currently, the program only supports 3, 10, 20, and 60 for the number of accounts. Please enter one of those numbers then.

Example of a correct command: ./run_finelocks.sh 60

## Submission (Plots, etc.)

View the chart:
1. [On Google Sheets](https://docs.google.com/spreadsheets/d/1mtUx7LQpBl23vn5DIu2pu2hS0-sxtfLj0VMvD3Qsl6U/edit?usp=sharing)  
2. [Download File](./chart/chart.xlsx)

Official submission with multiple plots, execution times, corresponding code, and more

## Notes

- This was run on a Sunlab machine with 16 CPUs (try 'less /proc/cpuinfo'), therefore any configuration with a higher number of parallel threads won't produce an actual parallel execution
- The Sunlab computers have a specific configuration that might not be replicable on other machines
- fastlocks.cpp is not a working implementation but my idea was to keep track of the amount of threads checking the total balance. If this reached 0, I can do a deposit. Since I did not implement it, I cannot tell if it this type of synchronization would be both correct and achieve speedup.

## License

    Copyright 2025 Samir Hassan

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
