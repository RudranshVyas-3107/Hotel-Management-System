#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define SHARED_MEMORY_SIZE 4096

// Define the structure for shared memory communication
struct TerminationRequest {
    char close_request; // 'Y' for yes, 'N' for no
};

int main() {
    key_t key;
    int shmid;
    struct TerminationRequest *termination_request;

    // Generate a key for the shared memory segment
    key = ftok("admin.c", 1234);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Create or open the shared memory segment
    shmid = shmget(key, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the shared memory segment to the process's address space
    termination_request = (struct TerminationRequest *)shmat(shmid, NULL, 0);
    if (termination_request == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Main loop for admin process
    while (1) {
        char input;
        printf("Do you want to close the hotel? Enter Y for Yes and N for No: ");
        scanf(" %c", &input);
        if (input == 'Y') {
            // Inform hotel manager that the hotel needs to close
            termination_request->close_request = 'Y';
            printf("Hotel closing request sent to the manager.\n");
            break; // Terminate the admin process
        } else if (input == 'N') {
            // Continue running the admin process
            continue;
        } else {
            printf("Invalid input. Please enter Y or N.\n");
        }
    }

    // Detach the shared memory segment
    if (shmdt(termination_request) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    //Delete the shared memory segment
    // if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    //     perror("shmctl");
    //     exit(EXIT_FAILURE);
    // }

    return 0;
}

