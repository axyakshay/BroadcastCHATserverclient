#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "Attr.h"

//Message from the server
int MessagefromServer(int sockfd){

    struct Message servMessage;
    int status = 0;
    int nbytes=0;
     int value, i;
    char a[]="1";

    nbytes=read(sockfd,(struct Message *) &servMessage,sizeof(servMessage));
    if(nbytes <=0){
	perror("Server disconnected \n");
	kill(0, SIGINT);
	exit(1);
	}


    //FWD message
    if(servMessage.header.type==3)
    {
    	if((servMessage.attribute[0].payload!=NULL || servMessage.attribute[0].payload!='\0') && (servMessage.attribute[1].payload!=NULL || servMessage.attribute[1].payload!='\0') && servMessage.attribute[0].type==4 && servMessage.attribute[1].type==2)
	{     
		printf("%s : %s ", servMessage.attribute[1].payload, servMessage.attribute[0].payload);
	}
        status=0;
    }

    //NAK message
    if(servMessage.header.type==5)
    {
    	if((servMessage.attribute[0].payload!=NULL || servMessage.attribute[0].payload!='\0') && servMessage.attribute[0].type==1)
       {
                printf("Disconnected.NAK Message from Server is %s \n",servMessage.attribute[0].payload);
       }
       status=1;
    }

    //OFFLINE message
    if(servMessage.header.type==6)
    {
	if((servMessage.attribute[0].payload!=NULL || servMessage.attribute[0].payload!='\0') && servMessage.attribute[0].type==2)
       {
                printf("User '%s' is now OFFLINE \n",servMessage.attribute[0].payload);
       }
       status=0;
    }

   //ACK Message
   if(servMessage.header.type==7)
    {    	
	if((servMessage.attribute[0].payload!=NULL || servMessage.attribute[0].payload!='\0') && servMessage.attribute[0].type==4)
       {
                printf("ACK Message from Server is %s \n",servMessage.attribute[0].payload);
       }
       status=0;
    }

/*
    if(servMessage.header.type==7)
    {    	

	printf("the ack is %s \n", servMessage.attribute[0].payload);
        
	if((!strcmp(a, servMessage.attribute[0].payload)) && servMessage.attribute[0].type==3)
       {
                printf("ACK Message from Server is \n");
		printf("clientcount:  %s \n",servMessage.attribute[0].payload);
       
		value=(int)servMessage.attribute[0].payload;
		for (i=1 ; i<= value; i++){
			if((servMessage.attribute[i].payload!=NULL || servMessage.attribute[i].payload!='\0') && servMessage.attribute[i].type==2)
			printf("username %d is : %s \n", i, servMessage.attribute[i].payload);
		}
	}
		
		
       status=0;
    }

*/
    //ONLINE Message
    if(servMessage.header.type==8)
    {
	if((servMessage.attribute[0].payload!=NULL || servMessage.attribute[0].payload!='\0') && servMessage.attribute[0].type==2)
       {
                printf("User '%s' is now ONLINE \n",servMessage.attribute[0].payload);
       }
       status=0;
    }

    //IDLE Message
    if(servMessage.header.type==9)
    {
		if((servMessage.attribute[0].payload!=NULL || servMessage.attribute[0].payload!='\0') && servMessage.attribute[0].type==2)
       {
                printf("User '%s' is now IDLE \n",servMessage.attribute[0].payload);
       }
       status=0;
    }


    return status;
}

//Send a JOIN Message to server when connected to server
void sendJoin(int sockfd,const char *arg[]){


    struct MessageAttribute attribute_join;
    struct Header header;
    struct Message message;
    int return_status = 0;

    //Building 'JOIN' Header
    header.version = '3';
    header.type = '2';
    message.header = header;

    //Building username attribute
    attribute_join.type = 2;
    attribute_join.length = strlen(arg[1]) + 1;
    strcpy(attribute_join.payload,arg[1]);
    message.attribute[0] = attribute_join;

    write(sockfd,(void *) &message,sizeof(message));

    // Wait for server's reply
    sleep(1);
    return_status = MessagefromServer(sockfd); 
    if(return_status == 1)
            {close(sockfd);
	     exit(0);
	}

		
}


