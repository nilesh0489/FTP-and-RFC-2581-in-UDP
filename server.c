#include "unpifiplus.h"
#include "unp.h"
#define SIZE 1024
#define DATA_SIZE 496
#define MAX_SOCKETS 20

int socket_count = 0;
int port_no, window_size;

// Structure that stores the information for each created socket
typedef struct
{
	int listen_fd;
	struct sockaddr_in serverip;
	struct sockaddr_in netmask;
	struct sockaddr_in submask;

}server_data;

// Global socket array for holding socket information for all created sockets
server_data sdata[MAX_SOCKETS];

// Structure representing a packet of data
struct packet 
{
      uint32_t seq;               /* sequence #*/
      uint32_t ack;               /* acknowledgement */ 
      uint32_t last_flag;	  /* flag to indicate last packet */
      uint32_t window_size;       /* window size from client*/
      char data_buffer[DATA_SIZE]; /* buffer that stores actual data */
      struct packet *next; 	  /* pointer to the next packet in the list */
 }send_packet, recv_packet;

typedef struct packet packet;

packet *head = NULL;	

void add_packet_to_buffer(packet *new_packet)
{ 
  	packet *incoming_packet = (packet *) malloc(sizeof(packet));

	// Copy the buffer into the new packet
	// Copy the seq number
	incoming_packet->seq = new_packet->seq;
	strncpy(incoming_packet->data_buffer, new_packet->data_buffer, sizeof(new_packet->data_buffer));
  
	// If this is the first packet
	// Set this as the head node
	if(NULL == head)
   	{ 
    		incoming_packet->next = NULL;
    		head = incoming_packet;
   	}
   
	// Else find the end of the packet list
	// Insert the new packet at the end
  	else
   	{ 
    		packet *last_packet;
    
		for(last_packet = head; last_packet->next != NULL;)
       			last_packet = last_packet->next;
       
      		last_packet->next = incoming_packet;
      		incoming_packet->next = NULL;
    	}
}

int delete_packet_from_buffer(int sequence_number)
{
	int count = 0;

	if(head == NULL)
	{
		printf("No packet to delete\n");
		return;
	}

	packet *current_packet = head;
	packet *prev_packet = head;

	if(head->seq == sequence_number)
	{
		head = head->next;      // transfer the address of 'packet->next' to 'head'
		free(current_packet);
		return;
	}


	while(current_packet->next != NULL && current_packet->seq != sequence_number)
	{
		prev_packet = current_packet;
		current_packet = current_packet->next;
	}

	prev_packet->next = current_packet->next;
	free(current_packet);
	
}     

int checkIfLocal(struct sockaddr_in IPServer, struct sockaddr_in IPClient, struct sockaddr_in IPNetMask)
{
	int isLocal = 0;
	int isSameHost = 0;

	printf("\n*************************************************************************\n");

	if(IPServer.sin_addr.s_addr == IPClient.sin_addr.s_addr)  //  same host
	{
		printf("\n[INFO]Client and server running on same host\n");
		isLocal = 1;
		isSameHost = 1;
		
	}
	else
	{
		printf("\n[INFO]Client and server running on different hosts\n");
		
	}

	struct in_addr client_network;
	struct in_addr server_network;

	// bitwise AND of ip and netmask gives the network
	client_network.s_addr = IPClient.sin_addr.s_addr & IPNetMask.sin_addr.s_addr;
	server_network.s_addr = IPServer.sin_addr.s_addr & IPNetMask.sin_addr.s_addr;

	//printf("Network address of server = %s\n", inet_ntoa(server_network));

	if(client_network.s_addr == server_network.s_addr && isSameHost == 0)
	{
		isLocal = 1;
	}
	else if(isSameHost == 0)
	{
		printf("\n[INFO]Client and server running on different networks\n");
		//printf("Client IP address = %s\n", Sock_ntop_host(ip_addr,sizeof(*ip_addr)));
		//printf("Server IP address = %s\n", Sock_ntop_host(&IPServer,sizeof(IPServer)));
	}

	printf("\n*************************************************************************\n");
	return isLocal;
}

void read_input()
{
	/* Read from server.in */	
	FILE*fp ;
        char buf[SIZE] = {0};
	char *n;
	
	if ((fp = fopen("server.in", "rt")) == NULL)
	{
      		printf("\n[ERROR]Cannot open server.in\n");
		exit(0);
	}	

	n = fgets(buf, sizeof(buf), fp);
	port_no = atoi(buf);

	printf("\n*************************************************************************\n");

	printf("\n[INFO] Server Port number = %d\n", port_no);

	n = fgets(buf,sizeof(buf),fp);
	window_size = atoi(buf);

	printf("\n[INFO] Sliding Window size = %d\n", window_size);

	printf("\n*************************************************************************\n");

      	fclose(fp);
    
}

