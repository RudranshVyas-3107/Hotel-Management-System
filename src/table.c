#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define MAX_CUSTOMERS 5
#define MAX_MENU_ITEM_LENGTH 100
#define SHARED_MEMORY_SIZE 4096
#define MAX_ITEMS 10

int temp=1;
int option;
struct Order
{

    int table_number;

    int num_items;

    int items[MAX_ITEMS];
};

struct Bill
{

    int table_number;

    int bill_amount;
};

int number_of_items = 1;
// Function to display the menu
void display_menu(){
    FILE *menu_file;
    char menu_item[MAX_MENU_ITEM_LENGTH];
    menu_file = fopen("menu.txt", "r");
    if (menu_file == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    printf("Menu:\n");
    while (fgets(menu_item, sizeof(menu_item), menu_file) != NULL){
        printf("%s", menu_item);
        number_of_items++;
    }
    fclose(menu_file);
}
// Function to create customer processes
void create_customer_processes(int table_number, int num_customers, int *order_items){
    int i;
    int pipefd[num_customers][2]; // Array of pipes for communication with customers
    // Create pipes for each customer
    for (i = 0; i < num_customers; i++){
        if (pipe(pipefd[i]) == -1){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    // Create customer processes
    for (i = 0; i < num_customers; i++){
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0){ // Child process (customer)
            close(pipefd[i][0]);
            int temp[number_of_items];
            for (int k = 0; k < number_of_items; k++)
                temp[k] = 0;
            int order;
            printf("\n Enter the serial number(s) of the item(s) to order from the menu. Enter -1 when done: \n");
            do
            {
                int freq;
                scanf("%d", &order);
                if (order == -1)
                    break;
                scanf("%d",&freq);    
                if (order >= number_of_items){
                    temp[0]++;
                }
                else{
                    temp[order] += freq;
                }
            } while (order != -1);
            write(pipefd[i][1], temp, sizeof(temp));
            close(pipefd[i][1]); // Close write end of the pipe
            exit(EXIT_SUCCESS);
        }
        // Parent process (table)
        else{
            close(pipefd[i][1]); // Close write end of the pipe
            wait(NULL);
            printf("Customer %d sitting at table %d\n", i+1, table_number);
            int temp[number_of_items];
            read(pipefd[i][0], temp, sizeof(temp)); // Read one item at a time
            for (int k = 0; k < number_of_items; k++){
                order_items[k] += temp[k];
            }
            close(pipefd[i][0]); // Close read end of the pipe
        }
    }
}
void send_order_to_waiter(int table_number, int num_customers, int *order_items, int shmid){
    struct Order *order;
    // Attach the shared memory segment to the address space of the process
    order = (struct Order *)shmat(shmid, NULL, 0);
    if (order == (void *)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    // Populate the shared memory segment with the order data
    order->table_number = table_number;
    order->num_items = number_of_items;
    memcpy(order->items, order_items, number_of_items * sizeof(int));
    // Inform the waiter process that the order is ready
    printf("Table %d has sent the order to the waiter.\n", table_number);
    sleep(1.5);
    // Detach the shared memory segment
    if (shmdt(order) == -1){
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
}
int receive_bill_from_waiter(int table_number){
    key_t key;
    int shmid;
    struct Bill *bill;
    // Generate a key for the shared memory segment
    key = ftok("table.c", table_number);
    if (key == -1){
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    // Access the shared memory segment
    shmid = shmget(key, sizeof(struct Bill), 0666);
    if (shmid == -1){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    // Attach the shared memory segment to the address space of the process
    bill = (struct Bill *)shmat(shmid, NULL, 0);
    if (bill == (void *)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    while (bill->table_number != table_number){
        sleep(1);
    }
    int total_amount = bill->bill_amount;
    // Detach the shared memory segment
    if (shmdt(bill) == -1){
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    return total_amount;
}

int main(){
    int table_number;
    int num_customers;
    int order_items[100];
    printf("Enter Table Number: ");
    scanf("%d", &table_number);
    key_t key;
    int shmid;
    struct Order *order;
    // Generate a key for the shared memory segment
    key = ftok("table.c", table_number);
    if (key == -1){
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    // Create or access the shared memory segment
    shmid = shmget(key, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);
    if (shmid == -1){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    else{
        // Attach the shared memory segment to the address space of the process
        order = (struct Order *)shmat(shmid, NULL, 0);
        if (order == (void *)-1){
            perror("shmat");
            exit(EXIT_FAILURE);
        }
        // Populate the shared memory segment with the order data
        order->table_number = 0;
    }
    printf("Enter Number of Customers at Table (maximum no. of customers can be 5): ");
    scanf("%d", &num_customers);
    while(temp==1){
        number_of_items=1;
        order->table_number = 0;
        if (num_customers < 1 || num_customers > MAX_CUSTOMERS){
            fprintf(stderr, "Invalid number of customers. Must be between 1 and 5.\n");
            exit(EXIT_FAILURE);
        }
        printf("Table %d created with %d customers.\n", table_number, num_customers);
        display_menu();
        int bill_amount = -1;
        key_t key;
        int shmid;
        struct Bill *bill;
        // Generate a key for the shared memory segment
        key = ftok("table.c", table_number);
        if (key == -1){
            perror("ftok");
            exit(EXIT_FAILURE);
        }
        // Access the shared memory segment
        shmid = shmget(key, sizeof(struct Bill), 0666);
        if (shmid == -1){
            perror("shmget");
            exit(EXIT_FAILURE);
        }
        // Attach the shared memory segment to the address space of the process
        bill = (struct Bill *)shmat(shmid, NULL, 0);
        if (bill == (void *)-1){
            perror("shmat");
            exit(EXIT_FAILURE);
        }
        while (bill_amount == -1){
            for (int i = 0; i < number_of_items; i++){
                order_items[i] = 0;
            }
            create_customer_processes(table_number, num_customers, order_items);
            // Sending the entire order to the waiter process
            send_order_to_waiter(table_number, num_customers, order_items, shmid);
            // Receive bill amount from waiter
            bill_amount = receive_bill_from_waiter(table_number);
            order->table_number = 0;
        }
        // Display the bill amount
        printf("The total bill amount is %d INR.\n", bill_amount);
        printf("If you wish to seat new customers, Enter any value between 1-5, else enter -1\n");
        scanf("%d",&option);        
        if(option==-1){
            temp=0;
        }
        else if(option>=1 && option<6){
            temp=1;
        }
        else{
            printf("invalid response\n");
            exit(0);
        }
        bill->bill_amount=temp;
        bill->table_number=100;
        sleep(1.5);
        if(temp==0)
            break;
        num_customers=option;
    }
    return 0;
}
