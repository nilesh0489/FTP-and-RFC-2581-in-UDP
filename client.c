#include "unp.h"
#include "unpifiplus.h"
#include "stdio.h"
#define SIZE 1024
#define LINE_SIZE 50
#define BUF_SIZE 128
#define DATA_SIZE 496

static void     sig_alrm(int);

/* Structure that holds the data read from the client.in file */
/* TODO - Add other variables to be read in */
typedef struct client_data
{
	char server_ip[256];
	int server_port_no;
	char file_name[256];
	int sliding_window_size;
	int rg_seed_value;
	float probability;
	int mean;

}client_data;

client_data cdata;

int current_window_size;

/* Global IP Addresses to be assigned to the server and client */
struct sockaddr_in IPServer;
struct sockaddr_in IPClient;

struct packet 
{
      uint32_t seq;               /* sequence #*/
      uint32_t ack;               /* acknowledgement*/ 
      uint32_t last_flag;	  /* last flag */
      uint32_t window_size;       /* window size from client*/
      char data_buffer[DATA_SIZE];
      struct packet *next;
}send_packet, recv_packet;

typedef struct packet packet;

packet *head = NULL;

void read_input_file(char* file);
void prifinfo_plus(struct ifi_info *ifi);
void receive_file(int sockfd);

int main(int argc, char **argv)
{
	struct ifi_info	*ifihead;
	int sockfd;

	if (argc != 2)
		err_quit("usage: client <FileName>");

	read_input_file(argv[1]);
	
	// Print information about the IP address and network masks
	ifihead = Get_ifi_info_plus(AF_INET, 1);
	prifinfo_plus(ifihead);

	// Create a socket that connects to the server
	sockfd = create_socket();

	receive_file(sockfd);

	return 0;
}

void add_packet_to_buffer(packet *new_packet)
{ 
  	packet *incoming_packet = (packet *) malloc(sizeof(packet));

	current_window_size = current_window_size - 1;

	// Copy the buffer into the new packet
	// Copy the seq number
	incoming_packet->seq = new_packet->seq;
	memcpy(incoming_packet->data_buffer, new_packet->data_buffer, sizeof(new_packet->data_buffer));

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
    		packet *last_packet; //(struct packet *) malloc(sizeof(struct packet));
    
		for(last_packet = head; last_packet->next != NULL;)
       			last_packet = last_packet->next;
       
      		last_packet->next = incoming_packet;
      		incoming_packet->next = NULL;
    	}
}

void read_packet_from_buffer()
{

	if(head == NULL)
	{
		printf("[ERROR] No packet available for read\n");
	}

	else
	{ 
		
		do
		{
			//packet *read_packet = (packet *) malloc(sizeof(packet)); // allocate space for node
			packet *read_packet = NULL;			
			read_packet = head;                   // transfer the address of 'head' to 'packet'

			printf("%s", read_packet->data_buffer);
			head = read_packet->next;      // transfer the address of 'packet->next' to 'head'
			free(read_packet);
			current_window_size = current_window_size + 1;
		}while(head != NULL);
	}

}     

/* Function that reads the input file and stores read values in client_data structure */
void read_input_file(char *file)
{
	FILE *fp;
	char input[LINE_SIZE];
	char *n;

	if ((fp = fopen(file, "rt")) == NULL)
	{
      		printf("Cannot open %s\n", file);
		exit(0);
	}

	n = fgets(input, sizeof(input), fp);

	printf("\n[INFO] IP Address of server is %s\n",input);
	strcpy(cdata.server_ip, input);

	n = fgets(input,sizeof(input),fp);
	cdata.server_port_no = atoi(input);

	n = fgets(input,sizeof(input),fp);
	strcpy(cdata.file_name, input);

	printf("\n*************************************************************************\n");

	printf("[INFO]The file name is %s\n", cdata.file_name);

	n = fgets(input,sizeof(input),fp);
	cdata.sliding_window_size = atoi(input);

	current_window_size = cdata.sliding_window_size;
	printf("[INFO]The sliding window size is  %d\n", cdata.sliding_window_size);

	n = fgets(input,sizeof(input),fp);
	cdata.rg_seed_value = atof(input);

	printf("\n[INFO]Random generator seed value is  %d\n", cdata.rg_seed_value);

	n = fgets(input,sizeof(input),fp);
	cdata.probability = atoi(input);

	printf("\n[INFO]Probability of datagram loss is %f\n", cdata.probability);

	n = fgets(input,sizeof(input),fp);
	cdata.mean = atoi(input);

	printf("\n[INFO]Mean of rate of read from receive buffer %d\n", cdata.mean);
	
	printf("\n*************************************************************************\n");

	fclose(fp);
}

