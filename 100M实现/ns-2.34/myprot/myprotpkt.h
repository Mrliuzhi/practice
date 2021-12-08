#ifndef __myprotpkt_h__
#define __myprotpkt_h__


#define	max_cluster_node_num	50	
#define HDR_MYPROT(p)		((struct hdr_myprot*)hdr_myprot::access(p))
#define HDR_MYPROT_CLOSE_HELLO(p)  	((struct hdr_myprot_close_hello*)hdr_myprot::access(p))
#define HDR_MYPROT_HELLO(p)	((struct hdr_myprot_hello*)hdr_myprot::access(p))
#define HDR_MYPROT_CLOSE_RREQ(p)	((struct hdr_myprot_close_rreq*)hdr_myprot::access(p))
#define HDR_MYPROT_RREQ(p)	((struct hdr_myprot_request*)hdr_myprot::access(p))
#define HDR_MYPROT_RREP(p)	((struct hdr_myprot_reply*)hdr_myprot::access(p))
#define HDR_MYPROT_ERROR(p)	((struct hdr_myprot_error*)hdr_myprot::access(p))

enum{
	MYPROT_CLOSE_HELLO,
	MYPROT_HELLO,
	MYPROT_CLOSE_RREQ,
	MYPROT_RREQ,
	MYPROT_RREP,
	MYPROT_ERROR
};
struct hdr_myprot {
    u_int8_t myprot_type;
		// Header access methods
	static int offset_; 
	inline static int& offset() { return offset_; }
	inline static hdr_myprot* access(const Packet* p) {
		return (hdr_myprot*) p->access(offset_);
	}
};


struct hdr_myprot_close_hello{
	u_int8_t	ch_type;				//Packet Type
	u_int8_t    cluster_ID_line_num;
	u_int8_t    cluster_ID_column_num;
	u_int8_t	location_ID;
	double		location_x;
	double 		location_y;
	double 		dist_to_center;
	nsaddr_t	src_;
	double    	lifetime;			//value time
	inline int size() { 
		int sz = 0;
	
		sz = 5*sizeof(u_int32_t);
		assert (sz >= 0);
		return sz;
  }
};


struct myprot_nb_info{
	nsaddr_t	nb_addr;
	double		location_x;
	double 		location_y;		
};
struct hdr_myprot_hello{
	u_int8_t	mh_type;
	u_int8_t	node_num;
	u_int8_t    cluster_ID_line_num;
	u_int8_t    cluster_ID_column_num;
	struct 	myprot_nb_info	cluster_node[max_cluster_node_num];
	double 		lifetime;			//value time	
};


struct	hdr_myprot_close_rreq{
	u_int8_t        cr_type;
	u_int8_t       	rq_bcast_id;
	u_int8_t 		reserved[2];
	nsaddr_t        rq_dst;         // Destination IP Address
    u_int32_t       rq_dst_seqno;   // Destination Sequence Number
    u_int32_t       rq_src_seqno;   // Source Sequence Number
	double          rq_timestamp;   // when REQUEST sent;
	inline int size() { 
  		int sz = 0;
		sz = 5*sizeof(u_int32_t);
  	assert (sz >= 0);
	return sz;
	}
};
struct hdr_myprot_request {
        u_int8_t        rq_type;	// Packet Type
		u_int8_t        cluster_ID_line_num;
		u_int8_t        cluster_ID_column_num;
        u_int8_t        rq_cluster_count;   // cluster Count
		u_int32_t       rq_bcast_id;    // Broadcast ID
		nsaddr_t        src_head;         // Destination IP Address
        nsaddr_t        rq_dst;         // Destination IP Address
        u_int32_t       rq_dst_seqno;   // Destination Sequence Number
        nsaddr_t        rq_src;         // Source IP Address
        u_int32_t       rq_src_seqno;   // Source Sequence Number

        //double          rq_timestamp;   // when REQUEST sent;
					// used to compute route discovery latency
		inline int size() { 
  int sz = 0;
  /*
  	sz = sizeof(u_int8_t)		// rq_type
	     + 2*sizeof(u_int8_t) 	// reserved
	     + sizeof(u_int8_t)		// rq_hop_count
	     + sizeof(double)		// rq_timestamp
	     + sizeof(u_int32_t)	// rq_bcast_id
	     + sizeof(nsaddr_t)		// rq_dst
	     + sizeof(u_int32_t)	// rq_dst_seqno
	     + sizeof(nsaddr_t)		// rq_src
	     + sizeof(u_int32_t);	// rq_src_seqno
  */
  	sz = 7*sizeof(u_int32_t);
  	assert (sz >= 0);
	return sz;
  }
 
};

struct hdr_myprot_reply {
        u_int8_t        rp_type;        // Packet Type
        u_int8_t        reserved;
        u_int8_t        rp_hop_count;           // Hop Count
		u_int8_t		node_num_in_route;
		nsaddr_t		next_cluster_head;
		nsaddr_t		last_cluster_node_addr;
		nsaddr_t		cluster_in_node_addr[3];
		double			out_cluster_node_location_x;
		double			out_cluster_node_location_y;
		
		//double 			last_x_local;
		//double			last_y_local;
		u_int32_t       rp_src_seqno;           // Destination Sequence Number
        nsaddr_t        rp_dst;                 // Destination IP Address
        nsaddr_t        rp_src;                 // Source IP Address
        double	        rp_lifetime;            // Lifetime

        //double          rp_timestamp;           // when corresponding REQ sent;
						// used to compute route discovery latency
		/*u_int8_t        reserved[2];
        u_int8_t        rp_hop_count;           // Hop Count
        nsaddr_t        rp_dst;                 // Destination IP Address
        u_int32_t       rp_dst_seqno;           // Destination Sequence Number
        nsaddr_t        rp_src;                 // Source IP Address
        double	        rp_lifetime;            // Lifetime

        double          rp_timestamp;           // when corresponding REQ sent;
						// used to compute route discovery latency*/
						
  inline int size() { 
  int sz = 0;
  /*
  	sz = sizeof(u_int8_t)		// rp_type
	     + 2*sizeof(u_int8_t) 	// rp_flags + reserved
	     + sizeof(u_int8_t)		// rp_hop_count
	     + sizeof(double)		// rp_timestamp
	     + sizeof(nsaddr_t)		// rp_dst
	     + sizeof(u_int32_t)	// rp_dst_seqno
	     + sizeof(nsaddr_t)		// rp_src
	     + sizeof(u_int32_t);	// rp_lifetime
  */
  	sz = 6*sizeof(u_int32_t);
  	assert (sz >= 0);
	return sz;
  }				

};
#define MPROT_MAX_ERRORS	10
struct hdr_myprot_error {
        u_int8_t        re_type;                // Type
        u_int8_t        reserved[2];            // Reserved
        u_int8_t        DestCount;                 // DestCount
        // List of Unreachable destination IP addresses and sequence numbers
        nsaddr_t        unreachable_dst[MPROT_MAX_ERRORS];   
        u_int32_t       unreachable_dst_seqno[MPROT_MAX_ERRORS];   

  inline int size() { 
  int sz = 0;
  /*
  	sz = sizeof(u_int8_t)		// type
	     + 2*sizeof(u_int8_t) 	// reserved
	     + sizeof(u_int8_t)		// length
	     + length*sizeof(nsaddr_t); // unreachable destinations
  */
  	sz = (DestCount*2 + 1)*sizeof(u_int32_t);
	assert(sz);
        return sz;
  }

};





#endif