## The C++ Asynchronous Apache Cassandra Driver based on Framework Userver

## Source codes and service templates at github
Samples and the source codes of the driver itself are available at the
[cassandra-driver at github](https://github.com/uwa9k073/cassandra-driver)

Samples and the source codes of the userver framework are available at the
[userver-framework at github](https://github.com/userver-framework/).

## Introduction
* @ref docs/driver_comparison.md

@anchor tutorial_services
## Tutorial

Start here if you are new to the Cassandra driver for userver. The tutorials
below walk you through everything from initial setup to building a complete
service.

1. @ref docs/tutorial/component.md — Setting up the Cassandra component,
   configuring connection pools, and providing secure node credentials.

2. @ref docs/tutorial/session.md — Executing CQL queries, choosing consistency
   levels, using command controls, and running batch operations.

3. @ref docs/tutorial/result_set.md — Extracting typed results from queries,
   handling nulls, and iterating over multiple rows.

4. @ref docs/tutorial/supported_data_types.md — Mapping between Cassandra
   native types and the corresponding C++ types supported by the driver.

5. @ref docs/tutorial/example_service.md — A complete, runnable microservice
   example that ties all of the above concepts together.


## Apache Cassandra


## Opensource
* Distributed under [Apache-2.0 License](http://www.apache.org/licenses/LICENSE-2.0)