#include <myprot/myprot.h>
#include <random.h>
#include <cmu-trace.h>


#define max(a,b)        ( (a) > (b) ? (a) : (b) )
#define CURRENT_TIME    Scheduler::instance().clock()     //获取当前的时间

/******开辟包头空间的代码*****/
int hdr_myprot::offset_;
static class MYPROTHeaderClass : public PacketHeaderClass {
public:
        MYPROTHeaderClass() : PacketHeaderClass("PacketHeader/MYPROT",
                                              sizeof(hdr_myprot)) {
	  bind_offset(&hdr_myprot::offset_);
	} 
} class_rtProtoMYPROT_hdr;    

/*********协议名类*******/
static class MYPROTclass : public TclClass {
public:
        MYPROTclass() : TclClass("Agent/MYPROT") {}
        TclObject* create(int argc, const char*const* argv) {
          assert(argc == 5);
	      return (new MYPROT((nsaddr_t) Address::instance().str2addr(argv[4])));
        }
} class_rtProtoMYPROT;

/*********与tcl脚本的挂钩*******/
int
MYPROT::command(int argc, const char*const* argv) {
  if(argc == 2) {
  Tcl& tcl = Tcl::instance();
    
    if(strncasecmp(argv[1], "id", 2) == 0) {
      tcl.resultf("%d", index);
      return TCL_OK;
    }
    
    if(strncasecmp(argv[1], "start", 2) == 0) {
      btimer.handle((Event*) 0);
      rtimer.handle((Event*) 0);  
      ntimer.handle((Event*) 0);     
      return TCL_OK;
     }               
  }
  else if(argc == 3) {
    if(strcmp(argv[1], "index") == 0) {
      index = atoi(argv[2]);
      return TCL_OK;
    }

    else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
      logtarget = (Trace*) TclObject::lookup(argv[2]);
      if(logtarget == 0)
	    return TCL_ERROR;
      return TCL_OK;
    }
   else if (strcmp(argv[1], "port-dmux") == 0) {
      TclObject *obj;
      prot_dumx_=(NsObject*)obj;
      return TCL_OK;
    }
     else if (strcmp(argv[1], "up-target") == 0) {
			uptarget_ = (NsObject*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
  }
  return Agent::command(argc, argv);
}
/******类的构造函数*******/
MYPROT::MYPROT(nsaddr_t id) : Agent(PT_MYPROT),prot_dumx_(0),btimer(this),
                              rtimer(this),ntimer(this),rqueue(){
 
                
  index = id;
  seqno = 2;
  bid = 1;

  LIST_INIT(&hrthead);      //cushou jian luyou
  LIST_INIT(&bihead);
  logtarget=0;
}


/*------timers--------*/
void
MyBroadcastTimer::handle(Event*) {
  agent->id_purge();
  Scheduler::instance().schedule(this, &intr, BCAST_ID_SAVE);
}
void
MyNeighborTimer::handle(Event*) {
  agent->nb_purge();
  agent->nc_purge();
  Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}
void
MyRouteCacheTimer::handle(Event*) {
  agent->rt_purge();
#define FREQUENCY 0.5 // sec
  Scheduler::instance().schedule(this, &intr, FREQUENCY);
  
}



/*
  Helper Functions这个还不太懂？？？？？？？？？？？？？？？
*/

double
MYPROT::PerHopTime(myprot_rt_entry *rt) {
int num_non_zero = 0, i;
double total_latency = 0.0;

 if (!rt)
   return ((double) NODE_TRAVERSAL_TIME );
	
 for (i=0; i < MAX_HISTORY; i++) {
   if (rt->rt_disc_latency[i] > 0.0) {
      num_non_zero++;
      total_latency += rt->rt_disc_latency[i];
   }
 }
 if (num_non_zero > 0)
   return(total_latency / (double) num_non_zero);
 else
   return((double) NODE_TRAVERSAL_TIME);

}


void
MYPROT::rt_resolve(Packet *p) {
  ////printf("MYPROT::rt_resolve\n");
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
myprot_rt_entry *rt;
 /*
  *  Set the transmit failure callback.  That
  *  won't change.                                                    这一部分先不加
  */
 //ch->xmit_failure_ = aodv_rt_failed_callback;
 //ch->xmit_failure_data_ = (void*) this;
	rt = rtable.rt_lookup(ih->daddr());                     //是否有到目的节点的路由，如果没有的话添加该路由条目
 if(rt == 0) {
	  rt = rtable.rt_add(ih->daddr());
 }
  MY_Neighbor *nb=nb_lookup(ih->daddr());
  if(nb)
  {
    seqno += 2;
    assert ((seqno%2) == 0);
    if ( (rt->rt_seqno < seqno) ||   // newer route 
          ((rt->rt_seqno == seqno) &&  
          (rt->rt_hops > 1)) ) // shorter or better route
      
       
      rt_update(rt,seqno, 1,
        nb->nb_addr, CURRENT_TIME + MY_ROUTE_TIMEOUT);

  }
        
 /*
  * If the route is up, forward the packet 
  * 路由条目存在且陆游标志为有效，直接转发
  */
	
 if(rt->rt_flags == RTF_UP) {
   assert(rt->rt_hops != INFINITY2);
   forward(rt, p, NO_DELAY);
 }
 /*
  *  if I am the source of the packet, then do a Route Request.
  * 自己发出的包做路由请求
  */
	else if(ih->saddr() == index) {  
   rqueue.enque(p);
   sendClusterInRequest(rt->rt_dst);
 }
 /*
  *	A local repair is in progress. Buffer the packet. 
  *这个东西怎么说？？？？？？？？？
  */
 else if (rt->rt_flags == RTF_IN_REPAIR) {
   rqueue.enque(p);
 }

 /*
  * I am trying to forward a packet for someone else to which
  * I don't have a route.转发一个不知道路由的包，发送路由错误包并且丢弃该包
  */
 else {
 Packet *rerr = Packet::alloc();
 struct hdr_myprot_error *re = HDR_MYPROT_ERROR(rerr);
 /* 
  * For now, drop the packet and send error upstream.
  * Now the route errors are broadcast to upstream
  * neighbors - Mahesh 09/11/99
  */	
 
   assert (rt->rt_flags == RTF_DOWN);
   re->DestCount = 0;
   re->unreachable_dst[re->DestCount] = rt->rt_dst;
   re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
   re->DestCount += 1;
#ifdef DEBUG
   fprintf(stderr, "%s: sending RERR...\n", __FUNCTION__);
#endif
   sendError(rerr, false);

   drop(p, DROP_RTR_NO_ROUTE);
 }

}

void
MYPROT::rt_update(myprot_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,
	       	nsaddr_t nexthop, double expire_time) {

     rt->rt_seqno = seqnum;
     rt->rt_hops = metric;
     rt->rt_flags = RTF_UP;
     rt->rt_nexthop = nexthop;
     rt->rt_expire = expire_time;
}

void
MYPROT::rt_down(myprot_rt_entry *rt) {
  /*
   *  Make sure that you don't "down" a route more than once.
   */

  if(rt->rt_flags == RTF_DOWN) {
    return;
  }

  // assert (rt->rt_seqno%2); // is the seqno odd?
  //rt->rt_last_hop_count = rt->rt_hops;
  rt->rt_hops = INFINITY2;
  rt->rt_flags = RTF_DOWN;
  rt->rt_nexthop = 0;
  rt->rt_expire = 0;

} /* rt_down function */

/*
  Route Handling Functions
*/
void
MYPROT::rt_purge() {
myprot_rt_entry *rt, *rtn;
double now = CURRENT_TIME;
double delay = 0.0;
Packet *p;

 for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
   rtn = rt->rt_link.le_next;
   if ((rt->rt_flags == RTF_UP) && (rt->rt_expire < now)) {
   // if a valid route has expired, purge all packets from 
   // send buffer and invalidate the route.                    
	assert(rt->rt_hops != INFINITY2);
     while((p = rqueue.deque(rt->rt_dst))) {
#ifdef DEBUG
       fprintf(stderr, "%s: calling drop()\n",
                       __FUNCTION__);
#endif // DEBUG
       drop(p, DROP_RTR_NO_ROUTE);
     }
     rt->rt_seqno++;
     assert (rt->rt_seqno%2);
     rt_down(rt);
   }
   else if (rt->rt_flags == RTF_UP) {
   // If the route is not expired,
   // and there are packets in the sendbuffer waiting,
   // forward them. This should not be needed, but this extra 
   // check does no harm.
     assert(rt->rt_hops != INFINITY2);
     while((p = rqueue.deque(rt->rt_dst))) {
       forward (rt, p, delay);
       delay += ARP_DELAY;
     }
   } 
   else if (rqueue.find(rt->rt_dst))
   // If the route is down and 
   // if there is a packet for this destination waiting in
   // the sendbuffer, then send out route request. sendRequest
   // will check whether it is time to really send out request
   // or not.
   // This may not be crucial to do it here, as each generated 
   // packet will do a sendRequest anyway.

     sendClusterInRequest(rt->rt_dst); 
   }

}


void
MYPROT::head_rt_purge() {
myprot_head_rt_entry *rt, *rtn;
double now = CURRENT_TIME;
double delay = 0.0;
Packet *p;

 for(rt = hrthead.lh_first; rt; rt = rtn) {  // for each rt entry
   rtn = rt->rt_head_link.le_next;
   if ( (rt->rt_expire < now)) {
        head_rt_delete(rt->rt_dst);
   }
  
   
  }
}
void
MYPROT::head_rt_update(myprot_head_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,
	       	nsaddr_t nexthop, double expire_time) {

     rt->rt_seqno = seqnum;
     rt->rt_hops = metric;
     //rt->rt_flags = RTF_UP;
     rt->rt_nexthop = nexthop;
     rt->rt_expire = expire_time;
}

myprot_head_rt_entry*
MYPROT::head_rt_lookup(nsaddr_t id)
{
  myprot_head_rt_entry *rt = hrthead.lh_first;

  for(; rt; rt = rt->rt_head_link.le_next) {
    if(rt->rt_dst == id)
      break;
  }
  return rt;

}

void
MYPROT::head_rt_delete(nsaddr_t id)
{
myprot_head_rt_entry *rt = head_rt_lookup(id);

 if(rt) {
   LIST_REMOVE(rt, rt_head_link);
   delete rt;
 }

}

