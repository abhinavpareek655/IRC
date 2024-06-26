# Internet Relay Chat Service – IRC: a distributed messaging service
server hosts a multitude of public communication channels on various topics. 
Client-user processes can be attached to them, which can exchange messages 
publicly over the channels as well as privately with each other. Each user 
process connecting to a server can see a list of available channels as well as a 
list of available users. In the former he will be able to connect and exchange 
public messages, in the latter he will be able to send private messages directly. 
The list of channels is a Critical Area that cannot be written to by multiple users 
at the same time and access to it is controlled by the server/moderator (with a 
centralized request/reply/release algorithm). A user can be registered in a 
channel with a unique username. The list of usernames per channel is also a 
Critical Area handled by the same protocol. 

Upon starting the user process, the user is prompted and selects a username. Two 
users cannot log in with the same username. However, if a user disconnects from 
the server, then his username is released and can be used by a future user. 

User processes connected to the server can execute the following 
commands (commands always start with a slash): 
i. /list-channels: see a list of available channels. 
ii. /list-users: see a list of online users. 
iii. /join [channel_name]: to join a channel. All channels must have the hashtag 
(#) as their first symbol. 
iv. /leave: to exit the channel they are connected to (otherwise /leave 
does nothing). 
v. /exit: to disconnect from the server. 
vi. /help: should print the list of the above commands. 
vii. Anything the user sends while connected to a channel should be sent to 
all other users in the channel, otherwise the message “command not 
found, try /help” should be printed. 

A channel is created when the first user joins and deleted when the last user 
leaves. Once a channel is removed, it can be re-created if another user joins it. 
Each user can connect to a maximum of one channel. 

As soon as the server process starts, it is "empty", i.e. it has no connected users 
and channels.
