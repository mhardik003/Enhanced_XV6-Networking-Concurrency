#include "1.h"
#include <math.h>

// int coffee_types_time[MAX_CUSTOMERS];
// int customer_coffee_type[MAX_CUSTOMERS];
// int customer_arrival_time[MAX_CUSTOMERS];
// int customer_tolerance[MAX_CUSTOMERS];
// char *coffee_types[MAX_CUSTOMERS];

// typedef struct
// {
//     int c->id;
//     int served;
//     int barista_id;
//     int start_time;
// } order_info;

typedef struct customers
{
    int id;
    int arrival_time;
    int tolerance;
    char coffee_name[100];
    int coffee_prep_time;
    int barista_id;
    time_t abs_start_time;

} customer;

typedef struct coffees
{
    char name[100];
    int prep_time;
} coffee;

int B, K, N;

// One semaphore per barista.
sem_t barista_semaphores[MAX_BARISTAS];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int wasted_coffee = 0;
double average_wait_time = 0;

// Arrays for the coffees and the customers
coffee coffees[MAX_COFFEE_TYPES];
customer customers[MAX_CUSTOMERS];

// void printColoredMessage(const char* color, const char* msg) {
//     printf("%s%s%s\n", color, msg, RESET);
// }

void *customer_handler(void *arg)
{
    // order_info *order = (order_info *)arg;
    customer *c = (customer *)arg;

    c->abs_start_time = floor(time(NULL)); // Record the start time for this customer.

    int served = 0;
    struct timespec ts;

    // Wait for the customer's arrival time.
    sleep(c->arrival_time);

    printf(WHITE "Customer %d arrives at %d second(s)" RESET "\n", c->id + 1, c->arrival_time);
    printf(YELLOW "Customer %d orders an %s" RESET "\n", c->id + 1, c->coffee_name);

    // Check for available barista
    for (int i = 0; i < B; i++)
    {
        if (sem_trywait(&barista_semaphores[i]) == 0)
        {
            // printf(YELLOW "Customer %d orders a %s" RESET "\n", c->id + 1, c->coffee_name);
            sleep(1);
            int order_start_time = floor((time(NULL)) - c->abs_start_time);
            // average_wait_time += order_start_time - c->arrival_time;
            printf(CYAN "Barista %d begins preparing the order of customer %d at %d second(s)" RESET "\n", i + 1, c->id + 1, order_start_time);

            // Simulate the time taken to prepare the coffee.
            sleep(c->coffee_prep_time);

            int order_complete_time = floor((time(NULL)) - c->abs_start_time);

            printf(BLUE "Barista %d successfully completes the order of customer %d at %d seconds" RESET "\n", i + 1, c->id + 1, order_complete_time);
            served = 1;

            sem_post(&barista_semaphores[i]);
            break;
        }
    }

    // If not served yet, try to wait for a barista with a timeout.
    if (!served)
    {
        // Set the absolute time for timeout.
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += c->tolerance;

        for (int i = 0; i < B && !served; i++)
        {
            if (sem_timedwait(&barista_semaphores[i], &ts) == 0)
            {
                int barista_takes_order = floor((time(NULL)));
                int delay = barista_takes_order - c->abs_start_time - c->arrival_time;
                average_wait_time += delay - 1;

                // printf("Delay for customer %d is %d\n", c->id, delay);

                if (c->tolerance < delay + c->coffee_prep_time)
                {
                    // sleep(1);
                    int order_start_time = floor(time(NULL)) - c->abs_start_time;
                    printf(CYAN "Barista %d begins preparing the order of customer %d at %d second(s)" RESET "\n", i + 1, c->id + 1, order_start_time);

                    // Simulate the time taken to prepare the coffee.
                    // printf("Sleeping for customoer %d for %d seconds\n", c->id + 1, c->tolerance - delay);
                    sleep(c->tolerance - delay);

                    printf(RED "Customer %d leaves without their order at %d second(s)" RESET "\n", c->id + 1, c->tolerance + order_start_time - delay + 1);
                    wasted_coffee++;

                    // printf("Sleeping for customer %d for %d seconds\n ", c->id +1, c->coffee_prep_time - c->tolerance - delay);
                    sleep(c->coffee_prep_time - c->tolerance + delay);

                    int order_complete_time = floor((time(NULL)) - c->abs_start_time);
                    printf(BLUE "Barista %d successfully completes the order of customer %d at %d second(s)" RESET "\n", i + 1, c->id + 1, order_complete_time);

                    sem_post(&barista_semaphores[i]);
                    // free(c);
                    return NULL;
                }

                // printf(YELLOW "Customer %d orders an espresso" RESET "\n", c->id + 1);
                sleep(1);
                int order_start_time = floor((time(NULL)) - c->abs_start_time);

                printf(CYAN "Barista %d begins preparing the order of customer %d at %d second(s)" RESET "\n", i + 1, c->id + 1, order_start_time);

                // Simulate the time taken to prepare the coffee.
                sleep(c->coffee_prep_time);

                int order_complete_time = floor((time(NULL)) - c->abs_start_time);
                printf(BLUE "Barista %d successfully completes the order of customer %d at %d second(s)" RESET "\n", i + 1, c->id + 1, order_complete_time);
                served = 1;
                // printf("Served the customer %d", s)

                sem_post(&barista_semaphores[i]);
            }
        }
    }

    int customer_leave_time = floor((time(NULL)) - c->abs_start_time);
    if (served)
    {
        printf(GREEN "Customer %d leaves with their order at %d second(s)" RESET "\n", c->id + 1, customer_leave_time);
    }
    else
    {
        wasted_coffee++;
        printf(RED "Customer %d leaves without their order at %d second(s)" RESET "\n", c->id + 1, customer_leave_time);
    }

    // free(c);
    return NULL;
}