myprot_head_rt_entry*
MYPROT::head_rt_add(nsaddr_t id)
{
myprot_head_rt_entry *rt;

 assert(head_rt_lookup(id) == 0);
 rt = new myprot_head_rt_entry(id);
 assert(rt);
 rt->rt_dst = id;
 LIST_INSERT_HEAD(&hrthead, rt, rt_head_link);
 return rt;
}






/*
   Broadcast ID Management  Functions
   用来做重复广播的记录，查询，删除等工作
*/


void
MYPROT::id_insert(nsaddr_t id, u_int32_t bid) {
MPPROT_BroadcastID *b = new MPPROT_BroadcastID(id, bid);

 assert(b);
 b->expire = CURRENT_TIME + BCAST_ID_SAVE;
 LIST_INSERT_HEAD(&bihead, b, link);
}

/* SRD */
bool
MYPROT::id_lookup(nsaddr_t id, u_int32_t bid) {
MPPROT_BroadcastID *b = bihead.lh_first;
 
 // Search the list for a match of source and bid
 for( ; b; b = b->link.le_next) {
   if ((b->src == id) && (b->id == bid))
     return true;     
 }
 return false;
}

void
MYPROT::id_purge() {
MPPROT_BroadcastID *b = bihead.lh_first;
MPPROT_BroadcastID *bn;
double now = CURRENT_TIME;

 for(; b; b = bn) {
   bn = b->link.le_next;
   if(b->expire <= now) {
     LIST_REMOVE(b,link);
     delete b;
   }
 }
}
/*************邻居节点的查询和邻居簇的查询*****************************/
MY_Neighbor*
MYPROT::nb_lookup(nsaddr_t id) {
 Node *thisnode;
 thisnode = Node::get_node_by_address(index);
 MY_Neighbor *nb = ((MobileNode *)thisnode)->mnbhead.lh_first;

 for(; nb; nb = nb->mynb_link.le_next) {
   if(nb->nb_addr == id) break;
 }
 return nb;
}
MY_NeighborCluster*
MYPROT::nc_lookup(nsaddr_t id) {
 Node *thisnode;
 thisnode = Node::get_node_by_address(index);
 MY_NeighborCluster *nc = ((MobileNode *)thisnode)->mnchead.lh_first;

 for(; nc; nc = nc->mync_link.le_next) {
   if(nc->nb_addr == id) break;
 }
 return nc;
}
void
MYPROT::nb_delete(nsaddr_t id) {
 Node *thisnode;
 thisnode = Node::get_node_by_address(index);
 MY_Neighbor *nb = ((MobileNode *)thisnode)->mnbhead.lh_first;

 for(; nb; nb = nb->mynb_link.le_next) {
   if(nb->nb_addr == id) {
     LIST_REMOVE(nb,mynb_link);
     delete nb;
     break;
   }
 }

 }
void
MYPROT::nb_purge() {
 Node *thisnode;
 thisnode = Node::get_node_by_address(index);
 MY_Neighbor *nb = ((MobileNode *)thisnode)->mnbhead.lh_first;
 MY_Neighbor *nbn;
double now = CURRENT_TIME;

 for(; nb; nb = nbn) {
   nbn = nb->mynb_link.le_next;
   if(nb->nb_expire <= now) {
     nb_delete(nb->nb_addr);
   }
 }

}
void
MYPROT::nc_delete(nsaddr_t id) {
 Node *thisnode;
 thisnode = Node::get_node_by_address(index);
 MY_NeighborCluster *nb = ((MobileNode *)thisnode)->mnchead.lh_first;

 for(; nb; nb = nb->mync_link.le_next) {
   if(nb->nb_addr == id) {
     LIST_REMOVE(nb,mync_link);
     delete nb;
     break;
   }
 }

 }
void
MYPROT::nc_purge() {
 Node *thisnode;
 thisnode = Node::get_node_by_address(index);
 MY_NeighborCluster *nb = ((MobileNode *)thisnode)->mnchead.lh_first;
 MY_NeighborCluster *nbn;
double now = CURRENT_TIME;

 for(; nb; nb = nbn) {
   nbn = nb->mync_link.le_next;
   if(nb->nb_expire <= now) {
     nb_delete(nb->nb_addr);
   }
 }

}
void
MYPROT::recv(Packet *p, Handler*) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);

 assert(initialized());
 //assert(p->incoming == 0);
 // XXXXX NOTE: use of incoming flag has been depracated; In order to track direction of pkt flow, direction_ in hdr_cmn is used instead. see packet.h for details.
if(flag==0)
{
  calculate_cellular_area();
  ////printf("node<%d>:line_num=%d,column_num=%d\n",index,line_num,column_num);
  flag=1;
}
 if(ch->ptype() == PT_MYPROT) {
   ih->ttl_ -= 1;                 //判断是路由消息之后，先手将转发次数减一，后面就不用操作了
   recvMYPROT(p);
   return;
 }


 /*
  *  Must be a packet I'm originating...
  * 是自己产生的包
  */
if((ih->saddr() == index) && (ch->num_forwards() == 0)) {
 /*
  * Add the IP Header.  
  * TCP adds the IP header too, so to avoid setting it twice, we check if
  * this packet is not a TCP or ACK segment.
  */
  if (ch->ptype() != PT_TCP && ch->ptype() != PT_ACK) {
    ch->size() += IP_HDR_LEN;
  }
   // Added by Parag Dadhania && John Novatnack to handle broadcasting   //????????????????
  if ( (u_int32_t)ih->daddr() != IP_BROADCAST) {
    ih->ttl_ = NETWORK_DIAMETER;
  }
}
 /*
  *  I received a packet that I sent.  Probably
  *  a routing loop.
  * 收到自己发出去的包
  */
else if(ih->saddr() == index) {
   drop(p, DROP_RTR_ROUTE_LOOP);
   return;
 }
 /*
  *  Packet I'm forwarding...
  * 转发包
  */
 else {
 /*
  *  Check the TTL.  If it is zero, then discard.
  */
   if(--ih->ttl_ == 0) {
     drop(p, DROP_RTR_TTL);
     return;
   }
 }
// Added by Parag Dadhania && John Novatnack to handle broadcasting
 if ( (u_int32_t)ih->daddr() != IP_BROADCAST)
   rt_resolve(p);                 //不是广播包的话处理和路由相关的步骤
 else     
   forward((myprot_rt_entry*) 0, p, NO_DELAY);  //是广播包直接转发
}


void
MYPROT::recvMYPROT(Packet *p) {
 struct hdr_myprot *ah = HDR_MYPROT(p);

 assert(HDR_IP (p)->sport() == RT_PORT);
 assert(HDR_IP (p)->dport() == RT_PORT);

 /*
  * Incoming Packets.
  */
 switch(ah->myprot_type) {

 case MYPROT_CLOSE_RREQ:
   recvClusterInRequest(p);
   break;

 case MYPROT_RREQ:
   recvRequest(p);
   break;

 case MYPROT_RREP:
   recvReply(p);
   break;

 case MYPROT_ERROR:
   recvError(p);
   break;

 default:
   fprintf(stderr, "Invalid AODV type (%x)\n", ah->myprot_type);
   exit(1);
 }

}

void    MYPROT::recvClusterInRequest(Packet *p)
{
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_myprot_close_rreq *rq = HDR_MYPROT_CLOSE_RREQ(p);
  /******************该消息只有簇首节点接受并做出反应********************/
  Node *thisnode;
	thisnode = Node::get_node_by_address(index);
	int flag=((MobileNode *)thisnode)->clustre_head_flag;
  if(flag!=index)
  {
    //printf("In recvClusterInRequest,node <%d>not head ,drop\n",index);
    Packet::free(p);
    return;
  }
  MY_Neighbor *nb=nb_lookup(ih->saddr());
  //且只对自己簇内的节点做出反应
  if(nb==0)
  {
    //printf("In recvClusterInRequest,node <%d>not nb ,drop\n",index);
    Packet::free(p);
    return;
  }
  if(nb->line_num==line_num&&nb->column_num==column_num)
  {
    MY_Neighbor *nb1=nb_lookup(rq->rq_dst);
    if(nb1&&(nb1->line_num==line_num&&nb1->column_num==column_num))
    {
      //printf("node<%d>:wo keyi jinlai\n",index);
      MY_Neighbor *nb2=nb_lookup(index);
      for(; nb2; nb1 = nb2->mynb_link.le_next) {
        double dist_to_src=(nb->x_local-nb2->x_local)*(nb->x_local-nb2->x_local)+(nb->y_local-nb2->y_local)*(nb->y_local-nb2->y_local);
        double dist_to_dest=(nb1->x_local-nb2->x_local)*(nb1->x_local-nb2->x_local)+(nb1->y_local-nb2->y_local)*(nb1->y_local-nb2->y_local);
        if((dist_to_src<(250*250))&&(dist_to_dest<(250*250)))
          break;
      }
      Packet *pkt_ = Packet::alloc();
      struct hdr_cmn *ch = HDR_CMN(pkt_);
      struct hdr_ip *ihx = HDR_IP(pkt_);
      struct hdr_myprot_reply *rp = HDR_MYPROT_RREP(pkt_);
      //myprot_head_rt_entry *hrt = head_rt_lookup(ipdst);
      //assert(rt);
    
      rp->rp_type = MYPROT_RREP;
      //rp->rp_flags = 0x00;
      rp->rp_hop_count = 2;
      rp->rp_dst = ih->saddr();            //rp消息的目的节点，rq消息的源节点
      rp->rp_src_seqno = rq->rq_dst_seqno;
      rp->rp_src = rq->rq_dst;            //rq消息的目的节点
      rp->next_cluster_head=255;
      rp->rp_lifetime = MY_ROUTE_TIMEOUT;
      rp->node_num_in_route=3;
      rp->last_cluster_node_addr=rq->rq_dst;
      
      //printf("node<%d>:begin send reply\n",index);
      rp->cluster_in_node_addr[0]=rq->rq_dst;
      rp->cluster_in_node_addr[1]=nb2->nb_addr;
      rp->cluster_in_node_addr[2]=ih->saddr();
      for(int i=0;i<rp->node_num_in_route;i++)
      {
        if(rp->cluster_in_node_addr[i]==index)
        {
          if(i==0)
          {
            //printf("head is the dest\n");
            break;
          }
          else
          {
            myprot_rt_entry *rt;
            rt = rtable.rt_lookup(rp->rp_src);
          

            if(rt == 0) {
              rt = rtable.rt_add(rp->rp_src);
            }
            if ( (rt->rt_seqno < rp->rp_src_seqno) ||   // newer route 
              ((rt->rt_seqno == rp->rp_src_seqno) &&  
              (rt->rt_hops > 1)) ) { // shorter or better route
          
          // Update the rt entry 
          rt_update(rt, rp->rp_src_seqno, 1,
            rp->cluster_in_node_addr[i-1], CURRENT_TIME + rp->rp_lifetime);

          // reset the soft state
          rt->rt_req_cnt = 0;
          rt->rt_req_timeout = 0.0; 
          rt->rt_req_last_ttl = 1;


        //printf("node<%d> in route ,next hop=%d\n",index,rt->rt_nexthop);
          Packet *buf_pkt;
          while((buf_pkt = rqueue.deque(rt->rt_dst))) {
            if(rt->rt_hops != INFINITY2) {
                  assert (rt->rt_flags == RTF_UP);
            // Delay them a little to help ARP. Otherwise ARP 
            // may drop packets. -SRD 5/23/99
              forward(rt, buf_pkt, 0.0);
              
                  }
                }

            }
          }
        
        }
      }
      ch->ptype() = PT_MYPROT;
      ch->iface() = -2;
      ch->error() = 0;
      ch->addr_type() = NS_AF_NONE;
      //ch->next_hop_ = hrt->rt_nexthop;
      ch->prev_hop_ = index;          
      ch->direction() = hdr_cmn::DOWN;
      ch->size() = (IP_HDR_LEN + 12*sizeof(u_int32_t));
      ihx->saddr() = index;
      ihx->daddr() = IP_BROADCAST;
      ihx->sport() = RT_PORT;
      ihx->dport() = RT_PORT;
      ihx->ttl_ = 255;

      
      //printf("node<%d>: send reply \n",index);

      Scheduler::instance().schedule(target_, pkt_, 0);
    }
    else
    {
      sendRequest(p);     //是自己簇内的节点是做出反应，开始簇首间路由请求
      Packet::free(p);
    }
    
  }
  else
  {
    //printf("In recvClusterInRequest,node <%d>not same cluster,drop\n",index);
    Packet::free(p);
    return;
  }
}

