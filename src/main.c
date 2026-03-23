/* if defined, use POSIX over c23 standard */
// #define USE_POSIX

#ifdef USE_POSIX
#define _POSIX_C_SOURCE 199309L
#elif __STDC_VERSION__ >= 202311L
#define _ISOC23_SOURCE
#else
#error "must support POSIX or use c23 version"
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CACHE_LINE_SIZE 64

#define KiB 1024UL
#define MiB (1024UL * 1024UL)
#define MIN_CACHE (1 * KiB)
#define MAX_CACHE (64 * MiB)

// Get the current clock time in nanoseconds
static uint64_t get_time_ns()
{
    struct timespec clock_resolution = { 0 };
#ifdef USE_POSIX
    int result = clock_gettime(CLOCK_MONOTONIC, &clock_resolution);
    if(result != 0) {
        fprintf(stderr,
                "[Error]: failed to return clock time: "__FILE__
                "\n");
        _Exit(1);
    }
#else
    int result = timespec_get(&clock_resolution, TIME_MONOTONIC);
    if(result == 0) {
        (void)fprintf(stderr,
                      "[Error]: failed to return clock time: "__FILE__
                      "\n");
        _Exit(1);
    }
#endif

    const int nanosecond = 1000000000;
    return (uint64_t)(clock_resolution.tv_sec * nanosecond)
           + clock_resolution.tv_nsec;
}

// Get clock resolution in nanoseconds
static uint64_t clock_res_ns()
{
    struct timespec clock_resolution = { 0 };
#ifdef USE_POSIX
    int result = clock_getres(CLOCK_MONOTONIC, &clock_resolution);
    if(result != 0) {
        fprintf(stderr,
                "[Error]: failed to return clock time: "__FILE__
                "\n");
        _Exit(1);
    }
#else
    int result = timespec_getres(&clock_resolution, TIME_MONOTONIC);
    if(result == 0) {
        (void)fprintf(stderr,
                      "[Error]: failed to return clock resolution: "__FILE__
                      "\n");
        _Exit(1);
    }
#endif

    assert(clock_resolution.tv_sec == 0 && clock_resolution.tv_nsec > 0
           && "clock_res is actually set");
    return clock_resolution.tv_nsec;
}

static uint64_t rand_uint64(void)
{
    uint64_t random = 0;
    for(int i = 0; i < 64; i += 15 /*30*/) {// NOLINT
        random = random * ((uint64_t)RAND_MAX + 1) + rand();// NOLINT
    }
    return random;
}

static long double benchmark(const size_t alloc_size)
{
    // create memory aligned pointer chase buffer
    void** memory = (void**)aligned_alloc(CACHE_LINE_SIZE, alloc_size);
    const size_t alloc_ptr_count = alloc_size / sizeof(void*);
    for(size_t i = 0; i < alloc_ptr_count - 1; ++i) {
        memory[i] = (void*)&memory[i + 1];
    }
    // point last pointer to the first
    memory[alloc_ptr_count - 1] = (void*)&memory[0];

    // use Sattolo's algorithm to shuffle pointer chase buffer
    for(size_t i = alloc_ptr_count - 1; i >= 1; --i) {
        const size_t rand_index = rand_uint64() % i;
        void* temp = memory[i];
        memory[i] = memory[rand_index];
        memory[rand_index] = temp;
    }

    // first pointer
    void** pointer = &memory[0];

    // randomly choose 1,000,000 runs
    const size_t runs = 1000000;

    // warmup cache by discarding first 2 runs
    for(size_t i = 0; i < runs; ++i) {
        pointer = (void**)*pointer;
    }

    // start timer
    const uint64_t start_time = get_time_ns();

    // benchmark
    for(size_t i = 0; i < runs; ++i) {
        pointer = (void**)*pointer;
    }

    // end timer
    const uint64_t end_time = get_time_ns();

    volatile void* dummy_sink = (void*)pointer;
    (void)dummy_sink;
    free((void*)memory);

    const uint64_t resolution = clock_res_ns();
    const uint64_t duration = end_time - start_time;
    const uint64_t mod = duration % resolution;
    /* If remainder is greater than half of resolution, round up, otherwise
    round down.
    Then divide the toal number of ops by number of all ops to get
    average latency.*/
    return mod >= (resolution / 2)
               ? (duration + resolution - mod) / (long double)runs
               : (duration - mod) / (long double)runs;
}

int main(void)
{
    // set seed for benchmark
    unsigned int seed = 0;
    srand(seed);

    printf("%-20s %-20s\n", "Buffer Size", "Avg Latency");
    printf("-------------------------------------------------\n");

    for(size_t alloc = MIN_CACHE; alloc <= MAX_CACHE; alloc *= 2) {
        const long double latency_ns = benchmark(alloc);
        char size_label[20];// NOLINT
        if(alloc >= MiB) {
            sprintf(size_label, "%4lu MiB", (alloc / MiB));// NOLINT
        } else {
            sprintf(size_label, "%4lu KiB", (alloc / KiB));// NOLINT
        }
        printf("%-20s %10.2Lf ns\n", size_label, latency_ns);
    }

    return EXIT_SUCCESS;
}
