---
layout: page
title: fabtests(7)
tagline: Fabtests Programmer's Manual
---
{% include JB/setup %}

# NAME

Fabtests

# SYNOPSIS

Fabtests is a set of examples for fabric providers that demonstrates various features of libfabric- high-performance fabric software library.

# OVERVIEW
  
Libfabric defines sets of interface that fabric providers can support. The purpose of Fabtests examples is to demonstrate some of the major features. The goal is to familiarize users with different functionalities libfabric offers and how to use them. Although these tests report performance numbers, they are designed to test functionality and not performance.

The tests are divided into the following six categories. Except the unit tests all of them are client-server tests.

## Simple

These tests are a mix of very basic tests and tests that show major features of libfabric.

	fi_cq_data: A client-server example that tranfers CQ data
	fi_dgram: A basic DGRAM client-server example
	fi_dgram_waitset: A basic DGRAM client-server example that uses waitset
	fi_msg: A basic MSG client-server example
	fi_msg_sockets: Verifies transitioning a passive endpoint into an active one as required by sockets-over-RDMA implementations
	fi_poll: A basic RDM client-server example that uses poll
	fi_rdm: A basic RDM client-server example
	fi_rdm_rma_simple: A simple RDM client-sever RMA example
	fi_rdm_shared_ctx: An RDM client-server example that uses shared context
	fi_rdm_tagged_peek: An RDM client-server example that uses tagged FI_PEEK
	fi_scalable_ep: An RDM client-server example with scalable endpoints

## Pingpong

The client and the server exchange messages in a ping-pong manner for various messages sizes.

	fi_msg_pingpong: A ping-pong client-server example using MSG endpoints
	fi_rdm_pingpong: A ping pong client-server example using RDM endpoints
	fi_rdm_cntr_pingpong: An RDM ping pong client-server using counters
	fi_rdm_inject_pingpong: An RDM ping pong client-server example using inject
	fi_rdm_tagged_pingpong: A ping pong client-server example using tagged messages
	fi_ud_pingpong: A ping-pong client-server example using DGRAM endpoints

## Streaming

These are one way streaming data tests.

	fi_msg_rma: A streaming client-server example using RMA operations between MSG endpoints
	fi_rdm_rma: A streaming client-server example using RMA operations
	fi_rdm_atomic: An RDM streaming client-server using atomic operations
	fi_rdm_multi_recv: An RDM streaming client-server example using multi recv buffer

## Unit
	 fi_eq_test: Unit tests for evet queue
	 fi_dom_test: Unit tests for domain
	 fi_av_test: Unit tests for address vector
	 fi_size_left_test: Unit tests to query the lower bound of rx/tx entries

## Ported
	 fi_cmatose: A librdmacm client-server example
	 fi_rc_pingpong: A libibverbs ping pong client-server example

## Complex / Ubertest

These are comprehensive latency and bandwidth tests that can handle a variety of test configurations.

	fi_ubertest: This single test binary takes as input a test configuration file to run different tests.
		     Example test configurations are at complex/test_configs.

# HOW TO RUN TESTS
(1) Fabtests requires that libfabric be installed on the system, and at least one provider be usable.

(2) Install fabtests on the system. By default all the test executables are installed in /usr/bin directory unless specified otherwise.

(3) All the client-server tests have the following usage model:

	./fi_<testname> [OPTIONS]       start server
	./fi_<testname> <host>     	connect to server

# OPTIONS

*-f <provider_name>*
: The name of the underlying fabric provider e.g. sockets, verbs, psm etc. If the provider name is not provided, the test will pick one from the list of the available providers it finds by fi_getinfo call.

*-n <domain>*
: The name of the the specific domain to be used.

*-b <src_port>*
: The non-default source port number of the endpoint.

*-p <dest_port>*
: The non-default destination port number of the endpoint.

*-s <src_addr>*
: The source address.

*-I <iter>*
: Number of iterations of the test will run.

*-S <msg_size>*
: The specific size of the message in bytes the test will use or 'all' to run all the default sizes.

*-o <op_type>*
: The operation to be performed in the test. For atomic examples, selected operations are min, max, read, write, cswap, and all (all performs all five selected operations). For RMA examples, selected operations are read, write, and writedata.

*-m*
: Enables machine readable output.

*-i*
: Prints hints structure and exits.

*-h*
: Displays help output for the test.

# USAGE EXAMPLES

1. A simple example:

	run server: ./<test_name> -f <provider_name> -s <source_addr>
		e.g.	./fi_msg_rma -f sockets -s 192.168.0.123
	run client: ./<test_name> <server_addr> -f <provider_name>
		e.g.	./fi_msg_rma 192.168.0.123 -f sockets

2. An example with various options:

	run server: ./fi_rdm_atomic -f psm -s 192.168.0.123 -I 1000 -S 1024
	run client: ./fi_rdm_atomic 192.168.0.123 -f psm -I 1000 -S 1024

This will run the RDM example "fi_rdm_atomic" with

	- PSM provider
	- 1000 iterations
	- 1024 bytes message size

3. fi_ubertest:
	run server: e.g. ./fi_ubertest
	run client: ./fi_ubertest -f <config_file> <server>
		    e.g. ./fi_ubertest -f complex/test_configs/sockets.json 192.168.0.123
