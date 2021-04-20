
// This test program generates a random maze based on the seed set in this define:

#define MAZE_SEED 8123534     // <--- change this to change the maze layout!

// Then, it solves for the shortest path through the maze with A*
// After solving for the shortest path, an agent traverses the maze using this information.

// NOTE: If NON_PORTABLE_COLOURS is defined then this will render to the console with colours, but 
// this will only work in my kernel. If this is not defined, then the maze renders using ASCII 
// The ASCII display is slightly buggy (JamesM's fault, not mine!), but it is fully portable
// between kernels.

// This doesn't use parallel processing, but ends up being a good test that alloc(), sleep(), and 
// the scheduler are behaving sanely.

#include "app.h"
#include "kernel_ken.h"
#ifdef NON_PORTABLE_COLOURS
#include "syscall.h"
#endif

#define AIR ' '
#define WALL '#'
#define GRID_WIDTH 79
#define GRID_HEIGHT 25

// Binary heap
typedef struct
{
    void **memory;
    unsigned int size;
    unsigned int max_size;
    int (*comparator)(void *a, void *b);
} binary_heap_t;
int intcomparator(void *a, void *b);
binary_heap_t *binary_heap_init(unsigned int max_size, int (comparator)(void *a, void *b));
void binary_heap_insert(binary_heap_t *heap, void *value);
void binary_heap_adjust(binary_heap_t *heap, int n);
void *binary_heap_remove(binary_heap_t *heap);
int binary_heap_empty(binary_heap_t *heap);