void check_input()
{

    printf("---- PRINTING THE COFFEE TYPES ----\n");
    // printf("\n");
    for (int i = 0; i < K; i++)
    {
        printf("Name of the coffee : %s\t|\tIts Prep Time : %d\n", coffees[i].name, coffees[i].prep_time);
    }
    printf("\n");
    printf("\n");

    printf("---- PRINTING THE CUSTOMERS ----\n");
    for (int i = 0; i < N; i++)
    {
        printf("Customer ID : %d\t|\t Coffee ordered %s \t|\t Arrival Time : %d\t|\t Tolerance Time %d\n", customers[i].id, customers[i].coffee_name, customers[i].arrival_time, customers[i].tolerance);
    }
    printf("\n\n");
}

void get_input()
{
    // B --> B baristas, K --> K types of coffee, N --> N customers
    scanf("%d %d %d", &B, &K, &N);
    scanf("\n");

    // for (int i = 0; i < K; i++)
    // {
    //     coffee_types[i] = (char *)malloc(100 * sizeof(char));
    // }

    for (int i = 0; i < K; i++)
    {
        scanf("%s %d", coffees[i].name, &coffees[i].prep_time);
        // coffee_types_time[i] = coffees[i].prep_time;
        // strcpy(coffee_types[i], coffees[i].name);
    }
    scanf("\n");

    for (int i = 0; i < N; i++)
    {
        int customer_num;
        scanf("%d %s %d %d", &customers[i].id, customers[i].coffee_name, &customers[i].arrival_time, &customers[i].tolerance);
        customers[i].id--;

        for (int j = 0; j < K; j++)
        {
            if (strcmp(customers[i].coffee_name, coffees[j].name) == 0)
            {
                customers[i].coffee_prep_time = coffees[j].prep_time;
                strcmp(customers[i].coffee_name, coffees[j].name);
                break;
            }
        }
    }

    // check_input();
}

void initialize_threads_semaphores()
{
    // Initialize semaphores for baristas and set availability.

    for (int i = 0; i < B; i++)
    {
        sem_init(&barista_semaphores[i], 0, 1); // Initially all baristas are available.
    }
}

void clean_mem();

int main()
{

    get_input();
    initialize_threads_semaphores();

    // Create customer threads.
    pthread_t customer_handlers[N];
    for (int i = 0; i < N; i++)
    {
        pthread_create(&customer_handlers[i], NULL, customer_handler, &customers[i]);
    }

    // Wait for all customer threads to finish.
    for (int i = 0; i < N; i++)
    {
        pthread_join(customer_handlers[i], NULL);
    }

    clean_mem();

    // Free allocated memory for coffee names.

    printf("\nWasted Coffee: %d\n", wasted_coffee);
    printf("Average Wait Time: %lf\n", (double)(average_wait_time / (double)N));

    return 0;
}

void clean_mem()
{
    // Destroy semaphores and mutex.
    for (int i = 0; i < B; i++)
    {
        sem_destroy(&barista_semaphores[i]);
    }

    pthread_mutex_destroy(&mutex);

    // free(coffee);
    // free(customer);
}
