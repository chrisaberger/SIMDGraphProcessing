\timing on
SELECT COUNT(*) FROM edge e1, edge e2, edge e3 WHERE e1.b = e2.a AND e2.b = e3.b AND e1.a = e3.a;
\timing off