void
MYPROT::recvRequest(Packet *p) {
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_myprot_request *rq = HDR_MYPROT_RREQ(p);
  //myprot_rt_entry *rt;
  Node *thisnode;
	thisnode = Node::get_node_by_address(index);
	int flag=((MobileNode *)thisnode)->clustre_head_flag;
  if(flag!=index)
  {
    //printf("node <%d> not head drpo it \n",index);
    Packet::free(p);
    return;
  }

    /*
    * Drop if:
    *      - I'm the source
    *      - I recently heard this request.
    */
  if(rq->src_head == index) {
    //printf("node<%d>:i am the source\n",index);
    Packet::free(p);
    return;
  } 
  if(!calculate_connectedness(ih->saddr()))      //和邻簇之间没有链接，不转发，直接丢弃
  {
    //printf("node<%d>:connectness is  not ok\n",index);
    Packet::free(p);
    return;
  }
  

  if (id_lookup(rq->rq_src, rq->rq_bcast_id)) {
    Packet::free(p);
    return;
  }

  /*
    * Cache the broadcast ID
    */
  id_insert(rq->rq_src, rq->rq_bcast_id);

    myprot_head_rt_entry *rt0; 
    //簇首间路由，到某个簇的簇内节点的簇首间路由
    rt0 = head_rt_lookup(rq->rq_src);
    if(rt0 == 0) { 
    
      rt0 = head_rt_add(rq->rq_src);
    }
    
    rt0->rt_expire = max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE));   //这个时间还是要再想一下
    //printf("rt0->rt_seqno=%d\n",rt0->rt_seqno);
    if (rq->rq_src_seqno > rt0->rt_seqno   ) {
       //printf("node<%d>,head_rt_update\n",index);

      head_rt_update(rt0, rq->rq_src_seqno, rq->rq_cluster_count, ih->saddr(),
              max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE)) );

    }


//目的节点在簇内，回复;否则转发
  MY_Neighbor *nb=nb_lookup(rq->rq_dst);
  ////printf("destId=%d\n",rq->rq_dst);
  if(nb)
  {
    ////printf("nb=%d,nb->line_num=%d,nb->column_num=%d\n",nb,nb->line_num,nb->column_num);
    ////printf("line_num=%d,column_num=%d\n",line_num,column_num);
  }
  

  if(nb!=0&&(nb->line_num==line_num&&nb->column_num==column_num)) {
    
  
    //printf("node <%d> ,dest head \n",index);
    seqno = max(seqno, rq->rq_dst_seqno)+1;
    if (seqno%2) seqno++;

    sendReply(rq->rq_src,           // IP Destination
              1,                    // Hop Count
              rq->rq_dst,                // Dest IP Address
              seqno,                // Dest Sequence Num
              MY_ROUTE_TIMEOUT);  
  
    Packet::free(p);
  }
  else {                        
    ih->saddr() = index;
    ih->daddr() = IP_BROADCAST;
    rq->rq_cluster_count += 1;
    forward((myprot_rt_entry*) 0, p, DELAY);
  }

}


void
MYPROT::recvReply(Packet *p) {
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_myprot_reply *rp= HDR_MYPROT_RREP(p);
  struct hdr_cmn *ch = HDR_CMN(p);
  myprot_rt_entry *rt;
  myprot_head_rt_entry *hrt = head_rt_lookup(rp->rp_dst);

  //先做出路由，是不是自己簇的簇首发出的消息
  MY_Neighbor *nb=nb_lookup(ih->saddr()); 
  if(nb!=0)
  {
    if(nb->line_num==line_num&&nb->column_num==column_num)
    {
      
      for (int i=0;i<rp->node_num_in_route;i++)
      {
        if(index==rp->cluster_in_node_addr[i])
        {
          if(rp->last_cluster_node_addr==index)    //自己是目的节点
          {
            //printf("不用做操作\n");
          }
          else
          {
                
            rt = rtable.rt_lookup(rp->rp_src);
            //printf("node <%d> in route\n",index);
              
            /*
              *  If I don't have a rt entry to this host... adding
              */
            if(rt == 0) {
              rt = rtable.rt_add(rp->rp_src);
            }

            /*
              * Add a forward route table entry... here I am following 
              * Perkins-Royer AODV paper almost literally - SRD 5/99
              */

            if ( (rt->rt_seqno < rp->rp_src_seqno) ||   // newer route 
                  ((rt->rt_seqno == rp->rp_src_seqno) &&  
                  (rt->rt_hops > (rp->rp_hop_count-(rp->node_num_in_route-i)))) ) { // shorter or better route
              
              // Update the rt entry 
              if(i==0)
              {
                rt_update(rt, rp->rp_src_seqno, rp->rp_hop_count+i+1,
                  rp->last_cluster_node_addr, CURRENT_TIME + rp->rp_lifetime);
              }
              else
                rt_update(rt, rp->rp_src_seqno, rp->rp_hop_count+i+1,
                  rp->cluster_in_node_addr[i-1], CURRENT_TIME + rp->rp_lifetime);

              // reset the soft state   忘了是干啥的了
              rt->rt_req_cnt = 0;
              rt->rt_req_timeout = 0.0; 
              rt->rt_req_last_ttl = 1;

              //printf("node<%d>,next hop=%d\n",index,rt->rt_nexthop);
              /*
              * Send all packets queued in the sendbuffer destined for
              * this destination. 
              * XXX - observe the "second" use of p.
              */
              Packet *buf_pkt;
              while((buf_pkt = rqueue.deque(rt->rt_dst))) {
                if(rt->rt_hops != INFINITY2) {
                      assert (rt->rt_flags == RTF_UP);
                // Delay them a little to help ARP. Otherwise ARP 
                // may drop packets. -SRD 5/23/99
                  forward(rt, buf_pkt, 0.0);
                }
              }
          
            }
          }
        }
      }
    }
  }
  
  //目的簇首
  if(rp->next_cluster_head==index)
  {
    MY_Neighbor *nb1=nb_lookup(rp->rp_dst);
    if(nb1!=0&&(nb1->line_num==line_num&&nb1->column_num==column_num))
    {
      //printf("node<%d>:I am the dest head in rrep\n",index);
      if(!fill_end_rrep_packet(p))
        {
		Packet::free(p);
		return;
	}
      else
      {
          rp->next_cluster_head=255;//没有下一跳了
          ih->saddr() = index;
          ih->daddr() = IP_BROADCAST;
          forward((myprot_rt_entry*) 0, p, DELAY);
      
      }
    }
    //中间的簇首
    else
    {
      if(!fill_recv_rrep_packet(p))
        {
		Packet::free(p);
		return;
	}
      else
      {
        if (hrt) {
          assert(hrt->rt_flags == RTF_UP);
          rp->next_cluster_head=hrt->rt_nexthop;
          ih->saddr() = index;
          ih->daddr() = IP_BROADCAST;
          forward((myprot_rt_entry*) 0, p, DELAY);
          }
      }

    }  
    
  }
  else
  {
    drop(p);
  }
  
}

void
MYPROT::recvError(Packet *p) {
struct hdr_ip *ih = HDR_IP(p);
struct hdr_myprot_error *re = HDR_MYPROT_ERROR(p);
myprot_rt_entry *rt;
u_int8_t i;
Packet *rerr = Packet::alloc();
struct hdr_myprot_error *nre = HDR_MYPROT_ERROR(rerr);

 nre->DestCount = 0;

 for (i=0; i<re->DestCount; i++) {
 // For each unreachable destination
   rt = rtable.rt_lookup(re->unreachable_dst[i]);
   if ( rt && (rt->rt_hops != INFINITY2) &&
	(rt->rt_nexthop == ih->saddr()) &&
     	(rt->rt_seqno <= re->unreachable_dst_seqno[i]) ) {
	assert(rt->rt_flags == RTF_UP);
	assert((rt->rt_seqno%2) == 0); // is the seqno even?
#ifdef DEBUG
     fprintf(stderr, "%s(%f): %d\t(%d\t%u\t%d)\t(%d\t%u\t%d)\n", __FUNCTION__,CURRENT_TIME,
		     index, rt->rt_dst, rt->rt_seqno, rt->rt_nexthop,
		     re->unreachable_dst[i],re->unreachable_dst_seqno[i],
	             ih->saddr());
#endif // DEBUG
     	rt->rt_seqno = re->unreachable_dst_seqno[i];
     	rt_down(rt);

  /* // Not sure whether this is the right thing to do
   Packet *pkt;
	while((pkt = ifqueue->filter(ih->saddr()))) {
        	drop(pkt, DROP_RTR_MAC_CALLBACK);
     	}*/

     // if precursor list non-empty add to RERR and delete the precursor list
     	if (!rt->pc_empty()) {
     		nre->unreachable_dst[nre->DestCount] = rt->rt_dst;
     		nre->unreachable_dst_seqno[nre->DestCount] = rt->rt_seqno;
     		nre->DestCount += 1;
		  rt->pc_delete();
     	}
   }
 } 

 if (nre->DestCount > 0) {
#ifdef DEBUG
   fprintf(stderr, "%s(%f): %d\t sending RERR...\n", __FUNCTION__, CURRENT_TIME, index);
#endif // DEBUG
   sendError(rerr);
 }
 else {
   Packet::free(rerr);
 }

 Packet::free(p);
}


