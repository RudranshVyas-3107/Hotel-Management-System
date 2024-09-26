
# Hotel Management System

A POSIX-compliant C-based simulation of a hotel environment using OS concepts like process creation, inter-process communication, shared memory, and pipes. This system involves processes representing tables, customers, waiters, and a hotel manager to manage orders and calculate bills.

## Features
- Table process manages customer processes (up to 5 per table).
- Waiter process handles orders and communicates the total bill.
- Hotel Manager calculates total earnings, wages, and profit.
- Admin process allows the hotel to close down.

## Getting Started
### Prerequisites
- Ubuntu 22.04 (as the code is tested on this OS)
- GCC Compiler

### Running the Code
1. **Table Process**: 
   Run `table.out` in multiple terminals to create multiple table processes.

   ```bash
   gcc -o table src/table.c
   ./table
   ```

2. **Waiter Process**:
   Similarly, run `waiter.out` for waiter processes in separate terminals.

   ```bash
   gcc -o waiter src/waiter.c
   ./waiter
   ```

3. **Hotel Manager Process**:
   Run `hotelmanager.out` in a separate terminal.

   ```bash
   gcc -o hotelmanager src/hotelmanager.c
   ./hotelmanager
   ```

4. **Admin Process**:
   Run `admin.out` to handle closing the hotel.

   ```bash
   gcc -o admin src/admin.c
   ./admin
   ```

### Example Execution
- The table process asks for table number and customer count.
- The waiter receives and validates orders.
- The manager calculates earnings and handles hotel closure.

### File Description
- `menu.txt`: Pre-defined menu used by the system.
- `earnings.txt`: Output file generated by the hotel manager showing earnings per table.


