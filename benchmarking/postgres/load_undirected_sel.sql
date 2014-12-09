DROP TABLE IF EXISTS edge;
DROP TABLE IF EXISTS node;
DROP TABLE IF EXISTS edgeRaw;
DROP TABLE IF EXISTS nodeRaw;

CREATE TABLE edgeRaw(a BIGINT, b BIGINT, year TIMESTAMP, PRIMARY KEY(a, b));
CREATE TABLE edge(a BIGINT, b BIGINT, year INT, PRIMARY KEY(a, b));
CREATE TABLE nodeRaw(a BIGINT, place INT, PRIMARY KEY(a));
CREATE TABLE node(a BIGINT, place INT, PRIMARY KEY(a));

COPY edgeRaw FROM :'edgelist' WITH DELIMITER ' ';
COPY nodeRaw FROM :'attributes' WITH DELIMITER ' ';
 
INSERT INTO edge(a, b, year) (
   SELECT DISTINCT LEAST(a, b), GREATEST(a, b), EXTRACT(YEAR FROM year) FROM edgeRaw
);

INSERT INTO node(a, place) (
   SELECT a, place FROM nodeRaw
);

CREATE INDEX index_edge_a ON edge(a);
CREATE INDEX index_edge_b ON edge(b);
CREATE INDEX index_edge_year ON edge(year);
CREATE INDEX index_node_a ON node(a);

ANALYZE VERBOSE node;
ANALYZE VERBOSE edge;