void
MYPROT::forward(myprot_rt_entry *rt, Packet *p, double delay) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_myprot *ah = HDR_MYPROT(p);
/*********转发计数为0，不再转发********************/
 if(ih->ttl_ == 0) {

#ifdef DEBUG
  fprintf(stderr, "%s: calling drop()\n", __PRETTY_FUNCTION__);
#endif // DEBUG
 
  drop(p, DROP_RTR_TTL);
  return;
 }
/***************向上层传的包***********/
 if (ch->ptype() != PT_MYPROT && ch->direction() == hdr_cmn::UP &&
	((u_int32_t)ih->daddr() == IP_BROADCAST)
		|| (ih->daddr() == here_.addr_)) {
	prot_dumx_->recv(p);
	return;
 }
/********************添加公共包头，分是广播包和是广播包*****************/
 if (rt) {                     
   assert(rt->rt_flags == RTF_UP);
   rt->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
   ch->next_hop_ = rt->rt_nexthop;
   ch->addr_type() = NS_AF_INET;
   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }
 else { // if it is a broadcast packet
   // assert(ch->ptype() == PT_AODV); // maybe a diff pkt type like gaf
   assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
   ch->addr_type() = NS_AF_NONE;
   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }
/*************根据包的类型，调用下层的接受函数***************/
if (ih->daddr() == (nsaddr_t) IP_BROADCAST) {
 // If it is a broadcast packet
   assert(rt == 0);
   if (ch->ptype() == PT_MYPROT) {
     /*
      *  Jitter the sending of AODV broadcast packets by 10ms
      */
     Scheduler::instance().schedule(target_, p,
      				   0.001 * Random::uniform());
   } else {
     Scheduler::instance().schedule(target_, p, 0.);  // No jitter
   }
 }
 else { // Not a broadcast packet 
   if(delay > 0.0) {
     Scheduler::instance().schedule(target_, p, delay);
   }
   else {
   // Not a broadcast packet, no delay, send immediately
     Scheduler::instance().schedule(target_, p, 0.);
   }
 }

}

/****簇内节点向簇首发送路由请求***/
void
MYPROT::sendClusterInRequest(nsaddr_t dst) {
  ////printf("sendClusterInRequest\n");
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_myprot_close_rreq *rq = HDR_MYPROT_CLOSE_RREQ(p);
myprot_rt_entry *rt = rtable.rt_lookup(dst);

 assert(rt);

 /*
  *  Rate limit sending of Route Requests. We are very conservative
  *  about sending out route requests. 
  * 防止之前判断错误，再次判断
  */

 if (rt->rt_flags == RTF_UP) {
   assert(rt->rt_hops != INFINITY2);
   Packet::free((Packet *)p);
   return;
 }
/****再次请求发送的时间没有到，对应之前发送过没有得到回应的情况******/
 if (rt->rt_req_timeout > CURRENT_TIME) {
   Packet::free((Packet *)p);
   return;
 }

 // rt_req_cnt is the no. of times we did network-wide broadcast
 // RREQ_RETRIES is the maximum number we will allow before 
 // going to a long timeout.大于最大请求次数的时候，将缓存中的数据包和该包都丢弃

 if (rt->rt_req_cnt > RREQ_RETRIES) {
   rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
   rt->rt_req_cnt = 0;
 Packet *buf_pkt;
   while ((buf_pkt = rqueue.deque(rt->rt_dst))) {
       drop(buf_pkt, DROP_RTR_NO_ROUTE);
   }
   Packet::free((Packet *)p);
   return;
 }

 // Determine the TTL to be used this time. 
 // Dynamic TTL evaluation - SRD

 rt->rt_req_last_ttl = max(rt->rt_req_last_ttl,rt->rt_last_hop_count);

 if (0 == rt->rt_req_last_ttl) {
 // first time query broadcast
   ih->ttl_ = TTL_START;
 }
 else {
 // Expanding ring search.
   if (rt->rt_req_last_ttl < TTL_THRESHOLD)
     ih->ttl_ = rt->rt_req_last_ttl + TTL_INCREMENT;
   else {
   // network-wide broadcast
     ih->ttl_ = NETWORK_DIAMETER;
     rt->rt_req_cnt += 1;
   }
 }

 // remember the TTL used  for the next time
 rt->rt_req_last_ttl = ih->ttl_;
 // PerHopTime is the roundtrip time per hop for route requests.
 // The factor 2.0 is just to be safe .. SRD 5/22/99
 // Also note that we are making timeouts to be larger if we have 
 // done network wide broadcast before. 

 rt->rt_req_timeout = 2.0 * (double) ih->ttl_ * PerHopTime(rt);      //这里需要探究一下。。。。。。。。。。
 if (rt->rt_req_cnt > 0)
   rt->rt_req_timeout *= rt->rt_req_cnt;
 rt->rt_req_timeout += CURRENT_TIME;

 // Don't let the timeout to be too large, however .. SRD 6/8/99
 if (rt->rt_req_timeout > CURRENT_TIME + MAX_RREQ_TIMEOUT)
   rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
 rt->rt_expire = 0;

	

 // Fill out the RREQ packet 
 // ch->uid() = 0;
 ch->ptype() = PT_MYPROT;
 ch->size() = IP_HDR_LEN + rq->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_NONE;
 ch->prev_hop_ = index;          

 ih->saddr() = index;
 ih->daddr() = IP_BROADCAST;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;

 // Fill up some more fields. 
 rq->cr_type = MYPROT_CLOSE_RREQ;
 //rq->rq_hop_count = 1;
 rq->rq_bcast_id = 0;
 rq->rq_dst = dst;
 rq->rq_dst_seqno = (rt ? rt->rt_seqno : 0);
// rq->rq_src = index;
 seqno += 2;
 assert ((seqno%2) == 0);
 rq->rq_src_seqno = seqno;     //这个seq需要注意一下！！！！！！
 rq->rq_timestamp = CURRENT_TIME;
 Node *thisnode;
 thisnode = Node::get_node_by_address(index);
 int flag=((MobileNode *)thisnode)->clustre_head_flag;
 if(flag==index)
 {
   sendRequest(p);
 }
 else
    Scheduler::instance().schedule(target_, p, 0.0);

}

void   MYPROT::sendRequest(Packet *p)
{
    Packet *pkt_ = Packet::alloc();
    struct hdr_cmn *ch = HDR_CMN(pkt_);
    struct hdr_ip *ih = HDR_IP(pkt_);
    struct hdr_myprot_request *rq = HDR_MYPROT_RREQ(pkt_);
    struct hdr_myprot_close_rreq *crq = HDR_MYPROT_CLOSE_RREQ(p);
    struct hdr_ip *ihx= HDR_IP(p);


    ch->ptype() = PT_MYPROT;
    ch->size() =(IP_HDR_LEN + rq->size());
    ch->iface() = -2;
    ch->error() = 0;
    ch->addr_type() = NS_AF_NONE;
    ch->prev_hop_ = index;          

    ih->saddr() = index;
    ih->daddr() = IP_BROADCAST;
    ih->sport() = RT_PORT;
    ih->dport() = RT_PORT;
    ih->ttl_=ihx->ttl_+1;       //recv的时候先手减掉了1,加回来    

    rq->rq_type = MYPROT_RREQ;
    rq->rq_bcast_id = bid++;    //所有的共用一个没啥问题吧？？？
    rq->rq_cluster_count=1;
    rq->cluster_ID_line_num=line_num;
    rq->cluster_ID_column_num=column_num;
    rq->src_head=index;
    rq->rq_dst = crq->rq_dst;
    rq->rq_dst_seqno = crq->rq_dst_seqno;
    rq->rq_src=ihx->saddr();
    rq->rq_src_seqno = crq->rq_src_seqno;
    //printf("rq->rq_src_seqno=%d\n",rq->rq_src_seqno);
    //rq->rq_timestamp = CURRENT_TIME;

    Scheduler::instance().schedule(target_, pkt_, 0.0);

}



void
MYPROT::sendReply(nsaddr_t ipdst, u_int32_t hop_count, nsaddr_t rpsrc,
                u_int32_t rpseq, u_int32_t lifetime) 
{
  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_myprot_reply *rp = HDR_MYPROT_RREP(p);
  myprot_head_rt_entry *hrt = head_rt_lookup(ipdst);
  //assert(rt);
 
 rp->rp_type = MYPROT_RREP;
 //rp->rp_flags = 0x00;
 rp->rp_hop_count = hop_count;
 rp->rp_dst = ipdst;            //rp消息的目的节点，rq消息的源节点
 rp->rp_src_seqno = rpseq;
 rp->rp_src = rpsrc;            //rq消息的目的节点
 rp->next_cluster_head=hrt->rt_nexthop;
 rp->rp_lifetime = lifetime;
 
 //printf("node<%d>:begin send reply\n",index);
 if(!fill_send_rrep_packet(p))      //用来填充路由中的关键信息，即在陆游中的节点
  {
		Packet::free(p);
		return;
	}


 ch->ptype() = PT_MYPROT;
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_NONE;
 //ch->next_hop_ = hrt->rt_nexthop;
 ch->prev_hop_ = index;          
 //ch->direction() = hdr_cmn::DOWN;

 ih->saddr() = index;
 ih->daddr() = IP_BROADCAST;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;
 ih->ttl_ = 255;

 
//printf("node<%d>: send reply \n",index);

 Scheduler::instance().schedule(target_, p, 0.);
}




