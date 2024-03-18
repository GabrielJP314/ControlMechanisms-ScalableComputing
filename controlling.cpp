#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <mutex>
#include <string>
#include <algorithm>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <functional>
#include <atomic>

using namespace std;

int MAX_PRIMES = 1000; // Mudar este numero caso deseje imprimir mais ou menos primos

mutex queueMutex;
condition_variable cv;
queue<pair<int, int>> tasks;
atomic<bool> done(false); // Use atomic para garantir visibilidade entre as threads

bool isPrime(int num) {
    if (num <= 1) return false;
    if (num <= 3) return true;
    if (num % 2 == 0 || num % 3 == 0) return false;

    // Verifica se o número é divisível por 6k +/- 1
    for (int i = 5; i * i <= num; i += 6) {
        if (num % i == 0 || num % (i + 2) == 0) return false;
    }
    return true;
}


void workerFunction(vector<int>& primes, mutex& mtx, atomic<int>& count) {
    // Process tasks until there are no more
    
    vector<int> localPrimes;
    while (true) {
        pair<int, int> task;
        {
            unique_lock<mutex> lock(queueMutex);
            cv.wait(lock, [] { return !tasks.empty() || done; });
            if (tasks.empty() && done) break; // Ends only if there are no tasks and done = true
            task = tasks.front();
            tasks.pop();
        }

        // Process the task
        for (int i = task.first; i <= task.second; i++) {
            if (isPrime(i)) {
                localPrimes.push_back(i);
            }
        }

        // Merge the local results into the shared vector if there are any
        if (!localPrimes.empty()) {
            
            // Lock the shared vector and merge the local vector
            lock_guard<mutex> guard(mtx);
            primes.insert(primes.end(), localPrimes.begin(), localPrimes.end());
            
            // No need to lock the atomic variable, it's guaranteed to be atomic
            count += localPrimes.size();

            // Clear the local vector for the next task
            localPrimes.clear();
        }
    }
}

void addTasksDynamically(int maxNum, int chunkSize) {
 
    for (int start = 1; start <= maxNum; start += chunkSize) {
        int end = std::min(start + chunkSize - 1, maxNum);
        {
            lock_guard<mutex> lock(queueMutex);
            tasks.push({start, end});
        }
        cv.notify_one(); // Notify one thread at a time
    }
    done = true; // Set done to true to signal the worker threads to finish
    cv.notify_all(); // Notify all threads that there are no more tasks
}

void mainFunctionBalanced(int maxNum, int numThreads){
    auto initTotal = chrono::high_resolution_clock::now();
    vector<int> primes;
    mutex mtx;
    atomic<int> count(0);
    int chunkSize = 50; // Hyperparameter to control the chunk size

    // reset the done flag
    done = false;

    // Create the worker threads
    auto startTime = chrono::high_resolution_clock::now();
    vector<thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(thread(workerFunction, ref(primes), ref(mtx), ref(count)));
    }

    // Add tasks to the queue dynamically
    thread taskAdder(addTasksDynamically, maxNum, chunkSize);

    
    // Wait for the taskAdder thread to finish
    taskAdder.join();

    for (auto& t : threads) {
        t.join();
    }

    auto endTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

    // Write the primes to a file
    ofstream outfile("primes_balanced.txt");
    if (outfile.is_open()) {
        for (int prime : primes) {
            outfile << prime << '\n';
        }
        outfile.close();
    }

    // Write the time to a file
    ofstream outfile2("balanced.txt", ios::app);
    if (outfile2.is_open())
    {
        outfile2<< duration.count() << endl;
        outfile2.close();
    }

    std::cout << "Número de threads: " << numThreads << endl;
    std::cout << "Tempo de execução das threads: " << duration.count() << " miliseconds" << endl;
    std::cout << "Quantidade de números avaliados: " << maxNum << endl;
    std::cout << "Quantidade de números primos encontrados: " << count.load() << endl;
    
    // Print the primes if the count is less than 1000 to avoid flooding the console
    if (count.load() < MAX_PRIMES) {
        for (int prime : primes) {
            std::cout << prime << " ";
        }
        std::cout << std::endl;
    }
    auto endTotal = chrono::high_resolution_clock::now();
    auto timeSpanTotal = chrono::duration_cast<chrono::milliseconds>(endTotal - initTotal);
    cout << "Tempo total de execução: " << timeSpanTotal.count() << " miliseconds" << endl;


}

