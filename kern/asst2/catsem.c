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
// struct semaphore *cat_or_mouse;
struct semaphore *enter;
struct semaphore *exit;
struct semaphore *bowl_access;

// is the bowl being eaten from (1 means yes, 0 means no)
int bowl1 = 0;
int bowl2 = 0;

int catcount = 0;
int mousecount = 0;

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
        int meal = 0;
        int which = 0;
        // while the thread has not eaten all its meals yet
        while (meal < NMEALS) {
            while (mousecount > 0) thread_yield();
            
            P(enter);
            
            P(bowl_access);
            catcount++;

            V(enter);

            if (bowl1 == 0) {
                    bowl1 = 1;
                    which = 1;
                    catmouse_eat("cat", catnumber, which, meal);
                }
            else if (bowl2 == 0) {
                    bowl2 = 1; // bowl2 now occupied
                    which = 2;
                    catmouse_eat("cat", catnumber, which, meal);
                }
            
            meal++;

            P(exit);

            V(bowl_access);

            catcount--;
            if (which == 1) bowl1 = 0;
            else if (which == 2) bowl2 = 0;
            which = 0;
            V(exit);
            
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

       int meal = 0;
       int which = 0;
        // while the thread has not eaten all its meals yet
        while (meal < NMEALS) {
            while (catcount > 0) thread_yield();
            
            P(enter);

            P(bowl_access);
            mousecount++;

            V(enter);
            
            if (bowl1 == 0) {
                    bowl1 = 1;
                    which = 1;
                    catmouse_eat("mouse", mousenumber, which, meal);
                }
            else if (bowl2 == 0) {
                    bowl2 = 1; // bowl2 now occupied
                    which = 2;
                    catmouse_eat("mouse", mousenumber, which, meal);
                }

            meal++;

            P(exit);

            V(bowl_access);

            mousecount--;
            if (which == 1) bowl1 = 0;
            else if (which == 2) bowl2 = 0;
            which = 0;

            V(exit);

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
        // cat_or_mouse = sem_create("cat_or_mouse", 1);
        enter = sem_create("enter", 1);
        exit = sem_create("exit", 1);
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

        // destroy semaphores
        // sem_destroy(cat_or_mouse);
        sem_destroy(enter);
        sem_destroy(exit);
        sem_destroy(bowl_access);

        (void)nargs;
        (void)args;
        kprintf("catsem test done\n");

        return 0;
}

