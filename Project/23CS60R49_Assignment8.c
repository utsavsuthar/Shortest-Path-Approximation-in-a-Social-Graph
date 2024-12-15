#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
pthread_mutex_t mutex;
pthread_mutex_t lock;
pthread_mutex_t lock2;
FILE *updateLog, *path, *landmarklist, *landmarkpairread;
sem_t sem1, sem2, sem3;
int numVertices;
int landmark[100];
// Define a structure to represent a node in the adjacency list
typedef struct Node
{
    int data;
    struct Node *next;
} Node;

// Define a structure to represent the graph
typedef struct Graph
{
    int numVertices;
    Node **adjLists; // An array of linked lists
    pthread_mutex_t *mutexes;
} Graph;
Graph *graph;
typedef struct ThreadArgs
{
    Graph *graph;
    FILE *file;
    int threadid;
} ThreadArgs;
struct BFSData
{
    struct Graph *graph;
    int src;
    int dest;
};
int tossCoin(double p)
{
    return ((double)rand() / RAND_MAX) < p;
}
// Function to create a new node

Node *createNode(int data)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to create a graph with a given number of vertices
Graph *createGraph(int numVertices)
{
    struct Graph *graph = (struct Graph *)malloc(sizeof(struct Graph));
    graph->numVertices = numVertices;
    graph->adjLists = (struct Node **)malloc(numVertices * sizeof(struct Node *));

    for (int i = 0; i < numVertices; i++)
    {
        graph->adjLists[i] = NULL;
    }

    return graph;
}

// Function to add an edge to the graph
void addEdge(Graph *graph, int src, int dest)
{
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->data = dest;
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;
    // printf("edge %d %d added",src,dest);
    // If the graph is undirected, add an edge from dest to src as well
}

