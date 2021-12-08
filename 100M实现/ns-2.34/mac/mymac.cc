#include "mymac.h"
#define CURRENT_TIME    Scheduler::instance().clock()
//物理层参数设置，遵循 802.11
static MYMacPHY_MIB PMIB = 
{
	DSSS_CWMin, DSSS_CWMax, DSSS_SlotTime, DSSS_CCATime,
	DSSS_RxTxTurnaroundTime, DSSS_SIFSTime, DSSS_PreambleLength,
	DSSS_PLCPHeaderLength,DSSS_PLCPDataRate
};

//TCL Hooks for the simulator
static class MYMacClass : public TclClass 
{
public:
	MYMacClass() : TclClass("Mac/MYMac") {}
	TclObject* create(int, const char*const*) 
	{
		return (new MYMac(&PMIB));
	}
}class_macMYMac;

int MYMac::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "log-target") == 0) { 
			logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			if(logtarget_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "eventtrace") == 0) {
			// command added to support event tracing by Sushmita
                        et_ = (EventTrace *)TclObject::lookup(argv[2]);
                        return (TCL_OK);
                }
	}
	return Mac::command(argc, argv);
}


void MYMac::trace_event(char *eventtype, Packet *p) 
{
        if (et_ == NULL) return;
        char *wrk = et_->buffer();
        char *nwrk = et_->nbuffer();
        //char *src_nodeaddr =
	//       Address::instance().print_nodeaddr(iph->saddr());
        //char *dst_nodeaddr =
        //      Address::instance().print_nodeaddr(iph->daddr());
	
        struct hdr_mymac* dh = HDR_MYMAC(p);
	
        //struct hdr_cmn *ch = HDR_CMN(p);
	
	if(wrk != 0) {
		sprintf(wrk, "E -t "TIME_FORMAT" %s  ",
			et_->round(Scheduler::instance().clock()),
                        eventtype
                        //ETHER_ADDR(dh->dh_sa)
                        );
        }
        if(nwrk != 0) {
                sprintf(nwrk, "E -t "TIME_FORMAT" %s  ",
                        et_->round(Scheduler::instance().clock()),
                        eventtype
                        //ETHER_ADDR(dh->dh_sa)
                        );
        }
        et_->dump();
}
void
MYMac::trace_pkt(Packet *p) 
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mymac* dh = HDR_MYMAC(p);
	u_int16_t *t = (u_int16_t*) &dh->dh_fc;

	fprintf(stderr, "\t[ %2x %2x %2x %2x ] %x %s %d\n",
		*t, dh->dh_duration,
		 ETHER_ADDR(dh->dh_da), ETHER_ADDR(dh->dh_sa),
		index_, packet_info.name(ch->ptype()), ch->size());
}
//定时器相关
void MYMacTimer::start(Packet *p, double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);
	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
	s.schedule(this, p, rtime);//确定触发事件的时间，将调度事件插入调度队列
}
void MYMacTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
	s.schedule(this, &intr, rtime);
}
void MYMacTimer::stop(Packet *p) 
{
	Scheduler &s = Scheduler::instance();
	assert(busy_);
	if(paused_ == 0)
		s.cancel((Event *)p);//取消一个挂起的定时器
	// Should free the packet p.
	Packet::free(p);
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
}
void MYMacTimer::stop(void)
{
	//printf("NOW=%f,MYMacTimer::stop()\n",NOW);
	Scheduler &s = Scheduler::instance();
	assert(busy_);
	if(paused_ == 0)
		s.cancel(&intr);
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
}
void SlotMYMacTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->slotHandler();
}
/* Receive Timer */
void RxPktMYMacTimer::handle(Event *) 
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->recvHandler();
}
/* Send Timer */
void TxPktMYMacTimer::handle(Event *) 
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->sendHandler();
}
void IFMYMacTimer::handle(Event *)
{
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->txHandler();
}
void HelloBackoffMYMacTimer::handle(Event *) 
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->hello_backoffHandler();
}
void HeadHelloBackoffMYMacTimer::handle(Event *) 
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->headhello_backoffHandler();
}
void BackoffMYMacTimer::handle(Event *)
{
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	difs_wait = 0.0;
	mac->backoffHandler();
}
void BackoffMYMacTimer::start(int cw, int idle, double difs)
{
	//printf("NOW=%f,node<%d>,BackoffMYMacTimer::start(),idle=%d\n",NOW,mac->index_,idle);
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);
	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = (Random::random() % cw) * mac->phymib_->getSlotTime();
	//rtime = Random::integer(cw) * mac->phymib_->getSlotTime();
	//double rtime2 = (Random::random() % cw) * mac->phymib_->getSlotTime();
	//srand((unsigned)time(0));
	//double rtime2=(rand()%cw)* mac->phymib_->getSlotTime();
	//printf("rtime=%f,r2=%f\n",rtime,rtime2);
	//printf("BackoffMYMacTimer::start(),rtime=%f\n",rtime);
	difs_wait = difs;
	if(idle == 0)
	{
		paused_ = 1;
		//printf("node<%d>,tingma?\n",mac->index_);
	}	
	else 
	{
		assert(rtime + difs_wait >= 0.0);
		//printf("zou ni %f\n",rtime + difs_wait);
		//printf("node<%d>\n",mac->index_);
		s.schedule(this, &intr, rtime + difs_wait);
	}
	//printf("BackoffMYMacTimer::start(),rtime + difs_wait=%f+%f=%f\n",rtime,difs_wait,rtime+difs_wait);
}
void BackoffMYMacTimer::pause()
{
	//printf("NOW=%f,node<%d>,BackoffMYMacTimer::pause()\n",NOW,mac->index_);
	Scheduler &s = Scheduler::instance();
	//the caculation below make validation pass for linux though it
	// looks dummy
	double st = s.clock();
	double rt = stime + difs_wait;
	double sr = st - rt;
	double mst = (mac->phymib_->getSlotTime());
    int slots = int (sr/mst);
	if(slots < 0)
		slots = 0;
	assert(busy_ && ! paused_);
	paused_ = 1;
	rtime -= (slots * mac->phymib_->getSlotTime());
	assert(rtime >= 0.0);
	difs_wait = 0.0;
	s.cancel(&intr);
}
void BackoffMYMacTimer::resume(double difs)
{
	//printf("NOW=%f,node<%d>,BackoffMYMacTimer::resume()\n",NOW,mac->index_);
	Scheduler &s = Scheduler::instance();
	assert(busy_ && paused_);
	paused_ = 0;
	stime = s.clock();
	/*
	 * The media should be idle for DIFS time before we start
	 * decrementing the counter, so I add difs time in here.
	 */
	difs_wait = difs;
	//printf("BackoffMYMacTimer::resume(),rtime + difs_wait=%f+%f=%f\n",rtime,difs_wait,rtime+difs_wait);
	assert(rtime + difs_wait >= 0.0);
    s.schedule(this, &intr, rtime + difs_wait);
}
void DeferMYMacTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);
	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
	//printf("NOW=%f,node<%d>,DeferMYMacTimer::start(),rtime %f\n",NOW,mac->index_,rtime);
	//printf("%f\n",rtime);
	s.schedule(this, &intr, rtime);
}
void DeferMYMacTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->deferHandler();
}

void NavMYMacTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->navHandler();
}

void MYMac::navHandler()
{
	//printf("NOW=%f,node<%d> in MYMac::navHandler()\n",NOW,index_);
	//printf("is_idle()=%f,mhBackoff_.paused()=%d\n",is_idle(),mhBackoff_.paused());
	if(is_idle() && mhBackoff_.paused())
		mhBackoff_.resume(phymib_->getDIFS());
}
//MYMac 定义，构造函数
MYMac::MYMac(MYMacPHY_MIB* p) : //初始化，定时器
	Mac(), mhSlot_(this), mhTxPkt_(this), mhRxPkt_(this),mhHelloBackoff_(this),
	mhDefer_(this), mhBackoff_(this),mhIF_(this),mhNav_(this),mhHHBackoff_(this)
{	
	bandwidth_=1000000;
	thisnode = Node::get_node_by_address(index_);
	nav_ = 0.0;
	phymib_ = p;		//设置物理层参数
	sta_seqno_ = 1;		//接下来我将使用的seqno
	cw_ = phymib_->getCWMin();
    //slot_time = SLOT_TIME;		//更改时隙长度
	//初始化节点信道状态
	tx_state_ = rx_state_ = MAC_IDLE;
	tx_active_ = 0;
	slot_count= 0;		//时隙号从0开始
	mhSlot_.start(0.0);     //开启时隙定时器
}

