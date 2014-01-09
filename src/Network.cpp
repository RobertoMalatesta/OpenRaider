/* -*- Mode: C++; tab-width: 3; indent-tabs-mode: t; c-basic-offset: 3 -*- */
/*================================================================
 *
 * Project : UnRaider
 * Author  : Terry 'Mongoose' Hendrix II
 * Website : http://www.westga.edu/~stu7440/
 * Email   : stu7440@westga.edu
 * Object  : Network
 * License : No use w/o permission (C) 2002 Mongoose
 * Comments:
 *
 *
 *           This file was generated using Mongoose's C++
 *           template generator script.  <stu7440@westga.edu>
 *
 *-- History -------------------------------------------------
 *
 * 2002.06.21:
 * Mongoose - Created
 =================================================================*/

#include <Network.h>

#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>

//#define LOCAL_BCAST
#define NETWORK_RELIABLE
#define MAX_CLIENTS 32

typedef struct client_s
{
	unsigned int uid;
	char active;
	unsigned int seq;
	unsigned int frameExpected;

} client_t;


#ifdef USING_PTHREADS
#   include <pthread.h>
pthread_t gPThreadId[3];
#endif

unsigned int gUID;
client_t gClients[MAX_CLIENTS];
unsigned int gNumClients = 0;
network_frame_t gPiggyBack;


////////////////////////////////////////////////////////////
// Constructors
////////////////////////////////////////////////////////////

Network *Network::mInstance = 0x0;


Network *Network::Instance()
{
	if (mInstance == 0x0)
	{
		mInstance = new Network();
	}

	return mInstance;
}


void killNetworkSingleton()
{
	printf("Shutting down Network...\n");

	// Requires public deconstructor
	delete Network::Instance();
}


Network::Network()
{
	strncpy(mRemoteHost, "localhost", REMOTE_HOST_STR_SZ);
	memset(mBindHost, 0, BIND_HOST_STR_SZ);
	setPort(8080);

	mPiggyBack = true;
	mNetworkReliable = true;
	mSpawnedClient = false;
	mSpawnedServer = false;
	mKillClient = false;
	mKillServer = false;
	mDebug = false;

	gUID = getUID();

	printf("UID %u\n", gUID);

	for (gNumClients = MAX_CLIENTS; gNumClients > 0;)
	{
		--gNumClients;

		gClients[gNumClients].active = 0;
		gClients[gNumClients].uid = 0;
		gClients[gNumClients].seq = 0;
	}
}


Network::~Network()
{
	killServerThread();
	killClientThread();
}


////////////////////////////////////////////////////////////
// Public Accessors
////////////////////////////////////////////////////////////

network_frame_t &Network::getPiggyBack()
{
	return gPiggyBack;
}


unsigned int Network::getUID()
{
	struct timeval tv;
	struct timezone tz;


	gettimeofday(&tv, &tz);

	srand(tv.tv_usec);

	return ((unsigned int)(tv.tv_sec * getRandom(2.0, 3.3) -
								  tv.tv_sec * getRandom(1.0, 2.0)) +
			  (unsigned int)(tv.tv_usec * getRandom(2.0, 3.3) -
								  tv.tv_usec * getRandom(1.0, 2.0)) +
			  (unsigned int)getRandom(666.0, 5000.0));
}


float Network::getRandom(float from, float to)
{
	return from + (to*rand()/(RAND_MAX+1.0));
}


int Network::getPort()
{
	return mPort;
}


////////////////////////////////////////////////////////////
// Public Mutators
////////////////////////////////////////////////////////////

void *client_thread(void *v)
{
	Network &network = *Network::Instance();
	network.runClient();

	return NULL;
}


void *server_thread(void *v)
{
	Network &network = *Network::Instance();
	network.runServer();

	return NULL;
}


void Network::setBindHost(char *s)
{
	if (!s && !s[0])
		return;

	strncpy(mBindHost, s, BIND_HOST_STR_SZ);
}


void Network::setRemoteHost(char *s)
{
	if (!s && !s[0])
		return;

	strncpy(mRemoteHost, s, REMOTE_HOST_STR_SZ);
}


void Network::setDebug(bool toggle)
{
	mDebug = toggle;
}


void Network::setPort(unsigned int port)
{
	mPort = port;
}


