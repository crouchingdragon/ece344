/* 
 * stoplight.c
 *
 * You can use any synchronization primitives available to solve
 * the stoplight problem in this file.

 - Could have 2 keys: North_South Light, West_East Light
 - 3 condition variables: left, right, straight

 */

// 
/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>

/*
 * Number of cars created.
 */

#define NCARS 20
#define STRAIGHT 2
#define RIGHT 1
#define LEFT 3
 
#define goS 1
#define goR 2
#define goL 3
 
static struct lock *SWlock;
static struct lock *SElock;
static struct lock *NWlock;
static struct lock *NElock;
 
static void
printRealMessage(int numOfRegionsPassed, int carnumber, int cardirection, int destdirection);
static void
checkLocksIntersections(struct lock *lock1, struct lock *lock2, struct lock *lock3, int numOfRegionsPassed, int carnumber, int cardirection, int destdirection);
int
calculateDesDirection(int cardirection, int turn);
/*
 *
 * Function Definitions
 *
 */
 
static const char *directions[] = { "N", "E", "S", "W" };
 
static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};
 
/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };
 
static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */
 
static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{      
        int destdirection;
 
        destdirection = calculateDesDirection(cardirection, goS);
 
        if (cardirection == 0)
                checkLocksIntersections(NWlock, SWlock, NULL, STRAIGHT, carnumber, cardirection, destdirection);
        else if (cardirection == 1)
                checkLocksIntersections(NWlock, NElock, NULL, STRAIGHT, carnumber, cardirection, destdirection);
        else if (cardirection == 2)
                checkLocksIntersections(NElock, SElock, NULL, STRAIGHT, carnumber, cardirection, destdirection);
        else if (cardirection == 3)
                checkLocksIntersections(SWlock, SElock, NULL, STRAIGHT, carnumber, cardirection, destdirection);
 
 
 
        /*
         * Avoid unused variable warnings.
         */
       
     //   (void) cardirection;
       // (void) carnumber;
}
 
 
/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the
 *      intersection from any direction.
 *      Write and comment this function.
 */
 
static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        int destdirection;
 
        destdirection = calculateDesDirection(cardirection, goL);
 
        if (cardirection == 0)
                checkLocksIntersections(NWlock, SWlock, SElock, LEFT, carnumber, cardirection, destdirection);
        else if (cardirection == 1)
                checkLocksIntersections(NWlock, NElock, SWlock, LEFT, carnumber, cardirection, destdirection);
        else if (cardirection == 2)
                checkLocksIntersections(NWlock, NElock, SElock, LEFT, carnumber, cardirection, destdirection);
        else if (cardirection == 3)
                checkLocksIntersections(NElock, SWlock, SElock, LEFT, carnumber, cardirection, destdirection);
 
       
       
       
        /*
         * Avoid unused variable warnings.
         */
 
   //     (void) cardirection;
     //   (void) carnumber;
}
 
 
/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the
 *      intersection from any direction.
 *      Write and comment this function.
 */
 
static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
 
        int destdirection;
 
        destdirection = calculateDesDirection(cardirection, goR);
 
        if (cardirection == 0)
                checkLocksIntersections(NWlock, NULL, NULL, RIGHT, carnumber, cardirection, destdirection);
        else if (cardirection == 1)
                checkLocksIntersections(NElock, NULL, NULL, RIGHT, carnumber, cardirection, destdirection);
        else if (cardirection == 2)
                checkLocksIntersections(SElock, NULL, NULL, RIGHT, carnumber, cardirection, destdirection);
        else if (cardirection == 3)
                checkLocksIntersections(SWlock, NULL, NULL, RIGHT, carnumber, cardirection, destdirection);
 
 
        /*
         * Avoid unused variable warnings.
         */
 
       // (void) cardirection;
 //       (void) carnumber;
}
 
 
/*
 * approachintersection()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection;
        int turn;
        /*
         * Avoid unused variable and function warnings.
         */
 
        (void) unusedpointer;
        //(void) carnumber;
      //  (void) gostraight;
       // (void) turnleft;
       // (void) turnright;
 
        /*
         * cardirection is set randomly.
         */
 
        cardirection = random() % 4;
        turn = random() % 3;
 
        if (turn == 0)
                turnright(cardirection , carnumber);
       
        else if (turn == 1)
                turnleft(cardirection , carnumber);
       
        else
                gostraight(cardirection , carnumber);
}
 
static void
printRealMessage(int numOfRegionsPassed, int carnumber, int cardirection, int destdirection){
        int i;
 
        if (numOfRegionsPassed == 1) {// right turn
                for(i = 0; i <= numOfRegionsPassed; i++)
                message(i, carnumber, cardirection, destdirection);
                message(4,carnumber, cardirection, destdirection);                
        }
 
        else if (numOfRegionsPassed == 2){// straight way{
                for(i = 0; i <= numOfRegionsPassed; i++)
                message(i, carnumber, cardirection, destdirection);
                message(4,carnumber, cardirection, destdirection);
        }
       
        else{
                for(i = 0; i <= (numOfRegionsPassed + 1) ; i++)
                message(i, carnumber, cardirection, destdirection);
                //message(4,carnumber, cardirection, destdirection);
        }
}
 
static void
checkLocksIntersections(struct lock *lock1, struct lock *lock2, struct lock *lock3, int numOfRegionsPassed, int carnumber, int cardirection, int destdirection){
       
        assert(lock1);
 
        lock_acquire(lock1);
       //  assert(lock2);
        if(lock2)//lock_do_i_hold(lock2))
                lock_acquire(lock2);
                // assert(lock3);
        if(lock3)//lock_do_i_hold(lock3))
                lock_acquire(lock3);
        printRealMessage(numOfRegionsPassed, carnumber, cardirection, destdirection);
        if (lock3)
                lock_release(lock3);
        if (lock2)
                lock_release(lock2);
        lock_release(lock1);
}
 
int
calculateDesDirection(int cardirection, int turn){
 
        int desdirection;
        if (turn == 1)
                desdirection = (cardirection + 2) % 4;
        else if(turn == 2)
                desdirection = (cardirection + 3) % 4;
        else if(turn == 3)
                desdirection = (cardirection + 1) % 4;
 
        return desdirection;
}
 
 
/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        creating();

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {
                error = thread_fork("approachintersection thread",
                                    NULL, index, approachintersection, NULL);

                /*
                * panic() on error.
                */

                if (error) {         
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error));
                }
        }
        
        /*
         * wait until all other threads finish
         */

        while (thread_count() > 1)
                thread_yield();


	(void)message;
        (void)nargs;
        (void)args;
        kprintf("stoplight test done\n");

        destroying();

        return 0;
}







