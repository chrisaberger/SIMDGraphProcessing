from py2neo import Graph, Node
from py2neo.batch import CreateNodeJob, Batch

import time
import sys
import os
from sets import Set

nodefile = "nodes.txt"

graph = Graph()
graph.delete_all()

print "Reading input file"
nodes = Set()
edges = []
filename = sys.argv[1]
with open(filename) as f:
    for line in f:
        edge_strs = line.split(" ")
        edge_from, edge_to = int(edge_strs[0]), int(edge_strs[1])
        nodes.add(edge_from)
        nodes.add(edge_to)
        edges.append([edge_from, edge_to])

print "Nodes: " + str(len(nodes))
print "Edges: " + str(len(edges))

print "Writing node file"
with open(nodefile, "w") as f:
    for n in nodes:
        f.write(str(n) + "\n")

print "Inserting nodes"
full_path = os.path.abspath(nodefile)
# graph.cypher.execute("LOAD CSV FROM 'file://" + full_path + "' AS line CREATE (:AwesomeNode { node_id: toInt(line[0]) })")
# graph.cypher.execute("CREATE CONSTRAINT ON (node:AwesomeNode) ASSERT node.node_id IS UNIQUE")

print "Inserting edges"
graph.cypher.execute("CREATE CONSTRAINT ON (node:AwesomeNode) ASSERT node.node_id IS UNIQUE")
query = "USING PERIODIC COMMIT LOAD CSV FROM 'file://" + filename + "' AS line FIELDTERMINATOR ' ' MERGE (fromNode:AwesomeNode { node_id: toInt(line[0]) }) MERGE (toNode:AwesomeNode { node_id: toInt(line[1]) }) CREATE (fromNode)-[:AwesomeEdge]->(toNode)"
graph.cypher.execute(query)


# CYPHER queries
# Slow:
# MATCH (node:AwesomeNode {node_id: 88160}), (node2:AwesomeNode), p=shortestPath((node)-[*..2]-(node2)) WHERE length(p) = 2 RETURN DISTINCT node2 ORDER BY node2.node_id;

# Fast:
# MATCH (node:AwesomeNode {node_id: 88160})-[*2]-(node2) RETURN COUNT(DISTINCT node2);