void prifinfo_plus(struct ifi_info *ifi)
{
	struct sockaddr	*sa;
	u_char		*ptr;
	int		i;

	for (;ifi != NULL; ifi = ifi->ifi_next)
	{
		printf("%s: ", ifi->ifi_name);
		if (ifi->ifi_index != 0)
			printf("(%d) ", ifi->ifi_index);
		printf("<");

		if ( (i = ifi->ifi_hlen) > 0) {
			ptr = ifi->ifi_haddr;
			do {
				printf("%s%x", (i == ifi->ifi_hlen) ? "  " : ":", *ptr++);
			} while (--i > 0);
			printf("\n");
		}
		if (ifi->ifi_mtu != 0)
			printf("  MTU: %d\n", ifi->ifi_mtu);

		if ( (sa = ifi->ifi_addr) != NULL)
			printf("  IP addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));

		//Assignment 2 modifications

		if ( (sa = ifi->ifi_ntmaddr) != NULL)
			printf("  network mask: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));



		if ( (sa = ifi->ifi_brdaddr) != NULL)
			printf("  broadcast addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));
		if ( (sa = ifi->ifi_dstaddr) != NULL)
			printf("  destination addr: %s\n",
						Sock_ntop_host(sa, sizeof(*sa)));
	}

}

/* Function that checks if the client and server are on the same network */
/* If they are, IPClient and IPServer are assigned accordingly */

/* TODO - Need to set IPClient and IPServer if they are on different network */
/* TODO - Print the information properly. Currently displaying lot of data. Decide what needs to be printed */

int check_if_local()
{
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr *ip_addr, *ntm_addr;
	struct sockaddr_in clientAddr, clientMask;
	int isLocal = 0;
	int isSameHost = 0;

	memset((char *) &IPServer, 0, sizeof(IPServer));
      	IPServer.sin_family = AF_INET;
	IPServer.sin_port = htons(cdata.server_port_no);
	IPServer.sin_addr.s_addr = inet_addr(cdata.server_ip);

     	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1); ifi != NULL; ifi = ifi->ifi_next)
	{

		if ((ip_addr = ifi->ifi_addr) != NULL)
		{
			printf("\n*************************************************************************\n");
			printf("\n[INFO]Client IP address = %s\n", Sock_ntop_host(ip_addr,sizeof(*ip_addr)));
			//printf("\nServer IP address = %s\n", Sock_ntop_host(&IPServer,sizeof(IPServer)));

			clientAddr.sin_addr.s_addr = inet_addr(Sock_ntop_host(ip_addr,sizeof(*ip_addr)));

			if(IPServer.sin_addr.s_addr == clientAddr.sin_addr.s_addr)  //  same host
			{
				printf("\n[INFO]Client and server running on same host\n");
				IPClient.sin_family = AF_INET;
				IPClient.sin_addr.s_addr = inet_addr("127.0.0.1");
				IPServer.sin_addr.s_addr = inet_addr("127.0.0.1");
				isLocal = 1;
				isSameHost = 1;
			}
			else
			{
				//printf("Client and server running on different hosts\n");
				if(isLocal == 0)
				{
					IPClient.sin_family = AF_INET;
					IPClient.sin_addr.s_addr = inet_addr(Sock_ntop_host(ip_addr,sizeof(*ip_addr)));
				}
				
			}
		}

		if((ntm_addr = ifi->ifi_ntmaddr) != NULL)
		{
			clientAddr.sin_addr.s_addr = inet_addr(Sock_ntop_host(ip_addr,sizeof(*ip_addr)));
			clientMask.sin_addr.s_addr = inet_addr(Sock_ntop_host(ntm_addr,sizeof(*ntm_addr)));

    			struct in_addr client_network;
			struct in_addr server_network;

    			// bitwise AND of ip and netmask gives the network
	    		client_network.s_addr = clientAddr.sin_addr.s_addr & clientMask.sin_addr.s_addr;
	    		server_network.s_addr = IPServer.sin_addr.s_addr & clientMask.sin_addr.s_addr;

			printf("\n[INFO]Client Network Mask = %s\n", Sock_ntop_host(ntm_addr,sizeof(*ntm_addr)));
			printf("\n[INFO]Client Network Address = %s\n", inet_ntoa(client_network));

			//printf("Network address of server = %s\n", inet_ntoa(server_network));

			if(client_network.s_addr == server_network.s_addr && isSameHost == 0)
			{
				printf("\n[INFO]Client and server running on the same network\n");
				IPClient.sin_family = AF_INET;
				IPClient.sin_addr.s_addr = clientAddr.sin_addr.s_addr;
				isLocal = 1;
			}
			else if(isSameHost == 0)
			{
				printf("\n[INFO]Client and server running on different networks\n");
				IPClient.sin_family = AF_INET;
				IPClient.sin_addr.s_addr = inet_addr(Sock_ntop_host(ip_addr,sizeof(*ip_addr)));
			}

		}

	}

	printf("\n*************************************************************************\n");
	
	return isLocal;
}

int create_socket()
{
	char str[30];
	int i;
	const int on = 1;
	int sockfd;
	int connection_port;
	int is_local;
	int flags = 0;

	// Check if the server is local. If it is, set appropriate flag
	is_local = check_if_local();

	if(is_local == 1)
	{
		printf("\n[INFO]Server is local\n");
		printf("\n*************************************************************************\n");
		flags = flags | MSG_DONTROUTE;
	}

	inet_ntop(AF_INET, &IPServer.sin_addr, str, sizeof(str));
	printf("\n[INFO]The Server IP Address selected is : %s\n",str);

	inet_ntop(AF_INET, &IPClient.sin_addr, str, sizeof(str));
	printf("\n[INFO]The Client IP Address selected is : %s\n",str);

	printf("\n*************************************************************************\n");

	// Create a socket to connect to the server
	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	// Set this option if the server is local to the client
	if(is_local == 1)
	{
		Setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));
	}

	Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	// Bind the IPClient to the socket with 0 port number
	IPClient.sin_port = htons(0);
	Bind(sockfd, (SA *) &IPClient, sizeof(IPClient));

	/* Call getsockname() to find local address bound to socket:
	   local internet address is now determined (if multihomed). */
	i = sizeof(IPClient);
	if (getsockname(sockfd, (SA *) &IPClient, &i) < 0)
	{
		printf("getsockname() error\n");
		exit(0);
	}

	printf("\n[INFO]After Bind - Local IP Address : %s, Local Port no: %d\n", inet_ntoa(IPClient.sin_addr), ntohs(IPClient.sin_port));

	Connect(sockfd, (SA*)&IPServer, sizeof(IPServer));

	i = sizeof(IPServer);
	if (getpeername(sockfd, (SA *) &IPServer, &i) < 0)
	{
		printf("getpeername() error\n");
		exit(0);
	}

	printf("\n[INFO]After Connect - Server IP Address : %s, Server Port no: %d\n", inet_ntoa(IPServer.sin_addr), ntohs(IPServer.sin_port));

	/* Get the connection port from the server */
	connection_port = getConnectionPort(sockfd, (SA *) &IPServer, sizeof(IPServer), flags); 

	printf("\n*************************************************************************\n");
          
	/* Reconnect the client to the new port number */
	
      	IPServer.sin_family = AF_INET;
	IPServer.sin_addr.s_addr = inet_addr(cdata.server_ip);
	IPServer.sin_port = htons(connection_port);

	Connect(sockfd, (SA*)&IPServer, sizeof(IPServer));

	return sockfd;
}

