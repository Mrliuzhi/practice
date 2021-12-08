#ifndef ns_mymac_h
#define ns_mymac_h

#include <packet.h>
#include <arp.h>
#include <connector.h>
#include "marshall.h"
#include <ll.h>
#include <delay.h>
#include <random.h>
#include <mac.h>
#include <stddef.h>
#include "mobilenode.h"
#include "node.h"
#include "wireless-phy.h"
#include "cmu-trace.h"
#include <math.h>
#include "address.h"
#include   <ctime> 
#include <myprot/myprotpkt.h>
#include <myprot/myprot.h>
//设置 DSSS PHY MIB 默认参数，物理层，IEEE 802.11 Spec
#define DSSS_CWMin			        31
#define DSSS_CWMax			        1023
#define DSSS_SlotTime			    0.000020	// 20us
#define	DSSS_CCATime			    0.000015	// 15us
#define DSSS_RxTxTurnaroundTime		0.000005	// 5us
#define DSSS_SIFSTime			    0.000010	// 10us
#define DSSS_PreambleLength		    144		    // 144 bits
#define DSSS_PLCPHeaderLength		48		    // 48 bits
#define DSSS_PLCPDataRate			1.0e6	    // 1Mbps

//必须考虑信道模型在何时添加的传播延迟,计算tx超时
#define DSSS_MaxPropagationDelay        0.000002        // 2us   XXXX

//TDMA 时隙，退避
#define SLOT_TIME 0.01   //时隙间隔
#define delay_Max 10     //随机退避的最大值

#define HDR_MAC_HELLO(p) ((hdr_mac_hello *)hdr_mac::access(p))		//MYMac WQC


#define HDR_MAC_HEADHELLO(p) ((hdr_mac_headhello *)hdr_mac::access(p))
#define SET_RX_STATE(x)			\
{								\
	rx_state_ = (x);			\
}

#define SET_TX_STATE(x)			\
{								\
	tx_state_ = (x);			\
}



#define	MAC_ProtocolVersion	0x00

#define MAC_Type_Management 0x00
#define MAC_Type_Control	0x01
#define MAC_Type_Data		0x02

#define MAC_Subtype_Data	0x00
#define MAC_Subtype_HELLO 	0x02
#define MAC_Subtype_HEADHELLO 0x03
#define MAC_Subtype_RTS		0x0B
#define MAC_Subtype_CTS		0x0C
#define MAC_Subtype_ACK		0x0D
#define DATA_DURATION           5

// We are using same header structure as 802.11 currently
struct frame_control 
{
	u_char		fc_subtype		: 4;
	u_char		fc_type			: 2;		
	u_char		fc_protocol_version	: 2;

	u_char		fc_order		: 1;
	u_char		fc_wep			: 1;
	u_char		fc_more_data		: 1;
	u_char		fc_pwr_mgt		: 1;
	u_char		fc_retry		: 1;
	u_char		fc_more_frag		: 1;
	u_char		fc_from_ds		: 1;
	u_char		fc_to_ds		: 1;
};
struct rts_frame 
{
	struct frame_control	rf_fc;
	u_int16_t		rf_duration;
	u_char			rf_da[ETHER_ADDR_LEN];
	u_char			rf_sa[ETHER_ADDR_LEN];
	u_char			rf_fcs[ETHER_FCS_LEN];
};
struct cts_frame 
{
	struct frame_control	cf_fc;
	u_int16_t		cf_duration;
	u_char			cf_da[ETHER_ADDR_LEN];
	u_char			cf_fcs[ETHER_FCS_LEN];
};
struct ack_frame 
{
	struct frame_control	af_fc;
	u_int16_t		af_duration;
	u_char			af_da[ETHER_ADDR_LEN];
	u_char			af_fcs[ETHER_FCS_LEN];
};
struct hdr_mac_hello
{
	struct frame_control	hello_fc;
	u_int16_t		hello_duration;
	u_char			hello_da[ETHER_ADDR_LEN];
	u_char			hello_sa[ETHER_ADDR_LEN];
	//u_int8_t        hello_type;             // Packet Type
    u_int8_t        hello_hop_count;           // Hop Count
    nsaddr_t        hello_src;                 // Source IP Address
	double lx;
	double ly;
	u_char			dh_body[1]; 		//ANSI兼容性的大小为1
};



#define MAX_NODE_NUM_ 64
struct Cluster_in_Node{
        nsaddr_t        nb_addr;
        double          x_local;
        double          y_local;  
};

