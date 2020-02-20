/*
 * catsem.c
 *
 * Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include "catmouse.h"

// added this
#include <synch.h>

// access controls
struct semaphore *cat_or_mouse;
struct semaphore *bowl_access;

// is the bowl being eaten from (1 means yes, 0 means no)
int bowl1 = 0;
int bowl2 = 0;

// iteration number
int it = 0;

/*
 * 
 * Function Definitions
 * 
 */

/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */


        /* 
         * a thread was created/forked from catmousesem
         * now let this thread try and access the food/semaphore
         * if it can't (2 threads already eating), sleep
         * else if a mouse thread is eating, sleep
        */

        int meal;
        // while the thread has not eaten all its meals yet
        for (meal = 0; meal < NMEALS; meal++) {
                // If a cat is eating, it won't be able to acquire the semaphore
                P(cat_or_mouse);
                // If there are 2 things eating, it won't be able to acquire the other semaphore
                P(bowl_access);
                // critical region
                if (bowl1) {
                        catmouse_eat("cat", catnumber, 2, it);
                        bowl2 = 1;
                }
                else if (bowl2) {
                        catmouse_eat("cat", catnumber, 1, it);
                        bowl1 = 1; // bowl1 now occupied
                }
                it++;
                V(bowl_access);
                V(cat_or_mouse);
                bowl1 = 0;
                bowl2 = 0;
        }


        (void) unusedpointer;
        (void) catnumber;
}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */

        // if no one is there, go ahead
        // or if there is one mouse eating, go ahead
        // if there is one cat eating, wait
        // or if there are two things eating (no matter what they are), wait

        int meal;

        // while the thread has not eaten all its meals yet
        for (meal = 0; meal < NMEALS; meal++) {
                // If a cat is eating, it won't be able to acquire the semaphore
                P(cat_or_mouse);
                // If there are 2 things eating, it won't be able to acquire the other semaphore
                P(bowl_access);
                // critical region
                if (bowl1) {
                        catmouse_eat("mouse", mousenumber, 2, it);
                        bowl2 = 1;
                }
                else if (bowl2) {
                        catmouse_eat("mouse", mousenumber, 1, it);
                        bowl1 = 1; // bowl1 now occupied
                }
                it++;
                V(bowl_access);
                V(cat_or_mouse);
                bowl1 = 0;
                bowl2 = 0;
        }

        (void) unusedpointer;
        (void) mousenumber;
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        int index, error;
        cat_or_mouse = sem_create("cat_or_mouse", 1);
        bowl_access = sem_create("bowl_access", 2);


        /*
         * Start NCATS catsem() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * wait until all other threads finish
         */

        while (thread_count() > 1)
                thread_yield();

        (void)nargs;
        (void)args;
        kprintf("catsem test done\n");

        return 0;
}