void internal_memset(void *dest, unsigned char val, unsigned int len)
{
    unsigned char *temp = (unsigned char*)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

static int parent(int i)
{
    return (i - 1) / 2;
}

static int left(binary_heap_t *heap, int i)
{
    int ret = 2 * i + 1;
    if(ret >= heap->size){
        return -1;
    }

    return ret;
}

static int right(binary_heap_t *heap, int i)
{
    int ret = 2 * i + 2;
    if(ret >= heap->size){
        return -1;
    }
    return ret;
}

binary_heap_t *binary_heap_init(unsigned int max_size, int (comparator)(void *a, void *b))
{
    int memsize = max_size * sizeof(void*);
    void **mem = alloc(memsize, 0);
    internal_memset(mem, 0, memsize);

    binary_heap_t *bin = alloc(sizeof(*bin), 0);
    bin->max_size = max_size;
    bin->size = 0;
    bin->memory = mem;
    bin->comparator = comparator;
    return bin;
}

int intcomparator(void *a, void *b)
{
    return ((unsigned int)a < (unsigned int)b) ? 1 : ((unsigned int)a == (unsigned int)b) ? 0 : -1;
}

void binary_heap_insert(binary_heap_t *heap, void *value)
{
    heap->size++;
    int n = heap->size - 1;
    heap->memory[n] = value;
    while(n > 0 && heap->comparator(heap->memory[n], heap->memory[parent(n)]) == 1 ) {
        void *tmp = heap->memory[n];
        heap->memory[n] = heap->memory[parent(n)];
        heap->memory[parent(n)] = tmp;
        n = parent(n);
    }

}

void binary_heap_adjust(binary_heap_t *heap, int n)
{
    int l = left(heap, n);
    int r = right(heap, n);

    if(l == -1 && r == -1){
        return;
    }

    int swap_target = n;
    if(l != -1 && heap->comparator(heap->memory[n], heap->memory[l]) == -1) {
        swap_target = l;
    }

    if(r != -1 && heap->comparator(heap->memory[n], heap->memory[r]) == -1) {

        if(heap->comparator(heap->memory[l], heap->memory[r]) == -1) {
            swap_target = r;
        }
    }

    if(swap_target != n){
        void *tmp = heap->memory[swap_target];
        heap->memory[swap_target] = heap->memory[n];
        heap->memory[n] = tmp;

        binary_heap_adjust(heap, swap_target);
    }

}

void *binary_heap_remove(binary_heap_t *heap)
{
    if(!heap || heap->size == 0){
        return 0;
    }

    void *ret = heap->memory[0];
    heap->memory[0] = heap->memory[heap->size - 1];
    heap->size --;
    binary_heap_adjust(heap, 0);

    return ret;
}

int binary_heap_empty(binary_heap_t *heap)
{
    return heap->size == 0;
}


// The following maze generation code is based on the code from
// https://en.wikipedia.org/wiki/Maze_generation_algorithm by Jacek Wieczorek
// but modified in several ways. The main addition is an agent that traverses the maze.

typedef struct
{
    int x, y; //node position - little waste of memory, but it allows faster generation
    void *parent; //Pointer to parent node
    char c; //Character to be displayed
    char dirs; //Directions that still haven't been explored
} node;

struct vec2
{
    int cost;
    int x;
    int y;
    struct vec2 *parent;
};

struct vec2 *vec2_init(int x, int y, int cost, struct vec2 *next)
{
    struct vec2 *v = alloc(sizeof(*v), 0);
    v->x = x;
    v->y = y;
    v->cost = cost;
    v->parent = next;
    return v;
}

#define INTERNAL_RAND_MAX 32768

int internal_rand_r(unsigned long int *next)
{
    *next = *next * 1103515245 + 12345;
    return (unsigned int)(*next/65536) % INTERNAL_RAND_MAX;
}

int init(node *nodes)
{
    node *n;
    //Setup crucial nodes
    int x,y;
    for(x = 0; x < GRID_WIDTH; x++) {
        for(y = 0; y < GRID_HEIGHT; y++) {
            n = nodes + x + y * GRID_WIDTH;
            if(x * y % 2) {
                n->x = x;
                n->y = y;
                n->dirs = 15; //Assume that all directions can be explored (4 youngest bits set)
                n->c = AIR;
            } else {
                n->c = WALL; //Add walls between nodes
            }
        }
    }
    return 0;
}

node *link_maze(node *nodes, unsigned long int *seed, node *n)
{
    //Connects node to random neighbor (if possible) and returns
    //address of next node that should be visited

    int x, y;
    char dir;
    node *dest;

    //Nothing can be done if null pointer is given - return
    if(n == 0) {
        return 0;
    }

    while (n->dirs) {
        //Randomly pick one direction
        dir = (1 << (internal_rand_r(seed) % 4));

        //If it has already been explored - try again
        if(~n->dirs & dir) {
            continue;
        }

        //Mark direction as explored
        n->dirs &= ~dir;

        //Depending on chosen direction
        switch(dir)
        {
            //Check if it's possible to go right
            case 1:
                if(n->x + 2 < GRID_WIDTH) {
                    x = n->x + 2;
                    y = n->y;
                } else {
                    continue;
                }
                break;
                //Check if it's possible to go down
            case 2:
                if(n->y + 2 < GRID_HEIGHT) {
                    x = n->x;
                    y = n->y + 2;
                } else {
                    continue;
                }
                break;

                //Check if it's possible to go left
            case 4:
                if(n->x - 2 >= 0) {
                    x = n->x - 2;
                    y = n->y;
                } else {
                    continue;
                }
                break;
                //Check if it's possible to go up
            case 8:
                if(n->y - 2 >= 0) {
                    x = n->x;
                    y = n->y - 2;
                } else {
                    continue;
                }
                break;
        }

        //Get destination node into pointer (makes things a tiny bit faster)
        dest = nodes + x + y * GRID_WIDTH;

        //Make sure that destination node is not a wall
        if(dest->c == AIR) {
            //If destination is a linked node already - abort
            if(dest->parent != 0) {
                continue;
            }

            //Otherwise, adopt node
            dest->parent = n;

            //Remove wall between nodes
            nodes[n->x + (x - n->x) / 2 + (n->y + (y - n->y) / 2) * GRID_WIDTH].c = AIR;

            //Return address of the child node
            return dest;
        }
    }

    //If nothing more can be done here - return parent's address
    return n->parent;
}

void draw(node *nodes, int px, int py)
{
#ifndef NON_PORTABLE_COLOURS
    print("\n");
    char buffer[80 * 25];
    int i;
    for(i = 0; i < 80 * 25; i++){
        buffer[i] = ' ';
    }
//    buffer[80 * 25 - 1] = '\0';
//    print(buffer);
//    print("\n");
#endif

    int x,y;
    //Outputs maze to terminal - nothing special
    for(y = 0; y < GRID_HEIGHT; y++ ) {
        for (x = 0; x < GRID_WIDTH; x++ ) {
#ifdef NON_PORTABLE_COLOURS
            int colour = COLOUR_BLACK;

            if(y == py && x == px) {
                colour = COLOUR_LIGHT_MAGENTA;
            } else {
                if(nodes[x + y * GRID_WIDTH].c == AIR){
                    colour = COLOUR_WHITE;
                } else {
                    colour = COLOUR_BLACK;
                }
            }
            syscall_monitor_colour(x, y, colour);
#else
            if( y == py && x == px) {
                buffer[y * 80 + x] = 'P';
            } else {
                buffer[y * 80 + x] = nodes[x + y * GRID_WIDTH].c;
            }
#endif
        }
    }
#ifndef NON_PORTABLE_COLOURS
    buffer[80 * 25 - 1] = '\0';
    print(buffer);
#endif
}

int int_abs(int a, int b)
{
    if(a > b) {
        return a - b;
    } else { // b >= a
        return b - a;
    }
}

int h(int x, int y, int tx, int ty)
{
    return int_abs(x, tx) + int_abs(y, ty);
}

int node_comparator(void *a, void *b)
{
    struct vec2 *node1 = a;
    struct vec2 *node2 = b;
    return (node1->cost < node2->cost) ? 1 : (node1->cost == node2->cost) ? 0 : -1;
}

struct vec2 *reverse_linked_list(struct vec2 *start)
{
    if(!start) {
        return 0;
    }
    if(!start->parent){
        return start;
    }

    struct vec2 *prev = start;
    struct vec2 *curr = start->parent;
    while(curr) {
        struct vec2 *next = curr->parent;
        curr->parent = prev;
        prev = curr;
        curr = next;
    }
    start->parent = 0;

    return prev;
}

struct vec2 *a_star(node *nodes, char *grid, int x_start, int y_start, int x_target, int y_target, int x_max, int y_max)
{
    binary_heap_t *heap = binary_heap_init(10000, node_comparator);
    binary_heap_insert(heap, vec2_init(x_start, y_start, 0, 0));

    while(!binary_heap_empty(heap)) {
        struct vec2 *n = binary_heap_remove(heap);

        if(n->x < 0 || n->x >= x_max || n->y < 0 || n->y >= y_max){
            continue;
        }
        if(grid[n->y * x_max + n->x] == 1){
            continue;
        }
        grid[n->y * x_max + n->x] = 1;

        if(nodes[n->x + n->y * x_max].c == WALL){
            continue;
        }

        if(x_target == n->x && y_target == n->y) {
            return reverse_linked_list(n);
        }

        binary_heap_insert(heap, vec2_init(n->x - 1, n->y, n->cost + h(n->x - 1, n->y, x_target, y_target), n));
        binary_heap_insert(heap, vec2_init(n->x + 1, n->y, n->cost + h(n->x + 1, n->y, x_target, y_target), n));
        binary_heap_insert(heap, vec2_init(n->x, n->y - 1, n->cost + h(n->x, n->y - 1, x_target, y_target), n));
        binary_heap_insert(heap, vec2_init(n->x, n->y + 1, n->cost + h(n->x, n->y + 1, x_target, y_target), n));
    }

    return 0;
}

void my_app()
{
    // Sanity check
//    assert(GRID_WIDTH > 0 && GRID_WIDTH <= 80 && GRID_HEIGHT > 0 && GRID_HEIGHT <= 25 && GRID_WIDTH % 2 == 1 && GRID_HEIGHT % 2 == 1);

    node *start, *last;
    unsigned long int seed = MAZE_SEED;

    // Init the grid
    unsigned int nodes_size = GRID_WIDTH * GRID_HEIGHT * sizeof(node);
    unsigned int grid_size = GRID_WIDTH * GRID_HEIGHT * sizeof(char);
    node *nodes = alloc(nodes_size, 0);
    internal_memset(nodes, 0, nodes_size);
    char *grid = alloc(grid_size, 0);
    internal_memset(grid, 0, grid_size);
    init(nodes);

    //Setup start node
    start = nodes + 1 + GRID_WIDTH;
    start->parent = start;
    last = start;

    //Connect nodes until start node is reached and can't be left
    while ((last = link_maze(nodes, &seed, last)) != start);

    // Build a path using DFS - technically this might not be optimal, but it'll at least solve the problem.
    struct vec2 *result = a_star(nodes, grid, 1, 1, GRID_WIDTH - 2, GRID_HEIGHT - 2, GRID_WIDTH, GRID_HEIGHT);

    // There's really no comment I can write that makes the following ~9 lines of code seem sensible, but
    // here we go:
    //
    // Since there's no nanosleep, I can't make a process sleep for ~200 milliseconds, which I'd like
    // so the agent doesn't take like 5 minutes to traverse the maze.
    // the best workaround I can think of here is to spawn 4 processes and make them fight for the CPU.
    // if the scheduler is acting sensibly, this should cause the agent to run 5+ times a second, but not a crazy.
    // number of times (5 for 20 timeslices / second, 10 for 40 slices / second, ... etc)
    // This is about how fast I want it to move.
    // If the scheduler isn't behaving sensibly and causing processes with the same priority to act
    // in a round-robin fashion, I consider that a bug.

#ifdef NON_PORTABLE_COLOURS
    if(fork() == 0) {
        for(;;);
    }
    if(fork() == 0) {
        for(;;);
    }
    if(fork() == 0) {
        for(;;);
    }

    // Traverse the maze
    struct vec2 *iter = result;
    while(iter){
//        sleep(1);
        yield();
        draw(nodes, iter->x, iter->y);
        iter = iter->parent;
    }
#else
    struct vec2 *iter = result;
    while(iter){
        sleep(1);
        draw(nodes, iter->x, iter->y);
        iter = iter->parent;
    }
#endif

}