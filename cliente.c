/*###### para compilar gcc cliente.c -o -lpthread ######*/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <pthread.h>

#define IP "127.0.0.1"

/*VARIÁVEIS ADICIONAIS*/
pthread_mutex_t interations; // mutex responsavel pelas interações 
int conectionNumber = 0;  // numero de conexoes entre cliente servidor
int NUMBER_THREADS =0;
static int porta;
pthread_t *threads;
int countConex=0;
int areaCritia = 0;
int countRead = 1;
int countWrite= 1;
int totalConexao = 0;


int criarConexao(){

    // config variables
    int sockfd =0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 

    memset(recvBuff, '0',sizeof(recvBuff));
    
    /* Passo 1 - Criar socket */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket: ");
        exit(1);
    } 

    /* Passo 2 - Configura struct sockaddr_in */
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porta); 
    /* converte IP em formato string para o formato exigido pela struct sockaddr_in*/
    if(inet_pton(AF_INET,IP, &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(1);
    } 

    /* Passo 3 - Conectar ao servidor */
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("connect: ");
      exit(1);
    } 
	
    return sockfd;
}

void envioRecebimento(int valor, int id){
		//Chamada de conexao
		int sockfd = criarConexao();

		/* Envia a mensagem para o servidor. */
		printf("\nEnviou:%d", valor);
		send(sockfd, &valor, sizeof(valor), 0); 

		/*Recebe mensagem do servidor*/
		int buffer = 0;
		if(recv(sockfd,&buffer, sizeof(buffer), 0) < sizeof(int)){
			perror("recv : ");	
			close(sockfd);	
		}
	  	printf("\nRecebeu:%d",buffer);
}

int decrementaConexao(int id){
		int run = 0;
		// codigo critico onde é decrementado variavel 
		pthread_mutex_lock(&interations);		
			conectionNumber -= 1;			
			run  = (conectionNumber < 1) ? 0 : 1;	
			//printf("\nN conexoes = %d",conectionNumber);		
		pthread_mutex_unlock(&interations);
		return run;
}

void areaCritica(int valor, int id){
	int run = 1;
	int metadeConexao = (totalConexao/2);

	while(run){	
		if(valor == 1 && countRead <= metadeConexao){
			countRead++;
			envioRecebimento(valor,id);
			run = decrementaConexao(id);
		}
		if(valor == 2 && countWrite <= metadeConexao ){
			countWrite++;
			envioRecebimento(valor,id);
			run = decrementaConexao(id);
		}			
	}
}

void * leitura(void * arg){
	int id = *(int *)arg;
	areaCritica(1,id);
}	

void * escrita(void * arg){
	int id = *(int *)arg;
	areaCritica(2,id);
}

void closeThreads(){	
	int i = 0;	
	for(i = 0; i < NUMBER_THREADS; i++){
		if(pthread_join(threads[i], NULL)) {
			fprintf(stderr, "Error joining thread\n");
			exit(0);
		}
	}	
}

int main(int argc, char *argv[]){

	/*Escrever uma mensagem*/
	printf("\n Digite porta acima 5003:");
	scanf("%d",&porta);

	/*Escrever uma mensagem*/
	printf("\n Numero total de conexoes: ");
	scanf("%d",&conectionNumber);	

	/*Escrever uma mensagem*/
	printf("\n Numero total de threads: ");
	scanf("%d",&NUMBER_THREADS);

	//inicializa variaveis	
	int i = 0;
	int *ids;
	totalConexao = conectionNumber;
	pthread_mutex_init(&interations, NULL);
	threads = (pthread_t *)malloc(NUMBER_THREADS * sizeof(pthread_t)); 
	ids = (int *)malloc(NUMBER_THREADS * sizeof(int));

	for(i=0; i<NUMBER_THREADS; i++){
		ids[i] = i;
	}

	for(i = 0; i < NUMBER_THREADS; i++){

		if(i%2==0){		
			printf("Create Thread ID = %d \n", i);		
			pthread_create(&threads[i], NULL, leitura, (void*)&ids[i]);
		}else{
			printf("Create Thread ID = %d \n", i);
			pthread_create(&threads[i], NULL, escrita, (void*)&ids[i]);
		}		
	}
	
	printf("\n Escrita %d", countWrite);
	printf("\n Leitura %d", countRead);
	
	//Encerra todas as thread
	closeThreads();	
 
    return 0;
}




