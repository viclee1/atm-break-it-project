#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int file_exists(char *filename) {
    if (!fopen(filename, "r")) {
        return 0;
    } else {
        return 1;
    }
}

int main(int argc, char *argv[]) {
    FILE *bank = NULL;
    FILE *atm = NULL;

    printf("%s\n", argv[0]);
    if(argc != 2) {
        printf("Usage: init <filename>\n");
        return 62;
    }

    int n_slash = 0;
    int idx = 0;
    for (; idx < strlen(argv[1]); idx++) {
        if (argv[1][idx] == '/') {
            n_slash += 1;
        }
    }

    // create files in current directories 
    char *fname = (char*)malloc((strlen(argv[1]) + 1) * sizeof(char));
    strncpy(fname, argv[1], strlen(argv[1]));

    char *bank_fname =  (char*)malloc((strlen(argv[1]) + 6) * sizeof(char));
    char *atm_fname = (char*)malloc((strlen(argv[1]) + 5) * sizeof(char));
    

    strncpy(bank_fname, fname, strlen(fname));
    strncpy(atm_fname, fname, strlen(fname));
    strncat(bank_fname, ".bank", 5);
    strncat(atm_fname, ".atm", 4);


    if (file_exists(bank_fname) || file_exists(atm_fname)) {
        printf("Error: one of the files already exists\n");
        return 63;
    }

    bank = fopen(bank_fname, "w");
    atm = fopen(atm_fname, "w");

    if(bank == NULL || atm == NULL) {
        printf("Error creating initialization files\n");
        return 64;
    }

    srand(time(NULL));  
    int num = (rand() % 200)+ 1000; 

    fprintf(bank,"%d", num);
    fprintf(atm, "%d", num);
    
    fclose(bank);
    fclose(atm);
    free(fname);
    free(bank_fname);
    free(atm_fname);


    printf("Successfully initialized bank state\n");
    return 0;
    
}