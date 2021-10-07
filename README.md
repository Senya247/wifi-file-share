# wifi-file-share
Share fiels over wifi without specifying ip address

What this basically does is, the client sends a ping to the broadcast address with some headers, which include the file size and name. The server receives that and asks the user for confirmation.

only tested on ubuntu 21.04
