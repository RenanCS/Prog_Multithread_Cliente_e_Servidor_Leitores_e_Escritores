#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <semaphore.h>
#include <pthread.h>

// Variaveis globais
static int porta;
static int valor = 0;
static int listenfd = 0;
static struct sockaddr_in serv_addr, clientaddr;
static socklen_t sz;


//variaveis para controle de concorrencia
pthread_mutex_t db, mutex;     
int   rc;   

void montaConexao(){

    char sendBuff[1025];
    memset(sendBuff, '0', sizeof(sendBuff));
	/* Passo 1 - Criar Socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	  perror("socket: ");
	  exit(1);
    }	

    /* Passo 2 - Configurar estrutura sockaddr_in */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(porta); 
    
    /* Passo 3 - Associar socket com a estrutura sockaddr_in     */
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
   	  perror("bind: ");
	  exit(1);
    }

    /* Passo 4 - Tornar o servidor ativo  */
    listen(listenfd, 100); 

}


void * writeData(void * connfd){

	int conf = * (int *) connfd;
	
  	pthread_mutex_lock(&db);    //down(&db); garante acesso exclusivo a base de dados
       
		valor ++; // incrementa em 1		
		printf("\nEscrevendo %d", valor);
 		
		/* Envia a mensagem para o cliente. */
		send(conf, &valor, sizeof(valor), 0); //le a base de dados
		
		free(connfd);
	
		/* Encerra conexão com o cliente. */
		close(conf);

        pthread_mutex_unlock(&db);  //up(&db); libera o acesso a base de dad

}

void * readData(void * connfd){

	int conf = * (int *) connfd;

 	pthread_mutex_lock(&mutex);           //down(&mutex); garante acesso exclusivo a variavelrc
   	   rc=rc+1;                              //um novo leitor

    	if(rc==1)  pthread_mutex_lock(&db);   //caso este seja o primeiro leitor...
    	  pthread_mutex_unlock(&mutex);         //up(&mutex); libera o acesso a variavel rc

	
	/* Envia a mensagem para o cliente. */
	send(conf, &valor, sizeof(valor), 0); 
	printf("\nLendo %d", valor);

     	pthread_mutex_lock(&mutex);           //down(&mutex); garante acesso exclusivo a variavel rc
        rc=rc-1;                              //um leitor a menos...

     	if(rc==0) pthread_mutex_unlock(&db);  //caso este seja o ultimo leitor...
      	pthread_mutex_unlock(&mutex);         //up(&mutex); libera o acesso a variavel rc

	free(connfd);

	/* Encerra conexão com o cliente. */
	close(conf);
}

int main(int argc, char *argv[]){
	
	// variavel de conexao
	int res;
	
	 pthread_attr_t attr;
	//inicializacao mutex...
	pthread_mutex_init(&db, NULL);
	pthread_mutex_init(&mutex, NULL);
	pthread_attr_init(&attr);
  
	printf("Digite porta acima 5003: ");
	scanf("%d",&porta);

	montaConexao();
	//Pool assignment makes integer from pointer without a cast [-Wint-conversion]
	

	while(1){
		
		int * connfd;
		connfd = (int *)malloc(sizeof(int));

		/* Passo 5 - Aguardar conexao do cliente.  */
		if ((*connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &sz)) < 0){
			perror("accept: ");
			continue;
		}

		int buffer = 0;
	
		if(recv(*connfd,&buffer, sizeof(buffer), 0) < sizeof(int)){
			perror("recv: ");	
			close(*connfd);	
		}
	
		res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if (res != 0) {
			perror("Setting detached state failed");
			exit(EXIT_FAILURE);
		}	

		pthread_t th1;

		// cria uma thread passando o valor do buffer que contem 1 ou 2		
		//printf("Recebeu o valor: %d \n",data);	
		if(buffer == 1) {// thread de leitura
		    pthread_create(&th1, &attr, readData,connfd);

		}else if(buffer == 2){ // thread de escrita
		    pthread_create(&th1, &attr, writeData,connfd);
		}
	

	}
	printf("\nTotal de escrita:%d", valor);
      
}



