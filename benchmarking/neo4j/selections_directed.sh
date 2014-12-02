#!/bin/bash

echo "Nuking existing data"
${NEO4J_HOME}/bin/neo4j stop
old_pwd=`pwd`
cd ${NEO4J_HOME} && rm -rf data/*
cd $old_pwd
${NEO4J_HOME}/bin/neo4j start

echo "Loading data"
${NEO4J_HOME}/bin/neo4j-shell << EOF
CREATE CONSTRAINT ON (node:AwesomeNode) ASSERT node.node_id IS UNIQUE;
USING PERIODIC COMMIT LOAD CSV FROM 'file://$1/attributes.txt' AS line FIELDTERMINATOR ' ' CREATE (node:AwesomeNode { node_id: toInt(line[0]), location: toInt(line[1]) });
EOF

${NEO4J_HOME}/bin/neo4j-shell << EOF
USING PERIODIC COMMIT LOAD CSV FROM 'file://$1/glab_undirected/data.txt' AS line FIELDTERMINATOR ' ' MERGE (fromNode:AwesomeNode { node_id: toInt(line[0]) }) MERGE (toNode:AwesomeNode { node_id: toInt(line[1]) }) CREATE (fromNode)-[:AwesomeEdge { year: toInt(substring(line[2], 0, 4)) }]->(toNode);
EOF

echo "Path counting"
${NEO4J_HOME}/bin/neo4j-shell << EOF
MATCH (node:AwesomeNode {node_id: 88160})-[*2]->(node2) RETURN COUNT(DISTINCT node2);
EOF
