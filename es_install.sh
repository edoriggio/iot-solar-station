#!/bin/bash

# NOTE: Doesn't work out of the box, just notes

sudo sysctl -w vm.max_map_count=262144

# Create Docker Network
docker network create elastic

# Pull Elasticsearch Docker Image
docker pull docker.elastic.co/elasticsearch/elasticsearch:8.11.1


# Start Elasticsearch Container
docker run --name elastic --net elastic -p 9200:9200 -it -m 1GB -d docker.elastic.co/elasticsearch/elasticsearch:8.11.1

# Wait for Elasticsearch to start and generate password
echo "Waiting for Elasticsearch to start..."
sleep 60

# Reset and copy Elasticsearch password
ELASTIC_PASSWORD=$(docker exec elastic /usr/share/elasticsearch/bin/elasticsearch-reset-password -u elastic -b)
echo "Elasticsearch 'elastic' user password: $ELASTIC_PASSWORD"

# Create enrollment token for Kibana
ENROLLMENT_TOKEN=$(docker exec elastic /usr/share/elasticsearch/bin/elasticsearch-create-enrollment-token -s kibana)
echo "Enrollment token for Kibana: $ENROLLMENT_TOKEN"

# Copy SSL Certificate
docker cp elastic:/usr/share/elasticsearch/config/certs/http_ca.crt .

# Test Elasticsearch
curl --cacert http_ca.crt -u curl --cacert http_ca.crt -u elastic:1XIWZwil0yI5zNPm6gpJ https://localhost:9200

# Kibana
docker pull docker.elastic.co/kibana/kibana:8.11.1
docker run --name kibana --net elastic -p 5601:5601 -d docker.elastic.co/kibana/kibana:8.11.1