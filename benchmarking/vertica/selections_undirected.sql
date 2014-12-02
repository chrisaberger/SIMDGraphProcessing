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

\timing
-- Triangle counting
SELECT COUNT(*) FROM
      edges e1 JOIN attributes a1 ON e1.source = a1.node JOIN edges e2 ON e1.dest = e2.source JOIN attributes a2 ON e2.source = a2.node JOIN edges e3 ON e2.dest = e3.dest AND e3.source = e1.source JOIN attributes a3 ON e3.dest = a3.node
   WHERE
      e1.year = 2012 AND e2.year = 2012 AND e3.year = 2012 AND a1.attribute > 500 AND a2.attribute > 500 AND a3.attribute > 500;

-- Cycle counting
SELECT COUNT(*) FROM
      edges e1 JOIN attributes a1 ON e1.source = a1.node JOIN edges e2 ON e1.dest = e2.source JOIN attributes a2 ON e2.source = a2.node JOIN edges e3 ON e2.dest = e3.source JOIN attributes a3 ON e3.source = a3.node JOIN edges e4 ON e3.source = e4.dest AND e4.source = e1.source JOIN attributes a4 ON e4.dest = a4.node
   WHERE
      e1.year = 2012 AND e2.year = 2012 AND e3.year = 2012 AND e4.year = 2012 AND a1.attribute > 500 AND a2.attribute > 500 AND a3.attribute > 500 AND a4.attribute > 500;

-- Clique counting
SELECT COUNT(*) FROM
      edges e1 JOIN attributes a1 ON e1.source = a1.node JOIN edges e2 ON e1.dest = e2.source JOIN attributes a2 ON e2.source = a2.node JOIN edges e3 ON e2.dest = e3.source JOIN attributes a3 ON e3.source = a3.node JOIN edges e4 ON e3.source = e4.dest AND e4.source = e1.source JOIN attributes a4 ON e4.dest = a4.node
      JOIN edges e5 ON e1.source = e5.source AND e2.dest = e5.dest JOIN edges e6 ON e1.dest = e6.source AND e3.dest = e6.dest;
   WHERE
      e1.year = 2012 AND e2.year = 2012 AND e3.year = 2012 AND e4.year = 2012 AND e5.year = 2012 AND e6 = 2012 AND a1.attribute > 500 AND a2.attribute > 500 AND a3.attribute > 500 AND a4.attribute > 500;
\timing
