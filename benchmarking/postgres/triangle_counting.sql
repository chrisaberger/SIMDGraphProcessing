\timing on
SELECT COUNT(*) FROM edge e1 JOIN edge e2 ON e1.b = e2.a JOIN edge e3 ON e2.b = e3.b AND e1.a = e3.a;
\timing off
