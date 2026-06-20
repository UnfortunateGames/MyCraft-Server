# MyC-Server

> [!WARNING]
> The current state of this project is broken, I've given up
> working on it, and I intend to rewrite this project in the
> future.

## About MyC-Server

This is a server for Minecraft **BETA** *1.8.1*.

"*Why did I settle on an OLD Minecraft version?*"

Good question, that's because I intended for it to be simple,
only a simple chunk to be sent, the client to move around.

And for the current `netty` version of the Minecraft server,
I was not taking that gamble on the modern servers, heck I'm
not even gonna allow you to use this because it's dangerous.

> Which at its current state, unusable anyway.

This is a simple C TCP server that completes a server process
from the client and doesn't mess up in a way.

## Why did I abandon this project

It's gotten so large, and the design, architectural state of this
current project is just so limiting because I've learned a lot
in programming during the time I was making this project.

This in turn, turned into a huge backfire, designs are literally
all over the place, some functions that should've been more
modular were made mutators because of my lack of knowledge at
the time.

Basically, to sum it up; I've outgrown the project so much, to the
point past me wrote such horrendous code to present me, that
it caused a lot of architectural problems. It desperately needs a
rewrite, but for my current schedule I can't.

## Known Bugs and Issues

### PACKET CREATION IS SCUFFED

Packet creation should be sent as they are created.

For example:

```c
void writeU16(int connfd, const char *restrict msg, const int packetid);

void insertU16(const char *restrict msg, void *restrict dest, const int idx);
```

### LOGIN RESPONSE IS BAD

The login process is currently damned to oblivion.

It's not deterministic, plus the assumptions are wrong.

### NO DESIGN PATTERNS ARE APPARENT

It's pretty self-explanatory

## Review

The process of making this was really fun! Even if what I say is
saying the opposite, the process of creating the features was fun,
nonetheless, it's actually fun making and learning how the packet
system worked was a blast.

I had to rewrite it 3 times, this is actually the third rebuild
of the project, because I learned a lot of philosophies, some
actual useful stuff on Youtube, special mention to [Tsoding](https://www.youtube.com/@Tsoding)
the man actually made me learn a bunch of useful stuff.

I just actually hope that this doesn't happen often to my projects
that I change so drastically, that I can't even work with my
project **I** made by hand.

Nonetheless, to summarize this whole Project, don't learn while
building, building is supposed to be a mark in your educational
work, not an advancement along with learning.
