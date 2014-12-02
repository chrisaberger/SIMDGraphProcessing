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

DROP VIEW frontier1;
DROP VIEW frontier2;
DROP VIEW frontier3;
DROP VIEW visited1;
DROP VIEW visited2;
DROP VIEW visited3;

\timing
-- Path counting
CREATE VIEW frontier1 AS SELECT e1.dest AS v FROM edges e1 JOIN attributes a1 ON e1.dest = a1.node AND a1.attribute > 500 WHERE e1.source = 222074 AND e1.year = 2012;
CREATE VIEW visited1 AS SELECT 222074 UNION SELECT * FROM frontier1;
CREATE VIEW frontier2 AS SELECT e2.dest AS v FROM frontier1 JOIN edges e2 ON frontier1.v = e2.source JOIN attributes a2 ON e2.dest = a2.node AND a2.attribute > 500 WHERE e2.dest NOT IN (SELECT * FROM visited1) AND e2.year = 2012;
CREATE VIEW visited2 AS SELECT * FROM visited1 UNION SELECT * FROM frontier2;
CREATE VIEW frontier3 AS SELECT e3.dest AS v FROM frontier2 JOIN edges e3 ON frontier2.v = e3.source JOIN attributes a3 ON e3.dest = a3.node AND a3.attribute > 500 WHERE e3.dest NOT IN (SELECT * FROM visited2) AND e3.year = 2012;
CREATE VIEW visited3 AS SELECT * FROM visited2 UNION SELECT * FROM frontier3;
SELECT COUNT(DISTINCT e4.dest) FROM frontier3 JOIN edges e4 ON frontier3.v = e4.source JOIN attributes a4 ON e4.dest = a4.node AND a4.attribute > 500 WHERE e4.dest NOT IN (SELECT * FROM visited3) AND e4.year = 2012;
\timing
