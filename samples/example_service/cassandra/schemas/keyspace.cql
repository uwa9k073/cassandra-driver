drop keyspace if exists benchmark_ks;

create keyspace benchmark_ks with replication = { 'class': 'SimpleStrategy', 'replication_factor': 1 };