void Network::killServerThread()
{
	mKillServer = true;

	// Remember for mutex
	//	while (mKillServer)
	//	{
	//	}

	mSpawnedServer = false;
}


void Network::killClientThread()
{
	mKillClient = true;

	// Remember for mutex
	//	while (mKillClient)
	//	{
	//	}

	mSpawnedClient = false;
}


void Network::spawnServerThread()
{
	// For now don't handle shutting down server to start client and vv
	if (!mSpawnedServer && !mSpawnedClient)
	{
#ifdef USING_PTHREADS
		pthread_create(gPThreadId + 0, 0, server_thread, NULL);
#else
		printf("Network::spawnServerThread> Build doesn't support threads\n");
#endif

		mSpawnedServer = true;
	}
}


void Network::spawnClientThread()
{
	// For now don't handle shutting down server to start client and vv
	if (!mSpawnedServer && !mSpawnedClient)
	{
#ifdef USING_PTHREADS
		pthread_create(gPThreadId + 1, 0, client_thread, NULL);
#else
		printf("Network::spawnClientThread> Build doesn't support threads\n");
#endif

		mSpawnedClient = true;
	}
}


////////////////////////////////////////////////////////////
// Protected Mutators
////////////////////////////////////////////////////////////

int Network::runServer()
{
	unsigned int fsize;
 	int socket_fd, cc, cip;
	struct sockaddr_in s_in, from;
	char hostid[64];
	network_frame_t f;
	unsigned int i;
	unsigned int packetsRecieved = 0;


	socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (socket_fd < 0)
	{
		perror("recv_udp:socket");
		return -1;
	}

	if (mBindHost[0])
	{
		strncpy(hostid, mBindHost, 64);
	}
	else
	{
		if (gethostname(hostid, 64) < 0)
		{
			perror("Server: recv_udp:gethostname");
			return -1;
		}

		printf("Server: gethostname returned '%s'\n", hostid);
		fflush(stdout);
	}

	// Setup for port binding
	memset(&s_in, 0, sizeof(s_in));
	s_in.sin_family = AF_INET;
#ifdef LOCAL_BCAST
	struct hostent *hostptr;

	if ((hostptr = gethostbyname(hostid)) == NULL)
	{
		fprintf(stderr, "Server: recv_udp, Invalid host name '%s'\n", hostid);
		return -1;
	}

	memcpy((void *)(&s_in.sin_addr), hostptr->h_addr, hostptr->h_length);
#else
	s_in.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
	int port = getPort();
	s_in.sin_port = htons(port); // htons new

	fflush(stdout);

	// Bind
	while (bind(socket_fd, (struct sockaddr *)&s_in, sizeof(s_in)) < 0)
	{
		if (s_in.sin_port++ > (port + 10))
		{
			perror("Server: recv_udp:bind exhausted");
			return -1;
		}
	}

	cip = ntohl(s_in.sin_addr.s_addr);

	printf("Server: Started on ( %i.%i.%i.%i:%i )\n",
			 cip >> 24, cip << 8 >> 24,
			 cip << 16 >> 24, cip << 24 >> 24, s_in.sin_port);

	for (; !mKillClient;)
	{
		fsize = sizeof(from);

		// 1. Wait for event
		// 2. Get inbound frame
		cc = recvfrom(socket_fd, &f, sizeof(network_frame_t), 0,
						  (struct sockaddr *)&from, &fsize);

		if (cc < 0)
		{
			perror("Server: recv_udp:recvfrom");
			continue;
		}

		++packetsRecieved;

		if (mDebug)
		{
			printf("=====================================================\n");
			printf("Packet %i\n", packetsRecieved);
			printf("Server: Recieved packet from %u\n",
					 f.uid);
		}

		// A. Look and see if this client has connected before
		for (i = 0; i < gNumClients; ++i)
		{
			if (gClients[i].uid == f.uid)
			{
				break;
			}
		}

		// B. Collect client data if it's a new connection
		if (!gClients[i].active)
		{
			for (i = 0; i < gNumClients+1; ++i)
			{
				if ((i + 1) < MAX_CLIENTS && !gClients[i].active)
				{
					gClients[i].uid = f.uid;
					gClients[i].active = 1;
					gClients[i].frameExpected = 0;
					++gNumClients;

					printf("Server: %u made connection, as client %i\n",
							 gClients[i].uid, i);
					break;
				}
			}

			if (i == MAX_CLIENTS || !gClients[i].active)
			{
				if (mDebug)
				{
					printf("Server: Handshake packet from %u failed?\n",
							 f.uid);
				}
				continue;
			}
		}

		cip = ntohl(from.sin_addr.s_addr);

		if (mDebug)
		{
			printf("Server: Client (Famliy %d, Address %i.%i.%i.%i:%d)\n",
					 ntohs(from.sin_family), cip >> 24, cip << 8 >> 24,
					 cip << 16 >> 24, cip << 24 >> 24,
					 ntohs(from.sin_port));

			printf("Server: Datalink layer recieved: packet seq %i\n",
					 f.seq);
		}

		if (mNetworkReliable)
		{
			if (f.seq == gClients[i].seq)
			{
				if (mDebug)
				{
					printf("SERVER> Msg from %u\n", f.uid);
				}

				to_network_layer(f.data);
				gClients[i].seq = f.seq;
			}
			else
			{
				continue;
			}
		}

		// FIXME: Combine with above, duh
		// 3. Send to network layer
		if (gClients[i].frameExpected == f.header)
		{
			f.data.cid = i;

			to_network_layer(f.data);
			gClients[i].frameExpected = !gClients[i].frameExpected;
		}

		fflush(stdout);

#ifdef UNIT_TEST_NETWORK
		if ((rand() % 10 == 0))
		{
			printf("Server: Simulating a lost ack %i\n", f.seq);
			continue;
		}
#endif

		// 4. Send ACK, w/ piggyback if requested
		if (mPiggyBack)
		{
			gPiggyBack.header = 0;
			gPiggyBack.seq = f.seq;
			gPiggyBack.uid = gUID;

			if (mDebug)
			{
				printf("SERVER> Sending data by piggyback\n");
			}

			cc = sendto(socket_fd, &gPiggyBack, sizeof(gPiggyBack), 0,
							(struct sockaddr *)&from, sizeof(from));
		}
		else
		{
			f.header = 0;
			f.seq = 0;
			f.uid = gUID;

			cc = sendto(socket_fd, &f, sizeof(f), 0,
							(struct sockaddr *)&from, sizeof(from));
		}

		if (cc < 0)
		{
			perror("Server: send_udp:sendto");
		}
		else
		{
			if (mDebug)
			{
				printf("Server: Ack sent to %u\n", gClients[i].uid);
			}
		}
	}

	mKillClient = false;

	return 0;
}


