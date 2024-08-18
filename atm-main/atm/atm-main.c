/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <string.h>

static const char prompt[] = "ATM: ";

//int main(int argc, char **argv[])
int main()
{
    fd_set rfds;
    struct timeval time;
    int check;

    char user_input[10000];
    ATM *atm = atm_create();
    // FILE *fp;

    // fp = fopen(argv[1], "r");
    // if(fp == NULL)
    // {
    //     printf("Error opening ATM initialization file\n");
    //     exit(64);
    // }

    printf("%s", prompt);
    fflush(stdout);

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);

        time.tv_sec = 180;
        time.tv_usec = 0;

        check = select(1, &rfds, NULL, NULL, &time);

        if (check == -1) {
            exit(0);
        } else if (check) {
            if (fgets(user_input, 10000, stdin) != NULL) {
                atm_process_command(atm, user_input);
                if (atm->username != NULL) {
                    printf("ATM (%s): ", atm->username);
                }
                else {
                    printf("%s", prompt);
                }
                // printf("%s", prompt);
                fflush(stdout);
                //printf("User Input: %sk\n", user_input);
            }
        } else {
            if(atm->username != NULL) {
                char commandend[strlen("end-session\n")];
                strncpy(commandend, "end-session\n", sizeof("end-session\n"));
                printf("Timeout - Inactivity for 3 minutes\n");
                atm_process_command(atm, commandend);
                printf("%s", prompt);
                fflush(stdout);
            }
        }
    }
    // while (fgets(user_input, 10000,stdin) != NULL)
    // {

    //     atm_process_command(atm, user_input);
    //     if (atm->username != NULL) {
    //         printf("ATM (%s): ", atm->username);
    //     }
    //     else {
    //         printf("%s", prompt);
    //     }
    //     // printf("%s", prompt);
    //     fflush(stdout);
    //     //printf("User Input: %sk\n", user_input);
    // }
    return EXIT_SUCCESS;
}
