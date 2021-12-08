/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mac/ll.cc,v 1.46 2002/03/14 01:12:52 haldar Exp $ (UCB)";
#endif

#include <errmodel.h>
#include <mac.h>
#include <ll.h>
#include <address.h>
#include <dsr/hdr_sr.h>
#include <iostream>
#include "mobilenode.h"


int hdr_ll::offset_;

static class LLHeaderClass : public PacketHeaderClass {
public:
	LLHeaderClass()	: PacketHeaderClass("PacketHeader/LL",
					    sizeof(hdr_ll)) {
		bind_offset(&hdr_ll::offset_);
	}
} class_hdr_ll;


static class LLClass : public TclClass {
public:
	LLClass() : TclClass("LL") {}
	TclObject* create(int, const char*const*) {
		return (new LL);
	}
} class_ll;


LL::LL() : LinkDelay(), seqno_(0), ackno_(0), macDA_(0), ifq_(0),
	mac_(0), lanrouter_(0), arptable_(0), varp_(0),
	downtarget_(0), uptarget_(0)//,mhPrPkt_(this)
{
	bind("macDA_", &macDA_);


	//mhPrPkt_.start(& intr_, 0);
}

int LL::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "ifq") == 0) {
			ifq_ = (Queue*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if(strcmp(argv[1], "arptable") == 0) {
                        arptable_ = (ARPTable*)TclObject::lookup(argv[2]);
                        assert(arptable_);
                        return TCL_OK;
                }
		if(strcmp(argv[1], "varp") == 0) {
                        varp_ = (VARPTable*)TclObject::lookup(argv[2]);
                        assert(varp_);
                        return TCL_OK;
                }
		if (strcmp(argv[1], "mac") == 0) {
			mac_ = (Mac*) TclObject::lookup(argv[2]);
                        assert(mac_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "down-target") == 0) {
			downtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "up-target") == 0) {
			uptarget_ = (NsObject*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "lanrouter") == 0) {
			lanrouter_ = (LanRouter*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}

	}
	else if (argc == 2) {
		if (strcmp(argv[1], "ifq") == 0) {
			tcl.resultf("%s", ifq_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "mac") == 0) {
			tcl.resultf("%s", mac_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "down-target") == 0) {
			tcl.resultf("%s", downtarget_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "up-target") == 0) {
			tcl.resultf("%s", uptarget_->name());
			return (TCL_OK);
		}
	}
	return LinkDelay::command(argc, argv);
}



void LL::recv(Packet* p, Handler* /*h*/)
{
	double lzfnow_ = Scheduler::instance().clock();

	hdr_cmn *ch = HDR_CMN(p);

	printf("LL::recv-----%.9f-----分组ID%d\n", lzfnow_, ch->lzfseqno);
	//char *mh = (char*) HDR_MAC(p);
	//struct hdr_sr *hsr = HDR_SR(p);

	/*
	 * Sanity Check
	 */
	assert(initialized());

	//if(p->incoming) {
	//p->incoming = 0;
	//}
	// XXXXX NOTE: use of incoming flag has been depracated; In order to track direction of pkt flow, direction_ in hdr_cmn is used instead. see packet.h for details.

	// If direction = UP, then pass it up the stack
	// Otherwise, set direction to DOWN and pass it down the stack
	if(ch->direction() == hdr_cmn::UP) {
		//if(mac_->hdr_type(mh) == ETHERTYPE_ARP)
		if(ch->ptype_ == PT_ARP)
			arptable_->arpinput(p, this);
		/*else if(ch->ptype_ == PT_TEST)
			recvpkt(p);*/
		else 
			uptarget_ ? sendUp(p) : drop(p);
		return;
	}

	ch->direction() = hdr_cmn::DOWN;
	sendDown(p);
}


