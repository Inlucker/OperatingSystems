#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <stdbool.h>

#define THREADS_NUMBER 3
#define ITERATIONS 10

long active_readers = 0;
bool active_writer = false;
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
    InterlockedIncrement(&readers_queue);
    if (active_writer || writers_queue > 0)
        WaitForSingleObject(can_read, INFINITE);

    WaitForSingleObject(hMutex, INFINITE); //Не спорь!!!
    //Видимо мьютекс нужен только здесь, потому что здесь идёт две подряд неделимые операции (Для монопольного доступа)
    InterlockedDecrement(&readers_queue);
    InterlockedIncrement(&active_readers);
    SetEvent(can_read);
    ReleaseMutex(hMutex);
}

void stop_read()
{
    InterlockedDecrement(&active_readers);
    if (active_readers == 0)
        SetEvent(can_write);
}

DWORD WINAPI reader(CONST LPVOID lpParams)
{
    struct params *r = lpParams;
    int r_id = r->id;
    int delay = r->delay;
    //printf("Thread: Reader %d delay = %d\n", r_id, delay);
    for (int j = 0; j < ITERATIONS; j++)
    {
        Sleep(delay);

        start_read();

        //printf("Reader %d started reading\n", r_id);

        printf("Reader %d have read: %d\n", r_id, value);

        stop_read();

        //printf("Reader %d stoped reading\n", r_id);
    }

    //printf("Reader %d ended\n", r_id);
    ExitThread(0); //return 0;
}

void start_write()
{
    InterlockedIncrement(&writers_queue);
    if (active_readers > 0 || active_writer)
        ResetEvent(can_write);

    WaitForSingleObject(can_write, INFINITE);

    InterlockedDecrement(&writers_queue);
    active_writer = true;
}

void stop_write()
{
    active_writer = false;
    if (readers_queue > 0)
        SetEvent(can_read);
    else
        SetEvent(can_write);
}

DWORD WINAPI writer(CONST LPVOID lpParams)
{
    struct params *w = lpParams;
    int w_id = w->id;
    int delay = w->delay;
    //printf("Thread: Writer %d delay = %d\n", w_id, delay);
    for (int j = 0; j < ITERATIONS; j++)
    {
        Sleep(delay);

        start_write();

        //printf("Writer %d started writing\n", w_id);
        printf("Writer %d have written: %d\n", w_id, ++value);

        stop_write();

        //printf("Writer %d stoped writing\n", w_id);
    }

    //printf("Writer %d ended\n", w_id);
    ExitThread(0); //return 0;
}

#define TEST_ITERS 1

int main()
{
    srand(time(NULL));
    can_read = CreateEvent(NULL, FALSE, FALSE, NULL);
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
            struct params r = {i+1, rand()%30+10};
            printf("Main: Reader %d delay = %d\n", r.id, r.delay); // без принта, получаются неправильные id
            hReaders[i] = CreateThread(NULL, 0, &reader, &r, 0, NULL);
            struct params w = {i+1, rand()%30+10};
            printf("Main: Writer %d delay = %d\n", w.id, w.delay); // Это потому что объект структуры params успевает измениться, а адресс тот же самый
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
