#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Attr.h"

int NoofClients = 0;
struct InfoCli *clients;

//Checks for the availability of the username
int Username_check(char a[]){
    int counter = 0;
    int return_value = 0;
    for(counter = 0; counter < NoofClients ; counter++){
        if(!strcmp(a,clients[counter].username)){
	    printf("Another client with username '%s' tries to connect. But username already exists. So rejecting it\n",a);
            return_value = 1;
            break;
        }
    }
    return return_value;
}



//ACK message sent with the list of usernames and the clientcount


void ACK(int connfd){

    struct Message Message_ACK;
    struct Header Header_ACK;
    struct MessageAttribute Attribute_ACK;
    int counter = 0;
    char temp[180];

    Header_ACK.version=3;
    Header_ACK.type=7;

    Attribute_ACK.type = 4;

    
    temp[0] = (char)(((int)'0')+ NoofClients);
    temp[1] = ' ';
    temp[2] = '\0';
    for(counter =0; counter< NoofClients-1; counter++)
    {
        strcat(temp,clients[counter].username);
        if(counter != NoofClients-2)
        strcat(temp, ",");
    }
    Attribute_ACK.length = strlen(temp)+1;
    strcpy(Attribute_ACK.payload, temp);
    Message_ACK.header = Header_ACK;
    Message_ACK.attribute[0] = Attribute_ACK;

    write(connfd,(void *) &Message_ACK,sizeof(Message_ACK));
}


/*
void ACK(int connfd){

    struct Message Message_ACK;
    struct Header Header_ACK;
    struct MessageAttribute Attribute_ACK;
    int i = 0;
    char temp[180];
    char temp1[100];

    Header_ACK.version=3;
    Header_ACK.type=7;
    //Header_ACK.length = 

    Message_ACK.attribute[0].type = 3;

    
    temp1[0] = (char)(((int)'0')+ NoofClients);

    strcpy(Message_ACK.attribute[0].payload, temp1);

    
    for(i =1; i<=NoofClients; i++)
    {
	Message_ACK.attribute[i].type = 2;
        Message_ACK.attribute[i].length= strlen(clients[i-1].username);
	strcpy(Message_ACK.attribute[i].payload,clients[i-1].username); 
    }
    
    Message_ACK.header = Header_ACK;

    write(connfd,(void *) &Message_ACK,sizeof(Message_ACK));
}


*/
//send NAK message to client, to refuse connection (for e.g. when username already exists)
void NAK(int connfd,int code){

    struct Message Message_NAK;
    struct Header Header_NAK;
    struct MessageAttribute MessageAttribute_NAK;
    char temp[32];

    Header_NAK.version =3;
    Header_NAK.type =5;

    MessageAttribute_NAK.type = 1;

    
    if(code == 1)
        strcpy(temp,"Username is incorrect");
 
    if(code == 2)
	strcpy(temp, "Client count exceeded");

    MessageAttribute_NAK.length = strlen(temp);
    strcpy(MessageAttribute_NAK.payload, temp);

    Message_NAK.header = Header_NAK;
    Message_NAK.attribute[0] = MessageAttribute_NAK;

    write(connfd,(void *) &Message_NAK,sizeof(Message_NAK));

    close(connfd);

}

 //Broadcasting ONLINE Message to everyone except me and the listerner
void ONLINE(fd_set master,int serverSockFd, int connfd, int maxfd)
	
{
			    struct Message Message_fwd;
			    int j;
                            printf("Server accepted the client : %s \n",clients[NoofClients-1].username);
           		    Message_fwd.header.version=3;
                            Message_fwd.header.type=8;
        		    Message_fwd.attribute[0].type=2;
		            strcpy(Message_fwd.attribute[0].payload,clients[NoofClients-1].username);	
			    
                            for(j = 0; j <= maxfd; j++) 
   		    	    {
	                    
        	            	    if (FD_ISSET(j, &master)) 
        	            	    {
        	            	      
        	            	        if (j != serverSockFd && j != connfd)
			                {
        	                    	    if ((write(j,(void *) &Message_fwd,sizeof(Message_fwd))) == -1)
					    {
        	                            	perror("send");
        	                            }
        	                        }
        	                    }       
        	             }

}

//Broadcast OFFLINE message to everyone except me and listener

