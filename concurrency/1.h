#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define WHITE "\x1B[37m"
#define YELLOW "\x1B[33m"
#define CYAN "\x1B[36m"
#define BLUE "\x1B[34m"
#define GREEN "\x1B[32m"
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

// Assuming a fixed size for demonstration purposes.
#define MAX_CUSTOMERS 100
#define MAX_BARISTAS 10
#define MAX_COFFEE_TYPES 10