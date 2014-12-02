-- Create a resource pool with the number of threads given as an argument
DROP RESOURCE POOL bench;
CREATE RESOURCE POOL bench EXECUTIONPARALLELISM :num_threads PLANNEDCONCURRENCY 1;
SET SESSION RESOURCE_POOL = bench;

DROP TABLE edges;
CREATE TABLE edges (source INT NOT NULL, dest INT NOT NULL) SEGMENTED BY HASH(source,dest) ALL NODES;

copy edges from :file direct delimiter ' ';

DROP VIEW frontier1;
DROP VIEW frontier2;
DROP VIEW frontier3;
DROP VIEW visited1;
DROP VIEW visited2;
DROP VIEW visited3;

\timing
-- Path counting
CREATE VIEW frontier1 AS SELECT e1.dest AS v FROM edges e1 WHERE e1.source = 222074;
CREATE VIEW visited1 AS SELECT 222074 UNION SELECT * FROM frontier1;
CREATE VIEW frontier2 AS SELECT e2.dest AS v FROM frontier1 JOIN edges e2 ON frontier1.v = e2.source WHERE e2.dest NOT IN (SELECT * FROM visited1);
CREATE VIEW visited2 AS SELECT * FROM visited1 UNION SELECT * FROM frontier2;
CREATE VIEW frontier3 AS SELECT e3.dest AS v FROM frontier2 JOIN edges e3 ON frontier2.v = e3.source WHERE e3.dest NOT IN (SELECT * FROM visited2);
CREATE VIEW visited3 AS SELECT * FROM visited2 UNION SELECT * FROM frontier3;
SELECT COUNT(DISTINCT e4.dest) FROM frontier3 JOIN edges e4 ON frontier3.v = e4.source WHERE e4.dest NOT IN (SELECT * FROM visited3);
\timing
