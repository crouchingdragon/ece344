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
struct semaphore *bowl_check;
struct semaphore *bowl_access;

struct type{
        int cat;
        int mouse;
        int empty;
        
};   

struct type bowl1, bowl2;
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

int bowl;
        /* 
         * a thread was created/forked from catmousesem
         * now let this thread try and access the food/semaphore
         * if it can't (2 threads already eating), sleep
         * else if a mouse thread is eating, sleep
        */

        int meal = 0;
        // while the thread has not eaten all its meals yet
        while ( meal < NMEALS) {
                // If there are 2 things eating, it won't be able to acquire the other semaphore
                P(bowl_access);
                P(bowl_check);
                // if the mouse is eatiing the cat cant eat               
                if ((bowl1.mouse == 1)||(bowl2.mouse == 1)){
                        meal--;
                        V(bowl_check);
                        V(bowl_access);
                        continue;
                }
                assert (bowl1.mouse == 0 && bowl2.mouse == 0)
                        if (bowl1.cat == 1) {
                                bowl2.cat = 1;
                                catmouse_eat("cat", catnumber, 2, meal);
                                bowl = 2;
                        }
                        else {
                                bowl1.cat = 1;
                                catmouse_eat("cat", catnumber, 1, meal);
                                bowl = 1;
                        }
                        meal++;
                        V(bowl_check);
                
                P(bowl_check);
                if(bowl == 1){
                        bowl1.empty =1;
                        bowl1.cat = 0;
                }        
                else {
                        bowl2.empty = 1;
                        bowl2.cat = 0;
                }
                V(bowl_check);
                V(bowl_access);
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
int bowl = 0;
        // if no one is there, go ahead
        // or if there is one mouse eating, go ahead
        // if there is one cat eating, wait
        // or if there are two things eating (no matter what they are), wait
 
        int meal = 0;
 
        // while the thread has not eaten all its meals yet
        while (meal < NMEALS) {
                 // If there are 2 things eating, it won't be able to acquire the other semaphore
                P(bowl_access);
                P(bowl_check);
 
                if ((bowl1.cat == 1) || (bowl2.cat == 1)){
                        meal--;
                        V(bowl_check);
                        V(bowl_access);
                        continue;
                }        
                // If a cat is eating, it won't be able to acquire the semaphore
                assert (bowl1.cat == 0 && bowl2.cat == 0)

                        if (bowl1.mouse == 1) {
                                bowl2.mouse = 1;
                                catmouse_eat("mouse", mousenumber, 2, meal);
                                bowl = 2;
                        }
                        else  {
                                bowl1.mouse =1;
                                catmouse_eat("mouse", mousenumber, 1, meal);
                                bowl = 1; // bowl1 now occupied
                        }
                        meal++;
                        V(bowl_check);
               
                P(bowl_check);
               
                if(bowl == 1){
                        bowl1.empty = 1;
                        bowl1.mouse = 0;
                }
                else{
                        bowl2.empty = 1;
                        bowl2.mouse = 0;
                }
               
                V(bowl_check);
                V(bowl_access);
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
        bowl_check = sem_create("bowl_check", 2);
        bowl_access = sem_create("bowl_access", 1);
 
 
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

        sem_destroy(bowl_access);
        sem_destroy(bowl_check);
 
        return 0;
}