//Accept user input, and send it to server for broadcasting
void chat(int connect){

    struct Message message;
    struct MessageAttribute Attribute_client;

    int bytes_read = 0;
    char temp[600];
    
    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    select(STDIN_FILENO+1, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(STDIN_FILENO, &readfds))
    {
	bytes_read = read(STDIN_FILENO, temp, sizeof(temp));
        if(bytes_read > 0)
	temp[bytes_read] = '\0';

	strcpy(Attribute_client.payload,temp);
    	Attribute_client.type = 4;
    	message.attribute[0] = Attribute_client;
    	write(connect ,(void *) &message,sizeof(message));
    }
}

int main(int argc, char const *argv[])
{
     /* check command line arguments */
    if (argc != 4) {
       fprintf(stderr,"Please use this format: %s <username> <server_ip> <server_port>\n", argv[0]);
       exit(0);
    }
    /* code */
    if(argc == 4)
    {
	    int sockfd;
	    struct Message msg;
	    struct sockaddr_in servaddr;
	    struct hostent* hostret;
	    fd_set master;
	    fd_set new;
	    fd_set read_fds;
	    FD_ZERO(&read_fds);
	    FD_ZERO(&master);
	    sockfd = socket(AF_INET,SOCK_STREAM,0);

	    if(sockfd==-1)
	    {
        	perror("Failure in socket creation\n");
	        exit(0);
	    }

	    else
	        printf("Socket creation is successful\n");
    
	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family=AF_INET;
	    hostret = gethostbyname(argv[2]);
	    memcpy(&servaddr.sin_addr.s_addr, hostret->h_addr,hostret->h_length);
	    servaddr.sin_port = htons(atoi(argv[3]));

	    if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))!=0)
	    {
	        printf("ERROR: Connecting to the server\n");
	        exit(0);
	    }
	    else
	    {
	        sendJoin(sockfd, argv);
		printf("Server connection successful \n");
	        FD_SET(sockfd, &master);
		FD_SET(STDIN_FILENO, &new);
		struct timeval tv;
		tv.tv_sec=10;
		tv.tv_usec=0;
			
		pid_t recvpid;
		int secs, usecs;
		recvpid=fork();
		if(recvpid==0){
		
	           for(;;)
			{
			read_fds=new;
			tv.tv_sec=10;
		        tv.tv_usec=0;
			secs=(int) tv.tv_sec;
		        usecs=(int) tv.tv_usec;
			
			if(select(STDIN_FILENO+1, &read_fds, NULL, NULL, &tv) == -1){
	                	perror("ERROR: select isn't functioning properrly\n");
                		exit(4);
            	    	}
			if(FD_ISSET(STDIN_FILENO, & read_fds)){
				chat(sockfd);
				continue;}
	
			else if(tv.tv_sec==0 && tv.tv_usec==0){
				
				printf("Time out!! No user input for %d secs %d usecs\n", secs, usecs);
				msg.header.type=9;
				msg.header.version=3;
				tv.tv_sec=10;
		        	tv.tv_usec=0;
				read_fds=new;
				write(sockfd,(void *) &msg,sizeof(msg));
				continue;
			}
			}
			
		    }
		    else{	
	           

	            for (;;){
		    read_fds = master;
	            
		    if(select(sockfd+1, &read_fds, NULL, NULL, NULL) == -1)
		    {
	                perror("select");
                	exit(0);
            	    }
		    
	            if(FD_ISSET(sockfd, &read_fds))
                    	MessagefromServer(sockfd);
		    
		    }
		    kill(recvpid, SIGINT);
                }

                printf("\n initConnection Ends \n");
            }
     }
     printf("\n Client close \n");
     return 0;
}