void
MYPROT::sendError(Packet *p, bool jitter) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_myprot_error *re = HDR_MYPROT_ERROR(p);
    
#ifdef ERROR
fprintf(stderr, "sending Error from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG

 re->re_type = MYPROT_ERROR;
 //re->reserved[0] = 0x00; re->reserved[1] = 0x00;
 // DestCount and list of unreachable destinations are already filled

 // ch->uid() = 0;
 ch->ptype() = PT_MYPROT;
 ch->size() = IP_HDR_LEN + re->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_NONE;
 ch->next_hop_ = 0;
 ch->prev_hop_ = index;          
 ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction

 ih->saddr() = index;
 ih->daddr() = IP_BROADCAST;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;
 ih->ttl_ = 1;

 // Do we need any jitter? Yes
 if (jitter)
 	Scheduler::instance().schedule(target_, p, 0.01*Random::uniform());
 else
 	Scheduler::instance().schedule(target_, p, 0.0);

}
void MYPROT::calculate_cellular_area()//根据该节点的地理位置信息，计算该节点所在的簇的编号(line_num, column_num)，以及该节点所在的小区域
{
	////printf("calculate_cellular_area() in MobileNode \n");
	POINT_XY points[3];	//定义三个蜂窝的中心节点，用于比较本节点离哪个中心最近
	//double lx=0,ly=0,lz=0;
	Node *thisnode;
	thisnode = Node::get_node_by_address(index);
	((MobileNode *)thisnode)->getLoc(&location_x, &location_y, &location_z);
	int cx=0,cy=0;
	cx=(int)(location_x/unitx);//横轴位置       double强制转换成int类型
	cy=(int)(location_y/unity);//纵轴位置

	////printf("lx=%lf,ly=%lf,unitx=%lf,unity=%lf,cx=%d,cy=%d \n",lx,ly,unitx,unity,cx,cy);
	points[0].x = (double)(unitx * cx);
	points[1].x = (double)(unitx * (cx+1));
	points[2].x = (double)(unitx * (cx+0.5));
	
	//根据cy是否是偶数，决定三个点的纵坐标
	if(cy%2 == 0)
	{
		//偶数时，三个点组成正立三角
		points[0].y = (int)(unity * cy);
		points[1].y = (int)(unity * cy);
	    points[2].y = (int)(unity * (cy+1));
	}
	else
	{
		//奇数时，三个点组成倒立三角
	    points[0].y = (int)(unity * (cy+1));
		points[1].y = (int)(unity * (cy+1));
		points[2].y = (int)(unity * cy);
	}
	double dist=0.0;
	double mindist=0.0;
	int point_index=0;//所在蜂窝的索引
	//求出节点距离哪个中心点最近
	for(int i=0;i<3;i++)
	{
		dist=(location_x-points[i].x)*(location_x-points[i].x)+(location_y-points[i].y)*(location_y-points[i].y);
		if(i==0)
		{
			mindist=dist;
			point_index=0;
	 	}
		else
		{
			if(dist<mindist)
			{
				mindist=dist;
				point_index=i;
			}
		}
	}
	////printf("mindist=%f \n",mindist);
	//distance_to_center=sqrt(mindist);
	////printf("distance_to_center=%f \n",distance_to_center);
	////printf("%d \n",point_index);
	if(cy%2 == 0)
	{
		switch(point_index)
		{
			case 0:
				{
					line_num=cx;
					column_num=cy;
					break;
				}
			case 1:
				{
					line_num=cx+1;
					column_num=cy;
					break;
				}
			case 2:
				{
					line_num=cx;
					column_num=cy+1;
					break;
				}
		}
	}
	else
	{
		switch(point_index)
		{
			case 0:
				{
					line_num=cx;
					column_num=cy+1;
					break;
				}
			case 1:
				{
					line_num=cx+1;
					column_num=cy+1;
					break;
				}
			case 2:
				{
					line_num=cx;
					column_num=cy;
					break;
				}
		}

	}
	double tmp_x=0;
	double tmp_y=0;
	//for(int i=0;i<3;i++)
	//{
		////printf("[%d] ,x=%f, y=%f\n",i,points[i].x,points[i].y);
	//}
	tmp_x=location_x-points[point_index].x;
	tmp_y=location_y-points[point_index].y;
	////printf("tmp_x=%f,tmp_y=%f \n",tmp_x,tmp_y);
	if(tmp_x-0 > 1.0e-9)
	{
		if(tmp_y >= sqrt(3)/3.0*tmp_x)
			slot_area_num=0;
		else if(tmp_y <= sqrt(3)/3.0*tmp_x && tmp_y >= -sqrt(3)/3.0*tmp_x)
			slot_area_num=1;
		else
			slot_area_num=2;
	}
	else if(tmp_x-0 < -1.0e-9)
	{
		if(tmp_y <= sqrt(3)/3.0*tmp_x)
			slot_area_num=3;
		else if(tmp_y >= sqrt(3)/3.0*tmp_x && tmp_y <= -sqrt(3)/3.0*tmp_x)
			slot_area_num=4;
		else
			slot_area_num=5;
	}
	else //if(tmp_x==0 && tmp_y==0)
	{
		slot_area_num=0;
	}
}


void    MYPROT::calculate_distance()
{
  POINT points[3];	
	Node *thisnode;
	thisnode = Node::get_node_by_address(index);
	((MobileNode *)thisnode)->getLoc(&location_x, &location_y, &location_z);
	int cx=(int)(location_x/unitx);
	int cy=(int)(location_y/unity);
	
	points[0].x = (double)(unitx * cx);
	points[1].x = (double)(unitx * (cx+1));
	points[2].x = (double)(unitx * (cx+0.5));
	
	
	if(cy%2 == 0)
	{
		
		points[0].y = points[1].y = (int)(unity * cy);
	    points[2].y = (int)(unity * (cy+1));
	}
	else
	{
		
	    points[0].y = points[1].y = (int)(unity * (cy+1));
		points[2].y = (int)(unity * cy);
	}
	double dist;
	double mindist;
	int point_index;
	for(int i=0;i<3;i++)
	{
		dist=(location_x-points[i].x)*(location_x-points[i].x)+(location_y-points[i].y)*(location_y-points[i].y);
		if(i==0)
		{
			mindist=dist;
	 	}
		else
		{
			if(dist<mindist)
			{
				mindist=dist;
				point_index=i;
			}
		}
		
	}
	dist_to_center=mindist;
}
void    MYPROT::calculate_cellular_area(double lx,double ly)
{
  POINT points[3];	
	int cx=(int)(lx/unitx);
	int cy=(int)(ly/unity);
	
	points[0].x = (double)(unitx * cx);
	points[1].x = (double)(unitx * (cx+1));
	points[2].x = (double)(unitx * (cx+0.5));
	
	
	if(cy%2 == 0)
	{
		
		points[0].y = points[1].y = (int)(unity * cy);
	       	points[2].y = (int)(unity * (cy+1));
	}
	else
	{
		
	      	points[0].y = points[1].y = (int)(unity * (cy+1));
		points[2].y = (int)(unity * cy);
	}
  /*//printf("points[0].x=%f\n",points[0].x);
  //printf("points[0].y=%f\n",points[0].y);
  //printf("points[1].x=%f\n",points[1].x);
  //printf("points[1].y=%f\n",points[1].y);
  //printf("points[2].x=%f\n",points[2].x);
  //printf("points[2].y=%f\n",points[2].y);*/
	double dist;
	double mindist;
	int point_index=0;
	
	for(int i=0;i<3;i++)
	{
		dist=(lx-points[i].x)*(lx-points[i].x)+(ly-points[i].y)*(ly-points[i].y);
		if(i==0)
		{
			mindist=dist;
	 	}
		else
		{
			if(dist<mindist)
			{
				mindist=dist;
				point_index=i;
			}
		}
		
	}
	if(cy%2 == 0)
	{
		switch(point_index)
		{
			case 0:
				{
					line_num=cx;
					column_num=cy;
				}
			case 1:
				{
					line_num=cx+1;
					column_num=cy;
				}
			case 2:
				{
					line_num=cx;
					column_num=cy+1;
				}
		}
	}
	else
	{
		switch(point_index)
		{
			case 0:
				{
					line_num=cx;
					column_num=cy+1;
				}
			case 1:
				{
					line_num=cx+1;
					column_num=cy+1;
				}
			case 2:
				{
					line_num=cx;
					column_num=cy;
				}
		}

	}
	double tmp_x=lx-points[point_index].x;
	double tmp_y=ly-points[point_index].y;
	////printf("tmp_x=%f,tmp_y=%f \n",tmp_x,tmp_y);
	if(tmp_x-0 > 1.0e-9)
	{
		if(tmp_y >= sqrt(3)/3.0*tmp_x)
			slot_area_num=0;
		else if(tmp_y <= sqrt(3)/3.0*tmp_x && tmp_y >= -sqrt(3)/3.0*tmp_x)
			slot_area_num=1;
		else
			slot_area_num=2;
	}
	else if(tmp_x-0 < -1.0e-9)
	{
		if(tmp_y <= sqrt(3)/3.0*tmp_x)
			slot_area_num=3;
		else if(tmp_y >= sqrt(3)/3.0*tmp_x && tmp_y <= -sqrt(3)/3.0*tmp_x)
			slot_area_num=4;
		else
			slot_area_num=5;
	}
	else //if(tmp_x==0 && tmp_y==0)
	{
		slot_area_num=0;
	}
}
int  MYPROT::calculate_connectedness(nsaddr_t id)
{
  double dist;
  MY_NeighborCluster *nc = nc_lookup(id);
  if(nc==0)
    return 0;
  Node *thisnode;
  thisnode = Node::get_node_by_address(index);
  MY_Neighbor *nb = ((MobileNode *)thisnode)->mnbhead.lh_first;
  for(; nb; nb = nb->mynb_link.le_next) 
  {
    for (int i =0;i<nc->num_cluster_in_node_;i++){
        if(nb->line_num==line_num&&nb->column_num==column_num)
            {
              ////printf("nb<%d>,nb->x_local=%f,nb->y_local=%f\n",nb->nb_addr,nb->x_local,nb->y_local);
              ////printf("ncb<%d>,nc->x_local=%f,nc->y_local=%f\n",nc->cluster_in_node_[i].nb_addr,nc->cluster_in_node_[i].x_local,nc->cluster_in_node_[i].y_local);
              dist=(nb->x_local-nc->cluster_in_node_[i].x_local)*(nb->x_local-nc->cluster_in_node_[i].x_local)+(nb->y_local-nc->cluster_in_node_[i].y_local)*(nb->y_local-nc->cluster_in_node_[i].y_local);
              if(dist<(250*250))
                  return 1;
            }
    }
  }

  
 return 0;

}

