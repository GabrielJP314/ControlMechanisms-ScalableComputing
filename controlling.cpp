#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <algorithm>
#include <chrono>

using namespace std;

// Function to check if a number is prime
bool isPrime(int num)
{
    if (num <= 1) return false;
    if (num <= 3) return true;
    if (num % 2 == 0 || num % 3 == 0) return false;
    for (int i = 5; i * i <= num; i += 6)
    {
        if (num % i == 0 || num % (i + 2) == 0) return false;
    }
    return true;
}

// Function to process a range of numbers, that each thread will execute
void processRange(int start, int end, vector<int>& primes, mutex& mtx, int& count)
{
    int localCount = 0;
    for (int i = start; i <= end; i++)
    {
        if (isPrime(i))
        {
            mtx.lock();
            primes.push_back(i);
            mtx.unlock();
            localCount++;
        }
    }
    mtx.lock();
    count += localCount;
    mtx.unlock();
}

// Main function to control the threads
void mainFunction(int maxNum, int numThreads)
{
    chrono::steady_clock::time_point startTime = chrono::steady_clock::now();

    vector<int> primes;
    mutex mtx;
    int count = 0;
    int chunkSize = maxNum / numThreads;

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

    chrono::steady_clock::time_point endTime = chrono::steady_clock::now();
    chrono::duration<double> timeSpan = chrono::duration_cast<chrono::duration<double>>(endTime - startTime);

    ofstream outfile("primes.txt");
    if (outfile.is_open())
    {
        for (int prime : primes)
        {
            outfile << prime << endl;
        }
        outfile.close();
    }

    cout << "Número de threads: " << numThreads << endl;
    cout << "Tempo total de execução: " << timeSpan.count() << " seconds" << endl;
    cout << "Quantidade de números avaliados: " << maxNum << endl;
    cout << "Quantidade de números primos encontrados: " << count << endl;
    cout << "Números primos encontrados: ";
    for (int prime : primes)
    {
        cout << prime << " ";
    }
    cout << endl;
}

int main()
{
    for (int numThreads = 1; numThreads <= 10; numThreads++)
    {
        mainFunction(10000, numThreads);
    }
}