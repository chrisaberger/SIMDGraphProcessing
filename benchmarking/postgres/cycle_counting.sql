\timing on
EXPLAIN SELECT COUNT(*) FROM edge e1, edge e2, edge e3, edge e4 WHERE e1.b = e2.a AND e2.b = e3.a AND e3.b = e4.b AND e1.a = e4.a;
SELECT COUNT(*) FROM edge e1, edge e2, edge e3, edge e4 WHERE e1.b = e2.a AND e2.b = e3.a AND e3.b = e4.b AND e1.a = e4.a;
\timing off
