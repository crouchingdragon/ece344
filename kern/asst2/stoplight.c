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

/////////////////////////////////////////////////////////////////////////////////
// Stop Light Queues

void
up_for_grabs(){
        north_south = random() % 1;
}

int
get_destination(int turn, int cardirection){
        if (turn == 0)
                return ((cardirection + 2) % 4);
        else if (turn == 1)
                return ((cardirection + 1) % 4);
        else if (turn == 2)
                return ((cardirection + 3) % 4);
        else return 0;
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
                                path[0] = 1;
                                path[1] = 0;
                                break;
                        case 2:
                                path[0] = 3;
                                path[1] = 1;
                                break;
                        case 3:
                                path[0] = 2;
                                path[1] = 3;
                                break;

                }
                return path;
        }

        else if (path_way == "right"){
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
        else {
                static int path[3];

                switch(cardirection) {
                        case 0:
                                path[0] = 0;
                                path[1] = 2;
                                path[2] = 3;
                                break;
                        case 1:
                                path[0] = 1;
                                path[1] = 0;
                                path[2] = 2;
                                break;
                        case 2:
                                path[0] = 3;
                                path[1] = 1;
                                path[2] = 0;
                                break;
                        case 3:
                                path[0] = 2;
                                path[1] = 3;
                                path[2] = 1;
                                break;

                }
                return path;
        }
        return NULL;
}

// static void
// checkLocksIntersections(struct lock *lock1, struct lock *lock2, struct lock *lock3, int numOfRegionsPassed, int carnumber, int cardirection, int destdirection){
       
//         if (lock1 != NULL) lock_acquire(lock1);
//         if (lock2 != NULL) lock_acquire(lock2);
//         if (lock3 != NULL) lock_acquire(lock3);
//         printRealMessage(numOfRegionsPassed, carnumber, cardirection, destdirection);
//         if (lock3 != NULL) lock_release(lock3);
//         if (lock2 != NULL) lock_release(lock2);
//         if (lock1 != NULL) lock_release(lock1);
// }

// void
// right(int cardirection){
//         // north goes west (NW)
//         if (cardirection == 0){
//                 lock_acquire()
//         }
//         // east goes north (NE)
//         // south goes east (SE)
//         // west goes south (SW)
// }

// void
// acquire_path(int *path, int size, int carnumber, int cardirection, int turn){

//         int i;
//         int destination;
//         destination = get_destination(turn, cardirection);
//         for (i = 0; i < size; i++){
//                 // if (lock_do_i_hold(intersection[*(path + i)])) {
//                 //         cv_wait(right_of_way[*(path + i)], intersection[*(path + i)]);
//                 // }
//                 lock_acquire(intersection[*(path + i)]);
//                 message(i+1, carnumber, cardirection, destination);
//         }

// }

// void
// release_path(int *path, int size, /*int carnumber,*/ int cardirection, int turn){

//         int i;
//         int index;
//         int destination;
//         destination = get_destination(turn, cardirection);
//         for (i = 0; i < size; i++){
//                 index = size - i;
//                 // message(LEAVING, carnumber, cardirection, destination);
//                 lock_release(intersection[*(path + index)]);
//                 // cv_signal(right_of_way[*(path + i)], intersection[*(path + i)]);
//         }
// 
// }


// void path (int cardirection, int destdirection, int turn) {
        
// }
 
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

        // based on car direction, which locks should I try and acquire
        // 0 = north, 1 = east, 2 = south, 3 = west

        // int *path;
        // path = getPath("straight", cardirection);
        // acquire_path(path, 2, carnumber, cardirection, 0);
        // release_path(path, 2, /*carnumber,*/ cardirection, 0);
        
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
        /*
         * if direction is north or south, acquire the intersection
         */

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
         * { "N", "E", "S", "W" }
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

void
creating(){

        north_south_light = q_create(NCARS);
        west_east_light = q_create(NCARS);

        light = lock_create("light");
        not_approaching = cv_create("not_approaching");

        // intersection[0] = lock_create("NW");
        // intersection[1] = lock_create("NE");
        // intersection[2] = lock_create("SW");
        // intersection[3] = lock_create("SE");

        // right_of_way[0] = cv_create("NW");
        // right_of_way[1] = cv_create("NE");
        // right_of_way[2] = cv_create("SW");
        // right_of_way[3] = cv_create("SE");
}

void
destroying(){

        q_destroy(north_south_light);
        q_destroy(west_east_light);

        lock_destroy(light);
        cv_destroy(not_approaching);

        // lock_destroy(intersection[0]);
        // lock_destroy(intersection[1]);
        // lock_destroy(intersection[2]);
        // lock_destroy(intersection[3]);

        // cv_destroy(right_of_way[0]);
        // cv_destroy(right_of_way[1]);
        // cv_destroy(right_of_way[2]);
        // cv_destroy(right_of_way[3]);  
}

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        creating();
    

       NWlock = lock_create("NWlock");
       NElock = lock_create("NElock");
       SWlock = lock_create("SWlock");
       SElock = lock_create("SElock");
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







