# MPEG Transport Stream Continuity Counters Error Detector

# Installation
```
git clone https://github.com/SmurfManX/mpegts_cc_error_detector.git
cd mpegts_cc_error_detector
make
make install 
```

# Usage
```
Usage: ccdetect [-m <mcast>] [-p <port>] [-o <file>] [-r] [-h]
    -m <mcast>     # multicast address of channel
    -p <port>      # port of channel
    -o <file>      # output file path
    -r             # if stream is RTP
    -h             # prints help (this message)
```
# Console Log 
```
================================================================
[root@server ~]# ccdetect -m 224.0.0.1 -p 1234 -o file.log
2021-02-22 18:52:10.31 - [I] Starting reception...
2021-02-22 18:52:10.33 - [I] Started multicast stream
2021-02-22 18:52:10.36 - [I] CC ERROR: PID(100), MUSTBE(12), RECEIVED(3)
2021-02-22 18:52:10.40 - [I] CC ERROR: PID(100), MUSTBE(14), RECEIVED(5)
2021-02-22 18:52:10.41 - [I] CC ERROR: PID(0), MUSTBE(7), RECEIVED(9)
2021-02-22 18:52:10.41 - [I] CC ERROR: PID(32), MUSTBE(7), RECEIVED(9)
2021-02-22 18:52:10.43 - [I] CC ERROR: PID(100), MUSTBE(12), RECEIVED(3)
2021-02-22 18:52:10.44 - [I] CC ERROR: PID(100), MUSTBE(13), RECEIVED(4)
2021-02-22 18:52:10.45 - [I] CC ERROR: PID(100), MUSTBE(11), RECEIVED(1)
2021-02-22 18:52:10.45 - [I] CC ERROR: PID(200), MUSTBE(13), RECEIVED(14)
2021-02-22 18:52:10.47 - [I] CC ERROR: PID(100), MUSTBE(1), RECEIVED(8)
2021-02-22 18:52:10.48 - [I] CC ERROR: PID(100), MUSTBE(15), RECEIVED(6)
================================================================
```

<a href="https://www.buymeacoffee.com/smurfmanx" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>
