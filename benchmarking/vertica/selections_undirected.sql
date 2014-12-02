-- Create a resource pool with the number of threads given as an argument
DROP RESOURCE POOL bench;
CREATE RESOURCE POOL bench EXECUTIONPARALLELISM :num_threads PLANNEDCONCURRENCY 1;
SET SESSION RESOURCE_POOL = bench;

DROP TABLE attributes;
DROP TABLE edges;
DROP TABLE edgesRaw;
CREATE TABLE attributes (node INT NOT NULL, attribute INT NOT NULL) SEGMENTED BY HASH(node) ALL NODES;
CREATE TABLE edges (source INT NOT NULL, dest INT NOT NULL, year INT NOT NULL) SEGMENTED BY HASH(source,dest) ALL NODES;
CREATE TABLE edgesRaw (source INT NOT NULL, dest INT NOT NULL, tstamp TIMESTAMP NOT NULL) SEGMENTED BY HASH(source,dest) ALL NODES;

COPY edgesRaw FROM :edgelist DIRECT DELIMITER ' ';
COPY attributes FROM :attributes DIRECT DELIMITER ' ';

INSERT INTO edges SELECT e.source, e.dest, EXTRACT(YEAR FROM e.tstamp) AS year FROM edgesRaw e;

DROP VIEW trEdges;
DROP VIEW cyEdges;
DROP VIEW clEdges;

\timing
-- Triangle counting
CREATE VIEW trEdges AS
   SELECT
      e.source, e.dest FROM edges e, attributes a1, attributes a2
   WHERE a1.node = e.source AND a2.node = e.dest AND a1.attribute > 500 AND a2.attribute > 500 AND e.year = 2012;

SELECT
   COUNT(*)
FROM
   trEdges e1 JOIN trEdges e2 ON e1.dest = e2.source JOIN trEdges e3 ON e2.dest = e3.dest AND e3.source = e1.source;

-- Cycle counting
CREATE VIEW cyEdges AS
   SELECT
      e.source, e.dest FROM edges e, attributes a1, attributes a2
   WHERE a1.node = e.source AND a2.node = e.dest AND a1.attribute > 500 AND a2.attribute > 500 AND e.year = 2012;

SELECT
   COUNT(*)
FROM
   cyEdges e1 JOIN cyEdges e2 ON e1.dest = e2.source JOIN cyEdges e3 ON e2.dest = e3.source JOIN cyEdges e4 ON e3.source = e4.dest AND e4.source = e1.source;

-- Clique counting
CREATE VIEW clEdges AS
   SELECT
      e.source, e.dest FROM edges e, attributes a1, attributes a2
   WHERE a1.node = e.source AND a2.node = e.dest AND a1.attribute > 500 AND a2.attribute > 500 AND e.year = 2012;

SELECT COUNT(*) FROM
      clEdges e1 JOIN clEdges e2 ON e1.dest = e2.source JOIN clEdges e3 ON e2.dest = e3.source JOIN clEdges e4 ON e3.source = e4.dest AND e4.source = e1.source JOIN clEdges e5 ON e1.source = e5.source AND e2.dest = e5.dest JOIN clEdges e6 ON e1.dest = e6.source AND e3.dest = e6.dest;
\timing
