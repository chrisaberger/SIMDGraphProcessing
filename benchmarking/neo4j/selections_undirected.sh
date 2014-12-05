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
CREATE INDEX ON :AwesomeNode(location);
CREATE INDEX ON :AwesomeEdge(year);
USING PERIODIC COMMIT LOAD CSV FROM 'file://$1/attributes.txt' AS line FIELDTERMINATOR ' ' CREATE (node:AwesomeNode { node_id: toInt(line[0]), location: toInt(line[1]) });
EOF

${NEO4J_HOME}/bin/neo4j-shell << EOF
USING PERIODIC COMMIT LOAD CSV FROM 'file://$1/glab_undirected/data.txt' AS line FIELDTERMINATOR ' ' MERGE (fromNode:AwesomeNode { node_id: toInt(line[0]) }) MERGE (toNode:AwesomeNode { node_id: toInt(line[1]) }) CREATE (fromNode)-[:AwesomeEdge { year: toInt(substring(line[2], 0, 4)) }]->(toNode);
EOF

${NEO4J_HOME}/bin/neo4j-shell

echo "Triangle counting"
${NEO4J_HOME}/bin/neo4j-shell << EOF
MATCH (n:AwesomeNode)-[:AwesomeEdge { year: 2012 }]->(n2:AwesomeNode)-[:AwesomeEdge { year: 2012 }]->(n3:AwesomeNode)<-[:AwesomeEdge { year: 2012 }]-(n) WHERE n.location > 500 AND n2.location > 500 AND n3.location > 500 RETURN COUNT(n);
EOF

echo "Clique counting"
${NEO4J_HOME}/bin/neo4j-shell << EOF
MATCH (n:AwesomeNode)-[:AwesomeEdge { year: 2012 }]->(n2:AwesomeNode)-[:AwesomeEdge { year: 2012 }]->(n3:AwesomeNode)-[:AwesomeEdge { year: 2012 }]->(n4:AwesomeNode)<-[:AwesomeEdge { year: 2012 }]-(n) WHERE n.location > 500 AND n2.location > 500 AND n3.location > 500 AND n4.location > 500 AND n-[:AwesomeEdge { year: 2012 }]->n3 AND n2-[:AwesomeEdge { year: 2012 }]->n4 RETURN COUNT(n);
EOF

echo "Cycle counting"
${NEO4J_HOME}/bin/neo4j-shell << EOF
MATCH (n:AwesomeNode)-[:AwesomeEdge { year: 2012 }]->(n2:AwesomeNode)-[:AwesomeEdge { year: 2012 }]->(n3:AwesomeNode)-[:AwesomeEdge { year: 2012 }]->(n4:AwesomeNode)<-[:AwesomeEdge { year: 2012 }]-(n) WHERE n.location > 500 AND n2.location > 500 AND n3.location > 500 AND n4.location > 500 RETURN COUNT(n);
EOF
