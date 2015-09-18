/*******************************************************************************
*
* File Name         : icmp_datasender_client.c
* Created By        : Alexandre DUPUY
* Creation Date     : August 31sth, 2015
* Last Change       : September 18th, 2015 at 04:19:02 PM
* Last Changed By   : Alexandre DUPUY
* Purpose           : Sending data through ICMP packages (client side)
*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#define PACKET_SIZE	128
struct packet
{
	struct icmphdr hdr;
	char msg[PACKET_SIZE-sizeof(struct icmphdr)];
};

int pid=-1;
struct protoent *proto=NULL;

/*--------------------------------------------------------------------*/
/*--- checksum - standard 1s complement checksum                   ---*/
/*--------------------------------------------------------------------*/
unsigned short checksum(void *b, int len)
{	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

/*--------------------------------------------------------------------*/
/*--- listener - separate process to listen for and collect messages--*/
/*--------------------------------------------------------------------*/
void listener(void)
{	int sd;
	struct sockaddr_in addr;
	unsigned char buf[1024];

	sd = socket(AF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		perror("socket");
		exit(0);
	}
	for (;;)
		{	int bytes, len=sizeof(addr);

			bzero(buf, sizeof(buf));
			bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len);
			if ( bytes > 0 )
				printf("***Got Reply***\n");
			else
				perror("recvfrom");
		}
		exit(0);
}



/*--------------------------------------------------------------------*/
/*--- ping - Create message and send it.                           ---*/
/*--------------------------------------------------------------------*/
int my_ping(struct sockaddr_in *addr, char *pkt, int cnt)
{	
	const int val=255;
	int i, u, sd;
	struct packet pckt;
	struct sockaddr_in r_addr;

	sd = socket(AF_INET, SOCK_RAW, proto->p_proto);


	if ( sd < 0 )
		return 1;
	if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		return 2;
	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
		return 3;
	int len=sizeof(r_addr);
	bzero(&pckt, sizeof(pckt));
	pckt.hdr.type = ICMP_ECHO;
	pckt.hdr.un.echo.id = pid;
	 for(i = 0; pkt[i]; i++){
	 	pckt.msg[i] = pkt[i];
	}
	u = i;
	for ( i = u; i < sizeof(pckt.msg); i++ )
		pckt.msg[i] = '0';
	//pckt.msg[i] = 0;
	pckt.hdr.un.echo.sequence = cnt++;
	pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
	if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
		return 4;
	return 0;
}

/*--------------------------------------------------------------------*/
/*--- ping - Create message and send it.                           ---*/
/*--------------------------------------------------------------------*/
int sending(struct sockaddr_in *addr, char* folderPath)
{	
	int status, packetSent = 0, nbTries = 0;
	FILE *entry_file;
	char buffer[BUFSIZ];
	struct dirent **namelist;
 	char pathFile[PATH_MAX + 1];
	int i,n;
	char pkt[2048];
	char *pktTemp;
	int nbPkts = 1;


	/* Scanning the in directory */
	n = scandir(folderPath, &namelist, 0, alphasort);
	printf("n == %d \n", n);
	if (n < 0){
		fprintf(stderr, "Scan directory can not be performed\n");
		return(3);
	}else {
		for (i = 0; i < n; i++) {


			/* On linux/Unix we don't want current and parent directories
			*          * On windows machine too, thanks Greg Hewgill
			*                   */
			if (!strcmp (namelist[i]->d_name, ".") || !strcmp (namelist[i]->d_name, ".."))
				continue;
			pathFile[0] = '\0';
			strncat(pathFile, folderPath,PATH_MAX);
			strncat(pathFile, "/",PATH_MAX);
			strncat(pathFile, namelist[i]->d_name, PATH_MAX);
			printf("\n----------------\n");
			printf("File path read: %s \n", pathFile);
			entry_file = fopen(pathFile, "r");
			if (entry_file == NULL)
			{
				fprintf(stderr, "Failed to open entry file \n");
				return(3);
			}

			pkt[0] = '\0';
			packetSent = 0;
			while((pktTemp = fgets(buffer, BUFSIZ, entry_file)) != NULL){
				strncat(pkt,pktTemp, 2048);
			}
			printf("Sending Packet %d with message : \"%s\" \n", nbPkts, pkt);
			printf("----------------\n");
			while(packetSent == 0 && nbTries < 3){
				if((status = my_ping(addr, pkt, nbPkts)) == 0){
					packetSent = 1;
					nbPkts++;
				}else {

					nbTries++;
				}
				usleep(50000);
			}
			if(packetSent == 0){
				fprintf(stderr, "Connection has been lost, check if the host is alived\n");
			}
			/* When you finish with the file, close it */
			fclose(entry_file);
			free(namelist[i]);
		}
		free(namelist);
	}	
	return 0;
}

/*--------------------------------------------------------------------*/
/*--- main - look up host and start ping processes.                ---*/
/*--------------------------------------------------------------------*/
int main(int argc, char *argv[])
{	
	struct hostent *hname;
	struct sockaddr_in addr;
	if ( argc != 3 )
	{
		printf("usage: %s <addr>\n", argv[0]);
		exit(0);
	}
	if ( argc > 2 )
	{
		pid = getpid();
		proto = getprotobyname("ICMP");
		hname = gethostbyname(argv[1]);
		bzero(&addr, sizeof(addr));
		addr.sin_family = hname->h_addrtype;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = *(long*)hname->h_addr;
		if ( fork() == 0 )
			listener();
		else
			sending(&addr, argv[2]);
			//my_ping(&addr, argv[2]);
		wait(0);
	}
	else
		printf("Usage: ./bin <hostname> <folderPath>\n");
	return 0;
}