void Network::runClient()
{
	unsigned int fsize, last_frame_sent = 0;
	int socket_fd, cc, done;
	struct sockaddr_in dest;
	struct hostent *hostptr;
	network_frame_t f;
	struct timeval timeout;
	fd_set  readfds;
	unsigned int packetsSent = 0;
	unsigned int seq = 0;
	char timedOut = 1;

	if (!mRemoteHost || !mRemoteHost[0])
	{
		return;
	}

	memset((char*) &timeout, 0, sizeof(timeout));
	timeout.tv_sec = 5;

	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (socket_fd == -1)
	{
		perror("Client: send_udp: socket");
		exit(0);
	}

	if ((hostptr = gethostbyname(mRemoteHost)) == NULL)
	{
		fprintf(stderr, "Client: send_udp: invalid host name, %s\n",
				  mRemoteHost);
		exit(0);
	}

	// Setup connection
	bzero((char*) &dest, sizeof(dest));
	dest.sin_family = AF_INET;
	int port = getPort();
	dest.sin_port = htons(port);
#ifdef LOCAL_BCAST
	memcpy(hostptr->h_addr, (char *) &dest.sin_addr, hostptr->h_length);
#else
	if (inet_pton(AF_INET, mRemoteHost, &dest.sin_addr) < 0)
	{
		perror("inet_pton");
		return;
	}
#endif


	// init
	f.data.send = 0;
	f.seq = 0;

	for (; !mKillServer;)
	{
		++packetsSent;

		if (mDebug)
		{
			printf("=====================================================\n");
			printf("Packet %i\n", packetsSent);
		}

		// 1. Get packet to send over wire
		if (mNetworkReliable && timedOut && f.seq != seq)
		{
			if (mDebug)
			{
				printf("Client: Resending packet\n");
			}
		}
		else
		{
			from_network_layer(&f.data, &last_frame_sent);

			if (!f.data.send)
			{
				usleep(20);
				continue;
			}
		}

		// 2. Copy to frame
		f.seq = 0;//seq;  // 0 forces all packets to check out
		f.uid = gUID;

		// 3. Send over the wire
		done = 0;
		timedOut = 0;

		while (!done)
		{
			if (mDebug)
			{
				printf("Client: Sending packet %i\n", f.seq);
			}

			cc = sendto(socket_fd, &f, sizeof(f), 0,
							(struct sockaddr *)&dest, sizeof(dest));

			if (cc < 0)
			{
				perror("Client: send_udp:sendto");

				if (errno == EMSGSIZE)
				{
					printf("Client: packet was too large\n");
				}
			}
			else
			{
				f.data.send = 0;
			}

			// Comment out this to enable more reliable service
			done = 1;
		}

		// 4. Wait for +ack or resend
		FD_ZERO(&readfds);

		// Setup socket to listen on here
		FD_SET(socket_fd, &readfds);

		// Set timeout in milliseconds
		timeout.tv_usec = 850;

		cc = select(socket_fd + 1, &readfds, NULL, NULL, &timeout);

		if ((cc < 0) && (errno != EINTR))
		{
			// there was an local error with select
		}

		if (cc == 0)
		{
			if (mDebug)
			{
				printf("Client: Timeout detected on packet %i\n", f.seq);
			}
			timedOut = 1;
			continue;
		}

		// Clear header for recv use
		f.header = 0;

		fsize = sizeof(dest);
		cc = recvfrom(socket_fd, &f, sizeof(f), 0,
						  (struct sockaddr *)&dest, &fsize);

		if (cc < 0)
		{
			perror("Client: recv_udp:recvfrom");
		}
		else
		{
			if (mDebug)
			{
				printf("Client: Datalink layer recieved: packet seq %i\n", f.seq);
				printf("CLIENT> Msg from %u\n", f.uid);
			}

			to_network_layer(f.data);
		}

		if (seq == f.seq)
		{
			if (mDebug)
			{
				printf("Client: Recieved ack %i\n", f.seq);
			}

			++seq;
		}
	}

	mKillServer = false;
}