void receive_file(int sockfd)
{
	char str[30];
	const int on = 1;
	int flags = 0;
	int sequence_number = 0;
	size_t n = 1;

	// Send the initial window size to the server
	send_packet.window_size = cdata.sliding_window_size;
	Send(sockfd, &send_packet, sizeof(send_packet), 0);

	// Receive the packets in a loop and send ACK
	while(n > 0)
	{
		n = Recv(sockfd, &recv_packet, sizeof(recv_packet), 0);
		
		add_packet_to_buffer(&recv_packet);

		read_packet_from_buffer();

		// Send ACK to the server
		send_packet.seq = sequence_number;
		send_packet.last_flag = recv_packet.last_flag;
		send_packet.window_size = current_window_size;
		send_packet.ack = recv_packet.seq + 1;

		Send(sockfd, &send_packet, sizeof(send_packet), flags);

		if(recv_packet.last_flag == 1)
		{
			printf("\n[INFO]Last flag set\n");
			break;		
		} 
	}

}

int getConnectionPort(int sockfd, const SA *pservaddr, socklen_t servlen, int flags)
{
	int     n;
	char    recvline[MAXLINE + 1];

	Signal(SIGALRM, sig_alrm);

	while (1) {

		// Send ACK to the server with the file name to read
		send_packet.seq = 0;
		send_packet.last_flag = 0;
		send_packet.window_size = current_window_size;
		send_packet.ack = 1;
		strncpy(send_packet.data_buffer,  cdata.file_name, sizeof(cdata.file_name));

		Sendto(sockfd, &send_packet, sizeof(send_packet), flags, pservaddr, servlen);
		alarm(5);

		if ((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) < 0) {
        		if (errno == EINTR)
        		{
                		fprintf(stderr, "socket timeout\n");
                	}
        		else
        		{
                		err_sys("recvfrom error");
                		return -1;
                	}
		} 
		else {
        		alarm(0);
        		recvline[n] = 0;        /* null terminate */
        		printf("\n[INFO]Connection port of the server = %s\n", recvline);
        		return atoi(recvline);
		        break;
		}
	}
}

static void sig_alrm(int signo)
{
        return;                 /* just interrupt the recvfrom() */
}