// Bind the unicast addresses to the sockets
void bind_unicast()
{
	int i = 0;
	const int on = 1;
	struct ifi_info	 *ifi, *ifihead;
	struct sockaddr_in *sa, cliaddr, servaddr;

	printf("\n*************************************************************************\n");

    	/* Get IP address and other details */ 
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1); ifi != NULL; ifi = ifi->ifi_next)
	{         
		/*4bind unicast address */
		sdata[i].listen_fd = Socket(AF_INET, SOCK_DGRAM, 0);

		Setsockopt(sdata[i].listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		sa = (struct sockaddr_in *) ifi->ifi_addr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(port_no);

      		sdata[i].serverip.sin_family = AF_INET;
		sdata[i].serverip.sin_port = htons(port_no);
		sdata[i].serverip.sin_addr.s_addr = inet_addr(Sock_ntop_host((SA *)sa, sizeof(*sa)));

		Bind(sdata[i].listen_fd, (SA *)&sdata[i].serverip, sizeof(sdata[i].serverip));

		printf("\n[INFO] Bound unicast address - %s\n", Sock_ntop((SA *) sa, sizeof(*sa)));
		sa = (struct sockaddr_in *) ifi->ifi_ntmaddr;
		sdata[i].netmask = *sa;
		sdata[i].submask.sin_addr.s_addr = sdata[i].serverip.sin_addr.s_addr & sdata[i].netmask.sin_addr.s_addr;
		i++;
	}


        socket_count = i;
		
	for(i = 0;i < socket_count;i++)
	{ 
		printf("\n*************************************************************************\n");
		printf("\n[INFO]IP Address : %s\n", inet_ntoa(sdata[i].serverip.sin_addr));
		printf("\n[INFO] Network Mask : %s\n", inet_ntoa(sdata[i].netmask.sin_addr));
		printf("\n[INFO] Subnet Address : %s\n", inet_ntoa(sdata[i].submask.sin_addr));
	}

	printf("\n*************************************************************************\n");
}

void transfer_file(int connection_fd, char *fname)
{
	int i = 0;
	const int on = 1;
	FILE *fp;
	int flags = 0;

	char send_buffer[DATA_SIZE];

	fp = fopen (fname, "r" ) ;

	if( fp == NULL )
	{
		printf("\n[ERROR]Cannot open file %s\n", fname);
		exit(0);
	}
	else
	{
	 	printf("\n[INFO]Reading from %s\n", fname);
	}

	// Receive the ACK from the client and extract the client window size
	int recv_window_size;

	Recv(connection_fd, &recv_packet, sizeof(recv_packet), 0);
	recv_window_size = recv_packet.window_size;

	//printf("Receive window size = %d\n", recv_window_size);

	int min_window_size, congestion_window = 1;			

	int read_bytes = 0;
	int sequence_number = 0, counter = 0, receive_ack;     
	
	while(1)
	{
		min_window_size = MIN(window_size, congestion_window);

		for(i = 0; i < min_window_size; i++)
		{

			bzero(send_buffer, sizeof(send_buffer));
			read_bytes = Read(fileno(fp), send_buffer, DATA_SIZE);

			//printf("\nRead data = %s\n", send_buffer);

			strncpy(send_packet.data_buffer, send_buffer, DATA_SIZE);
			send_packet.seq = sequence_number;
			send_packet.last_flag = 0;
			sequence_number++;

			if(read_bytes < DATA_SIZE)
			{
				// This is the last packet
				send_packet.last_flag = 1;
			}

			// Write the packet to connection_fd			
			Send(connection_fd, &send_packet, sizeof(send_packet), flags);

			// Add packet to the buffer
			add_packet_to_buffer(&send_packet);
		} // end of for

		counter = sequence_number;

		
		// Receive ACK from the client and update the list	
		do {

			recv(connection_fd, &recv_packet, sizeof(recv_packet), 0);
		  	receive_ack = recv_packet.ack;
		  
			if(receive_ack == 0)
			{
				printf("\n[INFO]The window size is: %d\n so I am waiting", receive_ack);
				continue;
			}

		  
		  	//printf("The ack received is: %d\n", receive_ack);
		  
		 	int remaining_packets = delete_packet_from_buffer(receive_ack);
		 
		 }while(receive_ack != counter);
		 
		 congestion_window = congestion_window + 1;
		 recv_window_size = recv_packet.window_size;

	
	} // end of while

	fclose(fp);

}