int MYPROT::fill_send_rrep_packet(Packet *p)
{

  int FLAG_=0;
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_myprot_reply *rp = HDR_MYPROT_RREP(p);
  struct hdr_ip *ih = HDR_IP(p);
  myprot_head_rt_entry *hrt = head_rt_lookup(rp->rp_dst);
  ////printf("hrt=%d,hrt->rt_nexthop=%d\n",hrt,hrt->rt_nexthop);
  MY_NeighborCluster *nc = nc_lookup(hrt->rt_nexthop);    //做再次判断，防止之前判断错误
  if(nc==0)
    return 0;
  //接下来计算邻簇节点直接和本簇内的目的节点是否有连接
  int flag3=0;
  double dist_to_next_clusterhead=0;
  double temp=0;
  double nc_local_x=0;
  double nc_local_y=nc->column_num*unity;
  int flag=0,flag1=0;
  if(nc->column_num%2)
    nc_local_x=nc->line_num*unitx;
  else
    nc_local_x=nc->line_num*unitx+unitx/2;
  MY_Neighbor *nb=nb_lookup(rp->rp_src );   //rq目的节点
  rp->last_cluster_node_addr=rp->rp_src;
  rp->cluster_in_node_addr[0]=rp->rp_src;
  double dist=0;
  for (int i =0;i<nc->num_cluster_in_node_;i++)
  {
    dist=(nb->x_local-nc->cluster_in_node_[i].x_local)*(nb->x_local-nc->cluster_in_node_[i].x_local)+(nb->y_local-nc->cluster_in_node_[i].y_local)*(nb->y_local-nc->cluster_in_node_[i].y_local);
    if(dist<(250*250))
    {
      rp->cluster_in_node_addr[0]=rp->rp_src;
      rp->node_num_in_route=1;
      ch->size() = (IP_HDR_LEN + 10*sizeof(u_int32_t));
      rp->rp_hop_count = 0;
      //rp->out_cluster_node_location_x=nb->x_local;
      //rp->out_cluster_node_location_y=nb->y_local;
      ////printf("one hop \n");
      flag3=1;
    }
      
  }
  //接下来计算可不可以通过一跳到邻居簇
  if(flag3==0)
  {
    Node *thisnode;
    thisnode = Node::get_node_by_address(index);
    printf("node<%d>\n",index);
    MY_Neighbor *nb1 = ((MobileNode *)thisnode)->mnbhead.lh_first;     //计算自己簇内的节点是和目的节点有连接
    for(; nb1; nb1 = nb1->mynb_link.le_next) {                          //并且和邻簇的节点有连接
        
        if(nb1->line_num==line_num&&nb1->column_num==column_num)
            {
              printf("nb1<%d>,x=%f,y=%f in \n",nb1->nb_addr,nb1->x_local,nb1->y_local);
              //目的节点和簇内节点的连接计算
              dist=(nb->x_local-nb1->x_local)*(nb->x_local-nb1->x_local)+(nb->y_local-nb1->y_local)*(nb->y_local-nb1->y_local);
              if(dist<(250*250))
              {
                for (int i =0;i<nc->num_cluster_in_node_;i++)
                {
                  //选择的簇内节点和邻簇有没有连接
                  printf("ncb<%d>,nc->x_local=%f,nc->y_local=%f\n",nc->cluster_in_node_[i].nb_addr,nc->cluster_in_node_[i].x_local,nc->cluster_in_node_[i].y_local);
                  dist=(nb1->x_local-nc->cluster_in_node_[i].x_local)*(nb1->x_local-nc->cluster_in_node_[i].x_local)+(nb1->y_local-nc->cluster_in_node_[i].y_local)*(nb1->y_local-nc->cluster_in_node_[i].y_local);
                  if(dist<(250*250))
                  {
                    printf("ncb<%d>\n",nc->cluster_in_node_[i].nb_addr);
                    rp->node_num_in_route=2;
                    flag=1;
            
                  }   
                }
                  
                //选择离邻居簇首距离最近的本簇节点
                
                dist_to_next_clusterhead=(nc_local_x-nb1->x_local)*(nc_local_x-nb1->x_local)+(nc_local_y-nb1->y_local)*(nc_local_y-nb1->y_local);
                printf("temp=%f,dist_to_next_clusterhead=%f\n",temp,dist_to_next_clusterhead);
                if(temp==0||dist_to_next_clusterhead<temp)
                {
                  temp=dist_to_next_clusterhead;
                  rp->cluster_in_node_addr[1]=nb1->nb_addr;
                  printf("rp->cluster_in_node_addr[1]=%d\n",rp->cluster_in_node_addr[1]);
                  //rp->out_cluster_node_location_x=nb1->x_local;
                  //rp->out_cluster_node_location_y=nb1->y_local;
                }
              }
            }
    }
    temp=0;
    dist_to_next_clusterhead=0;
    //如果一跳到不了，计算两跳
    if(flag==0)
    {
      
      
      MY_Neighbor *nb2=nb_lookup(rp->cluster_in_node_addr[1]);//目的节点后的第一个点
	if(nb2==0)
		return 0;
      MY_Neighbor *nb1 = ((MobileNode *)thisnode)->mnbhead.lh_first;     //计算自己簇内的节点是和目的节点有连接
      for(; nb1; nb1 = nb1->mynb_link.le_next) 
      {
        if(nb1->line_num==line_num&&nb1->column_num==column_num)
        {
          dist=(nb2->x_local-nb1->x_local)*(nb2->x_local-nb1->x_local)+(nb2->y_local-nb1->y_local)*(nb2->y_local-nb1->y_local);
          if(dist<(250*250))
          {
            for (int i =0;i<nc->num_cluster_in_node_;i++)
            {
              //选择的簇内节点和邻簇有没有连接
              dist=(nb1->x_local-nc->cluster_in_node_[i].x_local)*(nb1->x_local-nc->cluster_in_node_[i].x_local)+(nb1->y_local-nc->cluster_in_node_[i].y_local)*(nb1->y_local-nc->cluster_in_node_[i].y_local);
              if(dist<(250*250))
              {
                //选择离邻居簇首距离最近的本簇节点
                ch->size() =(IP_HDR_LEN + 12*sizeof(u_int32_t));
                rp->node_num_in_route=3;
                rp->rp_hop_count = 2;
                flag1=1;
                
                dist_to_next_clusterhead=(nc_local_x-nb1->x_local)*(nc_local_x-nb1->x_local)+(nc_local_y-nb1->y_local)*(nc_local_y-nb1->y_local);
                if(temp==0||dist_to_next_clusterhead<temp)
                {
                  temp=dist_to_next_clusterhead;
                  rp->cluster_in_node_addr[2]=nb1->nb_addr;
                  //rp->out_cluster_node_location_x=nb1->x_local;
                  //rp->out_cluster_node_location_y=nb1->y_local;
                }
              }   
            }
              
          }
          
        }
      

      }

    }
    else
    {
      ch->size() = (IP_HDR_LEN +11 *sizeof(u_int32_t));
      rp->node_num_in_route=2;
      rp->rp_hop_count = 1;
      
    }
  }
  MY_Neighbor *nb3=nb_lookup(rp->cluster_in_node_addr[rp->node_num_in_route-1]);
  rp->out_cluster_node_location_x=nb3->x_local;
  rp->out_cluster_node_location_y=nb3->y_local;
  for(int i=0;i<rp->node_num_in_route;i++)
  {
    if(rp->cluster_in_node_addr[i]==index)
    {
      if(i==0)
      {
        ////printf("head is the dest\n");
        break;
      }
      else
      {
        myprot_rt_entry *rt;
        rt = rtable.rt_lookup(rp->rp_src);
      

        if(rt == 0) {
          rt = rtable.rt_add(rp->rp_src);
        }
        if ( (rt->rt_seqno < rp->rp_src_seqno) ||   // newer route 
          ((rt->rt_seqno == rp->rp_src_seqno) &&  
          (rt->rt_hops > 1)) ) { // shorter or better route
      
      // Update the rt entry 
      rt_update(rt, rp->rp_src_seqno, 1,
        rp->cluster_in_node_addr[i-1], CURRENT_TIME + rp->rp_lifetime);

      // reset the soft state
      rt->rt_req_cnt = 0;
      rt->rt_req_timeout = 0.0; 
      rt->rt_req_last_ttl = 1;


    ////printf("node<%d> in route ,next hop=%d\n",index,rt->rt_nexthop);
      Packet *buf_pkt;
      while((buf_pkt = rqueue.deque(rt->rt_dst))) {
        if(rt->rt_hops != INFINITY2) {
              assert (rt->rt_flags == RTF_UP);
        // Delay them a little to help ARP. Otherwise ARP 
        // may drop packets. -SRD 5/23/99
          forward(rt, buf_pkt, 0.0);
          
              }
            }

        }
      }
    
    }
  }
  for(int i=0;i<rp->node_num_in_route;i++)
  {
    ////printf("route:\n");
    ////printf("node<%d>\n",rp->cluster_in_node_addr[i]);
  }
  ////printf("rp->out_cluster_node_location_x=%f,rp->out_cluster_node_location_y=%f\n",rp->out_cluster_node_location_x,rp->out_cluster_node_location_y);
  return 1;
}
int MYPROT::fill_recv_rrep_packet(Packet *p)
{
  ////printf("node <%d>in fill_recv_rrep_packet\n",index);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_myprot_reply *rp= HDR_MYPROT_RREP(p);
  struct hdr_cmn *ch = HDR_CMN(p);
  myprot_head_rt_entry *hrt = head_rt_lookup(rp->rp_dst);
  MY_NeighborCluster *nc = nc_lookup(hrt->rt_nexthop);//取下一簇的簇首并计算寺坐标



  rp->last_cluster_node_addr=rp->cluster_in_node_addr[rp->node_num_in_route-1];
  //清空重新计算
  rp->cluster_in_node_addr[0]=0;
  rp->cluster_in_node_addr[1]=0; 
  rp->cluster_in_node_addr[2]=0;

  int flag=0;//簇中一个点在路由中的标志
  int flag1=0;//簇中两个点在路由中的标志
  int flag2=0;//最后看三个点成功不
  double dist=0,temp=0,dist_to_next_clusterhead=0;
  double nc_local_x=0;
  double nc_local_y=nc->column_num*unity;
  if(nc->column_num%2)
    nc_local_x=nc->line_num*unitx;
  else
    nc_local_x=nc->line_num*unitx+unitx/2;
  //先做出第一个节点
  Node *thisnode;
  thisnode = Node::get_node_by_address(index);
  MY_Neighbor *nb1 = ((MobileNode *)thisnode)->mnbhead.lh_first;     //计算自己簇内的节点是和目的节点有连接
  for(; nb1; nb1 = nb1->mynb_link.le_next) {                          //并且和邻簇的节点有连接
      if(nb1->line_num==line_num&&nb1->column_num==column_num)
          {
            //上一个簇最后一个节点和簇内节点的连接计算
            ////printf("nb<%d> :x=%f,y=%f in fill_recv_rrep_packet\n",nb1->nb_addr,nb1->x_local,nb1->y_local);
            ////printf("rp->out_cluster_node_location_x=%f,rp->out_cluster_node_location_y=%f\n",rp->out_cluster_node_location_x,rp->out_cluster_node_location_y);
            dist=(rp->out_cluster_node_location_x-nb1->x_local)*(rp->out_cluster_node_location_x-nb1->x_local)+(rp->out_cluster_node_location_y-nb1->y_local)*(rp->out_cluster_node_location_y-nb1->y_local);
            ////printf("dist to last cluster =%f\n",dist);
            if(dist<(250.0*250.0))
            {
              for (int i =0;i<nc->num_cluster_in_node_;i++)
              {
                //选择的簇内节点和邻簇有没有连接
                dist=(nb1->x_local-nc->cluster_in_node_[i].x_local)*(nb1->x_local-nc->cluster_in_node_[i].x_local)+(nb1->y_local-nc->cluster_in_node_[i].y_local)*(nb1->y_local-nc->cluster_in_node_[i].y_local);
                if(dist<(250*250))
                {
                  flag=1;
                  ////printf("node <%d> flag=%d in recv rrep\n",nb1->nb_addr,flag);
                }   
              }
                
            
              //选择离邻居簇首距离最近的本簇节点
              dist_to_next_clusterhead=(nc_local_x-nb1->x_local)*(nc_local_x-nb1->x_local)+(nc_local_y-nb1->y_local)*(nc_local_y-nb1->y_local);
              if(temp==0||dist_to_next_clusterhead<temp)
              {
                temp=dist_to_next_clusterhead;
                rp->cluster_in_node_addr[0]=nb1->nb_addr;
                ////printf("flag node =%d flag\n",rp->cluster_in_node_addr[0]);
                //rp->out_cluster_node_location_x=nb1->x_local;
                //rp->out_cluster_node_location_y=nb1->y_local;
              }
            }
          }
  }
  if(flag)
  {
    ch->size() = (IP_HDR_LEN + 10*sizeof(u_int32_t));
    rp->node_num_in_route=1;
    rp->rp_hop_count = rp->rp_hop_count+1;
    ////printf("the last node =%d flag\n",rp->cluster_in_node_addr[rp->node_num_in_route-1]);
    //return 1;
  }
  //计算第二个点
  else
  {
    temp=0;
    dist_to_next_clusterhead=0;
    
    MY_Neighbor *nb2=nb_lookup(rp->cluster_in_node_addr[0]);//第一个点
	if(nb2==0)
		return 0;
    ////printf("nb2<%d> :x=%f,y=%f in fill_recv_rrep_packet\n",nb2->nb_addr,nb2->x_local,nb2->y_local);
    MY_Neighbor *nb3 = ((MobileNode *)thisnode)->mnbhead.lh_first;     //计算自己簇内的节点是和第一个点有连接
    for(; nb3; nb3 = nb3->mynb_link.le_next) 
    {
      ////printf("nb3<%d> :x=%f,y=%f in fill_recv_rrep_packet\n",nb3->nb_addr,nb3->x_local,nb3->y_local);
      if(nb3->line_num==line_num&&nb3->column_num==column_num)
      {
        dist=(nb2->x_local-nb3->x_local)*(nb2->x_local-nb3->x_local)+(nb2->y_local-nb3->y_local)*(nb2->y_local-nb3->y_local);
        if(dist<(250*250))
        {
          ////printf("dist node =%d flag\n",nb3->nb_addr);
          for (int i =0;i<nc->num_cluster_in_node_;i++)
          {
            //选择的簇内节点和邻簇有没有连接
            dist=(nb3->x_local-nc->cluster_in_node_[i].x_local)*(nb3->x_local-nc->cluster_in_node_[i].x_local)+(nb3->y_local-nc->cluster_in_node_[i].y_local)*(nb3->y_local-nc->cluster_in_node_[i].y_local);
            if(dist<(250*250))
            {
              //选择离邻居簇首距离最近的本簇节点
              //rp->node_num_in_route=2;
              //ch->size() =(IP_HDR_LEN + 10*sizeof(u_int32_t));
              //rp->rp_hop_count =rp->rp_hop_count+2;
              flag1=1;

            }
              dist_to_next_clusterhead=(nc_local_x-nb3->x_local)*(nc_local_x-nb3->x_local)+(nc_local_y-nb3->y_local)*(nc_local_y-nb3->y_local);
              if(temp==0||dist_to_next_clusterhead<temp)
              {
                temp=dist_to_next_clusterhead;
                rp->cluster_in_node_addr[1]=nb3->nb_addr;
                ////printf("rp->cluster_in_node_addr[1]=%d\n",rp->cluster_in_node_addr[1]);
                //rp->out_cluster_node_location_x=nb3->x_local;
                //rp->out_cluster_node_location_y=nb3->y_local;
              }   
          }
            
        }
        
      }
    }
    
    if(flag1)
    {
      ch->size() = (IP_HDR_LEN + 11*sizeof(u_int32_t));
      rp->node_num_in_route=2;
      rp->rp_hop_count = rp->rp_hop_count+2;
      ////printf("the last node =%d flag1\n",rp->cluster_in_node_addr[rp->node_num_in_route-1]);
      //return 1;
    }
    //计算第三个点
    else
    {
      temp=0;
      dist_to_next_clusterhead=0;
      MY_Neighbor *nb4=nb_lookup(rp->cluster_in_node_addr[1]);//第二个点
	if(nb4==0)
		return 0;
      ////printf("nb4<%d>,x=%f,y=%f in dist\n",nb4->nb_addr,nb4->x_local,nb4->y_local);
      MY_Neighbor *nb5 = ((MobileNode *)thisnode)->mnbhead.lh_first;     //计算自己簇内的节点是和第二个点有连接
      for(; nb5; nb5 = nb5->mynb_link.le_next) 
      {
        ////printf("nb5<%d>,x=%f,y=%f in dist\n",nb5->nb_addr,nb5->x_local,nb5->y_local);
        if(nb5->line_num==line_num&&nb5->column_num==column_num)
        {
          dist=(nb5->x_local-nb4->x_local)*(nb5->x_local-nb4->x_local)+(nb5->y_local-nb4->y_local)*(nb5->y_local-nb4->y_local);
          if(dist<(250*250))
          {
            for (int i =0;i<nc->num_cluster_in_node_;i++)
            {
              //选择的簇内节点和邻簇有没有连接
              dist=(nb5->x_local-nc->cluster_in_node_[i].x_local)*(nb5->x_local-nc->cluster_in_node_[i].x_local)+(nb5->y_local-nc->cluster_in_node_[i].y_local)*(nb5->y_local-nc->cluster_in_node_[i].y_local);
              if(dist<(250*250))
              {
                //选择离邻居簇首距离最近的本簇节点
                //rp->node_num_in_route=2;
                //ch->size() =(IP_HDR_LEN + 10*sizeof(u_int32_t));
                //rp->rp_hop_count =rp->rp_hop_count+2;
                flag2=1;
                
                dist_to_next_clusterhead=(nc_local_x-nb5->x_local)*(nc_local_x-nb5->x_local)+(nc_local_y-nb5->y_local)*(nc_local_y-nb5->y_local);
                if(temp==0||dist_to_next_clusterhead<temp)
                {
                  temp=dist_to_next_clusterhead;
                  rp->cluster_in_node_addr[2]=nb5->nb_addr;
                  //rp->out_cluster_node_location_x=nb5->x_local;
                  //rp->out_cluster_node_location_y=nb5->y_local;
                }
              }   
            }
              
          }
          
        }
      

      }
      if(flag2)
      {
        ch->size() = (IP_HDR_LEN + 12*sizeof(u_int32_t));
        rp->node_num_in_route=3;
        rp->rp_hop_count = rp->rp_hop_count+3;
        ////printf("the last node =%d flag2\n",rp->cluster_in_node_addr[rp->node_num_in_route-1]);
      }
    }
  }
  ////printf("the last node =%d\n",rp->cluster_in_node_addr[rp->node_num_in_route-1]);
  MY_Neighbor *nb6=nb_lookup(rp->cluster_in_node_addr[rp->node_num_in_route-1]);
  //printf("nb6=%d\n",nb6);
  rp->out_cluster_node_location_x=nb6->x_local;
  rp->out_cluster_node_location_y=nb6->y_local;
  for(int i=0;i<rp->node_num_in_route;i++)
  {
    if(rp->cluster_in_node_addr[i]==index)
    {
      myprot_rt_entry *rt;
        rt = rtable.rt_lookup(rp->rp_src);
      

        if(rt == 0) {
          rt = rtable.rt_add(rp->rp_src);
        }
        if ( (rt->rt_seqno < rp->rp_src_seqno) ||   // newer route 
          ((rt->rt_seqno == rp->rp_src_seqno) &&  
          (rt->rt_hops > rp->rp_hop_count-(rp->node_num_in_route-i-1))) ) { // shorter or better route
          if(i==0)
          {
            ////printf("head is the dest\n");
            rt_update(rt, rp->rp_src_seqno, rp->rp_hop_count-(rp->node_num_in_route-i-1),
            rp->last_cluster_node_addr, CURRENT_TIME + rp->rp_lifetime);
          }
          else
          {
            
          
          // Update the rt entry 
          rt_update(rt, rp->rp_src_seqno, rp->rp_hop_count-(rp->node_num_in_route-i-1),
            rp->cluster_in_node_addr[i-1], CURRENT_TIME + rp->rp_lifetime);
          }

          // reset the soft state
          rt->rt_req_cnt = 0;
          rt->rt_req_timeout = 0.0; 
          rt->rt_req_last_ttl = 1;

//printf("node<%d> in route ,next hop=%d\n",index,rt->rt_nexthop);
        
          Packet *buf_pkt;
          while((buf_pkt = rqueue.deque(rt->rt_dst))) {
            if(rt->rt_hops != INFINITY2) {
                  assert (rt->rt_flags == RTF_UP);
            // Delay them a little to help ARP. Otherwise ARP 
            // may drop packets. -SRD 5/23/99
              forward(rt, buf_pkt, 0.0);
              
                  }
                }

            
      }
    
    }
  }
  for(int i=0;i<rp->node_num_in_route;i++)
  {
    //printf("route:\n");
    //printf("node<%d>\n",rp->cluster_in_node_addr[i]);
  }
  //printf("rp->out_cluster_node_location_x=%f,rp->out_cluster_node_location_y=%f\n",rp->out_cluster_node_location_x,rp->out_cluster_node_location_y);
    return 1;
}


