#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <regex.h>
#include <limits.h> 
#include "../util/list.c"


// #include "atm.h"
// #include "ports.h"
// #include <stdbool.h>
// #include <string.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <regex.h>
// #include "../util/list.c"

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>


ATM* atm_create()
{
    ATM *atm = (ATM*) malloc(sizeof(ATM));
    if(atm == NULL)
    {
        perror("Could not allocate ATM");
        exit(1);
    }

    // Set up the network state
    atm->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&atm->rtr_addr,sizeof(atm->rtr_addr));
    atm->rtr_addr.sin_family = AF_INET;
    atm->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&atm->atm_addr, sizeof(atm->atm_addr));
    atm->atm_addr.sin_family = AF_INET;
    atm->atm_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->atm_addr.sin_port = htons(ATM_PORT);
    bind(atm->sockfd,(struct sockaddr *)&atm->atm_addr,sizeof(atm->atm_addr));

    // Set up the protocol state
    // TODO set up more, as needed

    atm->username = NULL;
    atm->max_tries = 5;
    atm->pin_tries = list_create();


    return atm;
}

void atm_free(ATM *atm)
{
    if(atm != NULL)
    {
        close(atm->sockfd);
        free(atm);
    }
}

ssize_t atm_send(ATM *atm, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, data, data_len, 0,
                  (struct sockaddr*) &atm->rtr_addr, sizeof(atm->rtr_addr));
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);
}

void atm_process_command(ATM *atm, char *command)
{
    // TODO: Implement the ATM's side of the ATM-bank protocol

	/*
	 * The following is a toy example that simply sends the
	 * user's command to the bank, receives a message from the
	 * bank, and then prints it to stdout.
	 */

	/*
    char recvline[10000];
    int n;

    atm_send(atm, command, strlen(command));
    n = atm_recv(atm,recvline,10000);
    recvline[n]=0;
    fputs(recvline,stdout);
	*/

    command[strlen(command)-1] = '\0';

    char *tok1, *tok2, *tok3;
    tok1 = strtok(command, " ");

    // continue if only "Enter" is pressed 
    if (tok1 == NULL) {
        return;
    }

    if (strcmp(tok1, "begin-session") == 0)  {
        tok2 = strtok(NULL, " ");
        tok3 = strtok(NULL, " ");
        
        if (tok2 != NULL && tok3 == NULL) {
            
            if (username_check(tok2) == 1) {
                // command valid
                // TODO: start session function
                begin_session(atm, tok2);

            } else {
                printf("Usage: begin-session <user-name>\n");
                return;
            }
        } else{ 
            printf("Usage: begin-session <user-name>\n");
            return;
        }
    } else if (strcmp(tok1, "withdraw") == 0) {
        tok2 = strtok(NULL, " ");
        tok3 = strtok(NULL, " ");

        //printf("tok2: %s\n", tok2);
        if (tok2 != NULL && tok3 == NULL) {
            if (amount_check(tok2) == 1) {
                // command valid
                // TODO: withdraw function 
                //printf("In Withdraw\n");
                withdraw(atm, tok2);

            } else{
                printf("Usage: withdraw <amt>\n");
                return;
            }

        } else {
            printf("Usage: withdraw <amt>\n");
            return;
        }

    } else if (strcmp(tok1, "balance") == 0) {
        tok2 = strtok(NULL, " ");
        if (tok2 != NULL) {
            printf("Usage: balance\n");
            return;
        } 
        // command valid
        // TODO: balance function
        balance(atm);

    } else if (strcmp(tok1, "end-session") == 0) {
        tok2 = strtok(NULL, " ");
        if (tok2 != NULL) {
                printf("Invalid command\n");
            return;
        } 
        // command valid
        // TODO: end session function
        end_session(atm);

    } else {
        printf("Invalid command\n");
    }
    return;


}

int username_check(char *username) {
    int reg, valid;
    regex_t regex;

    if (username == NULL) {
        return 0;
    } 
    int len = strlen(username);
    if (len > 250) {
        return 0;
    }
    
    // checks if chars are valid
    reg = regcomp(&regex, "^[a-zA-Z]+$", REG_EXTENDED);
    if (reg == 0) {
        valid = regexec(&regex, username, 0, NULL, 0);
        regfree(&regex);
        if (valid == 0) {
            // valid username
            return 1;
        }
    }
    return 0;
}

int amount_check(char *amt) {
    int reg, valid;
    regex_t regex;
    if (amt == NULL) {
        //printf("line208\n");
        return 0;
    }

    reg = regcomp(&regex, "^[0-9]+$", REG_EXTENDED);
    if (reg == 0) {
        // regex compiles success
        valid = regexec(&regex, amt, 0, NULL, 0);
        regfree(&regex);
        if (valid == 0) {
            // pattern matched regex success
            long iamt = strtol(amt, NULL, 10);
            // check if the number is <= the max num that can be represented in int
            if (iamt > 0 && iamt <= INT_MAX) {
                return 1;
            }
        }
        //printf("line226\n");
    }
    return 0;
}

int pin_check(char *pin) {
    int reg, valid;
    regex_t regex;
    if (pin == NULL) {
        return 0;
    }

    reg = regcomp(&regex, "^[0-9]{4}$", REG_EXTENDED);
    if (reg == 0) {
        // regex compiles success
        valid = regexec(&regex, pin, 0, NULL, 0);
        regfree(&regex);
        if (valid == 0) {
            return 1;
        }
    }
    return 0;
}

