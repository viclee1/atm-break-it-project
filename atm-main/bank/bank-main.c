/* 
 * The main program for the Bank.
 *
 * You are free to change this as necessary.
 */

#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include "bank.h"
#include "ports.h"

static const char prompt[] = "BANK: ";

int main(int argc, char **argv)
{
    int n;
    char sendline[1000];
    char recvline[1000];
    // FILE *fp;

    // if(argc != 2)
    // {
    //     printf("Usage:  bank <filename>\n");
    //     exit(1);
    // }

    // fp = fopen(argv[1], "r");
    // if(fp == NULL)
    // {
    //     printf("Error opening bank initialization file\n");
    //     exit(64);
    // }
    // fclose(fp);

   Bank *bank = bank_create();

   printf("%s", prompt);
   fflush(stdout);

   while(1)
   {
       fd_set fds;
       FD_ZERO(&fds);
       FD_SET(0, &fds);
       FD_SET(bank->sockfd, &fds);
       select(bank->sockfd+1, &fds, NULL, NULL, NULL);

       if(FD_ISSET(0, &fds))
       {
           fgets(sendline, 10000,stdin);
           bank_process_local_command(bank, sendline, strlen(sendline)); //User input in bank is processed in local
           printf("%s", prompt);
           fflush(stdout);
       }
       else if(FD_ISSET(bank->sockfd, &fds))
       {
            n = bank_recv(bank, recvline, 10000);
            recvline[n] = '\0'; //flush garbage
            bank_process_remote_command(bank, recvline, n);
       }
   }

   return EXIT_SUCCESS;
}
