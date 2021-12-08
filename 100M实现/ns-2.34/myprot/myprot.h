#ifndef  __myprot_h__
#define  __myprot_h__


#include <agent.h>
#include <packet.h>
#include <sys/types.h>
#include <scheduler.h>

#include <cmu-trace.h>
#include <priqueue.h>
#include <classifier/classifier-port.h>
#include <myprot/myprotpkt.h>
#include <myprot/myprot_rtable.h>
#include <myprot/myprot_rqueue.h>
#include "mobilenode.h"
#include "node.h"

class MYPROT;




#define FRAME_COUNT_TIME        0.01

#define MY_ROUTE_TIMEOUT        10                      	// 100 seconds
#define ACTIVE_ROUTE_TIMEOUT    10				// 50 seconds
#define REV_ROUTE_LIFE          6				// 5  seconds
#define BCAST_ID_SAVE           6				// 3 seconds


// No. of times to do network-wide search before timing out for 
// MAX_RREQ_TIMEOUT sec. 
#define RREQ_RETRIES            3                               //这些玩意怎么确定啊
// timeout after doing network-wide search RREQ_RETRIES times
#define MAX_RREQ_TIMEOUT	10.0 //sec    这个有点太长了吧？？？？？？？？？？？？

/* Various constants used for the expanding ring search */
#define TTL_START     3
#define TTL_THRESHOLD 7
#define TTL_INCREMENT 1 

// This should be somewhat related to arp timeout
#define NODE_TRAVERSAL_TIME     0.03             // 30 ms
#define LOCAL_REPAIR_WAIT_TIME  0.15 //sec

// Should be set by the user using best guess (conservative) 
#define NETWORK_DIAMETER        8             // 30 hops

// Must be larger than the time difference between a node propagates a route 
// request and gets the route reply back.

//#define RREP_WAIT_TIME     (3 * NODE_TRAVERSAL_TIME * NETWORK_DIAMETER) // ms
//#define RREP_WAIT_TIME     (2 * REV_ROUTE_LIFE)  // seconds
#define RREP_WAIT_TIME         1.0  // sec

#define ID_NOT_FOUND    0x00
#define ID_FOUND        0x01
//#define INFINITY        0xff

// The followings are used for the forward() function. Controls pacing.
#define DELAY 1.0           // random delay
#define NO_DELAY -1.0       // no delay 

// think it should be 30 ms
#define ARP_DELAY 0.01      // fixed delay to keep arp happy


#define HELLO_INTERVAL          1               // 1000 ms
#define ALLOWED_HELLO_LOSS      3               // packets
#define BAD_LINK_LIFETIME       3               // 3000 ms
#define MaxHelloInterval        (1.25 * HELLO_INTERVAL)
#define MinHelloInterval        (0.75 * HELLO_INTERVAL)





typedef struct Point{
	double x;
	double y;
}POINT;



/*use for purge nb entry*/
class MyBroadcastTimer : public Handler {
public:
        MyBroadcastTimer(MYPROT* a) : agent(a) {}
        void	handle(Event*);
private:
        MYPROT    *agent;
	Event	intr;
};
class MyHelloTimer : public Handler {
public:
        MyHelloTimer(MYPROT* a) : agent(a) {}
        void	handle(Event*);
private:
        MYPROT    *agent;
	Event	intr;
};

class MyNeighborTimer : public Handler {
public:
        MyNeighborTimer(MYPROT* a) : agent(a) {}
        void	handle(Event*);
private:
        MYPROT    *agent;
	Event	intr;
};
/*use for purge route entry*/
class MyRouteCacheTimer : public Handler {
public:
        MyRouteCacheTimer(MYPROT* a) : agent(a) {}
        void	handle(Event*);
private:
        MYPROT    *agent;
	Event	intr;
};




class MYPROT : public  Agent {
        
        friend class myprot_rt_entry;
        friend class myprot_head_rt_entry;
        friend class MyBroadcastTimer;
        //friend class MyHelloTimer;
        friend class MyNeighborTimer;
        friend class MyRouteCacheTimer;
public:
        MYPROT(nsaddr_t id);

        void		recv(Packet *p, Handler *);
 protected:
        int             command(int, const char *const *);
        int             initialized() { return 1 && target_; }