void *readEdgesAndAddToGraph(void *args)
{

    ThreadArgs *threadArgs = (ThreadArgs *)args;

    int src, dest;
    while (fscanf(threadArgs->file, "%d %d", &src, &dest) != EOF)
    {
        pthread_mutex_lock(&mutex);
        addEdge(threadArgs->graph, src, dest);
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

void remove_Edge()
{
    int src, dest;
    do
    {
        src = rand() % graph->numVertices;
        struct Node *current = graph->adjLists[src];
        if (current)
        {
            dest = current->data;
        }
        else
        {
            // No edges from this source, try again
            continue;
        }
        // Remove the edge
        graph->adjLists[src] = current->next;
        free(current);
        break;
    } while (1);

    // In the "update.log" file, write the removed edge and the current timestamp
    fprintf(updateLog, "<REMOVE> <%d, %d> %ld\n", src, dest, time(NULL));
}

// Function to add an edge to the graph
void add_Edge()
{
    int src = rand() % graph->numVertices;
    int dest = rand() % graph->numVertices;

    // Check if the edge already exists in the graph
    struct Node *current = graph->adjLists[src];
    while (current)
    {
        if (current->data == dest)
        {
            // Edge already exists, do nothing
            return;
        }
        current = current->next;
    }
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->data = dest;
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;

    struct Node *newNode1 = (struct Node *)malloc(sizeof(struct Node));
    newNode1->data = src;
    newNode1->next = graph->adjLists[dest];
    graph->adjLists[dest] = newNode1;
    
    // In the "update.log" file, write the added edge and the current timestamp
    fprintf(updateLog, "<ADD> <%d, %d> %ld\n", src, dest, time(NULL));
}

void *graphUpdateThread(void *arg)
{
    ThreadArgs *threadArgs = (ThreadArgs *)arg;
    // printf("Thread %d started\n",threadArgs->threadid);
    int choice = tossCoin(0.8); // 1 with probability 0.8
    if (choice)
    {
        pthread_mutex_lock(&mutex);
        remove_Edge();
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        pthread_mutex_lock(&mutex);
        add_Edge();
        pthread_mutex_unlock(&mutex);
    }
    fflush(updateLog); // Ensure the log is immediately written

    // Check for exit condition (e.g., no more node pairs to update)
    // You may need to implement an exit condition based on your specific logic
    // printf("Thread %d completed\n",threadArgs->threadid);
    pthread_exit(NULL);
}
void dijkstra(int landmark_node, int *assigned_nodes, int num_assigned_nodes)
{
    int *distance = (int *)malloc(sizeof(int) * numVertices);
    int *visited = (int *)malloc(sizeof(int) * numVertices);

    for (int i = 0; i < numVertices; i++)
    {
        distance[i] = INT_MAX;
        visited[i] = 0;
    }

    distance[landmark_node] = 0;

    for (int i = 0; i < numVertices - 1; i++)
    {
        // Find the vertex with the minimum distance
        int minDistance = INT_MAX;
        int minIndex = -1;

        for (int j = 0; j < numVertices; j++)
        {
            if (!visited[j] && distance[j] < minDistance)
            {
                minDistance = distance[j];
                minIndex = j;
            }
        }

        if (minIndex == -1)
        {
            break;
        }

        visited[minIndex] = 1;

        // Update distances to neighbors
        Node *current = graph->adjLists[minIndex];
        while (current != NULL)
        {
            int neighbor = current->data;
            int weight = 1;

            if (!visited[neighbor] && distance[minIndex] != INT_MAX &&
                distance[minIndex] + weight < distance[neighbor])
            {
                distance[neighbor] = distance[minIndex] + weight;
            }

            current = current->next;
        }
    }

    // Store the computed distances in a shared data structure
    pthread_mutex_lock(&lock);
    // Output the shortest distances for the assigned nodes or use the 'distance' array as needed
    fprintf(path, "Shortest distances from landmark node %d:\n", landmark_node);
    for (int i = 0; i < num_assigned_nodes; i++)
    {
        if (distance[assigned_nodes[i]] == INT_MAX)
        {
            fprintf(path, "To node %d:No path exists\n", assigned_nodes[i]);
        }
        else
        {
            fprintf(path, "To node %d: %d\n", assigned_nodes[i], distance[assigned_nodes[i]]);
        }
    }

    pthread_mutex_unlock(&lock);

    free(distance);
    free(visited);
}
void *path_finder_thread(void *arg)
{

    ThreadArgs *threadArgs = (ThreadArgs *)arg;
    int size = numVertices * 5;
    char line[size];
    pthread_mutex_lock(&lock);
    while (fgets(line, sizeof(line), threadArgs->file))
    {

        // printf("-------%s---------\n\n",line);
        printf("Thread %d started\n", threadArgs->threadid);
        // int landmark_node;
        int *assigned_nodes = NULL;
        int num_assigned_nodes = 0;
        int initial_capacity = 10;

        // Allocate memory for the assigned_nodes array
        assigned_nodes = (int *)malloc(initial_capacity * sizeof(int));
        if (assigned_nodes == NULL)
        {
            fprintf(stderr, "Memory allocation error\n");
            exit(1);
        }

        char *token = strtok(line, " "); // Tokenize the input string using space as the delimiter

        while (token != NULL)
        {
            if (num_assigned_nodes >= initial_capacity)
            {
                // Double the array size
                initial_capacity *= 2;
                assigned_nodes = (int *)realloc(assigned_nodes, initial_capacity * sizeof(int));
                if (assigned_nodes == NULL)
                {
                    fprintf(stderr, "Memory allocation error\n");
                    exit(1);
                }
            }
            if (sscanf(token, "%d", &assigned_nodes[num_assigned_nodes]) == 1)
            {
                // printf("%d ", assigned_nodes[num_assigned_nodes]);
                num_assigned_nodes++;
            }

            token = strtok(NULL, " "); // Get the next token
        }

        printf("%d\n", num_assigned_nodes);
        pthread_mutex_unlock(&lock);
        dijkstra(assigned_nodes[0], assigned_nodes, num_assigned_nodes);

        free(assigned_nodes); // Free dynamically allocated memory
        printf("Thread %d completed\n", threadArgs->threadid);
    }

    // while (fscanf(threadArgs->file, "%d\t%d", &src, &dest) != EOF)
    // {
    //     pthread_mutex_lock(&mutex);
    //     addEdge(threadArgs->graph, src, dest);
    //     pthread_mutex_unlock(&mutex);
    // }

    pthread_exit(NULL);
}
void *path_finder_thread_landmark(void *arg)
{

    int src, dest;
    char line[100]; // Assuming each line will not exceed 100 characters

    fgets(line, sizeof(line), landmarkpairread);
    sscanf(line, "%d %d", &src, &dest);
    //printf("\n%d %d", src, dest);
    int *distance = (int *)malloc(sizeof(int) * numVertices);
    int *visited = (int *)malloc(sizeof(int) * numVertices);

    for (int i = 0; i < numVertices; i++)
    {
        visited[i] = 0;
        distance[i] = -1;
    }
    // bool visited[numVertices];
    // for (int i = 0; i < numVertices; i++)
    // {
    //     visited[i] = 0;
    // }
    visited[src] = 1;
    distance[src] = 0;

    int queue[numVertices];
    int front = 0, rear = 0;
    queue[rear++] = src;

    while (front < rear)
    {
        int u = queue[front++];

        struct Node *temp = graph->adjLists[u];
        struct Node *currentNode = graph->adjLists[u];
        while (currentNode != NULL)
        {
            int v = currentNode->data;
            if (!visited[v])
            {
                visited[v] = 1;
                distance[v] = distance[u] + 1;
                queue[rear++] = v;
            }
            if (v == dest)
            {
                pthread_mutex_lock(&lock2);
                fprintf(landmarklist, "Shortest distance between landmark %d and landmark %d: %d\n", src, dest, distance[v]);
                pthread_mutex_unlock(&lock2);
                pthread_exit(NULL);
            }
            currentNode = currentNode->next;
        }
    }
    pthread_mutex_lock(&lock2);
    fprintf(landmarklist, "Shortest distance between landmark %d and landmark %d: -1\n", src, dest);
    pthread_mutex_unlock(&lock2);
    free(distance);
    free(visited);
    pthread_exit(NULL);
}

// void* path_stitcher_thread(void* arg) {

//     while (true) {
//         // Determine a node pair that needs a path (you can use a queue or other mechanism)

//         int source = ...; // Get source node from the queue
//         int dest = ...;   // Get destination node from the queue

//         Path* path1 = NULL;
//         Path* path2 = NULL;
//         Path* path3 = NULL;

//         // Find paths from source to its landmark, from dest to its landmark, and between landmarks
//         findPath(source, landmark_of_source, &path1);
//         findPath(dest, landmark_of_dest, &path2);
//         findPath(landmark_of_source, landmark_of_dest, &path3);

//         if (path1 != NULL && path2 != NULL && path3 != NULL) {
//             // Combine the three paths and log
//             Path* combined_path = combinePaths(path1, path2, path3);

//             // Lock and remove the edges from the graph
//             EdgeToRemove edges_to_remove[MAX_EDGES_TO_REMOVE];
//             int num_edges_to_remove = extractEdgesToRemove(combined_path, edges_to_remove);
//             removeEdges(edges_to_remove, num_edges_to_remove);

//             // Log the path found and removed
//             logPath(source, dest, combined_path);

//             // Clean up data structures
//             freePath(combined_path);
//         } else {
//             // Log that no path was found
//             logNoPathFound(source, dest);
//         }
//     }

//     pthread_exit(NULL);

// }

void printGraph(Graph *graph)
{
    FILE *fptr = fopen("graph.txt", "w");
    for (int i = 0; i < graph->numVertices; i++)
    {
        fprintf(fptr, "Adjacency list for vertex %d: ", i);
        Node *currentNode = graph->adjLists[i];

        while (currentNode)
        {
            fprintf(fptr, "%d -> ", currentNode->data);
            currentNode = currentNode->next;
        }
        fprintf(fptr, "NULL\n");
    }
    // for (int i = 0; i < numVertices; i++)
    // {
    //     printf("Vertex %d is connected to: ", i);
    //     struct Node *currentNode = graph->adjLists[i];
    //     while (currentNode != NULL)
    //     {
    //         printf("%d ", currentNode->data);
    //         currentNode = currentNode->next;
    //     }
    //     printf("\n");
    // }
}

void extractLandmarks()
{
    FILE *inputFile = fopen("landmark.log", "r");
    if (inputFile == NULL)
    {
        printf("Error opening input file.\n");
        exit(1);
    }
    int size = numVertices * 5;
    // Read each line from the input file
    char line[size]; // Assuming a maximum line length of 1024 character
    int i = 0;
    while (fgets(line, sizeof(line), inputFile) != NULL)
    {
        int number;
        if (sscanf(line, "%d", &number) == 1)
        {
            // printf("%d ", number);
            //  Successfully read an integer from the line
            landmark[i++] = number; // Write the number to the output file
        }
    }
    FILE *fptr = fopen("parioflandmarks.txt", "w");
    for (int i = 0; i < 100; i++)
    {
        for (int j = i + 1; j < 100; j++)
        {
            fprintf(fptr, "%d %d\n", landmark[i], landmark[j]);
        }
    }
    // Close the input and output files
    fclose(fptr);
    fclose(inputFile);
    // fclose(outputFile);
}
int main()
{

    int num_threads = 25;
    updateLog = fopen("update.log", "w");
    path = fopen("path.log", "w");
    landmarkpairread = fopen("parioflandmarks.txt", "r");
    // Mutex for thread synchronization
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&lock2, NULL);
    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        // Handle initialization error
        perror("Mutex initialization failed");
        return 1;
    }

    char *filename = "edges.txt";
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("File not found");
        return 1;
    }
    int src, dest;
    while (fscanf(file, "%d %d", &src, &dest) != EOF)
    {
        if (src >= numVertices)
            numVertices = src + 1;
        if (dest >= numVertices)
            numVertices = dest + 1;
    }
    //printf("%d", numVertices);
    fclose(file);
    graph = createGraph(numVertices);
    ThreadArgs args[5];
    pthread_t thread1[5];

    file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("File not found");
        return 1;
    }

    for (int i = 0; i < 5; i++)
    {
        args[i].graph = graph;
        args[i].file = file;
    }
    for (int i = 0; i < 5; i++)
    {
        pthread_create(&thread1[i], NULL, readEdgesAndAddToGraph, &args[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < 5; i++)
    {
        pthread_join(thread1[i], NULL);
    }

    pthread_t threads[num_threads];
    int threadIDs[num_threads];
    // sem_init(&sem1, 0, 0);
    // sem_init(&sem2, 0, 0);
    // sem_init(&sem3, 0, 0);
    FILE *landmarkFile = fopen("landmark.log", "r");
    if (landmarkFile == NULL)
    {
        printf("Error opening landmark.log file.\n");
        return 1;
    }
    ThreadArgs arguments[num_threads];
    for (int i = 0; i < 25; i++)
    {
        arguments[i].graph = graph;
        arguments[i].file = landmarkFile;
        arguments[i].threadid = i;
    }
    // sem_post(&sem1);
    // Create and start graph_update threads

    extractLandmarks();
    
    for (int i = 0; i < num_threads; i++)
    {
        if (i < 5){
            pthread_create(&threads[i], NULL, graphUpdateThread, &arguments[i]);}
        else if (i > 4 && i < 25)
            pthread_create(&threads[i], NULL, path_finder_thread, &arguments[i]);
        // else
        //     pthread_create(&threads[i], NULL, path_stitcher_thread, &threadIDs[i]);
   }
    
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    int thread_count = 0;
    int NUM_THREADS = 10;
    pthread_t BFSthreads[NUM_THREADS];
    landmarklist = fopen("landmarkdistance.txt", "w");

    for (int i = 0; i < 100; i++)
    {
        for (int j = i + 1; j < 100; j++)
        {
            if (thread_count < NUM_THREADS)
            {
                //printf("call");
                pthread_create(&BFSthreads[thread_count], NULL, path_finder_thread_landmark, NULL);
                thread_count++;
            }
            else
            {
                // Wait for the threads to finish before creating new ones
                for (int k = 0; k < thread_count; k++)
                {
                    pthread_join(BFSthreads[k], NULL);
                }
                thread_count = 0;
                // Now, you can create new threads if necessary
                pthread_create(&BFSthreads[thread_count], NULL, path_finder_thread_landmark, NULL);
                thread_count++;
            }
        }
    }
    // After the loops, wait for any remaining threads to finish
    for (int k = 0; k < thread_count; k++)
    {
        pthread_join(BFSthreads[k], NULL);
    }
    // Join graph_update threads
    

    // // Close the log file and free resources
    fclose(landmarkpairread);
    fclose(landmarklist);
    fclose(updateLog);
    fclose(landmarkFile);
    fclose(path);
    printGraph(graph);
    pthread_mutex_destroy(&mutex);
    free(graph->adjLists);
    free(graph);
    fclose(file);
    printf("complete!!");
    return 0;
}
