# TarantulaDB
A simple database for storing data.

## License
GPL-3.0

TanatulaDB is an open source database written in C.  It's licensed under the GPL-3.0 license, which means you can use it in any project that you want.

Goals are for this to be lightweight, store data in json files, exhibit behaviors of a stream of events and a current state, and have a simple API.

Connectors will be available for many database clients.

It is built specifically to meet requirements for monitoring and control of solar systems and remote robotics projects off grid on the Tarantula Flats property in the Mojave desert of Southern California.

It's inspiration comes from a desire to learn more about sockets, distributed databases, and cloud technologies, as well as meeting short term performance and reliability needs in a remote low power environment.

See DesignGoals.md for more details.

To use, run make from the client and server directories.  The server will start up on port 2345 by default.