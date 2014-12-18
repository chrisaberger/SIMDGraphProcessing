CREATE TEMPORARY VIEW frontier1 AS SELECT e1.b AS v FROM edge e1 WHERE e1.a = :start_node;
CREATE TEMPORARY VIEW visited1 AS SELECT :start_node UNION SELECT * FROM frontier1;
CREATE TEMPORARY VIEW frontier2 AS SELECT e2.b AS v FROM frontier1 JOIN edge e2 ON frontier1.v = e2.a WHERE e2.b NOT IN (SELECT * FROM visited1);
CREATE TEMPORARY VIEW visited2 AS SELECT * FROM visited1 UNION SELECT * FROM frontier2;
CREATE TEMPORARY VIEW frontier3 AS SELECT e3.b AS v FROM frontier2 JOIN edge e3 ON frontier2.v = e3.a WHERE e3.b NOT IN (SELECT * FROM visited2);
CREATE TEMPORARY VIEW visited3 AS SELECT * FROM visited2 UNION SELECT * FROM frontier3;
\timing on
SELECT COUNT(DISTINCT e4.b) FROM frontier3 JOIN edge e4 ON frontier3.v = e4.a WHERE e4.b NOT IN (SELECT * FROM visited3);
-- SELECT * FROM frontier1;
-- SELECT * FROM frontier2;
-- SELECT * FROM frontier3;
\timing off