struct hdr_mac_headhello
{
	struct frame_control	headhello_fc;
	u_int16_t		headhello_duration;
	u_char			headhello_da[ETHER_ADDR_LEN];
	u_char			headhello_sa[ETHER_ADDR_LEN];
	u_int8_t        headhello_hop_count;           // Hop Count
    nsaddr_t        headhello_src;
	u_int8_t 		line_num;
	u_int8_t 		column_num;
	u_int8_t		num_cluster_in_node_;
	Cluster_in_Node cluster_in_node_[MAX_NODE_NUM_];
	u_char			dh_body[1]; 		//ANSI兼容性的大小为1

	inline int size() { 
		int sz = 0;
		//printf("sizeof(frame_control)=%d,sizeof(u_char)=%d,sizeof(Cluster_in_Node)=%d\n",sizeof(frame_control),sizeof(u_char),sizeof(Cluster_in_Node));
			sz = sizeof(frame_control)		// rp_type
				+ 3*sizeof(u_char) 	// rp_flags + reserved
				+ sizeof(u_int16_t)		// rp_hop_count
				+ 4*sizeof(u_int8_t)		// rp_timestamp
				+ sizeof(nsaddr_t)		// rp_dst
				+ num_cluster_in_node_*sizeof(Cluster_in_Node);	// rp_sr
			assert (sz >= 0);
			return sz;
  }
};
//MAC层 TDMA协议头
struct hdr_mymac
{
	struct frame_control	dh_fc;
	u_int16_t		dh_duration;		//预留传输时间(RTS/CTS)
	u_char			dh_da[ETHER_ADDR_LEN];		//接收此帧的MAC地址
	u_char			dh_sa[ETHER_ADDR_LEN];		//传输此帧的MAC地址
	u_char			dh_bssid[ETHER_ADDR_LEN];
	u_int16_t		dh_scontrol;
	u_char			dh_body[1]; 		//ANSI兼容性的大小为1
};
class MYMacPHY_MIB   //具体的参数需要什么还未确定
{     
public:
	u_int32_t	CWMin;
	u_int32_t	CWMax;
	double		SlotTime;
	double		CCATime;
	double		RxTxTurnaroundTime;
	double		SIFSTime;
	u_int32_t	PreambleLength;
	u_int32_t	PLCPHeaderLength;
	double		PLCPDataRate;
	inline u_int32_t getCWMin() { return(CWMin); }
    inline u_int32_t getCWMax() { return(CWMax); }
	inline double getSIFS() { return(SIFSTime); }
	inline u_int32_t getPLCPhdrLen() { return((PreambleLength + PLCPHeaderLength) >> 3); }
	inline u_int32_t getRTSlen() { return(getPLCPhdrLen() + sizeof(struct rts_frame)); }
	inline u_int32_t getCTSlen() { return(getPLCPhdrLen() + sizeof(struct cts_frame)); }
	inline u_int32_t getACKlen() { return(getPLCPhdrLen() + sizeof(struct ack_frame)); }
	inline double getPLCPDataRate() { return(PLCPDataRate); }
	inline double getSlotTime() { return(SlotTime); }
	inline double getDIFS() { return(SIFSTime + 2 * SlotTime); }
	inline double getDIFS_RP() { return(SIFSTime +  0.5* SlotTime); }
	inline double getEIFS() { return(SIFSTime + getDIFS() + (8 *  getACKlen())/PLCPDataRate);	}
};
#define ETHER_HDR_LEN				\
	((phymib_->PreambleLength >> 3) +	\
	 (phymib_->PLCPHeaderLength >> 3) +	\
	 offsetof(struct hdr_mymac, dh_body[0] ) +	\
	 ETHER_FCS_LEN)

#define ETHER_HDR_HELLO_LEN				\
	((phymib_->PreambleLength >> 3) +	\
	 (phymib_->PLCPHeaderLength >> 3) +	\
	 offsetof(struct hdr_mac_hello, dh_body[0] ) +	\
	 ETHER_FCS_LEN)

#define ETHER_HDR_HEADHELLO_LEN				\
((phymib_->PreambleLength >> 3) +	\
(phymib_->PLCPHeaderLength >> 3) +	\
offsetof(struct hdr_mac_headhello, dh_body[0] ) +	\
ETHER_FCS_LEN)


#define DATA_Time(len)	(8 * (len) / bandwidth_)		//计算包需要发送的时间 
class MYMac;
//MYMacTimer 定时器
class MYMacTimer : public Handler 
{
public:
	MYMacTimer(MYMac* m) : mac(m) 
    {
		busy_ = paused_ = 0; 
        stime = rtime = 0.0;
	}
	virtual void handle(Event *) = 0;
	virtual void start(Packet *p, double time);
	virtual void start(double time);
	virtual void stop(Packet *p);
	virtual void stop(void);
	virtual void pause(void) { assert(0); }
	virtual void resume(void) { assert(0); }
	inline int busy(void) { return busy_; }
	inline int paused(void) { return paused_; }
	inline double expire(void) 
    {
		return ((stime + rtime) - Scheduler::instance().clock());
	}
protected:
	MYMac* mac;
	int	busy_;
	int	paused_;
	Event intr;
	double stime;	// start time
	double rtime;	// remaining time
};

