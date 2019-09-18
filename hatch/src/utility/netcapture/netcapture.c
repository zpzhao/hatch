#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <errno.h>
#include <stdio.h>


#define UXDB 1
#define BUFFER_MAX 2048
#define SQL_BUFFER_MAX 2048

#define  LOGI printf

typedef int int32;
typedef unsigned int u_int32;
typedef unsigned char u_char;
typedef unsigned short u_short;

typedef struct mac_frm_hdr
{
	char dest_addr[6];	//destination MAC address shall be defined first.
	char src_addr[6];
	short type;
}__attribute__((packed)) MAC_FRM_HDR;

typedef struct ip_hdr
{ 	//header of IPV4
#ifdef __LITTLE_ENDIAN_BIFIELD
u_char ip_len:4, ip_ver:4;
#else
	u_char ip_ver :4, ip_len :4;
#endif

	u_char ip_tos;
	u_short ip_total_len;
	u_short ip_id;
	u_short ip_flags;
	u_char ip_ttl;
	u_char ip_protocol;
	u_short ip_chksum;
	u_int32 ip_src;
	u_int32 ip_dest;
}__attribute__((packed)) IP_HDR;


/* UDP header*/
typedef struct udp_header{
    u_short s_port;          // Source port
    u_short d_port;          // Destination port
    u_short len;            // Datagram length
    u_short crc;            // Checksum
}__attribute__((packed)) udp_header;


typedef int (*PGETPACKSQL)(char *buf, int len);
PGETPACKSQL pgetPackSql = NULL;
char sql_buf[SQL_BUFFER_MAX];
unsigned long long id = 0;

#ifdef UXDB

#include <stdio.h>
#include <stdlib.h>
#include "libuxsql-fe.h"


const char *conninfo;
UXconn	   *conn;


static void
exit_nicely()
{
	UXSQLfinish(conn);
	exit(1);
}

/**
 * insert into table about packet infomation.
 *
 */
int addPackInfo(char *buf, int len)
{
	UXresult   *res;
	if(NULL == buf)
		return -1;

	res = UXSQLexec(conn, buf);
	if (UXSQLresultStatus(res) != UXRES_COMMAND_OK)
	{
		fprintf(stderr, "excute failure: %s", UXSQLerrorMessage(conn));
		UXSQLclear(res);
		exit_nicely(conn);
	}
	return 0;
}


int startCapture(void);

int
main(int argc, char **argv)
{
	/*
	 * If the user supplies a parameter on the command line, use it as the
	 * conninfo string; otherwise default to setting dbname=uxdb and using
	 * environment variables or defaults for all other connection parameters.
	 */
	if (argc > 1)
		conninfo = argv[1];
	else
		conninfo = "dbname = uxdb";

	/* Make a connection to the database */
	conn = UXSQLconnectdb(conninfo);

	/* Check to see that the backend connection was successfully made */
	if (UXSQLstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s",
				UXSQLerrorMessage(conn));
		exit_nicely(conn);
	}

	/*
	 * Our test case here involves using a cursor, for which we must be inside
	 * a transaction block.  We could do the whole thing with a single
	 * UXSQLexec() of "select * from ux_database", but that's too trivial to make
	 * a good example.
	 */
	pgetPackSql = addPackInfo;
	(void)startCapture();

	/* close the connection to the database and cleanup */
	UXSQLfinish(conn);

	return 0;
}

#endif


/**
 * modified to so
 */
