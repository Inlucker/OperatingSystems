#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <stdbool.h>

#define THREADS_NUMBER 3
#define ITERATIONS 10

long active_readers = 0;
long active_writer = 0;
HANDLE can_read;
HANDLE can_write;

HANDLE hMutex;
long readers_queue = 0;
long writers_queue = 0;

int value = 0;

struct params
{
    int id;
    int delay;
};

void start_read()
{
    WaitForSingleObject(hMutex, INFINITE);
    readers_queue++; //InterlockedIncrement(&readers_queue);
    if (active_writer || writers_queue > 0)
        ResetEvent(can_read);
    ReleaseMutex(hMutex);

    WaitForSingleObject(can_read, INFINITE);

    WaitForSingleObject(hMutex, INFINITE);
    readers_queue--; //InterlockedDecrement(&readers_queue);
    active_readers++; //InterlockedIncrement(&active_readers);
    SetEvent(can_read);
    ReleaseMutex(hMutex);
}

void stop_read()
{
    WaitForSingleObject(hMutex, INFINITE);
    active_readers--; //InterlockedDecrement(&active_readers);
    if (active_readers == 0)
        SetEvent(can_write);
    ReleaseMutex(hMutex);
}

DWORD WINAPI reader(CONST LPVOID lpParams)
{
    struct params *r = lpParams;
    int r_id = r->id; //(int)(lpParams);
    int delay = r->delay; //(int)(lpParams+sizeof(int)); //rand() % 3000 + 1000;
    //printf("Thread: Reader %d delay = %d\n", r_id, delay);
    for (int j = 0; j < ITERATIONS; j++)
    {
        Sleep(delay);

        start_read();

        //printf("Reader %d started reading\n", r_id);

        WaitForSingleObject(hMutex, INFINITE);
        printf("Reader %d have read: %d\n", r_id, value);
        ReleaseMutex(hMutex);

        stop_read();

        //printf("Reader %d stoped reading\n", r_id);
    }

    //printf("Reader %d ended\n", r_id);
    return 0;
}

void start_write()
{
    WaitForSingleObject(hMutex, INFINITE);
    writers_queue++; //InterlockedIncrement(&writers_queue);
    if (active_readers > 0 || active_writer)
        ResetEvent(can_write);
    ReleaseMutex(hMutex);

    WaitForSingleObject(can_write, INFINITE);

    WaitForSingleObject(hMutex, INFINITE);
    writers_queue--; //InterlockedDecrement(&writers_queue);
    active_writer++; //InterlockedIncrement(&active_writer); //active_writer = true;
    ReleaseMutex(hMutex);
}

void stop_write()
{
    WaitForSingleObject(hMutex, INFINITE);
    active_writer--; //InterlockedDecrement(&active_writer); //active_writer = false;
    if (readers_queue > 0)
        SetEvent(can_read);
    else
        SetEvent(can_write);
    ReleaseMutex(hMutex);
}

DWORD WINAPI writer(CONST LPVOID lpParams)
{
    struct params *w = lpParams;
    int w_id = w->id; //(int)(lpParams);
    int delay = w->delay; //(int)(lpParams+sizeof(int)); //rand() % 3000 + 1000;
    //printf("Thread: Writer %d delay = %d\n", w_id, delay);
    for (int j = 0; j < ITERATIONS; j++)
    {
        Sleep(delay);

        start_write();

        //printf("Writer %d started writing\n", w_id);
        WaitForSingleObject(hMutex, INFINITE);
        printf("Writer %d have written: %d\n", w_id, ++value);
        ReleaseMutex(hMutex);

        stop_write();

        //printf("Writer %d stoped writing\n", w_id);
    }

    //printf("Writer %d ended\n", w_id);
    return 0;
}

#define TEST_ITERS 100

int main()
{
    srand(time(NULL));
    can_read = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == can_read)
        perror("Failed to create event can_read");

    can_write = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (NULL == can_write)
        perror("Failed to create event can_write");

    hMutex = CreateMutex(NULL, FALSE, NULL);
    if (NULL == hMutex)
        perror("Failed to create mutex");

    HANDLE hReaders[THREADS_NUMBER];
    HANDLE hWriters[THREADS_NUMBER];

    for (int j = 0; j < TEST_ITERS; j++)
    {
        for (int i = 0; i < THREADS_NUMBER; i++)
        {
            struct params r = {i+1, rand()%300+100};
            //printf("Main: Reader %d delay = %d\n", r.id, r.delay);
            hReaders[i] = CreateThread(NULL, 0, &reader, &r, 0, NULL);
            struct params w = {i+1, rand()%300+100};
            //printf("Main: Writer %d delay = %d\n", w.id, w.delay);
            hWriters[i] = CreateThread(NULL, 0, &writer, &w, 0, NULL);
        }

        WaitForMultipleObjects(THREADS_NUMBER, hReaders, TRUE, INFINITE);
        WaitForMultipleObjects(THREADS_NUMBER, hWriters, TRUE, INFINITE);
        printf("ENDED\n\n");
    }

    for (int i = 0; i < THREADS_NUMBER; i++)
    {
        CloseHandle(hReaders[i]);
        CloseHandle(hWriters[i]);
    }
    CloseHandle(hMutex);
    CloseHandle(can_read);
    CloseHandle(can_write);

    printf("ENDED\n");
    return 0;
}