//处理接收到的包
void MYMac::recv(Packet* p, Handler* h) 
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_mymac* dh = HDR_MYMAC(p);
	//printf("NOW=%f,node<%d> recv from node<%d> in MYMac\n",NOW,index_,ih->saddr());
	//printf("in MYMac::recv(),slot_count=%d\n",slot_count);
    //处理往外发送的包，从LL->sendDown()->PHY
	if (ch->direction() == hdr_cmn::DOWN) 
	{
		callback_ = h;	
		sendDown(p);		//配置向下传输的数据包，添加MAC头
		//printf("dh->dh_da=%d,ETHER_ADDR(dh->dh_da)=%#X\n",dh->dh_da,ETHER_ADDR(dh->dh_da));
		sendRTS(ETHER_ADDR(dh->dh_da));		//判断是否需要发送RTS
		//dh->dh_scontrol = sta_seqno_++;		//给数据包分配一个序列号
		//先检测当前是否正在backoff，（因为有可能前面已经成功发送过一个数据包，并进入退避过程）
		//如果处于退避状态，则直接等超时再处理。
		//printf("node<%d> in recv(),mhBackoff_.busy()=%d,is_idle()=%d,mhDefer_.busy()=%d\n",index_,mhBackoff_.busy(),is_idle(),mhDefer_.busy());
		if(mhBackoff_.busy() == 0) 
		{
			//printf("node<%d> in recv(),mhBackoff_.busy()==0,then backoff start()\n",index_);
			if(is_idle()) //如果信道是空闲的，我们必须等待一个difs才能传输。
			{
				//printf("node<%d> recv(),mhDefer_.busy()=%d\n",index_,mhDefer_.busy());
				if (mhDefer_.busy() == 0) //是否处在延迟定时器defer中。
				{
					//此处应该只延迟DIFS就发送
					struct hdr_myprot *hch = HDR_MYPROT(p);
					if(hch->myprot_type == MYPROT_RREQ||hch->myprot_type == MYPROT_RREP)
						mhBackoff_.start(cw_, is_idle(),phymib_->getDIFS_RP());
					else	
						mhBackoff_.start(cw_, is_idle(),phymib_->getDIFS());
					//mhBackoff_.start(phymib_->getDIFS());
				} //如果我们已经在延迟，则不需要重置延迟计时器，返回并等待延时结束。
			} 
			else 
			{
				//如果信道不空闲，则启动backoff定时器，时延为竞争窗口大小。
				//此时还将当前信道不空闲作为一个参数传递给定时器start()，
				//会造成定时器一启动就立刻被暂停，等到信道空闲再被激活。
				//重新等待DIFS+CW时延后发送数据。
				mhBackoff_.start(cw_, is_idle());
				//使用backoff是因为他可以被暂停（当信道变忙），而deferTimer不可以。
			}
		}
		else
		{
			//printf("node<%d> in recv(),mhBackoff_.busy()!=0,then just wait backoffhandler()\n",index_);
		}
		return;
	}
	//处理接收的包，从PHY->sendUp()->LL
	if (ch->direction() == hdr_cmn::UP) 
	{
		sendUp(p);
		return;
	}																				
}

//处理往外发送的包，从LL->sendDown()->PHY，需要计算传输的时间
//sendDown()，用于构造数据包的MAC头，将其存在pktTx_中，等发送时机发送
void MYMac::sendDown(Packet* p) 
{
	struct hdr_cmn* ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_mymac* dh = HDR_MYMAC(p);
	struct hdr_myprot *hch = HDR_MYPROT(p);
	//printf("NOW=%f,node<%d> recv from node<%d> and sendDown in MYMac\n",NOW,index_,ih->saddr());

	//更新MAC头，与802.11相同
	ch->size() += ETHER_HDR_LEN;
	//如果是TC类型，则包大小放大4倍。
	if(hch->myprot_type == MYPROT_RREQ||hch->myprot_type == MYPROT_RREP)
		ch->size()=ch->size()*4;
	dh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	dh->dh_fc.fc_type       = MAC_Type_Data;
	dh->dh_fc.fc_subtype    = MAC_Subtype_Data;
	dh->dh_fc.fc_to_ds      = 0;
	dh->dh_fc.fc_from_ds    = 0;
	dh->dh_fc.fc_more_frag  = 0;
	dh->dh_fc.fc_retry      = 0;
	dh->dh_fc.fc_pwr_mgt    = 0;
	dh->dh_fc.fc_more_data  = 0;
	dh->dh_fc.fc_wep        = 0;
	dh->dh_fc.fc_order      = 0;
	//存储数据传输时间
	//在这将传输时间变成4倍？WQC???
	//printf("bandwidth_=%f\n",bandwidth_);
	ch->txtime() = txtime(ch->size(), bandwidth_);
	if((u_int32_t)ETHER_ADDR(dh->dh_da) != MAC_BROADCAST)
		dh->dh_duration = usec(txtime(phymib_->getACKlen(), bandwidth_) + phymib_->getSIFS());
	else
		dh->dh_duration = 0;

	pktTx_ = p;		//缓冲要发送的数据包
	//u_int32_t dst, src, size;
	//dst = ETHER_ADDR(dh->dh_da);
	//src = ETHER_ADDR(dh->dh_sa);
	//size = ch->size();
}
void MYMac::sendUp(Packet* p) 
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	//printf("NOW=%f,node<%d> recv from node<%d> and sendUp in MYMac\n",NOW,index_,ih->saddr());
	//判断MAC层的状态，如果为发送状态，则将包置成错包，
	if (tx_active_ && ch->error() == 0) 
	{
		//printf("node<%d>, can't receive while transmitting!\n", index_);
		ch->error() = 1;
	}
	//如果是rx_state_为空闲状态，则置为接收状态
	if (rx_state_ == MAC_IDLE) 
	{
		//printf("node<%d>, receive while channel is MAC_IDLE!\n", NOW,index_);
		setRxState(MAC_RECV);
		pktRx_ = p;                 // Save the packet for timer reference.
		//计算接收时间
		double rtime = txtime(p);
		assert(rtime >= 0);
		//开启接收计时器
		mhRxPkt_.start(rtime);
	} 
	else
	{
		//printf("node<%d>, receiving, but the channel is not idle\n",index_);
		if(pktRx_->txinfo_.RxPr / p->txinfo_.RxPr >= p->txinfo_.CPThresh)
		{	//当新接收的包的功率小于正在接收的包的功率，则忽略新包
			//printf("the power of new < the packet currently recv ,then free new packet directly\n");
			set_nav(usec(phymib_->getEIFS() + txtime(p)));
			Packet::free(p);
		}
		else
		{	
			//printf("node <%d>, the power of new packet enough ,recv collision.\n", index_ );
			collision(p);	//否则，碰撞。
		}
	}
}
void MYMac::transmit(Packet *p, double timeout) 
{
	//printf("NOW=%f,node<%d>, in transmit(),timeout=%f\n",NOW,index_,timeout);
	tx_active_ = 1;		//发送标志位
	//当我传输的时候，任何接收的包都接收不到，必须丢弃。
	if(rx_state_ != MAC_IDLE) 
	{
		//printf("If I'm transmitting,any incoming packet will be missedand hence, must be discarded\n");
		assert(pktRx_);
		struct hdr_cmn *ch = HDR_CMN(pktRx_);
		ch->error() = 1;        /* force packet discard */
	}			     
	//把包传递给下层（Phy层）
	//mhTxPkt_.start(p->copy(), timeout);
	struct hdr_mymac *mh = HDR_MYMAC(p);
	/*if(mh->dh_fc.fc_subtype==MAC_Subtype_RTS&&index_==0)
	{
		printf("RTS %f %f\n",NOW,timeout);
	}
	if(mh->dh_fc.fc_subtype==MAC_Subtype_CTS&&index_==1)
	{
		printf("CTS %f %f\n",NOW,timeout);
	}
	if(mh->dh_fc.fc_subtype==MAC_Subtype_Data&&index_==0)
	{
		printf("DATA %f %f\n",NOW,timeout);
	}
	if(mh->dh_fc.fc_subtype==MAC_Subtype_ACK&&index_==1)
	{
		printf("ACK %f %f\n",NOW,timeout);
	}*/
	downtarget_->recv(p->copy(), this);        
	//启动一个发送计时器，超时表示发送完毕，（timeout中包含信道占用时间）
	//printf("in MYMac::transmit(),mhTxPkt_.busy()=%d,mhTxPkt_.paused()=%d\n",mhTxPkt_.busy(),mhTxPkt_.paused());
	//printf("timeout=%f\n",timeout);
	//mhTxPkt_.start(p->copy(), timeout);
	mhTxPkt_.start(timeout);
	//包实际传输时间定时器
	//printf("in MYMac::transmit(),mhIF_.busy()=%d,mhIF_.paused()=%d\n",mhIF_.busy(),mhIF_.paused());
	struct hdr_cmn *ch_test = HDR_CMN(p);
	//printf("TX_Time(p)=%f,txtime(p)=%f\n",TX_Time(p),txtime(ch_test->size(), bandwidth_));
	mhIF_.start(txtime(p));
	//printf("txtime=%f\n",txtime(p));
}

