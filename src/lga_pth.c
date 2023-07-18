#include "lga_base.h"
#include "lga_pth.h"
#include <pthread.h>
#include "pthread_barrier.h"

pthread_barrier_t barrier;

typedef struct {
    int ind;
    byte * grid_1;
    byte * grid_2;
    int grid_size;
    int chunk_size;
} thread_args;

static byte get_next_cell(int i, int j, byte *grid_in, int grid_size) {
    byte next_cell = EMPTY;

    for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
        int rev_dir = (dir + NUM_DIRECTIONS/2) % NUM_DIRECTIONS;
        byte rev_dir_mask = 0x01 << rev_dir;

        int di = directions[i%2][dir][0];
        int dj = directions[i%2][dir][1];
        int n_i = i + di;
        int n_j = j + dj;

        if (inbounds(n_i, n_j, grid_size)) {
            if (grid_in[ind2d(n_i,n_j)] == WALL) {
                next_cell |= from_wall_collision(i, j, grid_in, grid_size, dir);
            }
            else if (grid_in[ind2d(n_i, n_j)] & rev_dir_mask) {
                next_cell |= rev_dir_mask;
            }
        }
    }

    return check_particles_collision(next_cell);
}

static void update(byte *grid_in, byte *grid_out, int grid_size,int l, int h) {
    for (int i = l; i < h; i++) {
        for (int j = 0; j < grid_size; j++) {
            if (grid_in[ind2d(i,j)] == WALL)
                grid_out[ind2d(i,j)] = WALL;
            else
                grid_out[ind2d(i,j)] = get_next_cell(i, j, grid_in, grid_size);
        }
    }
}

static void *update_thread(void * arg){
    thread_args *args = (thread_args *) arg;
    int l = args->ind * args->chunk_size;
    int h = l + args->chunk_size;

    update(args->grid_1,args->grid_2,args->grid_size,l,h);
    pthread_barrier_wait(&barrier);
    update(args->grid_2,args->grid_1,args->grid_size,l,h);
}

void simulate_pth(byte *grid_1, byte *grid_2, int grid_size, int num_threads) {
    pthread_t threads[num_threads];
    thread_args args[num_threads];
    
    
    for (int i = 0; i < ITERATIONS/2; i++) {
        pthread_barrier_init(&barrier, NULL, num_threads);  
        for (int t = 0; t < num_threads; t++) {
            args[t].ind = t;
            args[t].chunk_size = grid_size/num_threads;
            args[t].grid_1 = grid_1;
            args[t].grid_2 = grid_2;
            args[t].grid_size = grid_size;

            pthread_create(&threads[t], NULL, update_thread, (void *) &args[t]);
        }

        for (int t = 0; t < num_threads; t++) {
            pthread_join(threads[t], NULL);
        }
    }
}