void begin_session(ATM *atm, char *username){

    unsigned char key[32] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                              0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                              0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
                              0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31
                            };
    /* A 128 bit IV */
    unsigned char iv[16] = {  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                              0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35
                           };

    if (atm->username != NULL) {
        printf("A user is already logged in\n");
        return;
    }

    
    // if (atm->pin_tries >= 3) {
    //     printf("Account Locked: Too Many Failed Attempts\n");
    //     return;
    // }

    int curr_tries = list_find(atm->pin_tries, username);

    // user not found then we add with 0 pin attempt so far
    if (curr_tries == -1){ 
        list_add(atm->pin_tries, username, 0);
        curr_tries = 0;
    } else {
        if (curr_tries >= atm->max_tries) {
            printf("Account Locked: Too Many Failed Attempts\n");
            return;
        }
    }

    char response[10000];
    int idx;
    char cmd[7 + strlen(username) + 1];
    //printf("%d\n", sizeof("search "));
    strncpy(cmd, "search ", sizeof("search "));
    strncat(cmd, username, sizeof(username));
    //printf("cmd: %s\n", cmd);
    //printf("username: %s\n", username);
    atm_send(atm, cmd, strlen(cmd));
    idx = atm_recv(atm, response, 10000);
    response[idx] = '\0';
    //printf("response: %s\n", response);
    if (strcmp(response, "DNE") == 0) {
        printf("No such user\n");
        return;
    }

    char filename[ strlen(username) + 6];
    strncpy(filename, username, strlen(username));
    strncat(filename, ".card", strlen(".card"));

    // WORKING PIN RETRIEVAL THROUGH .CARD
    FILE *card = fopen(filename, "r");

    if (card == NULL) {
        printf("Unable to access %s's card\n", username);
        return;
    }
    char cardcontent[5];
    fgets(cardcontent, sizeof(cardcontent), card);
    

    unsigned char decryptpin[128];   
    int decryptlen;
    decryptlen = decrypt(response, idx, key, iv, decryptpin);
    decryptpin[decryptlen] = '\0';

    if (strcmp(cardcontent, decryptpin) != 0) {
        printf("TAMPERED CARD FILE (CONTENT DONT MATCH) !!!!\n");
        return;
    }

    printf("PIN? ");

    char pin_entry[500];
    fgets(pin_entry, 500, stdin);
    int count = 0;

    for (int i =0; i <= 4; i++) {
        if (isdigit(pin_entry[i]) != 0) {
            count +=1;
        }
        if (i == 4) {
            pin_entry[4] = '\0';
            break;
        }
    }
    
    if (count != 4) {
        list_change(atm->pin_tries, username, curr_tries + 1);
        //atm->pin_tries += 1;
        printf("Not authorized\n");
    } else{

        if (pin_check(pin_entry) == 0 || strcmp(decryptpin, pin_entry) != 0) {
            list_change(atm->pin_tries, username, curr_tries + 1);
            //atm->pin_tries += 1;
            printf("Not authorized\n");
        } else {
            atm->username = (char*)malloc((strlen(username) + 1) * sizeof(char));
            strncpy(atm->username, username, sizeof(username));
            list_change(atm->pin_tries, username, 0);
            //atm->pin_tries = 0;
            printf("Authorized\n");
            return;
        }
    }
    fflush(stdout);
    return;
}


void withdraw(ATM *atm, char *amt) {
    if (atm->username == NULL) {
        printf("No user logged in\n");
        return;    
    }
    char response[5000];
    int idx;
    char cmd[5000];
    strncpy(cmd, "withdraw ", sizeof("withdraw "));
    strncat(cmd, atm->username, sizeof(atm->username));
    strncat(cmd, " ", sizeof(" "));
    strncat(cmd, amt, sizeof(amt));
    //printf("cmd %s, amt %s\n", cmd, amt);
    atm_send(atm, cmd, strlen(cmd));
    //printf("line324\n");
    //printf("cmd %s, amt %s\n", cmd, amt);
    idx = atm_recv(atm, response, 5000);
    //printf("line326\n");
    response[idx] = '\0';
    //printf("cmd %s, amt %s\n", cmd, amt);

    if (strcmp(response, "richboy") == 0) {
        printf("$%s dispensed\n", amt);
    } else if (strcmp(response, "brokeboy") == 0) {
        printf("Insufficient funds\n");
    } else {

    }
    return;
}

void balance(ATM *atm) {
    if(atm->username == NULL){
        printf("No user logged in\n");
        return;
    }

    char response[5000];
    int idx;
    char cmd[5000];
    strncpy(cmd, "balance ", sizeof("balance "));
    strncat(cmd, atm->username, sizeof(atm->username));
    //printf("cmd %s\n", cmd);
    atm_send(atm, cmd, strlen(cmd));
    idx = atm_recv(atm, response, 5000);
    response[idx] = '\0';

    printf("$%s\n", response);
    return;
}

void end_session(ATM *atm) {
    if(atm->username == NULL){
        printf("No user logged in\n");
        return;
    }
    atm->username = NULL;
    free(atm->username);
    printf("User logged out\n");

    return;
}

// CODE FROM OPENSSL
void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;


    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}