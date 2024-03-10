/**
 *  \file semSharedReceptionist.c (implementation file)
 *
 *  \brief Problem name: Restaurant
 *
 *  Synchronization based on semaphores and shared memory.
 *  Implementation with SVIPC.
 *
 *  Definition of the operations carried out by the receptionist:
 *     \li waitForGroup
 *     \li provideTableOrWaitingRoom
 *     \li receivePayment
 *
 *  \author Nuno Lau - December 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "probConst.h"
#include "probDataStruct.h"
#include "logging.h"
#include "sharedDataSync.h"
#include "semaphore.h"
#include "sharedMemory.h"

/** \brief logging file name */
static char nFic[51];

/** \brief shared memory block access identifier */
static int shmid;

/** \brief semaphore set access identifier */
static int semgid;

/** \brief pointer to shared memory region */
static SHARED_DATA *sh;

/* constants for groupRecord */
#define TOARRIVE 0
#define WAIT     1
#define ATTABLE  2
#define DONE     3

/** \brief receptioninst view on each group evolution (useful to decide table binding) */
static int groupRecord[MAXGROUPS];


/** \brief receptionist waits for next request */
static request waitForGroup ();

/** \brief receptionist waits for next request */
static void provideTableOrWaitingRoom (int n);

/** \brief receptionist receives payment */
static void receivePayment (int n);



/**
 *  \brief Main program.
 *
 *  Its role is to generate the life cycle of one of intervening entities in the problem: the receptionist.
 */
int main (int argc, char *argv[])
{
    int key;                                            /*access key to shared memory and semaphore set */
    char *tinp;                                                       /* numerical parameters test flag */

    /* validation of command line parameters */
    if (argc != 4) { 
        freopen ("error_RT", "a", stderr);
        fprintf (stderr, "Number of parameters is incorrect!\n");
        return EXIT_FAILURE;
    }
    else { 
        freopen (argv[3], "w", stderr);
        setbuf(stderr,NULL);
    }

    strcpy (nFic, argv[1]);
    key = (unsigned int) strtol (argv[2], &tinp, 0);
    if (*tinp != '\0') {   
        fprintf (stderr, "Error on the access key communication!\n");
        return EXIT_FAILURE;
    }

    /* connection to the semaphore set and the shared memory region and mapping the shared region onto the
       process address space */
    if ((semgid = semConnect (key)) == -1) { 
        perror ("error on connecting to the semaphore set");
        return EXIT_FAILURE;
    }
    if ((shmid = shmemConnect (key)) == -1) { 
        perror ("error on connecting to the shared memory region");
        return EXIT_FAILURE;
    }
    if (shmemAttach (shmid, (void **) &sh) == -1) { 
        perror ("error on mapping the shared region on the process address space");
        return EXIT_FAILURE;
    }

    /* initialize random generator */
    srandom ((unsigned int) getpid ());              

    /* initialize internal receptionist memory */
    int g;
    for (g=0; g < sh->fSt.nGroups; g++) {
       groupRecord[g] = TOARRIVE;
    }

    /* simulation of the life cycle of the receptionist */
    int nReq=0;
    request req;
    while( nReq < sh->fSt.nGroups*2 ) {
        req = waitForGroup();
        switch(req.reqType) {
            case TABLEREQ:
                   provideTableOrWaitingRoom(req.reqGroup); //TODO param should be groupid
                   break;
            case BILLREQ:
                   receivePayment(req.reqGroup);
                   break;
        }
        nReq++;
    }

    /* unmapping the shared region off the process address space */
    if (shmemDettach (sh) == -1) {
        perror ("error on unmapping the shared region off the process address space");
        return EXIT_FAILURE;;
    }

    return EXIT_SUCCESS;
}

/**
 *  \brief decides table to occupy for group n or if it must wait.
 *
 *  Checks current state of tables and groups in order to decide table or wait.
 *
 *  \return table id or -1 (in case of wait decision)
 */
static int decideTableOrWait(int n) // Associates a table to a group, if it's able to
{
     //TODO insert your code here

    int table_id = 0;
    int table_occupied = 0;
   
    // This is for the assignments of the tables
    for (int i = 0; i < MAXGROUPS; i++) { 
        // Checking for free tables
         
         if(sh->fSt.assignedTable[i] == 0) { // If the table 0 is occupied
           
            if (table_occupied == 0) { // If we have not found an occupied table yet
            
                table_id = 1; // Assign table 1
                table_occupied++;
            }
            else
                // Wait decision
                table_id = -1;
        } 
         else if (sh->fSt.assignedTable[i] == 1) { // If the table 1 is occupied
            
             if (table_occupied == 0) { // If we have not found an occupied table yet
                table_id = 0; // Assign table 0
                table_occupied++; 
            } else
                // Wait decision
                table_id = -1;
          }
     } 

     // Return table id and if not available -1
     // Store the groups to later assign the table
     if (table_id != -1) {
         groupRecord[n] = ATTABLE;
         return table_id;
     } else {
         groupRecord[n] = WAIT;
         sh->fSt.groupsWaiting++; 
         return -1;
    }

    //end of TODO
}

/**
 *  \brief called when a table gets vacant and there are waiting groups 
 *         to decide which group (if any) should occupy it.
 *
 *  Checks current state of tables and groups in order to decide group.
 *
 *  \return group id or -1 (in case of wait decision)
 */
