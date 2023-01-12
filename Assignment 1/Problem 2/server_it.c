#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFF_SIZE 100

// function to check if a character is an operator
int isOperator(char a){
    if(a == '+' || a == '-' || a == '/' || a == '*') return 1;
    else return 0;
}

// function to swap the values of two double variables
void swap(double *a, double *b){
    double temp = *a;
    *a = *b;
    *b = temp;
    return;
}

// function to evaluate the expression
double evaluateExpr(char exp[]){
    int len = 0;
    // evaluatinf the length of the expresssion received
    while (exp[len]!='\0')
    {
        len++;
    }

    double temp = 0;
    double res = 0; // variable that stores result

    int i=0;
    char op1 = '+'; // variable that stores the operation to be carried out on the operands
    char op2;

    // iterating through the expression
    while(i<len){
        // if the character is a number
        if(!(isOperator(exp[i]) || exp[i] == ' ' || exp[i] == '\0' || exp[i] == '(' || exp[i] == ')')){
            double in = 0, f = 0;
            // evauating the integer part of the number
            while (exp[i] != '.' && exp[i] != ' ' && exp[i] != '\0' && !isOperator(exp[i]) && exp[i] != '(' && exp[i] != ')')
            {
                in = 10*in + (exp[i] - '0');
                i++;
            }
            if(exp[i] == '.') i++;
            int pow = 0;
            // evaluating the frational part of the number
            while (exp[i] != ' ' && exp[i] != '\0' && !isOperator(exp[i]) && exp[i] != '(' && exp[i] != ')')
            {
                in = 10*in + (exp[i] - '0');
                pow++;
                i++;
            }
            while(pow){
                in /= 10;
                pow--;
            }

            // performing the operation on the operands according to the variable
            if(op1 == '+') res += in;
            else if(op1 == '-') res -= in;
            else if(op1 == '/') res /= in;
            else if(op1 == '*') res *= in;
        }else if(exp[i] == '('){
            // storing the result calculated till now in the new variable temp to evaluate the expression in the brackets
            swap(&temp,&res);
            i++;
            op2 = op1; // storing the operation to be performed on the operands before the brackets
            op1 = '+';
        }else if(exp[i] == ')'){
            op1 = op2;
            if(op1 == '+') res = temp + res;
            else if(op1 == '-') res = temp - res;
            else if(op1 == '/') res = temp / res;
            else if(op1 == '*') res = temp * res;
            temp = 0;
            // resetting the value of temp to 0 and applying the operation on the values outside and inside the bracket
            i++;
        }else if(isOperator(exp[i])){
            op1 = exp[i];
            i++;
        }else i++;
    }
    
    // returning the result
    return res;
}

int main(){
    int socketid;
    struct sockaddr_in server_addr, client_addr;

    int i;
    char buffer[100]; // buffer to recieve the packets from the client

    // creating a socket
    if ((socketid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // providing the server address
    server_addr.sin_family		= AF_INET;
	server_addr.sin_addr.s_addr	= INADDR_ANY;
	server_addr.sin_port		= htons(20000);

    // binding the socket to the server address
    if (bind(socketid, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

    // listening for connections
    listen(socketid, 5);

    // Iterative server that handles one client at a time
    for(;;){
        int clilen = sizeof(client_addr);
        // accepting a connection
        int new_socketid = accept(socketid, (struct sockaddr *) &client_addr, &clilen);

        if (new_socketid < 0) {
			perror("Accept error\n");
			exit(0);
		}

        printf("Client Connected!\n");

        char *expr = NULL;
        int cursize = 0;
        int done = 0;
        memset(buffer, '\0', BUFF_SIZE);
        // recieving the data from the client in packets
        while (!done)
        {
            recv(new_socketid, buffer, 100, 0);
            int len = 0;
            for(int i=0;i<BUFF_SIZE;i++){
                if(buffer[i]=='\0') {
                    done = 1;
                    break;
                }
                len++;
            }
            expr = realloc(expr,(cursize + len)*sizeof(char) );
            for(int i=0;i<len;i++){
                expr[cursize] = buffer[i];
                cursize++;
            }
        }
        expr[cursize] = '\0';
		if(expr[0]!='\0') printf("Recieved Expression: %s\n", expr);

        // evauating the recieved expression and storing it in the buffer
        sprintf(buffer, "%f", evaluateExpr(buffer));

        // sending the evauated result to the client
		send(new_socketid, buffer, strlen(buffer) + 1, 0);

        // closing the connection with the client
		close(new_socketid);

        printf("Client Disconnected!\n\n");
    }

    return 0;
}