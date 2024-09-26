#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define SHM_SIZE 4096
#define MAX_WAITERS 10

struct Bill {
    int table_number;
    int bill_amount;
};

// Define the structure for shared memory communication
struct TerminationRequest {
    char close_request; // 'Y' for yes, 'N' for no
};

int main() {
    int total_tables;
    int total_earnings = 0; // Variable to store total earnings
    int total_wages = 0;    // Variable to store total wages
    FILE *file;
    // Receive input from the user for the total number of tables
    printf("Enter the Total Number of Tables at the Hotel: ");
    scanf("%d", &total_tables);
    
     // Attach to shared memory
    key_t key1 = ftok("admin.c", 1234);
    if (key1 == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Create or open the shared memory segment
    int shmid = shmget(key1, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the shared memory segment to the process's address space
    struct TerminationRequest *termination_request = shmat(shmid, NULL, 0);
    if (termination_request == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    termination_request->close_request='N';
    
    // Open the earnings.txt file in append mode
    file = fopen("earnings.txt", "a");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    // Hotel manager logic to collect earnings from waiters via shared memory and write to file
    struct Bill *bill_info[total_tables+1]; 
    for (int i = 1; i <= total_tables; ++i) {
        key_t key2 = ftok("waiter.c", i);
        if (key2 == -1) {
            perror("ftok");
            exit(EXIT_FAILURE);
        }
        // Create or open the shared memory segment
        int shmid = shmget(key2, SHM_SIZE,IPC_CREAT |0666);
        if (shmid < 0) {
            perror("shmget");
            exit(EXIT_FAILURE);
        }
        bill_info[i]= shmat(shmid, NULL, 0);
        if (bill_info[i] == (void *) -1) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }
        bill_info[i]->table_number=0;
    }
    int k = 0;
    while (1) {
        for (int i = 1; i <= total_tables; i++) {
            if (bill_info[i]->table_number == 100) {
                continue; // Skip tables for which earnings are already collected
            } else if (bill_info[i]->table_number == 0) {
                continue; // Skip tables that have no earnings yet
            } else {
                k++;
                file = fopen("earnings.txt", "a");
                if (file == NULL) {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }
                // Collect earnings from the table
                int earnings = bill_info[i]->bill_amount;
                total_earnings += earnings;
                // Write earnings to the file
                fprintf(file, "Earning from Table %d: %d INR\n", i, earnings);
                fclose(file);
                // Mark table as processed
                bill_info[i]->table_number = 100;
            }
        }
        // Check if earnings are collected from all tables
        if (k == total_tables) {
            break;
        }
        sleep(2); // Wait for 2 seconds before checking again
    }

    while(termination_request->close_request == 'N')
    {
        sleep(1);
    }
    // Calculate total wages (40% of total earnings)
    sleep(5);
    total_wages = total_earnings * 0.4;

    // Calculate total profit
    int total_profit = total_earnings - total_wages;

    // Open the file again to append total earnings, total wages, and total profit
    file = fopen("earnings.txt", "a");
    if (file == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
    }

    // Write total earnings, total wages, and total profit to the file
    fprintf(file, "Total Earnings of Hotel: %d INR\n", total_earnings);
    fprintf(file, "Total Wages of Waiters: %d INR\n", total_wages);
    fprintf(file, "Total Profit: %d INR\n", total_profit);

    printf("Total Earnings of Hotel: %d INR\n", total_earnings);
    printf("Total Wages of Waiters: %d INR\n", total_wages);
    printf("Total Profit: %d INR\n", total_profit);
    
    // Close the file
    fclose(file);
    
    // Detach the shared memory segment
    if (shmdt(termination_request) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    
    // Exit the hotel manager process
    printf("Thank you for visiting the Hotel!\n");    
    return 0;
}

    

