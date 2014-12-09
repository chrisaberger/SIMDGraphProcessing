\timing on
CREATE TEMPORARY VIEW filtered_edges AS SELECT e.a, e.b FROM edge e, node n1, node n2 WHERE e.a = n1.a AND e.b = n2.a AND e.year = 2012 AND n1.place > 500 AND n2.place > 500;
CREATE TEMPORARY VIEW frontier1 AS SELECT e1.b AS v FROM filtered_edges e1 WHERE e1.a = 14293652286639;
CREATE TEMPORARY VIEW visited1 AS SELECT 14293652286639 UNION SELECT * FROM frontier1;
CREATE TEMPORARY VIEW frontier2 AS SELECT e2.b AS v FROM frontier1 JOIN filtered_edges e2 ON frontier1.v = e2.a WHERE e2.b NOT IN (SELECT * FROM visited1);
CREATE TEMPORARY VIEW visited2 AS SELECT * FROM visited1 UNION SELECT * FROM frontier2;
CREATE TEMPORARY VIEW frontier3 AS SELECT e3.b AS v FROM frontier2 JOIN filtered_edges e3 ON frontier2.v = e3.a WHERE e3.b NOT IN (SELECT * FROM visited2);
CREATE TEMPORARY VIEW visited3 AS SELECT * FROM visited2 UNION SELECT * FROM frontier3;
SELECT COUNT(DISTINCT e4.b) FROM frontier3 JOIN filtered_edges e4 ON frontier3.v = e4.a WHERE e4.b NOT IN (SELECT * FROM visited3);
\timing off