void create_connection_socket(server_data data)
{
	int i = 0;
	const int on = 1;
	struct sockaddr_in IPServer;
	struct sockaddr_in IPClient;	
	struct sockaddr_in IPNetMask;
	int flags = 0;

	int n;
	char file_name[SIZE] = {0};
	int connection_fd, maxfd = 0;
	char send_buffer[DATA_SIZE];
	int len = sizeof(IPClient);
	int isLocal;

	// Get the packet from the client
	// Read the file name that has to be transferred
	// TODO - File name received from client can't be opened - Weird issue
	n = recvfrom(data.listen_fd, &recv_packet, sizeof(recv_packet), 0, (SA *) &IPClient, &len);			

	char fname[SIZE] = {0};

	strncpy(fname, recv_packet.data_buffer, sizeof(recv_packet.data_buffer)); 

	// TODO - Hard coding the file name to be read for the time being - Need to fix	
	strcpy(fname, "server_file");

	IPServer = data.serverip;
	IPNetMask = data.netmask;

	printf("\n*************************************************************************\n");

	printf("\n[INFO]ServerIp : %s, Port number: %d\n", inet_ntoa(IPServer.sin_addr), ntohs(IPServer.sin_port));
	printf("\n[INFO]ClientIp : %s, Port number: %d\n", inet_ntoa(IPClient.sin_addr), ntohs(IPClient.sin_port));

	printf("\n*************************************************************************\n");

	struct sockaddr_in child_addr;

	// Create a new connection socket
	connection_fd = Socket(AF_INET, SOCK_DGRAM, 0);

	// Check if the client is local. If it is, then set SO_DONTROUTE socket flag
	isLocal = checkIfLocal(IPServer, IPClient, IPNetMask);

	if(isLocal == 1)
	{
		Setsockopt(data.listen_fd, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));
	}

	Setsockopt(connection_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
	// Bind the connection socket
	len = sizeof(child_addr);

	child_addr.sin_family = AF_INET;
	child_addr.sin_port = htons(0);
	child_addr.sin_addr.s_addr = sdata[i].serverip.sin_addr.s_addr;
	
	Bind(connection_fd, (SA *)&child_addr, len);

	printf("\n[INFO]Server(child) bind completed\n");

	// Get the port number that the socket is bound to
	if (getsockname(connection_fd, (SA *) &child_addr, &len) == -1) 
	{
		perror("getsockname() failed");
	}

	printf("\n*************************************************************************\n");
		 
	printf("\n[INFO]Server IP address is: %s\n", inet_ntoa(child_addr.sin_addr));
	printf("\n[INFO]Server port number is: %d\n", (int) ntohs(child_addr.sin_port));

	printf("\n*************************************************************************\n");
	// Connect to the client using the newly created connection socket
	len = sizeof(IPClient);				
	Connect(connection_fd, (SA *) &IPClient, len);
	
	// Get the client IP address and port bumber
	if (getpeername(connection_fd, (SA *) &IPClient, &len) == -1)
	{
	  	perror("getpeername() failed\n");
	}

	printf("\n[INFO]Client IP address is: %s\n", inet_ntoa(IPClient.sin_addr));
	printf("\n[INFO]Client port is: %d\n", (int) ntohs(IPClient.sin_port));

	printf("\n*************************************************************************\n");

	// Send the new port number that the client has to connect to using the listening socket
	sprintf(send_buffer, "%d", ntohs(child_addr.sin_port));
	Sendto(data.listen_fd, send_buffer, sizeof(send_buffer), 0, (SA *) &IPClient, sizeof(IPClient));

	transfer_file(connection_fd, fname);
		
}

void handle_file_transfer()
{
	int i, n;
	fd_set readset;
	pid_t childpid;
	int maxfd = 0;

	FD_ZERO(&readset);
	
 while(1)
 {	  
	for(i = 0; i < socket_count; i++)
	{
		FD_SET(sdata[i].listen_fd, &readset);
		if(sdata[i].listen_fd > maxfd)
		maxfd = sdata[i].listen_fd;
	}
	 
	 int socket_count = select(maxfd + 1, &readset, NULL, NULL, NULL);
	 
	 socklen_t len;
	  
	 for(i = 0; i < socket_count; i++)
	 { 
		if(FD_ISSET(sdata[i].listen_fd, &readset))
	   	{

	     		if ( (childpid = fork()) == 0) 
	      		{

				create_connection_socket(sdata[i]);
 
	      		} 
	   	}	 
	  
	 }  // End of socket count for loop

  }
}


int main(int argc, char **argv)
{

	int i = 0;
	const int on = 1;

	FILE *fp;

	// Read input from the server.in file
	read_input();

	// Bind the unicast addresses
	bind_unicast();	

	// Begin the file transfer process
	handle_file_transfer();	
		  
	return 0;  
	
}
