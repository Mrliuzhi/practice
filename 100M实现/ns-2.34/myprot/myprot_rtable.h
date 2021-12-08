#ifndef __myprot_rtable_h__
#define __myprot_rtable_h__


#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>

#define INFINITY2        0xff

/*
   MYPROT Neighbor Cache Entry
*/
class MYPROT_Neighbor {
        friend class MYPROT;
        friend class MYMac;
        friend class myprot_rt_entry;
 public:
        MYPROT_Neighbor(u_int32_t a) { nb_addr = a; }

 protected:
        LIST_ENTRY(MYPROT_Neighbor) nb_link;
        nsaddr_t        nb_addr;
        double          x_local;
        double          y_local;        //yonglai cunchu pingmian moxing jiedian de weizhi
        double          dist_to_center;
        double          nb_expire;      
};
LIST_HEAD(myprot_ncache, MYPROT_Neighbor);

#define MAX_NODE_NUM 50

struct Cluster_in_node{
        nsaddr_t        nb_addr;
        double          x_local;
        double          y_local; 
};

class MYPROT_NeighborCluster {
        friend class MYPROT;
        friend class myprot_rt_entry;
 public:
        MYPROT_NeighborCluster(u_int32_t a) { nb_addr = a; }

 protected:
        LIST_ENTRY(MYPROT_NeighborCluster) nc_link;
        nsaddr_t        nb_addr;
        Cluster_in_node   cluster_in_node_[MAX_NODE_NUM];
        double          nb_expire;      
};
LIST_HEAD(myprot_clusternode, MYPROT_NeighborCluster);

/*
   AODV Precursor list data structure
*/
class MYPROT_Precursor {
        friend class MYPROT;
        friend class myprot_rt_entry;
 public:
        MYPROT_Precursor(u_int32_t a) { pc_addr = a; }

 protected:
        LIST_ENTRY(MYPROT_Precursor) pc_link;
        nsaddr_t        pc_addr;	// precursor address
};

LIST_HEAD(myprot_precursors, MYPROT_Precursor);

/*
  Broadcast ID Cache
*/
class MPPROT_BroadcastID {
        friend class MYPROT;
 public:
        MPPROT_BroadcastID(nsaddr_t i, u_int32_t b) { src = i; id = b;  }
 protected:
        LIST_ENTRY(MPPROT_BroadcastID) link;
        nsaddr_t        src;
        u_int32_t       id;
        double          expire;         // now + BCAST_ID_SAVE s
};

LIST_HEAD(myprot_bcache, MPPROT_BroadcastID);
/*
  Route Table Entry
*/

class myprot_rt_entry {
        friend class myprot_rtable;
        friend class MYPROT;
 public:
        myprot_rt_entry();
        ~myprot_rt_entry();

        /*
         * Neighbor Management
         */
        void            nb_insert(nsaddr_t id);
        MYPROT_Neighbor*  nb_lookup(nsaddr_t id);

        void            pc_insert(nsaddr_t id);
        MYPROT_Precursor* pc_lookup(nsaddr_t id);
        void 		pc_delete(nsaddr_t id);
        void 		pc_delete(void);
        bool 		pc_empty(void);

        double          rt_req_timeout;         // when I can send another req
        u_int8_t        rt_req_cnt;             // number of route requests
	
 protected:
        LIST_ENTRY(myprot_rt_entry) rt_link;

        nsaddr_t        rt_dst;
        u_int32_t       rt_seqno;
	/* u_int8_t 	rt_interface; */
        u_int16_t       rt_hops;       		// hop count
	int 		rt_last_hop_count;	// last valid hop count
        nsaddr_t        rt_nexthop;    		// next hop IP address
        double          rt_expire;     		// when entry expires
        u_int8_t        rt_flags;

#define RTF_DOWN 0
#define RTF_UP 1
#define RTF_IN_REPAIR 2




#define MAX_HISTORY	3
	double 		rt_disc_latency[MAX_HISTORY];
	char 		hist_indx;
        int 		rt_req_last_ttl;        // last ttl value used



        
        /* list of precursors */ 
        myprot_precursors rt_pclist;


         /*
         * a list of neighbors that are using this route.
         */
        myprot_ncache      rt_nblist;
};


/*
  The Routing Table
*/

class myprot_rtable {
 public:
	myprot_rtable() { LIST_INIT(&rthead); }

        myprot_rt_entry*       head() { return rthead.lh_first; }

        myprot_rt_entry*       rt_add(nsaddr_t id);
        void                 rt_delete(nsaddr_t id);
        myprot_rt_entry*       rt_lookup(nsaddr_t id);

 private:
        LIST_HEAD(myprot_rthead, myprot_rt_entry) rthead;
};






class myprot_head_rt_entry {
        friend class MYPROT;
 public:
        myprot_head_rt_entry(nsaddr_t dst){rt_dst=dst;rt_seqno=0;rt_hops=255;rt_nexthop=0;rt_expire=0;}

        //double          rt_req_timeout;         // when I can send another req
        //u_int8_t        rt_req_cnt;             // number of route requests
	
 protected:
        LIST_ENTRY(myprot_head_rt_entry) rt_head_link;

        nsaddr_t        rt_dst;
        u_int32_t       rt_seqno;
	/* u_int8_t 	rt_interface; */
        u_int16_t       rt_hops;       		// hop count
	//int 		rt_last_hop_count;	// last valid hop count
        nsaddr_t        rt_nexthop;    		// next hop IP address
        double          rt_expire;     		// when entry expires
};
LIST_HEAD(myprot_hrthead, myprot_head_rt_entry);

#endif