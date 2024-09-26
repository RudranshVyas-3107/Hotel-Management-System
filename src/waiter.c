#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define MAX_MENU_ITEM_LENGTH 100
#define MAX_CUSTOMERS 5
#define MAX_ITEMS 10
#define SHARED_MEMORY_SIZE 4096

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

int menu_prices[10];

// Function to receive order from table
struct Order receive_order_from_table(int waiter_id){
    struct Order order;
    int shmid;
    key_t key;
    void *shm;
    printf("Waiting to receive orders:\n");
    // Generate a key based on the waiter ID
    key = ftok("table.c", waiter_id);
    if (key == -1){
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    // Create or open the shared memory segment
    shmid = shmget(key, SHARED_MEMORY_SIZE, 0666);
    if (shmid == -1){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    // Attach the shared memory segment to the process's address space
    if ((shm = shmat(shmid, NULL, 0)) == (void *)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    // Copy the order from shared memory
    memcpy(&order, shm, sizeof(struct Order));
    while (order.table_number != waiter_id){
        sleep(1);
        memcpy(&order, shm, sizeof(struct Order));
    }
    // Detach the shared memory segment
    if (shmdt(shm) == -1){
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    struct Bill *bill_info;
    bill_info = (struct Bill *)shmat(shmid, NULL, 0);
    if (bill_info == (void *)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    // Write the bill information to shared memory
    bill_info->table_number = 0;
    return order;
}
// Function to calculate total bill amount
int calculate_bill_amount(struct Order order){
    int total_amount = 0;
    for (int i = 1; i < order.num_items; i++){
        total_amount += menu_prices[i] * order.items[i]; // Subtracting 1 because item numbers start from 1
    }
    return total_amount;
}

int send_bill_amount_to_table(int table_number, int bill_amount,int order_zero){
    key_t key;
    int shmid;
    struct Bill *bill_info;
    // Generate a key for the shared memory segment
    key = ftok("table.c", table_number);
    if (key == -1){
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    // Create or open the shared memory segment
    shmid = shmget(key, sizeof(struct Bill), 0666);
    if (shmid < 0){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    // Attach the shared memory segment to the process's address space
    bill_info = (struct Bill *)shmat(shmid, NULL, 0);
    if (bill_info == (void *)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    // Write the bill information to shared memory
    bill_info->table_number = table_number;
    bill_info->bill_amount = bill_amount;
    // Notify the table process that the bill amount is ready
    if(bill_amount>0){
        printf("Bill amount for Table %d: %d INR\n", table_number, bill_amount);
        printf("Bill amount sent to the table.\n");
    }
    else{
        printf("Invalid order, please input a valid order.\n");
    }
    if(order_zero)
        return 1;
    while(bill_info->table_number !=100){
        sleep(1);
    }
    return bill_info->bill_amount;
}
// Function to send bill amount to hotel manager
void send_bill_amount_to_hotel_manager(int waiter_id, int bill_amount){
    key_t key;
    int shmid;
    struct Bill *bill_info;
    // Generate a key for the shared memory segment
    key = ftok("waiter.c", waiter_id);
    if (key == -1){
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    // Create or open the shared memory segment
    shmid = shmget(key, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);
    if (shmid < 0){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    // Attach the shared memory segment to the process's address space
    bill_info = (struct Bill *)shmat(shmid, NULL, 0);
    if (bill_info == (void *)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    // Write the bill information to shared memory
    bill_info->bill_amount = bill_amount;
    bill_info->table_number = waiter_id; // Assuming waiter_id is synonymous with table_number in this context
    // Detach the shared memory segment
    if (shmdt(bill_info) == -1){
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
}
void menu_prices_fuction(){
    FILE *menu_file;
    char menu_item[MAX_MENU_ITEM_LENGTH];
    menu_file = fopen("menu.txt", "r");
    if (menu_file == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    int i = 1;
    while (fgets(menu_item, sizeof(menu_item), menu_file) != NULL){
        int size = strlen(menu_item);
        int num = 0;
        int flag = 0;
        int pos = 1;
        for (int i = size - 1; i >= 0; i--){
            if (menu_item[i] >= '0' && menu_item[i] <= '9'){
                flag = 1;
                int temp = menu_item[i] - '0';
                num += pos * temp;
                pos *= 10;
                continue;
            }
            if (flag)
                break;
        }
        menu_prices[i] = num;
        i++;
    }
    fclose(menu_file);
}

int main(){
    int waiter_id;
    struct Order order;
    menu_prices_fuction();
    printf("Enter Waiter ID: ");
    scanf("%d", &waiter_id);
    printf("Waiter ID: %d\n", waiter_id);
    // Receive order from table
    int bill_amount = -1;
    int total=0;
    while (bill_amount == -1){
        order = receive_order_from_table(waiter_id);
        // Process the received order
        printf("Received order from table %d.\n", order.table_number);
        printf("\n");
        if (order.items[0] == 0){
            bill_amount = calculate_bill_amount(order);
            total+=bill_amount;
        }
        int k=send_bill_amount_to_table(order.table_number, bill_amount,order.items[0]);
        if(k==1)
            bill_amount=-1;
        sleep(1);
    }
    // Send bill amount to the hotel manager
    send_bill_amount_to_hotel_manager(waiter_id, total);
    printf("Waiter process terminating.\n");
    return 0;
}
