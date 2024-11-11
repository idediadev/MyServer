// Classics lib
#include <stdio.h>;
#include <stdlib.h>;
#include <string.h>;
#include <semaphore.h>;
#include <signal.h>;
#include <unistd.h>;
#include <pthread.h>
// Net lib 
#include <sys/socket.h>;
#include <sys/types.h>;
#include <netdb.h>;
#include <netinet/in.h>;
#include <arpa/inet.h>;
// Accesories 
#include <crypt.h>;
#include <fcntl.h>;


#define THREAD_POOL_SIZE 10 struct thread_pool 
#define JOB_QUEUE_SIZE 50

struct job 
{
    void (*function)(void *arg);  // Function pointer to the task
    void *arg;                    // Arguments to the task
};

struct job_queue 
{
    struct job jobs[JOB_QUEUE_SIZE];    // Array of jobs
    int front;                          // Front index
    int rear;                           // Rear index
    int count;                          // Number of jobs in the queue
    pthread_mutex_t mutex;              // Mutex for synchronization
    pthread_cond_t cond;                // Condition variable for signaling
};


struct client_info 
{ 
    int socket_fd; // Socket file descriptor for the client 
    struct sockaddr_in addr; // Client address information };
};


struct thread_pool
{ 
    pthread_t threads[THREAD_POOL_SIZE]; // Array of threads 
    pthread_mutex_t mutex; // Mutex for synchronization 
    pthread_cond_t cond; // Condition variable for signaling 
};

// Function declarations
void *worker_thread(void *arg);
void enqueue_job(struct job_queue *queue, struct job new_job);
struct job dequeue_job(struct job_queue *queue);

// Main function
int main() {
    struct server_config config = { .port = 8080, .max_connections = MAX_CONNECTIONS };
    struct thread_pool pool;
    struct job_queue queue = { .front = 0, .rear = -1, .count = 0 };
    
    // Initialize the thread pool
    pthread_mutex_init(&pool.mutex, NULL);
    pthread_cond_init(&pool.cond, NULL);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&pool.threads[i], NULL, worker_thread, &queue);
    }
    
    // Socket setup
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = { .sin_family = AF_INET, .sin_port = htons(config.port), .sin_addr.s_addr = INADDR_ANY };
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, config.max_connections);

    while (1) {
        struct client_info client;
        socklen_t client_len = sizeof(client.addr);
        client.socket_fd = accept(server_fd, (struct sockaddr *)&client.addr, &client_len);
        
        // Enqueue the client processing job
        struct job new_job = { .function = handle_client, .arg = (void *)&client };
        enqueue_job(&queue, new_job);
    }

    close(server_fd);
    return 0;
}

// Worker thread function
void *worker_thread(void *arg) {
    struct job_queue *queue = (struct job_queue *)arg;

    while (1) {
        struct job j = dequeue_job(queue);
        if (j.function) {
            j.function(j.arg);
        }
    }
    return NULL;
}

// Enqueue job function
void enqueue_job(struct job_queue *queue, struct job new_job) {
    pthread_mutex_lock(&queue->mutex);
    queue->rear = (queue->rear + 1) % JOB_QUEUE_SIZE;
    queue->jobs[queue->rear] = new_job;
    queue->count++;
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

// Dequeue job function
struct job dequeue_job(struct job_queue *queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->count == 0) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    struct job j = queue->jobs[queue->front];
    queue->front = (queue->front + 1) % JOB_QUEUE_SIZE;
    queue->count--;
    pthread_mutex_unlock(&queue->mutex);
    return j;
}

// Example client handler function
void handle_client(void *arg) {
    struct client_info *client = (struct client_info *)arg;
    // Handle client communication here
    close(client->socket_fd);
}