static int decideNextGroup() // Decides which group is next to occupy a table
{
     //TODO insert your code here

     if (sh->fSt.groupsWaiting != 0)
     {
        for (int i = 0; i < MAXGROUPS; i++)
        {
             if (groupRecord[i] == WAIT)
             {   
                 // Change group to the table 
                 groupRecord[i] = ATTABLE;
                 // Remove from waiting list
                 sh->fSt.groupsWaiting--; 

                 return i;
            }
        }
        return -1;
     }
     //end of TODO
     return -1;
}

/**
 *  \brief receptionist waits for next request 
 *
 *  Receptionist updates state and waits for request from group, then reads request,
 *  and signals availability for new request.
 *  The internal state should be saved.
 *
 *  \return request submitted by group
 */
static request waitForGroup() // Receptionist waits for request from group
{
    request req; 

    if (semDown (semgid, sh->mutex) == -1)  {                            /* enter critical region */
        perror ("error on the up operation for semaphore access (WT)");
        exit (EXIT_FAILURE);
    }

    // TODO insert your code here
    
    // Receptionist waits for request from group
    sh->fSt.st.receptionistStat = WAIT_FOR_REQUEST;
    saveState(nFic, &sh->fSt);

    //end of TODO
    
    if (semUp (semgid, sh->mutex) == -1)      {                           /* exit critical region */
          perror ("error on the down operation for semaphore access (WT)");
        exit (EXIT_FAILURE);
    }

    // TODO insert your code here

    // Receptionist waits for request from group
    if (semDown (semgid, sh->receptionistReq) == -1)                      /* enter critical region*/
    {
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    //end of TODO

    if (semDown (semgid, sh->mutex) == -1)  {                            /* enter critical region */
        perror ("error on the up operation for semaphore access (WT)");
        exit (EXIT_FAILURE);
    }

    // TODO insert your code here
    // Receptionist reads request
    req = sh->fSt.receptionistRequest; 

    if (semUp (semgid, sh->mutex) == -1) {                               /* exit critical region */
     perror ("error on the down operation for semaphore access (WT)");
        exit (EXIT_FAILURE);
    }

    // TODO insert your code here
    // Receptionist signals that new requests are possible
    if (semUp (semgid, sh->receptionistRequestPossible) == -1)           /* exit critical region */
    {
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    return req;

}

/**
 *  \brief receptionist decides if group should occupy table or wait
 *
 *  Receptionist updates state and then decides if group occupies table
 *  or waits. Shared (and internal) memory may need to be updated.
 *  If group occupies table, it must be informed that it may proceed. 
 *  The internal state should be saved.
 *
 */
static void provideTableOrWaitingRoom (int n) // Receptionist decides if group should occupy table or wait
{
    if (semDown (semgid, sh->mutex) == -1)  {                           /* enter critical region */
        perror ("error on the up operation for semaphore access (WT)");
        exit (EXIT_FAILURE);
    }

    // TODO insert your code here

    // Change receptionist state so it can assign tables
    sh->fSt.st.receptionistStat = ASSIGNTABLE;
    saveState(nFic, &sh->fSt);

    // Check all tables and checks if it's either assigned or put on the waiting list
    // If there's an available table, it lets the group know
    int table_id = decideTableOrWait(n);
    if (table_id != -1)
    {
        sh->fSt.assignedTable[n] = table_id;

        if (semUp (semgid, sh->waitForTable[n]) == -1)                  /* exit critical region */
        {
            perror ("error on the up operation for semaphore access (PT)");
            exit (EXIT_FAILURE);
        }
    }

    //end of TODO

    if (semUp (semgid, sh->mutex) == -1) {                               /* exit critical region */
        perror ("error on the down operation for semaphore access (WT)");
        exit (EXIT_FAILURE);
    }

}

/**
 *  \brief receptionist receives payment 
 *
 *  Receptionist updates its state and receives payment.
 *  If there are waiting groups, receptionist should check if table that just became
 *  vacant should be occupied. Shared (and internal) memory should be updated.
 *  The internal state should be saved.
 *
 */

static void receivePayment (int n) // Receptionist receives the payment, checks if there are any groups waiting 
 {                                 // and if so places them in a now free table

    if (semDown (semgid, sh->mutex) == -1)  {                          /* enter critical region */
        perror ("error on the up operation for semaphore access (WT)");
        exit (EXIT_FAILURE);
    }

    // TODO insert your code here
    // Receptionist changes state to receive the payment 
    sh->fSt.st.receptionistStat = RECVPAY;
    saveState(nFic, &sh->fSt);

    // Check if the table is free
    int table_id = sh->fSt.assignedTable[n];
    int group_id = decideNextGroup();

    if (group_id != -1)
    {
        // Next group gets the table that's available
         sh->fSt.assignedTable[group_id] = table_id;
         groupRecord[n] = DONE;
    
        if (semUp (semgid, sh->waitForTable[group_id]) == -1)
        {
            perror ("error on the up operation for semaphore access (PT)");
            exit (EXIT_FAILURE);
        }
    }
    
    sh->fSt.assignedTable[n] = -1;
    //end of TODO

    if (semUp (semgid, sh->mutex) == -1)  {                            /* exit critical region */
     perror ("error on the down operation for semaphore access (WT)");
        exit (EXIT_FAILURE);
    }

    // TODO insert your code here
    // Let the groups know that they are waiting for payment 
    if (semUp (semgid, sh->tableDone[table_id]) == -1)                 /* exit critical region */
    {
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }
    
    //end of TODO
}