void MYMac::collision(Packet *p)
{
	//printf("NOW=%f,node<%d>, in collision()\n",NOW,index_);
	hdr_cmn *ch_p = HDR_CMN(p);
	hdr_cmn *ch_pktRx_ = HDR_CMN(pktRx_);
	switch(rx_state_) 
	{
		case MAC_RECV:
			setRxState(MAC_COLL);
			// fall through
		case MAC_COLL:
			assert(pktRx_);
			assert(mhRxPkt_.busy());
			//printf("a collision has occurred,figure out which packet that caused the collision will last the longest\n");
			//由于发生了冲突，计算出导致冲突的包中哪个包持续时间最长。如果需要，使这个包成为pktRx_并复位Recv定时器。
			if(txtime(p) > mhRxPkt_.expire()) 
			{
				mhRxPkt_.stop();
				//discard(pktRx_, DROP_MAC_COLLISION);
				Packet::free(pktRx_);
				pktRx_ = p;
				mhRxPkt_.start(txtime(pktRx_));
			}
			else 
			{
				Packet::free(p);
				//discard(p, DROP_MAC_COLLISION);
			}
			break;
		default:
			assert(0);
	}
}

void MYMac::slotHandler() 
{
	//printf("NOW=%f,node<%d>,log in file:%s-func %s(),line:%d\n",NOW,index_,__FILE__ ,__FUNCTION__,__LINE__);
	//printf("node<%d>, slotHandler ,in slot_count=%d\n",index_,slot_count);
	if(calcu_flag==0)		//当前位置不动，只进行一次计算地理位置。
	{
		calculate_cellular_area();
		//printf("node<%d> XY=(%f,%f)\n",index_,((MobileNode *)thisnode)->X(),((MobileNode *)thisnode)->Y());
		//printf("node<%d> (line,column_num)=(%d,%d)\n",index_,((MobileNode *)thisnode)->line_num,((MobileNode *)thisnode)->column_num);
		//printf("node<%d> slot_area_num=%d,distance_to_center=%f\n",index_,((MobileNode *)thisnode)->slot_area_num,((MobileNode *)thisnode)->distance_to_center);
		calcu_flag=1;
		//printf("node<%d>\n",index_);
		MY_Neighbor *tmp = nb_lookup(index_);
		//
		//把自己的信息写进邻居列表中，方便之后的路由运算
		if(tmp == 0) 
		{
			nb_insert(index_);
			//printf("NOW=%f,node<%d> nb_insert() node<%d> succ \n",NOW,index,heh->hello_src);
			//thisnode->nb_print();
		}
		tmp = nb_lookup(index_);
		assert(tmp);
		//更新邻居节点表内的地址位置信息。
		tmp->x_local=((MobileNode *)thisnode)->X();
		tmp->y_local=((MobileNode *)thisnode)->Y();
		//printf("hef->lx=%f,hef->ly=%f\n",hef->lx,hef->ly);
		//printf("tmp->lx=%f,tmp->ly=%f\n",tmp->lx,tmp->ly);
		//tmp->CHID=hef->CHID;		//暂时未在hello中添加节点的簇首ID
		calculate_cellular_nb(tmp->x_local,tmp->y_local,tmp->line_num,tmp->column_num,tmp->area_num,tmp->distance_to_center);

	}
	//printf("NOW=%f,node<%d>,slot_count=%d,slot_area_num=%d,slotHandler() in MAC \n",NOW,index_,slot_count,((MobileNode *)thisnode)->slotAreaNum());
	if(slot_count>=0 && slot_count<=5)
	{
		//printf("slot_count == %d\n",slot_count);
		//当前时隙号等于自己分配到的时隙号
		if (slot_count == slot_area_num) 
		{
			//printf("node <%d> 's slot_count,time to send.\n", index_);
			sendHello();		//设置hello包
			assert(pktHELLO_);
			hello_backoff();	//开启hello backoff
		}
		else
		{
			//printf("not node <%d> 's slot_count_,wait next slot\n",index_);
		}
		mhSlot_.start(0.01);      	//开启下一次的定时器		
	}
	else 
	{
		CH_Campaign();		//在hello时帧结束后，根据邻居节点地理位置进行簇首竞选
		mhSlot_.start(1.94);      	//开启下一次的定时器
		if(((MobileNode *)thisnode)->clustre_head_flag==index_)
		{
			sendHeadHello();
			assert(pktHeadHELLO_);
			headhello_backoff();
		}
	}
	slot_count++;
	slot_count=slot_count%7;
	
	//printf("in MYMac::slotHandler(),slot_count=%d\n",slot_count);
}
//设置hello包
void MYMac::sendHello()
{
	Packet *p = Packet::alloc();
	struct hdr_cmn* ch = HDR_CMN(p);
	struct hdr_mac_hello *hef =HDR_MAC_HELLO(p);
	struct hdr_ip *ih = HDR_IP(p);
	assert(pktHELLO_ == 0);
	
	ih->saddr()=index_;

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() += ETHER_HDR_HELLO_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	bzero(hef, MAC_HDR_LEN);

	hef->hello_fc.fc_protocol_version = MAC_ProtocolVersion;
 	hef->hello_fc.fc_type	= MAC_Type_Management;
 	hef->hello_fc.fc_subtype	= MAC_Subtype_HELLO;
	hef->hello_duration = 0;

	int dst_broadcast;
	dst_broadcast = MAC_BROADCAST;
	STORE4BYTE(&dst_broadcast, (hef->hello_da));
	STORE4BYTE(&index_, (hef->hello_sa));

	hef->hello_hop_count = 1;
    hef->hello_src = index_;
	hef->lx=((MobileNode *)thisnode)->X();
	hef->ly=((MobileNode *)thisnode)->Y();

	ch->txtime() = TX_Time(p);
	pktHELLO_ = p;
} 
//设置簇首间hello包
void    MYMac::sendHeadHello()
{
	Packet *p = Packet::alloc();
	struct hdr_cmn* ch = HDR_CMN(p);
	struct hdr_mac_headhello *hhf =HDR_MAC_HEADHELLO(p);
	struct hdr_ip *ih = HDR_IP(p);
	assert(pktHeadHELLO_ == 0);
	
	ih->saddr()=index_;

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() *= 4;    //传输半径变长，传输速率变为原来的1/4，所以包包长变为原来的4倍可以起到同样的效果
	ch->iface() = -2;
	ch->error() = 0;
	bzero(hhf, MAC_HDR_LEN);

	hhf->headhello_fc.fc_protocol_version = MAC_ProtocolVersion;
 	hhf->headhello_fc.fc_type	= MAC_Type_Management;
 	hhf->headhello_fc.fc_subtype	= MAC_Subtype_HEADHELLO;
	hhf->headhello_duration = 0;

	int dst_broadcast;
	dst_broadcast = MAC_BROADCAST;
	STORE4BYTE(&dst_broadcast, (hhf->headhello_da));
	STORE4BYTE(&index_, (hhf->headhello_sa));

	hhf->headhello_hop_count = 1;
    hhf->headhello_src = index_;
	hhf->line_num=line_num;
	hhf->column_num=column_num;
	MY_Neighbor *nb = ((MobileNode *)thisnode)->mnbhead.lh_first;
	int i=0;
	for(; nb; nb = nb->mynb_link.le_next) 
	{
		//当该邻居节点与自己处于同一个蜂窝，才进行簇首竞选。
		if(line_num == nb->line_num && column_num == nb->column_num)
		{
			hhf->num_cluster_in_node_=i+1;
			//printf("nb=%d\n",nb->nb_addr);
			//printf("x_local=%f\n",nb->x_local);
			//printf("hhf->x_local=%f\n",hhf->cluster_in_node_[i].x_local);
			hhf->cluster_in_node_[i].nb_addr=nb->nb_addr;
			hhf->cluster_in_node_[i].x_local=nb->x_local;
			hhf->cluster_in_node_[i].y_local=nb->y_local;
			i++;
		}
 	}
	//把自己的信息也家进去，方便做路由
	/*hhf->num_cluster_in_node_+=1;
	hhf->cluster_in_node_[i].nb_addr=index_;
	hhf->cluster_in_node_[i].x_local=((MobileNode *)thisnode)->X();
	hhf->cluster_in_node_[i].y_local=((MobileNode *)thisnode)->Y();*/
	//printf("mac_head_len=%d,sizeof(double)=%d,sizeof(nsaddr_t)=%d\n",hhf->size(),sizeof(double),sizeof(nsaddr_t));
	ch->size() = hhf->size()+phymib_->getPLCPhdrLen();
	ch->size()=ch->size()*4;
	ch->txtime() = TX_Time(p);
	pktHeadHELLO_ = p;
}
//开启接收计时器，接收时间结束后，实际接收处理函数
void MYMac::recvHandler() 
{
	//printf("NOW=%f,node<%d>,recvHandler() in MYMac \n",NOW,index_);
	u_int32_t dst, src; 
	int size;
	struct hdr_cmn *ch = HDR_CMN(pktRx_);
	struct hdr_mymac *dh = HDR_MYMAC(pktRx_);
	u_int8_t  type = dh->dh_fc.fc_type;
	u_int8_t  subtype = dh->dh_fc.fc_subtype;
	assert(pktRx_);
	assert(rx_state_ == MAC_RECV || rx_state_ == MAC_COLL);
	//在发送状态下，当有包到达，本就接收不到这个包，应直接丢弃，
    if(tx_active_) 	
	{
		//printf("in transmiting,this packet arrives, then I would never have seen it,free\n");
		Packet::free(pktRx_);
		goto done;
    }
	//检查是否在接收过程中发生碰撞。
	if (rx_state_ == MAC_COLL) 
	{
		//printf("the rx_state_ is MAC_COLL,handle collisions\n");
		discard(pktRx_, DROP_MAC_COLLISION);	
		set_nav(usec(phymib_->getEIFS()));		//还没有添加NAV相关内容
		goto done;
	}
	if(ch->error()) 
	{
		//printf("this packet with ch->error() bit errors\n",index_);
		Packet::free(pktRx_);
		set_nav(usec(phymib_->getEIFS()));
		goto done;
	}
	/* check if this packet was unicast and not intended for me, drop it.*/ 
	dst = ETHER_ADDR(dh->dh_da);
	src = ETHER_ADDR(dh->dh_sa);
	size = ch->size();
	//update the NAV (Network Allocation Vector)
	if(dst != (u_int32_t)index_) 
	{
		set_nav(dh->dh_duration);
	}
	
	//printf("node <%d>, %f, recv a packet [from %d to %d], size = %d\n", index_, NOW, src, dst, size);
	//printf("MAC_BROADCAST=%#X,dst=%#X \n",MAC_BROADCAST,dst);
	// Not a pcket destinated to me.
	if ((dst != MAC_BROADCAST) && (dst != (u_int32_t)index_)) //这里需要对上层的人rrep包做相应的修改？？？
	{
		//printf("not a pcket destinated to me\n");
		drop(pktRx_);
		goto done;
	}
	//printf("deal with this packet,type=%d,subtype=%d\n",type,subtype);
	switch(type) 
	{
		case MAC_Type_Management:
		{
			switch(subtype)
			{
				case MAC_Subtype_HELLO:
					recvHello(pktRx_);
					break;
				case MAC_Subtype_HEADHELLO:
					//printf("MAC_Subtype_HEADHELLO,call recvHello\n");
					recvHeadHello(pktRx_);
					break;
			}
		}
		break;
		case MAC_Type_Control:
		{
			switch(subtype)
			{
			case MAC_Subtype_RTS:
				recvRTS(pktRx_);
				break;
			case MAC_Subtype_CTS:
				recvCTS(pktRx_);
				break;
			case MAC_Subtype_ACK:
				recvACK(pktRx_);
				break;
			default:
				fprintf(stderr,"recvTimer2:Invalid MAC Control Subtype %x\n",
					subtype);
				exit(1);			
			}
		}
		break;
		case MAC_Type_Data:
		{
			switch(subtype)
			{
				case MAC_Subtype_Data:
					recvDATA(pktRx_);
					break;
			}
		}
		break;
		default:
			fprintf(stderr, "recv_timer4:Invalid MAC Type %x\n", subtype);
			exit(1);
	}
done:
	pktRx_ = 0;
	assert(pktRx_ == 0);
	assert(mhRxPkt_.busy() == 0);
	setRxState(MAC_IDLE);
}
//当成功完成一个包的发送，进入发送完成处理函数
void MYMac::sendHandler() 
{
	//printf("NOW=%f,node <%d>, send a packet finished,sendHandler() in tx_state_=%#X.\n",NOW,index_,tx_state_);
	switch(tx_state_) 
	{
	case MAC_HELLO:
		assert(pktHELLO_);
		Packet::free(pktHELLO_);
		pktHELLO_ = 0;
		break;
	case MAC_HEADHELLO:
		assert(pktHeadHELLO_);
		Packet::free(pktHeadHELLO_);
		pktHeadHELLO_ = 0;
		break;
	//发送了一个RTS，但没有收到CTS，则重传RTS
	case MAC_RTS:
		RetransmitRTS();
		break;
	//发送了一个CTS，但没有收到数据包。
	case MAC_CTS:
		assert(pktCTRL_);
		Packet::free(pktCTRL_); 
		pktCTRL_ = 0;
		break;
	//发送数据，但没有收到ACK数据包。
	case MAC_SEND:
		RetransmitDATA();
		break;
	//发送了一个ACK，现在准备恢复传输。
	case MAC_ACK:
		assert(pktCTRL_);
		Packet::free(pktCTRL_); 
		pktCTRL_ = 0;
		break;
	case MAC_IDLE:
		break;
	default:
		assert(0);
	}
	if(mhDefer_.busy()) 
		mhDefer_.stop();
	tx_resume();
	//一旦传输完成，丢弃数据包。
	//setTxState(MAC_IDLE);
	//Packet::free((Packet *)e);

}
//如果需要，重传RTS数据包。
void MYMac::RetransmitRTS()
{
	//printf("NOW=%f,node<%d>,RetransmitRTS() in MYMac \n",NOW,index_);
	assert(pktTx_);
	assert(pktRTS_);
	assert(mhBackoff_.busy() == 0);
	//省掉了RTS重传失败计数，不增加退避窗口，直接进入backoff退避，重传RTS
	//inc_cw();
	mhBackoff_.start(cw_, is_idle());
}
//如果需要，重传上层数据包。
void MYMac::RetransmitDATA()
{
	//printf("NOW=%f,node<%d>,RetransmitDATA() in MYMac \n",NOW,index_);
	u_int32_t *rcount, thresh;
	assert(mhBackoff_.busy() == 0);
	assert(pktTx_);
	assert(pktRTS_ == 0);
	struct hdr_cmn *ch = HDR_CMN(pktTx_);
	struct hdr_mymac *mh = HDR_MYMAC(pktTx_);
	//广播的数据包不会有ACK，因此不会被重新传输。
	if((u_int32_t)ETHER_ADDR(mh->dh_da) == MAC_BROADCAST) 
	{
		//printf("in RetransmitDATA(),MAC_BROADCAST DATA no need retransmit data\n");
		Packet::free(pktTx_); 
		pktTx_ = 0;
		//rst_cw();
		mhBackoff_.start(cw_, is_idle());//发送完成后Backoff
		return;
	}
	//重传非广播的数据包
	//printf("in RetransmitDATA(),DATA need retransmit data\n");
	mh->dh_fc.fc_retry = 1;
	sendRTS(ETHER_ADDR(mh->dh_da));
	//inc_cw();
	mhBackoff_.start(cw_, is_idle());
}
void MYMac::hello_backoffHandler()
{
	if(!is_idle())		//如果当前信道忙，则继续hello backoff
	{
		//printf("channel is busy ,then hello_backoff \n");
		hello_backoff();
	}	
	else	//信道空闲，则直接transmit hello
	{
		//printf("channel is idle ,then transmit Hello \n");
		double timeout = TX_Time(pktHELLO_);		
		setTxState(MAC_HELLO);				     
		transmit(pktHELLO_, timeout);
	}
}
void MYMac::headhello_backoffHandler()
{
	if(!is_idle())		//如果当前信道忙，则继续hello backoff
	{
		//printf("channel is busy ,then hello_backoff \n");
		headhello_backoff();
	}	
	else	//信道空闲，则直接transmit hello
	{
		double timeout = TX_Time(pktHeadHELLO_);		
		setTxState(MAC_HEADHELLO);				     
		transmit(pktHeadHELLO_, timeout);
	}
}
void MYMac::backoffHandler() 
{
	//printf("NOW=%f,node<%d>,backoffHandler() in MYMac \n",NOW,index_);
	if(slot_count == 0)
	{
		if(pktCTRL_) 
		{
			assert(mhTxPkt_.busy() || mhDefer_.busy());
			return;
		}
		if(check_pktRTS() == 0)
			return;
		if(check_pktTx() == 0)
			return;
	}
	else
	{
		mhBackoff_.start(cw_, is_idle());
	}
	

}
int MYMac::check_pktRTS()
{
	//printf("NOW=%f,node<%d>,check_pktRTS() in MYMac \n",NOW,index_);
	assert(mhBackoff_.busy() == 0);
	double timeout;
	if(pktRTS_ == 0)
	{
		//printf("in check_pktRTS(),pktRTS_ == 0, no need send RTS\n");
		return -1;
	}	
	//printf("in check_pktRTS(),pktRTS_ ！= 0, need transmit RTS\n");
	struct hdr_mymac *mh = HDR_MYMAC(pktRTS_);
 	switch(mh->dh_fc.fc_subtype) 	//确认是RTS包类型
	 {
		case MAC_Subtype_RTS:
			if(! is_idle()) 	//当信道不空闲，则进入退避
			{
				//inc_cw();		//暂定不适用二进制指数退避，用固定CW
				mhBackoff_.start(cw_, is_idle());		
				return 0;
			}
			//当信道空闲，则计算发送RTS的发送定时器结束时间（预留接收CTS的时间）
			setTxState(MAC_RTS);		//设置信道发送状态MAC_RTS
			timeout = txtime(phymib_->getRTSlen(), bandwidth_)
					+ DSSS_MaxPropagationDelay                      // XXX
					+ phymib_->getSIFS()
					+ txtime(phymib_->getCTSlen(), bandwidth_)
					+ DSSS_MaxPropagationDelay;
			break;
		default:
			fprintf(stderr, "check_pktRTS:Invalid MAC Control subtype\n");
			exit(1);
	}
	transmit(pktRTS_, timeout);		//正式信道往外发送
	return 0;
}
int MYMac::check_pktCTRL()
{
	//printf("NOW=%f,node<%d>,check_pktCTRL() in MYMac \n",NOW,index_);
	double timeout;
	if(pktCTRL_ == 0)
		return -1;
	if(tx_state_ == MAC_CTS || tx_state_ == MAC_ACK)
		return -1;
	struct hdr_mymac *mh = HDR_MYMAC(pktCTRL_);		  
	switch(mh->dh_fc.fc_subtype) 
	{
	case MAC_Subtype_CTS:
		if(!is_idle()) //如果信道不是空闲的，不要发送CTS
		{
			discard(pktCTRL_, DROP_MAC_BUSY); 
			pktCTRL_ = 0;
			return 0;
		}
		setTxState(MAC_CTS);
		//这个超时是猜测，因为它没有指定	
		 timeout = txtime(phymib_->getCTSlen(), bandwidth_)
                        + DSSS_MaxPropagationDelay                      // XXX
                        + sec(mh->dh_duration)
                        + DSSS_MaxPropagationDelay                      // XXX
                        - phymib_->getSIFS()
                        - txtime(phymib_->getACKlen(), bandwidth_);
		break;
	//IEEE 802.11规范，在SIFS之后发送ACK，不考虑信道的忙/空闲状态。
	case MAC_Subtype_ACK:		
		setTxState(MAC_ACK);
		timeout = txtime(phymib_->getACKlen(), bandwidth_);
		break;
	default:
		fprintf(stderr, "check_pktCTRL:Invalid MAC Control subtype\n");
		exit(1);
	}
	transmit(pktCTRL_, timeout);
	return 0;
}