int MYPROT::fill_end_rrep_packet(Packet *p)
{
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_myprot_reply *rp= HDR_MYPROT_RREP(p);
  struct hdr_cmn *ch = HDR_CMN(p);

  rp->last_cluster_node_addr=rp->cluster_in_node_addr[rp->node_num_in_route-1];
  //清空重新计算
  rp->cluster_in_node_addr[0]=0;
  rp->cluster_in_node_addr[1]=0; 
  rp->cluster_in_node_addr[2]=0;

  int flag=0;//簇中一个点在路由中的标志
  int flag1=0;//簇中两个点在路由中的标志
  int flag2=0;//最后看三个点成功不
  double dist=0,temp=0,dist_to_next_clusterhead=0;



  //取目的节点
  MY_Neighbor *nb=nb_lookup(rp->rp_dst);
  //计算和上一簇最后一个节点和目的节点是不是直接相连
  dist=(rp->out_cluster_node_location_x-nb->x_local)*(rp->out_cluster_node_location_x-nb->x_local)+(rp->out_cluster_node_location_y-nb->y_local)*(rp->out_cluster_node_location_y-nb->y_local);
  if(dist<(250*250))
  {
    rp->cluster_in_node_addr[0]=nb->nb_addr;
    //rp->out_cluster_node_location_x=nb->x_local;
    //rp->out_cluster_node_location_y=nb->y_local;
    flag=1;
    //printf("flag =1 in fill_end_rrep_packet\n");
  }
  if(flag)
  {
    ch->size() = (IP_HDR_LEN + 10*sizeof(u_int32_t));
    rp->node_num_in_route=1;
    rp->rp_hop_count = rp->rp_hop_count+1;
    //printf("the last node =%d flag\n",rp->cluster_in_node_addr[rp->node_num_in_route-1]);
    //return 1;
  }
  //计算通过一个点和目的节点相连
  else
  {
    Node *thisnode;
    thisnode = Node::get_node_by_address(index);
    MY_Neighbor *nb1 = ((MobileNode *)thisnode)->mnbhead.lh_first;     //计算自己簇内的节点是和目的节点有连接
    for(; nb1; nb1 = nb1->mynb_link.le_next) {  
      dist=(rp->out_cluster_node_location_x-nb1->x_local)*(rp->out_cluster_node_location_x-nb1->x_local)+(rp->out_cluster_node_location_y-nb1->y_local)*(rp->out_cluster_node_location_y-nb1->y_local);
      if(dist<(250*250))
      {
        
        dist_to_next_clusterhead=(nb->x_local-nb1->x_local)*(nb->x_local-nb1->x_local)+(nb->y_local-nb1->y_local)*(nb->y_local-nb1->y_local);
        if(dist_to_next_clusterhead<(250*250))
        {
          flag1=1;
        } 
        if(temp==0||dist_to_next_clusterhead<temp)
        {
          temp=dist_to_next_clusterhead;
          rp->cluster_in_node_addr[0]=nb1->nb_addr;
          //rp->out_cluster_node_location_x=nb1->x_local;
          //rp->out_cluster_node_location_y=nb1->y_local;
        }
      }  
    }
    if(flag1)
    {
      rp->cluster_in_node_addr[1]=nb->nb_addr;
      //rp->out_cluster_node_location_x=nb->x_local;
      //rp->out_cluster_node_location_y=nb->y_local;
      ch->size() = (IP_HDR_LEN + 11*sizeof(u_int32_t));
      rp->node_num_in_route=2;
      rp->rp_hop_count = rp->rp_hop_count+2;
      //printf("the last node =%d flag1\n",rp->cluster_in_node_addr[rp->node_num_in_route-1]);
      //return 1;
    }
    //计算第二个点
    else
    {
      temp=0;
      dist_to_next_clusterhead=0;
      
      MY_Neighbor *nb2=nb_lookup(rp->cluster_in_node_addr[0]);//第一个点
	if(nb2==0)
		return 0;
      //printf("nb<%d>,x=%f,y=%f\n",rp->cluster_in_node_addr[0],nb2->x_local,nb2->y_local);
      Node *thisnode;
      thisnode = Node::get_node_by_address(index);
      MY_Neighbor *nb3 = ((MobileNode *)thisnode)->mnbhead.lh_first;     //计算自己簇内的节点是和第一个点有连接
      for(; nb3; nb3 = nb3->mynb_link.le_next) 
      {
        if(nb3->line_num==line_num&&nb3->column_num==column_num)
        {
          //printf("nb3<%d>,x=%f,y=%f\n",nb3->nb_addr,nb3->x_local,nb3->y_local);
          dist=(nb2->x_local-nb3->x_local)*(nb2->x_local-nb3->x_local)+(nb2->y_local-nb3->y_local)*(nb2->y_local-nb3->y_local);
          if(dist<(250*250))
          {
             //printf("nb3<%d>,x=%f,y=%f in dist\n",nb3->nb_addr,nb3->x_local,nb3->y_local);
            
            dist_to_next_clusterhead=(nb->x_local-nb3->x_local)*(nb->x_local-nb3->x_local)+(nb->y_local-nb3->y_local)*(nb->y_local-nb3->y_local);
            if(dist_to_next_clusterhead<(250*250))
            {
               //printf("nb3<%d>,x=%f,y=%f in dist_to_next_clusterhead\n",nb3->nb_addr,nb3->x_local,nb3->y_local);
              flag2=1;
            } 
            if(temp==0||dist_to_next_clusterhead<temp)
            {
              temp=dist_to_next_clusterhead;
              rp->cluster_in_node_addr[1]=nb3->nb_addr;
              //rp->out_cluster_node_location_x=nb3->x_local;
              //rp->out_cluster_node_location_y=nb3->y_local;
            }
          }
        }
      }
    }
    if(flag2)
    {
      rp->cluster_in_node_addr[2]=nb->nb_addr;
      //rp->out_cluster_node_location_x=nb->x_local;
      //rp->out_cluster_node_location_y=nb->y_local;
      ch->size() = (IP_HDR_LEN + 12*sizeof(u_int32_t));
      //printf("the last node =%d flag1\n",rp->cluster_in_node_addr[rp->node_num_in_route-1]);
      rp->node_num_in_route=3;
      rp->rp_hop_count = rp->rp_hop_count+3;
      //return 1;
    }                  
  }
  
  //printf("the last node =%d\n",rp->cluster_in_node_addr[rp->node_num_in_route-1]);
  MY_Neighbor *nb6=nb_lookup(rp->cluster_in_node_addr[rp->node_num_in_route-1]);
  //printf("nb6=%d\n",nb6);
  rp->out_cluster_node_location_x=nb6->x_local;
  rp->out_cluster_node_location_y=nb6->y_local;
  for(int i=0;i<rp->node_num_in_route;i++)
  {
    if(rp->cluster_in_node_addr[i]==index)
    {
      myprot_rt_entry *rt;
        rt = rtable.rt_lookup(rp->rp_src);
      

        if(rt == 0) {
          rt = rtable.rt_add(rp->rp_src);
        }
        if ( (rt->rt_seqno < rp->rp_src_seqno) ||   // newer route 
          ((rt->rt_seqno == rp->rp_src_seqno) &&  
          (rt->rt_hops > rp->rp_hop_count-(rp->node_num_in_route-i-1))) ) { // shorter or better route
          if(i==0)
          {
            ////printf("head is the dest\n");
            rt_update(rt, rp->rp_src_seqno, rp->rp_hop_count-(rp->node_num_in_route-i-1),
            rp->last_cluster_node_addr, CURRENT_TIME + rp->rp_lifetime);
          }
          else
          {
            
          
          // Update the rt entry 
          rt_update(rt, rp->rp_src_seqno, rp->rp_hop_count-(rp->node_num_in_route-i-1),
            rp->cluster_in_node_addr[i-1], CURRENT_TIME + rp->rp_lifetime);
          }
          //printf("node<%d> in route ,next hop=%d\n",index,rt->rt_nexthop);
          // reset the soft state
          rt->rt_req_cnt = 0;
          rt->rt_req_timeout = 0.0; 
          rt->rt_req_last_ttl = 1;


        
          Packet *buf_pkt;
          while((buf_pkt = rqueue.deque(rt->rt_dst))) {
            if(rt->rt_hops != INFINITY2) {
                  assert (rt->rt_flags == RTF_UP);
            // Delay them a little to help ARP. Otherwise ARP 
            // may drop packets. -SRD 5/23/99
              forward(rt, buf_pkt, 0.0);
              
                  }
                }

            
      }
    
    }
  }
  for(int i=0;i<rp->node_num_in_route;i++)
  {
    //printf("route:\n");
    //printf("node<%d>\n",rp->cluster_in_node_addr[i]);
  }
  //printf("rp->out_cluster_node_location_x=%f,rp->out_cluster_node_location_y=%f\n",rp->out_cluster_node_location_x,rp->out_cluster_node_location_y);
  return 1;
}
