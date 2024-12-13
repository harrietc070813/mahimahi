mahimahi: a web performance measurement toolkit

# Introduction
Two jitter attack modules have been added to this repo: `mm-bbr-attack` and `mm-copa-attack`. Both modules are designed and implemented based on the first attack model. For details of attack models, see [Design Doc](https://docs.google.com/document/d/13yn0OXKkXK3xg5d5hW8VVk_5SNbBZjEPgKadHVjYpUo/edit?tab=t.0#heading=h.wb3iy6yyoz31). Specifically, `mm-bbr-attack` executes jitter-based attack targeting BBR, while `mm-copa-attack` targets Copa.

## Usage
### Install
`./autogen.sh && ./configure && make && sudo make install`

### mm-bbr-attack
An example command: `mm-bbr-attack 1000 500 --attack-log=/mnt/ramdisk/test.log`

### mm-copa-attack
An example command: `mm-copa-attack 40 10 80 /mnt/ramdisk/test.uplink /mnt/ramdisk/test.downlink /mnt/ramdisk/test.uplink.attack /mnt/ramdisk/test.downlink.attack`

In this example, `test.uplink` contains packet arrival and departure information of the uplink (similar to the mm-link log), while `test.downlink` contains the information of the downlink. 

The log `test.uplink.attack` is also a uplink log. In `test.uplink.attack`, each line contains two values: a timestamp indicating when a packet is received, and the delay added to that packet when it is enqueued. Similarly, `test.downlink.attack` contains the downlink information.