int MYMac::check_pktTx()
{
	//printf("NOW=%f,node<%d>,check_pktTx() in MYMac \n",NOW,index_);
	assert(mhBackoff_.busy() == 0);
	double timeout;
	if(pktTx_ == 0)
	{
		//printf("in check_pktTx(),pktTx_ == 0, no need send pktTx_\n");
		return -1;
	}
	//printf("in check_pktTx(),pktTx_ ！= 0, need transmit pktTx_\n");
	struct hdr_mymac *mh = HDR_MYMAC(pktTx_);
	//是上层传下来的数据包
	switch(mh->dh_fc.fc_subtype) 
	{
		case MAC_Subtype_Data:
			//信道是否空闲，忙，则增加竞争窗口大小，开启backoff定时器，
			//backoff定时器会等信道空闲后真正开始。
			if(!is_idle()) 	
			{
				sendRTS(ETHER_ADDR(mh->dh_da));
				//inc_cw();
				mhBackoff_.start(cw_, is_idle());
				return 0;
			}
			//信道是否空闲，闲，则设置信道发送状态MAC_SEND，并计算传输时间，
			setTxState(MAC_SEND);
			//广播包不需要发送SIFS和ACK
			//printf("TX_Time(pktTx_)=%f,DSSS_MaxPropagationDelay=%f,phymib_->getSIFS()=%f\n",TX_Time(pktTx_),DSSS_MaxPropagationDelay,phymib_->getSIFS());
			//printf("txtime(phymib_->getACKlen(), bandwidth_)=%f,DSSS_MaxPropagationDelay=%f\n",txtime(phymib_->getACKlen(), bandwidth_),DSSS_MaxPropagationDelay);
			if((u_int32_t)ETHER_ADDR(mh->dh_da) != MAC_BROADCAST)
                timeout = txtime(pktTx_)
                        + DSSS_MaxPropagationDelay              // XXX
                        + phymib_->getSIFS()
                        + txtime(phymib_->getACKlen(), bandwidth_)
                        + DSSS_MaxPropagationDelay;             // XXX
			else
				timeout = txtime(pktTx_);
			break;
	default:
		fprintf(stderr, "check_pktTx:Invalid MAC Control subtype\n");
		exit(1);
	}
	transmit(pktTx_, timeout);
	return 0;
}
void MYMac::deferHandler() 
{
	//printf("NOW=%f,node<%d>,deferHandler() in MYMac \n",NOW,index_);
	assert(pktCTRL_ || pktRTS_ || pktTx_);
	if(check_pktCTRL() == 0)
		return;
	assert(mhBackoff_.busy() == 0);
	if(check_pktRTS() == 0)
		return;
	if(check_pktTx() == 0)
		return;
}
/* Test if the channel is idle. */
int MYMac::is_idle() 
{
	if(rx_state_ != MAC_IDLE)
		return 0;
	if(tx_state_ != MAC_IDLE)
		return 0;
	if(nav_ > Scheduler::instance().clock())
		return 0;	
	//可以在这添加时隙count？
	return 1;
}
void MYMac::recvHello(Packet *p) 
{
	struct hdr_mac_hello *hef =HDR_MAC_HELLO(p);
	//printf("NOW=%f,node<%d>,recvHello() from node<%d> in MAC \n",NOW,index_,hef->hello_src);
	Node *nb = Node::get_node_by_address(hef->hello_src);
	MY_Neighbor *tmp = nb_lookup(hef->hello_src);
	//printf("nb_lookup\n");
	//当第一次发现该节点，则插入邻居节点表。
	if(tmp == 0) 
    {
	    nb_insert(hef->hello_src);
        //printf("NOW=%f,node<%d> nb_insert() node<%d> succ \n",NOW,index,heh->hello_src);
        //thisnode->nb_print();
	}
	tmp = nb_lookup(hef->hello_src);
	assert(tmp);
	//更新邻居节点表内的地址位置信息。
	tmp->x_local=hef->lx;
	tmp->y_local=hef->ly;
	//printf("hef->lx=%f,hef->ly=%f\n",hef->lx,hef->ly);
	//printf("tmp->lx=%f,tmp->ly=%f\n",tmp->lx,tmp->ly);
	//tmp->CHID=hef->CHID;		//暂时未在hello中添加节点的簇首ID
	calculate_cellular_nb(tmp->x_local,tmp->y_local,tmp->line_num,tmp->column_num,tmp->area_num,tmp->distance_to_center);
	//printf("node<%d>,lx=%f,ly=%f,line_num=%d,column_num=%d,area_num=%d,distance_to_center=%f\n",tmp->nb_addr,tmp->x_local,tmp->y_local,tmp->line_num,tmp->column_num,tmp->area_num,tmp->distance_to_center);
	//收到hello后更新邻居节点过期时间，未添加
	//nb->nb_expire = CURRENT_TIME +	(1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
    Packet::free(p);
}
void MYMac::recvHeadHello(Packet *p) 
{
	if(((MobileNode *)thisnode)->clustre_head_flag!=index_)
	{
		Packet::free(p);
		return;
	}
	struct hdr_mac_headhello *hhf =HDR_MAC_HEADHELLO(p);
	//printf("NOW=%f,node<%d>,recvHeadHello() from node<%d> in MAC \n",NOW,index_,hhf->headhello_src);
	Node *nb = Node::get_node_by_address(hhf->headhello_src);
	MY_NeighborCluster *tmp = nc_lookup(hhf->headhello_src);
	//当第一次发现该节点，则插入邻居节点表。
	if(tmp == 0) 
    {
	    nc_insert(hhf->headhello_src);
        //printf("NOW=%f,node<%d> nb_insert() node<%d> succ \n",NOW,index,heh->hello_src);
        //thisnode->nb_print();
	}
	tmp = nc_lookup(hhf->headhello_src);
	assert(tmp);
	tmp->num_cluster_in_node_=hhf->num_cluster_in_node_;
	tmp->line_num=hhf->line_num;
	tmp->column_num=hhf->column_num;
	for(int i=0;i<hhf->num_cluster_in_node_;i++)
	{
		tmp->cluster_in_node_[i].nb_addr=hhf->cluster_in_node_[i].nb_addr;
		tmp->cluster_in_node_[i].x_local=hhf->cluster_in_node_[i].x_local;
		tmp->cluster_in_node_[i].y_local=hhf->cluster_in_node_[i].y_local;
	}
	//printf("tmp->cluster_in_node_[0].nb_addr=%d,cluster_in_node_[0].x_local=%f\n",tmp->cluster_in_node_[0].nb_addr,tmp->cluster_in_node_[0].x_local);
	//printf("tmp->cluster_in_node_[1].nb_addr=%d,cluster_in_node_[1].x_local=%f\n",tmp->cluster_in_node_[1].nb_addr,tmp->cluster_in_node_[1].x_local);

    Packet::free(p);
}
void MYMac::recvRTS(Packet *p)
{
	if(index_==1)
	{
		printf("RRTS %f\n",NOW);
	}
	struct rts_frame *rf = (struct rts_frame*)p->access(hdr_mac::offset_);
	//printf("NOW=%f,node<%d>,recvRTS() from node<%d> in MAC \n",NOW,index_,rf->rf_sa);
	//如果信道在忙着发送其他包，则丢弃这个包。
	if(tx_state_ != MAC_IDLE) 
	{
		discard(p, DROP_MAC_BUSY);
		return;
	}
	//如果我在回复别人，就把这个RTS扔掉
	if(pktCTRL_) 
	{
		discard(p, DROP_MAC_BUSY);
		return;
	}
	//设置CTS包
	sendCTS(ETHER_ADDR(rf->rf_sa), rf->rf_duration);
	//停止延迟-将在tx_resume()中重置
	if(mhDefer_.busy()) 
		mhDefer_.stop();
	tx_resume();
	//mac_log(p);
}
void MYMac::recvCTS(Packet *p)
{
	//printf("NOW=%f,node<%d>,recvCTS()\n",NOW,index_);
	if(index_==0)
	{
		printf("RCTS %f\n",NOW);
	}
	if(tx_state_ != MAC_RTS) 
	{
		discard(p, DROP_MAC_INVALID_STATE);
		return;
	}
	assert(pktRTS_);
	//释放RTS包。
	Packet::free(pktRTS_); 
	pktRTS_ = 0;
	assert(pktTx_);	
	//停止发送计时器,因为在发送RTS，定时器timeout时间为接收到CTS
	mhTxPkt_.stop();
	//成功接收到这个CTS包意味着RTS是发送成功的。
	//根据IEEE规范9.2.5.3，必须重置ssrc_，但不能重置拥塞窗口。
	//ssrc_ = 0;
	if(mhDefer_.busy()) 
		mhDefer_.stop();
	tx_resume();
	//mac_log(p);
}
void MYMac::recvACK(Packet *p)
{	
	//printf("NOW=%f,node<%d>,recvACK()\n",NOW,index_);
	if(index_==0)
	{
		printf("RACK %f\n",NOW);
	}
	if(tx_state_ != MAC_SEND) 	//DATA是否有发送过，没有则直接丢弃该ACK
	{
		discard(p, DROP_MAC_INVALID_STATE);
		return;
	}
	assert(pktTx_);
	mhTxPkt_.stop();		//停止发送计时器
	//成功接收到这个ACK包意味着我们的数据传输是成功的。
	//mac-802.11.cc在这重置Short/Long Retry Count and the CW。 有需要???	ssrc_，slrc_
	//需要检查我们发送的被ACK的数据包的大小，而不是ACK数据包的大小。
	//rst_cw();
	Packet::free(pktTx_); 
	pktTx_ = 0;
	//开始退避，准备下一次数据传输
	assert(mhBackoff_.busy() == 0);
	mhBackoff_.start(cw_, is_idle());
	if(mhDefer_.busy()) 
		mhDefer_.stop();
	tx_resume();
	//mac_log(p);
}
/* Actually receive data packet when RxPktTimer times out. */
void MYMac::recvDATA(Packet *p)
{ 
	//printf("NOW=%f,node<%d>,recvDATA()\n",NOW,index_);
	if(index_==1)
	{
		printf("RDATA %f\n",NOW);
	}
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mymac *dh = HDR_MYMAC(p);
	struct hdr_myprot *hch = HDR_MYPROT(p);
	/*Adjust the MAC packet size: strip off the mac header.*/
	u_int32_t dst, src, size;
	dst = ETHER_ADDR(dh->dh_da);
	src = ETHER_ADDR(dh->dh_sa);
	size = ch->size();
	//当包的内容放大，是不是在收到包后在这全部减小？先除后减？
	//调整MAC包的大小:去掉MAC的头
	if(hch->myprot_type == MYPROT_RREQ||hch->myprot_type == MYPROT_RREP)
		ch->size()=ch->size()/4;
	ch->size() -= ETHER_HDR_LEN;
	ch->num_forwards() += 1;

	if(dst != MAC_BROADCAST) 
	{
		if (tx_state_ == MAC_CTS) 	//如果已经发过CTS，则清除pktCTRL_（CTS）
		{
			assert(pktCTRL_);
			Packet::free(pktCTRL_);
			pktCTRL_ = 0;
			mhTxPkt_.stop();
		} 
		else 
		{
			discard(p, DROP_MAC_BUSY);
			return;
		}
		sendACK(src);		//发送ACK
		if(mhDefer_.busy()) 
		mhDefer_.stop();
		tx_resume();
	}
	//MAC-802.11.CC在这，根据序列号，创建/更新一个缓存队列，需要添加？？？

	/* Pass the packet up to the link-layer.*/
	uptarget_->recv(p, (Handler*) 0);
}
void MYMac::tx_resume()
{
	//printf("NOW=%f,node<%d>,tx_resume()\n",NOW,index_);
	double rTime;
	assert(mhTxPkt_.busy() == 0);
	assert(mhDefer_.busy() == 0);
	
	if(pktCTRL_) //需要发送CTS或ACK,开启defer定时器SIFS
	{
		//printf("node<%d>,in tx_resume(),need send pktCTRL_\n",index_);
		//printf("cts or ack\n");
		mhDefer_.start(phymib_->getSIFS());		//使用defer，能不能改成backoff
	} 
	else if(pktRTS_) //有RTS需要发送
	{
		//printf("node<%d>,in tx_resume(),need send pktRTS_\n",index_);
		if (mhBackoff_.busy() == 0) //banckoff定时器空闲，则开启backoff定时器，准备发送RTS
		{
			//printf("rts\n");
			mhBackoff_.start(cw_, is_idle(), phymib_->getDIFS());
		}//如果backcoff定时器在忙，则直接等待backoffhandler
	} 
	else if(pktTx_) //有数据包要发送
	{
		//printf("node<%d>,in tx_resume(),need send pktTx_\n",index_);
		//printf("data\n");
		if (mhBackoff_.busy() == 0) //banckoff定时器空闲
		{
			hdr_cmn *ch = HDR_CMN(pktTx_);
			struct hdr_mymac *mh = HDR_MYMAC(pktTx_);
			if ((u_int32_t) ETHER_ADDR(mh->dh_da) == MAC_BROADCAST)		//广播的包，直接退避发送
			{
				struct hdr_myprot *hch = HDR_MYPROT(pktTx_);
				if(hch->myprot_type == MYPROT_RREQ||hch->myprot_type == MYPROT_RREP)
					mhBackoff_.start(cw_, is_idle(),phymib_->getDIFS_RP());
				else	
					mhBackoff_.start(cw_, is_idle(),phymib_->getDIFS());
            } 
			else 		//指向的包，先defer
			{
				mhDefer_.start(phymib_->getSIFS());		//defer SIFS
            }
		}
	} 
	//当从MAC队列中取出数据包传递给MAC层对象的recv()时，同时传递一个QueueHandle对象的引用给MAC层作为callback_，
	//MAC层完成一个上层送来的数据包的发送后，会使用此callback_，从MAC层队列获取新的需要发送的数据包。
	else if(callback_) 
	{
		//printf("node<%d>,tx_resume(),callback_\n",index_);
		Handler *h = callback_;
		callback_ = 0;
		h->handle((Event*) 0);
	}
	setTxState(MAC_IDLE);		//置信道发送状态MAC_IDLE
}
void MYMac::discard(Packet *p, const char* why)
{
	hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mymac* mh = HDR_MYMAC(p);
	/* if the rcvd pkt contains errors, a real MAC layer couldn't
	   necessarily read any data from it, so we just toss it now */
	if(ch->error() != 0) 
	{
		Packet::free(p);
		return;
	}
	drop(p, why);
	Packet::free(p);
}
void MYMac::hello_backoff()
{
	struct hdr_cmn* ch = HDR_CMN(pktHELLO_);
	double delay_num=0.0;
	delay_num= Random::integer(delay_Max);
	double delay=0.0;
	//delay=delay_num*delay_time;
	delay=delay_num*DELAY_TIME;
	//double left_temp=SLOT_TIME*((int)(NOW/SLOT_TIME+1));
	//left_time=left_temp-NOW-delay;
	double t1=NOW/SLOT_TIME;
	double t2=t1+1;
	int t3=floor(t2+0.0000005);
	double left_temp=SLOT_TIME*t3;
	double left_time=0.0;
	left_time=left_temp-NOW-delay;
	double tx_remain_time=0.0;
    tx_remain_time = DATA_Time(ch->size());//计算该包发需要的时间
	//printf("node<%d> delay=%f,left_time=%f,tx_remain_time=%f\n",index_,delay,left_time,tx_remain_time);
	if(tx_remain_time<left_time)
	{
		//printf("node<%d> have enough time to tx \n",index_);
		mhHelloBackoff_.start(delay); 		//pktTx为Packet *   pktRx_;
	}
	else
	{
		//printf("node<%d> have no enough time to tx \n",index_);
		//当时间不够发送hello，需要将pktHELLO_丢弃。
		Packet::free(pktHELLO_);
		pktHELLO_ = 0;
		return;
	}
}
void MYMac::headhello_backoff()
{
	struct hdr_cmn* ch = HDR_CMN(pktHeadHELLO_);
	double delay_num=0.0;
	delay_num= Random::integer(delay_Max);
	double delay=0.0;
	//delay=delay_num*delay_time;
	delay=delay_num*DELAY_TIME;
	//double left_temp=SLOT_TIME*((int)(NOW/SLOT_TIME+1));
	//left_time=left_temp-NOW-delay;
	mhHHBackoff_.start(delay); 	
	//printf("node<%d>:now=%f,delay=%f in headhello_backoff\n",index_,NOW,delay);	
	

}

