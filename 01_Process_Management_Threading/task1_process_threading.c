#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define WORKER_COUNT 3
#define JOBS_PER_WORKER 3
#define MAX_PROCESSES 10

typedef struct {
    int workerId;
    int jobsToComplete;
} WorkerData;

typedef struct {
    int processId;
    int burstTime;
    int remainingTime;
    int completionTime;
    int waitingTime;
    int turnaroundTime;
} Process;

int completedJobs = 0;

pthread_mutex_t sharedDataMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resourceAMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resourceBMutex = PTHREAD_MUTEX_INITIALIZER;

void *workerTask(void *argument)
{
    WorkerData *worker = (WorkerData *) argument;

    for (int job = 1; job <= worker->jobsToComplete; job++) {
        /*
         * Every thread locks the resources in the same order.
         * This prevents circular waiting and reduces deadlock risk.
         */
        pthread_mutex_lock(&resourceAMutex);
        pthread_mutex_lock(&resourceBMutex);

        /*
         * completedJobs is shared by all threads, so the update
         * must be protected to prevent a race condition.
         */
        pthread_mutex_lock(&sharedDataMutex);

        completedJobs++;

        printf(
            "Worker %d completed job %d. Total completed jobs: %d\n",
            worker->workerId,
            job,
            completedJobs
        );

        pthread_mutex_unlock(&sharedDataMutex);

        pthread_mutex_unlock(&resourceBMutex);
        pthread_mutex_unlock(&resourceAMutex);
    }

    return NULL;
}

int runThreadSimulation(void)
{
    pthread_t workers[WORKER_COUNT];
    WorkerData workerData[WORKER_COUNT];
    int createdWorkerCount = 0;

    printf("========================================\n");
    printf("THREAD AND SYNCHRONIZATION SIMULATION\n");
    printf("========================================\n");

    for (int index = 0; index < WORKER_COUNT; index++) {
        workerData[index].workerId = index + 1;
        workerData[index].jobsToComplete = JOBS_PER_WORKER;

        int createResult = pthread_create(
            &workers[index],
            NULL,
            workerTask,
            &workerData[index]
        );

        if (createResult != 0) {
            printf(
                "Error: Could not create worker thread %d.\n",
                index + 1
            );
            break;
        }

        createdWorkerCount++;
    }

    for (int index = 0; index < createdWorkerCount; index++) {
        int joinResult = pthread_join(workers[index], NULL);

        if (joinResult != 0) {
            printf(
                "Error: Could not join worker thread %d.\n",
                index + 1
            );
            return 0;
        }
    }

    if (createdWorkerCount != WORKER_COUNT) {
        printf("Thread simulation could not start all workers.\n");
        return 0;
    }

    printf("\nThread simulation completed.\n");
    printf("Expected completed jobs: %d\n", WORKER_COUNT * JOBS_PER_WORKER);
    printf("Actual completed jobs  : %d\n", completedJobs);

    if (completedJobs == WORKER_COUNT * JOBS_PER_WORKER) {
        printf("Shared data was updated correctly.\n");
    } else {
        printf("Warning: Shared data result is incorrect.\n");
    }

    return 1;
}

int readPositiveInteger(const char *message, int *value)
{
    printf("%s", message);

    if (scanf("%d", value) != 1) {
        printf("Error: Please enter a valid whole number.\n");
        return 0;
    }

    if (*value <= 0) {
        printf("Error: The value must be greater than zero.\n");
        return 0;
    }

    return 1;
}

void displayProcessTable(Process processes[], int processCount)
{
    printf("\n============================================================\n");
    printf("ROUND-ROBIN RESULTS\n");
    printf("============================================================\n");
    printf(
        "%-8s %-10s %-12s %-12s %-12s\n",
        "PID",
        "Burst",
        "Completion",
        "Waiting",
        "Turnaround"
    );

    for (int index = 0; index < processCount; index++) {
        printf(
            "%-8d %-10d %-12d %-12d %-12d\n",
            processes[index].processId,
            processes[index].burstTime,
            processes[index].completionTime,
            processes[index].waitingTime,
            processes[index].turnaroundTime
        );
    }
}

