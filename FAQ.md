# FAQ

1. If you are finding that the client connecting to raveloxmidi is reporting issues trying to connect to an IPv6 address and you do NOT have IPv6 networking, you need to edit /etc/avahi/avahi-daemon.conf and set:
```
use-ipv6=no
publish-aaaa-on-ipv4=no
```
Obviously, if you want to use IPv6, both those options should be set to yes
You will need to restart Avahi for those changes to be picked up.