//定时发送和接收的计时器
class SlotMYMacTimer : public MYMacTimer 
{
public:
	SlotMYMacTimer(MYMac *m) : MYMacTimer(m) {}
	void	handle(Event *);
};

//控制分组 发送时间 的计时器
class TxPktMYMacTimer : public MYMacTimer 
{
public:
	TxPktMYMacTimer(MYMac *m) : MYMacTimer(m) {}
	void	handle(Event *);
};
//控制分组 接收时间 的计时器
class RxPktMYMacTimer : public MYMacTimer 
{
public:
	RxPktMYMacTimer(MYMac *m) : MYMacTimer(m) {}
	void	handle(Event *);
};

class BackoffMYMacTimer : public MYMacTimer 
{
public:
	BackoffMYMacTimer(MYMac *m) : MYMacTimer(m), difs_wait(0.0) {}
	void	start(int cw, int idle, double difs = 0.0);
	void	handle(Event *);
	void	pause(void);
	void	resume(double difs);
private:
	double	difs_wait;
};
class DeferMYMacTimer : public MYMacTimer 
{
public:
	DeferMYMacTimer(MYMac *m) : MYMacTimer(m) {}

	void	start(double time);
	void	handle(Event *);
};
class IFMYMacTimer : public MYMacTimer 
{
public:
	IFMYMacTimer(MYMac *m) : MYMacTimer(m) {}
	void	handle(Event *);
};
class NavMYMacTimer : public MYMacTimer {
public:
	NavMYMacTimer(MYMac *m) : MYMacTimer(m) {}

	void	handle(Event *);
};
//退避定时器
class HelloBackoffMYMacTimer : public MYMacTimer 
{
public:
	HelloBackoffMYMacTimer(MYMac *m) : MYMacTimer(m) {}
	void	handle(Event *);
};
class HeadHelloBackoffMYMacTimer : public MYMacTimer 
{
public:
	HeadHelloBackoffMYMacTimer(MYMac *m) : MYMacTimer(m) {}
	void	handle(Event *);
};

// The actual 802.11 MAC class.
class MYMac : public Mac 
{
    //定时器，友元类
    friend class SlotMYMacTimer; 
    friend class TxPktMYMacTimer;
    friend class RxPktMYMacTimer;
    friend class HelloBackoffMYMacTimer;  
	friend class BackoffMYMacTimer; 
	friend class DeferMYMacTimer; 
	friend class IFMYMacTimer;
	friend class NavMYMacTimer;
	friend class HeadHelloBackoffMYMacTimer;
public:
    MYMac(MYMacPHY_MIB* p);    //构造函数，参数为 物理层配置参数
    void recv(Packet *p, Handler *h);   //处理接收的数据包

    //定时器处理函数
    void slotHandler();     //时隙定时器超时时，主要的处理函数
	void helloHandler();     //时隙定时器超时时，主要的处理函数
    void recvHandler();     //接收处理函数
    void sendHandler();     //发送处理函数  
    void hello_backoffHandler();  //声明Backoff退避定时处理函数
	void hello_backoff();   //退避函数声明
	void headhello_backoffHandler();
	void headhello_backoff();   //退避函数声明
	void backoffHandler();
	void deferHandler();
	void txHandler();
	void navHandler();
	void trace_event(char *eventtype, Packet *p) ;
	void trace_pkt(Packet *p) ;
	NsObject*	logtarget_;
	EventTrace *et_;
protected:
    MYMacPHY_MIB		*phymib_;       //物理层设置
private:
    int command(int argc, const char*const* argv);

	

    //定义一个定时器
    SlotMYMacTimer mhSlot_;
    TxPktMYMacTimer mhTxPkt_;
    RxPktMYMacTimer mhRxPkt_;
    HelloBackoffMYMacTimer mhHelloBackoff_;
	HeadHelloBackoffMYMacTimer	mhHHBackoff_;
	DeferMYMacTimer	mhDefer_;	// defer timer
	BackoffMYMacTimer	mhBackoff_;	// backoff timer
	IFMYMacTimer		mhIF_;		// interface timer
	NavMYMacTimer		mhNav_;		// NAV timer
	inline int	hdr_dst(char* hdr, int dst = -2);
  	inline int	hdr_src(char* hdr, int src = -2);
  	inline int	hdr_type(char* hdr, u_int16_t type = 0);  
	void    sendHello();
	void    sendUp(Packet* p);//从PHY收到发给LL层的包。
  	void    sendDown(Packet* p);//MAC向物理层发送包
	void transmitData();//实际是发送缓存的包
	void transmitHello(); 
	MacState	rx_state_;	// incoming state (MAC_RECV or MAC_IDLE)
  	MacState	tx_state_;	// outgoing state