//ll层调用，用来地址解析IP->MAC
/* ======================================================================
   Packet Headers Routines
   ====================================================================== */
int MYMac::hdr_dst(char* hdr, int dst )
{
	//printf("in MYMac::hdr_dst(),dst=%d\n",dst);
	struct hdr_mymac *dh = (struct hdr_mymac*) hdr;
	if(dst > -2)
		STORE4BYTE(&dst, (dh->dh_da));
	return ETHER_ADDR(dh->dh_da);
}

int MYMac::hdr_src(char* hdr, int src )
{
	struct hdr_mymac *dh = (struct hdr_mymac*) hdr;
	if(src > -2)
		STORE4BYTE(&src, (dh->dh_sa));
  
	return ETHER_ADDR(dh->dh_sa);
}

int MYMac::hdr_type(char* hdr, u_int16_t type) 
{
	struct hdr_mymac *dh = (struct hdr_mymac*) hdr;
	if(type)
		STORE2BYTE(&type,(dh->dh_body));
	return GET2BYTE(dh->dh_body);
}

void MYMac::calculate_cellular_nb(double lx,double ly,int& line_num,int& column_num,int& area_num,double& distance_to_center)//根据该节点的地理位置信息，计算该节点所在的簇的编号(line_num, column_num)，以及该节点所在的小区域
{
	//printf("calculate_cellular_area() in MYMac \n");
	POINT_XY points[3];	//定义三个蜂窝的中心节点，用于比较本节点离哪个中心最近
	int cx=0,cy=0;
	cx=(int)(lx/unitx);//横轴位置       double强制转换成int类型
	cy=(int)(ly/unity);//纵轴位置
	//printf("calculate_cellular_nb(),lx=%lf,ly=%lf,unitx=%lf,unity=%lf,cx=%d,cy=%d\n",lx,ly,unitx,unity,cx,cy);
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
		dist=(lx-points[i].x)*(lx-points[i].x)+(ly-points[i].y)*(ly-points[i].y);
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
	distance_to_center=sqrt(mindist);
	//printf("point_index=%d\n",point_index);
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
		//printf("[%d] ,x=%f, y=%f\n",i,points[i].x,points[i].y);
	//}
	tmp_x=lx-points[point_index].x;
	tmp_y=ly-points[point_index].y;
	//printf("tmp_x=%f,tmp_y=%f \n",tmp_x,tmp_y);
	if(tmp_x-0 > 1.0e-9)
	{
		if(tmp_y >= sqrt(3)/3.0*tmp_x)
			area_num=0;
		else if(tmp_y <= sqrt(3)/3.0*tmp_x && tmp_y >= -sqrt(3)/3.0*tmp_x)
			area_num=1;
		else
			area_num=2;
	}
	else if(tmp_x-0 < -1.0e-9)
	{
		if(tmp_y <= sqrt(3)/3.0*tmp_x)
			area_num=3;
		else if(tmp_y >= sqrt(3)/3.0*tmp_x && tmp_y <= -sqrt(3)/3.0*tmp_x)
			area_num=4;
		else
			area_num=5;
	}
	else //if(tmp_x==0 && tmp_y==0)
	{
		area_num=0;
	}
}
//简单的簇首竞选实现，根据距离中心的距离，决定簇首。不考虑多簇首情况。
void MYMac::CH_Campaign()
{
	double min_distance=RADIUS;		//初始距离中心的最小距离
	((MobileNode *)thisnode)->clustre_head_flag=-1;
	MY_Neighbor *nb = ((MobileNode *)thisnode)->mnbhead.lh_first;
	for(; nb; nb = nb->mynb_link.le_next) 
	{
		//当该邻居节点与自己处于同一个蜂窝，才进行簇首竞选。
		if(line_num == nb->line_num && column_num == nb->column_num)
		{
			if(nb->distance_to_center < min_distance)
			{
				min_distance=nb->distance_to_center;
				((MobileNode *)thisnode)->clustre_head_flag=nb->nb_addr;
				//((MobileNode *)thisnode)->clustre_head_flag_lx=nb->lx;
				//((MobileNode *)thisnode)->clustre_head_flag_ly=nb->ly;
			}
		}
 	}
	if(distance_to_center < min_distance)		//当自己的地理位置是距离蜂窝中心最近的，则当选簇首
	{
		((MobileNode *)thisnode)->clustre_head_flag = index_;
	}

	//printf("node<%d>,CH_Campaign(),CHID=%d\n",index_,thisnode->CHID);
}

