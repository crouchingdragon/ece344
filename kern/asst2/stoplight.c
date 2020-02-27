/* 
 * stoplight.c
 *
 * You can use any synchronization primitives available to solve
 * the stoplight problem in this file.

 - Could have 2 keys: North_South Light, West_East Light
 - 3 condition variables: left, right, straight

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
 
// static struct lock *SWlock;
// static struct lock *SElock;
// static struct lock *NWlock;
// static struct lock *NElock;

struct lock *intersection[4];
struct lock *f;
// struct lock *path[3];
 
// static void
// printRealMessage(int numOfRegionsPassed, int carnumber, int cardirection, int destdirection);
// static void
// checkLocksIntersections(struct lock *lock1, struct lock *lock2, struct lock *lock3, int numOfRegionsPassed, int carnumber, int cardirection, int destdirection);
int
calculateDesDirection(int cardirection, int turn);
int
*getPath(const char *path_way, unsigned long cardirection);
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
        lock_acquire(intersection[*(getPath("straight", cardirection))]);
        lock_acquire(intersection[*(getPath("straight", cardirection) + 1)]);
        int i;
        int destdirection = calculateDesDirection(cardirection, goS);
        for(i = 0; i <= STRAIGHT; i++)
                message(i, carnumber, cardirection, destdirection);
                message(4,carnumber, cardirection, destdirection);
        lock_release(intersection[*(getPath("straight", cardirection) + 1)]);
        lock_release(intersection[*(getPath("straight", cardirection))]);
        // lock_release(intersection[*(getPath("straight", cardirection))]);
        // lock_release(intersection[*(getPath("straight", cardirection) + 1)]);

 
 
 
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
        lock_acquire(intersection[*(getPath("left", cardirection))]);
        lock_acquire(intersection[*(getPath("left", cardirection) + 1)]);
        lock_acquire(intersection[*(getPath("left", cardirection) + 2)]);
        int destdirection = calculateDesDirection(cardirection, goL);
        int i;
        for(i = 0; i <= (LEFT + 1) ; i++)
                message(i, carnumber, cardirection, destdirection);
        
        lock_release(intersection[*(getPath("left", cardirection) + 2)]);
        lock_release(intersection[*(getPath("left", cardirection) + 1)]);
        lock_release(intersection[*(getPath("left", cardirection))]);
        

       
       
       
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

        lock_acquire(intersection[*(getPath("right", cardirection))]);
        int destdirection = calculateDesDirection(cardirection, goR);
        int i;
        for(i = 0; i <= RIGHT; i++)
                message(i, carnumber, cardirection, destdirection);
                message(4,carnumber, cardirection, destdirection);
        lock_release(intersection[*(getPath("right", cardirection))]);

 
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
        // (void) gostraight;
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
        else if (turn == 3)
                gostraight(cardirection , carnumber);
}

int
*getPath(const char *path_way, unsigned long cardirection){
       
        if (path_way == "straight"){
                static int path[2];
 
                switch(cardirection) {
                        case 0:
                                path[0] = 0;
                                path[1] = 2;
                                break;
                        case 1:
                                path[0] = 0;
                                path[1] = 1;
                                break;
                        case 2:
                                path[0] = 1;
                                path[1] = 3;
                                break;
                        case 3:
                                path[0] = 2;
                                path[1] = 3;
                                break;
 
                }
                return path;
        }
 
        if (path_way == "right"){
                static int path[1];
                switch(cardirection) {
                        case 0:
                                path[0] = 0;
                                break;
                        case 1:
                                path[0] = 1;
                                break;
                        case 2:
                                path[0] = 3;
                                break;
                        case 3:
                                path[0] = 2;
                                break;
 
                }
                return path;
        }
        if (path_way == "left") {
                static int path[3];
 
                switch(cardirection) {
                        case 0:
                                path[0] = 0;
                                path[1] = 2;
                                path[2] = 3;
                                break;
                        case 1:
                                path[0] = 0;
                                path[1] = 1;
                                path[2] = 2;
                                break;
                        case 2:
                                path[0] = 0;
                                path[1] = 1;
                                path[2] = 3;
                                break;
                        case 3:
                                path[0] = 1;
                                path[1] = 2;
                                path[2] = 3;
                                break;
 
                }
                return path;
        }
        return NULL;
}
 
// static void
// checkLocksIntersections(struct lock *lock1, struct lock *lock2, struct lock *lock3, int numOfRegionsPassed, int carnumber, int cardirection, int destdirection){
       
//         lock_acquire(lock1);
//         if(lock2)
//                 lock_acquire(lock2);
//         if(lock3)
//                 lock_acquire(lock3);
//         printRealMessage(numOfRegionsPassed, carnumber, cardirection, destdirection);
//         if (lock3)
//                 lock_release(lock3);
//         if (lock2)
//                 lock_release(lock2);
//         lock_release(lock1);
// }
 
int
calculateDesDirection(int cardirection, int turn){
 
        if (turn == 1)
                return ((cardirection + 2) % 4);
        if (turn == 2)
                return ((cardirection + 3) % 4);
        if (turn == 3)
                return ((cardirection + 1) % 4);
        return 0;
}

static
void
creating(){
        // NWlock = lock_create("NWlock");
        // NElock = lock_create("NElock");
        // SWlock = lock_create("SWlock");
        // SElock = lock_create("SElock");

        intersection[0] = lock_create("NWlock");
        intersection[1] = lock_create("NElock");
        intersection[2] = lock_create("SWlock");
        intersection[3] = lock_create("SElock");

        // f = lock_create("fuck");

        // path[0] = lock_create("straight");
        // path[1] = lock_create("left");
        // path[2] = lock_create("right");
}

static
void
destroying(){
        // lock_destroy(NWlock);
        // lock_destroy(NElock);
        // lock_destroy(SWlock);
        // lock_destroy(SElock);

        lock_destroy(intersection[0]);
        lock_destroy(intersection[1]);
        lock_destroy(intersection[2]);
        lock_destroy(intersection[3]);
        
        // lock_destroy(f);

        // lock_destroy(path[0]);
        // lock_destroy(path[1]);
        // lock_destroy(path[2]);
}

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