void processRange(int start, int end, vector<int>& primes, mutex& mtx, atomic<int>& count)
{
    // Process a range of number checking for primes
    vector<int> localPrimes;
    for (int i = start; i <= end; i++)
    {
        if (isPrime(i))
        {
            // Writes primes to a local vector to avoid overhead of locking the shared vector
            localPrimes.push_back(i);
        }
    }

    // Lock the shared vector and merge the local vector
    lock_guard<mutex> guard(mtx);
    primes.insert(primes.end(), localPrimes.begin(), localPrimes.end());
    
    // No need to lock the atomic variable, it's guaranteed to be atomic
    count += localPrimes.size();
}

// Main function to control the threads
void mainFunction(int maxNum, int numThreads)
{

    auto initTotal = chrono::high_resolution_clock::now();
    // Shared vector to store the primes
    vector<int> primes;
    
    // Mutex to protect the shared vector
    mutex mtx;
    
    // Atomic variable to store the count of primes
    std::atomic<int> count = 0; 
    
    // In the unbalanced version, the chunk is just the number of numbers divided by the number of threads
    // given sequentially to each thread
    int chunkSize = maxNum / numThreads;

    auto startTime = chrono::high_resolution_clock::now();
    vector<thread> threads;
    for (int i = 0; i < numThreads; i++)
    {
        int start = i * chunkSize + 1;
        int end = (i == numThreads - 1) ? maxNum : (i + 1) * chunkSize;
        threads.push_back(thread(processRange, start, end, ref(primes), ref(mtx), ref(count)));
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
    auto endTime = chrono::high_resolution_clock::now();
    auto timeSpan = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

    // Write the primes to a file
    ofstream outfile("primes_unbalanced.txt");
    if (outfile.is_open())
    {
        for (int prime : primes)
        {
            outfile << prime << endl;
        }
        outfile.close();
    }

    // Write the time to a file
    ofstream outfile2("unbalanced.txt", ios::app);
    if (outfile2.is_open())
    {
        outfile2<< timeSpan.count() << endl;
        outfile2.close();
    }

    // Print the results
    cout << "Número de threads: " << numThreads << endl;
    cout << "Tempo total de execução das threads: " << timeSpan.count() << " miliseconds" << endl;
    cout << "Quantidade de números avaliados: " << maxNum << endl;
    cout << "Quantidade de números primos encontrados: " << count << endl;
    
    // Print the primes if the count is less than 1000 to avoid flooding the console
    if (count < MAX_PRIMES) 
    {
        for (int prime : primes) {
            cout << prime;
        }
        cout<<endl;
    }
    auto endTotal = chrono::high_resolution_clock::now();
    auto timeSpanTotal = chrono::duration_cast<chrono::milliseconds>(endTotal - initTotal);
    cout << "Tempo de total execução: " << timeSpanTotal.count() << " miliseconds" << endl;
}

int main() {
    ofstream outfile("unbalanced.txt");
    if (outfile.is_open()){
        outfile << "";
        outfile.close();
    }

    ofstream outfile2("balanced.txt");
    if (outfile2.is_open()){
        outfile2 << "";
        outfile2.close();
    }
    for (int numThreads = 1; numThreads <= 10; numThreads++) {
        for (int i = 0; i < 45; i++) {
            // Run the balanced and unbalanced versions multiple times to compare the results
            mainFunction(1000000, numThreads);
            mainFunctionBalanced(1000000, numThreads);
        
        }
    }
}















// #include <iostream>
// #include <vector>
// #include <thread>
// #include <fstream>
// #include <mutex>
// #include <string>
// #include <algorithm>
// #include <chrono>
// #include <queue>
// #include <condition_variable>
// #include <functional>
// #include <atomic>

// using namespace std;

// // Function to check if a number is prime

// mutex queueMutex;
// condition_variable cv;
// queue<pair<int, int>> tasks;

// bool done = false;

// bool isPrime(int num)
// {
//     if (num <= 1) return false;
//     if (num <= 3) return true;
//     if (num % 2 == 0 || num % 3 == 0) return false;
//     for (int i = 5; i * i <= num; i += 6)
//     {
//         if (num % i == 0 || num % (i + 2) == 0) return false;
//     }
//     return true;
// }

// // Function to process a range of numbers, that each thread will execute
// void processRange(int start, int end, vector<int>& primes, mutex& mtx, atomic<int>& count)
// {
//     vector<int> localPrimes;
//     for (int i = start; i <= end; i++)
//     {
//         if (isPrime(i))
//         {
//             localPrimes.push_back(i);
//         }
//     }
//     lock_guard<mutex> guard(mtx);
//     primes.insert(primes.end(), localPrimes.begin(), localPrimes.end());
//     count += localPrimes.size();
// }

// // Main function to control the threads
// void mainFunction(int maxNum, int numThreads)
// {

//     vector<int> primes;
//     mutex mtx;
//     std::atomic<int> count = 0; 
//     int chunkSize = maxNum / numThreads;

//     vector<thread> threads;
//     for (int i = 0; i < numThreads; i++)
//     {
//         int start = i * chunkSize + 1;
//         int end = (i == numThreads - 1) ? maxNum : (i + 1) * chunkSize;
//         threads.push_back(thread(processRange, start, end, ref(primes), ref(mtx), ref(count)));
//     }

//     auto startTime = chrono::high_resolution_clock::now();
//     for (auto& thread : threads)
//     {
//         thread.join();
//     }
//     auto endTime = chrono::high_resolution_clock::now();
//     auto timeSpan = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

//     ofstream outfile("primes.txt");
//     if (outfile.is_open())
//     {
//         for (int prime : primes)
//         {
//             outfile << prime << endl;
//         }
//         outfile.close();
//     }

//     cout << "Número de threads: " << numThreads << endl;
//     cout << "Tempo total de execução: " << timeSpan.count() << " miliseconds" << endl;
//     cout << "Quantidade de números avaliados: " << maxNum << endl;
//     cout << "Quantidade de números primos encontrados: " << count << endl;
// }

// void workerFunction(vector<int>& primes, mutex& mtx, atomic<int>& count) {
//     vector<int> localPrimes; // Vetor local para armazenar primos
//     pair<int, int> task;
//     while (true) {
//         {
//             unique_lock<mutex> lock(queueMutex);
//             cv.wait(lock, [] { return !tasks.empty() || done; });
//             if (tasks.empty()) break;
//             task = tasks.front();
//             tasks.pop();
//         }
        
//         for (int i = task.first; i <= task.second; i++) {
//             if (isPrime(i)) {
//                 localPrimes.push_back(i); // Adiciona ao vetor local
//             }
//         }
        
//         // Bloco protegido para mesclar resultados
//         {
//             lock_guard<mutex> guard(mtx);
//             primes.insert(primes.end(), localPrimes.begin(), localPrimes.end());
//             count += localPrimes.size();
//         }
//         localPrimes.clear(); // Limpa o vetor local para a próxima tarefa
//     }
// }


// void addTasksToQueue(int maxNum, int chunkSize) {
//     for (int start = 1; start <= maxNum; start += chunkSize) {
//         int end = std::min(start + chunkSize - 1, maxNum);
//         std::lock_guard<std::mutex> lock(queueMutex);
//         tasks.push({start, end});
//     }
//     done = true;
//     cv.notify_all(); // Notifica as threads que todas as tarefas foram adicionadas
// }

// void mainFunctionBalanced(int maxNum, int numThreads){
//     vector<int> primes;
//     mutex mtx;
//     std::atomic<int> count = 0;
//     int chunkSize = maxNum / (numThreads*20);

//     vector<thread> threads;
//     for (int i = 0; i < numThreads; i++)
//     {
//         threads.emplace_back(workerFunction, ref(primes), ref(mtx), ref(count));
//     }

//     addTasksToQueue(maxNum, chunkSize);

//     auto startTime = chrono::high_resolution_clock::now();
//     for (auto& thread : threads)
//     {
//         thread.join();
//     }
//     auto endTime = chrono::high_resolution_clock::now();
//     auto timeSpan = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

//     ofstream outfile("primes2.txt");
//     if (outfile.is_open())
//     {
//         for (int prime : primes)
//         {
//             outfile << prime << endl;
//         }
//         outfile.close();
//     }

//     cout << "Número de threads: " << numThreads << endl;
//     cout << "Tempo total de execução: " << timeSpan.count() << " miliseconds" << endl;
//     cout << "Quantidade de números avaliados: " << maxNum << endl;
//     cout << "Quantidade de números primos encontrados: " << count << endl;
// }

// int main()
// {
//     for (int numThreads = 1; numThreads <= 10; numThreads++)
//     {
//         mainFunction(1000000, numThreads);
//         mainFunctionBalanced(1000000, numThreads);

//     }
// }