	int		tx_active_;	// transmitter is ACTIVE
  	inline double TX_Time(Packet *p) 
  	{
    	double t = DATA_Time((HDR_CMN(p))->size());
    	//printf("<%d>, packet size: %d, tx-time: %f\n", index_, (HDR_CMN(p))->size(), t);
    	if(t < 0.0) 
		{
      		drop(p, "XXX");
      		exit(1);
    	}
    	return t;
  	}
	void		discard(Packet *p, const char* why);
	void		collision(Packet *p);

	inline int	is_idle(void);//测试信道是否空闲
	static double slot_time;//每个时隙的持续时间（时隙长度）
	int slot_count;			//分配的时隙数累计值
	bool calcu_flag=0;
	void recvHello(Packet *p);
	void recvDATA(Packet *p);
	Packet		*pktHELLO_;	// outgoing HELLO packet
	Packet		*pktHeadHELLO_;	// outgoing HeadHELLO packet
	Packet		*pktRTS_;	// outgoing RTS packet
	Packet		*pktCTRL_;	// outgoing non-RTS packet
	Node *thisnode;
	void calculate_cellular_nb(double lx,double ly,int& row,int& col,int& area_num,double& distance_to_center);
	void CH_Campaign();

	inline u_int16_t usec(double t) 
	{
		u_int16_t us = (u_int16_t)floor((t *= 1e6) + 0.5);
		return us;
	}
	inline double sec(double t) { return(t *= 1.0e-6); }
	double txtime(Packet *p);
	double txtime(double psz, double drt);
	double		nav_;		// Network Allocation Vector
	inline void set_nav(u_int16_t us) 
	{
		double now = Scheduler::instance().clock();
		double t = us * 1e-6;
		if((now + t) > nav_) 
		{
			nav_ = now + t;
			if(mhNav_.busy())
				mhNav_.stop();
			mhNav_.start(t);
		}
	}
	u_int16_t	sta_seqno_;		// next seqno that I'll use
	u_int32_t	cw_;		// Contention Window
	inline void inc_cw() 
	{
		cw_ = (cw_ << 1) + 1;
		if(cw_ > phymib_->getCWMax())
			cw_ = phymib_->getCWMax();
	}
	inline void rst_cw() { cw_ = phymib_->getCWMin(); }

	void 	sendRTS(int dst);
	void	sendCTS(int dst, double duration);
	void	sendACK(int dst);
	void	sendDATA(Packet *p);
	inline void transmit(Packet *p, double timeout);
	void	RetransmitRTS();
	void	RetransmitDATA();

	void	recvRTS(Packet *p);
	void	recvCTS(Packet *p);
	void	recvACK(Packet *p);
	//void	recvDATA(Packet *p);

	//void		rx_resume(void);
	void		tx_resume(void);

	int		check_pktCTRL();
	int		check_pktRTS();
	int		check_pktTx();
	inline void checkBackoffTimer(void);
	inline void setRxState(MacState newState);
	inline void setTxState(MacState newState);
public:
	int line_num=0;
	int column_num=0;
	int slot_area_num=0;
	double distance_to_center;	//距离本蜂窝中心距离
	
	void calculate_cellular_area();
	inline int lineNum() { return line_num; }
	inline int columnNum() { return column_num; }
	inline int slotAreaNum() { return slot_area_num; }
	inline int DistanceToCenter() { return distance_to_center; }






	void    sendHeadHello();
	void 	recvHeadHello(Packet *p);
	MY_Neighbor*      nb_lookup(nsaddr_t id);
    MY_NeighborCluster* nc_lookup(nsaddr_t id);
    void            nb_insert(nsaddr_t id);     
    void            nc_insert(nsaddr_t id);
};
#define RADIUS (250*2/3.0)		//定义蜂窝的半径大小，通信半径的2/3     
#define unitx (sqrt(3)*RADIUS)	//定义蜂窝横坐标单位
#define unity (3.0/2*RADIUS)	//定义蜂窝纵坐标单位
//#WQC HC
/*typedef struct POINT_XY
{
	double x;
	double y;
}POINT_XY;*/
#define SLOT_TIME 0.01   //时隙间隔
#define DELAY_TIME 0.00002
//double delay_time=0.00002;
//double MYMac::slot_time= 0;
#endif  //__MYMac__
