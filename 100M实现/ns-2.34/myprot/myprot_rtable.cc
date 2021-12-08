#include <myprot/myprot_rtable.h>




/*
  The Routing Table
*/

myprot_rt_entry::myprot_rt_entry()
{
 int i;
 rt_req_timeout = 0.0;
 rt_req_cnt = 0;

 rt_dst = 0;
 rt_seqno = 0;
 rt_hops =  INFINITY2;
 rt_nexthop = 0;
 LIST_INIT(&rt_pclist);
 rt_expire = 0.0;
 rt_flags = RTF_DOWN;

 /*
 rt_errors = 0;
 rt_error_time = 0.0;
 */


 for (i=0; i < MAX_HISTORY; i++) {
   rt_disc_latency[i] = 0.0;
 }
 hist_indx = 0;
 rt_req_last_ttl = 0;

 LIST_INIT(&rt_nblist);

}


myprot_rt_entry::~myprot_rt_entry()
{
MYPROT_Neighbor *nb;

 while((nb = rt_nblist.lh_first)) {
   LIST_REMOVE(nb, nb_link);
   delete nb;
 }

MYPROT_Precursor *pc;

 while((pc = rt_pclist.lh_first)) {
   LIST_REMOVE(pc, pc_link);
   delete pc;
 }

}


void
myprot_rt_entry::nb_insert(nsaddr_t id)
{
MYPROT_Neighbor *nb = new MYPROT_Neighbor(id);
        
 assert(nb);
 nb->nb_expire = 0;
 LIST_INSERT_HEAD(&rt_nblist, nb, nb_link);

}


MYPROT_Neighbor*
myprot_rt_entry::nb_lookup(nsaddr_t id)
{
MYPROT_Neighbor *nb = rt_nblist.lh_first;

 for(; nb; nb = nb->nb_link.le_next) {
   if(nb->nb_addr == id)
     break;
 }
 return nb;

}


void
myprot_rt_entry::pc_insert(nsaddr_t id)
{
	if (pc_lookup(id) == NULL) {
	MYPROT_Precursor *pc = new MYPROT_Precursor(id);
        
 		assert(pc);
 		LIST_INSERT_HEAD(&rt_pclist, pc, pc_link);
	}
}


MYPROT_Precursor*
myprot_rt_entry::pc_lookup(nsaddr_t id)
{
MYPROT_Precursor *pc = rt_pclist.lh_first;

 for(; pc; pc = pc->pc_link.le_next) {
   if(pc->pc_addr == id)
   	return pc;
 }
 return NULL;

}

void
myprot_rt_entry::pc_delete(nsaddr_t id) {
MYPROT_Precursor *pc = rt_pclist.lh_first;

 for(; pc; pc = pc->pc_link.le_next) {
   if(pc->pc_addr == id) {
     LIST_REMOVE(pc,pc_link);
     delete pc;
     break;
   }
 }

}

void
myprot_rt_entry::pc_delete(void) {
MYPROT_Precursor *pc;

 while((pc = rt_pclist.lh_first)) {
   LIST_REMOVE(pc, pc_link);
   delete pc;
 }
}	

bool
myprot_rt_entry::pc_empty(void) {
MYPROT_Precursor *pc;

 if ((pc = rt_pclist.lh_first)) return false;
 else return true;
}	

/*
  The Routing Table
*/

myprot_rt_entry*
myprot_rtable::rt_lookup(nsaddr_t id)
{
myprot_rt_entry *rt = rthead.lh_first;

 for(; rt; rt = rt->rt_link.le_next) {
   if(rt->rt_dst == id)
     break;
 }
 return rt;

}

void
myprot_rtable::rt_delete(nsaddr_t id)
{
myprot_rt_entry *rt = rt_lookup(id);

 if(rt) {
   LIST_REMOVE(rt, rt_link);
   delete rt;
 }

}

myprot_rt_entry*
myprot_rtable::rt_add(nsaddr_t id)
{
myprot_rt_entry *rt;

 assert(rt_lookup(id) == 0);
 rt = new myprot_rt_entry;
 assert(rt);
 rt->rt_dst = id;
 LIST_INSERT_HEAD(&rthead, rt, rt_link);
 return rt;
}