void OFFLINE(fd_set master,int i, int serverSockFd, int connfd, int maxfd, int NoofClients){

struct Message Message_OFFLINE;
int y,j;
		                for(y=0;y<NoofClients;y++)
				{
				    	if(clients[y].fd==i)
					{
					        Message_OFFLINE.attribute[0].type=2;
						strcpy(Message_OFFLINE.attribute[0].payload,clients[y].username);	
					}
				}
                                
				printf("Socket %d belonging to user '%s'is disconnected\n", i,Message_OFFLINE.attribute[0].payload );
                                Message_OFFLINE.header.version=3;
                                Message_OFFLINE.header.type=6;
                                
                                for(j = 0; j <= maxfd; j++) 
   		    	        {
	                    	   // OFFLINE Message broadcasted to everyone except to myself and the listener
        	            	    if (FD_ISSET(j, &master)) 
        	            	    {
        	            	
        	            	        if (j != i && j != serverSockFd)
			                {

        	                    	    if ((write(j,(void *) &Message_OFFLINE,sizeof(Message_OFFLINE))) == -1)
					    {
        	                            	perror("ERROR: Sending");
        	                            }
        	                        }
        	                    }       
        	                }

}
//If client is valid, send an ACK, else a NAK
int Client_valid(int connfd, int maxClients){

    struct Message Message_join;
    struct MessageAttribute MessageAttribute_join;
    char temp[30];

    int return_status = 0;
    read(connfd,(struct Message *) & Message_join,sizeof(Message_join));
    
    MessageAttribute_join = Message_join.attribute[0];

    strcpy(temp, MessageAttribute_join.payload);
   

    if (NoofClients == maxClients){
	return_status=2;
	printf("A new client tries to connect. But Client count exceeded.So rejecting it\n");
	NAK(connfd, 2); // 2- indicates client count exceeded.
	return return_status;
    }
	
    return_status = Username_check(temp);
    if(return_status == 1)
        NAK(connfd, 1); // 1- indicates client present already
    else
    {
        strcpy(clients[NoofClients].username, temp);
        clients[NoofClients].fd = connfd;
        clients[NoofClients].NoofClients = NoofClients;
        NoofClients = NoofClients + 1;
	//printf("increment %d", NoofClients);
        ACK(connfd);
    }
    return return_status;
}