void runRoundRobinScheduler(void)
{
    Process processes[MAX_PROCESSES];
    int processCount;
    int timeQuantum;
    int currentTime = 0;
    int completedProcessCount = 0;
    float totalWaitingTime = 0.0f;
    float totalTurnaroundTime = 0.0f;

    printf("\n========================================\n");
    printf("ROUND-ROBIN SCHEDULING SIMULATION\n");
    printf("========================================\n");

    if (!readPositiveInteger(
            "Enter number of processes: ",
            &processCount
        )) {
        return;
    }

    if (processCount > MAX_PROCESSES) {
        printf(
            "Error: The maximum number of processes is %d.\n",
            MAX_PROCESSES
        );
        return;
    }

    for (int index = 0; index < processCount; index++) {
        int burstTime;

        printf(
            "Enter burst time for process P%d: ",
            index + 1
        );

        if (scanf("%d", &burstTime) != 1 || burstTime <= 0) {
            printf("Error: Burst time must be a positive whole number.\n");
            return;
        }

        processes[index].processId = index + 1;
        processes[index].burstTime = burstTime;
        processes[index].remainingTime = burstTime;
        processes[index].completionTime = 0;
        processes[index].waitingTime = 0;
        processes[index].turnaroundTime = 0;
    }

    if (!readPositiveInteger(
            "Enter the time quantum: ",
            &timeQuantum
        )) {
        return;
    }

    printf("\nExecution order\n");
    printf("----------------\n");

    while (completedProcessCount < processCount) {
        for (int index = 0; index < processCount; index++) {
            if (processes[index].remainingTime <= 0) {
                continue;
            }

            int executionTime;

            if (processes[index].remainingTime > timeQuantum) {
                executionTime = timeQuantum;
            } else {
                executionTime = processes[index].remainingTime;
            }

            printf(
                "Time %d to %d: P%d runs for %d unit(s)\n",
                currentTime,
                currentTime + executionTime,
                processes[index].processId,
                executionTime
            );

            currentTime += executionTime;
            processes[index].remainingTime -= executionTime;

            if (processes[index].remainingTime == 0) {
                processes[index].completionTime = currentTime;
                processes[index].turnaroundTime =
                    processes[index].completionTime;
                processes[index].waitingTime =
                    processes[index].turnaroundTime -
                    processes[index].burstTime;

                completedProcessCount++;

                printf(
                    "P%d completed at time %d\n",
                    processes[index].processId,
                    currentTime
                );
            }
        }
    }

    for (int index = 0; index < processCount; index++) {
        totalWaitingTime += processes[index].waitingTime;
        totalTurnaroundTime += processes[index].turnaroundTime;
    }

    displayProcessTable(processes, processCount);

    printf(
        "\nAverage waiting time    : %.2f\n",
        totalWaitingTime / processCount
    );

    printf(
        "Average turnaround time : %.2f\n",
        totalTurnaroundTime / processCount
    );

    // TODO: Add process arrival times if a more realistic scheduler is required.
}

int main(void)
{
    printf("========================================\n");
    printf("PROCESS MANAGEMENT AND THREADING\n");
    printf("========================================\n\n");

    if (!runThreadSimulation()) {
        pthread_mutex_destroy(&sharedDataMutex);
        pthread_mutex_destroy(&resourceAMutex);
        pthread_mutex_destroy(&resourceBMutex);

        return EXIT_FAILURE;
    }

    runRoundRobinScheduler();

    pthread_mutex_destroy(&sharedDataMutex);
    pthread_mutex_destroy(&resourceAMutex);
    pthread_mutex_destroy(&resourceBMutex);

    return EXIT_SUCCESS;
}