double MYMac::txtime(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	double t = ch->txtime();
	if (t < 0.0) {
		drop(p, "XXX");
 		exit(1);
	}
	return t;
}

//txtime(),计算传输时间，"psz" bytes at rate "drt=bandwidth_" bps,
double MYMac::txtime(double psz, double drt)
{
	//printf("txtime:%f,%f\n",psz,drt);
	double dsz = psz - phymib_->getPLCPhdrLen();
	//printf("dsz=%f\n",dsz);
	int plcp_hdr = phymib_->getPLCPhdrLen() << 3;
	//printf("plcp_hdr=%d\n",plcp_hdr);	
	int datalen = (int)dsz << 3;
	//printf("datalen=%d\n",datalen);
	double t = (((double)plcp_hdr)/phymib_->getPLCPDataRate()) + (((double)datalen)/drt);
	return(t);
}

//配置RTS包,dst是目的节点的MAC地址
void MYMac::sendRTS(int dst)
{
	//printf("NOW=%f,node<%d>,sendRTS()\n",NOW,index_);
	Packet *p = Packet::alloc();		//分配空间，后续根据需要设置
	hdr_cmn* ch = HDR_CMN(p);
	struct rts_frame *rf = (struct rts_frame*)p->access(hdr_mac::offset_);
	assert(pktTx_);
	assert(pktRTS_ == 0);
	//printf("dest=%#X,MAC_BROADCAST=%#X\n",dst,MAC_BROADCAST);
	//广播包不需要发送RTS
	if( (u_int32_t) dst == MAC_BROADCAST) 
	{
		//printf("in sendRTS(),dst == MAC_BROADCAST no need to send RTS\n");
		Packet::free(p);
		return;
	}

	//不是广播包，则设置RTS包内容
	//printf("in sendRTS(),dst != MAC_BROADCAST, need to send RTS\n");
	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = phymib_->getRTSlen();
	ch->iface() = -2;
	ch->error() = 0;
	bzero(rf, MAC_HDR_LEN);
	rf->rf_fc.fc_protocol_version = MAC_ProtocolVersion;
 	rf->rf_fc.fc_type	= MAC_Type_Control;
 	rf->rf_fc.fc_subtype	= MAC_Subtype_RTS;
	rf->rf_fc.fc_to_ds	= 0;
	rf->rf_fc.fc_from_ds	= 0;
 	rf->rf_fc.fc_more_frag	= 0;
 	rf->rf_fc.fc_retry	= 0;
 	rf->rf_fc.fc_pwr_mgt	= 0;
 	rf->rf_fc.fc_more_data	= 0;
 	rf->rf_fc.fc_wep	= 0;
 	rf->rf_fc.fc_order	= 0;
	STORE4BYTE(&dst, (rf->rf_da));		//RTS目的地址
	STORE4BYTE(&index_, (rf->rf_sa));	//RTS源地址
 	ch->txtime() = txtime(ch->size(), bandwidth_);	//存储RTS发送时间
	//计算rts持续时间字段,NAV(RTS)
	rf->rf_duration = usec(phymib_->getSIFS()
			       + txtime(phymib_->getCTSlen(), bandwidth_)
			       + phymib_->getSIFS()
                   + txtime(pktTx_)
			       + phymib_->getSIFS()
			       + txtime(phymib_->getACKlen(), bandwidth_));
	pktRTS_ = p;
}
//设置CTS
void MYMac::sendCTS(int dst, double rts_duration)
{
	//printf("NOW=%f,node<%d>,sendCTS()\n",NOW,index_);
	Packet *p = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(p);
	struct cts_frame *cf = (struct cts_frame*)p->access(hdr_mac::offset_);
	assert(pktCTRL_ == 0);
	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = phymib_->getCTSlen();
	ch->iface() = -2;
	ch->error() = 0;
	bzero(cf, MAC_HDR_LEN);
	cf->cf_fc.fc_protocol_version = MAC_ProtocolVersion;
	cf->cf_fc.fc_type	= MAC_Type_Control;
	cf->cf_fc.fc_subtype	= MAC_Subtype_CTS;
 	cf->cf_fc.fc_to_ds	= 0;
	cf->cf_fc.fc_from_ds	= 0;
 	cf->cf_fc.fc_more_frag	= 0;
 	cf->cf_fc.fc_retry	= 0;
 	cf->cf_fc.fc_pwr_mgt	= 0;
 	cf->cf_fc.fc_more_data	= 0;
 	cf->cf_fc.fc_wep	= 0;
 	cf->cf_fc.fc_order	= 0;
	STORE4BYTE(&dst, (cf->cf_da));
	ch->txtime() = txtime(ch->size(), bandwidth_);
	//计算CTS持续时间字段,NAV(CTS)
	cf->cf_duration = usec(sec(rts_duration)
                              - phymib_->getSIFS()
                              - txtime(phymib_->getCTSlen(), bandwidth_));
	pktCTRL_ = p;
}
void MYMac::sendACK(int dst)
{
	//printf("NOW=%f,node<%d>,sendACK()\n",NOW,index_);
	Packet *p = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(p);
	struct ack_frame *af = (struct ack_frame*)p->access(hdr_mac::offset_);
	assert(pktCTRL_ == 0);
	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = phymib_->getACKlen();
	ch->iface() = -2;
	ch->error() = 0;
	bzero(af, MAC_HDR_LEN);
	af->af_fc.fc_protocol_version = MAC_ProtocolVersion;
 	af->af_fc.fc_type	= MAC_Type_Control;
 	af->af_fc.fc_subtype	= MAC_Subtype_ACK;
 	af->af_fc.fc_to_ds	= 0;
 	af->af_fc.fc_from_ds	= 0;
 	af->af_fc.fc_more_frag	= 0;
 	af->af_fc.fc_retry	= 0;
 	af->af_fc.fc_pwr_mgt	= 0;
 	af->af_fc.fc_more_data	= 0;
 	af->af_fc.fc_wep	= 0;
 	af->af_fc.fc_order	= 0;
	STORE4BYTE(&dst, (af->af_da));
 	ch->txtime() = txtime(ch->size(), bandwidth_);
 	af->af_duration = 0;	
	pktCTRL_ = p;
}

