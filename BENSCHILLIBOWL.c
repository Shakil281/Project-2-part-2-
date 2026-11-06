#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = {
    "BensChilli",
    "BensHalfSmoke",
    "BensHotDog",
    "BensChilliCheeseFries",
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    return BENSCHILLIBOWLMenu[rand() % BENSCHILLIBOWLMenuLength];
}

/* Allocate memory for the Restaurant, then create the mutex and condition
   variables needed to instantiate the Restaurant */
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL* bcb = (BENSCHILLIBOWL*) malloc(sizeof(BENSCHILLIBOWL));
    if (bcb == NULL) {
        perror("malloc");
        exit(1);
    }

    bcb->orders              = NULL;
    bcb->current_size        = 0;
    bcb->max_size            = max_size;
    bcb->next_order_number   = 1;
    bcb->orders_handled      = 0;
    bcb->expected_num_orders = expected_num_orders;

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    printf("Restaurant is open!\n");
    return bcb;
}

/* check that the number of orders received is equal to the number handled
   (i.e. fulfilled). Remember to deallocate your resources */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    assert(bcb->orders_handled == bcb->expected_num_orders);
    assert(bcb->current_size == 0);

    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);
    free(bcb);

    printf("Restaurant is closed!\n");
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&bcb->mutex);

    /* wait while the restaurant is full (bounded buffer) */
    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    int order_number = bcb->next_order_number;
    order->order_number = order_number;
    bcb->next_order_number++;
    order->next = NULL;

    AddOrderToBack(&bcb->orders, order);
    bcb->current_size++;

    /* a cook can now take an order */
    pthread_cond_broadcast(&bcb->can_get_orders);

    pthread_mutex_unlock(&bcb->mutex);

    /* NOTE: do not touch `order` after unlocking - a cook may already
       have fulfilled and freed it. Return the saved copy instead. */
    return order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    /* wait while the queue is empty, unless every expected order has
       already been handled (then there is nothing left to cook) */
    while (IsEmpty(bcb)) {
        if (bcb->orders_handled >= bcb->expected_num_orders) {
            pthread_mutex_unlock(&bcb->mutex);
            return NULL;
        }
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    /* pop from the front of the queue */
    Order *order = bcb->orders;
    bcb->orders  = order->next;
    order->next  = NULL;
    bcb->current_size--;
    bcb->orders_handled++;

    /* if this was the final order, wake every waiting cook so they exit */
    if (bcb->orders_handled >= bcb->expected_num_orders) {
        pthread_cond_broadcast(&bcb->can_get_orders);
    }

    /* a customer can now add an order */
    pthread_cond_broadcast(&bcb->can_add_orders);

    pthread_mutex_unlock(&bcb->mutex);
    return order;
}

/* Optional helper functions */
bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size >= bcb->max_size;
}

/* this method adds order to rear of queue */
void AddOrderToBack(Order **orders, Order *order) {
    order->next = NULL;
    if (*orders == NULL) {
        *orders = order;
        return;
    }
    Order *current = *orders;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = order;
}