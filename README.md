jsonrpc-c
=========

JSON-RPC in C (server only for now)

What?
-----
A library for a C program to receive JSON-RPC requests on tcp sockets (no HTTP).

Free software, MIT license.

Why?
----
I needed a way for an application written in C, running on an embedded Linux system to be configured by
a Java/Swing configuration tool running on a connected laptop. Wanted something simple, human readable,
and saw no need for HTTP.

How?
----
Includes cJSON (with a small patch on my fork).
No further dependencies.

Limitation?
----
Jrpc-c use select. It's limited by his 1024 (by default) file descriptor.
4 files descriptors is reserved by the server and it uses 1 per connection.

###Testing

Run `autoreconf -i`  before `./configure` and `make`

Test the example server by running it and typing: 

`echo '{"jsonrpc":"2.0", "method":"sayHello", "params":["sam"], "id":null}' | nc localhost 1234`

or

`echo '{"jsonrpc":"2.0", "method":"exit", "id":null}' | nc localhost 1234`
`echo "{\"method\":\"exit\"}" | nc localhost 1234`
