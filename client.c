#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

// function which checks if 
int check(char* response) {
    char **error = (char**)calloc(LINELEN, sizeof(char**));
    int ok = 0, i = 0, j = 8, k = 0;
    error[0] = malloc(150);

    while(ok == 0 && i < strlen(response)) {
        // iterate the characters in the response from the server until
        // we reach the line which contains the "error" (if it contains it)
        // the error message always starts on the 8th position of the line
        // the line always starts with "{"error":" "
        if(response[i] == 'e' && response[i + 1] == 'r' && response[i + 2] == 'r'&& response[i + 3] == 'o' && response[i + 4] == 'r') {
            // copy the characters of the error 
            // it ends when we reach ";"
            while(response[i + j] != '"') {
                error[0][k] = response[i + j];
                k++;
                j++;
            }

            ok = 1;

        }

        i++;
    }

    if(strcmp(error[0], "You are not logged in!") == 0) {
        return 0;
    }


    return 1;
}

// DNS resolver
// returns the IP address from the hostname ("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com")
char* GetIPFromHostName(char *hostname)
{
	struct addrinfo hints, *results, *p;
	int rv;
    char *libraryIP = (char*)malloc(LINELEN);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname , "domain", &hints, &results)) != 0) {
        return "";
    }

	for(p = results; p != NULL; p = p->ai_next) {
        strcpy(libraryIP, inet_ntoa(((struct sockaddr_in*) p->ai_addr)->sin_addr));
	}
	
    freeaddrinfo(results);

	return libraryIP;
}

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;

    char str[100];
    char aux1[100];
    char aux2[100];
    char title[100];
    char author[100];
    char genre[100];
    char publisher[100];
    char page_count[100];
    char **registerJSON = (char**)malloc(sizeof(char*) * 5);
    char **loginJSON = (char**)malloc(sizeof(char*) * 5);
    char **addBookJSON = (char**)malloc(sizeof(char*) * 5);
    int verify = 0;
    char **cookie = (char**)calloc(LINELEN, sizeof(char**));
    char **token = (char**)calloc(LINELEN, sizeof(char**));
    int loggedOut = 1;
    int loggedIn = 0;
    int gotAccess = 0;
    int expired = 0;

    token[0] = malloc(300);
    cookie[0] = malloc(150);

    // read the first string from stdin
    gets(str);

    // continue reading until the message "exit" is reveived
    while(strcmp(str, "exit") != 0) {
        if(verify == 1) {
            gets(str);
        }

        if(verify == 0) {
            verify = 1;
        }

        // verify if we received the "register" command
        if(strcmp(str, "register") == 0) {
            // display the prompt and read the input
            printf("username=");
            gets(aux1);

            printf("password=");
            gets(aux2);
           
            // create the json with the input received
            registerJSON[0] =  malloc(1000);
            sprintf(registerJSON[0], "%s", "{");
            strcat(registerJSON[0], "\"username\": \"");
            strcat(registerJSON[0], aux1);
            strcat(registerJSON[0], "\",");
            strcat(registerJSON[0], "\"password\": \"");
            strcat(registerJSON[0], aux2);
            strcat(registerJSON[0], "\"");
            strcat(registerJSON[0], "}");

            // open connection
            sockfd = open_connection(GetIPFromHostName("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"), 8080, AF_INET, SOCK_STREAM, 0);
            // create the request
            message = compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/auth/register", 
                "application/json", registerJSON, 1, NULL, 1, NULL);
            // disply the request
            printf("REGISTER REQUEST:\n%s\n", message);
            // send to server the request
            send_to_server(sockfd, message);
            // receive the respnse from the server
            response = receive_from_server(sockfd);
            // close the connection
            close_connection(sockfd);

            // display the response from the server and free the memory
            puts(response);
            free(response);
            free(message);

        // verify if we received the "login" command
        } else if(strcmp(str, "login") == 0) {
            // check if I've never logged in or if the previous user logged out
            // a new user can't logg in if another user is already logged in
            if(loggedIn == 0 || loggedOut == 1 || expired == 1) {
                // display the prompt and read the input
                printf("username=");
                gets(aux1);

                printf("password=");
                gets(aux2);


                // create the json with the input received
                loginJSON[0] =  malloc(1000);
                sprintf(loginJSON[0], "%s", "{");
                strcat(loginJSON[0], "\"username\":\"");
                strcat(loginJSON[0], aux1);
                strcat(loginJSON[0], "\",");
                strcat(loginJSON[0], "\"password\":\"");
                strcat(loginJSON[0], aux2);
                strcat(loginJSON[0], "\"");
                strcat(loginJSON[0], "}");

                sockfd = open_connection(GetIPFromHostName("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"), 8080, AF_INET, SOCK_STREAM, 0);
                message = compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/auth/login", 
                    "application/json", loginJSON, 1, NULL, 1, NULL);
                
                printf("LOGIN REQUEST:\n%s\n", message);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                close_connection(sockfd);
                puts(response);

                // parse the cookie received
                int ok = 0, i = 0, j = 12, k = 0;
                while(ok == 0 && i < strlen(response)) {
                    // iterate the characters in the response from the server until
                    // we reach the line which contains the cookie (starts with "Set")
                    // the cookie always starts on the 12th position of the line
                    // the line always starts with "Set-Cookie: "
                    if(response[i] == 'S' && response[i + 1] == 'e' && response[i + 2] == 't') {
                        // copy the characters of the cookie 
                        // it ends when we reach ";"
                        while(response[i + j] != ';') {
                            cookie[0][k] = response[i + j];
                            k++;
                            j++;
                        }

                        ok = 1;

                        // if we found a cookie -> the user logged in successfully
                        // and the variable which tells us if the user logged out is reset
                        loggedOut = 0;
                        loggedIn = 1;
                        expired = 0;
                    }

                    i++;
                }

                free(response);
                free(message);
            } else {
                // if another user is already logged in a message is displayed
                printf("%s", "Another user already logged in!\n");
            }

        // verify if we received the "enter_library" command
        } else if(strcmp(str, "enter_library") == 0) {
            
            sockfd = open_connection(GetIPFromHostName("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"), 8080, AF_INET, SOCK_STREAM, 0);
            // sprintf(cookie[0], "%s", "connect.sid=s%3A34zbxOcPdTbTc_btsK3M7qgTm5JC_yuk.ugevL%2Fx%2BAAKYXm6cCeVfF7lr82beceWueyUOx3E3wSs");

            // create the request
            // if we haven't loged in the cookie sent will be NULL
            message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/library/access",
                            NULL, cookie, 1, NULL);
            
            printf("ACCESS REQUEST:\n%s\n", message);

            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            check(response);
            close_connection(sockfd);

            // if the user is not logged in
            // or if the cookie expired a message is displayed
            // the library can't be accessed if the user is not logged in
            // else the response from the server is displayed
            if(loggedIn == 0) {
                printf("\n");
                printf("You are not logged in!\n");
                printf("\n");
            } else if(strlen(cookie[0]) != 0 && check(response) == 0) {
                printf("\n");
                printf("The cookie expired! Log in again!\n");
                printf("\n");
                expired = 1;
            } else {
                puts(response);
            }


            // parse the JWT token received
            int ok = 0, i = 0, j = 10, k = 0;
            // iterate the characters in the response from the server until
            // we reach the line which contains the cookie (starts with "{")
            // the token always starts on the 10th position of the line
            // the line always starts with "{"token":""
            while(ok == 0 && i < strlen(response)) {
                if(response[i] == '{') {
                    // copy the characters of the token 
                    // it ends when we reach " " "
                    while(response[i + j] != '"') {
                        token[0][k] = response[i + j];
                        k++;
                        j++;
                    }

                    ok = 1;
                    // if we find a token it means the user got access to the library successfully 
                    gotAccess = 1;
                }

                i++;
            }

            
            free(response);
            free(message);

        // verify if we received the "get_books" command
        } else if(strcmp(str, "get_books") == 0) {
            sockfd = open_connection(GetIPFromHostName("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"), 8080, AF_INET, SOCK_STREAM, 0);
            // create the request
            // if we don't have access to the library
            // the token sent will be NULL
            message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/library/books",
                            NULL, NULL, 0, token);

            printf("GET BOOKS REQUEST:\n%s\n", message);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);

            // if the user doesn't have access to the library 
            // or if the cookie expired the message is displayed
            // otherwise the server response is displayed
            if(gotAccess == 0) {
                printf("\n");
                printf("You don't have access to the library!\n");
                printf("\n");
            } else if(strlen(cookie[0]) != 0 && check(response) == 0) {
                printf("\n");
                printf("The cookie expired! Log in again!\n");
                printf("\n");
                expired = 1;
            } else {
                puts(response);
            }
            free(response);
            free(message);

        // verify if we received the "add_book" command
        } else if(strcmp(str, "add_book") == 0) {
            // display the prompt and read the input
            printf("title=");
            gets(title);

            printf("author=");
            gets(author);

            printf("genre=");
            gets(genre);

            printf("publisher=");
            gets(publisher);

            printf("page_count=");
            gets(page_count);

            int ok = 1;

            // check if the page_count contains any non-numeric characters
            for(int i = 0; i < strlen(page_count); i++) {
                if(page_count[i] < 48 || page_count[i] > 57) {
                    ok = 0;
                    break;
                }
            }

            // if the page_count is a negative number the request is not made 
            // because the book would still be added and it would be wrong
            if(page_count[0] == '-') {
                printf("\n");
                printf("%s", "Page_count can't be a negative number!\n");
                printf("\n");
            } else {
                // create the json with the input received
                addBookJSON[0] =  malloc(1000);
                sprintf(addBookJSON[0], "%s", "{");
                strcat(addBookJSON[0], "\"title\": \"");
                strcat(addBookJSON[0], title);
                strcat(addBookJSON[0], "\",");
                strcat(addBookJSON[0], "\"author\": \"");
                strcat(addBookJSON[0], author);
                strcat(addBookJSON[0], "\",");
                strcat(addBookJSON[0], "\"genre\": \"");
                strcat(addBookJSON[0], genre);
                strcat(addBookJSON[0], "\",");
                strcat(addBookJSON[0], "\"page_count\": ");
                strcat(addBookJSON[0], page_count);
                strcat(addBookJSON[0], ",");
                strcat(addBookJSON[0], "\"publisher\": \"");
                strcat(addBookJSON[0], publisher);
                strcat(addBookJSON[0], "\"");
                strcat(addBookJSON[0], "}");

                sockfd = open_connection(GetIPFromHostName("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"), 8080, AF_INET, SOCK_STREAM, 0);
                // create the request
                // if we don't have access to the library
                // the token sent will be NULL
                message = compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/library/books", 
                    "application/json", addBookJSON, 1, NULL, 1, token);
                    
                printf("ADD BOOK REQUEST:\n%s\n", message);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                close_connection(sockfd);

                // check if the page_count is not valid (is not a number)
                // or if the user doesn't have access to the library
                // or if the cookie expired
                // and display the suitable message
                // otherwise the server response is displayed
                if(ok == 0) {
                    printf("\n");
                    printf("%s", "Invalid page_count!\n");
                    printf("\n");
                } else if(loggedIn == 0 || gotAccess == 0) {
                    printf("\n");
                    printf("You don't have access to the library!\n");
                    printf("\n");
                } else if(strlen(cookie[0]) != 0 && check(response) == 0) {
                    printf("\n");
                    printf("The cookie expired! Log in again!\n");
                    printf("\n");
                    expired = 1;
                } else {
                    puts(response);
                }

                free(response);
                free(message);

            } 

        // verify if we received the "logout" command
        } else if(strcmp(str, "logout") == 0) {
            loggedOut = 1;
            sockfd = open_connection(GetIPFromHostName("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"), 8080, AF_INET, SOCK_STREAM, 0);

            // create the request
            // if we haven't loged in the cookie sent will be NULL
            message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/auth/logout",
                        NULL, cookie, 1, NULL);

            printf("LOGOUT REQUEST:\n%s\n", message);

            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            
            // check if the user didn't log in or if the cookie expired
            // otherwise the server response is displayed
            if(loggedIn == 0) {
                printf("\n");
                printf("You are not logged in!\n");
                printf("\n");
            } else if(strlen(cookie[0]) != 0 && check(response) == 0) {
                printf("\n");
                printf("The cookie expired! Log in again!\n");
                printf("\n");
                expired = 1;
            } else {
                puts(response);
            }

            free(response);
            free(message);

        // verify if we received the "get_book" command
        } else if(strcmp(str, "get_book") == 0) {
            // display the prompt and read the input
            printf("id=");
            gets(aux1);

            int ok = 1;

            // check if the id contains any non-numeric characters
            for(int i = 0; i < strlen(aux1); i++) {
                if(aux1[i] < 48 || aux1[i] > 57) {
                    ok = 0;
                    break;
                }
            }

            // parse the path to the wanted book using the input read
            char *line = calloc(LINELEN, sizeof(char));
            memset(line, 0, LINELEN);
            sprintf(line, "/api/v1/tema/library/books/%s", aux1);

            sockfd = open_connection(GetIPFromHostName("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"), 8080, AF_INET, SOCK_STREAM, 0);
            // create the request
            // if we don't have access to the library
            // the token sent will be NULL
            message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", line,
                            NULL, NULL, 0, token);

            printf("GET BOOK REQUEST:\n%s\n", message);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);

            // check if the user entered an invalid id
            // or if the user doesn't have access to the library
            // or if the cookie expired
            // otherwise the server response is displayed
            if(gotAccess == 0) {
                printf("\n");
                printf("You don't have access to the library!\n");
                printf("\n");
            } else if(ok == 0){
                printf("\n");
                printf("Invalid Id!\n");
                printf("\n");
            } else if(strlen(cookie[0]) != 0 && check(response) == 0) {
                printf("\n");
                printf("The cookie expired! Log in again!\n");
                printf("\n");
                expired = 1;
            } else {
                puts(response);
            }

            free(response);
            free(message);

        // verify if we received the "delete_book" command
        } else if(strcmp(str, "delete_book") == 0) {
            printf("id=");
            gets(aux1);

            int ok = 1;
            // check if the id contains any non-numeric characters
            for(int i = 0; i < strlen(aux1); i++) {
                if(aux1[i] < 48 || aux1[i] > 57) {
                    ok = 0;
                    break;
                }
            }

            // parse the path to the wanted book using the input read
            char *line = calloc(LINELEN, sizeof(char));
            memset(line, 0, LINELEN);
            sprintf(line, "/api/v1/tema/library/books/%s", aux1);

            sockfd = open_connection(GetIPFromHostName("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"), 8080, AF_INET, SOCK_STREAM, 0);
            // create the request
            // if we don't have access to the library
            // the token sent will be NULL
            message = compute_delete_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", line,
                            "application/json", NULL, 0, token);

            printf("DELETE BOOK REQUEST:\n%s\n", message);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);

            // check if the user entered an invalid id
            // or if the user doesn't have access to the library
            // or if the cookie expired
            // otherwise the server response is displayed
            if(gotAccess == 0) {
                printf("\n");
                printf("You don't have access to the library!\n");
                printf("\n");
            } else if(ok == 0){
                printf("\n");
                printf("Invalid Id!\n");
                printf("\n");
            } else if(strlen(cookie[0]) != 0 && check(response) == 0) {
                printf("\n");
                printf("The cookie expired! Log in again!\n");
                printf("\n");
                expired = 1;
            } else {
                puts(response);
            }


            free(response);
            free(message);
        } else {
            if(strcmp(str, "exit") != 0) {
                // the following message is displayed if something else
                // than the mentioned commands is entered
                printf("%s", "Invalid command!\n");
            }
        }
    


    }

    return 0;
}
