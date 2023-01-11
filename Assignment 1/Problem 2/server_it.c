#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFF_SIZE 100

int isOperator(char a){
    if(a == '+' || a == '-' || a == '/' || a == '*') return 1;
    else return 0;
}

void swap(double *a, double *b){
    double temp = *a;
    *a = *b;
    *b = temp;
    return;
}

double evaluateExpr(char exp[]){
    int len = 0;
    while (exp[len]!='\0')
    {
        len++;
    }

    double temp = 0;
    double res = 0;

    int i=0;
    char op1 = '+';
    char op2;


    while(i<len){
        if(!(isOperator(exp[i]) || exp[i] == ' ' || exp[i] == '\0' || exp[i] == '(' || exp[i] == ')')){
            double in = 0, f = 0;
            while (exp[i] != '.' && exp[i] != ' ' && exp[i] != '\0' && !isOperator(exp[i]) && exp[i] != '(' && exp[i] != ')')
            {
                in = 10*in + (exp[i] - '0');
                i++;
            }
            int pow = 0;
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

            if(op1 == '+') res += in;
            else if(op1 == '-') res -= in;
            else if(op1 == '/') res /= in;
            else if(op1 == '*') res *= in;
        }else if(exp[i] == '('){
            swap(&temp,&res);
            i++;
            op2 = op1;
            op1 = '+';
        }else if(exp[i] == ')'){
            op1 = op2;
            if(op1 == '+') res = temp + res;
            else if(op1 == '-') res = temp - res;
            else if(op1 == '/') res = temp / res;
            else if(op1 == '*') res = temp * res;
            temp = 0;
            i++;
        }else if(isOperator(exp[i])){
            op1 = exp[i];
            i++;
        }else i++;
    }
    
    return res;
}

int main(){
    int socketid;
    struct sockaddr_in server_addr, client_addr;

    int i;
    char buffer[100];

    if ((socketid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    server_addr.sin_family		= AF_INET;
	server_addr.sin_addr.s_addr	= INADDR_ANY;
	server_addr.sin_port		= htons(20000);

    if (bind(socketid, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

    listen(socketid, 5);

    for(;;){
        int clilen = sizeof(client_addr);
        int new_socketid = accept(socketid, (struct sockaddr *) &client_addr, &clilen);

        if (new_socketid < 0) {
			perror("Accept error\n");
			exit(0);
		}

        char *expr = NULL;
        int cursize = 0;
        int done = 0;
        while (!done)
        {
            recv(new_socketid, buffer, 100, 0);
            int len = 0;
            for(int i=0;i<BUFF_SIZE;i++){
                if(buffer[i]=='\0') {
                    done = 1;
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

		printf("%s\n", expr);

        sprintf(buffer, "%f", evaluateExpr(buffer));

		send(new_socketid, buffer, strlen(buffer) + 1, 0);

		close(new_socketid);
    }

    return 0;
}