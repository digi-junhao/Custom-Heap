/**
 * READ ME
 * This is a stress test designed for smalloc and sfree
 * - Assumes 10MB total memory (to be set as MEM_SIZE value in allocator.h)
 * - Makes 50,000 allocation requests to push the allocator.
 * - Uses random sizes of up to 32 KB per allocation to exercise many paths.
 * - Keeps 512 live blocks (active allocations) to maintain pressure.
 * - Every 128 requests frees a random live block to create holes.
 * - Tracks an external fragmentation marker: (1 âˆ’ L/F) (L = largest free block, F = total free memory)
 * - Reports utilization (fraction of heap used) and turnover (total memory allocated as multiples of heap size) at the point of first failure 
 *   to show efficiency under stress.
 * 
 * - NOTE: In the allocator module, please provide the function: void allocator_stats(size* N, size* F, size* L) 
     which computes: N: number of free blocks, F: amount of free memory (in bytes), L: size of the largest free block (in bytes). 
     The allocator module should use appropriate freelist function to compute these values.
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "allocator.h"   // smalloc, sfree, allocator_stats, allocator_zone_stats

// Tunable Parameters (keep these value to test all version first)
#define N_REQUESTS   50000         // total number of allocation requests
#define MAX_REQ_SIZE 32 * 1024     // cap on a single request size (bytes)
#define D_FREQ       128           // every D_FREQ allocations, free a random live block to create holes
#define LIVE         512           // number of concurrently live allocations to keep

// Random request size in [1..MAX_REQ_SIZE]
static inline size_t rand_size() { return (size_t)(rand() % MAX_REQ_SIZE) + 1; }

int main(void) {
    srand(time(0)); // seed the rng
  
    size_t success = 0;                          // # successful allocations so far
    size_t before_first_failure = N_REQUESTS;    // stays N_REQUESTS if no failure occurs
    double ext_frag_max = 0.0;                   // running max of (1 - L/F)
    size_t freelist_len_max = 0;                 // running max of free-list length
    size_t total_requested = 0;                  // total bytes requested across all attempts
    size_t total_allocated = 0;                  // total bytes allocated (successful requests only)
    size_t requested_before_first_failure = 0;   // bytes requested strictly before the first failure
    double mem_ut_fail = 0.0;                    // fraction of memory utilized at first failure [0,1] 
    double bytes_to_fail_turnover = 0.0;         // how many MEM_SIZEs worths of data we served before the first failure?
    int failure_seen = 0;

    
    size_t fail_nodes = 0, fail_F = 0, fail_L = 0;
    size_t fail_req_size = 0;

    // Keep up to LIVE live allocations at any time
    void *pool[LIVE];
    for (size_t k = 0; k < LIVE; ++k) pool[k] = NULL;

    size_t idx = 0;

    // task5 addition: stats for each arena (0 = tiny, 1 = small, 2 = medium)
    const char *zone_names[NUM_ARENAS] = {"TINY", "SMALL", "MEDIUM"};
    size_t zone_requests[NUM_ARENAS] = {0, 0, 0};       // number of requests sent to each arena
    size_t zone_fail_F[NUM_ARENAS] = {0, 0, 0};         // free memory in each arena at the first failure
    size_t zone_len_max[NUM_ARENAS] = {0, 0, 0};        // maximum freelist length of each arena
    double zone_frag_max[NUM_ARENAS] = {0.0, 0.0, 0.0}; // maximum fragmentation of each arena

    for (size_t i = 0; i < N_REQUESTS; ++i) {
        // Allocation request
        size_t sz = rand_size();
        total_requested += sz;

        // task5 addition: count which arena this request goes to (size includes the header)
        size_t total_sz = allocator_req_mem(sz);
        if (total_sz <= TINY_MAX) zone_requests[0]++;
        else if (total_sz <= SMALL_MAX) zone_requests[1]++;
        else zone_requests[2]++;
        if (!failure_seen) requested_before_first_failure += sz;

        void *p = smalloc(sz);

        if (p) {
            success++;
            total_allocated += sz;

            // Keep up to LIVE active allocations: overwrite round-robin slot
            if (pool[idx]) sfree(pool[idx]);   // drop the old one in this slot
            pool[idx] = p;
            idx = (idx + 1) % LIVE;
        } else if (before_first_failure == N_REQUESTS) {
            // First failure observed on this request
            before_first_failure = i;
            failure_seen = 1;
            requested_before_first_failure -= sz;   // exclude failing request
            fail_req_size = sz;

            allocator_stats(&fail_nodes, &fail_F, &fail_L); // snapshot free memory at failure

            // task5 addition: free memory in each arena at the first failure
            for (int z = 0; z < NUM_ARENAS; z++) {
                size_t zN, zF, zL;
                allocator_zone_stats(z, &zN, &zF, &zL);
                zone_fail_F[z] = zF;
            }
            
            mem_ut_fail = 1.0 - (fail_F*1.0 / (MEM_SIZE*1.0));
            bytes_to_fail_turnover = (requested_before_first_failure * 1.0) / (MEM_SIZE * 1.0);
        }

        // Every D_FREQ requests, free a random live slot to create holes
        if ((i + 1) % D_FREQ == 0) {
            size_t k = (size_t)(rand() % LIVE);
            if (pool[k]) { sfree(pool[k]); pool[k] = NULL; }
        }

        // Update running maxes: external fragmentation and free-list length
        size_t nodes = 0, F = 0, L = 0;
        allocator_stats(&nodes, &F, &L);
       

        if (nodes > freelist_len_max) freelist_len_max = nodes;

        double ext_frag = (F > 0) ? (1.0 - (double)L / (double)F) : 0.0;
        if (ext_frag > ext_frag_max) ext_frag_max = ext_frag;

        // task5 addition: running maxes for each arena
        for (int z = 0; z < NUM_ARENAS; z++) {
            size_t zN, zF, zL;
            allocator_zone_stats(z, &zN, &zF, &zL);
            if (zN > zone_len_max[z]) zone_len_max[z] = zN;
            double zfrag = (zF > 0) ? (1.0 - (double)zL / (double)zF) : 0.0;
            if (zfrag > zone_frag_max[z]) zone_frag_max[z] = zfrag;
        }
    }

    // Final stats (end-of-run)
    size_t final_nodes = 0, final_F = 0, final_L = 0;
    allocator_stats(&final_nodes, &final_F, &final_L);
    
    if (final_nodes > freelist_len_max) freelist_len_max = final_nodes;

    double final_ext_frag = (final_F > 0) ? (1.0 - (double)final_L / (double)final_F) : 0.0;

    // Success ratios, etc.
    double success_rate_req   = N_REQUESTS ? (double)success / (double)N_REQUESTS : 0.0;
    double success_rate_bytes = total_requested ? (double)total_allocated / (double)total_requested : 0.0;

    // Print Report
    printf("\nOverall: \n");
    printf("\tTotal Memory: %.2f MB\n", MEM_SIZE / (1024.0 * 1024.0));
    printf("\tRequests: %zu\n", (size_t)N_REQUESTS);
    printf("\tMemory Requested: %.2f MB\n", total_requested / (1024.0 * 1024.0));
    printf("\tMemory Allocated: %.2f MB\n", total_allocated / (1024.0 * 1024.0));
    
    printf("\nSuccess Ratios: \n");
    printf("\tSuccessful Allocations: %zu\n", success);
    printf("\tSuccessful Requests: %.2f%%\n", 100.0 * success_rate_req);
    printf("\tSuccessful Allocation (bytes): %.2f%%\n", 100.0 * success_rate_bytes);

    printf("\nBefore Heap Overflow (First Failure): \n");
    printf("\tRequests: %zu\n",
           (before_first_failure == N_REQUESTS) ? (size_t)N_REQUESTS : before_first_failure);

    if (before_first_failure == N_REQUESTS) {
        printf("\tMemory Allocated: %.2f MB\n", total_requested / (1024.0 * 1024.0));
        printf("\t(No failure occurred)\n");
    } else {
        printf("\tMemory Allocated: %.2f MB\n",
               requested_before_first_failure / (1024.0 * 1024.0));
        // Print free memory available and request size at failure
        printf("\tFree Memory at Failure: %.2f MB (%.2f KB)\n",
               fail_F / (1024.0 * 1024.0), fail_F / 1024.0);
        printf("\tRequest Size at Failure: %.2f KB\n", fail_req_size / 1024.0);
        printf("\tMemory Utilization at Failure: %.2f\n", mem_ut_fail);
        printf("\tBytes-to-Failure Turnover (BTF): %.2f x MEM_SIZE\n", bytes_to_fail_turnover);
    }
    
    printf("\nFreelist Length: \n");
    printf("\tFinal: %zu\n", final_nodes);
    printf("\tMaximum: %zu\n", freelist_len_max);

    printf("\nExternal Fragmentation (1 - L/F): \n");
    printf("\tFinal: %.4f\n", final_ext_frag);
    printf("\tMaximum: %.4f\n", ext_frag_max);

    // task5 addition: report for each arena
    printf("\nPer Arena: \n");
    printf("\t%-7s %10s %16s %13s %13s %12s %12s\n", "Arena", "Requests", "Free at Fail(KB)", "List (final)", "List (max)", "Frag (final)", "Frag (max)");
    for (int z = 0; z < NUM_ARENAS; z++) {
        size_t zN, zF, zL;
        allocator_zone_stats(z, &zN, &zF, &zL);
        double zfrag = (zF > 0) ? (1.0 - (double)zL / (double)zF) : 0.0;
        printf("\t%-7s %10zu %16.2f %13zu %13zu %12.4f %12.4f\n", zone_names[z], zone_requests[z],
               zone_fail_F[z] / 1024.0, zN, zone_len_max[z], zfrag, zone_frag_max[z]);
    }

    printf("\n");

    return 0;
}