void LL::sendDown(Packet* p)
{	
	double lzfnow_ = Scheduler::instance().clock();

	//hdr_cmn *ch = HDR_CMN(p);

	//printf("LL::sendDown-----%.9f-----分组ID%d\n", lzfnow_, ch->lzfseqno);

	//cout<<"LL::sendDown\n";

	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);

	nsaddr_t dst = (nsaddr_t)Address::instance().get_nodeaddr(ih->daddr());
	//nsaddr_t dst = ih->dst();
	hdr_ll *llh = HDR_LL(p);
	char *mh = (char*)p->access(hdr_mac::offset_);
	
	llh->seqno_ = ++seqno_;

	printf("LL::sendDown-----%.9f-----分组IDlzfseqno:%d-----llh->seqno_%d:\n", lzfnow_, ch->lzfseqno, llh->seqno_);

	llh->lltype() = LL_DATA;

	mac_->hdr_src(mh, mac_->addr());
	mac_->hdr_type(mh, ETHERTYPE_IP);
	int tx = 0;
	
	switch(ch->addr_type()) {

	case NS_AF_ILINK:
		mac_->hdr_dst((char*) HDR_MAC(p), ch->next_hop());
		break;

	case NS_AF_INET:
		dst = ch->next_hop();
		/* FALL THROUGH */
		
	case NS_AF_NONE:
		
		if (IP_BROADCAST == (u_int32_t) dst)
		{
			mac_->hdr_dst((char*) HDR_MAC(p), MAC_BROADCAST);
			break;
		}
		/* Assuming arptable is present, send query */
		if (arptable_) {
			tx = arptable_->arpresolve(dst, p, this);
			break;
		}
		//if (varp_) {
		//tx = varp_->arpresolve(dst, p);
		//break;
			
		//}			
		/* FALL THROUGH */

	default:
		
		int IPnh = (lanrouter_) ? lanrouter_->next_hop(p) : -1;
		if (IPnh < 0)
			mac_->hdr_dst((char*) HDR_MAC(p),macDA_);
		else if (varp_)
			tx = varp_->arpresolve(IPnh, p);
		else
			mac_->hdr_dst((char*) HDR_MAC(p), IPnh);
		break;
	}
	
	if (tx == 0) {
		Scheduler& s = Scheduler::instance();
	// let mac decide when to take a new packet from the queue.
	//LL set delay_                   25us
		s.schedule(downtarget_, p, delay_);
	}
	//cout<<"LL is end\n";
}



void LL::sendUp(Packet* p)
{

	Scheduler& s = Scheduler::instance();
	if (hdr_cmn::access(p)->error() > 0)
		drop(p);
	else
		s.schedule(uptarget_, p, delay_);
}



int hdr_test::offset_;

static class TESTHeaderClass : public PacketHeaderClass {
public:
	TESTHeaderClass()	: PacketHeaderClass("PacketHeader/TEST",
					    sizeof(hdr_test)) {
		bind_offset(&hdr_test::offset_);
	}
} class_hdr_test;

/*void LLTimer::start(Event *e, double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);
  
	busy_ = 1;
	paused_ = 0;
	stime = s.clock();		// simulator virtual time
	rtime = time;			// remaining time
	assert(rtime >= 0.0);
  
  
  
	s.schedule(this, e, rtime);
}

void ProducePktTimer::handle(Event *e) 
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	ll->produceHandler(e);	// zhun bei fa song shujubao 
	
}
void LL::produceHandler(Event *e)
{
	mhPrPkt_.start(&intr_, 0.01);
	sendpkt();
}
int i=0;
void LL::sendpkt()
{
	
	using namespace std;
	cout<<"LL::sendpkt \n";
	cout<<"mac_->addr="<<mac_->addr()<<",NOW="<<NOW<<endl;

	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_test *test_ = HDR_TEST(p);
	if(mac_->addr()==1)
	{
		Node *thisnode;
  		thisnode = Node::get_node_by_address( mac_->addr());
		((MobileNode *)thisnode)->clustre_head_flag=1;
		clustre_head_flag=1;
		printf("clustre_head_flag=1,mac_->addr()=%d\n",mac_->addr());
	}
	if(clustre_head_flag)
	{
		if(i==0)
		{
			test_->testtype_=TEST_A;
			i=1;
		}
		else if (i==1)
		{       
			test_->testtype_=TEST_B;
			i=0;
			
		}
		
	}
	else 
		test_->testtype_=TEST_B;
	
	cout<<"test_->testtype_:"<<test_->testtype_<<endl;

	ch->ptype() = PT_TEST;
	//cout<<"ch->ptype:"<<ch->ptype()<<endl;
	ch->size() = IP_HDR_LEN +sizeof(hdr_test);
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
	ch->prev_hop_ = mac_->addr();          

	ih->saddr() = mac_->addr();
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;
	ch->direction() = hdr_cmn::DOWN;
	//cout<<"senddown   up\n";
	printf("%d\n",p);
	sendDown(p);
	//cout<<"senddown   down\n";
}
void LL::recvpkt(Packet* p)
{
	struct hdr_ip *ih = HDR_IP(p);
	drop(p);
	printf("LL::recvpkt,node=%d,pkt from node=%d\n",mac_->addr(),ih->saddr());
}
*/