////////////////////////////////////////////////////////////
// Private Accessors
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// Private Mutators
////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////
// Unit Test code
////////////////////////////////////////////////////////////

#ifdef UNIT_TEST_NETWORK
void from_network_layer(network_packet_t *p, unsigned int *last_id)
{
	static unsigned int i = 0;


	if (!p)
	{
		return;
	}

	*last_id = i++;

	sleep(1);

	p->send = 1;
	p->pos[0] = i*3;
	p->pos[1] = i*3+1;
	p->pos[2] = i*3+2;

	printf("<S>ending { %f %f %f }\n", p->pos[0], p->pos[1], p->pos[2]);
}


void to_network_layer(network_packet_t p)
{
	printf("<R>ecieved { %f %f %f }\n", p.pos[0], p.pos[1], p.pos[2]);

	gPiggyBack.data.pos[0] = gPiggyBack.seq*4;
	gPiggyBack.data.pos[1] = gPiggyBack.seq*4+1;
	gPiggyBack.data.pos[2] = gPiggyBack.seq*4+2;
	gPiggyBack.data.send = 1;
	gPiggyBack.data.yaw = 90.0f;
}


int main(int argc, char *argv[])
{
	printf("\n\n[Network class test]\n");
	Network &test = *Network::Instance();


	if (argc > 3)
	{
		if (argv[1][1] == 'v')
		{
			test.setDebug(true);
		}

		switch (argv[1][0])
		{
		case 'c':
			test.setRemoteHost(argv[2]);
			test.setPort(atoi(argv[3]));
			test.runClient();
			break;
		case 's':
			test.setBindHost(argv[2]);
			test.setPort(atoi(argv[3]));
			test.runServer();
			break;
		default:
			printf("Error in command line, run %s for help\n", argv[0]);
		}
	}
	else if (argc > 2)
	{
		test.setPort(atoi(argv[2]));
		test.runServer();
	}
	else
	{
		printf("Server: %s s [bind_host_name] port\n", argv[0]);
		printf("Client: %s c remote_host_name remote_host_port\n", argv[0]);
		printf("Append 'v' behind c/s option for verbose. eg cv\n");
	}

	killNetworkSingleton();

	return 0;
}
#endif