inline void MYMac::checkBackoffTimer()
{
	//printf("NOW=%f,node<%d>,checkBackoffTimer()\n",NOW,index_);
	//printf("is_idle()=%d,mhBackoff_.busy()=%d,mhBackoff_.paused()=%d\n",is_idle(),mhBackoff_.busy(),mhBackoff_.paused());
	if(is_idle() && mhBackoff_.paused())
	{
		mhBackoff_.resume(phymib_->getDIFS());
	}
	if(! is_idle() && mhBackoff_.busy() && ! mhBackoff_.paused())
		mhBackoff_.pause();
	//printf("then,is_idle()=%d,mhBackoff_.busy()=%d,mhBackoff_.paused()=%d\n",is_idle(),mhBackoff_.busy(),mhBackoff_.paused());
}
inline void MYMac::setRxState(MacState newState)
{
	//printf("NOW=%f,node<%d>,setRxState()\n",NOW,index_);
	rx_state_ = newState;
	checkBackoffTimer();
}

inline void MYMac::setTxState(MacState newState)
{
	//printf("NOW=%f,node<%d>,setTxState()\n",NOW,index_);
	tx_state_ = newState;
	checkBackoffTimer();
}
//根据包大小计算的包传输定时时长，包传输完成
void MYMac::txHandler()
{
	//printf("NOW=%f,node<%d>,txHandler()\n",NOW,index_);
	tx_active_ = 0;
	//Packet::free((Packet *)e);
}



