struct tcp_stats_without_InCsumErrors {
        count_t RtoAlgorithm;
        count_t RtoMin;
        count_t RtoMax;
        count_t MaxConn;
        count_t ActiveOpens;
        count_t PassiveOpens;
        count_t AttemptFails;
        count_t EstabResets;
        count_t CurrEstab;
        count_t InSegs;
        count_t OutSegs;
        count_t RetransSegs;
        count_t InErrs;
        count_t OutRsts;
};

struct icmpv4_stats_without_InCsumErrors {
	count_t InMsgs;
	count_t InErrors;
	count_t InDestUnreachs;
	count_t InTimeExcds;
	count_t InParmProbs;
	count_t InSrcQuenchs;
	count_t InRedirects;
	count_t InEchos;
	count_t InEchoReps;
	count_t InTimestamps;
	count_t InTimestampReps;
	count_t InAddrMasks;
	count_t InAddrMaskReps;
	count_t OutMsgs;
	count_t OutErrors;
	count_t OutDestUnreachs;
	count_t OutTimeExcds;
	count_t OutParmProbs;
	count_t OutSrcQuenchs;
	count_t OutRedirects;
	count_t OutEchos;
	count_t OutEchoReps;
	count_t OutTimestamps;
	count_t OutTimestampReps;
	count_t OutAddrMasks;
	count_t OutAddrMaskReps;
};

struct  netstat_wrong {
        struct ipv4_stats                        ipv4;
        struct icmpv4_stats_without_InCsumErrors icmpv4;
        struct udpv4_stats                       udpv4;

        struct ipv6_stats                        ipv6;
        struct icmpv6_stats                      icmpv6;
        struct udpv6_stats                       udpv6;

        struct tcp_stats_without_InCsumErrors  tcp;
};

