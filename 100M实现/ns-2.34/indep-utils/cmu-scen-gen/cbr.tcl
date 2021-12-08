#
# nodes: 30, max conn: 15, send rate: 0.10000000000000001, seed: 10
#
#
# 1 connecting to 2 at time 25.561746128630705
#
set udp_(0) [new Agent/UDP]
$ns_ attach-agent $node_(1) $udp_(0)
set null_(0) [new Agent/Null]
$ns_ attach-agent $node_(2) $null_(0)
set cbr_(0) [new Application/Traffic/CBR]
$cbr_(0) set packetSize_ 512
$cbr_(0) set interval_ 0.10000000000000001
$cbr_(0) set random_ 1
$cbr_(0) set maxpkts_ 10000
$cbr_(0) attach-agent $udp_(0)
$ns_ connect $udp_(0) $null_(0)
$ns_ at 25.561746128630705 "$cbr_(0) start"
#
# 2 connecting to 3 at time 74.10354682901108
#
set udp_(1) [new Agent/UDP]
$ns_ attach-agent $node_(2) $udp_(1)
set null_(1) [new Agent/Null]
$ns_ attach-agent $node_(3) $null_(1)
set cbr_(1) [new Application/Traffic/CBR]
$cbr_(1) set packetSize_ 512
$cbr_(1) set interval_ 0.10000000000000001
$cbr_(1) set random_ 1
$cbr_(1) set maxpkts_ 10000
$cbr_(1) attach-agent $udp_(1)
$ns_ connect $udp_(1) $null_(1)
$ns_ at 74.10354682901108 "$cbr_(1) start"
#
# 2 connecting to 4 at time 178.47412263903493
#
set udp_(2) [new Agent/UDP]
$ns_ attach-agent $node_(2) $udp_(2)
set null_(2) [new Agent/Null]
$ns_ attach-agent $node_(4) $null_(2)
set cbr_(2) [new Application/Traffic/CBR]
$cbr_(2) set packetSize_ 512
$cbr_(2) set interval_ 0.10000000000000001
$cbr_(2) set random_ 1
$cbr_(2) set maxpkts_ 10000
$cbr_(2) attach-agent $udp_(2)
$ns_ connect $udp_(2) $null_(2)
$ns_ at 178.47412263903493 "$cbr_(2) start"
#
# 9 connecting to 10 at time 77.022611916541408
#
set udp_(3) [new Agent/UDP]
$ns_ attach-agent $node_(9) $udp_(3)
set null_(3) [new Agent/Null]
$ns_ attach-agent $node_(10) $null_(3)
set cbr_(3) [new Application/Traffic/CBR]
$cbr_(3) set packetSize_ 512
$cbr_(3) set interval_ 0.10000000000000001
$cbr_(3) set random_ 1
$cbr_(3) set maxpkts_ 10000
$cbr_(3) attach-agent $udp_(3)
$ns_ connect $udp_(3) $null_(3)
$ns_ at 77.022611916541408 "$cbr_(3) start"
#
# 12 connecting to 13 at time 42.581176470304456
#
set udp_(4) [new Agent/UDP]
$ns_ attach-agent $node_(12) $udp_(4)
set null_(4) [new Agent/Null]
$ns_ attach-agent $node_(13) $null_(4)
set cbr_(4) [new Application/Traffic/CBR]
$cbr_(4) set packetSize_ 512
$cbr_(4) set interval_ 0.10000000000000001
$cbr_(4) set random_ 1
$cbr_(4) set maxpkts_ 10000
$cbr_(4) attach-agent $udp_(4)
$ns_ connect $udp_(4) $null_(4)
$ns_ at 42.581176470304456 "$cbr_(4) start"
#
# 13 connecting to 14 at time 169.44993819550143
#
set udp_(5) [new Agent/UDP]
$ns_ attach-agent $node_(13) $udp_(5)
set null_(5) [new Agent/Null]
$ns_ attach-agent $node_(14) $null_(5)
set cbr_(5) [new Application/Traffic/CBR]
$cbr_(5) set packetSize_ 512
$cbr_(5) set interval_ 0.10000000000000001
$cbr_(5) set random_ 1
$cbr_(5) set maxpkts_ 10000
$cbr_(5) attach-agent $udp_(5)
$ns_ connect $udp_(5) $null_(5)
$ns_ at 169.44993819550143 "$cbr_(5) start"
#
# 16 connecting to 17 at time 15.633268074893051
#
set udp_(6) [new Agent/UDP]
$ns_ attach-agent $node_(16) $udp_(6)
set null_(6) [new Agent/Null]
$ns_ attach-agent $node_(17) $null_(6)
set cbr_(6) [new Application/Traffic/CBR]
$cbr_(6) set packetSize_ 512
$cbr_(6) set interval_ 0.10000000000000001
$cbr_(6) set random_ 1
$cbr_(6) set maxpkts_ 10000
$cbr_(6) attach-agent $udp_(6)
$ns_ connect $udp_(6) $null_(6)
$ns_ at 15.633268074893051 "$cbr_(6) start"
#
# 18 connecting to 19 at time 166.15229858372001
#
set udp_(7) [new Agent/UDP]
$ns_ attach-agent $node_(18) $udp_(7)
set null_(7) [new Agent/Null]
$ns_ attach-agent $node_(19) $null_(7)
set cbr_(7) [new Application/Traffic/CBR]
$cbr_(7) set packetSize_ 512
$cbr_(7) set interval_ 0.10000000000000001
$cbr_(7) set random_ 1
$cbr_(7) set maxpkts_ 10000
$cbr_(7) attach-agent $udp_(7)
$ns_ connect $udp_(7) $null_(7)
$ns_ at 166.15229858372001 "$cbr_(7) start"
#
# 19 connecting to 20 at time 71.682518502549499
#
set udp_(8) [new Agent/UDP]
$ns_ attach-agent $node_(19) $udp_(8)
set null_(8) [new Agent/Null]
$ns_ attach-agent $node_(20) $null_(8)
set cbr_(8) [new Application/Traffic/CBR]
$cbr_(8) set packetSize_ 512
$cbr_(8) set interval_ 0.10000000000000001
$cbr_(8) set random_ 1
$cbr_(8) set maxpkts_ 10000
$cbr_(8) attach-agent $udp_(8)
$ns_ connect $udp_(8) $null_(8)
$ns_ at 71.682518502549499 "$cbr_(8) start"
#
# 22 connecting to 23 at time 139.2243019860351
#
set udp_(9) [new Agent/UDP]
$ns_ attach-agent $node_(22) $udp_(9)
set null_(9) [new Agent/Null]
$ns_ attach-agent $node_(23) $null_(9)
set cbr_(9) [new Application/Traffic/CBR]
$cbr_(9) set packetSize_ 512
$cbr_(9) set interval_ 0.10000000000000001
$cbr_(9) set random_ 1
$cbr_(9) set maxpkts_ 10000
$cbr_(9) attach-agent $udp_(9)
$ns_ connect $udp_(9) $null_(9)
$ns_ at 139.2243019860351 "$cbr_(9) start"
#
# 23 connecting to 24 at time 8.8952016219940049
#
set udp_(10) [new Agent/UDP]
$ns_ attach-agent $node_(23) $udp_(10)
set null_(10) [new Agent/Null]
$ns_ attach-agent $node_(24) $null_(10)
set cbr_(10) [new Application/Traffic/CBR]
$cbr_(10) set packetSize_ 512
$cbr_(10) set interval_ 0.10000000000000001
$cbr_(10) set random_ 1
$cbr_(10) set maxpkts_ 10000
$cbr_(10) attach-agent $udp_(10)
$ns_ connect $udp_(10) $null_(10)
$ns_ at 8.8952016219940049 "$cbr_(10) start"
#
# 27 connecting to 28 at time 157.39743432840214
#
set udp_(11) [new Agent/UDP]
$ns_ attach-agent $node_(27) $udp_(11)
set null_(11) [new Agent/Null]
$ns_ attach-agent $node_(28) $null_(11)
set cbr_(11) [new Application/Traffic/CBR]
$cbr_(11) set packetSize_ 512
$cbr_(11) set interval_ 0.10000000000000001
$cbr_(11) set random_ 1
$cbr_(11) set maxpkts_ 10000
$cbr_(11) attach-agent $udp_(11)
$ns_ connect $udp_(11) $null_(11)
$ns_ at 157.39743432840214 "$cbr_(11) start"
#
# 27 connecting to 29 at time 83.269324332135412
#
set udp_(12) [new Agent/UDP]
$ns_ attach-agent $node_(27) $udp_(12)
set null_(12) [new Agent/Null]
$ns_ attach-agent $node_(29) $null_(12)
set cbr_(12) [new Application/Traffic/CBR]
$cbr_(12) set packetSize_ 512
$cbr_(12) set interval_ 0.10000000000000001
$cbr_(12) set random_ 1
$cbr_(12) set maxpkts_ 10000
$cbr_(12) attach-agent $udp_(12)
$ns_ connect $udp_(12) $null_(12)
$ns_ at 83.269324332135412 "$cbr_(12) start"
#
# 28 connecting to 29 at time 21.653521415616162
#
set udp_(13) [new Agent/UDP]
$ns_ attach-agent $node_(28) $udp_(13)
set null_(13) [new Agent/Null]
$ns_ attach-agent $node_(29) $null_(13)
set cbr_(13) [new Application/Traffic/CBR]
$cbr_(13) set packetSize_ 512
$cbr_(13) set interval_ 0.10000000000000001
$cbr_(13) set random_ 1
$cbr_(13) set maxpkts_ 10000
$cbr_(13) attach-agent $udp_(13)
$ns_ connect $udp_(13) $null_(13)
$ns_ at 21.653521415616162 "$cbr_(13) start"
#
# 28 connecting to 30 at time 105.11998450621961
#
set udp_(14) [new Agent/UDP]
$ns_ attach-agent $node_(28) $udp_(14)
set null_(14) [new Agent/Null]
$ns_ attach-agent $node_(30) $null_(14)
set cbr_(14) [new Application/Traffic/CBR]
$cbr_(14) set packetSize_ 512
$cbr_(14) set interval_ 0.10000000000000001
$cbr_(14) set random_ 1
$cbr_(14) set maxpkts_ 10000
$cbr_(14) attach-agent $udp_(14)
$ns_ connect $udp_(14) $null_(14)
$ns_ at 105.11998450621961 "$cbr_(14) start"
#
#Total sources/connections: 12/15
#