void MYMac::calculate_cellular_area()//根据该节点的地理位置信息，计算该节点所在的簇的编号(line_num, column_num)，以及该节点所在的小区域
{
	//printf("calculate_cellular_area() in MobileNode \n");
	POINT_XY points[3];	//定义三个蜂窝的中心节点，用于比较本节点离哪个中心最近
	double lx=0,ly=0,lz=0;
	//Node *thisnode;
	//thisnode = Node::get_node_by_address(index);
	((MobileNode *)thisnode)->getLoc(&lx, &ly, &lz);
	int cx=0,cy=0;
	cx=(int)(lx/unitx);//横轴位置       double强制转换成int类型
	cy=(int)(ly/unity);//纵轴位置

	//printf("lx=%lf,ly=%lf,unitx=%lf,unity=%lf,cx=%d,cy=%d \n",lx,ly,unitx,unity,cx,cy);
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
		dist=(lx-points[i].x)*(lx-points[i].x)+(ly-points[i].y)*(ly-points[i].y);
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
	//printf("mindist=%f \n",mindist);
	distance_to_center=sqrt(mindist);
	//printf("distance_to_center=%f \n",distance_to_center);
	//printf("%d \n",point_index);
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
		//printf("[%d] ,x=%f, y=%f\n",i,points[i].x,points[i].y);
	//}
	tmp_x=lx-points[point_index].x;
	tmp_y=ly-points[point_index].y;
	//printf("tmp_x=%f,tmp_y=%f \n",tmp_x,tmp_y);
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



MY_Neighbor*
MYMac::nb_lookup(nsaddr_t id) {
 //thisnode = Node::get_node_by_address(index);
 MY_Neighbor *nb = ((MobileNode *)thisnode)->mnbhead.lh_first;
 //printf("nb=%d\n",nb);
 for(; nb; nb = nb->mynb_link.le_next) {
   if(nb->nb_addr == id) 
   		break;
 }
 return nb;
}
#define HELLO_INTERVAL          1               // 1000 ms   时间需要再确定一下
#define ALLOWED_HELLO_LOSS      3               // packets
void
MYMac::nb_insert(nsaddr_t id) {
MY_Neighbor *nb = new MY_Neighbor(id);

 assert(nb);
 nb->nb_expire = CURRENT_TIME +
                (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);				//有关于时间的操作需要重新确认一下
 LIST_INSERT_HEAD(&(((MobileNode *)thisnode)->mnbhead), nb, mynb_link);
}

MY_NeighborCluster*
MYMac::nc_lookup(nsaddr_t id) {
 //Node *thisnode;
 //thisnode = Node::get_node_by_address(index);
 MY_NeighborCluster *nc = ((MobileNode *)thisnode)->mnchead.lh_first;

 for(; nc; nc = nc->mync_link.le_next) {
   if(nc->nb_addr == id) break;
 }
 return nc;
}

void
MYMac::nc_insert(nsaddr_t id) {
MY_NeighborCluster *nc = new MY_NeighborCluster(id);

 assert(nc);
 nc->nb_expire = CURRENT_TIME +
                (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
 LIST_INSERT_HEAD(&(((MobileNode *)thisnode)->mnchead),nc, mync_link);
}