#ifdef UXDB
int startCapture(void)
#else
int main(int argc, char*argv[])
#endif
{
	int SOCKET_SRC;
	char buf[BUFFER_MAX];
	int n_rd;

	if ((SOCKET_SRC = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0)
	{
		fprintf(stderr, "create socket error.\n");
		exit(0);
	}
	while (1)
	{
		n_rd = recvfrom(SOCKET_SRC, buf, BUFFER_MAX, 0, NULL, NULL);
		if (n_rd < 46)
		{
			perror("recvfrom():");
			printf("Incomplete packet (errno is %d)\n", errno);
			close(SOCKET_SRC);
			exit(0);
		}
		/* An Ethernet frame was written to buf, frame analysis can be processed here */
		/* Termination control */
		procPacket(buf);
	}
	close(SOCKET_SRC);
	return 0;
}

void procPacket(char *buf)
{
	MAC_FRM_HDR *mac_hdr; //define a Ethernet frame header
	IP_HDR *ip_hdr;       //define a IP header
	udp_header *udp_hdr;
	char *tmp1, *tmp2;
	int AND_LOGIC = 0xFF;

	mac_hdr = (MAC_FRM_HDR *)buf;	//buf is what we got from the socket program
	ip_hdr = (IP_HDR *)(buf + sizeof(MAC_FRM_HDR));
	udp_hdr = (udp_header *)(buf + sizeof(MAC_FRM_HDR) + sizeof(IP_HDR)); //if we want to analyses the UDP/TCP

	tmp1 = mac_hdr->src_addr;
	tmp2 = mac_hdr->dest_addr;
	/* print the MAC addresses of source and receiving host */
	printf(
			"MAC: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X==>" "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
			tmp1[0] & AND_LOGIC, tmp1[1] & AND_LOGIC, tmp1[2] & AND_LOGIC,
			tmp1[3] & AND_LOGIC, tmp1[4] & AND_LOGIC, tmp1[5] & AND_LOGIC,
			tmp2[0] & AND_LOGIC, tmp2[1] & AND_LOGIC, tmp2[2] & AND_LOGIC,
			tmp2[3] & AND_LOGIC, tmp2[4] & AND_LOGIC, tmp2[5] & AND_LOGIC);
	if(NULL != pgetPackSql)
	{
		snprintf(sql_buf, SQL_BUFFER_MAX, "insert into tbl_mac_packet values("
								"%u, "
								"'%.2X:%.2X:%.2X:%.2X:%.2X:%.2X',"
								"'%.2X:%.2X:%.2X:%.2X:%.2X:%.2X', now());",
								++id,
								tmp1[0] & AND_LOGIC, tmp1[1] & AND_LOGIC, tmp1[2] & AND_LOGIC,
											tmp1[3] & AND_LOGIC, tmp1[4] & AND_LOGIC, tmp1[5] & AND_LOGIC,
											tmp2[0] & AND_LOGIC, tmp2[1] & AND_LOGIC, tmp2[2] & AND_LOGIC,
											tmp2[3] & AND_LOGIC, tmp2[4] & AND_LOGIC, tmp2[5] & AND_LOGIC);
		pgetPackSql(sql_buf, SQL_BUFFER_MAX);
	}


	tmp1 = (char*) &ip_hdr->ip_src;
	tmp2 = (char*) &ip_hdr->ip_dest;
	/* print the IP addresses of source and receiving host */
	printf("IP: %d.%d.%d.%d => %d.%d.%d.%d", tmp1[0] & AND_LOGIC,
			tmp1[1] & AND_LOGIC, tmp1[2] & AND_LOGIC, tmp1[3] & AND_LOGIC,
			tmp2[0] & AND_LOGIC, tmp2[1] & AND_LOGIC, tmp2[2] & AND_LOGIC,
			tmp2[3] & AND_LOGIC);
	if (NULL != pgetPackSql)
	{
		snprintf(sql_buf, SQL_BUFFER_MAX, "insert into tbl_ip_packet values("
				"%u, '%s',"
				"'%d.%d.%d.%d',"
				"'%d.%d.%d.%d');",
				id,
				ip_hdr->ip_protocol == IPPROTO_ICMP? "ICMP" :
						ip_hdr->ip_protocol == IPPROTO_IGMP ? "IGMP" :
								ip_hdr->ip_protocol == IPPROTO_IPIP ? "IPIP" :
										ip_hdr->ip_protocol == IPPROTO_TCP ? "TCP" :
												ip_hdr->ip_protocol == IPPROTO_UDP ? "UDP" :
														ip_hdr->ip_protocol == IPPROTO_RAW ? "RAW" : "Unknown",
				tmp2[0] & AND_LOGIC, tmp2[1] & AND_LOGIC, tmp2[2] & AND_LOGIC,tmp2[3] & AND_LOGIC,
				tmp1[0] & AND_LOGIC,tmp1[1] & AND_LOGIC, tmp1[2] & AND_LOGIC, tmp1[3] & AND_LOGIC);
		pgetPackSql(sql_buf, SQL_BUFFER_MAX);
	}

	/* print the IP protocol which was used by the socket communication */
	switch (ip_hdr->ip_protocol)
	{
	case IPPROTO_ICMP:
		LOGI("ICMP");
		break;
	case IPPROTO_IGMP:
		LOGI("IGMP");
		break;
	case IPPROTO_IPIP:
		LOGI("IPIP");
		break;
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		LOGI("Protocol: %s",
				ip_hdr->ip_protocol == IPPROTO_TCP ? "TCP" : "UDP");
		LOGI("Source port: %u, destination port: %u", udp_hdr->s_port,
				udp_hdr->d_port);


		if (NULL != pgetPackSql)
			{
				snprintf(sql_buf, SQL_BUFFER_MAX, "insert into tbl_tcp_packet values("
						"%u, "
						"'%u',"
						"'%u');",
						id,
						udp_hdr->d_port, udp_hdr->s_port);
				pgetPackSql(sql_buf, SQL_BUFFER_MAX);
			}
		break;
	case IPPROTO_RAW:
		LOGI("RAW");
		break;
	default:
		printf("Unknown, please query in inclued/linux/in.h\n");
		break;
	}
}



