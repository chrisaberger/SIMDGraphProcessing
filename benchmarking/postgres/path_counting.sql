\timing on

--SELECT COUNT(*) FROM edge e1, edge e2, edge e3, edge e4 WHERE
--   e1.a = 222074 AND e1.b = e2.a AND e2.b = e3.a AND e3.b = e4.a AND
--   e1.a != e2.a AND e1.a != e3.a AND e1.a != e4.a AND e1.a != e4.b AND
--   e2.a != e3.a AND e2.a != e4.a AND e2.a != e4.b AND
--   e3.a != e4.a AND e3.a != e4.b AND
--   e4.a != e4.b;

SELECT e.b FROM edge e WHERE
   e.a = 88160 ORDER BY e.b;

SELECT e2.b FROM edge e1, edge e2 WHERE
   e1.a = 88160 AND e1.b = e2.a AND e1.a != e2.b ORDER BY e2.b;
\timing off
