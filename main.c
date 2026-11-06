#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "BENSCHILLIBOWL.h"

// Feel free to play with these numbers! This is a great way to
// test your implementation.
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS NUM_CUSTOMERS * ORDERS_PER_CUSTOMER

// Global variable for the restaurant.
BENSCHILLIBOWL *bcb;

/**
 * Thread funtion that represents a customer. A customer should:
 *  - allocate space (memory) for an order.
 *  - select a menu item.
 *  - populate the order with their menu item and their customer ID.
 *  - add their order to the restaurant.
 */
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long) tid;

    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {
        /* allocate space for the order */
        Order *order = (Order*) malloc(sizeof(Order));
        if (order == NULL) {
            perror("malloc");
            exit(1);
        }

        /* select a menu item and populate the order */
        order->menu_item   = PickRandomMenuItem();
        order->customer_id = customer_id;
        order->next        = NULL;

        /* Save the item name now: once the order is handed to the
           restaurant a cook may fulfill and free() it at any moment,
           so we must not touch `order` after AddOrder() returns. */
        MenuItem item = order->menu_item;

        /* add the order to the restaurant */
        int order_number = AddOrder(bcb, order);
        printf("Customer #%d placed order #%d for %s\n",
               customer_id, order_number, item);
    }

    return NULL;
}

/**
 * Thread function that represents a cook in the restaurant. A cook should:
 *  - get an order from the restaurant.
 *  - if the order is valid, it should fulfill the order, and then
 *    free the space taken by the order.
 * The cook should take orders from the restaurants until it does not
 * receive an order.
 */
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long) tid;
    int orders_fulfilled = 0;

    Order *order;
    while ((order = GetOrder(bcb)) != NULL) {
        printf("Cook #%d fulfilled order #%d (%s) for customer #%d\n",
               cook_id, order->order_number, order->menu_item,
               order->customer_id);
        free(order);
        orders_fulfilled++;
    }

    printf("Cook #%d fulfilled %d orders\n", cook_id, orders_fulfilled);
    return NULL;
}

/**
 * Runs when the program begins executing. This program should:
 *  - open the restaurant
 *  - create customers and cooks
 *  - wait for all customers and cooks to be done
 *  - close the restaurant.
 */
int main() {
    srand(time(NULL));

    /* open the restaurant */
    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);

    pthread_t customers[NUM_CUSTOMERS];
    pthread_t cooks[NUM_COOKS];

    /* create the cooks */
    for (long i = 0; i < NUM_COOKS; i++) {
        if (pthread_create(&cooks[i], NULL, BENSCHILLIBOWLCook, (void*)(i + 1)) != 0) {
            perror("pthread_create (cook)");
            exit(1);
        }
    }

    /* create the customers */
    for (long i = 0; i < NUM_CUSTOMERS; i++) {
        if (pthread_create(&customers[i], NULL, BENSCHILLIBOWLCustomer, (void*)(i + 1)) != 0) {
            perror("pthread_create (customer)");
            exit(1);
        }
    }

    /* wait for all customers to finish ordering */
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    /* wait for all cooks to finish cooking */
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_join(cooks[i], NULL);
    }

    /* close the restaurant */
    CloseRestaurant(bcb);

    return 0;
}