        /*
         * Route Table Management
         */
        void            rt_resolve(Packet *p);
        void            rt_update(myprot_rt_entry *rt, u_int32_t seqnum,
		     	  	u_int16_t metric, nsaddr_t nexthop,
		      		double expire_time);
        void            rt_down(myprot_rt_entry *rt);
        //void            local_rt_repair(aodv_rt_entry *rt, Packet *p);
 public:
        //void            rt_ll_failed(Packet *p);
        void            handle_link_failure(nsaddr_t id);
 protected:
        void            rt_purge(void);

        void            enque(myprot_rt_entry *rt, Packet *p);
        Packet*         deque(myprot_rt_entry *rt);



        void head_rt_purge();
        void head_rt_update(myprot_head_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,
                nsaddr_t nexthop, double expire_time);
        myprot_head_rt_entry*       head_rt_add(nsaddr_t id);
        void                        head_rt_delete(nsaddr_t id);
        myprot_head_rt_entry*       head_rt_lookup(nsaddr_t id);
        /*
         * Broadcast ID Management
         */

        void            id_insert(nsaddr_t id, u_int32_t bid);
        bool	        id_lookup(nsaddr_t id, u_int32_t bid);
        void            id_purge(void);

         /*
         * Neighbor Management
         */
        MY_Neighbor*      nb_lookup(nsaddr_t id);
        MY_NeighborCluster* nc_lookup(nsaddr_t id);
        void            nb_purge(void);
        void            nb_delete(nsaddr_t id);       
        void            nc_delete(nsaddr_t id);
        void            nc_purge(void);
        /*
         * Packet TX Routines
         */
        void            forward(myprot_rt_entry *rt, Packet *p, double delay);
        void            sendClusterInRequest(nsaddr_t dst);
        void            sendRequest(Packet *p);

        void            sendReply(nsaddr_t ipdst, u_int32_t hop_count,
                                  nsaddr_t rpdst, u_int32_t rpseq,
                                  u_int32_t lifetime);
        void            sendError(Packet *p, bool jitter = true);
                                        
        /*
         * Packet RX Routines
         */
        void            recvMYPROT(Packet *p);
        void            recvClusterInRequest(Packet *p);
        void            recvRequest(Packet *p);
        void            recvReply(Packet *p);
        void            recvError(Packet *p);



        

        //void    sendClusterInHello();
	//void    sendClusterBetweenHello();
	//void	recvClusterInHello(Packet *p);
	//void	recvClusterBetweenHello(Packet *p);
        void calculate_cellular_area();
        void	calculate_distance();
	void	calculate_cellular_area(double lx,double ly);
        int     calculate_connectedness(nsaddr_t id);
        int    fill_send_rrep_packet(Packet *p);
        int    fill_recv_rrep_packet(Packet *p);
        int    fill_end_rrep_packet(Packet *p);
       /*
	 * History management
	 */
	
	double 		PerHopTime(myprot_rt_entry *rt);


        nsaddr_t        index;                  // IP Address of this node
        u_int32_t       seqno;                  // Sequence Number
        int             bid;                    // Broadcast ID



        Trace           *logtarget;


        
        /* for passing packets up to agents */
        NsObject* prot_dumx_;
        NsObject* uptarget_;		// for incoming packet


        myprot_rtable         rtable;                 // routing table
        myprot_bcache         bihead;                 // Broadcast ID Cache
        myprot_hrthead        hrthead;


        MyBroadcastTimer   btimer;
        MyRouteCacheTimer  rtimer;
        MyNeighborTimer    ntimer;            

        /*
         *  A "drop-front" queue used by the routing layer to buffer
         *  packets to which it does not have a route.
         */
        myprot_rqueue         rqueue;




        //int time_count=0;
	const double radius =250/3*2;	    
	const double unitx=sqrt(3)*radius;
	const double unity=3.0/2*radius;
	double dist_to_center;
	double	location_x;
	double	location_y;
	double	location_z;
	u_int8_t line_num;
	u_int8_t column_num;
	u_int8_t slot_area_num;


        int cluster_head_flag=1;
        int mid_dist;
        int node_incluster_num;
        int flag;
};
typedef struct POINT_XY
{
	double x;
	double y;
}POINT_XY;

#endif