int main(int argc, char const *argv[])
{
     /* check command line arguments */
    if (argc != 4) {
       fprintf(stderr,"Please use the correct format: %s <hostname> <port> <max_clients>\n", argv[0]);
       exit(0);
    }
    /* code */
    struct Message Message_fwd,Message_OFFLINE;
    struct Message clientMessage;
    struct MessageAttribute Attribute_client;

    int serverSockFd, connfd, m, k;
    unsigned int len;
    int CliStat = 0;
    struct sockaddr_in servAddr,*cli;
    struct hostent* hret;
    
    fd_set master;
    fd_set read_fds;
    int maxfd, temp, i=0, j=0, x=0, y, bytes_read, maxClients=0;

    serverSockFd = socket(AF_INET,SOCK_STREAM,0);
    if(serverSockFd == -1)
    {
        perror("ERROR: Couldn't create a socket");
	exit(0);
    }
    else
        printf("Server socket is created.\n");
    
    bzero(&servAddr,sizeof(servAddr));

    int enable=1;
    if (setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))<0){
    		perror("ERROR: Setsockopt\n");
   		exit(1);
	}

    servAddr.sin_family = AF_INET;
    hret = gethostbyname(argv[1]);
    memcpy(&servAddr.sin_addr.s_addr, hret->h_addr,hret->h_length);
    servAddr.sin_port = htons(atoi(argv[2]));

    maxClients=atoi(argv[3]);

    clients= (struct InfoCli *)malloc(maxClients*sizeof(struct InfoCli));
    cli=(struct sockaddr_in *)malloc(maxClients*sizeof(struct sockaddr_in));
			
    if((bind(serverSockFd, (struct sockaddr*)&servAddr, sizeof(servAddr)))!=0)
    {
        perror("ERROR: Couldn't bind the socket\n");
        exit(0);
    }
    else
        printf("Binding successful.\n");
    
    if((listen(serverSockFd, maxClients))!=0)
    {
        perror("ERROR: Couldn't listen.\n");
	exit(0);
    }
    else
        printf("Listening successful.\n");
  

    FD_SET(serverSockFd, &master);
    maxfd = serverSockFd;

    for(;;)
    {
        read_fds = master;
        if(select(maxfd+1, &read_fds, NULL, NULL, NULL) == -1)
	{
            perror("ERROR: select");
            exit(4);
        }

        for(i=0 ; i<=maxfd ; i++)
	{
            if(FD_ISSET(i, &read_fds))
	    {
                if(i == serverSockFd)
		{
                    //Incoming connection
                    len = sizeof(cli[NoofClients]);
                    connfd = accept(serverSockFd,(struct sockaddr *)&cli[NoofClients],&len);

	            //Checking the validity of accept
                    if(connfd < 0)
                    {
                        perror("ERROR: Couldn't accept \n");
			exit(0);
                    }
			
		    //accept working properly
                    else
                    {
                        temp = maxfd;
                        FD_SET(connfd, &master);
                        if(connfd > maxfd){
                            maxfd = connfd;
                        }   
                        CliStat = Client_valid(connfd , maxClients);
                        if(CliStat == 0)  
                 		ONLINE(master, serverSockFd, connfd, maxfd);
                       
		        else if (CliStat==1)
			{
                            	//Username already present.Restore maxFD and remove it from the set
                            	maxfd = temp;
                            	FD_CLR(connfd, &master);
                        }  
			else
			{
                            	//Username already present.Restore maxFD and remove it from the set
                            	maxfd = temp;
                            	FD_CLR(connfd, &master);//clear connfd to remove this client
                        }  
 
                    }
                }
                else
		{
		    //OLD connections
		    if ((bytes_read=read(i,(struct Message *) &clientMessage,sizeof(clientMessage))) <= 0) 
                    {
	        	// got error or connection closed by client
			if (bytes_read == 0) 
				OFFLINE(master, i, serverSockFd, connfd, maxfd, NoofClients);
			else
				perror("ERROR In receiving");
			
			//Cleaning up after closing the erroneous socket
			close(i); 
			FD_CLR(i, &master); // remove from master set
			for (k=0; k<NoofClients; k++){
				if (clients[k].fd==i){
					m=k;
					break;
				}
			}
				

			for(x=m;x<(NoofClients-1);x++)
		        {
				clients[x]=clients[x+1];
			}
                        NoofClients--;
		    }
                    else
		    {
			
 			int payloadLength=0;
                        char temp[16];
			//Checking if the existing user becomes idle
			if (clientMessage.header.type ==9){  
			
				Message_fwd=clientMessage;
							Message_fwd.attribute[0].type=2;
							for(y=0;y<NoofClients;y++){
								if(clients[y].fd==i){
									strcpy(Message_fwd.attribute[0].payload,clients[y].username);
									strcpy(temp,clients[y].username);
									payloadLength=strlen(clients[y].username);
									temp[payloadLength]='\0';
									printf("User '%s' is idle\n", temp);
								}
							}
						}

			else{
                        //Non-zero-count message received from client
			
                    		Attribute_client = clientMessage.attribute[0];//message
				Message_fwd=clientMessage;

 				Message_fwd.header.type=3;
		        	Message_fwd.attribute[1].type=2;//username
                        	payloadLength=strlen(Attribute_client.payload);
                       	 	strcpy(temp,Attribute_client.payload);
                        	temp[payloadLength]='\0';


                        	//Message forwarded to clients
 		        	for(y=0;y<NoofClients;y++){
			    		if(clients[y].fd==i)
			       			 strcpy(Message_fwd.attribute[1].payload,clients[y].username);	
				}
				printf("User '%s' : %s", Message_fwd.attribute[1].payload, temp);
			}
			
                    	for(j = 0; j <= maxfd; j++) 
		    	{
                    	    if (FD_ISSET(j, &master)) 
                    	    {
                    	        // Message broadcasted to everyone except to myself and the listener
                    	        if (j != i && j != serverSockFd)
		                {
                            	    if ((write(j,(void *) &Message_fwd,bytes_read)) == -1)
				    {
                                    	perror("send");
                                    }
                                }
                            }       
                       }
                    }
                }
             }
        }
    }
    
    close(serverSockFd);

    return 0;
}
